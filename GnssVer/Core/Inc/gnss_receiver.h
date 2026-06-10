#ifndef __GNSS_RECEIVER_H
#define __GNSS_RECEIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define GNSS_LINE_BUFFER_SIZE 160U

void GnssReceiver_Init(UART_HandleTypeDef *huart);
uint8_t GnssReceiver_GetLine(char *buffer, uint16_t size);
void GnssReceiver_RxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif
