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
    // 1. Bật clock cho PORTA
    RCC_APB2ENR |= (1 << 2);

    // 2. PA0-PA7 ở chế độ Input với Pull-up/Pull-down (mã 1000 = 0x8)
    GPIOA_CRL = 0x88888888;
    // Bật Pull-up cho PA0-PA7 (Ghi 1 vào các bit ODR tương ứng)
    GPIOA_ODR_BYTE = 0xFF;

    // 3. PA8-PA15 ở chế độ Output Push-Pull 50MHz (mã 0011 = 0x3)
    GPIOA_CRH = 0x33333333;

    while (1) {
        // Đọc 8 bit thấp từ IDR (PA0 - PA7)
        uint8_t input_val = (uint8_t)(GPIOA_IDR & 0xFF);

        // Đảo toàn bộ dữ liệu bit (0 thành 1, 1 thành 0)
        uint8_t inverted_val = ~input_val;

        // Xóa 8 bit cao hiện tại (PA8-PA15) và ghi giá trị mới vào
        GPIOA_ODR = (GPIOA_ODR & 0x00FF) | ((uint32_t)inverted_val << 8);
    }
    return 0;
}
