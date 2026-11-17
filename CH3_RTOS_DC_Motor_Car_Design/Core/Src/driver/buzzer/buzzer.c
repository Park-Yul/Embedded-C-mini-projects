/*
 * buzzer.c
 *
 *  Created on: 2026. 10. 30.
 *      Author: kccistc
 */

/* include */
#include "buzzer.h"

/* define */

/* typedef */

/* variable declarations */

/* variable definitions */
static TIM_HandleTypeDef *buzzer1_htim;
static uint32_t buzzer1_channel;
static TIM_HandleTypeDef *buzzer2_htim;
static uint32_t buzzer2_channel;

/* function declarations */

/* function definitions */
void buzzer_init(TIM_HandleTypeDef * htim1, uint32_t ch1, TIM_HandleTypeDef * htim2, uint32_t ch2){
	buzzer1_htim = htim1;
	buzzer1_channel = ch1;
	buzzer2_htim = htim2;
	buzzer2_channel = ch2;
	__HAL_TIM_SET_COMPARE(buzzer1_htim, buzzer1_channel, 0);
	__HAL_TIM_SET_COMPARE(buzzer2_htim, buzzer2_channel, 0);
	HAL_TIM_PWM_Start(buzzer1_htim, buzzer1_channel);
	HAL_TIM_PWM_Start(buzzer2_htim, buzzer2_channel);
}

void buzzer1_off(void){
	__HAL_TIM_SET_COMPARE(buzzer1_htim, buzzer1_channel, 0);
}

void buzzer1_on(uint32_t freq_hz){
	if(freq_hz < 20) freq_hz = 20;
	if(freq_hz > 20000) freq_hz = 20000;
	uint32_t arr = 1000000 / freq_hz - 1;
	uint32_t ccr = (arr + 1) / 2;
	__HAL_TIM_SET_AUTORELOAD(buzzer1_htim, arr);
	__HAL_TIM_SET_COMPARE(buzzer1_htim, buzzer1_channel, ccr);
}

void buzzer2_off(void){
	__HAL_TIM_SET_COMPARE(buzzer2_htim, buzzer2_channel, 0);
}

void buzzer2_on(uint32_t freq_hz){
	if(freq_hz < 20) freq_hz = 20;
	if(freq_hz > 20000) freq_hz = 20000;
	uint32_t arr = 1000000 / freq_hz - 1;
	uint32_t ccr = (arr + 1) / 2;
	__HAL_TIM_SET_AUTORELOAD(buzzer2_htim, arr);
	__HAL_TIM_SET_COMPARE(buzzer2_htim, buzzer2_channel, ccr);
}
