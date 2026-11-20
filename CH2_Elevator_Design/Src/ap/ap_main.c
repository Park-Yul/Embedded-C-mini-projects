/*
 * smart_watch.c
 *
 *  Created on: 2026. 10. 31.
 *      Author: kccistc
 */
/* include */
#include "ap_main.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "service/TimeClock/TimeClock.h"
#include "service/temp_humid/temp_humid.h"
#include "service/operation/operation.h"
#include "service/display/display.h"
#include "service/sound/sound.h"
#include "../driver/led/led.h"
#include "../driver/fnd/fnd.h"
#include "../driver/detection/detection.h"
#include "../driver/button/button.h"
#include "../driver/joystick/joystick.h"
#include "../driver/uart_protocol/uart_protocol.h"
#include "../common/queue/que_buffer.h"
#include "../common/delay/delay.h"

/* define */
#define START_BTN_PORT			GPIOA
#define START_BTN_PIN			GPIO_PIN_15
#define EMERG_BTN_PORT			GPIOA
#define EMERG_BTN_PIN			GPIO_PIN_1
#define FLOOR1_BTN_PORT			GPIOC
#define FLOOR1_BTN_PIN			GPIO_PIN_3
#define FLOOR2_BTN_PORT			GPIOC
#define FLOOR2_BTN_PIN			GPIO_PIN_1
#define FLOOR3_BTN_PORT			GPIOC
#define FLOOR3_BTN_PIN			GPIO_PIN_0
#define FLOOR1_DETECTION		GPIO_PIN_1
#define FLOOR2_DETECTION		GPIO_PIN_2
#define FLOOR3_DETECTION		GPIO_PIN_7
/* typedef */
typedef enum{
	STOP,
	READY,
	BUSY,
	EMERGENCY
}op_state_t;
typedef enum{
	UP,
	DOWN
}dir_state_t;
/* variable declarations */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern volatile uint8_t doorTimeout;
/* variable definitions */
static volatile op_state_t op_state = STOP;
static dir_state_t dir_state = UP;
static button_t btn_start, btn_emergency;
static button_t btn_floor_1, btn_floor_2, btn_floor_3;
static que_buf_t que_btnEvent;
static btn_event_t btnEvent[BTN_EVENT_Q_SIZE];
static uint8_t prev_floor = 1;
static volatile uint8_t cur_floor = 1;
static volatile bool sound_flag = false;
static uint8_t door_open = 1;
static uint8_t door_close_requested = 0;
static volatile uint8_t startEvent = 0;
static volatile uint8_t startBeepPending = 0;
volatile uint8_t debugStartRaw = 1;
volatile uint8_t debugStartEvent = 0;
volatile uint8_t debugDoorClosed = 0;
volatile uint8_t debugJoyStatus = 0;
volatile uint16_t debugJoyY = 0;
volatile uint8_t debugOpState = 0;
volatile uint8_t debugMotorGate = 0;
volatile uint8_t debugFloor1Raw = 1;
volatile uint8_t sensorFloor1 = 0;
volatile uint8_t sensorFloor2 = 0;
volatile uint8_t sensorFloor3 = 0;
volatile uint8_t sensorDoorOpen = 0;
volatile uint8_t sensorDoorClose = 0;
volatile uint8_t debugManualSource = 0;
volatile uint16_t debugJoyX = 0;
volatile uint32_t debugLiftSteps = 0;
volatile uint8_t debugFloorRawMask = 0;
volatile uint8_t debugFloorIrqMask = 0;
volatile uint8_t debugFloorStopReason = 0;
static volatile uint8_t floorHitPending = 0;
/* function declarations */

static void ap_init(void);

static void ap_btn(void);

static void ap_task(void);

static void ap_stop_mode(void);

static void ap_ready_mode(void);

static void ap_busy_mode(void);

static void ap_emergency_mode(void);
static void ap_sensor_task(void);

