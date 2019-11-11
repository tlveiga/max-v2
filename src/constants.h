#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define FWCODE "max"
#define FWVERSION "0.0.1"
#define FWDATE 20191030

// FILES
#define UIVERSION "/version.json"
#define INFOFILE "/info.json"
#define MQTTFILE "/mqtt.json"
#define WIFIFILE "/wifi.json"

// CONFIG ADDRESSEs
#define CONTROL_ADDR 0x8C000
#define CONTROL_VALUE (uint32_t)0xf1f0
#define INFO_CONFIG_ADDR (0x8C000 + sizeof(CONTROL_VALUE))
#define INFO_CONFIG_SIZE 0x200
#define MQTT_CONFIG_ADDR (INFO_CONFIG_ADDR + INFO_CONFIG_SIZE)
#define MQTT_CONFIG_SIZE 0x200
#define WIFI_CONFIG_ADDR (MQTT_CONFIG_ADDR + MQTT_CONFIG_SIZE)
#define WIFI_CONFIG_SIZE 0x1000

#define DEFAULTUPDATESERVER "http://tlv-ubuntu.westeurope.cloudapp.azure.com/fw"
#define DEFAULTMQTTSERVER "http://tlv-ubuntu.westeurope.cloudapp.azure.com/fw"

#define MAXJSONFILESIZE 1024

#define R_SUCCESS "{\"result\":\"success\"}"

#endif