// Cover.h
#ifdef USE_COVER

#ifndef _COVER_h
#define _COVER_h

#include "arduino.h"
#include <softwareSerial.h>
#include <ESP8266WebServer.h>
#include "Module.h"

const char HASS_DISCOVER_COVER[] PROGMEM =
	"{\"name\":\"%s\","
	"\"cmd_t\":\"%s\","
	"\"pl_open\":\"OPEN\","
	"\"pl_cls\":\"CLOSE\","
	"\"pl_stop\":\"STOP\","
	"\"pos_t\":\"%s\","
	"\"set_pos_t\":\"%s\","
	"\"position_open\":100,"
	"\"position_closed\":0,"
	"\"avty_t\":\"%s\","
	"\"pl_avail\":\"online\","
	"\"pl_not_avail\":\"offline\","
	"\"qos\":0,"
	"\"ret\":true,"
	"\"opt\":false}";

class Cover : public Module
{
private:
	uint8_t getInt(String str, uint8_t min, uint8_t max);
	void handleCoverPosition(ESP8266WebServer *server);
	void handleCoverSet(ESP8266WebServer *server);
	void handleCoverSetting(ESP8266WebServer *server);
	void handleCoverReset(ESP8266WebServer *server);
	void doPosition(uint8_t position, uint8_t command);
	void doSoftwareSerialTick(uint8_t *buf, int len);

	boolean getPositionState = false;
	SoftwareSerial *softwareSerial;		  // RX, TX
	uint8_t softwareSerialBuff[20];		  // 定义缓冲buff
	uint8_t softwareSerialPos = 0;		  // 收到的字节实际长度
	unsigned long softwareSerialTime = 0; // 记录读取最后一个字节的时间点
	boolean autoStroke = false;			  // 是否自动设置行程

	// 按键
	int buttonDebounceTime = 50;
	int buttonLongPressTime = 2000; // 2000 = 2s
	boolean buttonTiming = false;
	unsigned long buttonTimingStart = 0;
	int buttonAction = 0; // 0 = 没有要执行的动作, 1 = 执行短按动作, 2 = 执行长按动作

public:
	Cover();
	String moduleName();
	void mqttCallback(String topicStr, String str);
	void readSoftwareSerialTick();
	void getPositionTask();
	void checkButton();
	void loop();
	void mqttConnected();
	void perSecondDo();
	void mqttDiscovery(boolean isEnable = true);

	void httpAdd(ESP8266WebServer *server);
	String httpGetStatus(ESP8266WebServer *server);
	void httpHtml(ESP8266WebServer *server);

	bool moduleLed();
};
#endif

#endif