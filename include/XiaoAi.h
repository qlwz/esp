// XiaoAi.h
#ifdef USE_XIAOAI

#ifndef _XIAOAI_h
#define _XIAOAI_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include "Module.h"

#define MODULE_CFG_VERSION 3001 //3001 - 3500

typedef struct _XiaoAiConfigMessage
{
    uint8_t pin_rx;
    uint8_t pin_tx;
    uint8_t pin_led;
    uint8_t pin_btn;
    char password[32];
} XiaoAiConfigMessage;

#define XiaoAiConfigMessage_size 58
extern const pb_field_t XiaoAiConfigMessage_fields[6];

class XiaoAi : public Module
{
private:
    XiaoAiConfigMessage config;

    // 按键
    int buttonDebounceTime = 50;
    int buttonLongPressTime = 2000; // 2000 = 2s
    boolean buttonTiming = false;
    unsigned long buttonTimingStart = 0;
    int buttonAction = 0; // 0 = 没有要执行的动作, 1 = 执行短按动作, 2 = 执行长按动作

    uint8_t operationFlag = 0;
    boolean isLogin = false;

    uint8_t lastVolume = 0;  // 最后音量
    uint8_t lastStatus = 0;  // 最后状态
    char *lastContext[1024]; // 最后内容

    void serialEvent();
    void checkButton();

    void httpSetting(ESP8266WebServer *server);
    void httpCmd(ESP8266WebServer *server);

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