static void packet_do_func(uint8_t cmd, uint8_t *payload, uint8_t payload_len);
/* function definitions */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(htim->Instance == TIM3){
        static uint8_t lastRaw = 1;
        static uint8_t stable = 1;
        static uint8_t stableCount = 0;
        uint8_t raw = (HAL_GPIO_ReadPin(START_BTN_PORT, START_BTN_PIN) == GPIO_PIN_SET);

        debugStartRaw = raw;
        if(raw != lastRaw){
            lastRaw = raw;
            stableCount = 0;
        }
        else if(stableCount < 30){
            stableCount++;
        }
        if(stableCount == 30 && stable != raw){
            stable = raw;
            if(raw == 0){
                startEvent = 1;
                startBeepPending = 1;
                debugStartEvent++;
            }
        }
        operation_door_task();
        fnd_show_number();
    }
    else if(htim->Instance == TIM4){
        TimeClock_ISR();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    uint8_t floor = 0;

    if(GPIO_Pin == FLOOR1_DETECTION){
        floor = 1;
        debugFloorIrqMask |= 0x01;
    }
    else if(GPIO_Pin == FLOOR2_DETECTION){
        floor = 2;
        debugFloorIrqMask |= 0x02;
    }
    else if(GPIO_Pin == FLOOR3_DETECTION){
        floor = 3;
        debugFloorIrqMask |= 0x04;
    }

    if(op_state == BUSY && floor == cur_floor){
        floorHitPending = 1;
    }
}

int ap_main(void){
    ap_init();
    while(1){
        ap_sensor_task();
        debugFloor1Raw = (HAL_GPIO_ReadPin(FLOOR1_BTN_PORT, FLOOR1_BTN_PIN) == GPIO_PIN_SET);
        ap_btn();
        ap_task();
        if(startBeepPending){
            startBeepPending = 0;
            sound_button();
        }
        TimeClock_task();
        if(op_state != BUSY){
            TH_task();
        }

        joystick_data_t joy;
        debugJoyStatus = (joystick_read(&joy) == HAL_OK);
        if(debugJoyStatus){
            debugJoyY = joy.y;
        }
        debugDoorClosed = operation_door_is_closed();
        debugOpState = op_state;

        if(op_state == EMERGENCY){
            fnd_set_num(8888);
            fnd_set_dp(FND_DIGIT_1, FND_DP_ON);
            fnd_set_dp(FND_DIGIT_2, FND_DP_ON);
            fnd_set_dp(FND_DIGIT_3, FND_DP_ON);
            fnd_set_dp(FND_DIGIT_4, FND_DP_ON);
        }
        else if(op_state == READY || op_state == BUSY){
            fnd_set_move(dir_state == UP, cur_floor);
            fnd_set_dp(FND_DIGIT_1, FND_DP_OFF);
            fnd_set_dp(FND_DIGIT_2, FND_DP_OFF);
            fnd_set_dp(FND_DIGIT_3, FND_DP_OFF);
            fnd_set_dp(FND_DIGIT_4, FND_DP_OFF);
        }
        else{
            TimeClock_t *pTime = TimeClock_getTimeData();
            fnd_set_num(pTime->hour * 100 + pTime->min);
            fnd_set_dp(FND_DIGIT_1, FND_DP_OFF);
            fnd_set_dp(FND_DIGIT_2, FND_DP_ON);
            fnd_set_dp(FND_DIGIT_3, FND_DP_OFF);
            fnd_set_dp(FND_DIGIT_4, FND_DP_OFF);
        }
    }
}

static void ap_init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = FLOOR1_BTN_PIN|FLOOR2_BTN_PIN|FLOOR3_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = START_BTN_PIN|EMERG_BTN_PIN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    button_init(&btn_start, START_BTN_PORT, START_BTN_PIN);
    button_init(&btn_floor_1, FLOOR1_BTN_PORT, FLOOR1_BTN_PIN);
    button_init(&btn_floor_2, FLOOR2_BTN_PORT, FLOOR2_BTN_PIN);
    button_init(&btn_floor_3, FLOOR3_BTN_PORT, FLOOR3_BTN_PIN);
    button_init(&btn_emergency, EMERG_BTN_PORT, EMERG_BTN_PIN);
    TimeClock_init();
    TH_init();
    operation_init();
    disp_init();
    delay_init();
    que_buf_init(&que_btnEvent, btnEvent, sizeof(btn_event_t), BTN_EVENT_Q_SIZE);
    sound_init();
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start_IT(&htim4);
}

