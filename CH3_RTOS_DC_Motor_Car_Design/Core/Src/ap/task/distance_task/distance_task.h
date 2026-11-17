/*
 * distance_task.h
 *
 *  Created on: 2026. 10. 29.
 *      Author: kccistc
 */

#ifndef SRC_AP_TASK_DISTANCE_TASK_DISTANCE_TASK_H_
#define SRC_AP_TASK_DISTANCE_TASK_DISTANCE_TASK_H_

/* include */
#include "../../message.h"
#include "../../../driver/sr04/sr04.h"

/* define */
#define DISTANCE_TRIGGER_MS 65U

/* typedef */
/* Front fan: center faces forward, L/R face about 30 degrees outward.
 * Readings are raw beam distances in centimetres. */

typedef enum{ DISTANCE_RIGHT, DISTANCE_CENTER, DISTANCE_LEFT, DISTANCE_COUNT } distance_sensor_t;
typedef struct{
	float centimetres;
	uint32_t measured_at;
	sr04_status_t status;
	bool valid;
} distance_reading_t;
typedef struct{
	distance_reading_t sensor[DISTANCE_COUNT];
} distance_snapshot_t;

/* variable declarations */
/* Copy shared task data inside a FreeRTOS critical section. */
extern volatile bool echo_in_flight;
extern volatile bool dht11_reserved;
extern distance_snapshot_t distances;

/* function declarations */
void distance_task_init(void);
void distance_task(void);
void HAL_GPIO_EXTI_Callback(uint16_t pin);

#endif /* SRC_AP_TASK_DISTANCE_TASK_DISTANCE_TASK_H_ */
