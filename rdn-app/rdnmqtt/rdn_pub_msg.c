#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mqtt.h"

void PkgMcuErrorInfoJson(int error_code, struct json_object *obj)
{
	char node[32] = {0};

	memset(node, 0, sizeof(node));
	snprintf(node, sizeof(node), ERROR_MCU_TYPE_NODE, error_code);
	SpliceJSONString(ROBOT_NODE, "sn", "SN", obj);
	SpliceJSONString(node, "code", "error_code", obj);
	SpliceJSONString(node, "desc", "error_desc", obj);

	return;
}

void PkgSysErrorInfoJson(int error_code, struct json_object *obj)
{
	char node[32] = {0};

	memset(node, 0, sizeof(node));
	snprintf(node, sizeof(node), ERROR_SYS_TYPE_NODE, error_code);
	SpliceJSONString(ROBOT_NODE, "sn", "SN", obj);
	SpliceJSONString(node, "code", "error_code", obj);
	SpliceJSONString(node, "desc", "error_desc", obj);

	return;
}

void HandleErrorMsg(void)
{	
	int i = 0;
	int len = 0;
	char topic[64] = "0";
	char buf[64] = {0};
	unsigned long long mask0 = 0;
	unsigned long long mask1 = 0;
	struct json_object* obj = NULL;
	const char* msg  = NULL;
	char error_list[64] = {0};
	
	LOG_LOOP("Enter======>\n");
	memset(topic, 0, sizeof(topic));
	snprintf(topic, sizeof(topic), "STC21/%s/msg/error_info", sn);

	memset(error_list, 0, sizeof(error_list));
	memset(buf, 0, sizeof(buf));
	rdn_get(ERROR_MCU_NODE, "mask0", buf, sizeof(buf));
	mask0 = atoll(buf);
	if(mask0)
	{
		/*error 1~32*/
		for(i=0; i<32; i++)
		{
			if(GET_BIT(mask0, i))
			{	
				len += snprintf(error_list + len, sizeof(error_list)-len, "%d_", i+1);
				obj = json_object_new_object();
				PkgMcuErrorInfoJson(i+1, obj);
				msg = json_object_to_json_string(obj);
				RdnMqttPubMsg(topic, msg);
				json_object_put(obj);
			}
		}
		rdn_set(ERROR_MCU_NODE, "mask0", "0");
	}

	memset(buf, 0, sizeof(buf));
	rdn_get(ERROR_MCU_NODE, "mask1", buf, sizeof(buf));
	mask1 = atoll(buf);
	if(mask1)
	{
		/*error 32~64*/
		for(i=0; i<32; i++)
		{	
			if(GET_BIT(mask1, i))
			{
				len += snprintf(error_list + len, sizeof(error_list)-len, "%d_", i+32+1);
				obj = json_object_new_object();
				PkgMcuErrorInfoJson(i+32+1, obj);
				msg = json_object_to_json_string(obj);
				RdnMqttPubMsg(topic, msg);
				json_object_put(obj);
			}
		}
		rdn_set(ERROR_MCU_NODE, "mask1", "0");
	}

	if(error_list[0] != '\0')
	{
		error_list[strlen(error_list)-1] = '\0';
		rdn_set(ERROR_MCU_NODE, "list", error_list);	
	}

/*********************************SYS ERROR*****************************************/
	memset(error_list, 0, sizeof(error_list));
	memset(buf, 0, sizeof(buf));
	rdn_get(ERROR_SYS_NODE, "mask0", buf, sizeof(buf));
	mask0 = atoll(buf);
	if(mask0)
	{
		/*error 1~32*/
		for(i=0; i<32; i++)
		{
			if(GET_BIT(mask0, i))
			{	
				len += snprintf(error_list + len, sizeof(error_list)-len, "%d_", i+1);
				obj = json_object_new_object();
				PkgSysErrorInfoJson(i+1, obj);
				msg = json_object_to_json_string(obj);
				RdnMqttPubMsg(topic, msg);
				json_object_put(obj);
			}
		}
		rdn_set(ERROR_SYS_NODE, "mask0", "0");
	}

	memset(buf, 0, sizeof(buf));
	rdn_get(ERROR_SYS_NODE, "mask1", buf, sizeof(buf));
	mask1 = atoll(buf);
	if(mask1)
	{
		/*error 32~64*/
		for(i=0; i<32; i++)
		{	
			if(GET_BIT(mask1, i))
			{
				len += snprintf(error_list + len, sizeof(error_list)-len, "%d_", i+32+1);
				obj = json_object_new_object();
				PkgSysErrorInfoJson(i+32+1, obj);
				msg = json_object_to_json_string(obj);
				RdnMqttPubMsg(topic, msg);
				json_object_put(obj);
			}
		}
		rdn_set(ERROR_SYS_NODE, "mask1", "0");
	}

	if(error_list[0] != '\0')
	{
		error_list[strlen(error_list)-1] = '\0';
		rdn_set(ERROR_SYS_NODE, "list", error_list);	
	}
	

	return;
}


