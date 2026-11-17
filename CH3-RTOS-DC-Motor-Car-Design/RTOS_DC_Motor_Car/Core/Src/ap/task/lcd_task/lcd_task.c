/*
 * lcd_task.c
 *
 *  Created on: 2026. 11. 16.
 *      Author: kccistc
 */

/* include */
#include "lcd_task.h"
#include "../motor_control_task/motor_control_task.h"
#include "../time_task/time_task.h"
#include "../temp_humid_task/temp_humid_task.h"
#include "../../../driver/lcd/lcd.h"
#include "i2c.h"
#include "task.h"
#include <string.h>

/* define */

/* typedef */

/* variable declarations */

/* variable definitions */
static bool initialized = false;
static bool retry_pending = false;
static bool dirty[3] = {true, true, true};
static char shown_time[17] = {0}, shown_state[7] = {0}, shown_sensor[11] = {0};
static char next_time[17], next_state[7], next_temp_humid[11];
static motor_status_t lcd_motor_status;
static uint32_t retry_at = 0;
static TickType_t display_next;

/* function declarations */
static void temp_humid_task_get_text(char text[11]);
static void time_task_get_text(char text[17]);
static void motor_format_display(const motor_status_t *lcd_motor_status, char text[7]);
static void motor_get_status(motor_status_t *lcd_motor_status);

/* function definitions */
void lcd_task_init(void){
	initialized = false;
	retry_pending = false;
	dirty[0] = dirty[1] = dirty[2] = true;
	display_next = xTaskGetTickCount();
}

void lcd_task(void){
	motor_get_status(&lcd_motor_status);
	motor_format_display(&lcd_motor_status, next_state);
	time_task_get_text(next_time);
	temp_humid_task_get_text(next_temp_humid);
	uint32_t now = HAL_GetTick();
	if((!retry_pending || (int32_t)(now - retry_at) >= 0) && osMutexWait(lcdMutexHandle, 0) == osOK){
		retry_pending = false;
		bool ok = true;
		if(!initialized){
			initialized = Lcd_init(&hi2c1);
			ok = initialized;
			dirty[0] = dirty[1] = dirty[2] = true;
		}
		if(ok && (dirty[0] || memcmp(shown_time, next_time, 16) != 0)){
			/* A whole row takes longer than 10ms over this 100kHz bus.
			 * Update only the changed span (usually one or two digits). */
			uint8_t first = 0, end = 16;
			if(!dirty[0]){
				while(first < end && shown_time[first] == next_time[first]){
					first++;
				}
				while(end > first && shown_time[end - 1] == next_time[end - 1]){
					end--;
				}
			}
			char fragment[17];
			memcpy(fragment, next_time + first, end - first);
			fragment[end - first] = '\0';
			ok = Lcd_writeStringXY(first, 0, fragment);
			if(ok){
				memcpy(shown_time, next_time, 17);
				dirty[0] = false;
			}
		}
		if(ok && (dirty[1] || memcmp(shown_state, next_state, 6) != 0)){
			ok = Lcd_writeStringXY(0, 1, next_state);
			if(ok){
				memcpy(shown_state, next_state, 7);
				dirty[1] = false;
			}
		}
		if(ok && (dirty[2] || memcmp(shown_sensor, next_temp_humid, 10) != 0)){
			ok = Lcd_writeStringXY(6, 1, next_temp_humid);
			if(ok){
				memcpy(shown_sensor, next_temp_humid, 11);
				dirty[2] = false;
			}
		}
		osMutexRelease(lcdMutexHandle);
		if(!ok){
			initialized = false;
			retry_at = HAL_GetTick() + 200U;
			retry_pending = true;
		}
	}
	/* Include transfer time in the nominal 10ms refresh period. */
	display_next += pdMS_TO_TICKS(10);
	TickType_t display_now = xTaskGetTickCount();
	int32_t wait = (int32_t)(display_next - display_now);
	if(wait > 1){
		vTaskDelay((TickType_t)(wait - 1));
	}
	else if(wait <= 0){
		display_next = display_now;
	}
}

static void motor_get_status(motor_status_t *lcd_motor_status){
	taskENTER_CRITICAL();
	*lcd_motor_status = applied;
	taskEXIT_CRITICAL();
}

static void motor_format_display(const motor_status_t *lcd_motor_status, char text[7]){
	if(lcd_motor_status->command.state == STOP){
		memcpy(text, "STOP /", 7);
	}
	else {
		memcpy(text, lcd_motor_status->command.direction == DRIVE_REVERSE ? "REV  /" : "FOR  /", 7);
		if(lcd_motor_status->command.speed != 0){
			if(lcd_motor_status->command.steering > 0){
				text[4] = 'R';
			}
			else if(lcd_motor_status->command.steering < 0){
				text[4] = 'L';
			}
		}
	}
}

static void time_task_get_text(char text[17]){
	uint32_t value, reference;
	taskENTER_CRITICAL();
	value = clock_ms;
	reference = last_tick;
	taskEXIT_CRITICAL();
	/* Interpolate at the instant the LCD owner requests text, even if it runs
	 * before TimeTask at a tick boundary. */
	value = (value + (HAL_GetTick() - reference) % 86400000U) % 86400000U;
	uint32_t h = value / 3600000U, m = value / 60000U % 60U, s = value / 1000U % 60U;
	const char prefix[] = "TIME:";
	for(unsigned i = 0; i < 5; i++){
		text[i] = prefix[i];
	}
	text[5] = '0' + h / 10U;
	text[6] = '0' + h % 10U;
	text[7] = ':';
	text[8] = '0' + m / 10U;
	text[9] = '0' + m % 10U;
	text[10] = ':';
	text[11] = '0' + s / 10U;
	text[12] = '0' + s % 10U;
	text[13] = ':';
	uint32_t hundredths = value / 10U % 100U;
	text[14] = '0' + hundredths / 10U;
	text[15] = '0' + hundredths % 10U;
	text[16] = '\0';
}

static void temp_humid_task_get_text(char text[11]){
	dht11_data_t value;
	bool valid;
	taskENTER_CRITICAL();
	value = TH_data;
	valid = sample_valid;
	taskEXIT_CRITICAL();
	if(!valid){
		memcpy(text, "-- C, -- %", 11);
		return;
	}
	memcpy(text, "00 C, 00 %", 11);
	text[0] = value.t_int >= 10U ? '0' + value.t_int / 10U : ' ';
	text[1] = '0' + value.t_int % 10U;
	if(value.h_int < 100U){
		text[6] = value.h_int >= 10U ? '0' + value.h_int / 10U : ' ';
		text[7] = '0' + value.h_int % 10U;
	}
	else {
		/* Keep 100% within the same ten reserved columns. */
		text[2] = 'C';
		text[3] = ',';
		text[4] = ' ';
		text[5] = '1';
		text[6] = '0';
		text[7] = '0';
	}
}
