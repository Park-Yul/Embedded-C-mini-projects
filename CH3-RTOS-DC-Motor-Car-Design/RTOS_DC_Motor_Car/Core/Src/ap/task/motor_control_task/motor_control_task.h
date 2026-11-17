/*
 * motor_control_task.h
 *
 *  Created on: 2026. 11. 12.
 *      Author: kccistc
 */

#ifndef SRC_AP_TASK_MOTOR_CONTROL_TASK_MOTOR_CONTROL_TASK_H_
#define SRC_AP_TASK_MOTOR_CONTROL_TASK_MOTOR_CONTROL_TASK_H_

/* include */
#include "../../message.h"

/* define */

/* typedef */
typedef struct{
	comm_data_t command;
	int16_t left_output;
	int16_t right_output;
	uint32_t stop_sequence;
} motor_status_t;

/* variable declarations */
/* Copy shared task data inside a FreeRTOS critical section. */
extern motor_status_t applied;
extern motor_state_t motor_state;

/* function declarations */
void motor_control_task_init(void);
void motor_control_task(void);

#endif /* SRC_AP_TASK_MOTOR_CONTROL_TASK_MOTOR_CONTROL_TASK_H_ */
