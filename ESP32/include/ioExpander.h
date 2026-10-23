#pragma once

#include <Arduino.h>
#include <array>
#include <cstdint>
#include "MCP23018.h"
#include "SensorState.h"
#include "config.h"

/**
 * Attempts to clear an unresponsive or locked I2C bus by clocking SCL line
 * 9 cycles and pulsing a clean STOP condition.
 *
 * @param[in] sdaPin GPIO pin number of the I2C SDA line.
 * @param[in] sclPin GPIO pin number of the I2C SCL line.
 */
void recoverI2CBus(const uint8_t sdaPin, const uint8_t sclPin);

/**
 * High-level manager coordinating multiple MCP23018 expanders over I2C.
 */
class IO_Expander {
private:
    const uint8_t num_io_exp;
    std::array<MCP23018, NUM_IO_EXPANDER> io_exp;

public:
    /**
     * Constructs the IO_Expander instance.
     *
     * @param[in] num_io_exp Number of connected expanders.
     * @param[in] addresses Array of 7-bit I2C addresses for each expander.
     * @param[in] irq_pins Array of GPIO pins connected to the interrupt lines.
     */
    IO_Expander(const uint8_t num_io_exp, const uint8_t* const addresses, const uint8_t* const irq_pins);

    ~IO_Expander() = default;
    IO_Expander(const IO_Expander&) = delete;
    IO_Expander& operator=(const IO_Expander&) = delete;

    /**
     * Initializes all configured MCP23018 I/O expanders.
     *
     * @return `true` if all expanders initialized successfully, `false` otherwise.
     */
    bool init();

    /**
     * Reads all pending inputs from expanders that signaled an interrupt into thread-safe SensorState.
     *
     * @param[out] sensorState Target thread-safe sensor state container.
     * @param[in] numFields Number of valid fields to populate.
     *
     * @return `true` if all pending reads succeeded, `false` on bus error.
     */
    bool read(SensorState& sensorState, const uint8_t numFields);

    /**
     * Pulses hardware reset pin and re-initializes all MCP23018 expanders.
     *
     * @return `true` if all expanders re-initialized successfully, `false` otherwise.
     */
    bool resetAndReinit();
};