static void ap_sensor_task(void){
    static uint8_t prevRaw = 0;
    static uint32_t changeTime = 0;
    uint8_t floor = 0;
    uint8_t targetActive = 0;
    uint32_t now = HAL_GetTick();

    sensorFloor1 = (HAL_GPIO_ReadPin(GPIOB, FLOOR1_DETECTION) == GPIO_PIN_SET);
    sensorFloor2 = (HAL_GPIO_ReadPin(GPIOC, FLOOR2_DETECTION) == GPIO_PIN_SET);
    sensorFloor3 = (HAL_GPIO_ReadPin(GPIOB, FLOOR3_DETECTION) == GPIO_PIN_SET);
    sensorDoorOpen = 0;
    sensorDoorClose = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);

    debugFloorRawMask = (sensorFloor1 ? 0x01 : 0) |
                        (sensorFloor2 ? 0x02 : 0) |
                        (sensorFloor3 ? 0x04 : 0);

    if(debugFloorRawMask == 0x01) floor = 1;
    else if(debugFloorRawMask == 0x02) floor = 2;
    else if(debugFloorRawMask == 0x04) floor = 3;

    if(floor != prevRaw){
        prevRaw = floor;
        changeTime = now;
    }

    if(floor != 0 && now - changeTime >= 15){
        if(op_state == BUSY){
            if(floor != cur_floor) prev_floor = floor;
        }
        else if(debugFloorStopReason == 0){
            prev_floor = floor;
        }
    }

    if(op_state != BUSY) return;

    if(cur_floor == 1) targetActive = sensorFloor1;
    else if(cur_floor == 2) targetActive = sensorFloor2;
    else if(cur_floor == 3) targetActive = sensorFloor3;

    if(floorHitPending || targetActive){
        debugFloorStopReason = floorHitPending ? 2 : 1;
        floorHitPending = 0;
        operation_idle();
        prev_floor = cur_floor;
        op_state = STOP;
        door_close_requested = 0;
        sound_flag = true;
        operation_door_open();
    }
}

static void ap_btn(void){
    static uint32_t prevTime = 0;
    static btn_event_t event;
    uint8_t selected = 0;

    if(button_getState(&btn_floor_1) == ACT_PUSH){
        event = BTN_FLOOR_1;
        que_put(&que_btnEvent, &event);
    }
    if(button_getState(&btn_floor_2) == ACT_PUSH){
        event = BTN_FLOOR_2;
        que_put(&que_btnEvent, &event);
    }
    if(button_getState(&btn_floor_3) == ACT_PUSH){
        event = BTN_FLOOR_3;
        que_put(&que_btnEvent, &event);
    }
    if(HAL_GetTick() - prevTime < 100 || que_is_empty(&que_btnEvent)){
        return;
    }
    prevTime = HAL_GetTick();
    que_get(&que_btnEvent, &event);

    if(op_state != STOP && op_state != READY){
        return;
    }
    if(event == BTN_FLOOR_1) selected = 1;
    else if(event == BTN_FLOOR_2) selected = 2;
    else if(event == BTN_FLOOR_3) selected = 3;

    if(selected == 0){
        return;
    }
    if(selected == prev_floor){
        sound_button();
        if(op_state == READY && !door_close_requested){
            cur_floor = prev_floor;
            op_state = STOP;
        }
        return;
    }
    cur_floor = selected;
    dir_state = (cur_floor > prev_floor) ? UP : DOWN;
    op_state = READY;
    sound_button();
}

static void ap_task(void){
    switch(op_state){
    case STOP: ap_stop_mode(); break;
    case READY: ap_ready_mode(); break;
    case BUSY: ap_busy_mode(); break;
    case EMERGENCY: ap_emergency_mode(); break;
    default: ap_stop_mode(); break;
    }
}

static void ap_stop_mode(void){
    static uint32_t prevLcdTime = 0;
    debugMotorGate = 0;
    if(sound_flag){
        sound_flag = false;
        sound_alarm();
    }
    if(button_getState(&btn_emergency) == ACT_PUSH){
        op_state = EMERGENCY;
        operation_door_stop();
        door_close_requested = 0;
        operation_idle();
        return;
    }
    startEvent = 0;
    operation_idle();
    disp_stop();

    if(HAL_GetTick() - prevLcdTime >= 200){
        prevLcdTime = HAL_GetTick();
        TimeClock_t *pTime = TimeClock_getTimeData();
        TimeDate_t *pDate = TimeClock_getDateData();
        disp_setTimeDate((TimeClockTime_t *)pTime, (TimeClockDate_t *)pDate);
        disp_setTempHumid(TH_getTempHumid());
    }
}

