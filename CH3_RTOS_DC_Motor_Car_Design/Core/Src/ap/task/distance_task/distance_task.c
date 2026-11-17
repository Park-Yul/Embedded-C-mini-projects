/*
 * distance_task.c
 *
 *  Created on: 2026. 10. 29.
 *      Author: kccistc
 */

/* include */
#include "distance_task.h"
#include "tim.h"
#include "task.h"
#include <string.h>

/* define */

/* typedef */

/* variable declarations */

/* variable definitions */
distance_snapshot_t distances;
static volatile uint16_t active_echo;
volatile bool echo_in_flight;
volatile bool dht11_reserved;
static uint8_t next_sensor;
static distance_sensor_t measuring;
static uint32_t next_trigger;
static GPIO_TypeDef *const trig_ports[DISTANCE_COUNT] = {GPIOC, GPIOC, GPIOA};
static const uint16_t trig_pins[DISTANCE_COUNT] = {GPIO_PIN_13, GPIO_PIN_3, GPIO_PIN_4};
static const uint16_t echo_pins[DISTANCE_COUNT] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2};
static const distance_sensor_t scan_order[DISTANCE_COUNT] = {DISTANCE_CENTER, DISTANCE_RIGHT, DISTANCE_LEFT};

/* function declarations */
static void finish_measurement(sr04_status_t result, uint32_t echo_us);
static void distance_request(void);
static void distance_ISR(void);

/* function definitions */
void distance_task_init(void){
	taskENTER_CRITICAL();
	memset(&distances, 0, sizeof distances);
	for(unsigned i = 0; i < DISTANCE_COUNT; i++){
		distances.sensor[i].status = SR04_ERR_NOT_STARTED;
	}
	active_echo = 0;
	echo_in_flight = false;
	/* Do not clear dht11_reserved: TempHumidTask may already own the window. */
	taskEXIT_CRITICAL();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13 | GPIO_PIN_3, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	next_sensor = 0;
	next_trigger = HAL_GetTick();
}

void distance_task(void){
	if(echo_in_flight){
		uint32_t us = 0;
		sr04_status_t result = sr04_getResult(&us);
		if(result == SR04_IN_PROGRESS){
			return;
		}
		finish_measurement(result, us);
	}
	uint32_t now = HAL_GetTick();
	if((int32_t)(now - next_trigger) < 0){
		return;
	}
	distance_request();
}

static void finish_measurement(sr04_status_t result, uint32_t echo_us){
	distance_reading_t reading = {.measured_at = HAL_GetTick(), .status = result};
	if(result == SR04_OK){
		/* Preserve completed Echo distances even outside the nominal 2-400cm
		 * range, so control can distinguish near, far and missing Echo. */
		reading.centimetres = (float)echo_us / 58.0f;
		reading.valid = echo_us >= 116U && echo_us <= 23200U;
		if(!reading.valid){
			reading.status = SR04_ERR_OUT_OF_RANGE;
		}
	}
	taskENTER_CRITICAL();
	active_echo = 0;
	distances.sensor[measuring] = reading;
	echo_in_flight = false;
	taskEXIT_CRITICAL();
}

static void distance_request(void){
	taskENTER_CRITICAL();
	if(dht11_reserved){
		taskEXIT_CRITICAL();
		return;
	}
	echo_in_flight = true;
	taskEXIT_CRITICAL();
	measuring = scan_order[next_sensor];
	next_sensor = (uint8_t)((next_sensor + 1U) % DISTANCE_COUNT);
	/* The supplied driver is a singleton. Rebind only after consuming its
	 * previous result; unrelated sensor EXTI edges are ignored. */
	uint32_t mask = __get_PRIMASK();
	__disable_irq();
	active_echo = 0;
	sr04_init(trig_ports[measuring], trig_pins[measuring], GPIOC, echo_pins[measuring], &htim5);
	__HAL_GPIO_EXTI_CLEAR_IT(echo_pins[measuring]);
	active_echo = echo_pins[measuring];
	__set_PRIMASK(mask);
	sr04_status_t result = sr04_start();
	next_trigger = HAL_GetTick() + DISTANCE_TRIGGER_MS;
	if(result != SR04_IN_PROGRESS){
		finish_measurement(result, 0);
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t pin){
	if(active_echo != 0 && pin == active_echo){
		distance_ISR();
	}
}

static void distance_ISR(void){
	sr04_echoISR();
}
