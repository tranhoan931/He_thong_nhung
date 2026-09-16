#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"

volatile uint32_t msTick = 0;


/*==================== GPIO ====================*/

void initgpio(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &gpio);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2);
}


/*==================== SysTick ====================*/

void SysTick_Handler(void)
{
    msTick++;
}


void systick_init(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}


/*==================== MAIN ====================*/

int main(void)
{
    SystemInit();

    initgpio();

    systick_init();

    uint32_t last_0_1Hz = 0;
    uint32_t last_1Hz   = 0;
    uint32_t last_10Hz  = 0;

    while (1)
    {

        if ((msTick - last_0_1Hz) >= 5000)
        {
            last_0_1Hz += 5000;
            GPIOA->ODR ^= GPIO_Pin_0;
        }


        if ((msTick - last_1Hz) >= 500)
        {
            last_1Hz += 500;

            GPIOA->ODR ^= GPIO_Pin_1;
        }


        if ((msTick - last_10Hz) >= 50)
        {
            last_10Hz += 50;

            GPIOA->ODR ^= GPIO_Pin_2;
        }
    }
}

