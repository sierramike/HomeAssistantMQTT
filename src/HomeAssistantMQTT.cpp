#include "HomeAssistantMQTT.h"
#define DEBUG
#define CFG_ON_SERIAL

HomeAssistantMQTT::HomeAssistantMQTT()
{
  mqttClient = nullptr;
  wifiClient = nullptr;
    
  for (int i = 0; i < HAMQTT_MAXITEMS; i++)
    values[i] = 0;
}

HomeAssistantMQTT::~HomeAssistantMQTT()
{
  delete mqttClient;
  mqttClient = nullptr;
  delete wifiClient;
  wifiClient = nullptr;
}

void HomeAssistantMQTT::setCallback(HAMQTT_CALLBACK_SIGNATURE)
{
  this->cb_callback = cb_callback;
}

void HomeAssistantMQTT::begin(const char* server, const uint16_t port)
{
	begin(server, port, 1024, 15);
}

void HomeAssistantMQTT::begin(const char* server, const uint16_t port, const uint16_t bufferSize, const uint16_t keepAlive)
{
  StateTopic = Manufacturer + "/" + MQTTDeviceName;

#ifdef DEBUG
  Serial.print("StateTopic: \"");
  Serial.print(StateTopic);
  Serial.print(", Online state topic: \"");
  Serial.print(StateTopic + "/state");
  Serial.println("\"");
#endif

  delete wifiClient;
  wifiClient = new WiFiClient();

  delete mqttClient;
  mqttClient = new PubSubClient(*wifiClient);
  mqttClient->setBufferSize(bufferSize);
  mqttClient->setServer(server, port);
  mqttClient->setCallback(std::bind(&HomeAssistantMQTT::MqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  mqttClient->setKeepAlive(keepAlive);
}

void HomeAssistantMQTT::loop()
{
  if (!mqttClient->connected())
    connect();
  
  mqttClient->loop();
}

void HomeAssistantMQTT::connect()
{
  while (!mqttClient->connected())
  {
#ifdef CFG_ON_SERIAL
    Serial.print("Attempting MQTT connection...");
#endif
    String mqttClientId = MQTTDeviceName;
    if (mqttClient->connect(mqttClientId.c_str(), MqttUser.c_str(), MqttPassword.c_str(), (StateTopic + "/state").c_str(), 1, true, "{\"state\":\"offline\"}"))
    {
      mqttClient->publish((StateTopic + "/state").c_str(), "{\"state\":\"online\"}", true);
#ifdef CFG_ON_SERIAL
      Serial.println("connected");
#endif
    }
    else
    {
#ifdef CFG_ON_SERIAL
      Serial.print("failed, rc=");
      Serial.print(mqttClient->state());
      Serial.println(" will try again in 5 seconds");
#endif
      delay(5000);
    }
  }
}

bool HomeAssistantMQTT::connected()
{
  return mqttClient->connected();
}

void HomeAssistantMQTT::publishConfigSensor(String category, String deviceClass, String stateClass, String name, String icon, String unit, String startupValue)
{
  publishConfig("sensor", category, deviceClass, stateClass, name, icon, unit, false, true, true, "", "", startupValue);
}

void HomeAssistantMQTT::publishConfigBinarySensor(String category, String deviceClass, String name, String icon, String payloadOff, String payloadOn, String startupValue)
{
  String complement = ",\"payload_off\":\"" + payloadOff + "\",\"payload_on\":\"" + payloadOn + "\"";
  publishConfig("binary_sensor", category, deviceClass, "", name, icon, "", false, true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigNumber(String category, String name, String icon, String unit, String min, String max, String startupValue)
{
  String complement = ",\"min\":" + min + ",\"max\":" + max + "";
  publishConfig("number", category, "", "", name, icon, unit, true, true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigButton(String category, String name, String icon, String commandTopicName, String payload)
{
  String complement = ",\"payload_press\":\"" + payload + "\"";
  publishConfig("button", category, "", "", name, icon, "", true, false, false, commandTopicName, complement, "");
}

void HomeAssistantMQTT::publishConfigSelect(String category, String name, String icon, String options[], unsigned short optionsCount, String startupValue)
{
  String complement = ",\"options\":[\"";
  for (int i = 0; i < optionsCount; i++)
  {
    complement += (i > 0 ? "\",\"" : "") + options[i];
  }
  complement += "\"]";
  publishConfig("select", category, "", "", name, icon, "", true, true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigSwitch(String category, String name, String icon, String startupValue)
{
  String nameForTopic = name;
  nameForTopic.replace(" ", "_");

  String complement = ",\"payload_off\":\"false\",\"payload_on\":\"true\"";
  publishConfig("switch", category, "", "", name, icon, "", true, true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigDeviceAutomation(String category, String type, String subtype)
{
  String complement = ", \"payload\":\"" + subtype + "\""
      + ", \"subtype\":\"" + subtype + "\""
      + ", \"topic\":\"" + Manufacturer + "/" + MQTTDeviceName + "/" + type + "\""
      + ", \"type\":\"" + type + "\""
	  + ",\"automation_type\":\"trigger\"";
	
  String name = type + "_" + subtype;
  name.replace(" ", "_");
	
  publishConfig("device_automation", category, "", "", name, "", "", false, false, false, "", complement, "");
}

void HomeAssistantMQTT::publishConfigEvent(String category, String name, String eventTypes[], unsigned short eventTypesCount)
{
  String complement = ",\"event_types\":[\"";
  for (int i = 0; i < eventTypesCount; i++)
  {
    complement += (i > 0 ? "\",\"" : "") + eventTypes[i];
  }
  complement += "\"], \"platform\":\"event\"";

  publishConfig("event", category, "", "", name, "", "", false, true, false, "", complement, "");
}

void HomeAssistantMQTT::publishConfig(const char* type, String category, String deviceClass, String stateClass, String name, String icon, String unit, bool commandTopic, bool stateTopic, bool stateTopicValueTemplate, String commandTopicName, String complement, String startupValue)
{
  String nameForTopic = (name.length() > 0 ? name : deviceClass);
  nameForTopic.replace(" ", "_");
  String COMMAND_TOPIC = Manufacturer + "/" + MQTTDeviceName + "/set/" + (commandTopicName.length() > 0 ? commandTopicName : nameForTopic);

  String topic = "homeassistant/" + String(type) + "/" + MQTTDeviceName + "/" + nameForTopic + "/config";
  
  String data = "{\"availability\":[{\"topic\":\"" + Manufacturer + "/" + MQTTDeviceName + "/state\",\"value_template\":\"{{ value_json.state }}\"}]"
      + ",\"device\":{\"identifiers\":[\"" + MQTTDeviceName + "\"],\"manufacturer\":\"" + Manufacturer + "\",\"model\":\"" + Model + "\",\"name\":\"" + HADeviceName + "\",\"sw_version\":\"" + Version + "\"}"
	  + ",\"origin\":{\"name\":\"" + OriginName + "\",\"sw\":\"" + OriginVersion + "\"}"
      + (name.length() > 0 ? ",\"name\":\"" + name + "\"" : "")
      + ",\"unique_id\":\"" + MQTTDeviceName + "_" + nameForTopic + "\""
      
	  + ",\"enabled_by_default\":true"

      + (category.length() > 0 ? ",\"entity_category\":\"" + category + "\"" : "")
      
      + (icon.length() > 0 ? ",\"icon\":\"" + icon + "\"" : "")
      + (unit.length() > 0 ? ",\"unit_of_measurement\":\"" + unit + "\"" : "")
      + (deviceClass.length() > 0 ? ",\"device_class\":\"" + deviceClass + "\"" : "")
      + (stateClass.length() > 0 ? ",\"state_class\":\"" + stateClass + "\"" : "")

      + complement

      + (commandTopic ? ",\"command_topic\":\"" + COMMAND_TOPIC + "\"" : "")
      
      + (stateTopic ? ",\"state_topic\":\"" + StateTopic + (!stateTopicValueTemplate ? "/" + nameForTopic : "") + "\"" : "")
      + (stateTopicValueTemplate ? ",\"value_template\":\"{{ value_json." + nameForTopic + " }}\"" : "")

      + "}";

#ifdef CFG_ON_SERIAL
  Serial.print("  - ");
  Serial.println(topic.c_str());
  Serial.print("    ");
  Serial.println(data.c_str());
#endif

  mqttClient->publish(topic.c_str(), data.c_str(), true);

  if (commandTopic)
    mqttClient->subscribe(COMMAND_TOPIC.c_str());

  if (stateTopic)
    setValue(nameForTopic, startupValue);
}

void HomeAssistantMQTT::publishConfigCover(String category, String name, String commandTopicName, String statusEntity, String setPositionTopic, String positionEntity, String payloadOpen, String payloadClose, String payloadStop)
{
  String complement = ",\"state_topic\":\"" + StateTopic + "\""
      + ", \"value_template\":\"" + "{{ value_json." + statusEntity + " }}" + "\""
      + ", \"set_position_topic\":\"" + StateTopic + "/set/" + setPositionTopic + "\""
      + ", \"position_topic\":\"" + StateTopic + "\""
      + ", \"position_template\":\"" + "{{ value_json." + positionEntity + " }}" + "\""
      + ", \"payload_open\":\"" + payloadOpen + "\""
      + ", \"payload_close\":\"" + payloadClose + "\""
      + ", \"payload_stop\":\"" + payloadStop + "\""
      + ", \"payload_available\":\"online\""
      + ", \"payload_not_available\":\"offline\"";
	
  publishConfig("cover", category, "shutter", "", name, "", "", true, false, false, commandTopicName, complement, "");
}

void HomeAssistantMQTT::publishConfigClimate(String category, String name, String icon, String unit, String min, String max, String step, String startupValue)
{
  String cmdTopic = Manufacturer + "/" + MQTTDeviceName + "/set/" + name;

  String complement = ",\"temp_cmd_t\":\"" + cmdTopic + "\"";

  // supress the "+" sign in front of max, step value to avoid troubles in MQTT
  max.replace("+", "");
  min.replace("+", "");
  step.replace("+", "");
  complement += ",\"min_temp\":" + min + ",\"max_temp\":" + max + ",\"temp_step\":" + step;

  publishConfig("climate", category, "", "", name, icon, unit, true, true, true, name, complement, startupValue);
}

void HomeAssistantMQTT::clearSetTopic(String item)
{
  String topic = Manufacturer + "/" + MQTTDeviceName + "/set/" + item;
  mqttClient->publish(topic.c_str(), "", false);
}

void HomeAssistantMQTT::setValue(String item, String value)
{
  bool bFound = false;
  int i = 0;
  while (i < HAMQTT_MAXITEMS && !bFound)
  {
    if (values[i] != 0)
    {
      if (values[i]->item == item)
      {
        values[i]->value = value;
        bFound = true;
      }
    }
    else
    {
      // if we reach an entry with pointer 0, that means we reached end of existing items and didn't find it. Now create a new one.
      ItemValue* iv = new ItemValue;
      iv->item = item;
      iv->value = value;
      values[i] = iv;
      bFound = true;
    }
    i++;
  }
}

String HomeAssistantMQTT::getValue(String item)
{
  int i = 0;
  while (i < HAMQTT_MAXITEMS)
  {
    if (values[i] != 0)
    {
      if (values[i]->item == item)
        return values[i]->value;
    }
    i++;
  }
  return String("");
}

void HomeAssistantMQTT::readValues()
{
#ifdef DEBUG
  Serial.print("readValues: topic \"");
  Serial.print(StateTopic);
  Serial.println("\"");
#endif
  mqttClient->subscribe(StateTopic.c_str());
}

void HomeAssistantMQTT::sendValues()
{
  int ln = 0;
  int i = 0;
  while (i < HAMQTT_MAXITEMS)
  {
    if (values[i] != 0)
      ln += values[i]->item.length() + values[i]->value.length() + 6;

    i++;
  }
  ln++;

  String str;
  str = str + "{";
  i = 0;
  while (i < HAMQTT_MAXITEMS)
  {
    if (values[i] != 0)
    {
      str = str + "\"" + values[i]->item + "\":\"" + values[i]->value + "\",";
    }
    i++;
  }
  str[ln - 1] = '}';
  char c[ln];
  str.toCharArray(c, ln + 1);
  
#ifdef DEBUG
  Serial.print("MQTT PUBLISH: ");
  Serial.print(StateTopic);
  Serial.print(" => ");
  Serial.println(c);
#endif
  
  mqttClient->publish(StateTopic.c_str(), c, true);
}

void HomeAssistantMQTT::sendCommand(String commandTopic, String payload)
{
  String topic = StateTopic + "/" + commandTopic;
  mqttClient->publish(topic.c_str(), payload.c_str(), false);
}

void HomeAssistantMQTT::sendEvent(String eventName, String eventType)
{
  sendCommand(eventName, "{\"event_type\":\"" + eventType + "\"}");
}

void HomeAssistantMQTT::MqttCallback(char* topic, byte* payload, unsigned int length)
{
#ifdef DEBUG
  Serial.print("  ** Message arrived on topic: '");
  Serial.print(topic);
  Serial.print("' with payload: ");
#endif

  char cPayload[length + 1];
  for (unsigned int i = 0; i < length; i++)
  {
    cPayload[i] = (char)payload[i];
  }
  cPayload[length] = '\0';

#ifdef DEBUG
  Serial.print(cPayload);
  Serial.println();
#endif

  if (StateTopic == topic)
  {
    mqttClient->unsubscribe(StateTopic.c_str());
    // Received message on device state topic, read the values and unsubscribe from the topic
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cPayload);
    if (error)
    {
#ifdef DEBUG
      Serial.print("     Error reading old values, deserializeJson() returned: ");
      Serial.println(error.c_str());
#endif
    }
    else
    {
      for (JsonPair kv : doc.as<JsonObject>())
      {
#ifdef DEBUG
        Serial.print("     Key: \"");
        Serial.print(kv.key().c_str());
        Serial.print("\", Value: \"");
        Serial.print(kv.value().as<const char*>());
        Serial.println("\"");
#endif
        setValue(String(kv.key().c_str()), String(kv.value().as<const char*>()));
        if (cb_callback != NULL)
          cb_callback(String(kv.key().c_str()), String(kv.value().as<const char*>()), true);
      }
      sendValues();
    }
    // mqttClient->unsubscribe(StateTopic.c_str());
  }
  else
  {
    String COMMAND_TOPIC = Manufacturer + "/" + MQTTDeviceName + "/set/";
    if (strncmp(topic, COMMAND_TOPIC.c_str(), COMMAND_TOPIC.length()) == 0)
    {
      char buffer[strlen(topic + COMMAND_TOPIC.length())];
      strcpy(buffer, topic + COMMAND_TOPIC.length());
      if (cb_callback != NULL)
        cb_callback(String(buffer), String(cPayload), false);
    }
  }
}
