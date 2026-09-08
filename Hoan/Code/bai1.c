#include<stm32f103xb.h>
#include<stdint.h>
void delay_ms(volatile uint32_t ms){
    while(ms--){
        __asm__("nop");
    }
}

int main(void){
    RCC->AHB2ENR |= RCC_AHB2ENR_IOPCEN;
    GPIOC->CRH &= ~(0xF << 20);
    GPIOC->CRH |= (0x2 << 20);

    uint32_t time = 1000000;

    while(1){
        GPIOC->ODR ^= (1 << 13);
        delay_ms(time);
    }
}

