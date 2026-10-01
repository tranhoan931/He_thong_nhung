
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "misc.h"
#include <stdint.h>

/*
====================================================
BAI TAP 03
====================================================

Yeu cau:
- Cau hinh UART
- Nut nhan
- DMA
- Moi lan nhan nut:
    + Gia tri tang 1
    + Gui ban tin qua UART bang DMA

Cau truc ban tin:

<ID-Lop><ID-Nhom>:BTN:<Gia tri nut nhan>\n\r

Vi du:

HTN02N6:BTN:1
HTN02N6:BTN:2
HTN02N6:BTN:3
...

====================================================

PHAN CUNG:

STM32F103

PA0  -> Nut nhan
PA9  -> USART1 TX
PA10 -> USART1 RX

USART1:
115200 baud
8 bit
1 stop bit
No parity

DMA:
USART1 TX -> DMA1 Channel 4

====================================================
*/


/* =================================================
   THONG TIN SINH VIEN
   ================================================= */

#define ID_LOP  "HTN02"
#define ID_NHOM "N6"


/* =================================================
   NUT NHAN
   ================================================= */

#define BUTTON_PORT GPIOA
#define BUTTON_PIN  GPIO_Pin_0


/* =================================================
   UART DMA BUFFER
   ================================================= */

char tx_buffer[64];


/*
   uart_busy = 1:
       DMA dang truyen

   uart_busy = 0:
       DMA da truyen xong
*/

volatile uint8_t uart_busy = 0;


/*
   So lan nhan nut
*/

volatile uint32_t button_count = 0;


/* =================================================
   DELAY
   ================================================= */

void delay_ms(uint32_t ms)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 7200; j++)
        {
            __NOP();
        }
    }
}


/* =================================================
   GPIO INIT
   ================================================= */

void GPIO_Init_All(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;


    /*
    -----------------------------------------------
    Bat clock GPIOA
    -----------------------------------------------
    */

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA,
        ENABLE
    );


    /*
    -----------------------------------------------
    PA0 = BUTTON

    Pull-up noi

    Khong nhan:
        PA0 = 1

    Nhan:
        PA0 = 0
    -----------------------------------------------
    */

    GPIO_InitStructure.GPIO_Pin =
        BUTTON_PIN;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IPU;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(
        BUTTON_PORT,
        &GPIO_InitStructure
    );


    /*
    -----------------------------------------------
    PA9 = USART1 TX
    -----------------------------------------------
    */

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


    /*
    -----------------------------------------------
    PA10 = USART1 RX
    -----------------------------------------------
    */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_10;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IN_FLOATING;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
}


/* =================================================
   USART1 INIT
   ================================================= */

void USART1_Init(void)
{
    USART_InitTypeDef USART_InitStructure;


    /*
    Bat clock USART1
    */

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_USART1,
        ENABLE
    );


    /*
    UART = 115200 baud
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
    Bat USART1
    */

    USART_Cmd(
        USART1,
        ENABLE
    );
}


/* =================================================
   DMA USART1 TX
   DMA1 CHANNEL4
   ================================================= */

