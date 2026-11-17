/*
 * lcd.h
 *
 *  Created on: 2026. 10. 21.
 *      Author: kccistc
 */

#ifndef DRIVER_LCD_H_
#define DRIVER_LCD_H_

/* include */
#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* define */

/* typedef */

/* variable declarations */
extern volatile HAL_StatusTypeDef lcd_last_status;

/* function declarations */
bool Lcd_init(I2C_HandleTypeDef *hi2c);
bool Lcd_writeStringXY(uint8_t col, uint8_t row, const char *text);

#endif /* DRIVER_LCD_H_ */
