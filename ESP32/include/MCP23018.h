#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

/**
 * Maximum number of connection retry attempts when probing the I2C expander.
 */
inline constexpr uint8_t MAX_CONNECTION_ATTEMPTS = 10;

/**
 * Low-level register address and bitmask definitions for MCP23018.
 */
namespace MCP23018Registers {
    inline constexpr uint8_t IODIRA   = 0x00;
    inline constexpr uint8_t IPOLA    = 0x02;
    inline constexpr uint8_t GPINTENA = 0x04;
    inline constexpr uint8_t DEFVALA  = 0x06;
    inline constexpr uint8_t INTCONA  = 0x08;
    inline constexpr uint8_t IOCON    = 0x0A;
    inline constexpr uint8_t GPPUA    = 0x0C;
    inline constexpr uint8_t GPIOA    = 0x12;
}

/**
 * Hardware driver for the Microchip MCP23018 16-bit I2C I/O expander.
 */
class MCP23018 {
private:
    uint8_t i2cAddress = 0;
    uint8_t interruptPin = 0;

public:
    /** Flag set by the hardware ISR when a pin state change occurs. */
    volatile bool needsRead = false;

    /**
     * Default constructor required for static array storage.
     */
    MCP23018();

    /**
     * Constructs an MCP23018 instance with specific I2C address and interrupt line.
     *
     * @param[in] i2cAddress 7-bit I2C slave address of the expander.
     * @param[in] interruptPin GPIO pin connected to the expander interrupt line.
     */
    MCP23018(const uint8_t i2cAddress, const uint8_t interruptPin);

    /**
     * Initializes internal MCP23018 configuration registers and attaches the ISR.
     *
     * @return `true` if initialization succeeded and registers were acknowledged, `false` on bus error.
     */
    bool init();

    /**
     * Reads all 16 input pins (Port A and Port B) over I2C.
     *
     * @param[out] data Target variable for the combined 16-bit pin states.
     *
     * @return `true` if 2 bytes were read successfully, `false` on bus error or timeout.
     */
    bool read(uint16_t &data);

    /**
     * Retrieves the configured I2C address.
     *
     * @return The 7-bit I2C slave address.
     */
    uint8_t getI2CAddress() const {
        return i2cAddress;
    }

    /**
     * Retrieves the configured interrupt GPIO pin.
     *
     * @return The interrupt GPIO pin number.
     */
    uint8_t getInterruptPin() const {
        return interruptPin;
    }
};
