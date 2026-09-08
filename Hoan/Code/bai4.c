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

#define GPIOC_BSRR      (*(volatile uint32_t *)(GPIOC_BASE + 0x10))
#define GPIOC_BRR       (*(volatile uint32_t *)(GPIOC_BASE + 0x14))


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
    uint32_t led_state = 0;

    /* Bật clock GPIOC */
    RCC_APB2ENR |= RCC_IOPCEN;


    /*
     * ==========================
     * PC0: INPUT PULL-UP
     * ==========================
     *
     * MODE = 00
     * CNF  = 10
     *
     * PC0 = 1000
     */
    GPIOC_CRL &= ~(0xF << 0);
    GPIOC_CRL |=  (0x8 << 0);


    /*
     * Kích hoạt Pull-up cho PC0
     *
     * ODR0 = 1
     */
    GPIOC_ODR |= (1 << 0);


    /*
     * ==========================
     * PC13: OUTPUT PUSH-PULL
     * ==========================
     *
     * MODE13 = 01 -> Output 10 MHz
     * CNF13  = 00 -> Push-pull
     *
     * PC13 nằm ở bit 20-23 CRH
     */
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |=  (0x1 << 20);


    /*
     * LED onboard Blue Pill
     * active LOW
     *
     * 1 -> LED OFF
     */
    GPIOC_BSRR = (1 << 13);


    while (1)
    {
        /*
         * Kiểm tra nút
         *
         * PC0 = 0 khi nhấn
         */
        if ((GPIOC_IDR & (1 << 0)) == 0)
        {
            /* Chống dội */
            delay_ms(20);


            /* Kiểm tra lại */
            if ((GPIOC_IDR & (1 << 0)) == 0)
            {
                /*
                 * Đảo trạng thái LED
                 */
                led_state = !led_state;


                if (led_state)
                {
                    /*
                     * LED ON
                     * PC13 = 0
                     */
                    GPIOC_BRR = (1 << 13);
                }
                else
                {
                    /*
                     * LED OFF
                     * PC13 = 1
                     */
                    GPIOC_BSRR = (1 << 13);
                }


                /*
                 * Chờ nhả nút
                 */
                while ((GPIOC_IDR & (1 << 0)) == 0)
                {
                }


                /* Chống dội khi nhả */
                delay_ms(20);
            }
        }
    }
}
