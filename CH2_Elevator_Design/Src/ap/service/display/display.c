/*
 * display.c
 *
 *  Created on: 2026. 11. 5.
 *      Author: kccistc
 */

/* include */
#include "display.h"
#include "../../../driver/led/led.h"
#include "../../../driver/fnd/fnd.h"
#include "../../../driver/lcd/lcd.h"
#include "../../../driver/rtc/rtc.h"
#include "../../../driver/dht11/dht11.h"

/* define */
#define FLOOR1_LED_PORT			GPIOB
#define FLOOR1_LED_PIN			GPIO_PIN_15
#define FLOOR2_LED_PORT			GPIOB
#define FLOOR2_LED_PIN			GPIO_PIN_14
#define FLOOR3_LED_PORT			GPIOB
#define FLOOR3_LED_PIN			GPIO_PIN_13
#define EMERG_LED_PORT			GPIOC
#define EMERG_LED_PIN			GPIO_PIN_4

/* typedef */
typedef enum{
	LED_STOP,
	LED_READY,
	LED_BUSY,
	LED_EMERGENCY
}led_state_t;



/* variable declarations */
extern I2C_HandleTypeDef hi2c1;

/* variable definitions */
static led_t led_floor_1, led_floor_2, led_floor_3;
static led_t led_emergency;
static TimeClockTime_t *pTimeClockTime;
static TimeClockDate_t *pTimeClockDate;
static dht11_data_t *pTempHumid;

/* function declarations */
static void disp_led_init(void);
static void disp_lcd_init(void);
static void disp_time_date(void);
static void disp_temp_humid(void);

/* function definitions */
void disp_init(void){
	Lcd_init(&hi2c1);
	led_init(&led_floor_1, FLOOR1_LED_PORT, FLOOR1_LED_PIN);
	led_init(&led_floor_2, FLOOR2_LED_PORT, FLOOR2_LED_PIN);
	led_init(&led_floor_3, FLOOR3_LED_PORT, FLOOR3_LED_PIN);
	led_init(&led_emergency, EMERG_LED_PORT, EMERG_LED_PIN);
	disp_led_init();
	disp_lcd_init();
}

static void disp_led_init(void){
	led_on(&led_floor_1);
	led_on(&led_floor_2);
	led_on(&led_floor_3);
	led_on(&led_emergency);
}

static void disp_lcd_init(void){
	char buff1[30], buff2[30];
	sprintf(buff1, "Welcome to");
	sprintf(buff2, "Elevator");
	Lcd_writeStringXY(0, 0, buff1);
	Lcd_writeStringXY(0, 1, buff2);
}

void disp_setTimeDate(TimeClockTime_t *pTime, TimeClockDate_t *pDate){
	pTimeClockTime = pTime;
	pTimeClockDate = pDate;
	disp_time_date();
}

void disp_setTempHumid(dht11_data_t *pTH){
	pTempHumid = pTH;
	disp_temp_humid();
}

static void disp_time_date(void){
	char Str[30];
	sprintf(Str, "%02d/%02d/%02d %02d:%02d", pTimeClockDate->year, pTimeClockDate->month, pTimeClockDate->date, pTimeClockTime->hour, pTimeClockTime->min);
	Lcd_writeStringXY(0, 0, Str);
}

static void disp_temp_humid(void){
	char Str[30];
	sprintf(Str, "T:%02dC H:%02d%%     ", pTempHumid->t_int, pTempHumid->h_int);
	Lcd_writeStringXY(0, 1, Str);
}

void disp_stop(void){
	led_off(&led_floor_1);
	led_off(&led_floor_2);
	led_off(&led_floor_3);
	led_off(&led_emergency);

	//fnd_set_num(prev_floor);

}

void disp_ready(uint8_t floor){
	static int prevTime;
	if(HAL_GetTick() - prevTime < 500){
		return;
	}
	prevTime = HAL_GetTick();
	switch(floor){
	case 1:
		led_toggle(&led_floor_1);
		led_off(&led_floor_2);
		led_off(&led_floor_3);
		led_off(&led_emergency);
		break;
	case 2:
		led_off(&led_floor_1);
		led_toggle(&led_floor_2);
		led_off(&led_floor_3);
		led_off(&led_emergency);
		break;
	case 3:
		led_off(&led_floor_1);
		led_off(&led_floor_2);
		led_toggle(&led_floor_3);
		led_off(&led_emergency);
		break;
	default:
		led_toggle(&led_floor_1);
		led_off(&led_floor_2);
		led_off(&led_floor_3);
		led_off(&led_emergency);
		break;
	}

	//fnd_set_num(prev_floor);

}

void disp_busy(uint8_t floor){
	switch(floor){
	case 1:
		led_on(&led_floor_1);
		led_off(&led_floor_2);
		led_off(&led_floor_3);
		led_off(&led_emergency);
		break;
	case 2:
		led_off(&led_floor_1);
		led_on(&led_floor_2);
		led_off(&led_floor_3);
		led_off(&led_emergency);
		break;
	case 3:
		led_off(&led_floor_1);
		led_off(&led_floor_2);
		led_on(&led_floor_3);
		led_off(&led_emergency);
		break;
	default:
		led_toggle(&led_floor_1);
		led_off(&led_floor_2);
		led_off(&led_floor_3);
		led_off(&led_emergency);
		break;
	}

	//fnd_set_num(prev_floor);

}

void disp_move(uint8_t floor, uint8_t up){
    char Str[20];
    sprintf(Str, "%s %02u           ", up ? "UP" : "DO", floor);
    Lcd_writeStringXY(0, 0, Str);
}

void disp_emergency(void){
	static int prevTime;
	if(HAL_GetTick() - prevTime < 500){
		return;
	}
	prevTime = HAL_GetTick();
	led_off(&led_floor_1);
	led_off(&led_floor_2);
	led_off(&led_floor_3);
	led_toggle(&led_emergency);

	char Str[30];
	sprintf(Str, "EMERGENCY!!!!!!!");
	Lcd_writeStringXY(0, 0, Str);
	Lcd_writeStringXY(0, 1, Str);

	fnd_set_num(8888);
	fnd_set_dp(FND_DIGIT_1, FND_DP_ON);
	fnd_set_dp(FND_DIGIT_2, FND_DP_ON);
	fnd_set_dp(FND_DIGIT_3, FND_DP_ON);
	fnd_set_dp(FND_DIGIT_4, FND_DP_ON);
}
