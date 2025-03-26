/* This is a program for implementing a PID motor control 
    protocal using PWM voltage control and magnetic encoder feedback. */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <math.h>
// I^2C communication
#include "hardware/i2c.h"

// PWM Library
#include "hardware/pwm.h"

// PI constant
#define PI 3.1415926

// PID LOOP Constants
#define K_P 4.25
#define K_I 1.0
#define K_D 0.001

// I^2C Definitions
#define I2C_PORT            i2c0
#define AS5600_ADDR         0x36         // AS5600 I2C address
#define AS5600_RAW_ANGLE_REG 0x0C         // Register for raw angle reading
#define SDA_PIN 4
#define SCL_PIN 5

// PWM Pin Definitions 
#define PWM_PIN 13 // GPIO Pin #13
#define IN_1 14
#define IN_2 15

// LED PWM to match motor
#define MY_LED 25

// PWM Frequency Definitions
#define PWM_WRAP 256 // number of PWM cycles until reset
// PWM Freq = 133,000,000 Hz/1,000 = 133,000 Hz
// PWM Cycle period ~ 0.0075 milliseconds
// --> about 665 PWM cycles per sampling period T = 5 ms

#define RUN_TIME 15.05

// define reference angle (degrees)
static double ref_angle = 20;

// define delay in seconds
const double delay_sec =  0.01;

// global elapsed time
static double elapsed_time = 0.0;

// define global time and angle buffer
#define BUFF_LEN 3000
static double time_buffer[BUFF_LEN];
static double *time_p = time_buffer;
static double angle_buffer[BUFF_LEN];
static double *angle_p = angle_buffer;
static double velocity_buffer[BUFF_LEN];
static double *velocity_p = velocity_buffer;
static double signal_buffer[BUFF_LEN];
static double *signal_p = signal_buffer;

// define global count
static int count = 0;

// angle read function
// define global angle variables
static uint16_t raw_angle = 0;
static double current_angle = 0.0;
static double total_movement = 0.0;
static double last_angle = 0.0;
static double angle_delta = 0.0;
// angular velocity global variables
static double curr_velocity = 0.0;
static double last_velocity = 0.0;
static double vel_input = 0.0;

// PID controller variables
static double last_signal = 0.0;
static double last_error = 0.0;
static double last_error_2 = 0.0;

static double error;
static double signal;

// pwm slice and channel variables
static uint pwm_slice;
static uint pwm_channel;
static uint led_slice;
static uint led_channel;

// const static double alpha = delay_sec/(((delay_sec/PI)*0.0001)+delay_sec);
const static double alpha = 0.90;

// Function to read the 12-bit angle from the AS5600
uint16_t read_as5600_angle() {
    uint8_t reg = AS5600_RAW_ANGLE_REG;
    uint8_t buffer[2];

    // Write the register address to the sensor.
    // The 'true' parameter sends a repeated start instead of a stop.
    int ret = i2c_write_blocking(I2C_PORT, AS5600_ADDR, &reg, 1, true);
    if (ret < 0) {
        printf("I2C write error\n");
        return 0;
    }

    // Read 2 bytes from the sensor.
    ret = i2c_read_blocking(I2C_PORT, AS5600_ADDR, buffer, 2, false);
    if (ret < 0) {
        printf("I2C read error\n");
        return 0;
    }

    // Combine the two bytes into a 16-bit value.
    uint16_t angle = ((uint16_t)buffer[0] << 8) | buffer[1];
    // The AS5600 provides a 12-bit value (0-4095), so mask the upper bits.
    angle &= 0x0FFF;
    return angle;
}

void first_order_filt(double vel_input){
    curr_velocity = (alpha*vel_input) + (1-alpha)*last_velocity;
    last_velocity = curr_velocity;
}

void calculate_angular_velocity(void){
    vel_input = angle_delta/delay_sec;
    first_order_filt(vel_input);
    if (velocity_p<velocity_buffer+BUFF_LEN){
        *velocity_p++ = curr_velocity;
    }
    // printf("Speed in RPM: %f\n",angular_velocity*(60.0/360.0));
}

