#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h> 
#include <json-c/json.h>
#include "rdn_api.h"
#include "debug.h"
#include "rdn_mqtt.h"

MsgHandle_t msg_handle_table[] = {
	{"robot_info",			HandleGetRobotInfo},
	{"robot_diagnosis",		HandleGetDiagInfo},
	{"robot_maintenance", 	HandleGetMaintainInfo},
	{"reboot",				HandleRemoteRestart},
	{"auto_charging",		HandleAutoCharge},		// 自动充电
	{"set_ap_name", 		HandleSetAPName},
	{"set_robot_name", 		HandleSetRobotName},
	{"reconfigure", 		HandleRestoreDefault},	// 恢复默认配置
	{"ota", 				HandleOTAUpgrade},		// 远程OTA升级
	{"EnableAdb", 			HandleAdbEnable},		// adb调试开关
	{"calibrate", 			HandleCalibrate},		// 摄像头标定
	{"remotepair",			HandleRemotepair},		// 遥控器配对
	{"photograph",			HandlePhotograph},		// 录像拍照
};

int FindIndexByType(const char* type)
{
	int i = 0;
	int size = sizeof(msg_handle_table)/sizeof(msg_handle_table[0]);

	for(i=0; i<size; i++)
	{
		if( 0 == strcmp(type, msg_handle_table[i].type))
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

/*
** 获取机器基础信息
*/
void HandleGetRobotInfo(struct mosquitto *mosq, char* payload)
{
	struct json_object * buf = json_object_new_object();
	char topic[64] = "0";

	SpliceJSONString(ROBOT_NODE, "sn", "SN", buf);
	SpliceJSONString(ROBOT_NODE, "name", "robot_name", buf);
	SpliceJSONString(ENABLE_NODE, "adb", "rdn21_adb_enable", buf);
	SpliceJSONString(BATTERY_NODE, "percent_cap", "percent_cap", buf);
	SpliceJSONString(ROBOT_NODE, "currarea", "curr_area", buf);
	SpliceJSONString(ROBOT_NODE, "totalarea", "total_area", buf);
	SpliceJSONString(ROBOT_NODE, "currtime", "curr_time", buf);
	SpliceJSONString(ROBOT_NODE, "usingtime", "total_time", buf);

	// 单位时间清扫面积
	//SpliceJSONString();
	SpliceJSONString(PANEL_NODE, "heigh", "heigh", buf);
	SpliceJSONString(PANEL_NODE, "width", "width", buf);
	SpliceJSONString(PANEL_NODE, "angle", "angle", buf);
	SpliceJSONString(SENSOR_TEMP_NODE, "value", "temp", buf);
	SpliceJSONString(PANEL_NODE, "mode", "hvmode", buf);
	SpliceJSONString(PANEL_NODE, "isleft", "startmode", buf);
	SpliceJSONString(WIRELESS_AP_NODE, "ssid", "apname", buf);
	SpliceJSONString(WIRELESS_AP_NODE, "ip", "ap_ip", buf);
	SpliceJSONString(WIRELESS_WIFI_NODE, "ssid0", "wifi_name", buf);
	SpliceJSONString(WIRELESS_WIFI_NODE, "ip", "wifi_ip", buf);
	SpliceJSONString(WIRELESS_G4_NODE, "simid", "sim_id", buf);
	SpliceJSONString(WIRELESS_G4_NODE, "ip", "sim_ip", buf);
	SpliceJSONString(WIRELESS_G4_NODE, "GPS", "rdn21_gps_gd", buf);
	SpliceJSONString(VERSION_NODE, "sysver", "sys_ver", buf);
	SpliceJSONString(VERSION_NODE, "appver", "app_ver", buf);
	SpliceJSONString(VERSION_NODE, "mcuver", "mcu_ver", buf);
	
	snprintf(topic, sizeof(topic), "STC21/%s/msg/robot_info", sn);
	const char* msg = json_object_to_json_string(buf);
	mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, atoi(qos), false);

	json_object_put(buf);

	return;
}

/*
** 诊断模式下机器的相关数据
*/
void HandleGetDiagInfo(struct mosquitto *mosq, char* payload)
{
	struct json_object * buf = json_object_new_object();
	char topic[64] = "0";

	SpliceJSONString(ROBOT_NODE, "sn", "SN", buf);
	SpliceJSONString(ROBOT_NODE, "reboot_num", "reboot_num", buf);
	SpliceJSONString(ROBOT_NODE, "cpu_usage", "cpu_usage", buf);
	SpliceJSONString(ROBOT_NODE, "cpu_temp1", "cpu_temp", buf);
	SpliceJSONString(SENSOR_GYRO_NODE, "rolling_angle", "rolling_angle", buf);
	SpliceJSONString(SENSOR_GYRO_NODE, "pitch_angle", "pitch_angle", buf);
	SpliceJSONString(SENSOR_GYRO_NODE, "heading_angle", "heading_angle", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "left_detected", "left_ultr_detected", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "right_detected", "right_ultr_detected", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "lf_detected", "lf_ultr_detected", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "rf_detected", "rf_ultr_detected", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "left_ok", "left_ultr_ok", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "right_ok", "right_ultr_ok", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "lf_ok", "lf_ultr_ok", buf);
	SpliceJSONString(SENSOR_ULTR_NODE, "rf_ok", "rf_ultr_ok", buf);
	SpliceJSONString(SENSOR_CODE_NODE, "left_value", "left_cd", buf);
	SpliceJSONString(SENSOR_CODE_NODE, "right_value", "right_cd", buf);
	SpliceJSONString(SENSOR_PRES_NODE, "value", "pressure", buf);
	SpliceJSONString(DIAG_CAME_NODE, "left_open", "is_left_open", buf);
	SpliceJSONString(DIAG_CAME_NODE, "right_open", "is_right_open", buf);
	SpliceJSONString(BATTERY_NODE, "rem_cap", "rem_cap", buf);
	SpliceJSONString(BATTERY_NODE, "vt", "voltage", buf);
	SpliceJSONString(BATTERY_NODE, "current", "current", buf);
	SpliceJSONString(BATTERY_NODE, "bat_t1", "bat_temp", buf);
	SpliceJSONString(BATTERY_NODE, "cycle_count", "cycle_count", buf);
	SpliceJSONString(BATTERY_NODE, "hw_ver", "hw_ver", buf);
	SpliceJSONString(BATTERY_NODE, "sw_ver", "sw_ver", buf);
	SpliceJSONString(DIAG_CURR_NODE, "brush", "brush_current", buf);
	SpliceJSONString(DIAG_CURR_NODE, "leftwheel", "leftwheel_current", buf);
	SpliceJSONString(DIAG_CURR_NODE, "rightwheel", "rightwheel_current", buf);
	SpliceJSONString(DIAG_CURR_NODE, "adsorb", "adsorb_current", buf);

	// update_time && create_by

	snprintf(topic, sizeof(topic), "STC21/%s/msg/diagnose_info", sn);
	const char* msg = json_object_to_json_string(buf);
	mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, atoi(qos), false);

	json_object_put(buf);

	return;
}

