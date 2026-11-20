/*
 * AnalogClock.c
 *
 *  Created on: 2026. 10. 25.
 *      Author: kccistc
 */
// include
#include "AnalogClock.h"
#include "../../driver/StepMotor/StepMotor.h"

// variable definitions
static StepMotor_pin_t stepMotorPinSec = {
		.port = {GPIOA, GPIOA, GPIOA, GPIOB},
		.pin = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_4, GPIO_PIN_0}
};

static StepMotor_pin_t stepMotorPinMin = {
		.port = {GPIOC, GPIOC, GPIOC, GPIOC},
		.pin = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11}
};

static StepMotor_pin_t stepMotorPinHour = {
		.port = {GPIOC, GPIOC, GPIOB, GPIOB},
		.pin = {GPIO_PIN_12, GPIO_PIN_2, GPIO_PIN_1, GPIO_PIN_2}
};

static StepMotor_t motorSec = {.pin = &stepMotorPinSec, .stepCounter = 0};
static StepMotor_t motorMin = {.pin = &stepMotorPinMin, .stepCounter = 0};
static StepMotor_t motorHour = {.pin = &stepMotorPinHour, .stepCounter = 0};

static volatile uint16_t targetSec;
static volatile uint16_t targetMin;
static volatile uint16_t targetHour;
static uint16_t currentSec;
static uint16_t currentMin;
static uint16_t currentHour;
static uint8_t stepTick;

// function declarations
static void AnalogClock_moveOneStep(StepMotor_t *motor,
		uint16_t *current, uint16_t target);

// function definitions
void AnalogClock_init(void)
{
	targetSec = 0;
	targetMin = 0;
	targetHour = 0;
	currentSec = 0;
	currentMin = 0;
	currentHour = 0;
	stepTick = 0;

	StepMotor_release(&motorSec);
	StepMotor_release(&motorMin);
	StepMotor_release(&motorHour);
}

void AnalogClock_setTarget(uint8_t hour, uint8_t min, uint8_t sec)
{
	uint32_t minSeconds = ((uint32_t)(min % 60U) * 60U) + (sec % 60U);
	uint32_t hourSeconds = ((uint32_t)(hour % 12U) * 3600U) + minSeconds;
	uint16_t newSec = (uint16_t)(((uint32_t)(sec % 60U)
			* ANALOG_CLOCK_STEPS_PER_REV) / 60U);
	uint16_t newMin = (uint16_t)((minSeconds
			* ANALOG_CLOCK_STEPS_PER_REV) / 3600U);
	uint16_t newHour = (uint16_t)((hourSeconds
			* ANALOG_CLOCK_STEPS_PER_REV) / 43200U);
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	targetSec = newSec;
	targetMin = newMin;
	targetHour = newHour;
	if (primask == 0U) {
		__enable_irq();
	}
}

void AnalogClock_ISR(void)
{
	stepTick++;
	if (stepTick < ANALOG_CLOCK_STEP_INTERVAL_MS) {
		return;
	}
	stepTick = 0;

	AnalogClock_moveOneStep(&motorSec, &currentSec, targetSec);
	AnalogClock_moveOneStep(&motorMin, &currentMin, targetMin);
	AnalogClock_moveOneStep(&motorHour, &currentHour, targetHour);
}

static void AnalogClock_moveOneStep(StepMotor_t *motor,
		uint16_t *current, uint16_t target)
{
	uint16_t cwDistance;
	uint16_t ccwDistance;

	if (*current == target) {
		return;
	}

	cwDistance = (uint16_t)((target + ANALOG_CLOCK_STEPS_PER_REV - *current)
			% ANALOG_CLOCK_STEPS_PER_REV);
	ccwDistance = (uint16_t)((*current + ANALOG_CLOCK_STEPS_PER_REV - target)
			% ANALOG_CLOCK_STEPS_PER_REV);

	if (cwDistance <= ccwDistance) {
		StepMotor_StepPhase(motor, STEP_MOTOR_DIR_CW);
		*current = (uint16_t)((*current + 1U) % ANALOG_CLOCK_STEPS_PER_REV);
	}
	else {
		StepMotor_StepPhase(motor, STEP_MOTOR_DIR_CCW);
		*current = (uint16_t)((*current + ANALOG_CLOCK_STEPS_PER_REV - 1U)
				% ANALOG_CLOCK_STEPS_PER_REV);
	}
}
