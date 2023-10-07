/**
 * @file rdn_mqtt.h
 * @author Lei.Zhang
 * @email zhanglei@radiantpv.com
 * @version V0.0.0
 * @date 2022-3-9
 * @license 2014-2021,Radiant Solar Technology Co., Ltd.
**/ 
#ifndef _RDNMQTT_H_
#define _RDNMQTT_H_

#include <pthread.h>
#include "mosquitto.h"
#include "json-c/json.h"

extern pthread_t rdn_mqtt_sub_th;
extern pthread_t rdn_mqtt_pub_th;
extern struct mosquitto *mosq;

extern char host[32];
extern char port[16];
extern char account[32];
extern char passwd[32];
extern char keepalive[8];
extern char qos[4];
extern char sn[32];

typedef struct MsgHandle_s{
	char type[32];
	char replyinfo[64];
	void (*HandleFun)(struct mosquitto*, json_object*);
}MsgHandle_t;

int FindIndexByType(const char* type, MsgHandle_t msg_table[], int size);
void SpliceJSONString(char* node, char* node_name, char* key, struct json_object * buf);
void RdnReplyMsg(char* msg);
void RdnMqttPubMsg(char* topic, const char* msg);
void* RdnMqttSubClient(void* p);
void* RdnMqttPubClient(void* p);

/**********************Following:Handle sub msg**********************************/
void RdnHandleSubMsg(struct mosquitto *mosq, char* msg);
void HandleRemotectrlMsg(struct mosquitto *mosq, json_object *jobj);
void HandleRequestMsg(struct mosquitto *mosq, json_object *jobj);
void HandleSettingMsg(struct mosquitto *mosq, json_object *jobj);
void HandleDebugMsg(struct mosquitto *mosq, json_object *jobj);
void HandleOtaUpgradeMsg(struct mosquitto *mosq, json_object *jobj);

/***********************Following:Handle pub msg********************************/
void HandleErrorMsg(void);

/*******************************************************************************/
void HandleRemotectrlMovement(struct mosquitto *mosq, json_object *jobj);
void HandleRemotectrlCharging(struct mosquitto *mosq, json_object *jobj);
void HandleRemotectrlReboot(struct mosquitto *mosq, json_object *jobj);



#endif // _RDNMQTT_H_
