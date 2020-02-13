// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>
#include "Module.h"

#define GLOBAL_CFG_VERSION 1 // 1 - 999

//#define WIFI_SSID "qlwz"     // WiFi ssid
//#define WIFI_PASS "" // WiFi 密码

//#define MQTT_SERVER "10.0.0.25"   // MQTTַ 地址
//#define MQTT_PORT 1883            // MQTT 端口
//#define MQTT_USER "mqtt"          // MQTT 用户名
//#define MQTT_PASS "" // MQTT 密码

#define MQTT_FULLTOPIC "%module%/%hostname%/%prefix%/" // MQTT 主题格式
#ifndef VERSION
#define VERSION "2020.02.03.0000" // 版本
#endif

#define MAX_STUDY_RECEIVER_NUM 10 // 遥控最大学习数

#define OTA_URL "http://10.0.0.50/esp/%module%.bin"

#define WEB_LOG_SIZE 4000 // Max number of characters in weblog

#define WifiManager_ConfigPortalTimeOut 120
#define MinimumWifiSignalQuality 8

typedef struct _DebugConfigMessage
{
    uint8_t type;
    char server[40];
    uint16_t port;
} DebugConfigMessage;

typedef struct _HttpConfigMessage
{
    uint16_t port;
    char user[15];
    char pass[15];
    char ota_url[150];
} HttpConfigMessage;

typedef struct _MqttConfigMessage
{
    char server[30];
    uint16_t port;
    char user[20];
    char pass[30];
    bool retain;
    char topic[50];
    bool discovery;
    char discovery_prefix[30];
} MqttConfigMessage;

typedef struct _WifiConfigMessage
{
    char ssid[20];
    char pass[30];
    bool is_static;
    char ip[15];
    char sn[15];
    char gw[15];
} WifiConfigMessage;

typedef PB_BYTES_ARRAY_T(500) GlobalConfigMessage_module_cfg_t;
typedef struct _GlobalConfigMessage
{
    WifiConfigMessage wifi;
    HttpConfigMessage http;
    MqttConfigMessage mqtt;
    DebugConfigMessage debug;
    uint16_t cfg_version;
    uint16_t module_crc;
    GlobalConfigMessage_module_cfg_t module_cfg;
    char uid[20];
} GlobalConfigMessage;

extern const pb_field_t GlobalConfigMessage_fields[9];
extern const pb_field_t WifiConfigMessage_fields[7];
extern const pb_field_t HttpConfigMessage_fields[5];
extern const pb_field_t MqttConfigMessage_fields[9];
extern const pb_field_t DebugConfigMessage_fields[4];

#define GlobalConfigMessage_size 1081

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
