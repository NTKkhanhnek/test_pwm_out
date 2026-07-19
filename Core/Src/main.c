#include <stdint.h>
#include "clock.h"
#include "pwm.h"
#include "uart2.h"

static uint16_t uart2_receive_uint(void)
{
    char ch;
    uint16_t number = 0;
    uint8_t has_digit = 0;

    while (1)
    {
        if (uart2_read_char(&ch) == 0)
        {
            continue;
        }

        if ((ch >= '0') && (ch <= '9'))
        {
            number = (uint16_t)((number * 10U) + (uint16_t)(ch - '0'));
            has_digit = 1;
        }
        else if (((ch == ' ') || (ch == '\r') || (ch == '\n')) && (has_digit != 0))
        {
            return number;
        }
    }
}

int main(void)
{
    uint16_t motor;
    uint16_t pulse_width;

    clock_init();
    uart2_init();
    pwm_init();

    for (motor = 1; motor <= 4; motor++)
    {
        control_motor((uint8_t)motor, 1100);
    }

    while (1)
    {
        motor = uart2_receive_uint();
        pulse_width = uart2_receive_uint();

        if ((motor >= 1) && (motor <= 4) && (pulse_width >= PWM_MIN_US) && (pulse_width <= PWM_MAX_US))
        {
            control_motor((uint8_t)motor, pulse_width);
            uart2_send_uint(motor);
            uart2_send_string(" ");
            uart2_send_uint(pulse_width);
            uart2_send_string("\r\n");
        }
        else
        {
            uart2_send_string("ERR\r\n");
        }
    }
}
