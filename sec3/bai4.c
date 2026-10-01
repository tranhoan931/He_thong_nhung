
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "misc.h"
#include <stdint.h>


/* =========================================================
   CONFIG
   ========================================================= */

#define ADC_BUFFER_SIZE     200
#define ADC_HALF_SIZE       100

/*
 * 100 samples ADC.
 *
 * Giá trị lớn nhất:
 * 100 x 4 chữ số + CR/LF
 * = 100 x 6
 * = 600 bytes
 *
 * Chừa thêm khoảng dự phòng.
 */
#define UART_BUFFER_SIZE    700


/* =========================================================
   GLOBAL VARIABLES
   ========================================================= */

/*
 * ADC DMA buffer
 *
 * DMA1 Channel1:
 * ADC1->DR
 *      ↓
 * adc_buffer[]
 */
volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];


/*
 * UART transmit buffer
 *
 * DMA1 Channel4:
 * uart_buffer[]
 *      ↓
 * USART1->DR
 */
char uart_buffer[UART_BUFFER_SIZE];


/*
 * UART DMA đang truyền
 *
 * 0 = rảnh
 * 1 = đang truyền
 */
volatile uint8_t uart_busy = 0;


/*
 * Cờ ADC DMA
 *
 * ISR chỉ set cờ.
 *
 * main() mới xử lý dữ liệu.
 */
volatile uint8_t adc_half_ready = 0;
volatile uint8_t adc_full_ready = 0;


/* =========================================================
   DELAY
   ========================================================= */

void delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    volatile uint32_t j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 7200; j++)
        {
            __NOP();
        }
    }
}


/* =========================================================
   GPIO
   =========================================================

   PA0  -> ADC1_IN0
   PA9  -> USART1_TX
   PA10 -> USART1_RX

   ========================================================= */

void GPIO_Init_All(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;


    /*
     * GPIOA clock
     */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA,
        ENABLE
    );


    /* -----------------------------------------------------
       PA0 = ADC1_IN0
       ----------------------------------------------------- */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AIN;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /* -----------------------------------------------------
       PA9 = USART1 TX
       ----------------------------------------------------- */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_9;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /* -----------------------------------------------------
       PA10 = USART1 RX
       ----------------------------------------------------- */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_10;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOA, &GPIO_InitStructure);
}


/* =========================================================
   USART1
   =========================================================

   115200
   8 bit
   No parity
   1 stop bit

   ========================================================= */

void USART1_Init(void)
{
    USART_InitTypeDef USART_InitStructure;


    /*
     * USART1 clock
     */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_USART1,
        ENABLE
    );


    /*
     * USART configuration
     */
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


    /*
     * Enable USART1
     */
    USART_Cmd(
        USART1,
        ENABLE
    );
}


/* =========================================================
   TIM3
   =========================================================

   Clock STM32 = 72 MHz

   PSC = 7199

   72 MHz / (7199 + 1)
   = 10 kHz

   ARR = 99

   10 kHz / (99 + 1)
   = 100 Hz

   => 1 ADC conversion / 10 ms

   TIM3 Update
       ↓
   TRGO
       ↓
   ADC1

   ========================================================= */

void TIM3_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;


    /*
     * TIM3 clock
     */
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_TIM3,
        ENABLE
    );


    /*
     * Timer configuration
     */
    TIM_TimeBaseStructure.TIM_Prescaler =
        7199;

    TIM_TimeBaseStructure.TIM_CounterMode =
        TIM_CounterMode_Up;

    TIM_TimeBaseStructure.TIM_Period =
        99;

    TIM_TimeBaseStructure.TIM_ClockDivision =
        TIM_CKD_DIV1;

    TIM_TimeBaseStructure.TIM_RepetitionCounter =
        0;


    TIM_TimeBaseInit(
        TIM3,
        &TIM_TimeBaseStructure
    );


    /*
     * TIM3 Update -> TRGO
     */
    TIM_SelectOutputTrigger(
        TIM3,
        TIM_TRGOSource_Update
    );


    /*
     * Chưa chạy timer.
     *
     * main() sẽ ENABLE sau khi
     * ADC + DMA đã sẵn sàng.
     */
    TIM_Cmd(
        TIM3,
        DISABLE
    );
}


/* =========================================================
   ADC1
   =========================================================

   PA0 = ADC1_IN0

   ADC clock:

       72 MHz / 6
       = 12 MHz

   Trigger:

       TIM3 TRGO

   Continuous:

       OFF

   Mỗi TIM3 TRGO tạo 1 ADC conversion.

   ========================================================= */

