
#include "stm32f10x.h"

void GPIO_Config(void);
void Delay_ms(uint32_t ms);

int main(void)
{
    uint8_t led = 0;
    int8_t direction = 1;

    GPIO_Config();

    while (1)
    {
        // Tắt toàn bộ LED
        GPIOA->ODR &= ~(0xFF);

        // Bật LED hiện tại
        GPIOA->ODR |= (1 << led);

        Delay_ms(200);

        // Đang chạy sang phải
        if (direction == 1)
        {
            if (led == 7)
            {
                direction = -1;
                led--;
            }
            else
            {
                led++;
            }
        }
        // Đang chạy sang trái
        else
        {
            if (led == 0)
            {
                direction = 1;
                led++;
            }
            else
            {
                led--;
            }
        }
    }
}


/* PA0 - PA7: Output Push-Pull */
void GPIO_Config(void)
{
    // Cấp clock cho GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // PA0 - PA7 Output Push-Pull, 2 MHz
    GPIOA->CRL = 0x22222222;

    // Tắt LED ban đầu
    GPIOA->ODR &= ~(0xFF);
}


/* Delay đơn giản */
void Delay_ms(uint32_t ms)
{
    uint32_t i, j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 8000; j++)
        {
            __NOP();
        }
    }
}