// Function to calculate angle movement, accounting for rollover
void calculate_angle_movement(float current_angle) {
    angle_delta = current_angle - last_angle;
    // Handle rollover cases
    if (angle_delta > 180.0) {
        angle_delta -= 360.0;  // Passed 0° clockwise
    } else if (angle_delta < -180.0) {
        angle_delta += 360.0;  // Passed 360° counterclockwise
    }
    calculate_angular_velocity();
    total_movement += angle_delta;
    last_angle = current_angle;

    // save total angular displacement to buffer
    if(angle_p<angle_buffer+BUFF_LEN){
        *angle_p++ = total_movement;
    }

}

void cap_signal(double uncap_signal){
    double cap_signal;
    if (uncap_signal > PWM_WRAP){
        signal = PWM_WRAP;
    }
    else if (uncap_signal<(PWM_WRAP*(-1))){
        signal = PWM_WRAP*(-1);
    }
    else {
        signal = uncap_signal;
    }
}

void calculate_output_pid(void){
    signal = 0;
    error = ref_angle - total_movement;
    signal += last_signal;
    signal += K_P*(error) -K_P*last_error;
    signal += K_I*delay_sec*(error/2) +K_I*delay_sec*(last_error/2);
    // signal -= K_D*curr_velocity;
    cap_signal(signal);
    if (signal_p<signal_buffer+BUFF_LEN){
        *signal_p++ = signal;
    }
    last_signal = signal;
    last_error_2 = last_error;
    last_error = error;
}

void pin_init(void){
    // Initialize direction pins
    gpio_init(IN_1);
    gpio_set_dir(IN_1, GPIO_OUT);
    gpio_init(IN_2);
    gpio_set_dir(IN_2, GPIO_OUT);

    // Initialize I2C at 100 kHz.
    i2c_init(I2C_PORT, 100 * 1000);
    // Set up GPIO pins for I2C functionality.
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    // Enable pull-ups on I2C lines.
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Set PWM Pin for motor control
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(PWM_PIN);
    pwm_channel = pwm_gpio_to_channel(PWM_PIN);
    pwm_set_wrap(pwm_slice, PWM_WRAP);
    pwm_set_enabled(pwm_slice, true);
    pwm_set_chan_level(pwm_slice,pwm_channel, 0);

    // Set LED Pin to follow motor control
    gpio_set_function(MY_LED, GPIO_FUNC_PWM);
    led_slice = pwm_gpio_to_slice_num(MY_LED);
    led_channel = pwm_gpio_to_channel(MY_LED);
    pwm_set_wrap(led_slice, MY_LED);
    pwm_set_enabled(led_slice, true);
    pwm_set_chan_level(led_slice,led_channel, 0);

}

// ------ Blink Delay Function -------
void blink_delay_ms(int blink_ms){
    pwm_set_chan_level(led_slice,led_channel, PWM_WRAP);
    sleep_ms(blink_ms);
    pwm_set_chan_level(led_slice,led_channel, 0);
}

// motor direction function
void set_motor_dir(bool direction){
    if (direction==true){
        gpio_put(IN_1,1);
        gpio_put(IN_2,0);
    }
    else{
        gpio_put(IN_1,0);
        gpio_put(IN_2,1);
    }
}

