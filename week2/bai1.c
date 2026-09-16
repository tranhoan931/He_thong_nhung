#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#define malop "HTN02"
#define manhom "N6"
#define size_max 128

char rx_buffer[size_max];
uint16_t rx_index = 0;

void delay(unsigned int time){
    unsigned int i,j;
    for(i=0;i<time;i++){
        for(j=0;j<0x2aff;j++);
    }
}

void inituart(){
    USART_InitTypeDef uart;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    uart.USART_BaudRate = 9600;
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    uart.USART_Parity = USART_Parity_No;
    uart.USART_StopBits = USART_StopBits_1;
    uart.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1,&uart);
    USART_Cmd(USART1, ENABLE);
}

void initgpio(){
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &gpio);


}

void sendchar(char c){
    USART_SendData(USART1,(uint16_t)c);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void sendstring(char *str){
    while(*str){
        sendchar(*str);
        str++;
    }
}


char receichar(void){
    while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
    return (char)USART_ReceiveData(USART1);
}

void send_result(void){
    sendstring(malop);
    sendstring(" ");
    sendstring(manhom);
    sendstring(" ");
    sendstring(rx_buffer);
    sendstring("\r\n");
}

int main(void)
{
    char received_char;
    inituart();
    initgpio();
    while(1){
        received_char = receivechar();
        if(received_char == '!'){
            rx_buffer[rx_index] = '\0';
            send_result();
            rx_index = 0;
        }
        else{
            if(rx_index < size_max - 1){
                rx_buffer[rx_index]= received_char;
                rx_index++;
            }
            else{
                rx_index = 0;
            }
        }
    }
}
