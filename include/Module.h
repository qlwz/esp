// Module.h

#ifndef _MODULE_h
#define _MODULE_h

#include "arduino.h"
#include <ESP8266WebServer.h>

class Module
{
public:
    virtual String moduleName();
    virtual bool moduleLed();

    virtual void loop();
    virtual void perSecondDo();

    virtual void httpAdd(ESP8266WebServer *server);
    virtual String httpGetStatus(ESP8266WebServer *server);
    virtual void httpHtml(ESP8266WebServer *server);

    virtual void mqttDiscovery(boolean isEnable = true);
    virtual void mqttCallback(String topicStr, String str);
    virtual void mqttConnected();
};
#endif
