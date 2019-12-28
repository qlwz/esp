// RelayButton.h
#ifdef USE_RELAY

#ifndef _RELAYBUTTON_h
#define _RELAYBUTTON_h

#include "Arduino.h"

class RelayButton
{
protected:
    uint8_t ch = -1;
    uint8_t io = -1;
    // 按键
    int debounceTime = 50;
    boolean timing = false;
    unsigned long timingStart = 0;
    unsigned long intervalStart, now;
    boolean state, previousState = 0;
    int switchCount = 0;

    // 等待开关再次切换的时间（以毫秒为单位）。
    // 300对我来说效果很好，几乎没有引起注意。 如果您不想使用此功能，请设置为0。
    unsigned long specialFunctionTimeout = 300;
    int lastTime = 0;

    Relay *relay;

public:
    void init(Relay *_relay, uint8_t _ch, uint8_t _io);
    void loop();
};

#endif

#endif