static void ap_ready_mode(void){
    static uint32_t prevLcdTime = 0;
    debugMotorGate = 1;
    if(button_getState(&btn_emergency) == ACT_PUSH){
        op_state = EMERGENCY;
        operation_door_stop();
        door_close_requested = 0;
        operation_idle();
        return;
    }
    if(startEvent){
        startEvent = 0;
        if(!door_close_requested){
            operation_door_close();
            door_close_requested = 1;
        }
    }
    if(door_close_requested){
        if(doorTimeout){
            door_close_requested = 0;
            debugMotorGate = 5;
            operation_idle();
            return;
        }
        if(operation_door_is_closed()){
            door_close_requested = 0;
            floorHitPending = 0;
            debugFloorIrqMask = 0;
            debugFloorStopReason = 0;
            op_state = BUSY;
            debugMotorGate = 0;
            return;
        }
        debugMotorGate = 2;
    }
    operation_idle();
    disp_ready(cur_floor);
    if(HAL_GetTick() - prevLcdTime >= 200){
        prevLcdTime = HAL_GetTick();
        TimeClock_t *pTime = TimeClock_getTimeData();
        TimeDate_t *pDate = TimeClock_getDateData();
        disp_setTimeDate((TimeClockTime_t *)pTime, (TimeClockDate_t *)pDate);
        disp_setTempHumid(TH_getTempHumid());
    }
}

static void ap_busy_mode(void){
    static uint32_t prevLcdTime = 0;
    startEvent = 0;
    if(button_getState(&btn_emergency) == ACT_PUSH){
        op_state = EMERGENCY;
        operation_door_stop();
        door_close_requested = 0;
        operation_idle();
        return;
    }
    if(floorHitPending){
        operation_idle();
        return;
    }
    if(!operation_door_is_closed()){
        operation_idle();
        debugMotorGate = 3;
        op_state = READY;
        door_close_requested = 0;
        return;
    }
    debugMotorGate = 0;
    operation_task(dir_state);
    debugLiftSteps++;
    disp_busy(cur_floor);
    if(HAL_GetTick() - prevLcdTime >= 200){
        prevLcdTime = HAL_GetTick();
        disp_setTempHumid(TH_getTempHumid());
    }
}

static void ap_emergency_mode(void){
    static uint8_t entered = 0;
    static uint8_t joyArmed = 0;
    static uint32_t neutralSince = 0;
    joystick_data_t joy;
    int32_t y = 0;
    int32_t x = 0;
    uint32_t now = HAL_GetTick();
    uint8_t doorLeftPressed = (HAL_GPIO_ReadPin(FLOOR1_BTN_PORT, FLOOR1_BTN_PIN) == GPIO_PIN_RESET);
    uint8_t upPressed = (HAL_GPIO_ReadPin(FLOOR2_BTN_PORT, FLOOR2_BTN_PIN) == GPIO_PIN_RESET);
    uint8_t downPressed = (HAL_GPIO_ReadPin(FLOOR3_BTN_PORT, FLOOR3_BTN_PIN) == GPIO_PIN_RESET);

    if(!entered){
        entered = 1;
        joyArmed = 0;
        neutralSince = 0;
        operation_idle();
    }
    if(startEvent){
        startEvent = 0;
        operation_door_test_left(0);
        operation_idle();
        debugManualSource = 0;
        entered = 0;
        op_state = STOP;
        return;
    }

    operation_door_test_left(doorLeftPressed);
    debugMotorGate = 0;
    debugManualSource = 0;
    if(doorLeftPressed){
        debugManualSource = 7;
        operation_idle();
    }
    else if(upPressed && !downPressed){
        debugManualSource = 1;
        operation_task(UP);
        debugLiftSteps++;
    }
    else if(downPressed && !upPressed){
        debugManualSource = 2;
        operation_task(DOWN);
        debugLiftSteps++;
    }
    else if(joystick_read(&joy) == HAL_OK){
        debugJoyX = joy.x;
        debugJoyY = joy.y;
        debugJoyStatus = 1;
        y = (int32_t)joy.y - 2048;
        x = (int32_t)joy.x - 2048;
        if(!joyArmed){
            operation_idle();
            if(x >= -650 && x <= 650 && y >= -650 && y <= 650){
                if(neutralSince == 0){
                    neutralSince = now;
                }
                else if(now - neutralSince >= 150){
                    joyArmed = 1;
                }
            }
            else{
                neutralSince = 0;
            }
        }
        else if(y > 650){
            debugManualSource = 3;
            operation_task(UP);
            debugLiftSteps++;
        }
        else if(y < -650){
            debugManualSource = 4;
            operation_task(DOWN);
            debugLiftSteps++;
        }
        else if(x > 650){
            debugManualSource = 5;
            operation_task(UP);
            debugLiftSteps++;
        }
        else if(x < -650){
            debugManualSource = 6;
            operation_task(DOWN);
            debugLiftSteps++;
        }
        else{
            operation_idle();
        }
    }
    else{
        debugJoyStatus = 0;
        operation_idle();
    }
    disp_emergency();
}

