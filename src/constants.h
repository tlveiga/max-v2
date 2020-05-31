#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define FWCODE "leena"
#define FWVERSION "1.1.1"

// FILES
#define INFOFILE "/info.json"
#define MQTTFILE "/mqtt.json"
#define WIFIFILE "/wifi.json"

#define DEFAULTUPDATESERVER "http://tlv-ubuntu.westeurope.cloudapp.azure.com/fw"
#define DEFAULTMQTTSERVER "tlv-ubuntu.westeurope.cloudapp.azure.com"

#define MAXJSONFILESIZE 1024

#define UPDATELOOPTIMESPAN 10000
#define MINRSSILEVEL -200
#define FWCHECKTIMESPAN 3600000
#define MQTTRECONNECTTIMESPAN 10000
// com delay de 200ms
#define CONNECTIONRETRIES 50

#define R_SUCCESS "{\"result\":\"success\"}"
#define R_FAILED "{\"result\":\"failed\"}"
#define R_OK "{\"result\":\"ok\"}"
#define R_NOK "{\"result\":\"nok\"}"

#endif