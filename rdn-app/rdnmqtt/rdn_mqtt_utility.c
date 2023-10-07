#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"

pthread_t rdn_mqtt_sub_th;
pthread_t rdn_mqtt_pub_th;

int FindIndexByType(const char* type, MsgHandle_t msg_table[], int size)
{
	int i = 0;
	
	for(i=0; i<size; i++)
	{
		if( 0 == strcmp(type, msg_table[i].type))
		{
			return i;		
		}
	}

	return -1;
}

void SpliceJSONString(char* node, char* node_name, char* key, struct json_object * buf)
{
	char tmp[32] = "0";
	rdn_get(node, node_name, tmp, sizeof(tmp));
	json_object_object_add(buf, key, json_object_new_string(tmp));

	return;
}

void RdnMqttPubMsg(char* topic, const char* msg)
{
	LOG_DBG("Pub msg:topic=[%s],msg=%s====>\n",topic, msg);
	mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, atoi(qos), false);	

	return;
}

void RdnReplyMsg(char* msg)
{
	char topic[64] = {0};
	const char* message = NULL;
	struct json_object* obj = NULL;
	
	if (strlen(msg) != 0)
	{
		obj = json_object_new_object();
		memset(topic, 0, sizeof(topic));
		snprintf(topic, sizeof(topic), "STC21/%s/msg/reply_info", sn);

		json_object_object_add(obj, "SN", json_object_new_string(sn));
		json_object_object_add(obj, "reply_info", json_object_new_string(msg));

		message = json_object_to_json_string(obj);
		RdnMqttPubMsg(topic, message);
		json_object_put(obj);
	}

	return;
}
