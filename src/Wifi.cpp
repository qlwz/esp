
#include "Config.h"
#include "Debug.h"
#include "Wifi.h"
#include "Led.h"
#include <WiFiClient.h>
#include <ESP8266httpUpdate.h>
#include <DNSServer.h>

WiFiClient Wifi::wifiClient;
WiFiEventHandler Wifi::STAGotIP;
bool Wifi::isDHCP = true;

unsigned long Wifi::configPortalStart = 0;
bool Wifi::connect = false;
String Wifi::_ssid = "";
String Wifi::_pass = "";
DNSServer *Wifi::dnsServer;

void Wifi::OTA(String url)
{
    if (url.indexOf(F("%04d")) != -1)
    {
        url.replace("%04d", String(ESP.getChipId() & 0x1fff));
    }
    else if (url.indexOf(F("%d")) != -1)
    {
        url.replace("%d", String(ESP.getChipId()));
    }
    url.replace(F("%hostname%"), UID);
    url.replace(F("%module%"), module->moduleName);

    Debug.AddLog(LOG_LEVEL_INFO, PSTR("OTA Url: %s"), url.c_str());
    Led::blinkLED(200, 5);
    WiFiClient OTAclient;
    if (ESPhttpUpdate.update(OTAclient, url, VERSION) == HTTP_UPDATE_FAILED)
    {
        Debug.AddLog(LOG_LEVEL_ERROR, PSTR("HTTP_UPDATE_FAILD Error (%d): %s"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    }
}

void Wifi::connectWifi()
{
    delay(50);
    if (globalConfig.wifi.ssid[0] != '\0')
    {
        setupWifi();
    }
    else
    {
        setupWifiManager(true);
    }
}

void Wifi::setupWifi()
{
    WiFi.persistent(false); // Solve possible wifi init errors (re-add at 6.2.1.16 #4044, #4083)
    WiFi.disconnect(true);  // Delete SDK wifi config
    delay(200);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.hostname(UID);
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Connecting to %s %s Wifi"), globalConfig.wifi.ssid, globalConfig.wifi.pass);
    STAGotIP = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("WiFi1 connected. SSID: %s IP address: %s"), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        if (globalConfig.wifi.is_static && String(globalConfig.wifi.ip).equals(WiFi.localIP().toString()))
        {
            isDHCP = false;
        }
    });
    if (globalConfig.wifi.is_static)
    {
        isDHCP = false;
        IPAddress static_ip;
        IPAddress static_sn;
        IPAddress static_gw;
        static_ip.fromString(globalConfig.wifi.ip);
        static_sn.fromString(globalConfig.wifi.sn);
        static_gw.fromString(globalConfig.wifi.gw);
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Custom STA IP/GW/Subnet: %s %s %s"), globalConfig.wifi.ip, globalConfig.wifi.sn, globalConfig.wifi.gw);
        WiFi.config(static_ip, static_gw, static_sn);
    }
    WiFi.begin(globalConfig.wifi.ssid, globalConfig.wifi.pass);
}

void Wifi::setupWifiManager(bool resetSettings)
{
    if (resetSettings)
    {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("WifiManager ResetSettings"));
        Config::resetConfig();

        Debug.AddLog(LOG_LEVEL_INFO, PSTR("settings invalidated"));
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA."));
        WiFi.disconnect(true);
    }
    //WiFi.setAutoConnect(true);
    //WiFi.setAutoReconnect(true);
    WiFi.hostname(UID);
    STAGotIP = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("WiFi2 connected. SSID: %s IP address: %s"), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    });

    configPortalStart = millis();
    if (WiFi.isConnected())
    {
        WiFi.mode(WIFI_AP_STA);
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("SET AP STA Mode"));
    }
    else
    {
        WiFi.persistent(false);
        WiFi.disconnect();
        WiFi.mode(WIFI_AP_STA);
        WiFi.persistent(true);
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("SET AP Mode"));
    }

    connect = false;
    WiFi.softAP(UID);
    delay(500);
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("AP IP address: %s"), WiFi.softAPIP().toString().c_str());

    dnsServer = new DNSServer();
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start(53, "*", WiFi.softAPIP());
}

void Wifi::tryConnect(String ssid, String pass)
{
    _ssid = ssid;
    _pass = pass;
    connect = true;
}

void Wifi::loop()
{
    if (configPortalStart == 0)
    {
        return;
    }
    dnsServer->processNextRequest();

    if (connect)
    {
        connect = false;
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Connecting to new AP"));

        WiFi.begin(_ssid.c_str(), _pass.c_str());
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Waiting for connection result with time out"));
    }

    if (_ssid.length() > 0 && WiFi.isConnected())
    {
        strcpy(globalConfig.wifi.ssid, _ssid.c_str());
        strcpy(globalConfig.wifi.pass, _pass.c_str());
        Config::saveConfig();

        //	为了使WEB获取到IP 2秒后才关闭AP
        Ticker *ticker = new Ticker();
        ticker->attach(3, []() {
            WiFi.mode(WIFI_STA);
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("SET STA Mode"));
            ESP.reset();
        });

        Debug.AddLog(LOG_LEVEL_INFO, PSTR("WiFi connected. SSID: %s IP address: %s"), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

        dnsServer->stop();
        configPortalStart = 0;
        _ssid = "";
        _pass = "";
        return;
    }

    // 检查是否超时
    if (millis() > configPortalStart + (WifiManager_ConfigPortalTimeOut * 1000))
    {
        dnsServer->stop();
        configPortalStart = 0;
        _ssid = "";
        _pass = "";
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("startConfigPortal TimeOut"));
        if (WiFi.isConnected())
        {
            //	为了使WEB获取到IP 2秒后才关闭AP
            Ticker *ticker = new Ticker();
            ticker->attach(3, []() {
                WiFi.mode(WIFI_STA);
                Debug.AddLog(LOG_LEVEL_INFO, PSTR("SET STA Mode"));
                ESP.reset();
            });
        }
        else
        {
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("Wifi failed to connect and hit timeout. Rebooting..."));
            delay(3000);
            ESP.reset(); // 重置，可能进入深度睡眠状态
            delay(5000);
        }
    }
}

boolean Wifi::isIp(String str)
{
    int a, b, c, d;
    if ((sscanf(str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) && (a >= 0 && a <= 255) && (b >= 0 && b <= 255) && (c >= 0 && c <= 255) && (d >= 0 && d <= 255))
    {
        return true;
    }
    return false;
}