/*
 * AnalogClock.h
 *
 *  Created on: 2026. 10. 25.
 *      Author: kccistc
 */

#ifndef SRC_AP_ANALOGCLOCK_ANALOGCLOCK_H_
#define SRC_AP_ANALOGCLOCK_ANALOGCLOCK_H_

// include
#include "stm32f4xx_hal.h"

// define
#define ANALOG_CLOCK_STEPS_PER_REV       4096U
#define ANALOG_CLOCK_STEP_INTERVAL_MS    3U

// typedef

// variable declarations

// function declarations
void AnalogClock_init(void);
void AnalogClock_setTarget(uint8_t hour, uint8_t min, uint8_t sec);
void AnalogClock_ISR(void);

#endif /* SRC_AP_ANALOGCLOCK_ANALOGCLOCK_H_ */
