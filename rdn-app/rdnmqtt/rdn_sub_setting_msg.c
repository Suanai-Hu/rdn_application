#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"

void HandleSetApname(struct mosquitto *mosq, json_object *jobj);
void HandleSetRobotname(struct mosquitto *mosq, json_object *jobj);
void HandleSetMaintain(struct mosquitto *mosq, json_object *jobj);
void HandleSetSpeed(struct mosquitto *mosq, json_object *jobj);
void HandleSetThreshold(struct mosquitto *mosq, json_object *jobj);

MsgHandle_t handle_setting_table[] = {
	{"maintain",  			"MSG_SETTING_MAINTAIN_OK",  	HandleSetMaintain},
	{"ap_name",  			"MSG_SETTING_APNAME_OK",  		HandleSetApname},
	{"robot_name",  		"MSG_SETTING_ROBOTNAME_OK",  	HandleSetRobotname},
	{"left_direct_speed",  	"MSG_SETTING_LEFT_DIRECT_OK",  	HandleSetSpeed},
	{"right_direct_speed",  "MSG_SETTING_RIGHT_DIRECT_OK",  HandleSetSpeed},
	{"left_turn_speed",  	"MSG_SETTING_LEFT_TURN_OK",  	HandleSetSpeed},
	{"right_turn_speed",  	"MSG_SETTING_RIGHT_TURN_OK",  	HandleSetSpeed},
	{"ultr_threshold",  	"MSG_SETTING_ULTRASONIC_OK",  	HandleSetThreshold},
	{"pressure_threshold",  "MSG_SETTING_PRESSURESENSOR_OK",HandleSetThreshold},
	{"lifting_threshold",  	"MSG_SETTING_LIFTINGMOTOR_OK",  HandleSetThreshold}
};

void HandleSetApname(struct mosquitto *mosq, json_object *jobj)
{
	char buf[32] = {0};
	const char* msg = NULL;
	char topic[64] = "0";
	json_object * obj = NULL;

	LOG_DBG("Enter====>\n");
	
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	obj = json_object_object_get(jobj, "value");
	if(obj)
	{
		strncpy(buf, json_object_get_string(obj), sizeof(buf)-1);
	}

	rdn_set(WL_AP_NODE, "ssid", buf);

	return;
}

void HandleSetRobotname(struct mosquitto *mosq, json_object *jobj)
{
	const char* msg = NULL;
	char buf[32] = {0};
	char topic[64] = "0";
	json_object * obj = NULL;

	LOG_DBG("Enter====>\n");
	
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	obj = json_object_object_get(jobj, "value");
	if(obj)
	{
		strncpy(buf, json_object_get_string(obj), sizeof(buf)-1);
	}

	rdn_set(ROBOT_NODE, "name", buf);

	return;
}

void HandleSetMaintain(struct mosquitto *mosq, json_object *jobj)
{
	char buf[32] = {0};
	char count[10] = {0};
	char node[32] = {0};
	char topic[64] = "0";
	const char* msg = NULL;
	char* change_time = "2023-08-01";
	const char* part_name = NULL;
	const char* part_changer = NULL;

	json_object * obj = NULL;
	json_object * vobj = NULL;

	LOG_DBG("Enter====>\n");

	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	obj = json_object_object_get(jobj, "value");
	if(obj)
	{
		vobj = json_object_object_get(obj, "part_name");
		if(vobj)
		{
			part_name = json_object_get_string(vobj);
		}

		vobj = json_object_object_get(obj, "part_changer");
		if(vobj)
		{
			part_changer = json_object_get_string(vobj);
		}

		memset(node, 0, sizeof(node));
		snprintf(node, sizeof(node), MAINTAIN_PART_NODE, part_name);

		rdn_get(node, "changecount", count, sizeof(count));
		snprintf(buf, sizeof(buf), "%d", atoi(count)+1);
		rdn_set(node, "changecount", buf);
		strncpy(buf, part_changer, sizeof(buf)-1);
		rdn_set(node, "recoder", buf);
		rdn_set(node, "changetime", change_time);
		rdn_set(node, "lasttime", "0");
		rdn_set(node, "usingtime", "0");
		rdn_set(node, "timestring", "0");

		obj = json_object_new_object();

		SpliceJSONString(ROBOT_NODE, "sn", "SN", obj);
		json_object_object_add(obj, "part_name", json_object_new_string(part_name));
		SpliceJSONString(node, "changecount", "change_count", obj);
		SpliceJSONString(node, "usingtime", "using_time", obj);
		SpliceJSONString(node, "recoder", "recoder", obj);
		msg = json_object_to_json_string(obj);

		memset(topic, 0, sizeof(topic));
		snprintf(topic, sizeof(topic), "STC21/%s/msg/maintenance_info", sn);
		RdnMqttPubMsg(topic, msg);
		json_object_put(obj);
	}

	return;
}

