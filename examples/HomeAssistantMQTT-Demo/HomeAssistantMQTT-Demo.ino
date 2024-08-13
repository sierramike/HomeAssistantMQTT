#define DEBUG
#define CFG_ON_SERIAL

#include "Arduino.h"
#include "ESP8266WiFi.h"

#include "HomeAssistantMQTT.h"

/////////////////////////////////////////////////////////////////////////////////
/// Settings
/////////////////////////////////////////////////////////////////////////////////

const char* mqtt_server = "mymqttserver.domain.com";  // MQTT Server URL
const uint16_t mqtt_server_port = 1883;               // MQTT Server port

String _MQTTUSER = "mqttusername";                    // MQTT Username
String _MQTTPASSWORD = "mqttpassword";                // MQTT Password

const char* ssid = "MyWifiSSID";                      // WIFI SSID
const char* password = "MYWIFIPASSWORD";              // WIFI Password

String _MANUF = "MY";                   // Manufacturer name
String _MODEL = "MqttDemo";             // Device model name
String _VERSION = "v0.1";               // Device firmware version
String _NAME = "Shutter";               // Device name in Home Assistant

/////////////////////////////////////////////////////////////////////////////////


WiFiClient wifiClient;
HomeAssistantMQTT mqtt;
bool _bMqttConfigPublished = false;


void setup() {
#if defined(DEBUG) || defined(CFG_ON_SERIAL)
  Serial.begin(115200);
  delay(1000);
#endif

#ifdef CFG_ON_SERIAL
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#ifdef CFG_ON_SERIAL
    Serial.print(".");
#endif
  }
#ifdef CFG_ON_SERIAL
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
#endif

  String mac = WiFi.macAddress();
  String MAC = mac.substring(0, 2) + mac.substring(3, 5) + mac.substring(6, 8) + mac.substring(9, 11) + mac.substring(12, 14) + mac.substring(15, 17);

  mqtt.MqttUser = _MQTTUSER;
  mqtt.MqttPassword = _MQTTPASSWORD;
  mqtt.Manufacturer = _MANUF;
  mqtt.Model = _MODEL;
  mqtt.Version = _VERSION;
  mqtt.Name = _NAME;
  mqtt.DeviceName = _MANUF + "_" + _MODEL + "_" + MAC;
  mqtt.setCallback(HAMQTT_Callback);
  
  mqtt.begin(&wifiClient, mqtt_server, mqtt_server_port);
}

void loop() {
  mqtt.loop();
  if (mqtt.connected())
  {
    if (!_bMqttConfigPublished)
    {
      // Publish config upon boot
      publishMqttConfig();
      // Try to read actual values from MQTT to restore previous state
      mqtt.readValues();
      _bMqttConfigPublished = true;
    }
  }
}

void publishMqttConfig()
{
#ifdef DEBUG
  Serial.println("Publishing config:");
#endif
  
  mqtt.publishConfigSensor("", "Button", "mdi:gesture-tap-button", "", "None");
  mqtt.publishConfigSensor("", "Position", "mdi:window-shutter-settings", "%", "0");
  mqtt.publishConfigSensor("", "Status", "mdi:window-shutter", "", "Up");
  mqtt.publishConfigBinarySensor("smoke", "", "", "false", "true", "false");
  mqtt.publishConfigNumber("config", "Duration", "mdi:timer", "s", "0", "300", "25");
  mqtt.publishConfigNumber("config", "Shadow position", "mdi:window-shutter-alert", "%", "5", "95", "85");
  mqtt.publishConfigButton("", "Open", "mdi:window-shutter-open", "Command", "open");
  mqtt.publishConfigButton("", "Close", "mdi:window-shutter", "Command", "close");
  mqtt.publishConfigButton("", "Shadow", "mdi:window-shutter-alert", "Command", "shadow");
  mqtt.publishConfigButton("", "Custom", "mdi:window-shutter-cog", "Command", "custom");
  mqtt.publishConfigNumber("", "Custom position", "mdi:window-shutter-cog", "%", "5", "95", "50");
  String options[] = { "Open", "Close", "None" };
  mqtt.publishConfigSelect("config", "Startup init", "mdi:cog-refresh", options, 3, "Open");
  mqtt.publishConfigSwitch("config", "Enabled", "mdi:blur", "true");
}

void HAMQTT_Callback(String item, String payload)
{
#ifdef DEBUG
  Serial.print("Message arrived for item: '");
  Serial.print(item);
  Serial.print("' with payload: ");
  Serial.print(payload);
  Serial.println();
#endif

  // PROCESS NEW DATA HERE

  if (item != "Command")
  {
    mqtt.setValue(item, payload);
    mqtt.sendValues();
  }
}
