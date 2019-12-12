// Relay.h
#ifdef USE_RELAY

#ifndef _RELAY_h
#define _RELAY_h

#include "arduino.h"
#include <Ticker.h>
#include <ESP8266WebServer.h>
#include "RelayConfig.pb.h"
#include "Module.h"
#include "RelayButton.h"
#include "RadioReceive.h"

#define MODULE_CFG_VERSION 1001 //1001 - 1500
#define MAX_GPIO_PIN 17         // Number of supported GPIO
#define MIN_FLASH_PINS 4        // Number of flash chip pins unusable for configuration (GPIO6, 7, 8 and 11)

const char HASS_DISCOVER_RELAY[] PROGMEM =
    "{\"name\":\"%s_%d\","
    "\"cmd_t\":\"%s\","
    "\"stat_t\":\"%s\","
    "\"pl_off\":\"OFF\","
    "\"pl_on\":\"ON\","
    "\"avty_t\":\"%s\","
    "\"pl_avail\":\"online\","
    "\"pl_not_avail\":\"offline\"}";

class Relay : public Module
{
private:
    uint8_t GPIO_PIN[MAX_GPIO_PIN - MIN_FLASH_PINS];

    // PWM
    Ticker *ledTicker;
    int ledLevel = 0;
    int ledLight = 2023;
    boolean ledUp = true;
    boolean canLed = true;
    void led(uint8_t ch, bool isOn);
    void ledPWM(uint8_t ch, bool isOn);
    void ledTickerHandle();
    boolean checkCanLed(boolean re = false);

    String powerTopic;
    RelayButton *btns;

    void httpDo(ESP8266WebServer *server);
    void httpRadioReceive(ESP8266WebServer *server);
    void httpSetting(ESP8266WebServer *server);

    void loadModule(uint8_t module);

public:
    RelayConfigMessage config;
    RadioReceive *radioReceive;
    boolean lastState[4] = {false, false, false, false};
    uint8_t channels = 0;

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

    void switchRelay(uint8_t ch, bool isOn, bool isSave = true);
};

typedef struct MYTMPLT
{
    char name[15];
    uint8_t io[MAX_GPIO_PIN - MIN_FLASH_PINS];
} mytmplt;

enum UserSelectablePins
{
    GPIO_NONE, // Not used
    GPIO_REL1, // Relays
    GPIO_REL2,
    GPIO_REL3,
    GPIO_REL4,
    GPIO_KEY1, // Button usually connected to GPIO0
    GPIO_KEY2,
    GPIO_KEY3,
    GPIO_KEY4,
    GPIO_LED_POWER,     // Power Led (1 = On, 0 = Off)
    GPIO_LED_POWER_INV, // Power Led (0 = On, 1 = Off)
    GPIO_LED1,          // Leds
    GPIO_LED2,
    GPIO_LED3,
    GPIO_LED4,
    GPIO_RFRECV, // RF receiver
};

enum SupportedModules
{
    SONOFF_BASIC,
    CH1,
    CH2,
    CH3,
    iciness_CH3,

    END // 占位
};

const mytmplt Modules[] PROGMEM = {
    {
        "Sonoff Basic",     // Sonoff Basic (ESP8266)
        GPIO_KEY1,          // GPIO00 Button
        GPIO_NONE,          // GPIO01
        GPIO_NONE,          // GPIO02
        GPIO_NONE,          // GPIO03
        GPIO_NONE,          // GPIO04
        0,                  // GPIO05
                            // GPIO06 (SD_CLK   Flash)
                            // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                            // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        0,                  // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        0,                  // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                            // GPIO11 (SD_CMD   Flash)
        GPIO_REL1,          // GPIO12 Red Led and Relay (0 = Off, 1 = On)
        GPIO_LED_POWER_INV, // GPIO13 Green Led (0 = On, 1 = Off) - Link and Power status
        GPIO_NONE,          // GPIO14
        0,                  // GPIO15
        0                   // GPIO16
    },
    {
        "1 Channel",   // 3 Channel (ESP8285)
        GPIO_NONE,     // GPIO00
        GPIO_NONE,     // GPIO01
        GPIO_KEY1,     // GPIO02
        GPIO_NONE,     // GPIO03
        GPIO_NONE,     // GPIO04
        GPIO_NONE,     // GPIO05
                       // GPIO06 (SD_CLK   Flash)
                       // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                       // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_NONE,     // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_NONE,     // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                       // GPIO11 (SD_CMD   Flash)
        GPIO_NONE,     // GPIO12
        GPIO_RFRECV,   // GPIO13
        GPIO_REL1,     // GPIO14
        GPIO_LED1,     // GPIO15
        GPIO_LED_POWER // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
    {
        "2 Channel",   // 3 Channel (ESP8285)
        GPIO_LED2,     // GPIO00
        GPIO_NONE,     // GPIO01
        GPIO_KEY1,     // GPIO02
        GPIO_NONE,     // GPIO03
        GPIO_KEY2,     // GPIO04
        GPIO_NONE,     // GPIO05
                       // GPIO06 (SD_CLK   Flash)
                       // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                       // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_NONE,     // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_NONE,     // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                       // GPIO11 (SD_CMD   Flash)
        GPIO_REL2,     // GPIO12
        GPIO_RFRECV,   // GPIO13
        GPIO_REL1,     // GPIO14
        GPIO_LED1,     // GPIO15
        GPIO_LED_POWER // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
    {
        "3 Channel",   // 3 Channel (ESP8285)
        GPIO_LED2,     // GPIO00
        GPIO_NONE,     // GPIO01
        GPIO_LED3,     // GPIO02
        GPIO_NONE,     // GPIO03
        GPIO_KEY2,     // GPIO04
        GPIO_REL3,     // GPIO05
                       // GPIO06 (SD_CLK   Flash)
                       // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                       // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_KEY1,     // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_KEY3,     // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                       // GPIO11 (SD_CMD   Flash)
        GPIO_REL2,     // GPIO12
        GPIO_RFRECV,   // GPIO13
        GPIO_REL1,     // GPIO14
        GPIO_LED1,     // GPIO15
        GPIO_LED_POWER // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
    {
        "iciness CH3", // 3 Channel (ESP8285)
        GPIO_NONE,     // GPIO00
        GPIO_NONE,     // GPIO01
        GPIO_KEY3,     // GPIO02
        GPIO_NONE,     // GPIO03
        GPIO_LED1,     // GPIO04
        GPIO_LED2,     // GPIO05
                       // GPIO06 (SD_CLK   Flash)
                       // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                       // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_KEY1,     // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_KEY2,     // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                       // GPIO11 (SD_CMD   Flash)
        GPIO_REL3,     // GPIO12
        GPIO_NONE,     // GPIO13
        GPIO_REL1,     // GPIO14
        GPIO_REL2,     // GPIO15
        GPIO_LED3      // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
};

#endif

#endif