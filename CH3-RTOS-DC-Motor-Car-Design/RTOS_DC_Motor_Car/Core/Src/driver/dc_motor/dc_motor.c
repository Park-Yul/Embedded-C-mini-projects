/*
 * dc_motor.c
 *
 *  Created on: 2026. 11. 11.
 *      Author: kccistc
 */

/* include */
#include "dc_motor.h"
#include <stdbool.h>

/* define */

/* typedef */

/* variable declarations */

/* variable definitions */
static int16_t motor_speed;

/* function declarations */

/* function definitions */
void motor_init(motor_t *motor, TIM_HandleTypeDef *htim, uint32_t Channel, GPIO_TypeDef *GPIO1, uint16_t GPIO1_Pin,
    GPIO_TypeDef *GPIO2, uint16_t GPIO2_Pin){
	motor->htim = htim;
	motor->Channel = Channel;
	motor->GPIO1 = GPIO1;
	motor->GPIO1_Pin = GPIO1_Pin;
	motor->GPIO2 = GPIO2;
	motor->GPIO2_Pin = GPIO2_Pin;
	motor->output = 0;
	__HAL_TIM_SET_COMPARE(htim, Channel, 0);
	HAL_TIM_PWM_Stop(htim, Channel);
	HAL_GPIO_WritePin(GPIO1, GPIO1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIO2, GPIO2_Pin, GPIO_PIN_RESET);
}

void motor_set_speed(int16_t speed){
	if(speed > 1000){
		speed = 1000;
	}
	else if(speed < 0){
		speed = 0;
	}
	motor_speed = speed;
}

void motor_set_output(motor_t *motor, int16_t output){
	if(output > 1000){
		output = 1000;
	}
	if(output < -1000){
		output = -1000;
	}
	if(output == motor->output){
		return;
	}
	bool changing_direction = (output > 0 && motor->output < 0) || (output < 0 && motor->output > 0);
	if(output == 0 || changing_direction){
		/* Disable the Channel before touching direction pins: CCR is preloaded. */
		HAL_TIM_PWM_Stop(motor->htim, motor->Channel);
		__HAL_TIM_SET_COMPARE(motor->htim, motor->Channel, 0);
		HAL_GPIO_WritePin(motor->GPIO1, motor->GPIO1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(motor->GPIO2, motor->GPIO2_Pin, GPIO_PIN_RESET);
	}
	if(output != 0){
		uint32_t magnitude = (uint32_t)(output < 0 ? -output : output);
		uint32_t compare = magnitude * (__HAL_TIM_GET_AUTORELOAD(motor->htim) + 1U) / 1000U;
		HAL_GPIO_WritePin(motor->GPIO1, motor->GPIO1_Pin, output < 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
		HAL_GPIO_WritePin(motor->GPIO2, motor->GPIO2_Pin, output > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(motor->htim, motor->Channel, compare);
		if(motor->output == 0 || changing_direction){
			/* Load the new compare before re-enabling this Channel. TIM3 has
			 * a fixed period shared by the two motors; neither clock nor ARR changes. */
			motor->htim->Instance->EGR = TIM_EGR_UG;
			HAL_TIM_PWM_Start(motor->htim, motor->Channel);
		}
	}
	motor->output = output;
}

/* Existing driver entry points remain usable by other application code. */
void motor_stop(motor_t *right, motor_t *left){
	motor_set_output(right, 0);
	motor_set_output(left, 0);
}
void motor_forward(motor_t *right, motor_t *left){
	motor_set_output(right, motor_speed);
	motor_set_output(left, motor_speed);
}
void motor_backward(motor_t *right, motor_t *left){
	motor_set_output(right, -motor_speed);
	motor_set_output(left, -motor_speed);
}
void motor_right(motor_t *right, motor_t *left){
	motor_set_output(right, 0);
	motor_set_output(left, motor_speed);
}
void motor_left(motor_t *right, motor_t *left){
	motor_set_output(right, motor_speed);
	motor_set_output(left, 0);
}
