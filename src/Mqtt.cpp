
#include "Config.h"
#include "Debug.h"
#include "Mqtt.h"
#include "Wifi.h"
#include <PubSubClient.h>

bool Mqtt::mqttConnect()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		Debug.AddLog(LOG_LEVEL_INFO, PSTR("wifi disconnected"));
		return false;
	}
	if (config.mqtt_port == 0)
	{
		return false;
	}
	if (mqttClient.connected())
	{
		return true;
	}

	Debug.AddLog(LOG_LEVEL_INFO, PSTR("Connecting to %s:%d Broker . . "), config.mqtt_server, config.mqtt_port);
	mqttClient.setServer(config.mqtt_server, config.mqtt_port);

	if (mqttClient.connect(UID, config.mqtt_user, config.mqtt_pass, getTeleTopic(F("availability")).c_str(), 0, false, "offline"))
	{
		Debug.AddLog(LOG_LEVEL_INFO, PSTR("(Re)Connected."));
		if (_connectedCallback != NULL)
		{
			_connectedCallback();
		}
	}
	else
	{
		Debug.AddLog(LOG_LEVEL_INFO, PSTR("failed, rc=%d"), mqttClient.state());
	}

	return mqttClient.connected();
}

String Mqtt::msToHumanString(uint32_t const msecs)
{
	uint32_t totalseconds = msecs / 1000;
	if (totalseconds == 0)
		return F("Now");

	// Note: millis() can only count up to 45 days, so uint8_t is safe.
	uint8_t days = totalseconds / (60 * 60 * 24);
	uint8_t hours = (totalseconds / (60 * 60)) % 24;
	uint8_t minutes = (totalseconds / 60) % 60;
	uint8_t seconds = totalseconds % 60;

	String result = "";
	if (days)
		result += String(days) + " day";
	if (days > 1)
		result += 's';
	if (hours)
		result += ' ' + String(hours) + " hour";
	if (hours > 1)
		result += 's';
	if (minutes)
		result += ' ' + String(minutes) + " minute";
	if (minutes > 1)
		result += 's';
	if (seconds)
		result += ' ' + String(seconds) + " second";
	if (seconds > 1)
		result += 's';
	result.trim();
	return result;
}

String Mqtt::timeSince(uint32_t const start)
{
	if (start == 0)
		return F("Never");
	uint32_t diff = 0;
	uint32_t now = millis();
	if (start < now)
		diff = now - start;
	else
		diff = UINT32_MAX - start + now;
	return msToHumanString(diff) + " ago";
}

void Mqtt::doReport()
{
	char message[250];
	sprintf(message, "{\"UID\":\"%s\",\"SSID\":\"%s\",\"RSSI\":\"%s\",\"Version\":\"%s\",\"ip\":\"%s\",\"mac\":\"%s\",\"freeMem\":%d,\"uptime\":%d}",
			UID, WiFi.SSID().c_str(), String(WiFi.RSSI()).c_str(), VERSION, WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str(), ESP.getFreeHeap(), millis() / 1000);
	Debug.AddLog(LOG_LEVEL_INFO, PSTR("%s"), message);
	publish(getTeleTopic(F("HEARTBEAT")), message);

	publish(getTeleTopic(F("availability")), "online", false);
}

void Mqtt::perSecondDo()
{
	if (perSecond % 60 != 0)
	{
		return;
	}
	if (mqttClient.connected())
	{
		doReport();
	}
}

