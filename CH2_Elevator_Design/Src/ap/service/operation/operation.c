/*
 * operation.c
 *
 *  Created on: 2026. 11. 5.
 *      Author: kccistc
 */

#include "operation.h"
#include "../../../driver/StepMotor/StepMotor.h"
#include "../../../driver/joystick/joystick.h"

#define MOTOR1_IN4_PORT     GPIOC
#define MOTOR1_IN4_PIN      GPIO_PIN_10
#define MOTOR1_IN3_PORT     GPIOC
#define MOTOR1_IN3_PIN      GPIO_PIN_11
#define MOTOR1_IN2_PORT     GPIOC
#define MOTOR1_IN2_PIN      GPIO_PIN_12
#define MOTOR1_IN1_PORT     GPIOD
#define MOTOR1_IN1_PIN      GPIO_PIN_2

#define MOTOR2_IN4_PORT     GPIOC
#define MOTOR2_IN4_PIN      GPIO_PIN_8
#define MOTOR2_IN3_PORT     GPIOC
#define MOTOR2_IN3_PIN      GPIO_PIN_9
#define MOTOR2_IN2_PORT     GPIOB
#define MOTOR2_IN2_PIN      GPIO_PIN_12
#define MOTOR2_IN1_PORT     GPIOA
#define MOTOR2_IN1_PIN      GPIO_PIN_12

#define ADC_CENTER         2048
#define JOY_DEAD           500
#define STEP_TIME          2

#define DOOR_STEP_TIME     3
#define DOOR_OPEN_TIME     2000U
#define DOOR_CLOSE_TIME    2000U

typedef enum{
    MOTOR_UP,
    MOTOR_DOWN
}motor_state_t;

extern ADC_HandleTypeDef hadc1;

static StepMotor_pin_t stepMotorPinOp = {
    .port = {MOTOR1_IN1_PORT, MOTOR1_IN2_PORT, MOTOR1_IN3_PORT, MOTOR1_IN4_PORT},
    .pin = {MOTOR1_IN1_PIN, MOTOR1_IN2_PIN, MOTOR1_IN3_PIN, MOTOR1_IN4_PIN}
};

static StepMotor_t motorOp = {.pin = &stepMotorPinOp, .stepCounter = 0};

static StepMotor_pin_t stepMotorPinDoor = {
    .port = {MOTOR2_IN1_PORT, MOTOR2_IN2_PORT, MOTOR2_IN3_PORT, MOTOR2_IN4_PORT},
    .pin = {MOTOR2_IN1_PIN, MOTOR2_IN2_PIN, MOTOR2_IN3_PIN, MOTOR2_IN4_PIN}
};

static StepMotor_t motorDoor = {.pin = &stepMotorPinDoor, .stepCounter = 0};

volatile uint8_t doorMove = 0;
volatile uint8_t doorTimeout = 0;
volatile uint8_t debugDoorResult = 0;
volatile uint8_t debugDoorDir = 0;
volatile uint8_t debugDoorTestHeld = 0;
volatile uint8_t debugDoorPhase = 0;
volatile uint32_t debugDoorCWCount = 0;
volatile uint32_t debugDoorCCWCount = 0;
static volatile uint32_t doorStartTime = 0;
static volatile uint8_t doorCloseReady = 0;
static uint8_t emergencyArmed = 0;
static uint8_t emergencyNeutralSeen = 0;
static uint32_t emergencyNeutralSince = 0;
volatile uint8_t debugEmergencyCode = 0;
volatile uint8_t debugEmergencyArmed = 0;
volatile uint16_t debugEmergencyY = 2048;
/* function declarations */
void operation_idle(void){
	StepMotor_release(&motorOp);
}

void operation_task(int dir){
    static int prevTime = 0;
    if(HAL_GetTick() - prevTime < STEP_TIME){
        return;
    }
    prevTime = HAL_GetTick();
    if(dir == MOTOR_UP){
        StepMotor_StepPhase(&motorOp, STEP_MOTOR_DIR_CW);
    }
    else if(dir == MOTOR_DOWN){
        StepMotor_StepPhase(&motorOp, STEP_MOTOR_DIR_CCW);
    }
}

void operation_emergency_reset(void){
    emergencyArmed = 0;
    emergencyNeutralSeen = 0;
    emergencyNeutralSince = 0;
    debugEmergencyArmed = 0;
    debugEmergencyCode = 0;
    debugEmergencyY = 2048;
    StepMotor_release(&motorOp);
}

