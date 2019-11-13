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

#define DEFAULTUPDATESERVER "http://tlv-ubuntu.westeurope.cloudapp.azure.com/fw"
#define DEFAULTMQTTSERVER "http://tlv-ubuntu.westeurope.cloudapp.azure.com/fw"

#define MAXJSONFILESIZE 1024

#define UPDATELOOPTIMESPAN 10000

#define R_SUCCESS "{\"result\":\"success\"}"
#define R_FAILED "{\"result\":\"failed\"}"
#define R_OK "{\"result\":\"ok\"}"
#define R_NOK "{\"result\":\"nok\"}"

#endif