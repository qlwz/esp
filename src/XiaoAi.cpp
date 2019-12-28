#ifdef USE_XIAOAI

#include "Debug.h"
#include "Led.h"
#include "XiaoAi.h"
#include "Mqtt.h"
#include "Wifi.h"

#pragma region 继承

void XiaoAi::init()
{
    if (config.pin_led != 99)
    {
        Led::init(config.pin_led > 30 ? config.pin_led - 30 : config.pin_led, config.pin_led > 30 ? LOW : HIGH);
    }
    Serial.println();
}

String XiaoAi::getModuleName()
{
    return F("xiaoai");
}

String XiaoAi::getModuleCNName()
{
    return F("小爱音箱");
}

bool XiaoAi::moduleLed()
{
    return false;
}

void XiaoAi::loop()
{
    serialEvent();
    checkButton();

    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
        Serial.println("[ ! -f /data/dropbear_rsa_host_key ] && dropbearkey -t rsa -f /data/dropbear_rsa_host_key");
    }
    if (bitRead(operationFlag, 1))
    {
        bitClear(operationFlag, 1);
        Serial.println("test `ps|grep 'dropbear -r /data/dropbear_rsa_host_key'|grep -v grep|wc -l` -eq 0 && dropbear -r /data/dropbear_rsa_host_key");
    }
    if (bitRead(operationFlag, 2))
    {
        bitClear(operationFlag, 2);
        Serial.println("test `ps|grep 'sh /data/mico.sh'|grep -v grep|wc -l` -eq 0 && sh /data/mico.sh > /tmp/mico.log 2>&1 &");
    }
    if (bitRead(operationFlag, 3))
    {
        bitClear(operationFlag, 3);
        Serial.println("echo 'mqtt:context:'$(ubus call mediaplayer player_get_context)");
    }
    if (bitRead(operationFlag, 4))
    {
        bitClear(operationFlag, 4);
        Serial.println("echo 'mqtt:status:'$(ubus call mediaplayer player_get_play_status | grep status | awk -F \",\" '{print $1}' | awk '{print $4}')");
    }
    if (bitRead(operationFlag, 5))
    {
        bitClear(operationFlag, 5);
        Serial.println("echo 'mqtt:volume:'$(mphelper volume_get)");
    }
}

void XiaoAi::perSecondDo()
{
    if (perSecond % 48 == 0)
    {
        Serial.println();
    }
    if (perSecond % 5 == 0)
    {
        bitSet(operationFlag, 3);
    }
    /*
    if (perSecond % 6 == 0)
    {
        bitSet(operationFlag, 4);
    }
    if (perSecond % 7 == 0)
    {
        bitSet(operationFlag, 5);
    }
    */
    if (isLogin)
    {
        if (perSecond % 53 == 0)
        {
            bitSet(operationFlag, 0);
        }
        else if (perSecond % 63 == 0)
        {
            bitSet(operationFlag, 1);
        }
        else if (perSecond % 77 == 0)
        {
            bitSet(operationFlag, 2);
        }
    }
}

void XiaoAi::checkButton()
{
    boolean buttonState = digitalRead(config.pin_btn);

    if (buttonState == 0)
    { // 按下按钮
        if (buttonTiming == false)
        {
            buttonTiming = true;
            buttonTimingStart = millis();
        }
        else
        { // buttonTiming = true
            if (millis() >= (buttonTimingStart + buttonDebounceTime))
            {
                buttonAction = 1;
            }

            if (millis() >= (buttonTimingStart + buttonLongPressTime))
            {
                buttonAction = 2;
            }
        }
    }
    else
    { // buttonState == 1, 释放按钮
        buttonTiming = false;

        if (buttonAction != 0)
        {
            if (buttonAction == 1) // 执行短按动作
            {
                Debug.AddLog(LOG_LEVEL_INFO, PSTR("buttonShortPressAction"));
            }
            else if (buttonAction == 2) // 执行长按动作
            {
                Debug.AddLog(LOG_LEVEL_INFO, PSTR("buttonLongPressAction"));
                Wifi::setupWifiManager(false);
            }
            buttonAction = 0;
        }
    }
}
#pragma endregion

#pragma region 配置

