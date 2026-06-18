#pragma once

#include <Arduino.h>
#include "MCP23018.h"
#include "config.h"

void recoverI2CBus(uint8_t sdaPin, uint8_t sclPin);

class IO_Expander
{
private:
    const uint8_t num_io_exp;
    std::vector<MCP23018*> io_exp;

public:
    IO_Expander(const uint8_t num_io_exp, const uint8_t* addresses, const uint8_t* irq_pins);
    ~IO_Expander();
    IO_Expander(const IO_Expander&) = delete;
    IO_Expander& operator=(const IO_Expander&) = delete;

    bool init();
    bool read(volatile bool* isPressed, const uint8_t numFields);
    bool resetAndReinit();
};