void operation_emergency_task(void){
    static uint32_t prevTime = 0;
    joystick_data_t joydata;
    int32_t y;
    uint32_t now = HAL_GetTick();

    if(now - prevTime < STEP_TIME){
        return;
    }
    prevTime = now;

    if(joystick_read(&joydata) != HAL_OK){
        debugEmergencyCode = 1;
        StepMotor_release(&motorOp);
        return;
    }

    debugEmergencyY = joydata.y;
    y = (int32_t)joydata.y - ADC_CENTER;

    if(!emergencyArmed){
        debugEmergencyCode = 2;
        StepMotor_release(&motorOp);
        if(y >= -JOY_DEAD && y <= JOY_DEAD){
            if(!emergencyNeutralSeen){
                emergencyNeutralSeen = 1;
                emergencyNeutralSince = now;
            }
            if(now - emergencyNeutralSince >= 150){
                emergencyArmed = 1;
                debugEmergencyArmed = 1;
            }
        }
        else{
            emergencyNeutralSeen = 0;
        }
        return;
    }

    if(y > JOY_DEAD){
        debugEmergencyCode = 4;
        StepMotor_StepPhase(&motorOp, STEP_MOTOR_DIR_CW);
    }
    else if(y < -JOY_DEAD){
        debugEmergencyCode = 5;
        StepMotor_StepPhase(&motorOp, STEP_MOTOR_DIR_CCW);
    }
    else{
        debugEmergencyCode = 3;
        StepMotor_release(&motorOp);
    }
}

void operation_init(void){
    StepMotor_release(&motorOp);
    StepMotor_release(&motorDoor);
    joystick_init(&hadc1);
}

void operation_door_open(void){
    doorTimeout = 0;
    doorCloseReady = 0;
    debugDoorResult = 6;
    doorStartTime = HAL_GetTick();
    doorMove = 1;
}

void operation_door_close(void){
    doorTimeout = 0;
    doorCloseReady = 0;
    debugDoorResult = 6;
    doorStartTime = HAL_GetTick();
    doorMove = 2;
}

uint8_t operation_door_is_closed(void){
    return (doorMove == 0 && doorCloseReady && doorTimeout == 0);
}

void operation_door_test_left(uint8_t pressed){
    debugDoorTestHeld = pressed ? 1U : 0U;
}

void operation_door_task(void){
    static uint32_t prevTime = 0;
    static uint8_t wasTesting = 0;
    uint32_t now = HAL_GetTick();

    if(debugDoorTestHeld){
        wasTesting = 1;
        if(now - prevTime >= DOOR_STEP_TIME){
            prevTime = now;
            StepMotor_StepPhase(&motorDoor, STEP_MOTOR_DIR_CCW);
            debugDoorDir = 3;
            debugDoorPhase = motorDoor.stepCounter;
            debugDoorCCWCount++;
        }
        return;
    }
    if(wasTesting){
        wasTesting = 0;
        StepMotor_release(&motorDoor);
    }

    if(doorMove == 0){
        return;
    }

    if(doorMove == 1 && now - doorStartTime >= DOOR_OPEN_TIME){
        doorMove = 0;
        StepMotor_release(&motorDoor);
        debugDoorResult = 8;
        return;
    }

    if(doorMove == 2 && now - doorStartTime >= DOOR_CLOSE_TIME){
        doorMove = 0;
        StepMotor_release(&motorDoor);
        doorCloseReady = 1;
        debugDoorResult = 4;
        return;
    }

    if(now - prevTime < DOOR_STEP_TIME){
        return;
    }
    prevTime = now;

    if(doorMove == 1){
        StepMotor_StepPhase(&motorDoor, STEP_MOTOR_DIR_CW);
        debugDoorDir = 1;
        debugDoorPhase = motorDoor.stepCounter;
        debugDoorCWCount++;
    }
    else if(doorMove == 2){
        StepMotor_StepPhase(&motorDoor, STEP_MOTOR_DIR_CCW);
        debugDoorDir = 2;
        debugDoorPhase = motorDoor.stepCounter;
        debugDoorCCWCount++;
    }
}

void operation_door_stop(void){
    debugDoorTestHeld = 0;
    debugDoorResult = 7;
    doorMove = 0;
    doorCloseReady = 0;
    StepMotor_release(&motorDoor);
}