void XiaoAi::readConfig()
{
    globalConfig.debug.type = globalConfig.debug.type & ~1;
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(XiaoAiConfigMessage), XiaoAiConfigMessage_fields, &config);
}

void XiaoAi::resetConfig()
{
    globalConfig.debug.type = globalConfig.debug.type & ~1;
    globalConfig.debug.type = globalConfig.debug.type | 8;
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(XiaoAiConfigMessage));

    config.pin_rx = 14;
    config.pin_tx = 12;
    config.pin_led = 16;
    config.pin_btn = 0;
}

void XiaoAi::saveConfig()
{
    globalConfig.debug.type = globalConfig.debug.type & ~1;
    Config::moduleSaveConfig(MODULE_CFG_VERSION, XiaoAiConfigMessage_size, XiaoAiConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void XiaoAi::mqttCallback(String topicStr, String str)
{
    if (topicStr.endsWith("/tts"))
    {
        Serial.println("ubus call mibrain text_to_speech \"{\\\"text\\\":\\\"" + str + "\\\",\\\"save\\\":0}\" > /dev/null");
    }
    else if (topicStr.endsWith("/cmd"))
    {
        Serial.println(str);
    }
    else if (topicStr.endsWith("/volume"))
    {
        if (str == F("up") || str == F("down"))
        {
            Serial.printf("mphelper volume_%s > /dev/null", str.c_str());
            Serial.println();
        }
        else
        {
            uint8_t n = str.toInt();
            if (n > 100)
                n = 100;
            if (n < 0)
                n = 0;
            Serial.printf("mphelper volume_set %d > /dev/null", n);
            Serial.println();
        }
        Serial.println("echo 'mqtt:volume:'$(mphelper volume_get)");
    }
    else if (topicStr.endsWith("/player"))
    {
        if (str == F("ch") || str == F("prev") || str == F("next") || str == F("toggle") || str == F("play") || str == F("pause"))
        {
            Serial.printf("mphelper %s > /dev/null", str.c_str());
            Serial.println();
        }
        else if (str.startsWith("http"))
        {
            Serial.printf("mphelper tone %s > /dev/null", str.c_str());
            Serial.println();
        }
        Serial.println("echo 'mqtt:status:'$(ubus call mediaplayer player_get_play_status | grep status | awk -F \",\" '{print $1}' | awk '{print $4}')");
        delay(100);
        Serial.println("echo 'mqtt:context:'$(ubus call mediaplayer player_get_context)");
    }
}

void XiaoAi::mqttDiscovery(boolean isEnable)
{
}

void XiaoAi::mqttConnected()
{
}
#pragma endregion

#pragma region HTTP

void XiaoAi::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/xiaoai_setting"), std::bind(&XiaoAi::httpSetting, this, server));
    server->on(F("/cmd_setting"), std::bind(&XiaoAi::httpCmd, this, server));
}

String XiaoAi::httpGetStatus(ESP8266WebServer *server)
{
    return "";
}

void XiaoAi::httpHtml(ESP8266WebServer *server)
{
    String page = F("<form method='post' action='/xiaoai_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>小爱设置</th></tr></thead><tbody>");
    page += F("<tr><td>密码</td><td><input type='text' name='password' value='{password}'></td></tr>");
    page.replace(F("{password}"), config.password);
    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button></td></tr>");
    page += F("</tbody></table></form>");

    page += F("<form method='post' action='/cmd_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>执行命令</th></tr></thead><tbody>");
    page += F("<tr><td>命令</td><td><input type='text' name='cmd' style='width:98%'></td></tr>");
    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>执行</button></td></tr>");
    page += F("</tbody></table></form>");
    server->sendContent(page);
}

void XiaoAi::httpSetting(ESP8266WebServer *server)
{
    strcpy(config.password, server->arg(F("password")).c_str());

    Config::saveConfig();
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置成功。\"}"));
}

void XiaoAi::httpCmd(ESP8266WebServer *server)
{
    String cmd = server->arg(F("cmd")).c_str();

    Serial.print(cmd);
    delay(50);
    Serial.println();

    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已执行\"}"));
}

#pragma endregion

