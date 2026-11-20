/*
 * display.h
 *
 *  Created on: 2026. 11. 5.
 *      Author: kccistc
 */

#ifndef SRC_AP_SERVICE_DISPLAY_DISPLAY_H_
#define SRC_AP_SERVICE_DISPLAY_DISPLAY_H_

/* include */
#include "stm32f4xx_hal.h"
#include "../../../driver/dht11/dht11.h"

/* define */

/* typedef */
typedef struct{
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t msec;
} TimeClockTime_t;

typedef struct{
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
} TimeClockDate_t;

/* variable declarations */

/* function declarations */
void disp_init(void);
void disp_setTimeDate(TimeClockTime_t *pTime, TimeClockDate_t *pDate);
void disp_setTempHumid(dht11_data_t *pTH);
void disp_stop(void);
void disp_ready(uint8_t floor);
void disp_busy(uint8_t floor);
void disp_emergency(void);
void disp_move(uint8_t floor, uint8_t up);

#endif /* SRC_AP_SERVICE_DISPLAY_DISPLAY_H_ */
