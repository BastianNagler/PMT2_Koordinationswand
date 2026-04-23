#include "MCP23017.h"

static void irqHandler(void* arg)
{
    MCP23017* mcp = static_cast<MCP23017*>(arg);

    mcp->needsRead = true;
}


MCP23017::MCP23017(const uint8_t i2cAddress, const uint8_t interruptPin): 
    i2cAddress(i2cAddress), interruptPin(interruptPin)
{}

void MCP23017::init()
{
    // check if MCP23017 is connected
    Wire.beginTransmission(i2cAddress);
    if (Wire.endTransmission())
    {
        Serial.printf("MCP23017 (I2C-address 0x%02x) not found \n", i2cAddress);
        while (1);
    }
    Serial.printf("MCP23017 (I2C-address 0x%02x) successfully connected", i2cAddress);

    // configure MCP23017 internal registers
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x00); //starting Registry Address
    // IODIR: Set PINs to Input
    Wire.write(0xFF);
    Wire.write(0xFF);
    // IPOL: Set Polarity to Normal
    Wire.write(0x00);
    Wire.write(0x00);
    // GPINTEN: Set Interrupt-On-Change
    Wire.write(0xFF);
    Wire.write(0xFF);
    // DEFVAL: Set Interrupt when PIN isn't 0
    Wire.write(0x00);
    Wire.write(0x00);
    // INTCON: Set Interrupt to compare against previous value
    Wire.write(0x00);
    Wire.write(0x00);
    // IOCON: Set Interrupt-Pin-Mirroring and Interrupt Active-High
    Wire.write(0b01000010);
    Wire.write(0b01000010);
    // GPPU: Set Pull-Ups to none
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();

    pinMode(interruptPin, INPUT);
    attachInterruptArg(digitalPinToInterrupt(interruptPin), irqHandler, this, RISING);

    needsRead = true; // force initial read
}

uint16_t MCP23017::read()
{
    this->needsRead = false;

    Wire.beginTransmission(i2cAddress);
    Wire.write(0x12);
    Wire.endTransmission();

    Wire.requestFrom(i2cAddress, (uint8_t)2);

    if (Wire.available() == 2) {
        uint8_t portA = Wire.read();
        uint8_t portB = Wire.read();
        
        return (portB << 8) | portA; 
    }

    return 0;
}