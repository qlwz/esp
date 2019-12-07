
#include "Led.h"
#include "Mqtt.h"
#include "RadioReceive.h"
#include "Config.h"
#include <Ticker.h>
#include <ESP8266WiFi.h>

Ticker *Led::ledTicker;
Ticker *Led::ledTicker2;
uint8_t Led::io = 99;
uint8_t Led::light;
uint8_t Led::ledType = 0;

void Led::init(uint8_t _io, uint8_t _light)
{
    io = _io;
    light = _light;
    pinMode(io, OUTPUT);

    Led::ledType = 0;
    ledTicker = new Ticker();
    ledTicker2 = new Ticker();
    digitalWrite(io, !light);
    ledTicker->attach(0.2, []() { digitalWrite(io, !digitalRead(io)); });
}

void Led::loop()
{
    if (io == 99)
    {
        return;
    }
    if (module && module->moduleLed())
    {
        digitalWrite(io, light);
        Led::ledType = 3;
    }
    else if (WiFi.status() != WL_CONNECTED)
    {
        if (Led::ledType != 0)
        {
            Led::ledType = 0;
            ledTicker->attach(0.2, []() { digitalWrite(io, !digitalRead(io)); });
        }
    }
    else if (!mqtt->mqttClient.connected())
    {
        if (Led::ledType != 1)
        {
            Led::ledType = 1;
            ledTicker->attach(0.3, []() { digitalWrite(io, !digitalRead(io)); });
        }
    }
    else
    {
        if (Led::ledType != 2)
        {
            Led::ledType = 2;
            ledTicker->attach(5, led, 200);
        }
    }
}

void Led::led(int ms)
{
    if (io == 99)
    {
        return;
    }
    digitalWrite(io, light);
    ledTicker2->once_ms(ms, []() { digitalWrite(io, !light); });
}

void Led::blinkLED(int duration, int n)
{
    if (io == 99)
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        digitalWrite(io, light);
        delay(duration);
        digitalWrite(io, !light);
        if (n != i + 1)
        {
            delay(duration);
        }
    }
}
