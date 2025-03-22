/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// serial print library --> stdio.h and disable uart in cmakelist.txt
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <math.h>
#include <stdlib.h>

// define motor pins
#define PWM_PIN 20
#define IN1 19
#define IN2 21
// define led pin number
#define MYLED 25

// define wrap counter --> what pwm counts to
#define PWM_WRAP 1000
// define frequency of PWM signal
#define PWM_FREQ 20000

// Perform initialisation and set motor direction
void in_pin_init(void) {
    gpio_init(IN1);
    gpio_set_dir(IN1,GPIO_OUT);
    gpio_init(IN2);
    gpio_set_dir(IN2,GPIO_OUT);
    gpio_put(IN1,1); // PIN 19
    gpio_put(IN2,0); // PIN 21
}

int main() {
    stdio_init_all(); //enables printf()
    in_pin_init(); // sets motor direction

    // set pin 20 to act as pwm pin
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    // determine slice number and channel number
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    uint channel = pwm_gpio_to_channel(PWM_PIN);

    gpio_set_function(MYLED, GPIO_FUNC_PWM);
    uint led_slice = pwm_gpio_to_slice_num(MYLED);
    uint led_channel = pwm_gpio_to_channel(MYLED);

    //set PWM wrap 
    pwm_set_wrap(slice_num,PWM_WRAP);
    pwm_set_wrap(led_slice,PWM_WRAP);

    // calculate and set clock divider
    float divider = 133000000.0f/(PWM_FREQ*(PWM_WRAP+1));
    pwm_set_clkdiv(slice_num, divider);
    pwm_set_clkdiv(led_slice, divider);

    // start with 0 duty cycle
    pwm_set_chan_level(slice_num,channel,0);
    pwm_set_enabled(slice_num,true);
    pwm_set_chan_level(led_slice,channel,0);
    pwm_set_enabled(led_slice,true);

    // sinusoidal parameters
    const float sine_period = 2.0f;
    float time_elapsed = 0.0f;
    const float dt = 0.01f;

    bool in1_val = true;
    bool in2_val = false;
    while (true) {
        //compute sine value (-1 to 1)
        float cos_val = cosf((2*M_PI*time_elapsed)/sine_period);
        // map sine value to a duty cycle between 0 and 1
        float duty_cycle_ratio = (-cos_val + 1.0f)/2.0f;

        // convert ratio to PWM Level (0 to 1000)
        uint pwm_level = (uint)(duty_cycle_ratio*PWM_WRAP);

        //update PWM duty cycle
        pwm_set_chan_level(slice_num,channel,pwm_level);
        pwm_set_chan_level(led_slice,led_channel,pwm_level);

        // wait for next update
        sleep_ms((int)(dt*1000));
        time_elapsed += dt;

        if (time_elapsed>=2){
            time_elapsed = 0;
            in1_val = !in1_val;
            in2_val = !in2_val;
            gpio_put(IN1,in1_val);
            gpio_put(IN2,in2_val);
            printf("The direction has flipped IN1: %d IN2: %d", in1_val, in2_val);
        }
    }
}
