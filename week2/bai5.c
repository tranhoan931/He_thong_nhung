#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <stdint.h>


/* =========================================================
   BIẾN
   ========================================================= */

/* 0 = OFF, 1 = ON */
volatile uint8_t led_enable = 0;

/* PWM gần nhất: 0 -> 100% */
volatile uint8_t pwm_percent = 50;

/* Bộ đệm UART */
volatile char uart_buffer[32];

/* Số ký tự hiện tại */
volatile uint8_t uart_index = 0;


/* =========================================================
   UART SEND CHAR
   ========================================================= */

void USART1_SendChar(char c)
{
    USART_SendData(USART1, (uint16_t)c);

    while (USART_GetFlagStatus(
               USART1,
               USART_FLAG_TXE
           ) == RESET);
}


/* =========================================================
   UART SEND STRING
   ========================================================= */

void USART1_SendString(const char *str)
{
    while (*str != '\0')
    {
        USART1_SendChar(*str);
        str++;
    }
}


/* =========================================================
   SO SÁNH CHUỖI
   ========================================================= */

uint8_t StringEqual(
    const char *a,
    const char *b
)
{
    while (*a != '\0' && *b != '\0')
    {
        if (*a != *b)
        {
            return 0;
        }

        a++;
        b++;
    }

    if (*a == '\0' && *b == '\0')
    {
        return 1;
    }

    return 0;
}


/* =========================================================
   KIỂM TRA CHUỖI BẮT ĐẦU BẰNG "PWM:"
   ========================================================= */

uint8_t IsPWMCommand(const char *cmd)
{
    if (cmd[0] != 'P') return 0;
    if (cmd[1] != 'W') return 0;
    if (cmd[2] != 'M') return 0;
    if (cmd[3] != ':') return 0;

    return 1;
}


/* =========================================================
   PWM INIT
   PA0 = TIM2_CH1

   Clock TIM2 = 72 MHz

   72 MHz / (71 + 1)
   = 1 MHz

   1 MHz / (999 + 1)
   = 1 kHz
   ========================================================= */

void PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;


    /* Clock GPIOA + AFIO */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_AFIO,
        ENABLE
    );

    /* Clock TIM2 */
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_TIM2,
        ENABLE
    );


    /* -----------------------------------------------------
       PA0 = TIM2_CH1
       ----------------------------------------------------- */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );


    /* -----------------------------------------------------
       TIM2
       ----------------------------------------------------- */

    TIM_TimeBaseStructure.TIM_Prescaler = 71;

    TIM_TimeBaseStructure.TIM_CounterMode =
        TIM_CounterMode_Up;

    TIM_TimeBaseStructure.TIM_Period = 999;

    TIM_TimeBaseStructure.TIM_ClockDivision =
        TIM_CKD_DIV1;

    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(
        TIM2,
        &TIM_TimeBaseStructure
    );


    /* -----------------------------------------------------
       PWM CH1
       ----------------------------------------------------- */

    TIM_OCInitStructure.TIM_OCMode =
        TIM_OCMode_PWM1;

    TIM_OCInitStructure.TIM_OutputState =
        TIM_OutputState_Enable;

    TIM_OCInitStructure.TIM_OCPolarity =
        TIM_OCPolarity_High;

    /*
       Ban đầu CCR = 0
       => LED OFF
    */

    TIM_OCInitStructure.TIM_Pulse = 0;

    TIM_OC1Init(
        TIM2,
        &TIM_OCInitStructure
    );

    TIM_OC1PreloadConfig(
        TIM2,
        TIM_OCPreload_Enable
    );

    TIM_ARRPreloadConfig(
        TIM2,
        ENABLE
    );

    /* Start TIM2 */
    TIM_Cmd(
        TIM2,
        ENABLE
    );
}


/* =========================================================
   SET PWM
   percent = 0 -> 100
   ========================================================= */

void PWM_SetPercent(uint8_t percent)
{
    uint16_t compare;


    if (percent > 100)
    {
        percent = 100;
    }


    /*
       ARR = 999

       PWM = percent * 1000 / 100
    */

    compare =
        ((uint16_t)percent * 1000) / 100;


    TIM_SetCompare1(
        TIM2,
        compare
    );


    pwm_percent = percent;
}


/* =========================================================
   USART1 INIT
   PA9  = TX
   PA10 = RX
   ========================================================= */

void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    /* Clock GPIOA + USART1 */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_USART1,
        ENABLE
    );


    /* -----------------------------------------------------
       PA9 = TX
       ----------------------------------------------------- */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_9;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );


    /* -----------------------------------------------------
       PA10 = RX
       ----------------------------------------------------- */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_10;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IN_FLOATING;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );


    /* -----------------------------------------------------
       USART1
       ----------------------------------------------------- */

    USART_InitStructure.USART_BaudRate =
        115200;

    USART_InitStructure.USART_WordLength =
        USART_WordLength_8b;

    USART_InitStructure.USART_StopBits =
        USART_StopBits_1;

    USART_InitStructure.USART_Parity =
        USART_Parity_No;

    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;

    USART_InitStructure.USART_Mode =
        USART_Mode_Tx |
        USART_Mode_Rx;


    USART_Init(
        USART1,
        &USART_InitStructure
    );


    /* Enable RX interrupt */
    USART_ITConfig(
        USART1,
        USART_IT_RXNE,
        ENABLE
    );


    /* -----------------------------------------------------
       NVIC USART1
       ----------------------------------------------------- */

    NVIC_InitStructure.NVIC_IRQChannel =
        USART1_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;

    NVIC_Init(
        &NVIC_InitStructure
    );


    /* Enable USART */
    USART_Cmd(
        USART1,
        ENABLE
    );
}


