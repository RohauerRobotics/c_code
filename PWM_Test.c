#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <math.h>

// Define the GPIO pins
#define PWM_PIN    20  // ENA1 pin (PWM output; will be level-shifted to 5V)
#define DIR_PIN1   19  // IN1: Direction control pin 1
#define DIR_PIN2   21  // IN2: Direction control pin 2

// PWM parameters
#define PWM_WRAP        1000      // PWM counter range: 0 to 1000
#define PWM_FREQUENCY   20000     // Desired PWM frequency in Hz

int main() {
    // Initialize stdio (optional if you want to use USB serial output)
    stdio_init_all();

    // Set up the direction pins as digital outputs.
    // For “forward” motion, we set IN1 high and IN2 low.
    gpio_init(DIR_PIN1);
    gpio_set_dir(DIR_PIN1, GPIO_OUT);
    gpio_put(DIR_PIN1, 1);

    gpio_init(DIR_PIN2);
    gpio_set_dir(DIR_PIN2, GPIO_OUT);
    gpio_put(DIR_PIN2, 0);

    // Set up PWM on PWM_PIN (GP20)
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    uint channel = pwm_gpio_to_channel(PWM_PIN);

    // Set the PWM wrap (defines the period)
    pwm_set_wrap(slice_num, PWM_WRAP);

    // Calculate and set the clock divider:
    // PWM frequency = 125e6 / (divider * (PWM_WRAP + 1))
    float divider = 125000000.0f / (PWM_FREQUENCY * (PWM_WRAP + 1));
    pwm_set_clkdiv(slice_num, divider);

    // Start with 0 duty cycle.
    pwm_set_chan_level(slice_num, channel, 0);
    pwm_set_enabled(slice_num, true);

    // Parameters for the sinusoidal modulation:
    const float sine_period = 2.0f;   // 2 seconds for a full sine cycle
    float time_elapsed = 0.0f;          // Keeps track of time in seconds
    const float dt = 0.01f;             // Update interval in seconds (10 ms)

    while (true) {
        // Compute the sine value (range: -1 to 1)
        float sine_value = sinf((2 * M_PI * time_elapsed) / sine_period);
        // Map the sine value to a duty cycle ratio between 0 and 1
        float duty_cycle_ratio = (sine_value + 1.0f) / 2.0f;
        // Convert the ratio to a PWM level (0 to PWM_WRAP)
        uint pwm_level = (uint)(duty_cycle_ratio * PWM_WRAP);

        // Update the PWM duty cycle
        pwm_set_chan_level(slice_num, channel, pwm_level);

        // Wait for the next update
        sleep_ms((int)(dt * 1000));
        time_elapsed += dt;
    }

    return 0;
}
