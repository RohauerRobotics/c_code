#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT            i2c0
#define AS5600_ADDR         0x36         // AS5600 I2C address
#define AS5600_RAW_ANGLE_REG 0x0C         // Register for raw angle reading

#define SDA_PIN 4
#define SCL_PIN 5

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

int main() {
    // Initialize stdio (for USB serial output)
    stdio_init_all();

    // Initialize I2C at 100 kHz.
    i2c_init(I2C_PORT, 100 * 1000);
    // Set up GPIO pins for I2C functionality.
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    // Enable pull-ups on I2C lines.
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    printf("AS5600 Magnetic Encoder Demo\n");

    while (true) {
        uint16_t angle = read_as5600_angle();
        // The angle is a raw 12-bit value (0-4095). Optionally convert to degrees:
        float angle_deg = (angle * 360.0f) / 4096.0f;
        printf("Raw Angle: %4d, Angle: %6.2f°\n", angle, angle_deg);
        sleep_ms(100);
    }

    return 0;
}
