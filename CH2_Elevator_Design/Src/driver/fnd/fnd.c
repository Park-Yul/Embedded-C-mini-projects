/*
 * fnd.c
 *
 *  Created on: 2026. 10. 14.
 *      Author: kccistc
 */

// include
#include "fnd.h"

// define

// typedef

// variable declarations

// variable definitions
static uint16_t num = 0;
static uint16_t dp = 0;
static volatile uint8_t moveMode = 0;
static volatile uint8_t moveDigits[4] = {0, 0, 0, 0};

// function declarations
static void fnd_all_off(void);
static void fnd_sel_digit(int digit);
static void fnd_disp_number(int number);

// function definitions
void fnd_set_dp(int dig, uint16_t state)
{
	if(state == FND_DP_ON){
		dp |= (1<<dig);	// ex) dp |= (1<<FND_DIGIT); dp |= (1<<1); => 2bxx1x
	}
	else{
		dp &= ~(1<<dig); // ex) dp &= (1<<FND_DIGIT); dp &= ~(1<<1); => 2bxx0x
	}
}

uint16_t fnd_get_dp(void)
{
	return dp;
}

void fnd_set_num(uint16_t n)
{
    num = n;
    moveMode = 0;
}

void fnd_set_move(uint8_t up, uint8_t floor)
{
    uint8_t d1 = up ? 12 : 14;
    uint8_t d2 = up ? 13 : 15;
    uint8_t d4 = floor % 10;

    if(moveMode && moveDigits[0] == d1 && moveDigits[1] == d2 &&
       moveDigits[2] == 0 && moveDigits[3] == d4){
        return;
    }

    moveDigits[0] = d1;
    moveDigits[1] = d2;
    moveDigits[2] = 0;
    moveDigits[3] = d4;
    moveMode = 1;
}

uint16_t fnd_get_num(void)
{
	return num;
}

void fnd_show_number(void)
{
    static int state = 0;
    uint16_t number = fnd_get_num();
    int digit;

    switch(state){
    case 0: digit = (number / 1000) % 10; break;
    case 1: digit = (number / 100) % 10; break;
    case 2: digit = (number / 10) % 10; break;
    default: digit = number % 10; break;
    }

    if(moveMode){
        digit = moveDigits[state];
    }
    fnd_all_off();
    fnd_disp_number(digit);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10,
        (fnd_get_dp() & (1 << (state + 1))) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    fnd_sel_digit(state + 1);
    state = (state + 1) % 4;
}

static void fnd_all_off(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
}

static void fnd_sel_digit(int digit)
{
	switch(digit)
	{
	case 1:
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
		break;
	case 2:
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
		break;
	case 3:
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
		break;
	case 4 :
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		break;

	}
}

static void fnd_disp_number(int number)
{
	set_port_t seg_font[8] = {
			{GPIOA, GPIO_PIN_6},	// A
			{GPIOA, GPIO_PIN_9},	// B
			{GPIOB, GPIO_PIN_5},	// C
			{GPIOB, GPIO_PIN_4},	// D
			{GPIOA, GPIO_PIN_8},	// E
			{GPIOA, GPIO_PIN_7},	// F
			{GPIOB, GPIO_PIN_3},	// G
			{GPIOB, GPIO_PIN_10},	// DP
	};
	int seg_font_data[16][8] = {
			{1, 1, 1, 1, 1, 1, 0, 0}, // 0
			{0, 1, 1, 0, 0, 0, 0, 0}, // 1
			{1, 1, 0, 1, 1, 0, 1, 0}, // 2
			{1, 1, 1, 1, 0, 0, 1, 0}, // 3
			{0, 1, 1, 0, 0, 1, 1, 0}, // 4
			{1, 0, 1, 1, 0, 1, 1, 0}, // 5
			{1, 0, 1, 1, 1, 1, 1, 0}, // 6
			{1, 1, 1, 0, 0, 1, 0, 0}, // 7
			{1, 1, 1, 1, 1, 1, 1, 0}, // 8
			{1, 1, 1, 1, 0, 1, 1, 0}, // 9
			{0, 0, 0, 0, 0, 0, 0, 1}, // 10, dp on
			{0, 0, 0, 0, 0, 0, 0, 0}, // 11, dp off
            {0, 1, 1, 1, 1, 1, 0, 0}, // 12, U
            {1, 1, 0, 0, 1, 1, 1, 0}, // 13, P
            {0, 1, 1, 1, 1, 0, 1, 0}, // 14, d
            {1, 1, 1, 1, 1, 1, 0, 0}  // 15, O (same shape as 0)
	};

	for(int i=0; i<8 ;i++) {
		HAL_GPIO_WritePin(seg_font[i].GPIOx, seg_font[i].GPIO_Pin, seg_font_data[number][i]);
	}
}
