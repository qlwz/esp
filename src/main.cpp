
#include "Config.h"
#include "Ntp.h"
#include "Debug.h"
#include "Led.h"
#include "Http.h"
#include "Wifi.h"
#include "Mqtt.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#ifdef USE_RELAY
#include "Relay.h"
#elif defined USE_COVER
#include "Cover.h"
#elif defined USE_ZINGUO
#include "Zinguo.h"
#elif defined USE_WEILE
#include "Weile.h"
#else
#error "not support module"
#endif

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
    EEPROM.begin(GlobalConfigMessage_size + 6);
    globalConfig.debug.type = 1;

#ifdef USE_RELAY
    module = new Relay();
#elif defined USE_COVER
    module = new Cover();
#elif defined USE_ZINGUO
    module = new Zinguo();
#elif defined USE_WEILE
    module = new Weile();
#endif

    Config::readConfig();
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("---------------------  v%s  %s  -------------------"), VERSION, Ntp::GetBuildDateAndTime().c_str());
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("UID: %s"), UID);
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Config Len: %d"), GlobalConfigMessage_size + 6);
    //Config::resetConfig();
    if (MQTT_MAX_PACKET_SIZE == 128)
    {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("WRONG PUBSUBCLIENT LIBRARY USED PLEASE INSTALL THE ONE FROM OMG LIB FOLDER"));
    }

    mqtt = new Mqtt();
    tickerPerSecond = new Ticker();
    tickerPerSecond->attach(1, tickerPerSecondDo);
    Http::begin();
    Wifi::connectWifi();
    Ntp::init();

    module->init();

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
