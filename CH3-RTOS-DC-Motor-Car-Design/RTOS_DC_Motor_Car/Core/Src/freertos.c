/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ap/task/comm_task/comm_task.h"
#include "ap/task/motor_control_task/motor_control_task.h"
#include "ap/task/sound_task/sound_task.h"
#include "ap/task/fnd_task/fnd_task.h"
#include "ap/task/distance_task/distance_task.h"
#include "ap/task/time_task/time_task.h"
#include "ap/task/temp_humid_task/temp_humid_task.h"
#include "ap/task/lcd_task/lcd_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId CommTaskHandle;
osThreadId MotorTaskHandle;
osThreadId SoundTaskHandle;
osThreadId FNDTaskHandle;
osThreadId TimeTaskHandle;
osThreadId TempHumidTaskHandle;
osThreadId DistanceTaskHandle;
osThreadId LcdTaskHandle;
osMessageQId commQueHandle;
osMutexId lcdMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartCommTask(void const * argument);
void StartMotorTask(void const * argument);
void StartSoundTask(void const * argument);
void StartFNDTask(void const * argument);
void StartTimeTask(void const * argument);
void StartTempHumidTask(void const * argument);
void DistanceStartTask(void const * argument);
void StartLcdTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
	/* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
	/* USER CODE BEGIN Init */

	/* USER CODE END Init */
	/* Create the mutex(es) */
	/* definition and creation of lcdMutex */
	osMutexDef(lcdMutex);
	lcdMutexHandle = osMutexCreate(osMutex(lcdMutex));

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* Create the queue(s) */
	/* definition and creation of commQue */
	osMessageQDef(commQue, 16, uint32_t);
	commQueHandle = osMessageCreate(osMessageQ(commQue), NULL);

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	if (!comm_mailbox_init() || lcdMutexHandle == NULL) Error_Handler();
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* definition and creation of defaultTask */
	osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
	defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

	/* definition and creation of CommTask */
	osThreadDef(CommTask, StartCommTask, osPriorityNormal, 0, 128);
	CommTaskHandle = osThreadCreate(osThread(CommTask), NULL);

	/* definition and creation of MotorTask */
	osThreadDef(MotorTask, StartMotorTask, osPriorityNormal, 0, 128);
	MotorTaskHandle = osThreadCreate(osThread(MotorTask), NULL);

	/* definition and creation of SoundTask */
	osThreadDef(SoundTask, StartSoundTask, osPriorityNormal, 0, 128);
	SoundTaskHandle = osThreadCreate(osThread(SoundTask), NULL);

	/* definition and creation of FNDTask */
	osThreadDef(FNDTask, StartFNDTask, osPriorityNormal, 0, 128);
	FNDTaskHandle = osThreadCreate(osThread(FNDTask), NULL);

	/* definition and creation of TimeTask */
	osThreadDef(TimeTask, StartTimeTask, osPriorityNormal, 0, 128);
	TimeTaskHandle = osThreadCreate(osThread(TimeTask), NULL);

	/* definition and creation of TempHumidTask */
	osThreadDef(TempHumidTask, StartTempHumidTask, osPriorityNormal, 0, 128);
	TempHumidTaskHandle = osThreadCreate(osThread(TempHumidTask), NULL);

	/* definition and creation of DistanceTask */
	osThreadDef(DistanceTask, DistanceStartTask, osPriorityNormal, 0, 128);
	DistanceTaskHandle = osThreadCreate(osThread(DistanceTask), NULL);

	/* definition and creation of LcdTask */
	osThreadDef(LcdTask, StartLcdTask, osPriorityNormal, 0, 128);
	LcdTaskHandle = osThreadCreate(osThread(LcdTask), NULL);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
	/* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
	/* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartCommTask */
/**
 * @brief Function implementing the CommTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartCommTask */
void StartCommTask(void const * argument)
{
	/* USER CODE BEGIN StartCommTask */
	comm_task_init();
	/* Infinite loop */
	for(;;)
	{
		comm_task();
		osDelay(1);
	}
	/* USER CODE END StartCommTask */
}

/* USER CODE BEGIN Header_StartMotorTask */
/**
 * @brief Function implementing the MotorTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartMotorTask */
void StartMotorTask(void const * argument)
{
	/* USER CODE BEGIN StartMotorTask */
	motor_control_task_init();
	/* Infinite loop */
	for(;;)
	{
		motor_control_task_init();
		osDelay(1);
	}
	/* USER CODE END StartMotorTask */
}

/* USER CODE BEGIN Header_StartSoundTask */
/**
 * @brief Function implementing the SoundTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartSoundTask */
void StartSoundTask(void const * argument)
{
	/* USER CODE BEGIN StartSoundTask */
	sound_task_init();
	/* Infinite loop */
	for(;;)
	{
		sound_task();
		osDelay(1);
	}
	/* USER CODE END StartSoundTask */
}

/* USER CODE BEGIN Header_StartFNDTask */
/**
 * @brief Function implementing the FNDTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartFNDTask */
void StartFNDTask(void const * argument)
{
	/* USER CODE BEGIN StartFNDTask */
	fnd_task_init();
	/* Infinite loop */
	for(;;)
	{
		fnd_task();
		osDelay(1);
	}
	/* USER CODE END StartFNDTask */
}

/* USER CODE BEGIN Header_StartTimeTask */
/**
 * @brief Function implementing the TimeTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTimeTask */
void StartTimeTask(void const * argument)
{
	/* USER CODE BEGIN StartTimeTask */
	time_task_init();
	/* Infinite loop */
	for(;;)
	{
		time_task();
		osDelay(1);
	}
	/* USER CODE END StartTimeTask */
}

/* USER CODE BEGIN Header_StartTempHumidTask */
/**
 * @brief Function implementing the TempHumidTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTempHumidTask */
void StartTempHumidTask(void const * argument)
{
	/* USER CODE BEGIN StartTempHumidTask */
	temp_humid_task_init();
	/* Infinite loop */
	for(;;)
	{
		temp_humid_task();
		osDelay(1);
	}
	/* USER CODE END StartTempHumidTask */
}

/* USER CODE BEGIN Header_DistanceStartTask */
/**
 * @brief Function implementing the DistanceTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_DistanceStartTask */
void DistanceStartTask(void const * argument)
{
	/* USER CODE BEGIN DistanceStartTask */
	distance_task_init();
	/* Infinite loop */
	for(;;)
	{
		distance_task();
		osDelay(1);
	}
	/* USER CODE END DistanceStartTask */
}

/* USER CODE BEGIN Header_StartLcdTask */
/**
 * @brief Function implementing the LcdTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartLcdTask */
void StartLcdTask(void const * argument)
{
	/* USER CODE BEGIN StartLcdTask */
	lcd_task_init();
	/* Infinite loop */
	for(;;)
	{
		lcd_task();
		osDelay(1);
	}
	/* USER CODE END StartLcdTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
