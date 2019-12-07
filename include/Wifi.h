// Wifi.h

#ifndef _WIFI_h
#define _WIFI_h

#include "arduino.h"
#include <WiFiClient.h>
#include <DNSServer.h>

class Wifi
{
private:
    static bool connect;

    static String _ssid;
    static String _pass;

    static DNSServer *dnsServer;

public:
    static unsigned long configPortalStart;
    static void OTA(String url);
    static bool isDHCP;
    static WiFiEventHandler STAGotIP;
    static WiFiClient wifiClient;
    static void connectWifi();
    static void setupWifi();
    static void setupWifiManager(bool resetSettings);
    static boolean isIp(String str);

    static uint8_t waitForConnectResult();
    static void tryConnect(String ssid, String pass);

    static void loop();
};

#endif
