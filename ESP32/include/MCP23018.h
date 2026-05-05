#pragma once

#include <Arduino.h>
#include <Wire.h>

#define MAX_CONNECTION_ATTEMPTS 10

static void IRAM_ATTR irqHandler(void* arg);

class MCP23018
{
private:
    const uint8_t i2cAddress;
    const uint8_t interruptPin;
    

public:
    volatile bool needsRead;

    MCP23018(const uint8_t i2cAddress, const uint8_t interruptPin);

    bool init();
    bool read(uint16_t& data);
};