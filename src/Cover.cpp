#ifdef USE_COVER

#include <DOOYACommand.h>
#include "Debug.h"
#include "Led.h"
#include "Cover.h"
#include "Mqtt.h"
#include "Wifi.h"

#pragma region 继承

void Cover::init()
{
    if (config.weak_switch == 126)
    {
        config.weak_switch = 127;
    }
    if (config.power_switch == 126)
    {
        config.power_switch = 127;
    }
    softwareSerial = new SoftwareSerial(config.pin_rx, config.pin_tx); // RX, TX
    softwareSerial->begin(9600);
    if (config.pin_led != 99)
    {
        Led::init(config.pin_led > 30 ? config.pin_led - 30 : config.pin_led, config.pin_led > 30 ? HIGH : LOW);
    }
}

String Cover::getModuleName()
{
    return F("cover");
}

String Cover::getModuleCNName()
{
    return F("杜亚窗帘");
}

bool Cover::moduleLed()
{
    return false;
}

void Cover::loop()
{
    readSoftwareSerialTick();
    checkButton();
}

void Cover::perSecondDo()
{
    if (getPositionState || perSecond == 5 || perSecond % 60 == 0)
    {
        getPositionTask();
    }
}

void Cover::checkButton()
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

void Cover::readConfig()
{
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(CoverConfigMessage), CoverConfigMessage_fields, &config);
}

void Cover::resetConfig()
{
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(CoverConfigMessage));
    config.position = 127;
    config.direction = 127;
    config.hand_pull = 127;
    config.weak_switch = 127;
    config.power_switch = 127;

    config.pin_rx = 14;
    config.pin_tx = 12;
    config.pin_led = 4;
    config.pin_btn = 13;
}

void Cover::saveConfig()
{
    Config::moduleSaveConfig(MODULE_CFG_VERSION, CoverConfigMessage_size, CoverConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void Cover::mqttCallback(String topicStr, String str)
{
    uint8_t tmp[10];
    uint8_t len = 0;
    if (topicStr.endsWith(F("/set_position")))
    {
        uint8_t n = getInt(str, 0, 100);
        len = DOOYACommand::setPosition(tmp, 0xFEFE, 0, n);
        getPositionState = true;
    }
    else if (topicStr.endsWith(F("/get_position")))
    {
        len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
    }
    else if (topicStr.endsWith(F("/set")))
    {
        if (str.equals(F("OPEN")))
        {
            len = DOOYACommand::open(tmp, 0xFEFE, 0);
            getPositionState = true;
        }
        else if (str.equals(F("CLOSE")))
        {
            len = DOOYACommand::close(tmp, 0xFEFE, 0);
            getPositionState = true;
        }
        else if (str.equals(F("STOP")))
        {
            len = DOOYACommand::stop(tmp, 0xFEFE, 0);
            getPositionState = false;
        }
        else if (str.equals(F("GetPosition")))
        {
            len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
        }
    }
    else if (topicStr.endsWith(F("/get")))
    {
        if (str == F("Position"))
        {
            len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
        }
        else if (str == F("Direction"))
        {
            len = DOOYACommand::getDirectionStatus(tmp, 0xFEFE, 0);
        }
        else if (str == F("HandPull"))
        {
            len = DOOYACommand::getHandPullStatus(tmp, 0xFEFE, 0);
        }
        else if (str == F("Motor"))
        {
            len = DOOYACommand::getMotorStatus(tmp, 0xFEFE, 0);
        }
        else if (str == F("WeakSwitchType"))
        {
            len = DOOYACommand::getWeakSwitchType(tmp, 0xFEFE, 0);
        }
        else if (str == F("PowerSwitchType"))
        {
            len = DOOYACommand::getPowerSwitchType(tmp, 0xFEFE, 0);
        }
        else if (str == F("ProtocolVersion"))
        {
            len = DOOYACommand::getProtocolVersion(tmp, 0xFEFE, 0);
        }
    }
    else if (topicStr.endsWith(F("/delete_trip")))
    {
        len = DOOYACommand::deleteTrip(tmp, 0xFEFE, 0);
    }
    else if (topicStr.endsWith(F("/reset")))
    {
        len = DOOYACommand::reset(tmp, 0xFEFE, 0);
    }
    else if (topicStr.endsWith(F("/set_scene")))
    {
        uint8_t n = getInt(str, 1, 254);
        len = DOOYACommand::setScene(tmp, 0xFEFE, 0, n);
    }
    else if (topicStr.endsWith(F("/run_scene")))
    {
        uint8_t n = getInt(str, 1, 254);
        len = DOOYACommand::runScene(tmp, 0xFEFE, 0, n);
    }
    else if (topicStr.endsWith(F("/delete_scene")))
    {
        uint8_t n = getInt(str, 1, 254);
        len = DOOYACommand::deleteScene(tmp, 0xFEFE, 0, n);
    }
    else if (topicStr.endsWith(F("/open_close")))
    {
        len = DOOYACommand::openOrClose(tmp, 0xFEFE, 0);
    }
    if (len > 0)
    {
        softwareSerial->write(tmp, len);
    }
}

void Cover::mqttDiscovery(boolean isEnable)
{
    if (!mqtt)
    {
        return;
    }
    char topic[50];
    sprintf(topic, "%s/cover/%s/config", globalConfig.mqtt.discovery_prefix, UID);
    if (isEnable)
    {
        char message[500];
        sprintf(message, HASS_DISCOVER_COVER, UID, mqtt->getCmndTopic(F("set")).c_str(), mqtt->getStatTopic(F("position")).c_str(),
                mqtt->getCmndTopic(F("set_position")).c_str(), mqtt->getTeleTopic(F("availability")).c_str());
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("discovery: %s - %s"), topic, message);
        mqtt->publish(topic, message);
    }
    else
    {
        mqtt->publish(topic, "");
    }
}

void Cover::mqttConnected()
{
    if (globalConfig.mqtt.discovery)
    {
        mqttDiscovery(true);
        mqtt->doReport();
    }
    getPositionTask();
}
#pragma endregion

#pragma region HTTP

void Cover::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/cover_position"), std::bind(&Cover::httpPosition, this, server));
    server->on(F("/cover_set"), std::bind(&Cover::httpDo, this, server));
    server->on(F("/cover_setting"), std::bind(&Cover::httpSetting, this, server));
    server->on(F("/cover_reset"), std::bind(&Cover::httpReset, this, server));
}

String Cover::httpGetStatus(ESP8266WebServer *server)
{
    String data = F("\"cover_position\":");
    data += config.position;
    return data;
}

