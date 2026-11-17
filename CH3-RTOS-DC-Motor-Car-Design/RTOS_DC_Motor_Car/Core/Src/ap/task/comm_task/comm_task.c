/*
 * comm_task.c
 *
 *  Created on: 2026. 11. 12.
 *      Author: kccistc
 */

/* include */
#include "comm_task.h"
#include "../../message.h"
#include "../../../common/queue/que_buffer.h"
#include "usart.h"
#include "task.h"
#include <stdbool.h>
#include <string.h>

/* define */
#define RX_SIZE 512U
#define LINE_SIZE 48U

/* typedef */

/* variable declarations */

/* variable definitions */
static uint8_t rx_ring[RX_SIZE];
static uint8_t rx_storage[RX_SIZE + 1U];
static que_buf_t rx_queue;
static uint16_t rx_tail;
static volatile uint32_t dma_wraps;
static uint32_t consumed_bytes, last_produced;
static volatile bool rx_fault;
static char rx_buff[LINE_SIZE];
static uint16_t rx_index;
static bool discard_line;
static comm_data_t comm_data;
volatile uint32_t comm_rx_bytes;
volatile uint32_t comm_frame_count;
volatile uint32_t comm_command_count;
volatile uint32_t comm_uart_errors;
volatile uint32_t comm_mode_stops, comm_button_stops, comm_fault_stops;
volatile char comm_last_frame[LINE_SIZE];

/* function declarations */
static void publish(void);
static void stop_on_fault(void);
static bool decimal_milli(const char **text, uint32_t maximum, uint32_t *value);
static bool process_line(void);
static uint32_t dma_produced(void);
static bool start_dma(void);

/* function definitions */
void comm_task_init(void){
	que_buf_init(&rx_queue, rx_storage, sizeof(uint8_t), RX_SIZE + 1U);
	rx_index = 0;
	rx_fault = discard_line = false;
	comm_rx_bytes = comm_frame_count = comm_command_count = comm_uart_errors = 0;
	comm_mode_stops = comm_button_stops = comm_fault_stops = 0;
	comm_last_frame[0] = '\0';
	comm_data = (comm_data_t){DRIVE_MANUAL, STOP, 0, 0, DRIVE_FORWARD};
	publish();
	if(!start_dma()){
		rx_fault = true;
	}
}

void comm_task(void){
	if(rx_fault){
		stop_on_fault();
		que_buf_init(&rx_queue, rx_storage, sizeof(uint8_t), RX_SIZE + 1U);
		rx_index = 0;
		discard_line = true;
		/* HAL completes error handling before returning to task context. */
		HAL_UART_DMAStop(&huart6);
		rx_fault = false;
		if(!start_dma()){
			rx_fault = true;
		}
		return;
	}
	uint32_t produced = dma_produced();
	comm_rx_bytes += produced - last_produced;
	last_produced = produced;
	if(produced - consumed_bytes > RX_SIZE){
		/* DMA lapped the consumer. Discard stale bytes, stop, then resync at LF. */
		consumed_bytes = produced;
		rx_tail = (uint16_t)(produced % RX_SIZE);
		rx_index = 0;
		discard_line = true;
		stop_on_fault();
		return;
	}
	for(uint32_t budget = 0; budget < RX_SIZE && consumed_bytes != produced; budget++){
		uint8_t ch = rx_ring[rx_tail];
		__DMB();
		rx_tail = (uint16_t)((rx_tail + 1U) % RX_SIZE);
		consumed_bytes++;
		if(!que_put(&rx_queue, &ch)){
			stop_on_fault();
			rx_index = 0;
			discard_line = true;
			return;
		}
	}
	uint8_t ch;
	while(que_get(&rx_queue, &ch)){
		if(ch == '\n'){
			if(!discard_line){
				if(rx_index && rx_buff[rx_index - 1U] == '\r'){
					rx_index--;
				}
				rx_buff[rx_index] = '\0';
				for(uint16_t i = 0; i <= rx_index; i++){
					comm_last_frame[i] = rx_buff[i];
				}
				comm_frame_count++;
				if(process_line()){
					comm_command_count++;
				}
			}
			rx_index = 0;
			discard_line = false;
		}
		else if(!discard_line){
			if(rx_index >= LINE_SIZE - 1U || ch == '\0'){
				rx_index = 0;
				discard_line = true;
				stop_on_fault();
			}
			else {
				rx_buff[rx_index++] = (char)ch;
			}
		}
	}
}

static void publish(void){
	if(commQueHandle != NULL){
		xQueueOverwrite(commQueHandle, &comm_data);
	}
}

static void stop_on_fault(void){
	comm_fault_stops++;
	comm_data.state = STOP;
	comm_data.speed = 0;
	comm_data.steering = 0;
	publish();
}