void HandleSetSpeed(struct mosquitto *mosq, json_object *jobj)
{
	char buf[32] = {0};
	char topic[64] = "0";
	const char* type = NULL;
	const char* value = NULL;
	const char* msg = NULL;
	json_object * obj = NULL;

	LOG_DBG("Enter====>\n");
	
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	obj = json_object_object_get(jobj, "type");
	if(obj)
	{
		type = json_object_get_string(obj);
	}
	
	obj = json_object_object_get(jobj, "value");
	if(obj)
	{
		value = json_object_get_string(obj);
		strncpy(buf, value, sizeof(buf)-1);
	}

	if(strcmp(type, "left_direct_speed") == 0)
	{
		rdn_set(SPORT_DIRECT_PARA_NODE, "left_speed", buf);
	}
	else if(strcmp(type, "right_direct_speed") == 0)
	{
		rdn_set(SPORT_DIRECT_PARA_NODE, "right_speed", buf);
	}
	else if(strcmp(type, "left_turn_speed") == 0)
	{
		rdn_set(SPORT_TURN_PARA_NODE, "left_speed", buf);
	}
	else if(strcmp(type, "right_turn_speed") == 0)
	{
		rdn_set(SPORT_TURN_PARA_NODE, "right_speed", buf);
	}

	return;
}

void HandleSetThreshold(struct mosquitto *mosq, json_object *jobj)
{
	char buf[32] = {0};
	char topic[64] = "0";
	const char* msg = NULL;
	const char* type = NULL;
	const char* value = NULL;
	json_object * obj = NULL;

	LOG_DBG("Enter====>\n");
	
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	obj = json_object_object_get(jobj, "type");
	if(obj)
	{
		type = json_object_get_string(obj);
	}
	
	obj = json_object_object_get(jobj, "value");
	if(obj)
	{
		value = json_object_get_string(obj);
		strncpy(buf, value, sizeof(buf)-1);
	}

	if(strcmp(type, "ultr_threshold") == 0)
	{
		rdn_set(SENSOR_THD_NODE, "ultrasonic", buf);
	}
	else if(strcmp(type, "pressure_threshold") == 0)
	{
		rdn_set(SENSOR_THD_NODE, "pressure", buf);
	}
	else if(strcmp(type, "lifting_threshold") == 0)
	{
		rdn_set(SENSOR_THD_NODE, "liftmotor", buf);
	}

	return;
}

void HandleSettingMsg(struct mosquitto *mosq, json_object *jobj)
{
	int idx = -1;
	int size = 0;
	const char* type = NULL; 
	json_object* obj = NULL;

	LOG_DBG("Enter====>\n");

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
			size = sizeof(handle_setting_table)/sizeof(handle_setting_table[0]);
			idx = FindIndexByType(type, handle_setting_table, size );
			if(idx >= 0)
			{
				handle_setting_table[idx].HandleFun(mosq, jobj);
				RdnReplyMsg(handle_setting_table[idx].replyinfo);
			}
		}
	}
	
	return;
}

