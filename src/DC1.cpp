#ifdef USE_DC1

#include "Debug.h"
#include "DC1.h"
#include "Mqtt.h"
#include "Ntp.h"
#include "Led.h"
#include "Wifi.h"

#pragma region 继承

void DC1::init()
{
    Led::init(LED_PIN, LOW);
    cat9554 = new CAT9554(CAT9554_SDA_PIN, CAT9554_SCL_PIN);
    cat9554->setIrqPin(CAT9554_IRQ_PIN);
    cat9554->setup();

    cse7766 = new CSE7766(CSE7766_RX_PIN, CSE7766_BAUDRATE);

    // 按键
    pinMode(KEY_0_PIN, INPUT_PULLDOWN_16);
    cat9554->pinMode(KEY_1_PIN, INPUT);
    cat9554->pinMode(KEY_2_PIN, INPUT);
    cat9554->pinMode(KEY_3_PIN, INPUT);

    // 继电器
    cat9554->pinMode(REL_0_PIN, OUTPUT);
    cat9554->pinMode(REL_1_PIN, OUTPUT);
    cat9554->pinMode(REL_2_PIN, OUTPUT);
    cat9554->pinMode(REL_3_PIN, OUTPUT);

    pinMode(LOGO_LED_PIN, OUTPUT);
    logoLed();

    channels = 4;
    for (uint8_t ch = 0; ch < channels; ch++)
    {
        // 0:开关通电时断开  1 : 开关通电时闭合  2 : 开关通电时状态与断电前相反  3 : 开关通电时保持断电前状态
        if (config.power_on_state == 2)
        {
            switchRelay(ch, !bitRead(config.last_state, ch), false); // 开关通电时状态与断电前相反
        }
        else if (config.power_on_state == 3)
        {
            switchRelay(ch, bitRead(config.last_state, ch), false); // 开关通电时保持断电前状态
        }
        else
        {
            switchRelay(ch, config.power_on_state == 1, false); // 开关通电时闭合
        }
        // 总开关关时跳过其他
        if (ch == 0 && !lastState[0])
        {
            break;
        }
    }
}

String DC1::getModuleName()
{
    return F("dc1");
}

String DC1::getModuleCNName()
{
    return F("DC1插线板");
}

bool DC1::moduleLed()
{
    if (WiFi.status() == WL_CONNECTED && mqtt->mqttClient.connected())
    {
        if (config.wifi_led == 0)
        {
            Led::on();
            return true;
        }
        else if (config.wifi_led == 1)
        {
            Led::off();
            return true;
        }
    }
    return false;
}

void DC1::loop()
{
    cse7766->loop();
    for (size_t ch = 0; ch < channels; ch++)
    {
        checkButton(ch);
    }

    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
        updataCSE7766();
    }
}

void DC1::perSecondDo()
{
    bitSet(operationFlag, 0);
}
#pragma endregion

#pragma region 配置

void DC1::readConfig()
{
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(DC1ConfigMessage), DC1ConfigMessage_fields, &config);
}

void DC1::resetConfig()
{
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(DC1ConfigMessage));

    config.power_on_state = 3;
    config.power_mode = 0;
    config.logo_led = 0;
    config.wifi_led = 0;
    config.sub_kinkage = 1;
    config.cse7766_interval = 60;
    config.voltage_delta = 3;
    config.current_delta = 0.5;
    config.power_delta = 5;
}

void DC1::saveConfig()
{
    Config::moduleSaveConfig(MODULE_CFG_VERSION, DC1ConfigMessage_size, DC1ConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void DC1::mqttCallback(String topicStr, String str)
{
    if (channels >= 1 && topicStr.endsWith("/POWER") || topicStr.endsWith("/POWER1"))
    {
        switchRelay(0, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[0])));
    }
    else if (channels >= 2 && topicStr.endsWith("/POWER2"))
    {
        switchRelay(1, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[1])));
    }
    else if (channels >= 3 && topicStr.endsWith("/POWER3"))
    {
        switchRelay(2, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[2])));
    }
    else if (channels >= 4 && topicStr.endsWith("/POWER4"))
    {
        switchRelay(3, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[3])));
    }
}

void DC1::mqttConnected()
{
    powerTopic = mqtt->getStatTopic(F("POWER"));
    if (globalConfig.mqtt.discovery)
    {
        mqttDiscovery(true);
        mqtt->doReport();
    }

    // 连接MQTT成功重新发送状态
    for (size_t ch = 0; ch < channels; ch++)
    {
        mqtt->publish((powerTopic + (ch + 1)), lastState[ch] ? "ON" : "OFF", globalConfig.mqtt.retain);
    }
}

void DC1::mqttDiscovery(boolean isEnable)
{
    char topic[50];
    char message[500];

    String tmp = mqtt->getCmndTopic(F("POWER"));
    for (size_t ch = 0; ch < channels; ch++)
    {
        sprintf(topic, "%s/switch/%s_%d/config", globalConfig.mqtt.discovery_prefix, UID, (ch + 1));
        if (isEnable)
        {
            sprintf(message, HASS_DISCOVER_DC1, UID, (ch + 1),
                    (tmp + (ch + 1)).c_str(),
                    (powerTopic + (ch + 1)).c_str(),
                    mqtt->getTeleTopic(F("availability")).c_str());
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("discovery: %s - %s"), topic, message);
            mqtt->publish(topic, message, true);
        }
        else
        {
            mqtt->publish(topic, "", true);
        }
    }

    String tims[] = {"voltage", "current", "power"};
    String tims2[] = {"V", "A", "W"};
    for (size_t i = 0; i < 3; i++)
    {
        sprintf(topic, "%s/sensor/%s_%s/config", globalConfig.mqtt.discovery_prefix, UID, tims[i].c_str());
        if (isEnable)
        {
            sprintf(message, "{\"name\":\"%s_%s\","
                             "\"stat_t\":\"%s\","
                             "\"unit_of_meas\":\"%s\","
                             "\"val_tpl\":\"{{value_json.%s}}\","
                             "\"avty_t\":\"%s\","
                             "\"pl_avail\":\"online\","
                             "\"pl_not_avail\":\"offline\"}",
                    UID, tims[i].c_str(),
                    mqtt->getTeleTopic("ENERGY").c_str(),
                    tims2[i].c_str(),
                    tims[i].c_str(),
                    mqtt->getTeleTopic(F("availability")).c_str());
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("discovery: %s - %s"), topic, message);
            mqtt->publish(topic, message, true);
        }
        else
        {
            mqtt->publish(topic, "", true);
        }
    }
}
#pragma endregion

#pragma region Http

void DC1::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/dc1_do"), std::bind(&DC1::httpDo, this, server));
    server->on(F("/dc1_setting"), std::bind(&DC1::httpSetting, this, server));
}

String DC1::httpGetStatus(ESP8266WebServer *server)
{
    String data;
    for (size_t ch = 0; ch < channels; ch++)
    {
        data += ",\"relay_" + String(ch + 1) + "\":";
        data += lastState[ch] ? 1 : 0;
    }
    data += F(",\"voltage\":\"");
    data += cse7766->voltage;
    data += F("\",\"current\":\"");
    data += cse7766->current;
    data += F("\",\"power\":\"");
    data += cse7766->power;
    data += F("\",\"factor\":\"");
    data += cse7766->factor;
    data += F("\"");
    return data.substring(1);
}

void DC1::httpHtml(ESP8266WebServer *server)
{
    String radioJs = F("<script type='text/javascript'>");
    radioJs += F("function setDataSub(data,key){if(key.substr(0,5)=='relay'){var t=id(key);var v=data[key];t.setAttribute('class',v==1?'btn-success':'btn-info');t.innerHTML=v==1?'开':'关';return true}return false}");
    String page = F("<table class='gridtable'><thead><tr><th colspan='2'>开关状态</th></tr></thead><tbody>");
    page += F("<tr colspan='2' style='text-align:center'><td>");
    for (size_t ch = 0; ch < channels; ch++)
    {
        page += F(" <button type='button' style='width:50px' onclick=\"ajaxPost('/dc1_do', 'do=T&c={ch}');\" id='relay_{ch}' ");
        page.replace(F("{ch}"), String(ch + 1));
        if (lastState[ch])
        {
            page += F("class='btn-success'>开</button>");
        }
        else
        {
            page += F("class='btn-info'>关</button>");
        }
    }
    page += F("<br><br>电压：<span id='voltage'>{voltage}</span>V<br>电流：<span id='current'>{current}</span>A<br>功率：<span id='power'>{power}</span>W<br>功率因数：<span id='factor'>{factor}</span>");
    page += F("</td></tr></tbody></table>");
    page.replace(F("{voltage}"), String(cse7766->voltage));
    page.replace(F("{current}"), String(cse7766->current));
    page.replace(F("{power}"), String(cse7766->power));
    page.replace(F("{factor}"), String(cse7766->factor));

    page += F("<form method='post' action='/dc1_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>DC1插线板设置</th></tr></thead><tbody>");
    page += F("<tr><td>上电状态</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='0'/><i class='bui-radios'></i> 开关通电时断开</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='1'/><i class='bui-radios'></i> 开关通电时闭合</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='2'/><i class='bui-radios'></i> 开关通电时状态与断电前相反</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='3'/><i class='bui-radios'></i> 开关通电时保持断电前状态</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_on_state', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_on_state));

    page += F("<tr><td>开关模式</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_mode' value='0'/><i class='bui-radios'></i> 自锁</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_mode' value='1'/><i class='bui-radios'></i> 互锁</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_mode', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_mode));

    page += F("<tr><td>LOGO LED</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='0'/><i class='bui-radios'></i> 常亮</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='1'/><i class='bui-radios'></i> 常灭</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='2'/><i class='bui-radios'></i> 跟随总开关</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='3'/><i class='bui-radios'></i> 与总开关相反</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('logo_led', '{v}');");
    radioJs.replace(F("{v}"), String(config.logo_led));

    page += F("<tr><td>WIFI LED</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='wifi_led' value='0'/><i class='bui-radios'></i> 常亮</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='wifi_led' value='1'/><i class='bui-radios'></i> 常灭</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='wifi_led' value='2'/><i class='bui-radios'></i> 闪烁</label><br>未连接WIFI或者MQTT时为快闪");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('wifi_led', '{v}');");
    radioJs.replace(F("{v}"), String(config.wifi_led));

    page += F("<tr><td>分开关</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='sub_kinkage' value='0'/><i class='bui-radios'></i> 总开关关闭时禁开</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='sub_kinkage' value='1'/><i class='bui-radios'></i> 同时打开总开关</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('sub_kinkage', '{v}');");
    radioJs.replace(F("{v}"), String(config.sub_kinkage));

    page += F("<tr><td>电力上传间隔</td><td><input type='number' min='1' max='65535' name='cse7766_interval' required value='{v}'>&nbsp;秒</td></tr>");
    page.replace(F("{v}"), String(config.cse7766_interval));

    page += F("<tr><td>电压波动</td><td><input type='number' min='1' step='0.1' max='50' name='voltage_delta' required value='{v}'>&nbsp;V<br>数据波动大于设置的值将会上报一次，下同</td></tr>");
    page.replace(F("{v}"), String(config.voltage_delta));
    page += F("<tr><td>电流波动</td><td><input type='number' min='0.01' step='0.01' max='10' name='current_delta' required value='{v}'>&nbsp;A</td></tr>");
    page.replace(F("{v}"), String(config.current_delta));
    page += F("<tr><td>功率波动</td><td><input type='number' min='1' step='0.1' max='250' name='power_delta' required value='{v}'>&nbsp;W</td></tr>");
    page.replace(F("{v}"), String(config.power_delta));

    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button></td></tr>");
    page += F("</tbody></table></form>");
    radioJs += F("</script>");

    server->sendContent(page);
    server->sendContent(radioJs);
}

void DC1::httpDo(ESP8266WebServer *server)
{
    String c = server->arg(F("c"));
    if (c != F("1") && c != F("2") && c != F("3") && c != F("4"))
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"参数错误。\"}"));
        return;
    }
    uint8_t ch = c.toInt() - 1;
    if (ch > channels)
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"继电器数量错误。\"}"));
        return;
    }
    String str = server->arg(F("do"));
    switchRelay(ch, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[ch])));

    server->send(200, F("text/html"), "{\"code\":1,\"msg\":\"操作成功\",\"data\":{" + httpGetStatus(server) + "}}");
}

void DC1::httpSetting(ESP8266WebServer *server)
{
    config.power_on_state = server->arg(F("power_on_state")).toInt();
    config.power_mode = server->arg(F("power_mode")).toInt();
    config.logo_led = server->arg(F("logo_led")).toInt();
    config.wifi_led = server->arg(F("wifi_led")).toInt();
    config.sub_kinkage = server->arg(F("sub_kinkage")).toInt();

    config.cse7766_interval = server->arg(F("cse7766_interval")).toInt();
    config.voltage_delta = server->arg(F("voltage_delta")).toFloat();
    config.current_delta = server->arg(F("current_delta")).toFloat();
    config.power_delta = server->arg(F("power_delta")).toFloat();

    logoLed();

    Config::saveConfig();
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置成功。\"}"));
}
#pragma endregion

void DC1::logoLed()
{
    if (config.logo_led == 0)
    {
        digitalWrite(LOGO_LED_PIN, LOW);
    }
    else if (config.logo_led == 1)
    {
        digitalWrite(LOGO_LED_PIN, HIGH);
    }
    else if (config.logo_led == 2)
    {
        digitalWrite(LOGO_LED_PIN, lastState[0] ? LOW : HIGH);
    }
    else if (config.logo_led == 3)
    {
        digitalWrite(LOGO_LED_PIN, lastState[0] ? HIGH : LOW);
    }
}

void DC1::switchRelay(uint8_t ch, bool isOn, bool isSave)
{
    if (ch > channels)
    {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("invalid channel: %d"), ch);
        return;
    }
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Relay %d . . . %s"), ch + 1, isOn ? "ON" : "OFF");

    if (ch > 0)
    {
        if (!lastState[0] && isOn)
        {
            if (config.sub_kinkage == 0 || !isSave)
            {
                isOn = false;
            }
            else if (config.sub_kinkage == 1)
            {
                switchRelay(0, true);
            }
        }

        if (isOn && config.power_mode == 1)
        {
            for (size_t ch2 = 1; ch2 < channels; ch2++)
            {
                if (ch2 != ch && lastState[ch2])
                {
                    switchRelay(ch2, false, isSave);
                }
            }
        }
    }

    if (!cat9554->digitalWrite(relGPIO[ch], isOn ? HIGH : LOW))
    {
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("CAT9554 digitalWrite Error"));
        if (!cat9554->digitalWrite(relGPIO[ch], isOn ? HIGH : LOW))
        {
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("CAT9554 digitalWrite Error2"));
            return;
        }
    }
    lastState[ch] = isOn;

    mqtt->publish((powerTopic + (ch + 1)), isOn ? "ON" : "OFF", globalConfig.mqtt.retain);

    if (isSave && config.power_on_state > 0)
    {
        bitWrite(config.last_state, ch, isOn);
    }

    if (ch == 0)
    {
        logoLed();
        if (isSave)
        {
            for (size_t ch2 = 1; ch2 < channels; ch2++)
            {
                if (isOn)
                {
                    if (bitRead(config.last_state, ch2))
                    {
                        switchRelay(ch2, true, false);
                    }
                }
                else
                {
                    switchRelay(ch2, false, false);
                }
            }
        }
    }
}