/* Non-negative decimal to thousandths; no float/scanf stack overhead.
 * Accept up to six fractional digits and round to the nearest 0.001. */
static bool decimal_milli(const char **text, uint32_t maximum, uint32_t *value){
	const char *p = *text;
	uint32_t whole = 0, fraction = 0, digits = 0, round_digit = 0;
	bool fractional_nonzero = false;
	if(*p < '0' || *p > '9'){
		return false;
	}
	while(*p >= '0' && *p <= '9'){
		whole = whole * 10U + (uint32_t)(*p++ - '0');
		if(whole > maximum){
			return false;
		}
	}
	if(*p == '.'){
		p++;
		if(*p < '0' || *p > '9'){
			return false;
		}
		while(*p >= '0' && *p <= '9'){
			uint32_t digit = (uint32_t)(*p++ - '0');
			if(++digits > 6U){
				return false;
			}
			if(digit){
				fractional_nonzero = true;
			}
			if(digits <= 3U){
				fraction = fraction * 10U + digit;
			}
			if(digits == 4U){
				round_digit = digit;
			}
		}
	}
	if(whole == maximum && fractional_nonzero){
		return false;
	}
	while(digits < 3U){
		fraction *= 10U;
		digits++;
	}
	*value = whole * 1000U + fraction + (round_digit >= 5U ? 1U : 0U);
	*text = p;
	return true;
}

static bool process_line(void){
	if(strcmp(rx_buff, "T0:0") == 0 || strcmp(rx_buff, "T0:1") == 0){
		/* A repeated mode report is not a new STOP or RUN command. */
		drive_mode_t requested_mode = rx_buff[3] == '0' ? DRIVE_MANUAL : DRIVE_AUTO;
		if(comm_data.mode != requested_mode){
			comm_mode_stops++;
			comm_data.mode = requested_mode;
			comm_data.state = STOP;
			comm_data.speed = 0;
			comm_data.steering = 0;
			publish();
		}
	}
	else if(strcmp(rx_buff, "B0:D") == 0){
		comm_button_stops++;
		comm_data.state = STOP;
		publish();
	}
	else if(strcmp(rx_buff, "B1:D") == 0){
		comm_data.state = RUN;
		publish();
	}
	else if(strncmp(rx_buff, "J0:", 3) == 0 && comm_data.mode == DRIVE_MANUAL){
		const char *p = rx_buff + 3;
		uint32_t angle, magnitude;
		if(!decimal_milli(&p, 360U, &angle) || *p++ != ',' || !decimal_milli(&p, 1U, &magnitude) || *p != '\0'){
			return false;
		}
		comm_data.speed = (uint16_t)magnitude;
		if(magnitude == 0U){
			comm_data.steering = 0;
		}
		else {
			if(angle > 0U && angle < 180000U){
				comm_data.direction = DRIVE_FORWARD;
			}
			else if(angle > 180000U && angle < 360000U){
				comm_data.direction = DRIVE_REVERSE;
			}
			int32_t offset = angle <= 180000U ? 90000 - (int32_t)angle : (int32_t)angle - 270000;
			comm_data.steering = (int16_t)(offset * 1000 / 90000);
		}
		publish(); /* Joystick updates never arm a stopped car. */
	}
	else {
		return false;
	}
	return true;
}

/* Read a monotonic DMA write position, including a full-transfer IRQ which
 * became pending while the DHT11 driver masked interrupts. No DMA pause needed. */
static uint32_t dma_produced(void){
	uint32_t wraps, remaining;
	taskENTER_CRITICAL();
	wraps = dma_wraps;
	remaining = __HAL_DMA_GET_COUNTER(huart6.hdmarx);
	if(__HAL_DMA_GET_FLAG(huart6.hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(huart6.hdmarx))){
		wraps++;
		remaining = __HAL_DMA_GET_COUNTER(huart6.hdmarx);
		if(remaining == 0U){
			remaining = RX_SIZE;
		}
	}
	taskEXIT_CRITICAL();
	__DMB();
	return wraps * RX_SIZE + RX_SIZE - remaining;
}

static bool start_dma(void){
	dma_wraps = consumed_bytes = last_produced = 0;
	rx_tail = 0;
	/* A missing DMA configuration must not crash the scheduler. */
	if(huart6.hdmarx == NULL){
		return false;
	}
	return HAL_UART_Receive_DMA(&huart6, rx_ring, RX_SIZE) == HAL_OK;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART6){
		dma_wraps++;
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART6){
		comm_uart_errors++;
		rx_fault = true;
	}
	/* Rearm from the task after HAL finishes its error handling. */
}
