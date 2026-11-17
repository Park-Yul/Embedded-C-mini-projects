/*
 * motor_control_task.c
 *
 *  Created on: 2026. 11. 12.
 *      Author: kccistc
 */

/* include */
#include "motor_control_task.h"
#include "../distance_task/distance_task.h"
#include "../../../driver/dc_motor/dc_motor.h"
#include "../../../driver/fnd/fnd.h"
#include "tim.h"
#include "task.h"

/* define */
#define AUTO_SPEED 550U
#define AUTO_REVERSE_CM 10.0f
#define AUTO_TURN_CM 20.0f
#define AUTO_ESCAPE_TRANSITIONS 10U /* Five forward/reverse pairs. */
#define AUTO_ESCAPE_MS 400U
#define AUTO_STABLE_MS 2000U

/* variable definitions */
static motor_t motor_r, motor_l;
motor_state_t motor_state = STOP;
motor_status_t applied = {.command = {DRIVE_MANUAL, STOP, 0, 0, DRIVE_FORWARD}};
static comm_data_t desired = {DRIVE_MANUAL, STOP, 0, 0, DRIVE_FORWARD};
static bool escape_active, direction_known;
static unsigned direction_changes;
static uint32_t direction_since, escape_started;
static drive_direction_t previous_direction;
static int16_t escape_turn, next_escape_turn = -1000;

/* function declarations */
static void distance_auto_control(comm_data_t *control);
static void distance_get_snapshot(distance_snapshot_t *snapshot);
static void auto_escape_control(comm_data_t *control);
static void auto_escape_reset(void);

/* function definitions */
void motor_control_task_init(void){
	auto_escape_reset();
	motor_init(&motor_r, &htim3, TIM_CHANNEL_1, GPIOC, GPIO_PIN_10, GPIOC, GPIO_PIN_12);
	motor_init(&motor_l, &htim3, TIM_CHANNEL_3, GPIOC, GPIO_PIN_11, GPIOD, GPIO_PIN_2);
	motor_stop(&motor_r, &motor_l);
	fnd_set_num(0);
}
void motor_control_task(void){
	if(commQueHandle != NULL){
		xQueueReceive(commQueHandle, &desired, 0);
	}
	motor_status_t next = {.command = desired};
	if(next.command.mode == DRIVE_AUTO && next.command.state == RUN){
		distance_auto_control(&next.command);
		if(next.command.speed != 0){
			auto_escape_control(&next.command);
		}
		else {
			auto_escape_reset();
		}
	}
	else {
		auto_escape_reset();
	}
	if(next.command.state == STOP){
		next.command.speed = 0;
		next.command.steering = 0;
	}
	int32_t left = next.command.speed;
	int32_t right = left;
	if(next.command.steering > 0){
		right = right * (1000 - next.command.steering) / 1000;
	}
	else if(next.command.steering < 0){
		left = left * (1000 + next.command.steering) / 1000;
	}
	if(next.command.direction == DRIVE_REVERSE){
		left = -left;
		right = -right;
	}
	next.left_output = (int16_t)left;
	next.right_output = (int16_t)right;
	motor_set_output(&motor_r, next.right_output);
	motor_set_output(&motor_l, next.left_output);
	fnd_set_num(next.command.speed);
	taskENTER_CRITICAL();
	next.stop_sequence = applied.stop_sequence;
	if(applied.command.state == RUN && next.command.state == STOP){
		next.stop_sequence++;
	}
	applied = next;
	motor_state = next.command.state;
	taskEXIT_CRITICAL();
}

static void distance_get_snapshot(distance_snapshot_t *snapshot){
	taskENTER_CRITICAL();
	*snapshot = distances;
	taskEXIT_CRITICAL();
}

static void auto_escape_reset(void){
    escape_active = direction_known = false;
    direction_changes = 0;
    next_escape_turn = -1000;
}

static void auto_escape_control(comm_data_t *control){
    uint32_t now = HAL_GetTick();
    if(escape_active){
        if(now - escape_started < AUTO_ESCAPE_MS){
            control->speed = AUTO_SPEED;
            control->direction = DRIVE_FORWARD;
            control->steering = escape_turn;
            return;
        }
        escape_active = direction_known = false;
        direction_changes = 0;
    }
    if(!direction_known){
        direction_known = true;
        previous_direction = control->direction;
        direction_since = now;
        return;
    }
    /* A sustained direction breaks the oscillation sequence. This is a
     * command-based heuristic; there is no wheel-position feedback. */
    if(now - direction_since >= AUTO_STABLE_MS){
        direction_changes = 0;
        previous_direction = control->direction;
        direction_since = now;
        return;
    }
    if(control->direction != previous_direction){
        previous_direction = control->direction;
        direction_since = now;
        if(++direction_changes >= AUTO_ESCAPE_TRANSITIONS){
            escape_active = true;
            escape_started = now;
            escape_turn = next_escape_turn;
            next_escape_turn = (int16_t)-next_escape_turn;
            direction_changes = 0;
            control->speed = AUTO_SPEED;
            control->direction = DRIVE_FORWARD;
            control->steering = escape_turn;
        }
    }
}

static void distance_auto_control(comm_data_t *control){
    distance_snapshot_t snapshot;
    distance_get_snapshot(&snapshot);
    control->speed = 0;
    control->steering = 0;
    control->direction = DRIVE_FORWARD;
    if(control->state != RUN || control->mode != DRIVE_AUTO){
        return;
    }
    bool no_echo = false;
    for(unsigned i = 0; i < DISTANCE_COUNT; i++){
        if(snapshot.sensor[i].status == SR04_ERR_NO_ECHO){
            no_echo = true;
            continue;
        }
        /* A completed out-of-range Echo still supplies a near/far distance.
         * Other measurement failures remain stop conditions. */
        if(!snapshot.sensor[i].valid && snapshot.sensor[i].status != SR04_ERR_OUT_OF_RANGE){
            return;
        }
    }
    if(no_echo){
        control->speed = AUTO_SPEED;
        control->direction = DRIVE_REVERSE;
        /* Missing-Echo priority: center straight back, left arc, right arc. */
        if(snapshot.sensor[DISTANCE_CENTER].status == SR04_ERR_NO_ECHO){
            control->steering = 0;
        }
        else if(snapshot.sensor[DISTANCE_LEFT].status == SR04_ERR_NO_ECHO){
            control->steering = -1000;
        }
        else {
            control->steering = 1000;
        }
        return;
    }
    float center = snapshot.sensor[DISTANCE_CENTER].centimetres;
    float left = snapshot.sensor[DISTANCE_LEFT].centimetres;
    float right = snapshot.sensor[DISTANCE_RIGHT].centimetres;
    control->speed = AUTO_SPEED;
    /* Priority: center reverse, left reverse arc, right reverse arc,
     * then forward obstacle avoidance. */
    if(center <= AUTO_REVERSE_CM){
        control->direction = DRIVE_REVERSE;
    }
    else if(left <= AUTO_REVERSE_CM){
        control->direction = DRIVE_REVERSE;
        control->steering = -1000;
    }
    else if(right <= AUTO_REVERSE_CM){
        control->direction = DRIVE_REVERSE;
        control->steering = 1000;
    }
    else if(right <= AUTO_TURN_CM && center <= AUTO_TURN_CM){
        control->steering = -1000;
    }
    else if(left <= AUTO_TURN_CM && center <= AUTO_TURN_CM){
        control->steering = 1000;
    }
}
