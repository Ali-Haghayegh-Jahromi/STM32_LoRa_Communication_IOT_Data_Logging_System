/*
 * sim.h
 */

#ifndef _SIM_H
#define _SIM_H

#include <String.h>
#include <Stdio.h>
#include "lcd.h"

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
#define device_uart &huart2
extern IWDG_HandleTypeDef hiwdg;

#define reset_Pin GPIO_PIN_1
#define reset_GPIO_Port GPIOA

#define TxBuf_SIZE 		1300
#define RxBuf_SIZE 		500
#define MainBuf_SIZE 	5500

extern uint8_t gTxBuf[TxBuf_SIZE];
extern uint8_t gMainBuf[MainBuf_SIZE];
extern uint8_t gRxBuf[RxBuf_SIZE];
extern uint16_t gOldPos;
extern uint16_t gNewPos;

void simreset(void);

void uart_sendstring_device(void* string);

void uart_sendstring_pc(void* string);

int waitfor(void* string);

int simstart(void); //returns 0 if simcard or module is not ok ,1 if they are ok 

int	srevercon(void* ip,void* port);	//returns 0&-1 if could not connect ,1 if connected to server

int	srevercon_udp(void* ip,void* port);	//returns 0&-1 if could not connect ,1 if connected to server

int simsend(void* string,int i);	//i==0:fast send i==1:safe send

void uartflush(void);

#endif // _SIM_H
