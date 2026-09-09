#include <stdint.h>

/* ================= RCC ================= */

#define RCC_BASE        0x40021000UL
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18))

#define RCC_IOPCEN      (1 << 4)

/* ================= GPIOC ================= */

#define GPIOC_BASE      0x40011000UL

#define GPIOC_CRH       (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_BSRR      (*(volatile uint32_t *)(GPIOC_BASE + 0x10))
#define GPIOC_BRR       (*(volatile uint32_t *)(GPIOC_BASE + 0x14))

/* ================= SysTick ================= */

#define SYSTICK_BASE    0xE000E010UL

#define SYST_CSR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

#define SYSTICK_ENABLE  (1 << 0)
#define SYSTICK_CLKSRC  (1 << 2)
#define SYSTICK_COUNTFLAG (1 << 16)


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


int main(void)
{
    /* Bật clock GPIOC */
    RCC_APB2ENR |= RCC_IOPCEN;

    /*
     * PC13:
     * MODE13 = 01 -> Output 10 MHz
     * CNF13  = 00 -> General purpose push-pull
     *
     * PC13 nằm ở bit 20-23 của CRH.
     */
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |=  (0x1 << 20);

    while (1)
    {
        /* LED ON - Blue Pill LED active LOW */
        GPIOC_BRR = (1 << 13);

        delay_ms(1000);

        /* LED OFF */
        GPIOC_BSRR = (1 << 13);

        delay_ms(1000);
    }
}
