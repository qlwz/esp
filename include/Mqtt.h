// Mqtt.h

#ifndef _MQTT_h
#define _MQTT_h

#define MQTT_SOCKET_TIMEOUT 5
#define MQTT_MAX_PACKET_SIZE 768

#include "arduino.h"
#include <PubSubClient.h>

class Mqtt
{
protected:
    String getTopic(uint8_t prefix, String subtopic);
    String topicCmnd;
    String topicStat;
    String topicTele;

public:
    PubSubClient mqttClient;
    void (*_connectedCallback)(void) = NULL;

    bool wasConnected = true;
    uint32_t lastDisconnectedTime = 0;         // 最后重连时间
    uint32_t lastConnectedTime = 0;            // 最后在线时间
    uint32_t mqttDisconnectCounter = 0;        // 重连次数
    uint32_t lastReconnectAttempt = 0;         // 最后尝试重连时间
    const uint32_t kMqttReconnectTime = 30000; // 重新连接尝试之间的延迟（ms）
    bool mqttIsFirst = true;

    bool mqttConnect();
    void doReport();
    void loop();
    void mqttSetLoopCallback(MQTT_CALLBACK_SIGNATURE);
    void mqttSetConnectedCallback(void (*func)(void));

    void setTopic();
    String getCmndTopic(String topic);
    String getStatTopic(String topic);
    String getTeleTopic(String topic);

    PubSubClient &setClient(Client &client);
    boolean publish(String topic, const char *payload);
    boolean publish(String topic, const char *payload, boolean retained);

    boolean publish(const char *topic, const char *payload);
    boolean publish(const char *topic, const char *payload, boolean retained);
    boolean publish(const char *topic, const uint8_t *payload, unsigned int plength);
    boolean publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained);
    boolean publish_P(const char *topic, const char *payload, boolean retained);
    boolean publish_P(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained);

    boolean subscribe(String topic);
    boolean subscribe(String topic, uint8_t qos);
    boolean unsubscribe(String topic);
    void perSecondDo();
};

extern Mqtt *mqtt;

#endif
