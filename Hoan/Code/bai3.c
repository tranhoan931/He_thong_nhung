#include "stm32f10x.h"

void GPIO_Config(void);

int main(void)
{
    uint8_t data;

    GPIO_Config();

    while (1)
    {
        // Đọc PA0 - PA7
        data = GPIOA->IDR & 0xFF;

        // Đảo dữ liệu
        data = ~data;

        // Chỉ lấy 8 bit
        data &= 0xFF;

        // Xóa PA8 - PA15
        GPIOA->ODR &= 0x00FF;

        // Ghi dữ liệu vào PA8 - PA15
        GPIOA->ODR |= ((uint16_t)data << 8);
    }
}


/*
 * PA0 - PA7  : Input Floating
 * PA8 - PA15 : Output Push-Pull 2 MHz
 */
void GPIO_Config(void)
{
    // Cấp clock GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /*
     * CRL:
     * PA0 - PA7 Input Floating
     * CNF = 01
     * MODE = 00
     *
     * Mỗi chân = 0x4
     */
    GPIOA->CRL = 0x44444444;

    /*
     * CRH:
     * PA8 - PA15 Output Push-Pull 2 MHz
     * MODE = 10
     * CNF = 00
     *
     * Mỗi chân = 0x2
     */
    GPIOA->CRH = 0x22222222;

    // Tắt LED
    GPIOA->ODR &= 0x00FF;
}
