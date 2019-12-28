// Weile.h
#ifdef USE_WEILE

#ifndef _WEILE_h
#define _WEILE_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include "Module.h"

#define MODULE_CFG_VERSION 2501 //2501 - 3000

const char HASS_DISCOVER_WEILE[] PROGMEM =
    "{\"name\":\"%s\","
    "\"cmd_t\":\"%s\","
    "\"stat_t\":\"%s\","
    "\"pl_off\":\"OFF\","
    "\"pl_on\":\"ON\","
    "\"avty_t\":\"%s\","
    "\"pl_avail\":\"online\","
    "\"pl_not_avail\":\"offline\"}";

typedef struct _WeileConfigMessage
{
    uint16_t jog_time;
    uint16_t start_interval;
    uint16_t weile_time;
    uint16_t screen_time;
    uint16_t close_power;
    uint8_t pin_rel;
    uint8_t pin_led;
    uint8_t pin_btn;
} WeileConfigMessage;

extern const pb_field_t WeileConfigMessage_fields[9];
#define WeileConfigMessage_size 51

class Weile : public Module
{
private:
    WeileConfigMessage config;

    // 按键
    int buttonDebounceTime = 50;
    int buttonLongPressTime = 2000; // 2000 = 2s
    boolean buttonTiming = false;
    unsigned long buttonTimingStart = 0;
    int buttonAction = 0; // 0 = 没有要执行的动作, 1 = 执行短按动作, 2 = 执行长按动作

    void httpPosition(ESP8266WebServer *server);
    void httpDo(ESP8266WebServer *server);
    void httpSetting(ESP8266WebServer *server);
    void httpReset(ESP8266WebServer *server);

    void checkButton();

    String powerTopic;
    boolean weiLeStatus = false;
    uint64_t weileTime = false;

    boolean screenStatus = false;
    uint64_t screenTime = false;

    void openScreen(boolean isInterval);
    void pressBtn();
    void weileOpen();
    void weileClose();

public:
    void init();
    String getModuleName();
    String getModuleCNName();
    bool moduleLed();

    void loop();
    void perSecondDo();

    void readConfig();
    void resetConfig();
    void saveConfig();

    void mqttCallback(String topicStr, String str);
    void mqttConnected();
    void mqttDiscovery(boolean isEnable = true);

    void httpAdd(ESP8266WebServer *server);
    void httpHtml(ESP8266WebServer *server);
    String httpGetStatus(ESP8266WebServer *server);
};
#endif

#endif