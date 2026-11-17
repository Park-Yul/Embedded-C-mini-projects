/*
 * temp_humid_task.h
 *
 *  Created on: 2026. 10. 28.
 *      Author: kccistc
 */

#ifndef SRC_AP_TASK_TEMP_HUMID_TASK_TEMP_HUMID_TASK_H_
#define SRC_AP_TASK_TEMP_HUMID_TASK_TEMP_HUMID_TASK_H_

/* include */
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "../../../driver/dht11/dht11.h"

/* define */

/* typedef */

/* variable declarations */
/* Copy shared task data inside a FreeRTOS critical section. */
extern bool sample_valid;
extern dht11_data_t TH_data;
extern volatile dht11_status_t temp_humid_status;

/* function declarations */
void temp_humid_task_init(void);
void temp_humid_task(void);

#endif /* SRC_AP_TASK_TEMP_HUMID_TASK_TEMP_HUMID_TASK_H_ */
