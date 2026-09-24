#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include <stdint.h>

/* =========================
   MAX7219 CONNECTION
   =========================
   PA5 -> CLK
   PA7 -> DIN
   PA4 -> CS/LOAD
   GND -> GND
   VCC -> 5V
   ========================= */

#define MAX7219_CS_LOW() \
    GPIO_ResetBits(GPIOA, GPIO_Pin_4)

#define MAX7219_CS_HIGH() \
    GPIO_SetBits(GPIOA, GPIO_Pin_4)


/* =========================
   Delay
   ========================= */
void delay_ms(uint32_t ms)
{
    uint32_t i, j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 7200; j++)
        {
            __NOP();
        }
    }
}


/* =========================
   SPI1 INIT
   ========================= */
void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    /* Clock GPIOA + SPI1 */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_SPI1,
        ENABLE
    );

    /* PA5 = SPI1_SCK
       PA7 = SPI1_MOSI */
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_5 |
        GPIO_Pin_7;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /* PA6 = MISO
       Không sử dụng nhưng cấu hình input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /* PA4 = CS/LOAD */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    MAX7219_CS_HIGH();


    /* SPI1 */
    SPI_InitStructure.SPI_Direction =
        SPI_Direction_2Lines_FullDuplex;

    SPI_InitStructure.SPI_Mode =
        SPI_Mode_Master;

    SPI_InitStructure.SPI_DataSize =
        SPI_DataSize_8b;

    SPI_InitStructure.SPI_CPOL =
        SPI_CPOL_Low;

    SPI_InitStructure.SPI_CPHA =
        SPI_CPHA_1Edge;

    SPI_InitStructure.SPI_NSS =
        SPI_NSS_Soft;

    /* 72 MHz / 16 = 4.5 MHz */
    SPI_InitStructure.SPI_BaudRatePrescaler =
        SPI_BaudRatePrescaler_16;

    SPI_InitStructure.SPI_FirstBit =
        SPI_FirstBit_MSB;

    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}


/* =========================
   SPI SEND BYTE
   ========================= */
void SPI1_SendByte(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(
               SPI1,
               SPI_I2S_FLAG_TXE) == RESET)
    {
    }

    SPI_I2S_SendData(SPI1, data);

    while (SPI_I2S_GetFlagStatus(
               SPI1,
               SPI_I2S_FLAG_BSY) == SET)
    {
    }
}


/* =========================
   MAX7219 WRITE
   ========================= */
void MAX7219_Write(uint8_t address, uint8_t data)
{
    MAX7219_CS_LOW();

    SPI1_SendByte(address);
    SPI1_SendByte(data);

    MAX7219_CS_HIGH();
}


/* =========================
   MAX7219 INIT
   ========================= */
void MAX7219_Init(void)
{
    /* Display Test OFF */
    MAX7219_Write(0x0F, 0x00);

    /* Normal operation */
    MAX7219_Write(0x0C, 0x01);

    /* Decode Mode cho cả 8 digit */
    MAX7219_Write(0x09, 0xFF);

    /* Scan cả 8 digit */
    MAX7219_Write(0x0B, 0x07);

    /* Độ sáng trung bình */
    MAX7219_Write(0x0A, 0x08);

    /* Xóa 8 digit */
    MAX7219_Write(0x01, 0x0F);
    MAX7219_Write(0x02, 0x0F);
    MAX7219_Write(0x03, 0x0F);
    MAX7219_Write(0x04, 0x0F);
    MAX7219_Write(0x05, 0x0F);
    MAX7219_Write(0x06, 0x0F);
    MAX7219_Write(0x07, 0x0F);
    MAX7219_Write(0x08, 0x0F);
}


const uint8_t digit_code[10] =
{
    0x7E,   // 0
    0x30,   // 1
    0x6D,   // 2
    0x79,   // 3
    0x33,   // 4
    0x5B,   // 5
    0x5F,   // 6
    0x70,   // 7
    0x7F,   // 8
    0x7B    // 9
};


/* =========================
   HIỂN THỊ 12345678
   ========================= */
void MAX7219_Display12345678(void)
{
    MAX7219_Write(0x01, 8);
    MAX7219_Write(0x02, 7);
    MAX7219_Write(0x03, 6);
    MAX7219_Write(0x04, 5);

    MAX7219_Write(0x05, 4);
    MAX7219_Write(0x06, 3);
    MAX7219_Write(0x07, 2);
    MAX7219_Write(0x08, 1);
}

/* =========================
   MAIN
   ========================= */
int main(void)
{
    SPI1_Init();
    delay_ms(100);

    MAX7219_Init();

    /* Decode mode */
    MAX7219_Write(0x09, 0xFF);

    /* Tất cả 8 digit đều hiện số 5 */
    MAX7219_Display12345678();

    while (1)
    {
    }
}