void Cover::httpHtml(ESP8266WebServer *server)
{
    String radioJs = F("<script type='text/javascript'>");
    radioJs += F("var iscover=0;function setDataSub(data,key){if(key=='cover_position'){var t=id(key);var v=data[key];if(iscover>0&&v==t.value&&iscover++>5){iscover=0;intervalTime=defIntervalTime}t.value=v;t.nextSibling.nextSibling.innerHTML=v+'%';id('cover_open').disabled=v==100;id('cover_close').disabled=v==0;return true}return false}");

    String page = F("<table class='gridtable'><thead><tr><th colspan='2'>控制窗帘</th></tr></thead><tbody>");
    page += F("<tr><td>操作</td><td><button type='button' class='btn-success' style='width:50px' id='cover_open' onclick=\"coverSet('OPEN')\">开</button> ");
    page += F("<button type='button' class='btn-success' style='width:50px' onclick=\"coverSet('STOP')\">停</button> ");
    page += F("<button type='button' class='btn-success' style='width:50px' id='cover_close' onclick=\"coverSet('CLOSE')\">关</button></td></tr>");
    page += F("<tr><td>当前位置</td><td><input type='range' min='0' max='100' id='cover_position' name='position' value='{v}' onchange='rangOnChange(this)'/>&nbsp;<span>{v}%</span></td></tr>");
    page += F("</tbody></table>");
    page += F("<script type='text/javascript'>function coverSet(t){ajaxPost('/cover_set','do='+t);iscover=1;intervalTime=1000}function rangOnChange(the){the.nextSibling.nextSibling.innerHTML=the.value+'%';ajaxPost('/cover_position', 'position=' + the.value);iscover=1;intervalTime=1000}</script>");
    page.replace(F("{v}"), String(config.position));

    page += F("<form method='post' action='/cover_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>窗帘设置</th></tr></thead><tbody>");
    page += F("<tr><td>电机方向</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='direction' value='0'/><i class='bui-radios'></i> 正向</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='direction' value='1'/><i class='bui-radios'></i> 反向</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('direction', '{v}');");
    radioJs.replace(F("{v}"), String(config.direction));

    page += F("<tr><td>手拉使能</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='hand_pull' value='0'/><i class='bui-radios'></i> 开启</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='hand_pull' value='1'/><i class='bui-radios'></i> 关闭</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('hand_pull', '{v}');");
    radioJs.replace(F("{v}"), String(config.hand_pull));

    page += F("<tr><td>弱点开关</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='weak_switch' value='1'/><i class='bui-radios'></i> 双反弹开关</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='weak_switch' value='2'/><i class='bui-radios'></i> 双不反弹开关</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='weak_switch' value='3'/><i class='bui-radios'></i> DC246电子开关</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='weak_switch' value='4'/><i class='bui-radios'></i> 单键循环开关</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('weak_switch', '{v}');");
    radioJs.replace(F("{v}"), String(config.weak_switch));

    page += F("<tr><td>强电开关</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_switch' value='0'/><i class='bui-radios'></i> 强电双键不反弹模式</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_switch' value='1'/><i class='bui-radios'></i> 酒店模式</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_switch' value='2'/><i class='bui-radios'></i> 强电双键可反弹模式</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_switch', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_switch));

    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button><br>");
    page += F("<button type='button' class='btn-danger' style='margin-top: 10px' onclick=\"javascript:if(confirm('确定要电机恢复出厂设置？')){ajaxPost('/cover_reset');}\">电机恢复出厂</button>");
    page += F("</td></tr>");
    page += F("</tbody></table></form>");
    radioJs += F("</script>");

    server->sendContent(page);
    server->sendContent(radioJs);
}

uint8_t Cover::getInt(String str, uint8_t min, uint8_t max)
{
    uint8_t n = str.toInt();
    if (n > max)
        n = max;
    if (n < min)
        n = min;
    return n;
}

void Cover::httpPosition(ESP8266WebServer *server)
{
    String position = server->arg(F("position"));
    uint8_t n = getInt(position, 0, 100);
    uint8_t tmp[10];
    int len = DOOYACommand::setPosition(tmp, 0xFEFE, 0, n);
    softwareSerial->write(tmp, len);
    delay(100);
    getPositionState = true;
    server->send(200, F("text/html"), "{\"code\":1,\"msg\":\"已设置窗帘位置。\",\"data\":{\"position\":" + position + "}}");
}

void Cover::httpDo(ESP8266WebServer *server)
{
    String str = server->arg(F("do"));
    uint8_t tmp[10];
    int len;
    if (str.equals(F("OPEN")))
    {
        len = DOOYACommand::open(tmp, 0xFEFE, 0);
        getPositionState = true;
    }
    else if (str.equals(F("CLOSE")))
    {
        len = DOOYACommand::close(tmp, 0xFEFE, 0);
        getPositionState = true;
    }
    else if (str.equals(F("STOP")))
    {
        len = DOOYACommand::stop(tmp, 0xFEFE, 0);
        getPositionState = false;
    }
    else
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"没有该操作。\"}"));
        return;
    }
    softwareSerial->write(tmp, len);
    delay(100);
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已执行窗帘操作。\"}"));
}

