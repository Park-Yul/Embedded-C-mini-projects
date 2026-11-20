/*
 * operation.h
 *
 *  Created on: 2026. 11. 5.
 *      Author: kccistc
 */

#ifndef SRC_AP_SERVICE_OPERATION_OPERATION_H_
#define SRC_AP_SERVICE_OPERATION_OPERATION_H_

/* include */
#include "stm32f4xx_hal.h"

/* define */

/* typedef */

/* variable declarations */

/* function declarations */
void operation_init(void);
void operation_idle(void);
void operation_task(int dir);
void operation_emergency_task(void);
void operation_emergency_reset(void);
void operation_door_stop(void);
void operation_door_open(void);
void operation_door_close(void);
uint8_t operation_door_is_closed(void);
void operation_door_task(void);
void operation_door_test_left(uint8_t pressed);

#endif /* SRC_AP_SERVICE_OPERATION_OPERATION_H_ */
