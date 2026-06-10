#include "uart_echo.h"
#include <string.h>

static UART_HandleTypeDef *echo_huart = NULL;

static uint8_t rx_byte;
static char rx_buffer[UART_ECHO_BUFFER_SIZE];

static volatile uint16_t rx_len = 0U;
static volatile uint8_t line_ready = 0U;
static volatile uint8_t input_too_long = 0U;
// 启动/重启下一次单字节中断接收
static void UartEcho_StartReceive(void)
{
    if (echo_huart == NULL)
    {
        return;
    }

    HAL_UART_Receive_IT(echo_huart, &rx_byte, 1U);
}

void UartEcho_Init(UART_HandleTypeDef *huart)
{
    echo_huart = huart;

    rx_len = 0U;
    line_ready = 0U;
    input_too_long = 0U;
    memset(rx_buffer, 0, sizeof(rx_buffer)); //把整个 rx_buffer 缓冲区清零。

    UartEcho_StartReceive();
}
/*
如果还没指定串口，直接返回
如果还没有收到完整一行，直接返回
如果输入超长，发送错误提示
否则发送 "RX: " + 接收到的内容 + "\r\n"
发送完成后清空状态，准备处理下一行
*/
void UartEcho_Process(void)
{
    if ((echo_huart == NULL) || (line_ready == 0U))
    {
        return;
    }

    if (input_too_long != 0U)
    {
        const uint8_t msg[] = "ERR: input too long\r\n";
        HAL_UART_Transmit(echo_huart, (uint8_t *)msg, sizeof(msg) - 1U, HAL_MAX_DELAY);
    }
    else
    {
        const uint8_t prefix[] = "RX: ";
        const uint8_t suffix[] = "\r\n";

        HAL_UART_Transmit(echo_huart, (uint8_t *)prefix, sizeof(prefix) - 1U, HAL_MAX_DELAY);//阻塞式发送 = 函数不返回，CPU 就不继续往下走。Timeout = 最多等多久，超过就放弃并返回超时状态。
        HAL_UART_Transmit(echo_huart, (uint8_t *)rx_buffer, rx_len, HAL_MAX_DELAY);
        HAL_UART_Transmit(echo_huart, (uint8_t *)suffix, sizeof(suffix) - 1U, HAL_MAX_DELAY);
    }

    rx_len = 0U;
    line_ready = 0U;
    input_too_long = 0U;
    memset(rx_buffer, 0, sizeof(rx_buffer));
}
/*
每收到一个字节时，把它归类处理：
普通字符就存起来，换行符就标记一行完成，最后继续等待下一个字节
*/
void UartEcho_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((echo_huart == NULL) || (huart != echo_huart))
    {
        return;
    }

    if (line_ready != 0U)
    {
        UartEcho_StartReceive();
        return;
    }

    if ((rx_byte == '\r') || (rx_byte == '\n'))
    {
        if ((rx_len > 0U) || (input_too_long != 0U))
        {
            rx_buffer[rx_len] = '\0';
            line_ready = 1U;
        }
    }
    else if (input_too_long == 0U)
    {
        if (rx_len < (UART_ECHO_BUFFER_SIZE - 1U))
        {
            rx_buffer[rx_len++] = (char)rx_byte;
        }
        else
        {
            input_too_long = 1U;
        }
    }

    UartEcho_StartReceive();
}