void Cover::httpSetting(ESP8266WebServer *server)
{
    uint8_t tmp[10];
    int len;
    if (server->hasArg(F("direction")))
    {
        String direction = server->arg(F("direction"));
        len = DOOYACommand::setDirection(tmp, 0xFEFE, 0, !direction.equals(F("1")));
        softwareSerial->write(tmp, len);
        delay(100);
    }

    if (server->hasArg(F("hand_pull")))
    {
        String handPull = server->arg(F("hand_pull"));
        len = DOOYACommand::setHandPull(tmp, 0xFEFE, 0, !handPull.equals(F("1")));
        softwareSerial->write(tmp, len);
        delay(100);
    }

    if (server->hasArg(F("weak_switch")))
    {
        config.weak_switch == 127;
        String weakSwitch = server->arg(F("weak_switch"));
        len = DOOYACommand::setWeakSwitchType(tmp, 0xFEFE, 0, weakSwitch.equals(F("2")) ? 0x02 : (weakSwitch.equals(F("3")) ? 0x03 : (weakSwitch.equals(F("4")) ? 0x04 : 0x01)));
        softwareSerial->write(tmp, len);
        delay(100);
    }

    if (server->hasArg(F("power_switch")))
    {
        config.power_switch == 127;
        String powerSwitch = server->arg(F("power_switch"));
        len = DOOYACommand::setPowerSwitchType(tmp, 0xFEFE, 0, powerSwitch.equals(F("1")) ? 0x01 : (powerSwitch.equals(F("2")) ? 0x02 : 0x00));
        softwareSerial->write(tmp, len);
    }

    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置。\"}"));
}

void Cover::httpReset(ESP8266WebServer *server)
{
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"正在重置电机 . . . 设备将会重启。\"}"));
    delay(200);

    Led::blinkLED(400, 4);

    uint8_t tmp[10];
    int len;
    DOOYACommand::reset(tmp, 0xFEFE, 0);
    softwareSerial->write(tmp, len);
    delay(100);
    config.position = 127;
    config.direction = 127;
    config.hand_pull = 127;
    config.weak_switch = 127;
    config.power_switch = 127;
    Config::saveConfig();
    ESP.restart();
}
#pragma endregion

void Cover::doPosition(uint8_t position, uint8_t command)
{
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("Position: %d"), position);
    if (position == 255)
    {
        // 自动设置行程
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("AutoStroke: %d"), autoStroke ? 1 : 0);
        if (!autoStroke)
        {
            autoStroke = true;
            delay(100);
            uint8_t tmp[10];
            uint8_t len2 = DOOYACommand::open(tmp, 0xFEFE, 0);
            softwareSerial->write(tmp, len2);
            getPositionState = true;
            if (config.position > 127)
            {
                config.position = 100;
            }
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("Start Auto Stroke: Last %d"), config.position);
        }
        return;
    }

    if (autoStroke && position == 100) // 设置完行程回到原来的位置
    {
        config.weak_switch = 127;
        config.power_switch = 127;
        autoStroke = false;
        delay(100);
        Debug.AddLog(LOG_LEVEL_INFO, PSTR("Stop Auto Stroke: Last %d"), config.position);
        uint8_t tmp[10];
        uint8_t len2 = DOOYACommand::setPosition(tmp, 0xFEFE, 0, config.position);
        softwareSerial->write(tmp, len2);
        getPositionState = true;
    }
    else if (config.position != position)
    {
        config.position = position;
        if (mqtt)
        {
            String topic = mqtt->getStatTopic(F("position"));
            mqtt->publish(topic, String(config.position).c_str());
        }
    }
}

