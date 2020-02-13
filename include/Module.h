// Module.h

#ifndef _MODULE_h
#define _MODULE_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include "Config.h"

class Module
{
public:
    virtual void init();
    virtual String getModuleName();
    virtual String getModuleCNName();
    virtual bool moduleLed();

    virtual void loop();
    virtual void perSecondDo();

    virtual void readConfig();
    virtual void resetConfig();
    virtual void saveConfig();

    virtual void httpAdd(ESP8266WebServer *server);
    virtual void httpHtml(ESP8266WebServer *server);
    virtual String httpGetStatus(ESP8266WebServer *server);

    virtual void mqttCallback(String topicStr, String str);
    virtual void mqttConnected();
    virtual void mqttDiscovery(boolean isEnable = true);
};
#endif
