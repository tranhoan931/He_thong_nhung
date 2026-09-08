#include "stm32f10x.h"

void GPIO_Config(void);
void Delay_ms(uint32_t ms);

int main(void)
{
    uint8_t led_state = 0;

    GPIO_Config();

    while (1)
    {
        // Kiểm tra nút nhấn
        if ((GPIOA->IDR & (1 << 0)) == 0)
        {
            // Chống dội phím
            Delay_ms(20);

            // Kiểm tra lại nút
            if ((GPIOA->IDR & (1 << 0)) == 0)
            {
                // Đảo trạng thái LED
                led_state = !led_state;

                if (led_state)
                {
                    GPIOA->BSRR = (1 << 8);       // LED ON
                }
                else
                {
                    GPIOA->BSRR = (1 << (8 + 16)); // LED OFF
                }

                // Chờ nhả nút
                while ((GPIOA->IDR & (1 << 0)) == 0);

                // Chống dội khi nhả
                Delay_ms(20);
            }
        }
    }
}


/*
 * PA0 : Input Pull-up
 * PA8 : Output Push-Pull
 */
void GPIO_Config(void)
{
    // Cấp clock GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /*
     * PA0 Input Pull-up/pull-down
     *
     * MODE = 00
     * CNF  = 10
     */
    GPIOA->CRL &= ~(0xF << 0);
    GPIOA->CRL |=  (0x8 << 0);

    /*
     * PA8 Output Push-Pull 2 MHz
     *
     * MODE = 10
     * CNF  = 00
     */
    GPIOA->CRH &= ~(0xF << 0);
    GPIOA->CRH |=  (0x2 << 0);

    // PA0 pull-up
    GPIOA->ODR |= (1 << 0);

    // LED OFF
    GPIOA->BSRR = (1 << (8 + 16));
}


/* Delay */
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