// Saves Data to buffer
void save_data(void){
    char key_rec = 'b';
    time_p = time_buffer;
    angle_p = angle_buffer;
    velocity_p = velocity_buffer;
    signal_p = signal_buffer;
    while(key_rec!='z'){
        key_rec = getchar();
        // if t --> send time
        if (key_rec=='t'){
            printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            *time_p,*(time_p+1),*(time_p+2),*(time_p+3),*(time_p+4),*(time_p+5),*(time_p+6),
            *(time_p+7),*(time_p+8),*(time_p+9));
            time_p = time_p+10;
            key_rec = 'b';
        }
        // if a --> send angle
        else if (key_rec=='a'){
            printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            *angle_p,*(angle_p+1),*(angle_p+2),*(angle_p+3),*(angle_p+4),*(angle_p+5),*(angle_p+6),
            *(angle_p+7),*(angle_p+8),*(angle_p+9));
            angle_p = angle_p+10;
            key_rec = 'b';
        }
        else if (key_rec=='v'){
            printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            *velocity_p,*(velocity_p+1),*(velocity_p+2),*(velocity_p+3),*(velocity_p+4),
            *(velocity_p+5),*(velocity_p+6),*(velocity_p+7),*(velocity_p+8),*(velocity_p+9));
            velocity_p=velocity_p+10;
            key_rec = 'b';
        }
        else if (key_rec=='s'){
            printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            *signal_p,*(signal_p+1),*(signal_p+2),*(signal_p+3),*(signal_p+4),
            *(signal_p+5),*(signal_p+6),*(signal_p+7),*(signal_p+8),*(signal_p+9));
            signal_p=signal_p+10;
            key_rec = 'b';
        }
    }
    // Blink to indicate data collection finished
    blink_delay_ms(1000);
}

static uint64_t get_time(void) {
    // Reading low latches the high value
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return ((uint64_t) hi << 32u) | lo;
}

// Timer Interrupt Request Function Definitions

static int abs_signal;

void actuate_signal(void){
    if(signal>0){
        set_motor_dir(false);
        abs_signal = (int)signal;
    }
    else if (signal<0){
        set_motor_dir(true);
        abs_signal = (int)signal*(-1);
    }
    pwm_set_chan_level(pwm_slice,pwm_channel,abs_signal);
    pwm_set_chan_level(led_slice,led_channel,abs_signal); 
}  

#define ALARM_NUM 0
#define ALARM_IRQ timer_hardware_alarm_get_irq_num(timer_hw, ALARM_NUM)

// Alarm interrupt handler
static volatile bool alarm_fired;

static void alarm_irq(void) {
    // try to hold counter in interrupt handler
    // static int count = 0;
    count = count + 1; // increment global count
    elapsed_time = count*delay_sec; // calulate elapsed time

    // find angular displacement
    raw_angle = read_as5600_angle();
    current_angle = (raw_angle*360.0f)/4096.0f; 
    calculate_angle_movement(current_angle);
    calculate_output_pid();
    actuate_signal();
    if(time_p<time_buffer+BUFF_LEN){
        *time_p++ = elapsed_time; // save time to buffer and increment buffer pointer
    }

    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    // Assume alarm 0 has fired
    // printf("Alarm IRQ fired: %d times\n", count);
    alarm_fired = true;
}

// Timer enabling function

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

int main() {
    stdio_init_all();
    printf("Timer lowlevel!\n");
    pin_init();
    // Delay data collection by 5 seconds
    blink_delay_ms(1000);
    sleep_ms(1000);

    // collect first angles
    last_angle = read_as5600_angle();
    last_angle = (last_angle*360.0f)/4096.0f;
    raw_angle = read_as5600_angle();
    current_angle = (raw_angle*360.0f)/4096.0f; 
    calculate_angle_movement(current_angle);
    *time_p = 0.0;
    time_p++;
    
    // Set alarm every 0.005 seconds
    while (elapsed_time<RUN_TIME) {
        alarm_fired = false;
        alarm_in_us(1000000*delay_sec); // 5 ms delay time

        while (!alarm_fired){
            // perform other task --> Timer is non-blocking
        }
    }
    // Turn off motor
    pwm_set_chan_level(pwm_slice,pwm_channel,0);
    pwm_set_chan_level(led_slice,led_channel,0);

    sleep_ms(1000); 
    // Blink to indicate data collection complete
    blink_delay_ms(500);

    // Transfer data to computer
    save_data();

    return 0;
}
