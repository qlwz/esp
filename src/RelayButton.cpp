#ifdef USE_RELAY

#include "Debug.h"
#include "RadioReceive.h"
#include "RelayButton.h"
#include "Config.h"
#include "Relay.h"
#include "Wifi.h"
#include "Led.h"

void RelayButton::init(Relay *_relay, uint8_t _ch, uint8_t _io)
{
    ch = _ch;
    io = _io;
    relay = _relay;
    pinMode(io, INPUT_PULLUP);
    previousState = digitalRead(io);
    //delay(debounceTime);
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("load button ch%d io%d"), ch + 1, io);
}

void RelayButton::loop()
{
    now = millis(); // 计时器计时
    state = digitalRead(io);
    if (state != previousState)
    {
        if (timing == false)
        {
            timing = true;
            timingStart = now;
        }
        else if (now >= (timingStart + debounceTime))
        {
            timing = false;
            if (now > lastTime + 300)
            {
                relay->switchRelay(ch, !relay->lastState[ch], true);
                lastTime = now;
            }
            // 计算结果并报告
            previousState = state;
            intervalStart = now;
            switchCount += 1;
        }
    }

    // 如果经过的时间大于超时并且计数大于0，则填充并重置计数
    if (switchCount > 0 && (now - intervalStart) > specialFunctionTimeout)
    {
        Led::led(200);
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("switchCount %d : %d"), ch + 1, switchCount);

        if (switchCount == 10 && relay->radioReceive)
        {
            relay->radioReceive->study(ch);
        }
        else if (switchCount == 12 && relay->radioReceive)
        {
            relay->radioReceive->del(ch);
        }
        else if (switchCount == 16 && relay->radioReceive)
        {
            relay->radioReceive->delAll();
        }
        else if (switchCount == 20)
        {
            Wifi::setupWifiManager(false);
        }
        switchCount = 0;
    }
}

#endif