#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"
#include "rdn_mq.h"

void HandleRemotectrlMovement(struct mosquitto *mosq, json_object *jobj);
void HandleRemotectrlCharging(struct mosquitto *mosq, json_object *jobj);
void HandleRemotectrlReboot(struct mosquitto *mosq, json_object *jobj);

MsgHandle_t handle_remotectrl_table[] = {
	{"movement",  	"MSG_REMOTECTRL_MOVEMENT_OK",  	HandleRemotectrlMovement},
	{"charging",  	"MSG_CHARGING_OK",  			HandleRemotectrlCharging},	
	{"reboot",  	"MSG_REBOOT_OK",  				HandleRemotectrlReboot}
};

void HandleRemotectrlMovement(struct mosquitto *mosq, json_object *jobj)
{
	json_object * obj = NULL;
	char direct_value[10] = {0};

	LOG_WARN("Enter======>\n");	
	obj = json_object_object_get(jobj, "value");
	snprintf(direct_value, sizeof(direct_value), "%s", json_object_get_string(obj));
	rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_MOVEMENT, direct_value);
	return;
}

void HandleRemotectrlCharging(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_CHARGING, NULL);
	return;
}

void HandleRemotectrlReboot(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_UPGRADE_MCU, NULL);
	return;
}

void HandleRemotectrlMsg(struct mosquitto *mosq, json_object *jobj)
{
	int idx = -1;
	int size = 0;
	const char* type = NULL; 
	json_object* obj = NULL;

	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	
	if (jobj == NULL) 
	{
		LOG_WARN("Invalid JSON message.\n");
		return;
	}

	obj = json_object_object_get(jobj, "type");
	if(obj)
	{
		type = json_object_get_string(obj);
		if(type)
		{
			size = sizeof(handle_remotectrl_table)/sizeof(handle_remotectrl_table[0]);
			idx = FindIndexByType(type, handle_remotectrl_table, size );
			if(idx >= 0)
			{
				handle_remotectrl_table[idx].HandleFun(mosq, jobj);
				RdnReplyMsg(handle_remotectrl_table[idx].replyinfo);
			}
		}
	}
	
	return;
}



