/*
 * temp_humid_task.c
 *
 *  Created on: 2026. 10. 28.
 *      Author: kccistc
 */

/* include */
#include "temp_humid_task.h"
#include "../distance_task/distance_task.h"
#include "../../../driver/dht11/dht11.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>

/* define */

/* typedef */

/* variable declarations */

/* variable definitions */
dht11_data_t TH_data;
bool sample_valid;
static uint32_t next_sample;
volatile dht11_status_t temp_humid_status = DHT11_ERR_NO_RESPONSE;

/* function declarations */
static void distance_resume_after_dht11(void);
static bool distance_pause_for_dht11(void);

/* function definitions */
void temp_humid_task_init(void){
	sample_valid = false;
	__HAL_RCC_GPIOA_CLK_ENABLE();
	dht11_init(GPIOA, GPIO_PIN_15);
	next_sample = HAL_GetTick() + 1000U; /* Sensor power-up settling. */
}

void temp_humid_task(void){
	uint32_t now = HAL_GetTick();
	if((int32_t)(now - next_sample) < 0){
		return;
	}
	if(!distance_pause_for_dht11()){
		return;
	}
	next_sample = now + 2000U;
	dht11_data_t measured = {0};
	temp_humid_status = dht11_read(&measured);
	distance_resume_after_dht11();
	if(temp_humid_status == DHT11_OK && measured.t_int <= 99U && measured.h_int <= 100U){
		taskENTER_CRITICAL();
		TH_data = measured;
		sample_valid = true;
		taskEXIT_CRITICAL();
	}
}

static bool distance_pause_for_dht11(void){
	bool accepted;
	taskENTER_CRITICAL();
	accepted = !echo_in_flight && !dht11_reserved;
	if(accepted){
		dht11_reserved = true;
	}
	taskEXIT_CRITICAL();
	return accepted;
}

static void distance_resume_after_dht11(void){
	taskENTER_CRITICAL();
	dht11_reserved = false;
	taskEXIT_CRITICAL();
}
