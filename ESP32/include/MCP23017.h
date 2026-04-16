#pragma once

#include <Arduino.h>
#include <Wire.h>

/*
    MCP23017-Config:
        Normal Polarity
        INTA and INTB coupled
        Interrupt active high
        Interrupt push-pull config
        Interrupt when PIN isn't like previous value

*/



static void IRAM_ATTR irqHandler(void* arg);

class MCP23017
{
private:
    const uint8_t i2cAddress;
    const uint8_t interruptPin;

public:
    volatile bool needsRead;

    MCP23017(uint8_t i2cAddress, uint8_t interruptPin);

    void init();
    uint16_t read();

protected:
};