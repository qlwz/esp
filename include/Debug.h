// Debug.h

#ifndef _DEBUG_h
#define _DEBUG_h

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "Config.h"

enum LoggingLevels
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_DEBUG_MORE,
    LOG_LEVEL_ALL
};

class DebugClass // : public Print
{
protected:
    size_t strchrspn(const char *str1, int character);

public:
    uint8_t webLogIndex = 1;
    char webLog[WEB_LOG_SIZE] = {'\0'};
    void GetLog(uint8_t idx, char **entry_pp, uint16_t *len_p);

    IPAddress ip;
    void Syslog();
    void AddLog(uint8_t loglevel);
    void AddLog(uint8_t loglevel, PGM_P formatP, ...);
};

extern DebugClass Debug;

#endif
