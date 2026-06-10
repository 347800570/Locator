#include "gnss_receiver.h"
#include <string.h>

static UART_HandleTypeDef *gnss_huart = NULL;

static uint8_t rx_byte;
static char rx_buffer[GNSS_LINE_BUFFER_SIZE];
static char ready_buffer[GNSS_LINE_BUFFER_SIZE];

static volatile uint16_t rx_len = 0U;
static volatile uint16_t ready_len = 0U;
static volatile uint8_t line_ready = 0U;
static volatile uint8_t receiving_line = 0U;
static volatile uint8_t current_line_overflow = 0U;
// 启动/重启下一次单字节中断接收
static void GnssReceiver_StartReceive(void)
{
    if (gnss_huart == NULL)
    {
        return;
    }

    HAL_UART_Receive_IT(gnss_huart, &rx_byte, 1U);
}

void GnssReceiver_Init(UART_HandleTypeDef *huart)
{
    gnss_huart = huart;

    rx_len = 0U;
    ready_len = 0U;
    line_ready = 0U;
    receiving_line = 0U;
    current_line_overflow = 0U;

    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ready_buffer, 0, sizeof(ready_buffer));

    GnssReceiver_StartReceive();
}
/*
主循环从接收模块里“安全取走一整行 NMEA”的函数。
它负责判断有没有新数据、复制数据、防止越界、清除已读状态。
*/
uint8_t GnssReceiver_GetLine(char *buffer, uint16_t size)
{
    uint16_t copy_len;

    if ((buffer == NULL) || (size == 0U) || (line_ready == 0U))
    {
        return 0U;
    }

    __disable_irq();//临时关闭中断，以避免在复制 ready_buffer 时被 RxCpltCallback 中断打断，导致数据不一致。

    copy_len = ready_len;
    if (copy_len >= size)
    {
        copy_len = size - 1U;
    }

    memcpy(buffer, ready_buffer, copy_len); //把内部的 ready_buffer 复制到外部 buffer。
    buffer[copy_len] = '\0';

    line_ready = 0U;
    ready_len = 0U;
    memset(ready_buffer, 0, sizeof(ready_buffer));

    __enable_irq();

    return 1U;
}
/*
等待字节
收到 '$' -> 开始一行
收到普通字符 -> 存入 rx_buffer
收到 '\r' -> 忽略
收到 '\n' -> 一行结束，复制到 ready_buffer，设置 line_ready
重新等待下一个字节
*/
void GnssReceiver_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((gnss_huart == NULL) || (huart != gnss_huart))
    {
        return;
    }

    if (rx_byte == '$')
    {
        receiving_line = 1U;
        current_line_overflow = 0U;
        rx_len = 0U;
        rx_buffer[rx_len++] = (char)rx_byte;
    }
    else if (receiving_line != 0U)
    {
        if (rx_byte == '\n')
        {
            if ((current_line_overflow == 0U) && (rx_len > 0U) && (line_ready == 0U))
            {
                rx_buffer[rx_len] = '\0';
                memcpy(ready_buffer, rx_buffer, rx_len + 1U);
                ready_len = rx_len;
                line_ready = 1U;
            }

            receiving_line = 0U;
            current_line_overflow = 0U;
            rx_len = 0U;
        }
        else if (rx_byte != '\r')
        {
            if (rx_len < (GNSS_LINE_BUFFER_SIZE - 1U))
            {
                rx_buffer[rx_len++] = (char)rx_byte;
            }
            else
            {
                current_line_overflow = 1U;
            }
        }
    }

    GnssReceiver_StartReceive();
}
