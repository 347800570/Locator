#ifndef __UART_ECHO_H
#define __UART_ECHO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define UART_ECHO_BUFFER_SIZE 128U

void UartEcho_Init(UART_HandleTypeDef *huart);
void UartEcho_Process(void);
void UartEcho_RxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif
