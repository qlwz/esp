#ifdef USE_WEILE

#include "Weile.h"

#pragma region 继承

void Weile::init()
{
    if (config.pin_led != 99)
    {
        Led::init(config.pin_led > 30 ? config.pin_led - 30 : config.pin_led, config.pin_led > 30 ? HIGH : LOW);
    }
    pinMode(config.pin_rel, OUTPUT); // 继电器
}

void Weile::loop()
{
    if (weiLeStatus)
    {
        if (millis() >= weileTime + (config.weile_time * 1000))
        {
            weiLeStatus = false;
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("Weile close . . ."));
            mqtt->publish(powerTopic, "OFF", globalConfig.mqtt.retain);
        }
    }
    if (screenStatus)
    {
        if (millis() >= screenTime + (config.screen_time * 1000))
        {
            screenStatus = false;
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("screen close . . ."));
        }
    }
    checkButton();
}

void Weile::perSecondDo()
{
}

void Weile::checkButton()
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

void Weile::readConfig()
{
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(WeileConfigMessage), WeileConfigMessage_fields, &config);
}

void Weile::resetConfig()
{
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(WeileConfigMessage));
    config.jog_time = 200;
    config.start_interval = 1000;
    config.weile_time = 120;
    config.screen_time = 15;
    config.pin_rel = 14;
    config.pin_led = 16;
}

void Weile::saveConfig()
{
    Config::moduleSaveConfig(MODULE_CFG_VERSION, WeileConfigMessage_size, WeileConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void Weile::mqttCallback(String topicStr, String str)
{
    if (topicStr.endsWith("/POWER"))
    {
        if (str == "ON")
        {
            weileOpen();
        }
        else if (str == "OFF")
        {
            weileClose();
        }
    }
    else if (topicStr.endsWith("/GL"))
    {
        if (str.toInt() < config.close_power)
        {
            if (weiLeStatus && millis() - 1000 > weileTime)
            {
                weiLeStatus = false;
            }
        }
        else
        {
            weiLeStatus = true;
        }
    }
}

void Weile::mqttDiscovery(boolean isEnable)
{
    char topic[50];

    String tmp = mqtt->getCmndTopic(F("POWER"));

    sprintf(topic, "%s/switch/%s/config", globalConfig.mqtt.discovery_prefix, UID);
    if (isEnable)
    {
        char message[500];
        sprintf(message, HASS_DISCOVER_WEILE, UID,
                tmp.c_str(),
                powerTopic.c_str(),
                mqtt->getTeleTopic(F("availability")).c_str());
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("discovery: %s - %s"), topic, message);
        mqtt->publish(topic, message, true);
    }
    else
    {
        mqtt->publish(topic, "", true);
    }
}

void Weile::mqttConnected()
{
    powerTopic = mqtt->getStatTopic(F("POWER"));
    if (globalConfig.mqtt.discovery)
    {
        mqttDiscovery(true);
        mqtt->availability();
    }
}
#pragma endregion

#pragma region HTTP

void Weile::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/weile_do"), std::bind(&Weile::httpDo, this, server));
    server->on(F("/weile_setting"), std::bind(&Weile::httpSetting, this, server));
}

String Weile::httpGetStatus(ESP8266WebServer *server)
{
    return "";
}