void Cover::doSoftwareSerialTick(uint8_t *buf, int len)
{
    DOOYACommand::Command command = DOOYACommand::parserReplyCommand(buf, len);

    char strHex[50];
    DOOYACommand::hex2Str(buf, len, strHex, true);
    Debug.AddLog(LOG_LEVEL_INFO, PSTR("%s: %s"), (command.command == 0x01 ? "Read" : (command.command == 0x02 ? "Write" : (command.command == 0x03 ? "Control" : (command.command == 0x04 ? "Request" : "??")))), strHex);

    if (command.command == 0x01)
    {
        String topic;
        if (command.address == 0x02)
        {
            if (getPositionState && command.data[0] == 0 || command.data[0] == 100)
            {
                getPositionState = false;
            }
            doPosition(command.data[0], command.command);
            return;
        }

        if (!mqtt)
        {
            return;
        }
        switch (command.address)
        {
        case 0x02:
            break;
        case 0x03:
            topic = mqtt->getStatTopic(F("direction"));
            break;
        case 0x04:
            topic = mqtt->getStatTopic(F("hand_pull"));
            break;
        case 0x05:
            topic = mqtt->getStatTopic(F("motor"));
            break;
        case 0x27:
            config.weak_switch = command.data[0];
            topic = mqtt->getStatTopic(F("weak_switch_type"));
            break;
        case 0x28:
            config.power_switch = command.data[0];
            topic = mqtt->getStatTopic(F("power_switch_type"));
            break;
        case 0xFE:
            topic = mqtt->getStatTopic(F("protocol_version"));
            break;
        default:
            break;
        }
        if (topic.length() > 0)
        {
            mqtt->publish(topic.c_str(), command.data, command.dataLen);
        }
    }
    else if (command.command == 0x02)
    {
    }
    else if (command.command == 0x03)
    {
    }
    else if (command.command == 0x04)
    {
        if (command.address == 0x02 && command.dataLen == 8)
        {
            // 0 : 百分比
            // 1 ：电机方向 0默认 1反向
            // 2 ：手拉使能 0开启 1关闭
            // 3 : 0电机停止 1开 2关 4电机卡停
            // 7 : 行程  0未设置行程  1已设置行程
            config.direction = command.data[1];
            config.hand_pull = command.data[2];
            if (command.data[3] == 0 || command.data[3] == 4) // 0电机停止 1 开 2 关 4 电机卡停
            {
                getPositionState = false;
            }
            else
            {
                getPositionState = true;
            }
            doPosition(command.data[0], command.command);
        }
    }
}

/*
  软串串口数据接受进程
 */
void Cover::readSoftwareSerialTick()
{
    while (softwareSerial->available())
    {
        uint8_t c = softwareSerial->read();
        Serial.printf("%02X ", c);
        if (softwareSerialPos == 0 && c != 0x55)
        { // 第一个字节不是0x55 抛弃
            Serial.print(F("\nBuff NO 0x55\n"));
            continue;
        }
        if (softwareSerialPos == 0)
        { // 记录0x55时间
            softwareSerialTime = millis();
        }
        softwareSerialBuff[softwareSerialPos++] = c;

        if (softwareSerialPos >= 6 && softwareSerialBuff[softwareSerialPos - 1] * 256 + softwareSerialBuff[softwareSerialPos - 2] == DOOYACommand::crc16(softwareSerialBuff, softwareSerialPos - 2)) // crc 正确
        {
            Serial.println();
            softwareSerialBuff[softwareSerialPos] = 0x00;
            doSoftwareSerialTick(softwareSerialBuff, softwareSerialPos);
            Led::led(200);
            softwareSerialPos = 0;
        }
        else if (softwareSerialPos > 15)
        {
            Debug.AddLog(LOG_LEVEL_INFO, PSTR("\nBuff Len Error"));
            softwareSerialPos = 0;
        }
    }

    if (softwareSerialPos > 0 && (millis() - softwareSerialTime >= 100))
    { // 距离0x55 超过100ms则抛弃指令
        softwareSerialPos = 0;
    }
}

void Cover::getPositionTask()
{
    uint8_t tmp[10];
    uint8_t len;
    if (config.weak_switch == 127)
    {
        config.weak_switch = 126;
        len = DOOYACommand::getWeakSwitchType(tmp, 0xFEFE, 0);
        softwareSerial->write(tmp, len);
        delay(100);
    }
    if (config.power_switch == 127)
    {
        config.power_switch = 126;
        len = DOOYACommand::getPowerSwitchType(tmp, 0xFEFE, 0);
        softwareSerial->write(tmp, len);
        delay(100);
    }

    len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
    softwareSerial->write(tmp, len);
}

#endif