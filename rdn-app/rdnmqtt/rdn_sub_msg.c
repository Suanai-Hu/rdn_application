#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"

MsgHandle_t msg_handle_table[] = {
	{"remotectrl",  "", HandleRemotectrlMsg},
	{"request",  	"", HandleRequestMsg},
	{"setting",  	"", HandleSettingMsg},
	{"debug",  		"", HandleDebugMsg},
	{"ota",  		"", HandleOtaUpgradeMsg}
};

void RdnHandleSubMsg(struct mosquitto *mosq, char* msg)
{
	int idx = -1;
	int size = 0;
	const char* type = NULL; 
	json_object* obj = NULL;
	json_object *jobj = json_tokener_parse(msg);
	
    if (jobj == NULL) 
	{
        LOG_WARN("Invalid JSON message.\n");
        return;
    }

	obj = json_object_object_get(jobj, "cmd");
    if(obj)
	{
		type = json_object_get_string(obj);
		if(type)
		{
			size = sizeof(msg_handle_table)/sizeof(msg_handle_table[0]);
			idx = FindIndexByType(type, msg_handle_table, size );
			if(idx >= 0)
			{
				msg_handle_table[idx].HandleFun(mosq, jobj);
			}
		}
    }
	
    json_object_put(jobj);
	return;
}