void Mqtt::loop()
{
	uint32_t now = millis();
	if (!mqttClient.connected())
	{
		if (WiFi.status() != WL_CONNECTED)
		{
			return;
		}
		if (wasConnected)
		{
			lastDisconnectedTime = now;
			wasConnected = false;
			mqttDisconnectCounter++;
		}
		if (now - lastReconnectAttempt > kMqttReconnectTime || lastReconnectAttempt == 0)
		{
			lastReconnectAttempt = now;
			Debug.AddLog(LOG_LEVEL_INFO, PSTR("client mqtt not connected, trying to connect"));
			if (mqttConnect())
			{
				lastReconnectAttempt = 0;
				wasConnected = true;
				if (mqttIsFirst)
				{
					Debug.AddLog(LOG_LEVEL_INFO, PSTR("MQTT just booted"));
					mqttIsFirst = false;
				}
				else
				{
					Debug.AddLog(LOG_LEVEL_INFO, PSTR("MQTT just (re)connected to MQTT. Lost connection about %s"), timeSince(lastConnectedTime).c_str());
				}
				lastConnectedTime = now;
				Debug.AddLog(LOG_LEVEL_INFO, PSTR("successful client mqtt connection"));
				doReport();
			}
		}
	}
	else
	{
		lastConnectedTime = now;
		mqttClient.loop();
	}
}

void Mqtt::setTopic()
{
	topicCmnd = getTopic(0, "");
	topicStat = getTopic(1, "");
	topicTele = getTopic(2, "");
}

String Mqtt::getCmndTopic(String topic)
{
	return topicCmnd + topic;
}

String Mqtt::getStatTopic(String topic)
{
	return topicStat + topic;
}
String Mqtt::getTeleTopic(String topic)
{
	return topicTele + topic;
}

void Mqtt::mqttSetLoopCallback(MQTT_CALLBACK_SIGNATURE)
{
	mqttClient.setCallback(callback);
}

void Mqtt::mqttSetConnectedCallback(void (*func)(void))
{
	_connectedCallback = func;
}

PubSubClient &Mqtt::setClient(Client &client)
{
	setTopic();
	return mqttClient.setClient(client);
}
boolean Mqtt::publish(String topic, const char *payload)
{
	return mqttClient.publish(topic.c_str(), payload);
}

boolean Mqtt::publish(String topic, const char *payload, boolean retained)
{
	return mqttClient.publish(topic.c_str(), payload, retained);
}

boolean Mqtt::publish(const char *topic, const char *payload)
{
	return mqttClient.publish(topic, payload);
}
boolean Mqtt::publish(const char *topic, const char *payload, boolean retained)
{
	return mqttClient.publish(topic, payload, retained);
}
boolean Mqtt::publish(const char *topic, const uint8_t *payload, unsigned int plength)
{
	return mqttClient.publish(topic, payload, plength);
}
boolean Mqtt::publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
	return mqttClient.publish(topic, payload, plength, retained);
}
boolean Mqtt::publish_P(const char *topic, const char *payload, boolean retained)
{
	return mqttClient.publish_P(topic, payload, retained);
}
boolean Mqtt::publish_P(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
	return mqttClient.publish_P(topic, payload, plength, retained);
}

boolean Mqtt::subscribe(String topic)
{
	return mqttClient.subscribe(topic.c_str());
}
boolean Mqtt::subscribe(String topic, uint8_t qos)
{
	return mqttClient.subscribe(topic.c_str(), qos);
}
boolean Mqtt::unsubscribe(String topic)
{
	return mqttClient.unsubscribe(topic.c_str());
}

String Mqtt::getTopic(uint8_t prefix, String subtopic)
{
	// 0: Cmnd  1:Stat 2:Tele
	String fulltopic = String(config.mqtt_topic);
	if ((0 == prefix) && (-1 == fulltopic.indexOf(F("%prefix%"))))
	{
		fulltopic += F("/%prefix%"); // Need prefix for commands to handle mqtt topic loops
	}
	fulltopic.replace(F("%prefix%"), (prefix == 0 ? F("cmnd") : ((prefix == 1 ? F("stat") : F("tele")))));
	fulltopic.replace(F("%hostname%"), UID);
	fulltopic.replace(F("%module%"), module->moduleName());
	fulltopic.replace(F("#"), "");
	fulltopic.replace(F("//"), "/");
	if (!fulltopic.endsWith(F("/")))
		fulltopic += F("/");
	return fulltopic + subtopic;
}

Mqtt *mqtt;