/* =========================================================
   CHUYỂN SỐ 0 -> 100 THÀNH CHUỖI
   Không dùng sprintf()
   ========================================================= */

void SendNumber(uint8_t number)
{
    if (number >= 100)
    {
        USART1_SendChar('1');

        USART1_SendChar('0' + ((number / 10) % 10));

        USART1_SendChar('0' + (number % 10));
    }
    else if (number >= 10)
    {
        USART1_SendChar('0' + (number / 10));

        USART1_SendChar('0' + (number % 10));
    }
    else
    {
        USART1_SendChar('0' + number);
    }
}


/* =========================================================
   XỬ LÝ LỆNH PWM

   PWM:0%!
   PWM:10%!
   PWM:50%!
   PWM:100%!
   ========================================================= */

void ProcessPWMCommand(const char *cmd)
{
    uint16_t value = 0;
    uint8_t i = 4;


    /*
       cmd[0..3] = "PWM:"
    */

    while (
        cmd[i] >= '0' &&
        cmd[i] <= '9'
    )
    {
        value =
            value * 10 +
            (cmd[i] - '0');

        i++;

        /*
           Không cho vượt 100
        */

        if (value > 100)
        {
            value = 100;
        }
    }


    /*
       Lưu mức PWM mới
    */

    pwm_percent = (uint8_t)value;


    /*
       Nếu LED đang ON
       thì thay đổi độ sáng ngay
    */

    if (led_enable)
    {
        PWM_SetPercent(
            pwm_percent
        );
    }


    /*
       Nếu LED OFF:
       chỉ lưu giá trị PWM.
    */

    USART1_SendString("PWM=");

    SendNumber(pwm_percent);

    USART1_SendString("%\r\n");
}


/* =========================================================
   XỬ LÝ LỆNH
   ========================================================= */

void Process_Command(char *cmd)
{
    /* -----------------------------------------------------
       ON!
       ----------------------------------------------------- */

    if (StringEqual(cmd, "ON"))
    {
        led_enable = 1;

        PWM_SetPercent(
            pwm_percent
        );

        USART1_SendString(
            "LED ON\r\n"
        );
    }


    /* -----------------------------------------------------
       OFF!
       ----------------------------------------------------- */

    else if (StringEqual(cmd, "OFF"))
    {
        led_enable = 0;

        /*
           Tắt LED
           nhưng giữ nguyên pwm_percent
        */

        TIM_SetCompare1(
            TIM2,
            0
        );

        USART1_SendString(
            "LED OFF\r\n"
        );
    }


    /* -----------------------------------------------------
       PWM:xxx%!
       ----------------------------------------------------- */

    else if (IsPWMCommand(cmd))
    {
        ProcessPWMCommand(cmd);
    }


    /* -----------------------------------------------------
       Status!
       ----------------------------------------------------- */

    else if (StringEqual(cmd, "Status"))
    {
        USART1_SendString(
            "Status: "
        );

        if (led_enable)
        {
            USART1_SendString(
                "ON"
            );
        }
        else
        {
            USART1_SendString(
                "OFF"
            );
        }

        USART1_SendString(
            ", PWM="
        );

        SendNumber(
            pwm_percent
        );

        USART1_SendString(
            "%\r\n"
        );
    }


    /* -----------------------------------------------------
       Lệnh không hợp lệ
       ----------------------------------------------------- */

    else
    {
        USART1_SendString(
            "Unknown command\r\n"
        );
    }
}


/* =========================================================
   USART1 INTERRUPT
   ========================================================= */

void USART1_IRQHandler(void)
{
    char c;


    if (
        USART_GetITStatus(
            USART1,
            USART_IT_RXNE
        ) != RESET
    )
    {
        /* Đọc dữ liệu */
        c = (char)USART_ReceiveData(
            USART1
        );


        /* -------------------------------------------------
           ! = kết thúc lệnh
           ------------------------------------------------- */

        if (c == '!')
        {
            uart_buffer[uart_index] =
                '\0';


            /*
               Xử lý lệnh
            */

            Process_Command(
                (char *)uart_buffer
            );


            /*
               Reset buffer
            */

            uart_index = 0;
        }


        /* -------------------------------------------------
           Ký tự bình thường
           ------------------------------------------------- */

        else
        {
            /*
               Bỏ qua CR / LF
            */

            if (
                c != '\r' &&
                c != '\n'
            )
            {
                if (uart_index < 31)
                {
                    uart_buffer[uart_index] =
                        c;

                    uart_index++;
                }
                else
                {
                    /*
                       Buffer đầy
                       reset
                    */

                    uart_index = 0;
                }
            }
        }
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    /*
       System clock = 72 MHz
    */

    SystemInit();


    /*
       PWM:
       PA0 / TIM2_CH1
    */

    PWM_Init();


    /*
       UART:
       PA9 / PA10
    */

    USART1_Init();


    /*
       PWM mặc định 50%
       nhưng LED OFF lúc khởi động
    */

    pwm_percent = 50;

    led_enable = 0;

    TIM_SetCompare1(
        TIM2,
        0
    );


    while (1)
    {
    }
}
