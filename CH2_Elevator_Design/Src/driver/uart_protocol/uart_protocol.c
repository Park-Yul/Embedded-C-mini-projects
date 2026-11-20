/*
 * uart_protocol.c
 *
 *  Created on: 2026. 11. 4.
 *      Author: kccistc
 *
 *  수신 구조
 *   - USART2 RX 인터럽트에서 1바이트씩 받아 패킷을 조립하고, 체크섬이 맞으면 수신 큐에 넣기만 함
 *   - 메인 루프의 UART_protocol_task() 가 큐에서 꺼내 콜백(packet_do_func) 호출
 *     → 엘리베이터 상태 변수를 인터럽트가 아닌 메인 루프에서만 바꾸므로 충돌이 없음
 */

/* include */
#include "uart_protocol.h"
#include <string.h>

/* define */
#define RX_PKT_Q_SIZE		4		// 처리 전에 쌓아둘 수 있는 패킷 수

/* typedef */
typedef enum{
	RX_WAIT_START,
	RX_WAIT_LEN,
	RX_WAIT_DATA,
	RX_WAIT_CHK
} rx_state_e;

typedef struct{
	uint8_t len;					// CMD + payload 길이
	uint8_t data[PKT_MAX_LEN];		// data[0] = CMD
} rx_pkt_t;

/* variable declarations */

/* variable definitions */
static UART_HandleTypeDef *protocol_huart;
static uart_packet_callback_t packet_callback = 0;
static volatile uint8_t rx_byte;
static rx_state_e rx_state;
static uint8_t rx_len;
static uint8_t rx_idx;
static uint8_t rx_buf[PKT_MAX_LEN];

static rx_pkt_t rx_q[RX_PKT_Q_SIZE];
static volatile uint8_t rx_q_head = 0;		// 인터럽트가 씀
static volatile uint8_t rx_q_tail = 0;		// 메인 루프가 씀
static volatile uint8_t rx_restart = 0;		// 수신 재시작이 필요하면 1

volatile uint32_t debugUartRxPkt = 0;		// 정상 수신 패킷 수
volatile uint32_t debugUartRxErr = 0;		// 체크섬 오류/큐 넘침/UART 오류 수

/* function declarations */
static void rx_parse_byte(uint8_t rx_data);
static void rx_start(void);

/* function definitions */
static void rx_start(void){
	if(HAL_UART_Receive_IT(protocol_huart, (uint8_t *)&rx_byte, 1) != HAL_OK){
		rx_restart = 1;		// 지금 못 걸었으면 메인 루프에서 다시 시도
	}
	else{
		rx_restart = 0;
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		rx_parse_byte(rx_byte);
		rx_start();
	}
}

/* 노이즈/오버런 등으로 HAL 이 수신을 멈추면 다시 시작 (없으면 수신이 영영 멈춤) */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		debugUartRxErr++;
		rx_state = RX_WAIT_START;
		rx_start();
	}
}

void UART_protocol_init(UART_HandleTypeDef *huart, uart_packet_callback_t packet_cb){
	protocol_huart = huart;
	packet_callback = packet_cb;
	rx_state = RX_WAIT_START;
	rx_q_head = 0;
	rx_q_tail = 0;
	rx_start();
}

void UART_protocol_task(void){
	if(protocol_huart == 0){
		return;
	}
	if(rx_restart){
		rx_start();
	}
	while(rx_q_tail != rx_q_head){
		rx_pkt_t *pkt = &rx_q[rx_q_tail];
		if(packet_callback){
			packet_callback(pkt->data[0], &pkt->data[1], (uint8_t)(pkt->len - 1));
		}
		rx_q_tail = (uint8_t)((rx_q_tail + 1) % RX_PKT_Q_SIZE);
	}
}

static void rx_parse_byte(uint8_t rx_data){
	switch(rx_state){
	case RX_WAIT_START:
		if(rx_data == PKT_START){
			rx_state = RX_WAIT_LEN;
		}
		break;
	case RX_WAIT_LEN:
		if(rx_data == 0 || rx_data >= PKT_MAX_LEN){
			rx_state = RX_WAIT_START;
		}
		else {
			rx_len = rx_data;
			rx_idx = 0;
			rx_state = RX_WAIT_DATA;
		}
		break;
	case RX_WAIT_DATA:
		rx_buf[rx_idx++] = rx_data;
		if(rx_idx >= rx_len){
			rx_state = RX_WAIT_CHK;
		}
		break;
	case RX_WAIT_CHK:{
		uint8_t cksum = rx_len;
		uint8_t next;
		for(int i=0; i<rx_len; i++){
			cksum += rx_buf[i];
		}
		if(cksum == rx_data){
			next = (uint8_t)((rx_q_head + 1) % RX_PKT_Q_SIZE);
			if(next != rx_q_tail){
				rx_q[rx_q_head].len = rx_len;
				memcpy(rx_q[rx_q_head].data, rx_buf, rx_len);
				rx_q_head = next;
				debugUartRxPkt++;
			}
			else{
				debugUartRxErr++;		// 큐가 꽉 참: 이 패킷은 버림
			}
		}
		else{
			debugUartRxErr++;			// 체크섬 오류
		}
		rx_state = RX_WAIT_START;
		break;
	}
	default:
		rx_state = RX_WAIT_START;
		break;
	}
}

void UART_protocol_send(uint8_t cmd, uint8_t *payload, uint8_t payload_len){
	uint8_t tx[3+PKT_MAX_LEN];
	uint8_t len = 1 + payload_len;
	uint8_t cksum = len;

	if(protocol_huart == 0 || payload_len > PKT_MAX_LEN - 1){
		return;
	}

	tx[0] = PKT_START;
	tx[1] = len;
	tx[2] = cmd;
	cksum += cmd;

	for(int i=0; i<payload_len; i++){
		tx[3+i] = payload[i];
		cksum += payload[i];
	}
	tx[3+payload_len] = cksum;
	HAL_UART_Transmit(protocol_huart, tx, (3+payload_len+1), 10);	// 최대 16바이트 ≈ 1.4ms
}