void USART1_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    /*
    -----------------------------------------------
    Bat clock DMA1
    -----------------------------------------------
    */

    RCC_AHBPeriphClockCmd(
        RCC_AHBPeriph_DMA1,
        ENABLE
    );


    /*
    -----------------------------------------------
    Reset DMA Channel 4
    -----------------------------------------------
    */

    DMA_DeInit(
        DMA1_Channel4
    );


    /*
    -----------------------------------------------
    Peripheral address

    USART1->DR
    -----------------------------------------------
    */

    DMA_InitStructure.DMA_PeripheralBaseAddr =
        (uint32_t)&USART1->DR;


    /*
    -----------------------------------------------
    Memory address

    tx_buffer
    -----------------------------------------------
    */

    DMA_InitStructure.DMA_MemoryBaseAddr =
        (uint32_t)tx_buffer;


    /*
    -----------------------------------------------
    Direction

    RAM -> USART
    -----------------------------------------------
    */

    DMA_InitStructure.DMA_DIR =
        DMA_DIR_PeripheralDST;


    /*
    Buffer size

    Se thay doi moi lan gui
    */

    DMA_InitStructure.DMA_BufferSize =
        0;


    /*
    Khong tang dia chi USART
    */

    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;


    /*
    Tang dia chi RAM
    */

    DMA_InitStructure.DMA_MemoryInc =
        DMA_MemoryInc_Enable;


    /*
    8 bit
    */

    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_Byte;

    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_Byte;


    /*
    Moi lan gui mot buffer
    */

    DMA_InitStructure.DMA_Mode =
        DMA_Mode_Normal;


    /*
    Uu tien cao
    */

    DMA_InitStructure.DMA_Priority =
        DMA_Priority_High;


    /*
    Khong memory-to-memory
    */

    DMA_InitStructure.DMA_M2M =
        DMA_M2M_Disable;


    /*
    Khoi tao DMA
    */

    DMA_Init(
        DMA1_Channel4,
        &DMA_InitStructure
    );


    /*
    -----------------------------------------------
    BAT NGAT TRANSFER COMPLETE
    -----------------------------------------------
    */

    DMA_ITConfig(
        DMA1_Channel4,
        DMA_IT_TC,
        ENABLE
    );


    /*
    -----------------------------------------------
    NVIC DMA1 CHANNEL4
    -----------------------------------------------
    */

    NVIC_InitStructure.NVIC_IRQChannel =
        DMA1_Channel4_IRQn;

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
    -----------------------------------------------
    Cho phep USART1 su dung DMA TX
    -----------------------------------------------
    */

    USART_DMACmd(
        USART1,
        USART_DMAReq_Tx,
        ENABLE
    );
}


/* =================================================
   UART SEND BANG DMA
   ================================================= */

void UART_DMA_Send(
    char *data,
    uint16_t length
)
{
    /*
    Neu DMA dang ban
    thi khong gui buffer moi
    */

    if (uart_busy)
    {
        return;
    }


    /*
    Bao DMA dang gui
    */

    uart_busy = 1;


    /*
    Tat DMA truoc khi thay doi cau hinh
    */

    DMA_Cmd(
        DMA1_Channel4,
        DISABLE
    );


    /*
    Xoa co Transfer Complete cu
    */

    DMA_ClearFlag(
        DMA1_FLAG_TC4
    );


    /*
    Dia chi buffer
    */

    DMA1_Channel4->CMAR =
        (uint32_t)data;


    /*
    So byte can gui
    */

    DMA_SetCurrDataCounter(
        DMA1_Channel4,
        length
    );


    /*
    Bat DMA
    */

    DMA_Cmd(
        DMA1_Channel4,
        ENABLE
    );
}


/* =================================================
   CHUYEN SO -> CHUOI
   ================================================= */

uint16_t NumberToString(
    char *buffer,
    uint32_t number
)
{
    char temp[12];

    uint8_t i = 0;
    uint8_t j;


    /*
    Truong hop number = 0
    */

    if (number == 0)
    {
        buffer[0] = '0';

        return 1;
    }


    /*
    Tach tung chu so
    */

    while (number > 0)
    {
        temp[i++] =
            '0' + (number % 10);

        number /= 10;
    }


    /*
    Dao lai thu tu
    */

    for (j = 0; j < i; j++)
    {
        buffer[j] =
            temp[i - j - 1];
    }


    return i;
}


/* =================================================
   TAO BAN TIN
   =================================================

   Vi du:

   HTN02N6:BTN:15\n\r
   ================================================= */

