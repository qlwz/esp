
#include "Config.h"
#include "Ntp.h"
#include "Debug.h"
#include "Led.h"
#include "Relay.h"
#ifdef USE_COVER
#include "Cover.h"
#endif
#include "Http.h"
#include "template.h"
#include "Wifi.h"
#include "Mqtt.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "config.pb.h"
#include "Zinguo.h"

void callback(char *topic, byte *payload, unsigned int length)
{
    String str;
    for (int i = 0; i < length; i++)
    {
        str += (char)payload[i];
    }

    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Subscribe: %s payload: %s"), topic, str.c_str());

    String topicStr = String(topic);
    if (topicStr.endsWith(F("/OTA")))
    {
        Wifi::OTA(str.endsWith(F(".bin")) ? str : OTA_URL);
    }
    else if (topicStr.endsWith(F("/restart")))
    {
        ESP.reset();
    }
    else if (module)
    {
        module->mqttCallback(topicStr, str);
    }

    Led::led(200);
}

void connectedCallback()
{
    mqtt->subscribe(mqtt->getCmndTopic(F("#")));
    Led::blinkLED(40, 8);
    if (module)
    {
        module->mqttConnected();
    }
}

void tickerPerSecondDo()
{
    perSecond++;
    Ntp::perSecondDo();

    Config::perSecondDo();
    mqtt->perSecondDo();
    module->perSecondDo();
}

void setup()
{
    sprintf(UID, HOST_PREFIX, ESP.getChipId());
    Serial.begin(115200);
    EEPROM.begin(ConfigMessage_size + 6);
    config.debug.type = 1;
    Config::readConfig();
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("---------------------  v%s  %s  -------------------"), VERSION, Ntp::GetBuildDateAndTime().c_str());
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("UID: %s"), UID);
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Config Len: %d"), ConfigMessage_size + 6);
    //Config::resetConfig();
    if (MQTT_MAX_PACKET_SIZE == 128)
    {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("WRONG PUBSUBCLIENT LIBRARY USED PLEASE INSTALL THE ONE FROM OMG LIB FOLDER"));
    }

    Config::loadModule(config.module_type);

    mqtt = new Mqtt();
#ifdef USE_RELAY
    module = new Relay();
#elif defined USE_COVER
    module = new Cover();
#elif defined USE_ZINGUO
    module = new Zinguo();
#else
#error "No CLASS"
#endif

    if (GPIO_PIN[GPIO_LED_POWER] != 99)
    {
        Led::init(GPIO_PIN[GPIO_LED_POWER], HIGH);
    }
    else if (GPIO_PIN[GPIO_LED_POWER_INV] != 99)
    {
        Led::init(GPIO_PIN[GPIO_LED_POWER_INV], LOW);
    }

    tickerPerSecond = new Ticker();
    tickerPerSecond->attach(1, tickerPerSecondDo);
    Http::begin();
    Wifi::connectWifi();
    Ntp::init();

    mqtt->setClient(Wifi::wifiClient);
    mqtt->mqttSetConnectedCallback(connectedCallback);
    mqtt->mqttSetLoopCallback(callback);
}

void loop()
{
    Led::loop();
    mqtt->loop();
    module->loop();
    Wifi::loop();
    Http::loop();
}
