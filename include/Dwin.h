// Dwin.h
#ifdef USE_DWIN

#ifndef _USE_DWIN_h
#define _USE_DWIN_h

#include "Module.h"

#define MODULE_CFG_VERSION 3501 //3501 - 4000

typedef struct _DwinConfigMessage
{
    uint8_t pin_rx;
    uint8_t pin_tx;
    uint8_t pin_led;
    uint8_t pin_btn;
    char password[32];
} DwinConfigMessage;

#define DwinConfigMessage_size 58
extern const pb_field_t DwinConfigMessage_fields[6];

typedef struct _DeviceInfo
{
    char id[50];
    char name[50];
    char state[2];
    uint8_t index;
    struct _DeviceInfo *next;
} DeviceInfo;

class Dwin : public Module
{
private:
    DwinConfigMessage config;

    uint8_t serialBuff[50];       // 定义缓冲buff
    uint8_t serialPos = 0;        // 收到的字节实际长度
    unsigned long serialTime = 0; // 记录读取最后一个字节的时间点
    void doserialTick(uint8_t *buf, int len);
    void serialEvent();

    void httpSetting(ESP8266WebServer *server);
    void httpCmd(ESP8266WebServer *server);

    DeviceInfo *deviceList;
    uint8_t deviceCount = 0;
    uint8_t nowPage = 1;
    uint8_t bjStatus = 0;
    uint8_t pageStatus = 0;
    uint8_t textColorStatus = 0;

    uint16_t iconStatus[6] = {0, 0, 0, 0, 0, 0};
    uint16_t bjAddress[6] = {0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006};
    uint16_t iconAddress[6] = {0x2011, 0x2012, 0x2013, 0x2014, 0x2015, 0x2016};
    uint16_t textAddress[6] = {0x2100, 0x2200, 0x2300, 0x2400, 0x2500, 0x2600};
    uint16_t text2Address[6] = {0x2150, 0x2250, 0x2350, 0x2450, 0x2550, 0x2650};

    void getList();
    void showList();
    void showDevice(uint8 i, DeviceInfo *info);
    void hideDevice(uint8 i);
    void tochDevice(uint16_t key);
    void jumpPage(uint16_t page);
    void tocuhStatus(uint16_t page, uint8_t id, uint8_t type, bool status);

    uint8_t dwinBuff[50];
    uint8_t generate82Command(uint8_t *outData, uint16_t address, uint8_t dataLen, uint8_t *data);
    uint8_t generate82CommandData(uint8_t *outData, uint16_t address, uint16_t data);
    void sendGenerate82Command(uint16_t address, uint8_t dataLen, uint8_t *data);
    void sendGenerate82CommandData(uint16_t address, uint16_t data);

    DeviceInfo *LinkListInit();
    void InsertValue(DeviceInfo *p_node, const char *id, const char *name, const char *state);
    void InsertPosValue(DeviceInfo *p_node, int pos, const char *id, const char *name, const char *state);
    void ForeachLinklist(DeviceInfo *p_node);
    void RemoveLinkList(DeviceInfo *p_node, int pos);
    void RemoveValueLinkList(DeviceInfo *p_node, char *id);
    void ClearLinkList(DeviceInfo *p_node);
    void DestoryLinkList(DeviceInfo *p_node);

public:
    void init();
    String getModuleName() { return F("dwin"); }
    String getModuleCNName() { return F("迪文屏"); }
    String getModuleVersion() { return F("2020.02.13.1200"); }
    bool moduleLed() { return false; }

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