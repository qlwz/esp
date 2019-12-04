// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "config.pb.h"
#include "Module.h"
#include "template.h"

#define CFG_HOLDER 1

#define HOST_PREFIX "ESP_%06X" // Hostname

//#define WIFI_SSID "qlwz"		// WiFi ssid
//#define WIFI_PASS "" // WiFi 密码

#define WifiManager_ConfigPortalTimeOut 120
#define MinimumWifiSignalQuality 8

//#define MQTT_SERVER "10.0.0.25"   // MQTTַ 地址
//#define MQTT_PORT 1883			  // MQTT 端口
//#define MQTT_USER "mqtt"		  // MQTT 用户名
//#define MQTT_PASS "" // MQTT 密码


#define MQTT_FULLTOPIC "%module%/%hostname%/%prefix%/" // MQTT 主题格式
#define VERSION "1.3"								 // 版本

#define MAX_STUDY_RECEIVER_NUM 10 // 遥控最大学习数

#define OTA_URL "http://10.0.0.50/esp/%module%.bin"

#define WEB_LOG_SIZE 4000 // Max number of characters in weblog

enum LoggingLevels
{
	LOG_LEVEL_NONE,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_DEBUG_MORE,
	LOG_LEVEL_ALL
};

extern Module *module;

extern char UID[16];
extern char tmpData[512];
extern uint8_t GPIO_PIN[20];
extern ConfigMessage config;
extern uint32_t perSecond;
extern Ticker *tickerPerSecond;

class Config
{
private:
	static uint16_t nowCrc;

public:
	static uint16_t crc16(uint8_t *ptr, uint16_t len);

	static bool resetConfig();

	static bool readConfig(bool isErrorReset = true);
	static bool saveConfig();

	static bool readConfigSPIFFS(bool isErrorReset = true);
	static bool saveConfigSPIFFS();
	static void perSecondDo();
};

#endif
