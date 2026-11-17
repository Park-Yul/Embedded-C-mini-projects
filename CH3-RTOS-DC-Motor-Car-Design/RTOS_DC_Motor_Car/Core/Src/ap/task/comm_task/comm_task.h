/*
 * comm_task.h
 *
 *  Created on: 2026. 11. 12.
 *      Author: kccistc
 */

#ifndef SRC_AP_TASK_COMM_TASK_COMM_TASK_H_
#define SRC_AP_TASK_COMM_TASK_COMM_TASK_H_

/* include */
#include "stm32f4xx_hal.h"

/* define */

/* typedef */

/* variable declarations */
/* Read these in CubeIDE Expressions without adding blocking UART prints. */
extern volatile uint32_t comm_rx_bytes;
extern volatile uint32_t comm_frame_count;
extern volatile uint32_t comm_command_count;
extern volatile uint32_t comm_uart_errors;
extern volatile uint32_t comm_mode_stops;
extern volatile uint32_t comm_button_stops;
extern volatile uint32_t comm_fault_stops;
extern volatile char comm_last_frame[48];

/* function declarations */
void comm_task_init(void);
void comm_task(void);
/* HAL requires externally linked callback functions. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif /* SRC_AP_TASK_COMM_TASK_COMM_TASK_H_ */
