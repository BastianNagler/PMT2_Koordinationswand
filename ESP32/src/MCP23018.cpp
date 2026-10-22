#include "MCP23018.h"
#include "WebLog.h"

static const char* getI2CErrorStr(const uint8_t err) {
    switch (err) {
        case 0: return "Success";
        case 1: return "Data too long";
        case 2: return "NACK on address (Device not found/no response)";
        case 3: return "NACK on data";
        case 4: return "Other error";
        case 5: return "Timeout";
        default: return "Unknown error";
    }
}

static void IRAM_ATTR irqHandler(void* const arg) {
    MCP23018* const mcp = static_cast<MCP23018*>(arg);
    if (mcp != nullptr) {
        mcp->needsRead = true;
    }
}

MCP23018::MCP23018() : i2cAddress(0), interruptPin(0), needsRead(false) {}

MCP23018::MCP23018(const uint8_t i2cAddress, const uint8_t interruptPin)
    : i2cAddress(i2cAddress), interruptPin(interruptPin), needsRead(false) {}

bool MCP23018::init() {
    Wire.beginTransmission(i2cAddress);
    const uint8_t pingErr = Wire.endTransmission();
    if (pingErr != 0) {
        WebLog.printf("[ERROR] MCP23018 (0x%02X) not found: %s (%d)\n",
                       i2cAddress, getI2CErrorStr(pingErr), pingErr);
        return false;
    }

    WebLog.printf("[IO-EXPANDER] MCP23018 (0x%02X) connected\n", i2cAddress);

    Wire.beginTransmission(i2cAddress);
    Wire.write(MCP23018Registers::IODIRA);
    // IODIR: configure all 16 pins as inputs
    Wire.write(0xFF);
    Wire.write(0xFF);
    // IPOL: keep input polarity non-inverted
    Wire.write(0x00);
    Wire.write(0x00);
    // GPINTEN: enable interrupt-on-change for all inputs
    Wire.write(0xFF);
    Wire.write(0xFF);
    // DEFVAL: compare against previous pin state
    Wire.write(0x00);
    Wire.write(0x00);
    // INTCON: compare with previous value (0x00)
    Wire.write(0x00);
    Wire.write(0x00);
    // IOCON: mirror INTA/INTB pins, active-high open-drain output (0b01000010)
    Wire.write(0b01000010);
    Wire.write(0b01000010);
    // GPPU: disable internal pull-ups (hardware has external pull-downs)
    Wire.write(0x00);
    Wire.write(0x00);

    const uint8_t configErr = Wire.endTransmission();
    if (configErr != 0) {
        WebLog.printf("[ERROR] MCP23018 (0x%02X) configuration failed: %s (%d)\n",
                       i2cAddress, getI2CErrorStr(configErr), configErr);
        return false;
    }

    uint16_t dummy = 0;
    if (!read(dummy)) {
        WebLog.printf("[ERROR] MCP23018 (0x%02X) initial read failed\n", i2cAddress);
        return false;
    }
    asm volatile("" : : "g"(dummy) : "memory");

    pinMode(interruptPin, INPUT);
    attachInterruptArg(digitalPinToInterrupt(interruptPin), irqHandler, this, RISING);

    needsRead = true;
    return true;
}

bool MCP23018::read(uint16_t& data) {
    needsRead = false;

    Wire.beginTransmission(i2cAddress);
    Wire.write(MCP23018Registers::GPIOA);
    const uint8_t writeErr = Wire.endTransmission();
    if (writeErr != 0) {
        WebLog.printf("[ERROR] MCP23018 (0x%02X) register select failed: %s (%d)\n",
                       i2cAddress, getI2CErrorStr(writeErr), writeErr);
        return false;
    }

    const uint8_t bytesReceived = Wire.requestFrom(i2cAddress, static_cast<uint8_t>(2));
    if (bytesReceived == 2) {
        const uint8_t portA = Wire.read();
        const uint8_t portB = Wire.read();
        data = static_cast<uint16_t>((static_cast<uint16_t>(portB) << 8) | portA);
        return true;
    }

    WebLog.printf("[ERROR] MCP23018 (0x%02X) read failed: expected 2 bytes, got %d\n",
                   i2cAddress, bytesReceived);
    return false;
}