/*
** 维护模式下机器的相关数据
*/
void HandleGetMaintainInfo(struct mosquitto *mosq, char* payload)
{
	struct json_object * buf = json_object_new_object();
	char topic[64] = "0";
	char partname[32] = "0";
	char nodename[64] = "0";

	json_object *jobj = json_tokener_parse(payload);
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	json_object *change_obj = json_object_object_get(jobj, "change");
	json_object *name_obj = json_object_object_get(jobj, "part_name");
	if(change_obj && name_obj)
	{
		int change = json_object_get_int(change_obj);

		switch(change)
		{
			case 0:
				strcpy(partname, json_object_get_string(name_obj));
				snprintf(nodename, sizeof(nodename), "root.maintain.%s", partname);

				SpliceJSONString(ROBOT_NODE, "sn", "SN", buf);
				json_object_object_add(buf, "part_name", json_object_new_string(partname));
				SpliceJSONString(nodename, "changetime", "part_change_time", buf);
				SpliceJSONString(nodename, "changecount", "part_change_count", buf);
				SpliceJSONString(nodename, "Usingtime", "part_using_time", buf);
				SpliceJSONString(nodename, "recoder", "part_changer", buf);

				snprintf(topic, sizeof(topic), "STC21/%s/msg/maintenance_info", sn);
				const char* msg = json_object_to_json_string(buf);
				mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, atoi(qos), false);

				break;
			case 1:
				printf("wait do!!!!!!!!!!!!!");
				break;
			default:
				printf("change error!!!!!!!!");
		}
	}

	return;
}

/*
** 工作模式下，地图的轨迹数据
*/
void HandleGetMapInfo(struct mosquitto *mosq, char* payload)
{
	return;


}

/*
** 远程重启
*/
void HandleRemoteRestart(struct mosquitto *mosq, char* payload)
{
	system("reboot");

	return;
}

/***
 * 
 * 设置ap名称
 * 
*/
void HandleSetAPName(struct mosquitto *mosq, char* payload)
{
	char ap_name[32] = {0};
	json_object *jobj = json_tokener_parse(payload);
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	json_object * json_obj = json_object_object_get(jobj, "ap_name");
	if(json_obj)
	{
		strcpy(ap_name, json_object_get_string(json_obj));
	}

	rdn_set(WIRELESS_AP_NODE, "ssid", ap_name);

	return;
}

/***
 * 
 * 自动充电
 * 
*/
void HandleAutoCharge(struct mosquitto *mosq, char* payload)
{
	return;
}

/***
 * 
 * 设置机器人名称
 * 
*/
void HandleSetRobotName(struct mosquitto *mosq, char* payload)
{
	char robot_name[32] = {0};
	json_object *jobj = json_tokener_parse(payload);
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	json_object * json_obj = json_object_object_get(jobj, "robot_name");
	if(json_obj)
	{
		strcpy(robot_name, json_object_get_string(json_obj));
	}

	rdn_set(ROBOT_NODE, "name", robot_name);

	return;
}

/***
 * 
 * 回复默认配置
 * 
*/
void HandleRestoreDefault(struct mosquitto *mosq, char* payload)
{

	return;
}

/***
 * 
 * 远程OTA升级
 * 
*/
void HandleOTAUpgrade(struct mosquitto *mosq, char* payload)
{

	return;
}

/***
 * 
 * adb
 * 
*/
void HandleAdbEnable(struct mosquitto *mosq, char* payload)
{
	char adbenbale[8] = {0};
	json_object *jobj = json_tokener_parse(payload);
	if(jobj == NULL)
	{
		printf("Invalid JSON message.\n");
		return;
	}

	json_object * json_obj = json_object_object_get(jobj, "baseValue");
	if(json_obj)
	{
		strcpy(adbenbale, json_object_get_string(json_obj));
	}

	rdn_set(ENABLE_NODE, "adb", adbenbale);

	return;
}

/***
 * 
 * 摄像头标定
 * 
*/
void HandleCalibrate(struct mosquitto *mosq, char* payload)
{

	return;
}

/***
 * 
 * 遥控器配对
 * 
*/
void HandleRemotepair(struct mosquitto *mosq, char* payload)
{

	return;
}

/***
 * 
 * 录像拍照
 * 
*/
void HandlePhotograph(struct mosquitto *mosq, char* payload)
{

	return;
}

void RdnHandleMsg(struct mosquitto *mosq, char* msg)
{
	int idx = -1;
	const char* type = NULL; 
	json_object *jobj = json_tokener_parse(msg);
    if (jobj == NULL) {
        printf("Invalid JSON message.\n");
        return;
    }

	json_object * type_obj = json_object_object_get(jobj, "cmd");
    if (type_obj)
	{
		type = json_object_get_string(type_obj);
		if(type)
		{
			idx = FindIndexByType(type);
			if(FindIndexByType(type) >= 0)
			{
				msg_handle_table[idx].HandleFun(mosq, msg);
			}
		}
    }
	
    json_object_put(jobj);
	return;
}