void Weile::httpHtml(ESP8266WebServer *server)
{
    String radioJs = F("<script type='text/javascript'>");
    radioJs += F("var iscover=0;function setDataSub(data,key){if(key=='cover_position'){var t=id(key);var v=data[key];if(iscover>0&&v==t.value&&iscover++>5){iscover=0;intervalTime=defIntervalTime}t.value=v;t.nextSibling.nextSibling.innerHTML=v+'%';id('cover_open').disabled=v==100;id('cover_close').disabled=v==0;return true}return false}");

    String page = F("<table class='gridtable'><thead><tr><th colspan='2'>威乐回水器</th></tr></thead><tbody>");
    page += F("<tr><td>操作</td><td><button type='button' class='btn-success' style='width:50px' id='cover_open' onclick=\"ajaxPost('/weile_do', 'do=OPEN');\">开</button></td></tr>");
    page += F("</tbody></table>");

    page += F("<form method='post' action='/weile_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>威乐回水器设置</th></tr></thead><tbody>");

    page += F("<tr><td>自锁时间</td><td><input type='number' name='jog_time' required value='{jog_time}'>&nbsp;&nbsp;毫秒</td></tr>");
    page.replace(F("{jog_time}"), String(config.jog_time));

    page += F("<tr><td>开机间隔</td><td><input type='number' name='start_interval' required value='{start_interval}'>&nbsp;&nbsp;毫秒</td></tr>");
    page.replace(F("{start_interval}"), String(config.start_interval));

    page += F("<tr><td>回水时间</td><td><input type='number' name='weile_time' required value='{weile_time}'>&nbsp;&nbsp;秒</td></tr>");
    page.replace(F("{weile_time}"), String(config.weile_time));

    page += F("<tr><td>关屏时间</td><td><input type='number' name='screen_time' required value='{screen_time}'>&nbsp;&nbsp;秒</td></tr>");
    page.replace(F("{screen_time}"), String(config.screen_time));

    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button></td></tr>");
    page += F("</tbody></table></form>");
    radioJs += F("</script>");

    server->sendContent(page);
    server->sendContent(radioJs);
}

void Weile::httpDo(ESP8266WebServer *server)
{
    String str = server->arg(F("do"));
    weileOpen();
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"操作成功\"}"));
}

void Weile::httpSetting(ESP8266WebServer *server)
{
    config.jog_time = server->arg(F("jog_time")).toInt();
    config.start_interval = server->arg(F("start_interval")).toInt();
    config.weile_time = server->arg(F("weile_time")).toInt();
    config.screen_time = server->arg(F("screen_time")).toInt();

    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置。\"}"));
}
#pragma endregion

void Weile::weileOpen()
{
    if (weiLeStatus)
    {
        return;
    }
    Debug.AddLog(LOG_LEVEL_INFO, "Open . . .");
    openScreen(true);
    weiLeStatus = true;
    weileTime = millis();
    Debug.AddLog(LOG_LEVEL_INFO, "Open Weile");
    pressBtn();

    mqtt->publish(powerTopic, "ON", globalConfig.mqtt.retain);
}

void Weile::weileClose()
{
    if (!weiLeStatus)
    {
        return;
    }
    Debug.AddLog(LOG_LEVEL_INFO, "Close . . .");
    openScreen(true);
    Debug.AddLog(LOG_LEVEL_INFO, "Close Weile");
    pressBtn();

    mqtt->publish(powerTopic, "OFF", globalConfig.mqtt.retain);
}

/**
 * 亮屏
 */
void Weile::openScreen(boolean isInterval)
{
    if (!screenStatus) // 屏幕关屏
    {
        Debug.AddLog(LOG_LEVEL_INFO, "Open Screen");
        pressBtn();
        screenStatus = true;
        screenTime = millis();
        if (isInterval)
        {
            delay(config.start_interval);
        }
    }
}

/**
 * 按一下按钮
 */
void Weile::pressBtn()
{
    digitalWrite(config.pin_rel, HIGH);
    delay(config.jog_time);
    digitalWrite(config.pin_rel, LOW);
    if (screenStatus) // 屏幕关屏
    {
        screenTime = millis();
    }
}

const pb_field_t WeileConfigMessage_fields[9] = {
    PB_FIELD(1, UINT32, SINGULAR, STATIC, FIRST, WeileConfigMessage, jog_time, jog_time, 0),
    PB_FIELD(2, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, start_interval, jog_time, 0),
    PB_FIELD(3, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, weile_time, start_interval, 0),
    PB_FIELD(4, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, screen_time, weile_time, 0),
    PB_FIELD(5, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, close_power, screen_time, 0),
    PB_FIELD(20, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, pin_rel, close_power, 0),
    PB_FIELD(21, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, pin_led, pin_rel, 0),
    PB_FIELD(22, UINT32, SINGULAR, STATIC, OTHER, WeileConfigMessage, pin_btn, pin_led, 0),
    PB_LAST_FIELD};
#endif