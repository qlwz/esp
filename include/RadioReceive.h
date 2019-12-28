// RadioReceive.h
#ifdef USE_RELAY

#ifndef _RADIORECEIVE_h
#define _RADIORECEIVE_h

#include "Arduino.h"
#include <RCSwitch.h>
#include "Relay.h"

class RadioReceive
{
protected:
    RCSwitch *mySwitch;
    unsigned long lastVaue = 0;
    int lastTime = 0;
    int studyTime = 0;
    Relay *relay;

public:
    uint8_t studyCH = 0;
    void init(Relay *_relay, uint8_t io);
    void study(uint8_t ch);
    void del(uint8_t ch);
    void delAll();
    void loop();
};

#endif

#endif