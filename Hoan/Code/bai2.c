#include <stdint.h>

/* ================= RCC ================= */

#define RCC_BASE        0x40021000UL
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18))

#define RCC_IOPCEN      (1 << 4)

/* ================= GPIOC ================= */

#define GPIOC_BASE      0x40011000UL

#define GPIOC_CRL       (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))

/* ================= SysTick ================= */

#define SYSTICK_BASE    0xE000E010UL

#define SYST_CSR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

#define SYSTICK_ENABLE      (1 << 0)
#define SYSTICK_CLKSRC      (1 << 2)
#define SYSTICK_COUNTFLAG   (1 << 16)


/* ================= Delay ================= */

void delay_ms(uint32_t ms)
{
    SYST_CSR = 0;

    /*
     * HSI = 8 MHz
     * 1 ms = 8000 clock
     */
    SYST_RVR = 8000 - 1;

    for (uint32_t i = 0; i < ms; i++)
    {
        SYST_CVR = 0;

        SYST_CSR = SYSTICK_ENABLE | SYSTICK_CLKSRC;

        while ((SYST_CSR & SYSTICK_COUNTFLAG) == 0)
        {
        }
    }

    SYST_CSR = 0;
}


/* ================= MAIN ================= */

int main(void)
{
    uint32_t i;

    /* Bật clock GPIOC */
    RCC_APB2ENR |= RCC_IOPCEN;

    /*
     * PC0 - PC7:
     *
     * MODE = 01 -> Output 10 MHz
     * CNF  = 00 -> General purpose push-pull
     *
     * Mỗi chân = 0001
     */
    GPIOC_CRL = 0x11111111;

    /* Tắt toàn bộ LED */
    GPIOC_ODR = 0x00;

    while (1)
    {
        /* ================= Chạy trái -> phải ================= */

        for (i = 0; i < 8; i++)
        {
            /* Chỉ bật 1 LED */
            GPIOC_ODR = (1 << i);

            delay_ms(200);
        }


        /* ================= Chạy phải -> trái ================= */

        for (i = 7; i > 0; i--)
        {
            GPIOC_ODR = (1 << i);

            delay_ms(200);
        }

        /* Bật PC0 trước khi quay lại */
        GPIOC_ODR = (1 << 0);

        delay_ms(200);
    }
}
