
#ifndef INC_PWM_H_
#define INC_PWM_H_

#include <stdint.h>

#define PWM_MIN_US 1100
#define PWM_MAX_US 1950

void pwm_init(void);
uint16_t pwm_clamp_pulse(uint16_t pulse_width);
void control_motor(uint8_t motor, uint16_t pulse_width);

#endif /* INC_PWM_H_ */
