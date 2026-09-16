#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"

void PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );

    /* =====================================================
       2. CONFIG PA0 - PA3
          PA0 = TIM2_CH1
          PA1 = TIM2_CH2
          PA2 = TIM2_CH3
          PA3 = TIM2_CH4
       ===================================================== */

    GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0 |GPIO_Pin_1 |GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, DISABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, DISABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM2, DISABLE);

    /* =====================================================
       3. TIMER 2
       
       Clock TIM2 = 72 MHz

       Prescaler = 71
       => 72 MHz / 72 = 1 MHz

       ARR = 999
       => 1 MHz / 1000 = 1 kHz
       ===================================================== */

    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_CounterMode =  TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit( TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OCInitStructure.TIM_Pulse = 100;
    TIM_OC1Init( TIM2, &TIM_OCInitStructure );
    TIM_OC1PreloadConfig( TIM2,TIM_OCPreload_Enable );

    TIM_OCInitStructure.TIM_Pulse = 300;
    TIM_OC2Init( TIM2,&TIM_OCInitStructure );
    TIM_OC2PreloadConfig( TIM2, TIM_OCPreload_Enable);

    TIM_OCInitStructure.TIM_Pulse = 500;
    TIM_OC3Init(TIM2,&TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable );

    TIM_OCInitStructure.TIM_Pulse = 700;
    TIM_OC4Init(TIM2,&TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM2,TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2,ENABLE);
    TIM_Cmd(IM2,ENABLE);
}


int main(void)
{
    SystemInit();
    PWM_Init();
    while (1)
    {
    }
}
