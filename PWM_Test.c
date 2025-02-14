#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "math.h"

#define PWM_PIN 20  // ENA1
#define IN1 19      // Direction control
#define IN2 21      // Direction control
#define PWM_FREQ 1000  // PWM frequency in Hz
#define MAX_DUTY 65535  // 100% duty cycle
#define MIN_DUTY 0      // 0% duty cycle
#define PERIOD_MS 2000  // One full sine cycle duration in ms

void init_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice_num, MAX_DUTY);  
    pwm_set_enabled(slice_num, true);
}

void set_motor_direction(bool forward) {
    gpio_put(IN1, forward);
    gpio_put(IN2, !forward);
}

int main() {
    stdio_init_all();
    
    // Initialize GPIO
    gpio_init(IN1);
    gpio_init(IN2);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_set_dir(IN2, GPIO_OUT);

    // Initialize PWM
    init_pwm(PWM_PIN);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    while (1) {
        uint32_t elapsed_time = to_ms_since_boot(get_absolute_time()) - start_time;
        float theta = 2 * M_PI * elapsed_time / PERIOD_MS;
        float sine_val = (sinf(theta) + 1.0) / 2.0;  // Normalize to [0,1]

        uint16_t duty_cycle = (uint16_t)(sine_val * MAX_DUTY);
        pwm_set_gpio_level(PWM_PIN, duty_cycle);

        // Change direction every half cycle
        set_motor_direction(theta < M_PI);

        sleep_ms(10); // Small delay for smoother transition
    }

    return 0;
}
