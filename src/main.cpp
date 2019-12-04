
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
	if (mqtt)
	{
		mqtt->perSecondDo();
	}
	if (module)
	{
		module->perSecondDo();
	}
}

void setup()
{
	sprintf(UID, HOST_PREFIX, ESP.getChipId());
	Serial.begin(115200);
	EEPROM.begin(ConfigMessage_size + 6);
	config.debug_type = 1;
	Config::readConfig();
	Debug.AddLog(LOG_LEVEL_INFO, PSTR("---------------------  v%s  %s  -------------------"), VERSION, Ntp::GetBuildDateAndTime().c_str());
	Debug.AddLog(LOG_LEVEL_INFO, PSTR("UID: %s"), UID);
	Debug.AddLog(LOG_LEVEL_INFO, PSTR("Config Len: %d"), ConfigMessage_size + 6);
	//Config::resetConfig();
	if (MQTT_MAX_PACKET_SIZE == 128)
	{
		Debug.AddLog(LOG_LEVEL_INFO, PSTR("WRONG PUBSUBCLIENT LIBRARY USED PLEASE INSTALL THE ONE FROM OMG LIB FOLDER"));
	}

	for (uint16_t i = 0; i < sizeof(GPIO_PIN); i++)
	{
		GPIO_PIN[i] = 99;
	}

	mytmplt m = Modules[config.module_type];
	uint8_t j = 0;
	for (uint8_t i = 0; i < sizeof(m.io); i++)
	{
		if (6 == i)
		{
			j = 9;
		}
		if (8 == i)
		{
			j = 12;
		}
		GPIO_PIN[m.io[i]] = j;
		//Debug.printf("F %d %d \n", m.io[i], j);

		if (m.io[i] == GPIO_LED_POWER)
		{
			Led::init(j, HIGH);
		}
		else if (m.io[i] == GPIO_LED_POWER_INV)
		{
			Led::init(j, LOW);
		}
		j++;
	}

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

	if (!module)
	{
		Debug.AddLog(LOG_LEVEL_INFO, PSTR("NO MODULE"));
		Config::resetConfig();
		Config::saveConfig();
		ESP.reset();
		return;
	}

	tickerPerSecond = new Ticker();
	tickerPerSecond->attach(1, tickerPerSecondDo);
	Http::begin();
	Wifi::connectWifi();
	Ntp::init();

	mqtt->setTopic();
	mqtt->setClient(Wifi::wifiClient);
	mqtt->mqttSetConnectedCallback(connectedCallback);
	mqtt->mqttSetLoopCallback(callback);
}

void loop()
{
	Led::loop();
	if (mqtt)
	{
		mqtt->loop();
	}
	if (module)
	{
		module->loop();
	}
	Wifi::loop();
	Http::loop();
}
