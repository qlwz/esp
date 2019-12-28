#ifdef USE_RELAY

#include "Debug.h"
#include "RadioReceive.h"
#include "RCSwitch.h"
#include "Relay.h"
#include "Config.h"
#include "Led.h"

void RadioReceive::init(Relay *_relay, uint8_t io)
{
    relay = _relay;
    mySwitch = new RCSwitch();
    pinMode(io, INPUT);
    mySwitch->enableReceive(digitalPinToInterrupt(io));
}

void RadioReceive::study(uint8_t ch)
{
    studyCH = 10 + ch;
    studyTime = millis();
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Receive study . . . "));
}

void RadioReceive::del(uint8_t ch)
{
    studyCH = 20 + ch;
    studyTime = millis();
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Receive del . . . "));
}

void RadioReceive::delAll()
{
    relay->config.study_index[0] = 0;
    relay->config.study_index[1] = 0;
    relay->config.study_index[2] = 0;
    relay->config.study_index[3] = 0;
    Config::saveConfig();
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Receive delAll . . . "));
}

void RadioReceive::loop()
{
    if (studyCH != 0 && millis() > studyTime + 10000) // 10秒超时
    {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Receive study timeout"));
        studyCH = 0;
    }

    if (!mySwitch->available())
    {
        return;
    }

    unsigned long value = mySwitch->getReceivedValue();
    mySwitch->resetAvailable();
    if (lastVaue == value && millis() < lastTime + 300)
    {
        return;
    }
    lastVaue = value;
    lastTime = millis();

    if (studyCH == 0)
    {
        bool isOk = false;

        /*
		for (size_t i = 0; i < 40; i++)
		{
			if (config.relay_study[i] != 0)
			{
				Debug.AddLog(LOG_LEVEL_INFO, PSTR("study %d %d"), i, config.relay_study[i]);
			}
		}
		*/

        for (size_t ch = 0; ch < relay->channels; ch++)
        {
            //Debug.AddLog(LOG_LEVEL_INFO, PSTR("study channel %d index %d"), ch, config.relay_study_index[ch]);
            for (size_t i = 0; i < relay->config.study_index[ch]; i++)
            {
                //Debug.AddLog(LOG_LEVEL_INFO, PSTR("study id %d"), (ch * 10) + i);
                if (relay->config.study[(ch * 10) + i] == value)
                {
                    isOk = true;
                    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Received %d to channel %d"), value, ch + 1);
                    relay->switchRelay(ch, !relay->lastState[ch], true);
                    break;
                }
            }
        }
        if (!isOk)
        {
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("Receive %d no channel"), value);
        }
        Led::led(200);
    }
    else if (studyCH >= 20) // 删除学习
    {
        uint8_t ch = studyCH - 20;

        uint8_t index = relay->config.study_index[ch];
        for (int i = 0; i <= index; ++i)
        {
            if (relay->config.study[(ch * 10) + i] == value)
            {
                for (int j = i; j <= index - 1; j++)
                {
                    relay->config.study[(ch * 10) + j] = relay->config.study[(ch * 10) + (j + 1)];
                }
                relay->config.study_index[ch] = --index;
                Config::saveConfig();
            }
        }

        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Received %d del to channel %d"), value, (ch) + 1);
        studyCH = 0;
        Led::blinkLED(200, 5);
    }
    else if (studyCH >= 10) // 学习
    {
        uint8_t ch = studyCH - 10;
        uint8_t index = relay->config.study_index[ch];
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("study index %d %d"), ch, index);
        for (uint8_t i = 0; i < index; i++)
        {
            if (relay->config.study[(ch * 10) + i] == value)
            {
                Debug.AddLog(LOG_LEVEL_INFO, PSTR("Received %d study to channel %d is has"), value, (ch) + 1);
                studyCH = 0;
                return;
            }
        }

        if (index >= MAX_STUDY_RECEIVER_NUM)
        {
            for (int j = 0; j < index - 1; j++)
            {
                relay->config.study[(ch * 10) + j] = relay->config.study[(ch * 10) + (j + 1)];
            }
            relay->config.study[(ch * 10) + (MAX_STUDY_RECEIVER_NUM - 1)] = value;
        }
        else
        {
            relay->config.study[(ch * 10) + index] = value;
            relay->config.study_index[ch] = ++index;
        }
        Config::saveConfig();

        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Received %d study to channel %d"), value, (ch) + 1);
        studyCH = 0;
        Led::blinkLED(200, 5);
    }
}

#endif