void ADC1_Init_All(void)
{
    ADC_InitTypeDef ADC_InitStructure;


    /*
     * ADC1 clock
     */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_ADC1,
        ENABLE
    );


    /*
     * ADC clock = 12 MHz
     */
    RCC_ADCCLKConfig(
        RCC_PCLK2_Div6
    );


    /*
     * Reset ADC
     */
    ADC_DeInit(ADC1);


    /* -----------------------------------------------------
       ADC configuration
       ----------------------------------------------------- */

    ADC_InitStructure.ADC_Mode =
        ADC_Mode_Independent;

    ADC_InitStructure.ADC_ScanConvMode =
        DISABLE;

    ADC_InitStructure.ADC_ContinuousConvMode =
        DISABLE;


    /*
     * TIM3 TRGO trigger
     */
    ADC_InitStructure.ADC_ExternalTrigConv =
        ADC_ExternalTrigConv_T3_TRGO;


    ADC_InitStructure.ADC_DataAlign =
        ADC_DataAlign_Right;

    ADC_InitStructure.ADC_NbrOfChannel =
        1;


    ADC_Init(
        ADC1,
        &ADC_InitStructure
    );


    /* -----------------------------------------------------
       PA0 = ADC Channel 0
       ----------------------------------------------------- */

    ADC_RegularChannelConfig(
        ADC1,
        ADC_Channel_0,
        1,
        ADC_SampleTime_55Cycles5
    );


    /*
     * Enable ADC
     */
    ADC_Cmd(
        ADC1,
        ENABLE
    );


    /* -----------------------------------------------------
       Calibration
       ----------------------------------------------------- */

    ADC_ResetCalibration(ADC1);

    while (
        ADC_GetResetCalibrationStatus(ADC1)
    )
    {
    }


    ADC_StartCalibration(ADC1);

    while (
        ADC_GetCalibrationStatus(ADC1)
    )
    {
    }
}


/* =========================================================
   DMA ADC
   =========================================================

   DMA1 Channel1

   ADC1->DR
       ↓
   adc_buffer[200]

   Circular mode.

   200 samples:

       [0 ... 99]
       [100 ... 199]

   Half Transfer:
       100 samples đầu

   Transfer Complete:
       100 samples sau

   ========================================================= */

void DMA_ADC_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    /*
     * DMA1 clock
     */
    RCC_AHBPeriphClockCmd(
        RCC_AHBPeriph_DMA1,
        ENABLE
    );


    /*
     * Reset Channel 1
     */
    DMA_DeInit(
        DMA1_Channel1
    );


    /* -----------------------------------------------------
       Peripheral address

       ADC data register
       ----------------------------------------------------- */

    DMA_InitStructure.DMA_PeripheralBaseAddr =
        (uint32_t)&ADC1->DR;


    /* -----------------------------------------------------
       Memory address

       adc_buffer
       ----------------------------------------------------- */

    DMA_InitStructure.DMA_MemoryBaseAddr =
        (uint32_t)adc_buffer;


    /*
     * Direction:

         ADC -> RAM
     */
    DMA_InitStructure.DMA_DIR =
        DMA_DIR_PeripheralSRC;


    /*
     * 200 samples
     */
    DMA_InitStructure.DMA_BufferSize =
        ADC_BUFFER_SIZE;


    /*
     * ADC->DR cố định
     */
    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;


    /*
     * RAM tăng địa chỉ
     */
    DMA_InitStructure.DMA_MemoryInc =
        DMA_MemoryInc_Enable;


    /*
     * ADC = 16 bit
     */
    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_HalfWord;


    /*
     * buffer = uint16_t
     */
    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_HalfWord;


    /*
     * Circular

     * Sau sample 199:
     *
     * quay lại sample 0
     */
    DMA_InitStructure.DMA_Mode =
        DMA_Mode_Circular;


    DMA_InitStructure.DMA_Priority =
        DMA_Priority_High;


    DMA_InitStructure.DMA_M2M =
        DMA_M2M_Disable;


    DMA_Init(
        DMA1_Channel1,
        &DMA_InitStructure
    );


    /* -----------------------------------------------------
       DMA interrupts
       ----------------------------------------------------- */

    DMA_ITConfig(
        DMA1_Channel1,
        DMA_IT_HT | DMA_IT_TC,
        ENABLE
    );


    /* -----------------------------------------------------
       NVIC
       ----------------------------------------------------- */

    NVIC_InitStructure.NVIC_IRQChannel =
        DMA1_Channel1_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        1;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;


    NVIC_Init(
        &NVIC_InitStructure
    );


    /*
     * Enable DMA channel
     */
    DMA_Cmd(
        DMA1_Channel1,
        ENABLE
    );


    /*
     * Enable ADC -> DMA
     */
    ADC_DMACmd(
        ADC1,
        ENABLE
    );
}


