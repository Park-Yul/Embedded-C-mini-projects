/*
 * uart_protocol.h
 *
 *  Created on: 2026. 11. 4.
 *      Author: kccistc
 *
 *  패킷 형식 (PC <-> STM32 공통)
 *    [0xAA][LEN][CMD][PAYLOAD ...][CHK]
 *     LEN = 1 + payload 길이 (CMD 포함, 1 ~ PKT_MAX_LEN-1)
 *     CHK = (LEN + CMD + payload 합) 의 하위 8비트
 *  응답 CMD = 요청 CMD + 0x80
 */

#ifndef SRC_DRIVER_UART_PROTOCOL_UART_PROTOCOL_H_
#define SRC_DRIVER_UART_PROTOCOL_UART_PROTOCOL_H_

/* include */
#include "stm32f4xx_hal.h"

/* define */
#define PKT_START		0xAA
#define PKT_MAX_LEN		16

// request command(STM32 <- PC)
#define CMD_LED0_SET		0x10
#define CMD_LED1_SET		0x11
#define CMD_LED2_SET		0x12
#define CMD_LED3_SET		0x13
#define CMD_LED_STATUS_REQ	0x14
#define CMD_TEMP_HUMID_REQ	0x20
#define CMD_DISTANCE_REQ	0x30

// 엘리베이터 명령 (STM32 <- PC)
#define CMD_CALL_FLOOR		0x40	// payload: [층 1~3]          → 층 버튼 누른 것과 같음
#define CMD_START			0x41	// payload: 없음              → START 버튼 누른 것과 같음
#define CMD_EMERGENCY		0x42	// payload: [1=진입, 0=해제]   → 해제는 START 와 같은 동작
#define CMD_STATUS_REQ		0x43	// payload: 없음              → 상태 응답(CMD_STATUS_RSP)

// response command(STM32 -> PC)
#define CMD_LED_RSP			0x90
#define CMD_LED_STATUS_RSP	0x94
#define CMD_TEMP_HUMID_RSP	0xA0
#define CMD_DISTACNE_RSP	0xB0
#define CMD_CALL_FLOOR_RSP	0xC0	// payload: [RSP_OK/RSP_ERR]
#define CMD_START_RSP		0xC1	// payload: [RSP_OK/RSP_ERR]
#define CMD_EMERGENCY_RSP	0xC2	// payload: [RSP_OK/RSP_ERR]
#define CMD_STATUS_RSP		0xC3	// payload: 12바이트, ap_main.c 의 ap_send_status() 참고

#define RSP_OK				0x00
#define RSP_ERR				0x01

/* typedef */
typedef void (*uart_packet_callback_t)(uint8_t, uint8_t *, uint8_t);

/* variable declarations */

/* function declarations */
void UART_protocol_init(UART_HandleTypeDef *huart, uart_packet_callback_t packet_cb);
void UART_protocol_task(void);		// 메인 루프에서 계속 호출: 받은 패킷을 콜백으로 넘김
void UART_protocol_send(uint8_t cmd, uint8_t *payload, uint8_t payload_len);

#endif /* SRC_DRIVER_UART_PROTOCOL_UART_PROTOCOL_H_ */