uint16_t Make_Message(
    char *buffer,
    uint32_t value
)
{
    uint16_t index = 0;

    uint16_t length;


    /*
    -----------------------------------------------
    ID LOP
    -----------------------------------------------
    */

    {
        const char *p = ID_LOP;

        while (*p)
        {
            buffer[index++] = *p++;
        }
    }


    /*
    -----------------------------------------------
    ID NHOM
    -----------------------------------------------
    */

    {
        const char *p = ID_NHOM;

        while (*p)
        {
            buffer[index++] = *p++;
        }
    }


    /*
    -----------------------------------------------
    ":BTN:"
    -----------------------------------------------
    */

    buffer[index++] = ':';
    buffer[index++] = 'B';
    buffer[index++] = 'T';
    buffer[index++] = 'N';
    buffer[index++] = ':';


    /*
    -----------------------------------------------
    Gia tri
    -----------------------------------------------
    */

    length =
        NumberToString(
            &buffer[index],
            value
        );

    index += length;


    /*
    -----------------------------------------------
    Xuong dong

    \n\r
    -----------------------------------------------
    */

    buffer[index++] = '\n';
    buffer[index++] = '\r';


    return index;
}


/* =================================================
   DMA1 CHANNEL4 INTERRUPT
   ================================================= */

void DMA1_Channel4_IRQHandler(void)
{
    /*
    Kiem tra Transfer Complete
    */

    if (
        DMA_GetITStatus(
            DMA1_IT_TC4
        ) != RESET
    )
    {
        /*
        Xoa co ngat
        */

        DMA_ClearITPendingBit(
            DMA1_IT_TC4
        );


        /*
        Tat DMA
        */

        DMA_Cmd(
            DMA1_Channel4,
            DISABLE
        );


        /*
        Bao DMA da ranh

        Lan nhan nut tiep theo
        co the gui du lieu moi
        */

        uart_busy = 0;
    }
}


/* =================================================
   MAIN
   ================================================= */

int main(void)
{
    uint8_t button_old = 1;
    uint8_t button_new;

    uint16_t message_length;


    /*
    -----------------------------------------------
    Khoi tao GPIO
    -----------------------------------------------
    */

    GPIO_Init_All();


    /*
    -----------------------------------------------
    Khoi tao UART
    -----------------------------------------------
    */

    USART1_Init();


    /*
    -----------------------------------------------
    Khoi tao DMA UART
    -----------------------------------------------
    */

    USART1_DMA_Init();


    /*
    -----------------------------------------------
    Vong lap chinh
    -----------------------------------------------
    */

    while (1)
    {
        /*
        Doc nut
        */

        button_new =
            GPIO_ReadInputDataBit(
                BUTTON_PORT,
                BUTTON_PIN
            );


        /*
        -------------------------------------------
        Phat hien canh xuong:

        Trang thai truoc = 1
        Trang thai hien tai = 0

        => Nut vua duoc nhan
        -------------------------------------------
        */

        if (
            button_old == 1 &&
            button_new == 0
        )
        {
            /*
            Chong doi nut
            */

            delay_ms(20);


            /*
            Kiem tra lai
            */

            if (
                GPIO_ReadInputDataBit(
                    BUTTON_PORT,
                    BUTTON_PIN
                ) == 0
            )
            {
                /*
                Tang bien dem
                */

                button_count++;


                /*
                Tao ban tin
                */

                message_length =
                    Make_Message(
                        tx_buffer,
                        button_count
                    );


                /*
                Gui bang DMA
                */

                UART_DMA_Send(
                    tx_buffer,
                    message_length
                );


                /*
                Cho nha nut

                Khong cho DMA o day,
                chi cho nguoi dung nha nut.
                */

                while (
                    GPIO_ReadInputDataBit(
                        BUTTON_PORT,
                        BUTTON_PIN
                    ) == 0
                )
                {
                }


                /*
                Chong doi khi nha nut
                */

                delay_ms(20);
            }
        }


        /*
        Cap nhat trang thai nut
        */

        button_old = button_new;
    }
}
