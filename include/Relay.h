// Relay.h
#ifdef USE_RELAY

#ifndef _RELAY_h
#define _RELAY_h

#include "arduino.h"
#include "RelayButton.h"
#include "RadioReceive.h"
#include "Module.h"
#include <Ticker.h>
#include <ESP8266WebServer.h>

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

public:
	// PWM
	int ledLevel = 0;
	int ledLight = 2023;
	boolean ledUp = true;
	void ledTickerHandle();

	uint8_t channels = 0;
	boolean lastState[4] = {false, false, false, false};

	String powerTopic;
	RelayButton *btns;
	RadioReceive *radioReceive;
	Ticker *ledTicker;

	boolean canLed = true;
	boolean checkCanLed(boolean re = false);

	void led(uint8_t ch, bool isOn);
	void ledPWM(uint8_t ch, bool isOn);
	void switchRelay(uint8_t ch, bool isOn, bool isSave = true);
	void mqttCallback(String topicStr, String str);
	Relay();
	String moduleName();
	void loop();
	void mqttDiscovery(boolean isEnable = true);
	void mqttConnected();

	void handleRelaySet(ESP8266WebServer *server);
	void handleRadioReceive(ESP8266WebServer *server);
	void handleRelaySetting(ESP8266WebServer *server);

	void perSecondDo();

	void httpAdd(ESP8266WebServer *server);
	String httpGetStatus(ESP8266WebServer *server);
	void httpHtml(ESP8266WebServer *server);

	bool moduleLed();
};

#endif

#endif