/* =========================================================
   DMA UART
   =========================================================

   DMA1 Channel4

   RAM
       ↓
   USART1->DR

   USART1 TX = PA9

   ========================================================= */

void DMA_UART_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    /*
     * Reset Channel 4
     */
    DMA_DeInit(
        DMA1_Channel4
    );


    /*
     * USART1 data register
     */
    DMA_InitStructure.DMA_PeripheralBaseAddr =
        (uint32_t)&USART1->DR;


    /*
     * Default buffer
     */
    DMA_InitStructure.DMA_MemoryBaseAddr =
        (uint32_t)uart_buffer;


    /*
     * RAM -> USART
     */
    DMA_InitStructure.DMA_DIR =
        DMA_DIR_PeripheralDST;


    /*
     * Sẽ thay đổi trước mỗi lần truyền.
     */
    DMA_InitStructure.DMA_BufferSize =
        1;


    /*
     * USART DR cố định
     */
    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;


    /*
     * RAM tăng
     */
    DMA_InitStructure.DMA_MemoryInc =
        DMA_MemoryInc_Enable;


    /*
     * USART = byte
     */
    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_Byte;


    /*
     * Buffer = char
     */
    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_Byte;


    /*
     * Mỗi lần truyền xong phải
     * cấu hình lại.
     */
    DMA_InitStructure.DMA_Mode =
        DMA_Mode_Normal;


    DMA_InitStructure.DMA_Priority =
        DMA_Priority_Medium;


    DMA_InitStructure.DMA_M2M =
        DMA_M2M_Disable;


    DMA_Init(
        DMA1_Channel4,
        &DMA_InitStructure
    );


    /*
     * DMA Transfer Complete interrupt
     */
    DMA_ITConfig(
        DMA1_Channel4,
        DMA_IT_TC,
        ENABLE
    );


    /* -----------------------------------------------------
       NVIC DMA1 Channel4
       ----------------------------------------------------- */

    NVIC_InitStructure.NVIC_IRQChannel =
        DMA1_Channel4_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        2;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;


    NVIC_Init(
        &NVIC_InitStructure
    );


    /*
     * USART1 TX request by DMA
     */
    USART_DMACmd(
        USART1,
        USART_DMAReq_Tx,
        ENABLE
    );
}


/* =========================================================
   NUMBER -> ASCII
   =========================================================

   0    -> "0"
   123  -> "123"
   4095 -> "4095"

   Return:
       số ký tự

   ========================================================= */

uint16_t NumberToString(
    char *buffer,
    uint16_t value
)
{
    char temp[6];

    uint16_t i = 0;
    uint16_t j;


    /*
     * value = 0
     */
    if (value == 0)
    {
        buffer[0] = '0';

        return 1;
    }


    /*
     * Tách từng digit
     */
    while (value > 0)
    {
        temp[i++] =
            '0' + (value % 10);

        value /= 10;
    }


    /*
     * Đảo lại
     */
    for (j = 0; j < i; j++)
    {
        buffer[j] =
            temp[i - 1 - j];
    }


    return i;
}


/* =========================================================
   UART DMA SEND
   ========================================================= */

void UART_DMA_Send(
    char *data,
    uint16_t length
)
{
    /*
     * UART đang bận
     */
    if (uart_busy)
    {
        return;
    }


    /*
     * Đánh dấu busy
     */
    uart_busy = 1;


    /*
     * Disable trước khi thay đổi
     * địa chỉ/count.
     */
    DMA_Cmd(
        DMA1_Channel4,
        DISABLE
    );


    /*
     * Clear DMA flags
     */
    DMA_ClearFlag(
        DMA1_FLAG_GL4 |
        DMA1_FLAG_TC4 |
        DMA1_FLAG_HT4 |
        DMA1_FLAG_TE4
    );


    /*
     * Memory address
     */
    DMA1_Channel4->CMAR =
        (uint32_t)data;


    /*
     * Number of bytes
     */
    DMA_SetCurrDataCounter(
        DMA1_Channel4,
        length
    );


    /*
     * Start DMA UART
     */
    DMA_Cmd(
        DMA1_Channel4,
        ENABLE
    );
}