void XiaoAi::serialEvent()
{
    while (Serial.available())
    {
        delay(10);
        String str = Serial.readString();
        Serial1.println(str);
        //Debug.AddLog(LOG_LEVEL_INFO, PSTR("%s"), str.c_str());
        if ((str.indexOf("Please press Enter to activate this console") > 0) || (str.indexOf("crond (busybox 1.27.2) started, log level 5") > 0))
        {
            isLogin = false;
            Serial.println();
            delay(100);
            Serial.println();
        }
        if (str.indexOf("mico login: ") > 0)
        {
            isLogin = false;
            Serial.print("root"); //mico login:
            delay(100);
            Serial.println();
            Led::blinkLED(100, 10);
        }
        if (str.indexOf("Password: ") > 0)
        {
            isLogin = false;
            Serial.printf("%s", config.password); //Password:
            delay(100);
            Serial.println();
            Led::blinkLED(100, 10);
        }
        if (!isLogin && (str.indexOf("root@mico:~#") > 0 || str.indexOf("root@mico:/") > 0))
        {
            isLogin = true;
            bitSet(operationFlag, 2);
        }
        if (str.indexOf("mqtt:volume:") > 0)
        {
            char *p;
            p = strtok((char *)str.c_str(), "\n");
            while (p)
            {
                if (strncmp(p, "mqtt:volume:", 12) == 0)
                {
                    Debug.AddLog(LOG_LEVEL_INFO, PSTR("%s"), p);
                    char t[20] = {0};
                    strncpy(t, &p[12], strlen(p) - 12 - 1);
                    uint8_t val = atoi(t);
                    if (val != lastVolume)
                    {
                        lastVolume = val;
                        String topic = mqtt->getStatTopic(F("volume"));
                        mqtt->publish(topic, String(lastVolume).c_str(), globalConfig.mqtt.retain);
                        break;
                    }
                }
                p = strtok(NULL, "\n");
            }
        }
        if (str.indexOf("mqtt:status:") > 0)
        {
            char *p;
            p = strtok((char *)str.c_str(), "\n");
            while (p)
            {
                if (strncmp(p, "mqtt:status:", 12) == 0)
                {
                    bitSet(operationFlag, 5);
                    Debug.AddLog(LOG_LEVEL_INFO, PSTR("%s"), p);
                    char t[20] = {0};
                    strncpy(t, &p[12], strlen(p) - 12 - 1);
                    uint8_t val = atoi(t);
                    if (val != lastStatus)
                    {
                        lastStatus = val;
                        String topic = mqtt->getStatTopic(F("status"));
                        mqtt->publish(topic, String(lastStatus).c_str(), globalConfig.mqtt.retain);
                        break;
                    }
                }
                p = strtok(NULL, "\n");
            }
        }
        if (str.indexOf("mqtt:context:") > 0)
        {
            char *p;
            p = strtok((char *)str.c_str(), "\n");
            while (p)
            {
                if (strncmp(p, "mqtt:context:", 13) == 0)
                {
                    bitSet(operationFlag, 4);
                    int len = strlen(p) - 13 - 1;
                    if (strncmp((const char *)lastContext, &p[13], len) != 0)
                    {
                        strncpy((char *)lastContext, &p[13], len);
                        String topic = mqtt->getStatTopic(F("context"));
                        mqtt->publish(topic.c_str(), (const uint8_t *)lastContext, len, globalConfig.mqtt.retain);
                    }
                    //Debug.AddLog(LOG_LEVEL_INFO, PSTR("%s"), p);
                    break;
                }
                p = strtok(NULL, "\n");
            }
        }
    }
}

const pb_field_t XiaoAiConfigMessage_fields[6] = {
    PB_FIELD(1, UINT32, SINGULAR, STATIC, FIRST, XiaoAiConfigMessage, pin_rx, pin_rx, 0),
    PB_FIELD(2, UINT32, SINGULAR, STATIC, OTHER, XiaoAiConfigMessage, pin_tx, pin_rx, 0),
    PB_FIELD(3, UINT32, SINGULAR, STATIC, OTHER, XiaoAiConfigMessage, pin_led, pin_tx, 0),
    PB_FIELD(4, UINT32, SINGULAR, STATIC, OTHER, XiaoAiConfigMessage, pin_btn, pin_led, 0),
    PB_FIELD(5, STRING, SINGULAR, STATIC, OTHER, XiaoAiConfigMessage, password, pin_btn, 0),
    PB_LAST_FIELD};
#endif