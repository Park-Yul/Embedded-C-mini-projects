/*
 * lcd.c
 *
 *  Created on: 2026. 10. 21.
 *      Author: kccistc
 */

/* include */
#include "lcd.h"
#include "cmsis_os.h"

/* define */
#define LCD_RS 0
#define LCD_RW 1
#define LCD_EN 2
#define LCD_BL 3
#define LCD_TIMEOUT_MS 5U

/* typedef */

/* variable declarations */

/* variable definitions */
static I2C_HandleTypeDef *lcd_hi2c;
static uint8_t lcdData;
static bool display_cleared;
volatile HAL_StatusTypeDef lcd_last_status = HAL_OK;

/* function declarations */
static bool Lcd_sendData(uint8_t data);
static void Lcd_cmdMode(void);
static void Lcd_dataMode(void);
static void Lcd_writeMode(void);
static bool Lcd_enHigh(void);
static bool Lcd_enLow(void);
static void Lcd_BacklightOn(void);
static bool Lcd_writeNibble(uint8_t data);
static bool Lcd_writeByte(uint8_t data);
static bool Lcd_writeCmdData(uint8_t data);
static bool Lcd_writeCharData(uint8_t data);
static bool Lcd_writeString(const char *str);
static bool Lcd_gotoXY(uint8_t col, uint8_t row);

/* function definitions */
bool Lcd_init(I2C_HandleTypeDef *hi2c){
	lcd_hi2c = hi2c;
	lcdData = 0;
	Lcd_cmdMode();
	Lcd_writeMode();
	Lcd_BacklightOn();
	osDelay(41);
	if(!Lcd_writeNibble(0x30)){
		return false;
	}
	osDelay(6);
	if(!Lcd_writeNibble(0x30)){
		return false;
	}
	osDelay(1);
	if(!Lcd_writeNibble(0x30)){
		return false;
	}
	if(!Lcd_writeNibble(0x20)){
		return false;
	}
	if(!Lcd_writeByte(0x28)){
		return false;
	}
	if(!Lcd_writeByte(0x08)){
		return false;
	}
	/* Reinitialization must not erase the other display regions. */
	if(!display_cleared){
		if(!Lcd_writeByte(0x01)){
			return false;
		}
		display_cleared = true;
		osDelay(3);
	}
	if(!Lcd_writeByte(0x06)){
		return false;
	}
	return Lcd_writeByte(0x0C);
}

static bool Lcd_sendData(uint8_t data){
	if(lcd_hi2c == NULL){
		lcd_last_status = HAL_ERROR;
		return false;
	}
	uint32_t started = HAL_GetTick();
	while(__HAL_I2C_GET_FLAG(lcd_hi2c, I2C_FLAG_BUSY)){
		if(HAL_GetTick() - started >= LCD_TIMEOUT_MS){
			lcd_last_status = HAL_TIMEOUT;
			return false;
		}
	}
	lcd_last_status = HAL_I2C_Master_Transmit(lcd_hi2c, 0x27U << 1, &data, 1, LCD_TIMEOUT_MS);
	return lcd_last_status == HAL_OK;
}

static void Lcd_cmdMode(void){
	lcdData &= ~(1U << LCD_RS);
}
static void Lcd_dataMode(void){
	lcdData |= (1U << LCD_RS);
}
static void Lcd_writeMode(void){
	lcdData &= ~(1U << LCD_RW);
}
static bool Lcd_enHigh(void){
	lcdData |= (1U << LCD_EN);
	return Lcd_sendData(lcdData);
}
static bool Lcd_enLow(void){
	lcdData &= ~(1U << LCD_EN);
	return Lcd_sendData(lcdData);
}
static void Lcd_BacklightOn(void){
	lcdData |= (1U << LCD_BL);
}

static bool Lcd_writeNibble(uint8_t data){
	lcdData = (lcdData & 0x0FU) | (data & 0xF0U);
	/* Set data with EN low before issuing the complete latch pulse. */
	return Lcd_enLow() && Lcd_enHigh() && Lcd_enLow();
}

static bool Lcd_writeByte(uint8_t data){
	if(!Lcd_writeNibble(data)){
		return false;
	}
	return Lcd_writeNibble((uint8_t)(data << 4));
}

static bool Lcd_writeCmdData(uint8_t data){
	Lcd_cmdMode();
	Lcd_writeMode();
	return Lcd_writeByte(data);
}

static bool Lcd_writeCharData(uint8_t data){
	Lcd_dataMode();
	Lcd_writeMode();
	return Lcd_writeByte(data);
}

static bool Lcd_writeString(const char *str){
	for(uint8_t i = 0; str[i]; i++){
		if(!Lcd_writeCharData((uint8_t)str[i])){
			return false;
		}
	}
	return true;
}

static bool Lcd_gotoXY(uint8_t col, uint8_t row){
	uint8_t lcdRegAddr = (0x40U * row) + col;
	return Lcd_writeCmdData(0x80U + lcdRegAddr);
}

bool Lcd_writeStringXY(uint8_t col, uint8_t row, const char *str){
	if(col >= 16U || row >= 2U || str == NULL){
		return false;
	}
	if(!Lcd_gotoXY(col, row)){
		return false;
	}
	char text[17];
	uint8_t length = 0;
	while(str[length] && col + length < 16U){
		text[length] = str[length];
		length++;
	}
	text[length] = '\0';
	return Lcd_writeString(text);
}
