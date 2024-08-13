#include "HomeAssistantMQTT.h"
#define DEBUG

HomeAssistantMQTT::HomeAssistantMQTT()
{
  for (int i = 0; i < HAMQTT_MAXITEMS; i++)
    values[i] = 0;
}

void HomeAssistantMQTT::setCallback(CallbackFunction f)
{
  cb_callback = f;
}

void HomeAssistantMQTT::begin(WiFiClient* wifiClient, const char* server, const uint16_t port)
{
  int ln = Manufacturer.length() + DeviceName.length() + 8;
  MqttStateTopic = new char[ln];
  strcpy(MqttStateTopic, Manufacturer.c_str());
  strcat(MqttStateTopic, "/");
  strcat(MqttStateTopic, DeviceName.c_str());
  strcat(MqttStateTopic, "/state");

#ifdef DEBUG
  Serial.print("MqttStateTopic: \"");
  Serial.print(MqttStateTopic);
  Serial.println("\"");
#endif

  mqttClient = new PubSubClient(*wifiClient);
  mqttClient->setBufferSize(1024);
  mqttClient->setServer(server, port);
  mqttClient->setCallback(std::bind(&HomeAssistantMQTT::MqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  mqttClient->setKeepAlive(5);
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
    String mqttClientId = "";
    if (mqttClient->connect(mqttClientId.c_str(), MqttUser.c_str(), MqttPassword.c_str(), MqttStateTopic, 1, true, "{\"state\":\"offline\"}"))
    {
      mqttClient->publish(MqttStateTopic, "{\"state\":\"online\"}", true);
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

void HomeAssistantMQTT::publishConfigSensor(String deviceClass, String name, String icon, String unit, String startupValue)
{
  publishConfig("sensor", "", deviceClass, name, icon, unit, false, true, "", "", startupValue);
}

void HomeAssistantMQTT::publishConfigBinarySensor(String deviceClass, String name, String icon, String payloadOff, String payloadOn, String startupValue)
{
  String complement = ",\"payload_off\":\"" + payloadOff + "\",\"payload_on\":\"" + payloadOn + "\"";
  publishConfig("binary_sensor", "", deviceClass, name, icon, "", false, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigNumber(String category, String name, String icon, String unit, String min, String max, String startupValue)
{
  String complement = ",\"min\":" + min + ",\"max\":" + max + "";
  publishConfig("number", category, "", name, icon, unit, true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigButton(String category, String name, String icon, String commandTopicName, String payload)
{
  String complement = ",\"payload_press\":\"" + payload + "\"";
  publishConfig("button", category, "", name, icon, "", true, false, commandTopicName, complement, "");
}

void HomeAssistantMQTT::publishConfigSelect(String category, String name, String icon, String options[], unsigned short optionsCount, String startupValue)
{
  String complement = ",\"options\":[\"";
  for (int i = 0; i < optionsCount; i++)
  {
    complement += (i > 0 ? "\",\"" : "") + options[i];
  }
  complement += "\"]";
  publishConfig("select", category, "", name, icon, "", true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfigSwitch(String category, String name, String icon, String startupValue)
{
  String nameForTopic = name;
  nameForTopic.replace(" ", "_");

  String complement = ",\"payload_off\":\"false\",\"payload_on\":\"true\"";
  publishConfig("switch", category, "", name, icon, "", true, true, "", complement, startupValue);
}

void HomeAssistantMQTT::publishConfig(const char* type, String category, String deviceClass, String name, String icon, String unit, bool commandTopic, bool stateTopic, String commandTopicName, String complement, String startupValue)
{
  String nameForTopic = (name.length() > 0 ? name : deviceClass);
  nameForTopic.replace(" ", "_");
  String COMMAND_TOPIC = Manufacturer + "/" + DeviceName + "/set/" + (commandTopicName.length() > 0 ? commandTopicName : nameForTopic);

  StateTopic = new char[Manufacturer.length() + DeviceName.length() + 1];
  strcpy(StateTopic, Manufacturer.c_str());
  strcat(StateTopic, "/");
  strcat(StateTopic, DeviceName.c_str());

  String topic = "homeassistant/" + String(type) + "/" + DeviceName + "/" + nameForTopic + "/config";
  String data = "{\"availability\":[{\"topic\":\"" + Manufacturer + "/" + DeviceName + "/state\",\"value_template\":\"{{ value_json.state }}\"}]"
      + ",\"device\":{\"identifiers\":[\"" + DeviceName + "\"],\"manufacturer\":\"" + Manufacturer + "\",\"model\":\"" + Model + "\",\"name\":\"" + Name + "\",\"sw_version\":\"" + Version + "\"}"
      + ",\"enabled_by_default\":true"

      + (category.length() > 0 ? ",\"entity_category\":\"config\"" : "")
      
      + ",\"unique_id\":\"" + DeviceName + "_" + nameForTopic + "\""
      + (name.length() > 0 ? ",\"name\":\"" + name + "\"" : "")
      + (icon.length() > 0 ? ",\"icon\":\"" + icon + "\"" : "")
      + (unit.length() > 0 ? ",\"unit_of_measurement\":\"" + unit + "\"" : "")
      + (deviceClass.length() > 0 ? ",\"device_class\":\"" + deviceClass + "\"" : "")

      + complement

      + (commandTopic ? ",\"command_topic\":\"" + COMMAND_TOPIC + "\"" : "")
      
      + (stateTopic ? ",\"state_topic\":\"" + String(StateTopic) + "\"" : "")
      + (stateTopic ? ",\"value_template\":\"{{ value_json." + nameForTopic + " }}\"" : "")

      + "}";

#ifdef DEBUG
  Serial.println(topic.c_str());
  Serial.print("  - ");
  Serial.println(data.c_str());
#endif

  mqttClient->publish(topic.c_str(), data.c_str(), true);

  if (commandTopic)
    mqttClient->subscribe(COMMAND_TOPIC.c_str());

  if (stateTopic)
    setValue(nameForTopic, startupValue);
}

void HomeAssistantMQTT::setValue(String item, String value)
{
  bool bFound = false;
  int i = 0;
  while (i < HAMQTT_MAXITEMS && !bFound)
  {
    if (values[i] != 0)
    {
/*#ifdef DEBUG
      Serial.print("Item #");
      Serial.print(i);
      Serial.print(", Name: \"");
      Serial.print(values[i]->item);
      Serial.println("\"");
#endif*/
      if (strcmp(values[i]->item, item.c_str()) == 0)
      {
/*#ifdef DEBUG
        Serial.print("Found! Current value is: \"");
        Serial.print(values[i]->value);
        Serial.print("\", new value will be: \"");
        Serial.print(value);
        Serial.println("\"");
#endif*/

        strcpy(values[i]->value, value.c_str());
        //strncpy(values[i]->value, value.c_str(), value.length);
        //values[i]->value[value.length] = '\0';
        //values[i]->value = value;
        bFound = true;
      }
    }
    else
    {
/*#ifdef DEBUG
      Serial.print("Not found, adding ItemValue for item: \"");
      Serial.print(item);
      Serial.print("\", with value: \"");
      Serial.print(value);
      Serial.println("\"");
#endif*/
      // if we reach an entry with pointer 0, that means we reached end of existing items and didn't find it. Now create a new one.
      ItemValue* iv = new ItemValue;
      iv->item = new char[31];
      iv->value = new char[31];
      strcpy(iv->item, item.c_str());
      strcpy(iv->value, value.c_str());
      //iv->item = item;
      //iv->value = value;
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
      if (strcmp(values[i]->item, item.c_str()) == 0)
        return String(values[i]->value);
    }
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
  mqttClient->subscribe(StateTopic);
}

void HomeAssistantMQTT::sendValues()
{
  int ln = 0;
  int i = 0;
  while (i < HAMQTT_MAXITEMS)
  {
    if (values[i] != 0)
      ln += strlen(values[i]->item) + strlen(values[i]->value) + 6;

    i++;
  }
  ln++;

  char* c = new char[ln];
  strcpy(c, "{");
  
  i = 0;
  while (i < HAMQTT_MAXITEMS)
  {
    if (values[i] != 0)
    {
      strcat(c, "\"");
      strcat(c, values[i]->item);
      strcat(c, "\":\"");
      strcat(c, values[i]->value);
      strcat(c, "\",");
    }
    i++;
  }
  c[ln - 1] = '}';

  mqttClient->publish(StateTopic, c, true);
}

void HomeAssistantMQTT::MqttCallback(char* topic, byte* payload, unsigned int length)
{
#ifdef DEBUG
  Serial.print("Message arrived on topic: '");
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

  if (strcmp(StateTopic, topic) == 0)
  {
    // Received message on device state topic, read the values and unsubscribe from the topic
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cPayload);
    if (error)
    {
#ifdef DEBUG
      Serial.print("Error reading old values, deserializeJson() returned: ");
      Serial.println(error.c_str());
#endif
    }
    else
    {
      for (JsonPair kv : doc.as<JsonObject>())
      {
#ifdef DEBUG
        Serial.print("Key: \"");
        Serial.print(kv.key().c_str());
        Serial.print("\", Value: \"");
        Serial.print(kv.value().as<const char*>());
        Serial.println("\"");
#endif
        setValue(String(kv.key().c_str()), String(kv.value().as<const char*>()));
      }
      sendValues();
    }
    
    mqttClient->unsubscribe(StateTopic);
  }
  else
  {
    String COMMAND_TOPIC = Manufacturer + "/" + DeviceName + "/set/";
    if (strncmp(topic, COMMAND_TOPIC.c_str(), COMMAND_TOPIC.length()) == 0)
    {
      char buffer[strlen(topic + COMMAND_TOPIC.length())];
      strcpy(buffer, topic + COMMAND_TOPIC.length());
      cb_callback(String(buffer), String(cPayload));
    }
  }
}
