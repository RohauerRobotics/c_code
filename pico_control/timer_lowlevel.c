/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <math.h>

// define delay in seconds
const double delay_sec =  0.0005;

// elapsed time
static double elapsed_time = 0.0;

// define global data buffer
#define BUFF_LEN 200
static double data_buffer[BUFF_LEN];
static double *db_p = data_buffer;

// define global count
static int count = 0;

// Note on RP2350 timer_hw is the default timer instance (as per PICO_DEFAULT_TIMER)

/// \tag::get_time[]
// Simplest form of getting 64 bit time from the timer.
// It isn't safe when called from 2 cores because of the latching
// so isn't implemented this way in the sdk
static uint64_t get_time(void) {
    // Reading low latches the high value
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return ((uint64_t) hi << 32u) | lo;
}
/// \end::get_time[]

/// \tag::alarm_standalone[]
// Use alarm 0
#define ALARM_NUM 0
#define ALARM_IRQ timer_hardware_alarm_get_irq_num(timer_hw, ALARM_NUM)

// Alarm interrupt handler
static volatile bool alarm_fired;

static void alarm_irq(void) {
    // try to hold counter in interrupt handler
    // static int count = 0;
    count = count + 1; // increment global count
    elapsed_time = count*delay_sec; // calulate elapsed time

    if(db_p<data_buffer+BUFF_LEN){
        *db_p++ = elapsed_time; // save time to buffer and increment buffer pointer
    }

    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    // Assume alarm 0 has fired
    // printf("Alarm IRQ fired: %d times\n", count);
    alarm_fired = true;
}

static void alarm_in_us(uint32_t delay_us) {
    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);
    // Enable interrupt in block and at processor

    // Alarm is only 32 bits so if trying to delay more
    // than that need to be careful and keep track of the upper
    // bits
    uint64_t target = timer_hw->timerawl + delay_us;

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
    // printf("Enabled Timer\n");
}

void save_data(void){
    char key_rec = 'b';
    db_p = data_buffer;
    while(key_rec!='z'){
        key_rec = getchar();
        // printf("")
        if (key_rec=='a'){
            printf("%f",*db_p);
            db_p++;
            key_rec = 'b';
        }
    }
    // blink if pin high
    gpio_put(PICO_DEFAULT_LED_PIN,1);
    sleep_ms(1000);
    gpio_put(PICO_DEFAULT_LED_PIN,0);
}

int main() {
    stdio_init_all();
    // init blinking
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN,GPIO_OUT);
    printf("Timer lowlevel!\n");
    *db_p = 0.0;
    db_p++;
    // Set alarm every 1/2 seconds
    while (elapsed_time<2) {
        alarm_fired = false;
        alarm_in_us(1000000*delay_sec); // 1/2 ms delay time
        // printf("Timer Blocking Test.");
        // count = count + 1;
        // printf("Elapsed Time: %f\n",elapsed_time);
        // Wait for alarm to fire
        while (!alarm_fired){
            // perform other task
            // sleep_ms(500);
            // printf("Timer Blocking Test.");
        }
    }
    // Blink to indicate data transfer complete
    gpio_put(PICO_DEFAULT_LED_PIN,1);
    sleep_ms(500);
    gpio_put(PICO_DEFAULT_LED_PIN,0);

    // Transfer data to computer3
    save_data();

}

/// \end::alarm_standalone[]
