#include "ioExpander.h"

IO_Expander::IO_Expander(const uint8_t num_io_exp, const uint8_t* addresses, const uint8_t* irq_pins) : num_io_exp(num_io_exp)
{
    io_exp.reserve(num_io_exp);
    for (uint8_t i = 0; i < num_io_exp; i++)
    {
        io_exp.push_back(new MCP23018(addresses[i], irq_pins[i]));
    }
}

IO_Expander::~IO_Expander()
{
    for(uint8_t i = 0; i < num_io_exp; i++)
    {
        delete io_exp[i];
    }
}

bool IO_Expander::init()
{
    bool success = true;
    for (uint8_t i = 0; i < num_io_exp; i++)
    {
        if (!io_exp[i]->init())
        {
            success = false;
        }
    }
    return success;
}

bool IO_Expander::read(volatile bool* isPressed, const uint8_t numFields)
{
    for (uint8_t i = 0; i < num_io_exp; i++)
    {
        if (io_exp[i]->needsRead)
        {
            uint16_t state;
            if (!io_exp[i]->read(state))
            {
                return false;
            }
            for (uint8_t j = 0; j < NUM_IO_PER_EXPANDER; j++)
            {
                uint8_t fieldIndex = i * NUM_IO_PER_EXPANDER + j;
                if (fieldIndex < numFields)
                {
                    isPressed[fieldIndex] = state & (1U << j);
                }
            }
        }
    }
    return true;
}
