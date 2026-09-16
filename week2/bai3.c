#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_usart.h"

void delay(unsigned int time)
{
    for(unsigned int i=0;i<time;i++)
    {
        SysTick->LOAD = 72000 - 1;
        SysTick->VAL = 0;
         SysTick->CTRL = 5;
        while((SysTick->CTRL & 0x10000) == 0);
         SysTick->CTRL = 0;
    }
}
void inituart(){
    USART_InitTypeDef uart; GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    uart.USART_BaudRate = 9600;
    uart.USART_WordLength = USART_WordLength_8b;
    uart.USART_StopBits = USART_StopBits_1;
    uart.USART_Parity = USART_Parity_No;
    uart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &uart);
    USART_Cmd(USART1, ENABLE);
}

void adc_init(){
    ADC_InitTypeDef adc; GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1|RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    ADC_DeInit(ADC1);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = ENABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));

}

uint16_t ADC1_Read(void){
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

void adc_start(){
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void USART1_SendChar(char c){
    USART_SendData(USART1, c);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

}
void USART1_SendString(char *str){
    while(*str){
        USART1_SendChar(*str);
        str++;
    }
}

int main(void){
    inituart();
    adc_init();
    uint16_t adc_value;
    uint32_t voltage_mV;
    while(1){
        adc_value = ADC1_Read();
        voltage_mV = (uint32_t)adc_value * 3300 / 4095;
        USART1_SendString("ADC Value: ");
        {
            uint16_t temp = adc_value;
            char buffer[6];
            int i =0;
            if(temp == 0){
                USART1_SendChar('0');
            }else{
                while(temp > 0){
                    buffer[i++] = (temp % 10) + '0';
                    temp /= 10;
                }
                while(i > 0){
                    USART1_SendChar(buffer[--i]);
                }
            }
        }
        USART1_SendString(", Voltage: ");
        USART1_SendChar((voltage_mV / 1000) + '0');
        USART1_SendChar('.');
        USART1_SendChar(((voltage_mV % 1000) / 100) + '0');
        USART1_SendChar(((voltage_mV % 100) / 10) + '0');
        USART1_SendChar((voltage_mV % 10) + '0');
        USART1_SendString(" mV\r\n");
        delay(1000);
    }
}

