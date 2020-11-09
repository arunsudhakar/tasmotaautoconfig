# Tasmota Auto Config
An Arduino project which runs on ESP devices to automatically do the initial configuration of Tasmota devices via JSON config files

I was doing a Home Automation project which involved several Tasmota flashed Sonoff Switches and I found it rather cumbersome to connect to each device one by one and manually do the config. Therefore I decided to create a firmware which automatically scans for Tasmota networks and configures them using the Backlog command via HTTP,therefore you can give all supported Tasmota commands.

The config params are stored in a JSON file on the SDCARD. I've tested it on the ESP32 Cam module, since it already comes integrated with an SD Card Reader.

## There are 2 Config Modes :

### 1. Universal Config
In this mode, the config params are set for all Unconfigured Tasmota devices

_Sample Json_

`{
   "config":{
      "SSID1":"TestNetwork",
      "Password1":"MyPassword",
      "Module":"29"
   }
}`


### 2. Device Specifi Config
In this mode, the config params are set based on the Tasmota device's MAC address. This is useful if you need to configure multiple modules.

_Sample Json_

`{
   "f6:cf:a2:xx:xx:xx":{
      "SSID1":"MyNet",
      "Password1":"MyPass",
      "Module":"29",
      "MqttHost":"192.168.1.144"
   },
   "f6:cf:a2:x1:x1:x2":{
      "SSID1":"MyNet",
      "Password1":"MyPass",
      "Module":"27",
      "MqttHost":"192.168.1.144"
   }
 }`

P.S : The JSON file needs to be in a single line for some reason. This is a draft version and might not be buggy/unoptimized




### License
This program is licensed under GPL-3.0
