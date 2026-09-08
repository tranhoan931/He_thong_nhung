#include <stdint.h>

/* ================= RCC ================= */

#define RCC_BASE        0x40021000UL
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18))

#define RCC_IOPCEN      (1 << 4)

/* ================= GPIOC ================= */

#define GPIOC_BASE      0x40011000UL

#define GPIOC_CRL       (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_CRH       (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_IDR       (*(volatile uint32_t *)(GPIOC_BASE + 0x08))
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))


int main(void)
{
    uint32_t data;

    /* Bật clock GPIOC */
    RCC_APB2ENR |= RCC_IOPCEN;

    /*
     * ==========================
     * PC0 - PC7: INPUT
     * ==========================
     *
     * MODE = 00 -> Input
     * CNF  = 01 -> Floating input
     *
     * Mỗi chân = 0100
     */
    GPIOC_CRL = 0x44444444;


    /*
     * ==========================
     * PC8 - PC15: OUTPUT
     * ==========================
     *
     * MODE = 01 -> Output 10 MHz
     * CNF  = 00 -> Push-pull
     *
     * Mỗi chân = 0001
     */
    GPIOC_CRH = 0x11111111;


    while (1)
    {
        /*
         * Đọc PC0 - PC7
         */
        data = GPIOC_IDR & 0xFF;


        /*
         * Đảo 8 bit
         */
        data = (~data) & 0xFF;


        /*
         * Xóa PC8 - PC15
         */
        GPIOC_ODR &= 0x00FF;


        /*
         * Đưa dữ liệu lên PC8 - PC15
         */
        GPIOC_ODR |= (data << 8);
    }
}
