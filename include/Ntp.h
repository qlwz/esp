// Ntp.h

#ifndef _NTP_h
#define _NTP_h

#include "arduino.h"

#define LEAP_YEAR(Y) (((1970 + Y) > 0) && !((1970 + Y) % 4) && (((1970 + Y) % 100) || !((1970 + Y) % 400)))

typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_week; // sunday is day 1
    uint8_t day_of_month;
    uint8_t month;
    uint16_t day_of_year;
    uint16_t year;
    unsigned long days;
    boolean valid;
} TIME_T;

class Ntp
{
protected:
    static uint32_t utcTime;
    static void breakTime(uint32_t time_input, TIME_T &tm);

public:
    static String msToHumanString(uint32_t const msecs);
    static String timeSince(uint32_t const start);

    static String GetBuildDateAndTime();
    static void perSecondDo();
    static TIME_T rtcTime;
    static void init();
};

#endif
