#include "stm32f1xx_hal.h"
#include "ds1307.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
void Error_Handler(void);

int main(void)
{
    DS1307_TimeTypeDef time;

    char message[100];
    char rx_buffer[64];

    uint8_t rx_char;
    uint8_t rx_index = 0;

    HAL_Init();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();

    HAL_Delay(500);

    char start[] = "\r\nSTM32 DS1307 READY\r\n";
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)start,
        strlen(start),
        HAL_MAX_DELAY
    );

    uint32_t last_time = HAL_GetTick();

    while (1)
    {
        /*
         * Nhận UART từng byte
         */
        if (HAL_UART_Receive(&huart1, &rx_char, 1, 10) == HAL_OK)
        {
            /*
             * Kết thúc dòng khi nhận \n hoặc \r
             */
            if (rx_char == '\n' || rx_char == '\r')
            {
                if (rx_index > 0)
                {
                    rx_buffer[rx_index] = '\0';

                    /*
                     * Kiểm tra lệnh:
                     * SET YYYY MM DD HH MM SS
                     */
                    int year;
                    int month;
                    int date;
                    int hours;
                    int minutes;
                    int seconds;

                    if (sscanf(
                            rx_buffer,
                            "SET %d %d %d %d %d %d",
                            &year,
                            &month,
                            &date,
                            &hours,
                            &minutes,
                            &seconds
                        ) == 6)
                    {
                        /*
                         * Kiểm tra dữ liệu cơ bản
                         */
                        if (year >= 2000 &&
                            year <= 2099 &&
                            month >= 1 &&
                            month <= 12 &&
                            date >= 1 &&
                            date <= 31 &&
                            hours >= 0 &&
                            hours <= 23 &&
                            minutes >= 0 &&
                            minutes <= 59 &&
                            seconds >= 0 &&
                            seconds <= 59)
                        {
                            time.year    = year - 2000;
                            time.month   = month;
                            time.date    = date;
                            time.hours   = hours;
                            time.minutes = minutes;
                            time.seconds = seconds;

                            /*
                             * Thứ trong tuần.
                             * Tạm thời đặt 1 vì chương trình
* hiện tại không sử dụng trường này.
                             */
                            time.day = 1;

                            if (DS1307_SetTime(&hi2c1, &time) == HAL_OK)
                            {
                                char ok[] =
                                    "DS1307 SET OK\r\n";

                                HAL_UART_Transmit(
                                    &huart1,
                                    (uint8_t *)ok,
                                    strlen(ok),
                                    HAL_MAX_DELAY
                                );

                                /*
                                 * Đọc lại DS1307 ngay sau khi ghi
                                 */
                                if (DS1307_GetTime(
                                        &hi2c1,
                                        &time) == HAL_OK)
                                {
                                    sprintf(
                                        message,
                                        "SET TIME: 20%02d/%02d/%02d %02d:%02d:%02d\r\n",
                                        time.year,
                                        time.month,
                                        time.date,
                                        time.hours,
                                        time.minutes,
                                        time.seconds
                                    );

                                    HAL_UART_Transmit(
                                        &huart1,
                                        (uint8_t *)message,
                                        strlen(message),
                                        HAL_MAX_DELAY
                                    );
                                }
                            }
                            else
                            {
                                char error[] =
                                    "DS1307 SET ERROR\r\n";

                                HAL_UART_Transmit(
                                    &huart1,
                                    (uint8_t *)error,
                                    strlen(error),
                                    HAL_MAX_DELAY
                                );
                            }
                        }
                        else
                        {
                            char error[] =
                                "INVALID TIME\r\n";

                            HAL_UART_Transmit(
                                &huart1,
                                (uint8_t *)error,
                                strlen(error),
                                HAL_MAX_DELAY
                            );
                        }
                    }
                    else
                    {
                        char error[] =
                            "UNKNOWN COMMAND\r\n";
HAL_UART_Transmit(
                            &huart1,
                            (uint8_t *)error,
                            strlen(error),
                            HAL_MAX_DELAY
                        );
                    }

                    rx_index = 0;
                }
            }
            else
            {
                /*
                 * Lưu ký tự vào buffer
                 */
                if (rx_index < sizeof(rx_buffer) - 1)
                {
                    rx_buffer[rx_index++] = rx_char;
                }
                else
                {
                    /*
                     * Buffer đầy -> reset
                     */
                    rx_index = 0;
                }
            }
        }

        /*
         * Đọc DS1307 mỗi 1 giây
         */
        if (HAL_GetTick() - last_time >= 1000)
        {
            last_time = HAL_GetTick();

            if (DS1307_GetTime(&hi2c1, &time) == HAL_OK)
            {
                sprintf(
                    message,
                    "TIME: 20%02d/%02d/%02d %02d:%02d:%02d\r\n",
                    time.year,
                    time.month,
                    time.date,
                    time.hours,
                    time.minutes,
                    time.seconds
                );

                HAL_UART_Transmit(
                    &huart1,
                    (uint8_t *)message,
                    strlen(message),
                    HAL_MAX_DELAY
                );
            }
            else
            {
                char error[] =
                    "DS1307 ERROR\r\n";

                HAL_UART_Transmit(
                    &huart1,
                    (uint8_t *)error,
                    strlen(error),
                    HAL_MAX_DELAY
                );
            }
        }
    }
}


static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
}


static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;

    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}


static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;

    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin =
            GPIO_PIN_6 |
            GPIO_PIN_7;

        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /*
         * PA9 = USART1_TX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /*
         * PA10 = USART1_RX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}


void Error_Handler(void)
{
    while (1)
    {
    }
}
