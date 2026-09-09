#include <stdint.h>

// Địa chỉ gốc Bus APB2 & AHB
#define RCC_BASE        0x40021000
#define GPIOA_BASE      0x40010800

// Thanh ghi RCC
#define RCC_APB2ENR     (*((volatile uint32_t *)(RCC_BASE + 0x18)))

// Thanh ghi GPIOA
#define GPIOA_CRL       (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_CRH       (*((volatile uint32_t *)(GPIOA_BASE + 0x04)))
#define GPIOA_IDR       (*((volatile uint32_t *)(GPIOA_BASE + 0x08)))
#define GPIOA_ODR       (*((volatile uint32_t *)(GPIOA_BASE + 0x0C)))
#define GPIOA_ODR_BYTE  (*((volatile uint8_t  *)(GPIOA_BASE + 0x0C)))

void delay(volatile uint32_t count) {
    while (count--) __asm__("nop");
}
int main(void) {
    // 1. Bật clock cho PORTA (Bit 2 trong RCC_APB2ENR)
    RCC_APB2ENR |= (1 << 2);

    // 2. Cấu hình PA0 -> PA7 là Output Push-Pull 50MHz (mã 0011 = 0x3)
    GPIOA_CRL = 0x33333333;

    uint8_t led_data;
    
    while (1) {
        // Chạy từ trái sang phải (PA0 -> PA7)
        led_data = 0x01;
        for (int i = 0; i < 8; i++) {
            GPIOA_ODR_BYTE = led_data;
            delay(500000);
            led_data <<= 1;
        }

        // Chạy từ phải sang trái (PA6 -> PA1)
        led_data = 0x40;
        for (int i = 0; i < 6; i++) {
            GPIOA_ODR_BYTE = led_data;
            delay(500000);
            led_data >>= 1;
        }
    }
    return 0;
}
