#include "ioExpander.h"
#include "WebLog.h"

IO_Expander::IO_Expander(const uint8_t num_io_exp, const uint8_t* const addresses, const uint8_t* const irq_pins)
    : num_io_exp(num_io_exp)
{
    for (uint8_t i = 0; i < num_io_exp; i++) {
        io_exp[i] = MCP23018(addresses[i], irq_pins[i]);
    }
}

bool IO_Expander::init() {
    bool allInitialized = true;
    for (uint8_t i = 0; i < num_io_exp; i++) {
        if (!io_exp[i].init()) {
            allInitialized = false;
        }
    }
    return allInitialized;
}

bool IO_Expander::read(SensorState& sensorState, const uint8_t numFields) {
    for (uint8_t i = 0; i < num_io_exp; i++) {
        if (io_exp[i].needsRead) {
            uint16_t state = 0;
            if (!io_exp[i].read(state)) {
                return false;
            }

            for (uint8_t j = 0; j < NUM_IO_PER_EXPANDER; j++) {
                const uint8_t fieldIndex = static_cast<uint8_t>(i * NUM_IO_PER_EXPANDER + j);
                if (fieldIndex < numFields) {
                    sensorState.setPressed(fieldIndex, (state & (1U << j)) != 0);
                }
            }
        }
    }
    return true;
}

bool IO_Expander::resetAndReinit() {
    pinMode(IO_EXPANDER_RESET_PIN, OUTPUT);
    digitalWrite(IO_EXPANDER_RESET_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    digitalWrite(IO_EXPANDER_RESET_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));

    recoverI2CBus(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
    Wire.setTimeOut(100);

    bool allInitialized = true;
    for (uint8_t i = 0; i < num_io_exp; i++) {
        if (!io_exp[i].init()) {
            allInitialized = false;
        }
    }
    return allInitialized;
}

void recoverI2CBus(const uint8_t sdaPin, const uint8_t sclPin) {
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, OUTPUT);
    digitalWrite(sclPin, HIGH);

    for (uint8_t i = 0; i < 9; i++) {
        digitalWrite(sclPin, LOW);
        delayMicroseconds(5);
        digitalWrite(sclPin, HIGH);
        delayMicroseconds(5);
    }

    pinMode(sdaPin, OUTPUT);
    digitalWrite(sdaPin, LOW);
    delayMicroseconds(5);
    digitalWrite(sclPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sdaPin, HIGH);
    delayMicroseconds(5);
}
