#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"
#include "rdn_mq.h"

void HandleDebugReset(struct mosquitto *mosq, json_object *jobj);
void HandleDebugCalibrate(struct mosquitto *mosq, json_object *jobj);
void HandleDebugRemotepair(struct mosquitto *mosq, json_object *jobj);
void HandleDebugSaveimg(struct mosquitto *mosq, json_object *jobj);
void HandleDebugAdb(struct mosquitto *mosq, json_object *jobj);

MsgHandle_t handle_debug_table[] = {
	{"reset",  		"MSG_RESET_OK",		HandleDebugReset},
	{"calibrate",  	"MSG_CALIBRATE_OK", HandleDebugCalibrate},
	{"remotepair",  "MSG_REMOTEPAIR_OK",HandleDebugRemotepair},
	{"saveimg",	  	"MSG_SAVEING_OK",  	HandleDebugSaveimg},
	{"adb",  		"MSG_ADB_OK",  		HandleDebugAdb}
};

void HandleDebugReset(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	return;
}

void HandleDebugCalibrate(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_CALIBRATE, NULL);
	return;
}

void HandleDebugRemotepair(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_REMOTE_PAIR, NULL);
	return;
}

void HandleDebugSaveimg(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	return;
}

void HandleDebugAdb(struct mosquitto *mosq, json_object *jobj)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
	return;
}

void HandleDebugMsg(struct mosquitto *mosq, json_object *jobj)
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
			size = sizeof(handle_debug_table)/sizeof(handle_debug_table[0]);
			idx = FindIndexByType(type, handle_debug_table, size );
			if(idx >= 0)
			{
				handle_debug_table[idx].HandleFun(mosq, jobj);
				RdnReplyMsg(handle_debug_table[idx].replyinfo);
			}
		}
	}
	
	return;
}

