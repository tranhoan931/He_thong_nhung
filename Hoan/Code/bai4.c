#include <stdint.h>

// Định nghĩa địa chỉ thanh ghi cơ bản
#define RCC_BASE        0x40021000
#define GPIOA_BASE      0x40010800

#define RCC_APB2ENR     (*((volatile uint32_t *)(RCC_BASE + 0x18)))
#define GPIOA_CRL       (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_IDR       (*((volatile uint32_t *)(GPIOA_BASE + 0x08)))
#define GPIOA_ODR       (*((volatile uint32_t *)(GPIOA_BASE + 0x0C)))

void delay(volatile uint32_t count) {
    while (count--) __asm__("nop");
}

int main(void) {
    // 1. Bật Clock cho PORTA
    RCC_APB2ENR |= (1 << 2);

    // 2. Cấu hình PA0 là Input Pull-up/Pull-down (mã 0b1000 = 0x8)
    // Cấu hình PA1 là Output Push-Pull 50MHz (mã 0b0011 = 0x3)
    GPIOA_CRL &= ~(0xFF);         // Xóa cấu hình cũ của PA0 và PA1
    GPIOA_CRL |= (0x8 << 0) |     // PA0 = Input Pull-up/Pull-down
                 (0x3 << 4);      // PA1 = Output Push-Pull

    // Kích hoạt Pull-up cho PA0 (Ghi bit 0 vào ODR = 1)
    GPIOA_ODR |= (1 << 0);

    uint8_t button_prev = 1; // Mặc định nút chưa nhấn (mức cao do Pull-up)

    while (1) {
        // Đọc trạng thái chân PA0
        uint8_t button_curr = (GPIOA_IDR & (1 << 0)) ? 1 : 0;

        // Phát hiện cạnh xuống: Nút chuyển từ 1 (thả) sang 0 (nhấn)
        if (button_prev == 1 && button_curr == 0) {
            delay(50000); // Chống rung phím (Debounce)

            // Kiểm tra lại sau khi chống rung để xác nhận nút thực sự được nhấn
            if (!(GPIOA_IDR & (1 << 0))) {
                // Chờ cho đến khi người dùng NHẢ NÚT (PA0 quay lại mức 1)
                while (!(GPIOA_IDR & (1 << 0)));
                delay(50000); // Chống rung khi nhả nút

                // Đảo trạng thái LED tại PA1
                GPIOA_ODR ^= (1 << 1);
            }
        }

        button_prev = button_curr;
    }

    return 0;
}
