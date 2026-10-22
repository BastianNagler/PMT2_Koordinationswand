#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @file config.h
 * @brief Global hardware and software configuration constants for cWall.
 */

// ============================================================================
// Software & Grid Dimensions
// ============================================================================

/** Number of rows in the physical game grid (maximum supported is 4). */
inline constexpr uint8_t NUM_ROWS = 4;

/** Number of columns in the physical game grid. */
inline constexpr uint8_t NUM_COLUMNS = 8;

/** Total number of sensor/LED boxes on the wall. */
inline constexpr uint8_t NUM_FIELDS = NUM_ROWS * NUM_COLUMNS;

/** Maximum LED brightness level (0-255). */
inline constexpr uint8_t MAX_BRIGHTNESS = 255;

/** Number of WS2812B LEDs inside each individual sensor box. */
inline constexpr uint8_t NUM_LEDS_PER_FIELD = 16;

/** Total number of LEDs across all boxes. */
inline constexpr uint16_t NUM_LEDS = NUM_FIELDS * NUM_LEDS_PER_FIELD;

/** Number of MCP23018 I/O expanders mounted on the main controller PCB. */
inline constexpr uint8_t NUM_IO_EXPANDER = 2;

/** Number of digital input channels handled by each MCP23018 expander. */
inline constexpr uint8_t NUM_IO_PER_EXPANDER = 16;

// ============================================================================
// Hardware Pinout Configuration (ESP32-S3)
// ============================================================================

/** GPIO pin driving LED data line for row 0. */
inline constexpr uint8_t LED_DRIVER_0_PIN = 42;

/** GPIO pin driving LED data line for row 1. */
inline constexpr uint8_t LED_DRIVER_1_PIN = 41;

/** GPIO pin driving LED data line for row 2. */
inline constexpr uint8_t LED_DRIVER_2_PIN = 40;

/** GPIO pin driving LED data line for row 3. */
inline constexpr uint8_t LED_DRIVER_3_PIN = 39;

/** GPIO pin for the level-shifter output enable line. */
inline constexpr uint8_t LED_DATA_EN_PIN = 2;

/** GPIO pin connected to the onboard WS2812 RGB heartbeat / status LED. */
inline constexpr uint8_t HEARTBEAT_LED_PIN = 38;

/** GPIO pin for I2C SCL clock line. */
inline constexpr uint8_t I2C_SCL_PIN = 9;

/** GPIO pin for I2C SDA data line. */
inline constexpr uint8_t I2C_SDA_PIN = 8;

/** I2C bus clock speed in Hertz. */
inline constexpr uint32_t I2C_FREQ_HZ = 400000;

/** Hardware reset GPIO pin for both MCP23018 expanders. */
inline constexpr uint8_t IO_EXPANDER_RESET_PIN = 12;

/** Hardware errata bypass pin for sensor box 16. */
inline constexpr uint8_t SENSOR_16_OVERRIDE_PIN = 18;

/** I2C bus addresses for each MCP23018 expander. */
inline constexpr uint8_t IO_EXPANDER_ADDRESSES[NUM_IO_EXPANDER] = {0x20, 0x21};

/** Interrupt GPIO input pins connected to the open-drain interrupt lines of each MCP23018. */
inline constexpr uint8_t IO_EXPANDER_IRQ_PINS[NUM_IO_EXPANDER] = {10, 14};
