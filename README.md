# HomeAssistantMQTT
Arduino Library to create IoT devices connected to Home Assistant via MQTT.

This library manages MQTT structure to easily publish Sensors, Binary sensors, Buttons, Switches, Numbers and Options to create a device and communicate with it in Home Assistant. Supports reading actual values from MQTT upon restart.

## Features

* Publish Sensor, Binary sensor, Button, Number, Select, Switch items to MQTT
* Handles command and state topics automatically
* Publish device state
* Read previous device state from MQTT topic upon restart
* Callback function for user processing

## Quickstart

Open HomeAssistantMQTT-Demo example included in this library.

Change parameters to fit to your WiFi network, MQTT server and device name in the following section:

```c++
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
```

Upload to an ESP8266 device and see the device popup in MQTT integration in Home Assistant.

## Prerequisites

A functional Home Assistant instance with MQTT integration and a functional MQTT server (see Home Assistant documentation and tutorials if needed).

## Code usage

See example source code for a basic IoT device implementation.

### loop

In the loop method, call this library's instance loop method. Check if connection is successful, then publish your device configuration (ensure that you only publish configuration once after startup), then optionaly read actual values from MQTT to restore previous state after device restart:

```c++
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
```

### publishMqttConfig

The method name can be changed. Call publishMethodxxxxxxx functions to publish all your sensors, buttons switches etc. to Home Assistant for automated device creation:

```c++
void publishMqttConfig()
{
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
```

### callback

The callback method is defined in the setup method of the sketch. This method is called by the instance upon new message arriving from Home Assistant (user configuration change or button press). You can put all your logic where "PROCESS NEW DATA HERE" is written.

It is recommanded to update the state topic of the device with new incoming values and update the state topic, but buttons don't have states, so you may check for the item parameter content to only update other topics but not buttons, like in the example below:

```c++
void HAMQTT_Callback(String item, String payload)
{
  // PROCESS NEW DATA HERE

  if (item != "Command")
  {
    mqtt.setValue(item, payload);
    mqtt.sendValues();
  }
}
```

## publishConfig

Several methods are available to publish sensors, binary sensors, buttons, numbers, select and switches to Home Assistant.

### publishConfigSensor

A sensor will report values to Home Assistant that are not "ON/OFF" style.

```c++
void publishConfigSensor(String deviceClass, String name, String icon, String unit, String startupValue);
```

deviceClass: optional, for standardized sensors, use Home Assistant device class to benefit for default icons, names and units, otherwise leave empty (see https://www.home-assistant.io/integrations/sensor/#device-class)
name: name of the sensor, can be left empty if defining a device class
icon: material design icon to illustrate the sensor, can be left empty if defining a device class (see https://pictogrammers.com/library/mdi/)
unit: optional, unit of measurement, can be left empty if defining a device class
startupValue: value of the sensor at startup of the device, if no previous value can be read from MQTT state topic

### publishConfigBinarySensor

A binary sensor is a sensor that only reports ON/OFF or true/false states.

```c++
void publishConfigBinarySensor(String deviceClass, String name, String icon, String payloadOff, String payloadOn, String startupValue);
```

deviceClass: optional, for standardized sensors, use Home Assistant device class to benefit for default icons, names and units, otherwise leave empty (see https://www.home-assistant.io/integrations/sensor/#device-class)
name: name of the sensor, can be left empty if defining a device class
icon: material design icon to illustrate the sensor, can be left empty if defining a device class (see https://pictogrammers.com/library/mdi/)
payloadOff: text value of the sensor when it reports OFF/false state
payloadOn: text value of the sensor when it reports ON/true state
startupValue: value of the sensor at startup of the device, if no previous value can be read from MQTT state topic

### publishConfigButton

A button doesn't keep any state, but permits to send commands from Home Assistant to the IoT device.

```c++
void publishConfigButton(String category, String name, String icon, String commandTopicName, String payload);
```

category: config or diagnostic, or left empty, to define the group where this entity is diplayed in Home Assistant
name: name of the button
icon: material design icon to illustrate the button (see https://pictogrammers.com/library/mdi/)
commandTopicName: the topic that will get the payload when user clicks the button, several buttons can share the same commandTopicName with different payloads
payload: text value that will be sent to MQTT when the button is pressed

### publishConfigNumber

Number is a numeric field allowing user to change a numeric value.

```c++
void publishConfigNumber(String category, String name, String icon, String unit, String min, String max, String startupValue);
```

category: config or diagnostic, or left empty, to define the group where this entity is diplayed in Home Assistant
name: name of the button
icon: material design icon to illustrate the button (see https://pictogrammers.com/library/mdi/)
unit: optional, unit of measurement
min: minimum accepted value
max: maximum accepted value
startupValue: value of the numeric field at startup of the device, if no previous value can be read from MQTT state topic

### publishConfigSelect

Select is a dropdown list where user can pickup a predefined value.

```c++
void publishConfigSelect(String category, String name, String icon, String options[], unsigned short optionsCount, String startupValue);
```

category: config or diagnostic, or left empty, to define the group where this entity is diplayed in Home Assistant
name: name of the button
icon: material design icon to illustrate the button (see https://pictogrammers.com/library/mdi/)
options: String array containing the list of predefined values
optionsCount: number of predefined values in "options" array
startupValue: value of the numeric field at startup of the device, if no previous value can be read from MQTT state topic

### publishConfigSwitch

Switch is an ON/OFF slider usually for enabling/activating options of the device.

```c++
void publishConfigSwitch(String category, String name, String icon, String startupValue);
```

category: config or diagnostic, or left empty, to define the group where this entity is diplayed in Home Assistant
name: name of the button
icon: material design icon to illustrate the button (see https://pictogrammers.com/library/mdi/)
startupValue: value of the numeric field at startup of the device, if no previous value can be read from MQTT state topic
