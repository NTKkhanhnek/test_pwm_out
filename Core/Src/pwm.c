#include "pwm.h"
#include "clock.h"

#define GPIOB_ADDR 0x40020400UL
#define TIM4_ADDR  0x40000800UL

uint16_t pwm_clamp_pulse(uint16_t pulse_width)
{
    if (pulse_width < PWM_MIN_US)
    {
        return PWM_MIN_US;
    }

    if (pulse_width > PWM_MAX_US)
    {
        return PWM_MAX_US;
    }

    return pulse_width;
}

void pwm_init(void)
{
    volatile uint32_t* GPIOB_MODER   = (volatile uint32_t*)(GPIOB_ADDR + 0x00UL);
    volatile uint32_t* GPIOB_OTYPER  = (volatile uint32_t*)(GPIOB_ADDR + 0x04UL);
    volatile uint32_t* GPIOB_OSPEEDR = (volatile uint32_t*)(GPIOB_ADDR + 0x08UL);
    volatile uint32_t* GPIOB_PUPDR   = (volatile uint32_t*)(GPIOB_ADDR + 0x0CUL);
    volatile uint32_t* GPIOB_AFRL    = (volatile uint32_t*)(GPIOB_ADDR + 0x20UL);
    volatile uint32_t* GPIOB_AFRH    = (volatile uint32_t*)(GPIOB_ADDR + 0x24UL);

    volatile uint32_t* TIM4_CR1   = (volatile uint32_t*)(TIM4_ADDR + 0x00UL);
    volatile uint32_t* TIM4_EGR   = (volatile uint32_t*)(TIM4_ADDR + 0x14UL);
    volatile uint32_t* TIM4_CCMR1 = (volatile uint32_t*)(TIM4_ADDR + 0x18UL);
    volatile uint32_t* TIM4_CCMR2 = (volatile uint32_t*)(TIM4_ADDR + 0x1CUL);
    volatile uint32_t* TIM4_CCER  = (volatile uint32_t*)(TIM4_ADDR + 0x20UL);
    volatile uint32_t* TIM4_PSC   = (volatile uint32_t*)(TIM4_ADDR + 0x28UL);
    volatile uint32_t* TIM4_ARR   = (volatile uint32_t*)(TIM4_ADDR + 0x2CUL);
    volatile uint32_t* TIM4_CCR1  = (volatile uint32_t*)(TIM4_ADDR + 0x34UL);
    volatile uint32_t* TIM4_CCR2  = (volatile uint32_t*)(TIM4_ADDR + 0x38UL);
    volatile uint32_t* TIM4_CCR3  = (volatile uint32_t*)(TIM4_ADDR + 0x3CUL);
    volatile uint32_t* TIM4_CCR4  = (volatile uint32_t*)(TIM4_ADDR + 0x40UL);

    clock_enable_AHB1(GPIOB_peripheral);
    clock_enable_APB1(TIM4_peripheral);

    *TIM4_CR1 = 0; // tat timer

    *GPIOB_MODER &= ~(0xFF << 12);
    *GPIOB_MODER |= (0xAA << 12); // PB6-PB9 -> alternate function mode

    *GPIOB_OTYPER &= ~((1 << 6) | (1 << 7) | (1 << 8) | (1 << 9)); // push - pull

    *GPIOB_OSPEEDR &= ~(0xFF << 12);
    *GPIOB_OSPEEDR |= (0xAA << 12); // high speed

    *GPIOB_PUPDR &= ~(0xFF << 12); // no pull-up, pull-down.

    *GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28));
    *GPIOB_AFRL |= ((2 << 24) | (2 << 28));

    *GPIOB_AFRH &= ~((0xF << 0) | (0xF << 4));
    *GPIOB_AFRH |= ((2 << 0) | (2 << 4));

    *TIM4_PSC = 100 - 1; // 100MHz / 100 = 1MHz => 1cnt = 1us 
    *TIM4_ARR = 20000 - 1; // => 20000 cnt = 20ms

    *TIM4_CCR1 = PWM_MIN_US;
    *TIM4_CCR2 = PWM_MIN_US;
    *TIM4_CCR3 = PWM_MIN_US;
    *TIM4_CCR4 = PWM_MIN_US;

    *TIM4_CCMR1 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11); 
    // Mode 1 - channel 1,2 ; preload cr1, cr2

    *TIM4_CCMR2 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);
    // Mode 1 - channel 3,4 ; preload cr3, cr4

    *TIM4_CCER = (1 << 0) | (1 << 4) | (1 << 8) | (1 << 12);

    *TIM4_CR1 |= (1 << 7);
    *TIM4_EGR = 1;
    *TIM4_CR1 |= 1; // bat timer
}

void control_motor(uint8_t motor, uint16_t pulse_width)
{
    volatile uint32_t* TIM4_CCR[4] =
    {
        (volatile uint32_t*)(TIM4_ADDR + 0x34),
        (volatile uint32_t*)(TIM4_ADDR + 0x38),
        (volatile uint32_t*)(TIM4_ADDR + 0x3C),
        (volatile uint32_t*)(TIM4_ADDR + 0x40)
    };

    if ((motor < 1) || (motor > 4))
    {
        return;
    }

    *TIM4_CCR[motor - 1] = pwm_clamp_pulse(pulse_width);
}
