#ifndef AP_MESSAGE_H_
#define AP_MESSAGE_H_

/* include */
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdbool.h>

/* define */

/* typedef */
typedef enum{ DRIVE_MANUAL, DRIVE_AUTO } drive_mode_t;
typedef enum{ STOP, RUN } motor_state_t;
typedef enum{ DRIVE_FORWARD = 1, DRIVE_REVERSE = -1 } drive_direction_t;
/* Complete command copied by value. Steering: -1000 left, +1000 right. */
typedef struct{
	drive_mode_t mode;
	motor_state_t state;
	uint16_t speed;
	int16_t steering;
	drive_direction_t direction;
} comm_data_t;

/* variable declarations */
extern QueueHandle_t commQueHandle;
extern osMutexId lcdMutexHandle;

/* function declarations */
bool comm_mailbox_init(void);

#endif /* AP_MESSAGE_H_ */
