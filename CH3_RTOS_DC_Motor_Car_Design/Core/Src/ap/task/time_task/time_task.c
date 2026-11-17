/*
 * time_task.c
 *
 *  Created on: 2026. 10. 19.
 *      Author: kccistc
 */

/* include */
#include "time_task.h"
#include "../../message.h"
#include "../../../driver/rtc/rtc.h"
#include "i2c.h"
#include "task.h"
#include <stdbool.h>

/* define */
#define DAY_MS 86400000U
#define INITIAL_MS ((TIME_INITIAL_HOUR * 3600U + TIME_INITIAL_MINUTE * 60U + TIME_INITIAL_SECOND) * 1000U)

/* typedef */

/* variable declarations */

/* variable definitions */
_Static_assert(
    TIME_INITIAL_HOUR < 24U && TIME_INITIAL_MINUTE < 60U && TIME_INITIAL_SECOND < 60U, "Invalid initial time");
uint32_t clock_ms = INITIAL_MS;
uint32_t last_tick;
static uint32_t last_sync;
static bool rtc_seeded;
volatile HAL_StatusTypeDef time_rtc_status = HAL_ERROR;

/* function declarations */
static bool decode_bcd(uint8_t b, uint8_t maximum, uint8_t *v);
static void rtc_update(uint32_t *ms);

/* function definitions */
void time_task_init(void){
	clock_ms = INITIAL_MS;
	rtc_seeded = false;
	ds3231_init(&hi2c1);
	uint32_t value = INITIAL_MS;
	rtc_update(&value);
	taskENTER_CRITICAL();
	clock_ms = value;
	last_tick = last_sync = HAL_GetTick();
	taskEXIT_CRITICAL();
}

void time_task(void){
	uint32_t now = HAL_GetTick();
	uint32_t value = (clock_ms + (now - last_tick) % DAY_MS) % DAY_MS;
	if(now - last_sync >= 1000U){
		last_sync = now;
		rtc_update(&value);
	}
	taskENTER_CRITICAL();
	clock_ms = value;
	last_tick = now;
	taskEXIT_CRITICAL();
}

static bool decode_bcd(uint8_t b, uint8_t maximum, uint8_t *v){
	if((b & 15U) > 9U || (b >> 4) > 9U){
		return false;
	}
	*v = (b >> 4) * 10U + (b & 15U);
	return *v <= maximum;
}

static void rtc_update(uint32_t *ms){
	/* RTC and LCD share I2C1. Never touch the bus without the same mutex. */
	if(osMutexWait(lcdMutexHandle, 0) != osOK){
		return;
	}
	if(!rtc_seeded){
		time_rtc_status = HAL_I2C_IsDeviceReady(&hi2c1, 0x68U << 1, 1, 5);
		if(time_rtc_status == HAL_OK){
			ds3231_time_t seed = {.sec = (*ms / 1000U) % 60U, .min = (*ms / 60000U) % 60U, .hour = *ms / 3600000U};
			ds3231_setTime(&seed);
			rtc_seeded = hi2c1.ErrorCode == HAL_I2C_ERROR_NONE;
			if(!rtc_seeded){
				time_rtc_status = HAL_ERROR;
			}
		}
	}
	else {
		/* The supplied getter discards HAL errors and uses an uninitialized
		 * local buffer on failure. Keep that driver unchanged; check reads here. */
		uint8_t raw[3] = {0};
		ds3231_time_t value;
		time_rtc_status = HAL_I2C_Mem_Read(&hi2c1, 0x68U << 1, 0, I2C_MEMADD_SIZE_8BIT, raw, sizeof raw, 10);
		if(time_rtc_status == HAL_OK){
			if((raw[2] & 0x40U) || !decode_bcd(raw[0] & 0x7FU, 59, &value.sec) ||
			    !decode_bcd(raw[1] & 0x7FU, 59, &value.min) || !decode_bcd(raw[2] & 0x3FU, 23, &value.hour)){
				time_rtc_status = HAL_ERROR;
			}
			else {
				*ms = ((uint32_t)value.hour * 3600U + value.min * 60U + value.sec) * 1000U + *ms % 1000U;
			}
		}
	}
	osMutexRelease(lcdMutexHandle);
}
