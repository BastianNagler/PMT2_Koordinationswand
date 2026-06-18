#include "ioExpander.h"
#include "WebLog.h"

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

bool IO_Expander::resetAndReinit()
{
    WebLog.println("Resetting MCP23018s via GPIO 12...");
    
    // Set RESET pin as output and pull it LOW to reset both expanders
    pinMode(IO_EXPANDER_RESET_PIN, OUTPUT);
    digitalWrite(IO_EXPANDER_RESET_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(10)); // Keep LOW for 10ms
    
    // Release RESET by pulling it HIGH
    digitalWrite(IO_EXPANDER_RESET_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait 10ms for expanders to boot up
    
    // Recover the I2C bus if stuck
    recoverI2CBus(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.end();
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
    Wire.setTimeOut(100);
    
    // Reconfigure both expanders
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

void recoverI2CBus(uint8_t sdaPin, uint8_t sclPin) {
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, OUTPUT);
    digitalWrite(sclPin, HIGH);
    delay(1);

    if (digitalRead(sdaPin) == LOW) {
        WebLog.println("[I2C] SDA is stuck LOW! Attempting bus recovery...");
        for (int i = 0; i < 9; i++) {
            digitalWrite(sclPin, LOW);
            delayMicroseconds(5);
            digitalWrite(sclPin, HIGH);
            delayMicroseconds(5);
            if (digitalRead(sdaPin) == HIGH) {
                WebLog.printf("[I2C] SDA released after %d clock pulses.\n", i + 1);
                break;
            }
        }
    }

    // Generate a STOP condition to reset the bus state
    pinMode(sdaPin, OUTPUT);
    digitalWrite(sdaPin, LOW);
    delayMicroseconds(5);
    digitalWrite(sclPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sdaPin, HIGH);
    delayMicroseconds(5);

    // Set pins back to high-impedance
    pinMode(sdaPin, INPUT);
    pinMode(sclPin, INPUT);
}


