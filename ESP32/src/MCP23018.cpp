#include "MCP23018.h"

static void irqHandler(void* arg)
{
    MCP23018* mcp = static_cast<MCP23018*>(arg);

    mcp->needsRead = true;
}


MCP23018::MCP23018(const uint8_t i2cAddress, const uint8_t interruptPin): 
    i2cAddress(i2cAddress), interruptPin(interruptPin)
{}

bool MCP23018::init()
{
    uint8_t attempts = 0;
    // check if MCP23018 is connected
    Wire.beginTransmission(i2cAddress);
    while(Wire.endTransmission() && attempts < MAX_CONNECTION_ATTEMPTS)
    {
        Serial.printf("MCP23018 (I2C-address 0x%02x) not found. Retrying... \n", i2cAddress);
        delay(500);
        Wire.beginTransmission(i2cAddress);
        attempts++;
    }

    if(attempts >= MAX_CONNECTION_ATTEMPTS)
    {
        Serial.printf("MCP23018 (I2C-address 0x%02x) not found. Please fix issue");
        return false;
    }
    else
    {
        Serial.printf("MCP23018 (I2C-address 0x%02x) successfully connected \n", i2cAddress);
    }

    // configure MCP23018 internal registers
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
    return true;
}

bool MCP23018::read(uint16_t& data)
{
    this->needsRead = false;

    Wire.beginTransmission(i2cAddress);
    Wire.write(0x12);
    Wire.endTransmission();

    Wire.requestFrom(i2cAddress, (uint8_t)2);

    if (Wire.available() == 2) {
        uint8_t portA = Wire.read();
        uint8_t portB = Wire.read();
        
        data = (portB << 8) | portA;
        return true;
    } else {
        Serial.printf("I2C read error for MCP23018 (0x%02x): expected 2 bytes, got %d\n", i2cAddress, Wire.available());
        return false;
    }
}