#ifndef _CSE7766_h
#define _CSE7766_h

#include "Arduino.h"
#include <SoftwareSerial.h>

class CSE7766
{
protected:
    bool checkByte();
    void parseData();
    uint32_t get24BitUint(uint8_t startIndex);
    SoftwareSerial *serial;

    uint8_t rawData[24];
    uint8_t rawDataIndex = 0;
    uint32_t lastTransmission = 0;
    float voltageAcc = 0.0f;
    float currentAcc = 0.0f;
    float powerAcc = 0.0f;
    uint32_t voltageCounts = 0;
    uint32_t currentCounts = 0;
    uint32_t powerCounts = 0;

public:
    CSE7766(uint8_t rxPin, uint16_t baudrate = 4800);

    float voltage = 0.0f;
    float current = 0.0f;
    float power = 0.0f;
    float factor = 0.0f;

    void loop();
    void update();
};
#endif