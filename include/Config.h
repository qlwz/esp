// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "GlobalConfig.pb.h"
#include "Module.h"

#define GLOBAL_CFG_VERSION 1 // 1 - 999

//#define WIFI_SSID "qlwz"     // WiFi ssid
//#define WIFI_PASS "" // WiFi 密码

//#define MQTT_SERVER "10.0.0.25"   // MQTTַ 地址
//#define MQTT_PORT 1883            // MQTT 端口
//#define MQTT_USER "mqtt"          // MQTT 用户名
//#define MQTT_PASS "" // MQTT 密码

#define MQTT_FULLTOPIC "%module%/%hostname%/%prefix%/" // MQTT 主题格式
#define VERSION "2019.12.08.2300"                      // 版本

#define MAX_STUDY_RECEIVER_NUM 10 // 遥控最大学习数

#define OTA_URL "http://10.0.0.50/esp/%module%.bin"

#define WEB_LOG_SIZE 4000 // Max number of characters in weblog

#define WifiManager_ConfigPortalTimeOut 120
#define MinimumWifiSignalQuality 8

extern Module *module;

extern char UID[16];
extern char tmpData[512];
extern GlobalConfigMessage globalConfig;
extern uint32_t perSecond;
extern Ticker *tickerPerSecond;

class Config
{
private:
    static uint16_t nowCrc;

public:
    static uint16_t crc16(uint8_t *ptr, uint16_t len);

    static void readConfig();
    static void resetConfig();
    static boolean saveConfig();

    static void moduleReadConfig(uint16_t version, uint16_t size, const pb_field_t fields[], void *dest_struct);
    static boolean moduleSaveConfig(uint16_t version, uint16_t size, const pb_field_t fields[], const void *src_struct);

    static void perSecondDo();
};

#endif