static void packet_do_func(uint8_t cmd, uint8_t *payload, uint8_t payload_len){
	switch(cmd)
	{
	case CMD_LED0_SET:{
		uint8_t send_payload;
		if(payload_len>=1){
			if(payload[0] == 1){
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
			}
			else{
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
			}
			send_payload = RSP_OK;
		}
		else{
			send_payload = RSP_ERR;
		}
		UART_protocol_send((cmd + 0x80), &send_payload, 1);
		break;
	}
	case CMD_LED1_SET:{
		uint8_t send_payload;
		if(payload_len>=1){
			if(payload[0] == 1){
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
			}
			else{
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
			}
			send_payload = RSP_OK;
		}
		else{
			send_payload = RSP_ERR;
		}
		UART_protocol_send((cmd + 0x80), &send_payload, 1);
		break;
	}
	case CMD_LED2_SET:{
		uint8_t send_payload;
		if(payload_len>=1){
			if(payload[0] == 1){
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
			}
			else{
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
			}
			send_payload = RSP_OK;
		}
		else{
			send_payload = RSP_ERR;
		}
		UART_protocol_send((cmd + 0x80), &send_payload, 1);
		break;
	}
	case CMD_LED3_SET:{
		uint8_t send_payload;
		if(payload_len>=1){
			if(payload[0] == 1){
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
			}
			else{
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
			}
			send_payload = RSP_OK;
		}
		else{
			send_payload = RSP_ERR;
		}
		UART_protocol_send((cmd + 0x80), &send_payload, 1);
		break;
	}
	case CMD_LED_STATUS_REQ:{
		uint8_t send_payload[4];
		for (int i=0; i<4; i++){
			//			send_payload[i] = HAL_GPIO_ReadPin(led[i].port, led[i].pin);
		}
		send_payload[0] = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_SET) ? 1 : 0;
		send_payload[1] = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_SET) ? 1 : 0;
		send_payload[2] = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_SET) ? 1 : 0;
		send_payload[3] = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_SET) ? 1 : 0;
		UART_protocol_send((cmd + 0x80), send_payload, 4);
		break;
	}
	case CMD_TEMP_HUMID_REQ:{
		uint8_t dummy_temp_int = 0x15;	// 21
		uint8_t dummy_temp_dec = 0x01;	// 1
		uint8_t dummy_humid_int = 0x30;	// 48
		uint8_t dummy_humid_dec = 0x00;	// 0
		uint8_t send_payload[4];
		send_payload[0] = dummy_temp_int;
		send_payload[1] = dummy_temp_dec;
		send_payload[2] = dummy_humid_int;
		send_payload[3] = dummy_humid_dec;
		UART_protocol_send((cmd + 0x80), send_payload, 4);
		break;
	}
	case CMD_DISTANCE_REQ:{
		uint16_t dummy_distance = 0x01fa;
		uint8_t send_payload[2];
		send_payload[0] = (uint8_t)((uint16_t)dummy_distance >> 8);
		send_payload[1] = (uint8_t)((uint16_t)dummy_distance & 0x00ff);
		UART_protocol_send((cmd + 0x80), send_payload, 2);
		break;
	}
	}
}
