/*
 * time_task.h
 *
 *  Created on: 2026. 10. 19.
 *      Author: kccistc
 */

#ifndef SRC_AP_TASK_TIME_TASK_TIME_TASK_H_
#define SRC_AP_TASK_TIME_TASK_TIME_TASK_H_

/* include */
#include "stm32f4xx_hal.h"

/* define */
/* Initial RTC time on each MCU boot (24-hour format). */
#define TIME_INITIAL_HOUR 18U
#define TIME_INITIAL_MINUTE 48U
#define TIME_INITIAL_SECOND 13U

/* typedef */

/* variable declarations */
/* Copy shared task data inside a FreeRTOS critical section. */
extern uint32_t last_tick;
extern uint32_t clock_ms;
extern volatile HAL_StatusTypeDef time_rtc_status;

/* function declarations */
void time_task_init(void);
void time_task(void);

#endif /* SRC_AP_TASK_TIME_TASK_TIME_TASK_H_ */