void DC1::updataCSE7766()
{
    bool send = (perSecond % config.cse7766_interval) == 0;
    cse7766->update();

    if (send || fabs(cse7766->voltage - lastVoltage) > config.voltage_delta || fabs(cse7766->current - lastCurrent) > config.current_delta || fabs(cse7766->power - lastPower) > config.power_delta)
    {
        lastVoltage = cse7766->voltage;
        lastCurrent = cse7766->current;
        lastPower = cse7766->power;

        sprintf(tmpData, "{\"voltage\":%s,\"current\":%s,\"power\":%s,\"factor\":%s}", String(lastVoltage).c_str(), String(lastCurrent).c_str(), String(lastPower).c_str(), String(cse7766->factor).c_str());
        mqtt->publish(mqtt->getTeleTopic("ENERGY"), tmpData, globalConfig.mqtt.retain);
    }
}

void DC1::checkButton(uint8_t ch)
{
    boolean buttonState = ch == 0 ? digitalRead(btnGPIO[ch]) : cat9554->digitalRead(btnGPIO[ch]);

    if (buttonState == 0)
    { // 按下按钮
        if (buttonTiming[ch] == false)
        {
            buttonTiming[ch] = true;
            buttonTimingStart[ch] = millis();
        }
        else
        { // buttonTiming = true
            if (millis() >= (buttonTimingStart[ch] + buttonDebounceTime))
            {
                buttonAction[ch] = 1;
            }
            if (millis() >= (buttonTimingStart[ch] + buttonLongPressTime))
            {
                buttonAction[ch] = 2;
            }
        }
    }
    else
    { // buttonState == 1, 释放按钮
        buttonTiming[ch] = false;
        if (buttonAction[ch] != 0)
        {
            if (buttonAction[ch] == 1) // 执行短按动作
            {
                switchRelay(ch, !lastState[ch], true);
            }
            else if (buttonAction[ch] == 2) // 执行长按动作
            {
                if (ch == 0)
                {
                    Wifi::setupWifiManager(false);
                }
            }
            buttonAction[ch] = 0;
        }
    }
}

const pb_field_t DC1ConfigMessage_fields[11] = {
    PB_FIELD(1, UINT32, SINGULAR, STATIC, FIRST, DC1ConfigMessage, last_state, last_state, 0),
    PB_FIELD(2, UINT32, SINGULAR, STATIC, OTHER, DC1ConfigMessage, power_on_state, last_state, 0),
    PB_FIELD(3, UINT32, SINGULAR, STATIC, OTHER, DC1ConfigMessage, power_mode, power_on_state, 0),
    PB_FIELD(4, UINT32, SINGULAR, STATIC, OTHER, DC1ConfigMessage, logo_led, power_mode, 0),
    PB_FIELD(5, UINT32, SINGULAR, STATIC, OTHER, DC1ConfigMessage, wifi_led, logo_led, 0),
    PB_FIELD(6, UINT32, SINGULAR, STATIC, OTHER, DC1ConfigMessage, cse7766_interval, wifi_led, 0),
    PB_FIELD(7, FLOAT, SINGULAR, STATIC, OTHER, DC1ConfigMessage, voltage_delta, cse7766_interval, 0),
    PB_FIELD(8, FLOAT, SINGULAR, STATIC, OTHER, DC1ConfigMessage, current_delta, voltage_delta, 0),
    PB_FIELD(9, FLOAT, SINGULAR, STATIC, OTHER, DC1ConfigMessage, power_delta, current_delta, 0),
    PB_FIELD(11, UINT32, SINGULAR, STATIC, OTHER, DC1ConfigMessage, sub_kinkage, power_delta, 0),
    PB_LAST_FIELD};

#endif