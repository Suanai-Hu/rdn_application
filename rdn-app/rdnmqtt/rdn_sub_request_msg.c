#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"

void HandleRequestRobotInfo(struct mosquitto *mosq, json_object *jobj);
void HandleRequestDiagnosis(struct mosquitto *mosq, json_object *jobj);

MsgHandle_t handle_request_table[] = {
	{"robotinfo",  "MSG_REQUEST_ROBOTINFO_OK",  HandleRequestRobotInfo},
	{"diagnosis",  "MSG_REQUEST_DIAGNOSIS_OK",  HandleRequestDiagnosis}	
};

void HandleRequestRobotInfo(struct mosquitto *mosq, json_object *jobj)
{
	char topic[64] = "0";
	const char* msg  = NULL;
	struct json_object * obj = json_object_new_object();

	LOG_WARN("Enter======>\n");

	SpliceJSONString(ROBOT_NODE, "sn", "SN", obj);
	SpliceJSONString(ROBOT_NODE, "name", "robot_name", obj);
	SpliceJSONString(ENABLE_NODE, "adb", "rdn21_adb_enable", obj);
	SpliceJSONString(BATTERY_NODE, "percent_cap", "percent_cap", obj);
	SpliceJSONString(ROBOT_NODE, "currarea", "curr_area", obj);
	SpliceJSONString(ROBOT_NODE, "totalarea", "total_area", obj);
	SpliceJSONString(ROBOT_NODE, "currtime", "curr_time", obj);
	SpliceJSONString(ROBOT_NODE, "usingtime", "total_time", obj);
	SpliceJSONString(ROBOT_NODE, "unitarea", "rt_unit_area", obj);
	SpliceJSONString(PANEL_NODE, "heigh", "heigh", obj);
	SpliceJSONString(PANEL_NODE, "width", "width", obj);
	SpliceJSONString(PANEL_NODE, "angle", "angle", obj);
	SpliceJSONString(SENSOR_TEMP_NODE, "value", "temp", obj);
	SpliceJSONString(PANEL_NODE, "mode", "hvmode", obj);
	SpliceJSONString(TRACE_NODE, "isleft", "startmode", obj);
	SpliceJSONString(WL_AP_NODE, "ssid", "ap_name", obj);
	SpliceJSONString(WL_AP_NODE, "ip", "ap_ip", obj);
	SpliceJSONString(WL_WIFI_NODE, "loging_ssid", "wifi_name", obj);
	SpliceJSONString(WL_WIFI_NODE, "ip", "wifi_ip", obj);
	SpliceJSONString(WL_G4_NODE, "simid", "sim_id", obj);
	SpliceJSONString(WL_G4_NODE, "ip", "sim_ip", obj);
	SpliceJSONString(WL_G4_NODE, "GPS", "rdn21_gps_wgs84", obj);
	SpliceJSONString(VERSION_NODE, "sysver", "sys_ver", obj);
	SpliceJSONString(VERSION_NODE, "appver", "app_ver", obj);
	SpliceJSONString(VERSION_NODE, "mcuver", "mcu_ver", obj);
	msg = json_object_to_json_string(obj);

	memset(topic, 0, sizeof(topic));
	snprintf(topic, sizeof(topic), "STC21/%s/msg/robot_info", sn);
	RdnMqttPubMsg(topic, msg);
	json_object_put(obj);

	return;
}

void HandleRequestDiagnosis(struct mosquitto *mosq, json_object *jobj)
{
	char topic[64] = "0";
	const char* msg  = NULL;
	struct json_object * obj = json_object_new_object();

	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);

	SpliceJSONString(ROBOT_NODE, "sn", "SN", obj);
	SpliceJSONString(ROBOT_NODE, "reboot_num", "reboot_num", obj);
	SpliceJSONString(ROBOT_NODE, "cpu_usage", "cpu_usage", obj);
	SpliceJSONString(ROBOT_NODE, "cpu_temp1", "cpu_temp", obj);
	SpliceJSONString(SENSOR_GYRO_NODE, "rolling_angle", "rolling_angle", obj);
	SpliceJSONString(SENSOR_GYRO_NODE, "pitch_angle", "pitch_angle", obj);
	SpliceJSONString(SENSOR_GYRO_NODE, "heading_angle", "heading_angle", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "left_detected", "left_ultr_detected", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "right_detected", "right_ultr_detected", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "lf_detected", "lf_ultr_detected", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "rf_detected", "rf_ultr_detected", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "left_ok", "left_ultr_ok", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "right_ok", "right_ultr_ok", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "lf_ok", "lf_ultr_ok", obj);
	SpliceJSONString(SENSOR_ULTR_NODE, "rf_ok", "rf_ultr_ok", obj);
	SpliceJSONString(SENSOR_CD_NODE, "left_value", "left_cd", obj);
	SpliceJSONString(SENSOR_CD_NODE, "right_value", "right_cd", obj);
	SpliceJSONString(SENSOR_ADSORB_NODE, "pressure", "pressure", obj);
	SpliceJSONString(SENSOR_CAM_NODE, "left_open", "is_left_open", obj);
	SpliceJSONString(SENSOR_CAM_NODE, "right_open", "is_right_open", obj);
	SpliceJSONString(BATTERY_NODE, "rem_cap", "rem_cap", obj);
	SpliceJSONString(BATTERY_NODE, "vt", "voltage", obj);
	SpliceJSONString(BATTERY_NODE, "current", "current", obj);
	SpliceJSONString(BATTERY_NODE, "bat_t1", "bat_temp", obj);
	SpliceJSONString(BATTERY_NODE, "cycle_count", "cycle_count", obj);
	SpliceJSONString(BATTERY_NODE, "hw_ver", "hw_ver", obj);
	SpliceJSONString(BATTERY_NODE, "sw_ver", "sw_ver", obj);
	SpliceJSONString(SENSOR_BRUSH_NODE, "value", "brush_current", obj);
	SpliceJSONString(SENSOR_WHEEL_NODE, "left_value", "leftwheel_current", obj);
	SpliceJSONString(SENSOR_WHEEL_NODE, "right_value", "rightwheel_current", obj);
	SpliceJSONString(SENSOR_ADSORB_NODE, "airpump_curr", "adsorb_current", obj);
	msg = json_object_to_json_string(obj);

	memset(topic, 0, sizeof(topic));
	snprintf(topic, sizeof(topic), "STC21/%s/msg/diagnose_info", sn);
	RdnMqttPubMsg(topic, msg);
	json_object_put(obj);

	return;
}

void HandleRequestMsg(struct mosquitto *mosq, json_object *jobj)
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
			size = sizeof(handle_request_table)/sizeof(handle_request_table[0]);
			idx = FindIndexByType(type, handle_request_table, size );
			if(idx >= 0)
			{
				handle_request_table[idx].HandleFun(mosq, jobj);
				RdnReplyMsg(handle_request_table[idx].replyinfo);
			}
		}
	}
	
	return;
}

