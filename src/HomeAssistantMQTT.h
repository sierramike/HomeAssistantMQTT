#pragma once

#include "Arduino.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"

#ifdef ESP8266
#include "ESP8266WiFi.h"
#endif

#ifdef ESP32
#include "WiFi.h"
#endif

#define HAMQTT_MAXITEMS 30

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define HAMQTT_CALLBACK_SIGNATURE std::function<void(String, String, bool)> cb_callback
#else
#define HAMQTT_CALLBACK_SIGNATURE void (*cb_callback)(String, String, bool)
#endif

struct ItemValue
{
  String item;
  String value;
};


class HomeAssistantMQTT
{
  private:
    WiFiClient* wifiClient;
    PubSubClient* mqttClient;

    String StateTopic;

    ItemValue* values[HAMQTT_MAXITEMS];

    void connect();
    void publishConfig(const char* type, String category, String deviceClass, String stateClass, String name, String icon, String unit, bool commandTopic, bool stateTopic, bool stateTopicValueTemplate, String commandTopicName, String complement, String startupValue);
    void MqttCallback(char* topic, byte* payload, unsigned int length);

    HAMQTT_CALLBACK_SIGNATURE;

  public:
    String MqttUser;
    String MqttPassword;
  
    String OriginName;
    String OriginVersion;

    String Manufacturer;
    String Model;
    String Version;
    String HADeviceName;
    String MQTTDeviceName;
  
    HomeAssistantMQTT();
    ~HomeAssistantMQTT();

    void begin(const char* server, const uint16_t port);
    void begin(const char* server, const uint16_t port, const uint16_t bufferSize, const uint16_t keepAlive);
    void loop();

    bool connected();
    void readValues();
    void sendValues();
    void sendCommand(String commandTopic, String payload);
    void sendEvent(String eventName, String eventType);
    void setCallback(HAMQTT_CALLBACK_SIGNATURE);

    void setValue(String item, String value);
    String getValue(String item);
    void clearSetTopic(String item);

    void publishConfigSensor(String category, String deviceClass, String stateClass, String name, String icon, String unit, String startupValue);
    void publishConfigBinarySensor(String category, String deviceClass, String name, String icon, String payloadOff, String payloadOn, String startupValue);
    void publishConfigNumber(String category, String name, String icon, String unit, String min, String max, String startupValue);
    void publishConfigButton(String category, String name, String icon, String commandTopicName, String payload);
    void publishConfigSelect(String category, String name, String icon, String options[], unsigned short optionsCount, String startupValue);
    void publishConfigSwitch(String category, String name, String icon, String startupValue);
    void publishConfigDeviceAutomation(String category, String type, String subtype);
    void publishConfigEvent(String category, String name, String eventTypes[], unsigned short eventTypesCount);

    void publishConfigCover(String category, String name, String commandTopicName, String statusEntity, String setPositionTopic, String positionEntity, String payloadOpen, String payloadClose, String payloadStop);
    void publishConfigClimate(String category, String name, String icon, String unit, String min, String max, String step, String startupValue);
};