/* =========================================================
   BUILD ADC BLOCK
   =========================================================

   start_index = 0
       -> ADC[0..99]

   start_index = 100
       -> ADC[100..199]

   Hàm này chạy trong MAIN,
   KHÔNG chạy trong ISR.

   ========================================================= */

void Send_ADC_Block(
    uint16_t start_index
)
{
    uint16_t i;
    uint16_t index = 0;
    uint16_t len;


    /*
     * Nếu UART còn bận thì không
     * đụng vào uart_buffer.
     */
    if (uart_busy)
    {
        return;
    }


    /*
     * Chuyển 100 sample
     * thành text.
     */
    for (i = 0; i < ADC_HALF_SIZE; i++)
    {
        len = NumberToString(
            &uart_buffer[index],
            adc_buffer[start_index + i]
        );


        index += len;


        /*
         * CR LF
         */
        uart_buffer[index++] = '\r';
        uart_buffer[index++] = '\n';
    }


    /*
     * Gửi toàn bộ block bằng DMA.
     */
    UART_DMA_Send(
        uart_buffer,
        index
    );
}


/* =========================================================
   DMA1 CHANNEL1 IRQ
   =========================================================

   ISR CHỈ SET FLAG.

   Không xử lý:
       - NumberToString
       - UART
       - vòng for 100 mẫu

   ========================================================= */

void DMA1_Channel1_IRQHandler(void)
{
    /*
     * Half Transfer
     *
     * ADC[0..99] đã đầy.
     */
    if (
        DMA_GetITStatus(
            DMA1_IT_HT1
        ) != RESET
    )
    {
        DMA_ClearITPendingBit(
            DMA1_IT_HT1
        );


        /*
         * Chỉ set flag.
         */
        adc_half_ready = 1;
    }


    /*
     * Transfer Complete
     *
     * ADC[100..199] đã đầy.
     */
    if (
        DMA_GetITStatus(
            DMA1_IT_TC1
        ) != RESET
    )
    {
        DMA_ClearITPendingBit(
            DMA1_IT_TC1
        );


        /*
         * Chỉ set flag.
         */
        adc_full_ready = 1;
    }
}


/* =========================================================
   DMA1 CHANNEL4 IRQ
   =========================================================

   UART DMA truyền xong.

   ========================================================= */

void DMA1_Channel4_IRQHandler(void)
{
    if (
        DMA_GetITStatus(
            DMA1_IT_TC4
        ) != RESET
    )
    {
        /*
         * Clear interrupt
         */
        DMA_ClearITPendingBit(
            DMA1_IT_TC4
        );


        /*
         * Stop DMA channel
         */
        DMA_Cmd(
            DMA1_Channel4,
            DISABLE
        );


        /*
         * UART DMA đã rảnh
         */
        uart_busy = 0;
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    /*
     * GPIO
     */
    GPIO_Init_All();


    /*
     * USART1
     */
    USART1_Init();


    /*
     * TIM3 = 100 Hz
     */
    TIM3_Init();


    /*
     * ADC1 PA0
     *
     * Trigger = TIM3 TRGO
     */
    ADC1_Init_All();


    /*
     * USART1 TX DMA
     *
     * DMA1 Channel4
     */
    DMA_UART_Init();


    /*
     * ADC DMA
     *
     * DMA1 Channel1
     */
    DMA_ADC_Init();


    /*
     * Tất cả ADC + DMA đã sẵn sàng.
     *
     * Bây giờ mới start TIM3.
     */
    TIM_Cmd(
        TIM3,
        ENABLE
    );


    /* =====================================================
       MAIN LOOP

       ISR:
           chỉ set flag

       MAIN:
           xử lý ADC
           tạo chuỗi
           gửi UART DMA
       ===================================================== */

    while (1)
    {
        /*
         * -------------------------------------------------
         * Half buffer ready
         *
         * adc_buffer[0..99]
         * -------------------------------------------------
         */

        if (adc_half_ready)
        {
            /*
             * Clear flag trước.
             */
            adc_half_ready = 0;


            /*
             * Chỉ gửi nếu UART đang rảnh.
             *
             * Send_ADC_Block() tự kiểm tra
             * uart_busy lần nữa.
             */
            if (!uart_busy)
            {
                Send_ADC_Block(0);
            }
        }


        /*
         * -------------------------------------------------
         * Full buffer ready
         *
         * adc_buffer[100..199]
         * -------------------------------------------------
         */

        if (adc_full_ready)
        {
            /*
             * Clear flag
             */
            adc_full_ready = 0;


            /*
             * Gửi block thứ 2.
             */
            if (!uart_busy)
            {
                Send_ADC_Block(100);
            }
        }
    }
}
