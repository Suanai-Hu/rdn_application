/**
 * @file web_data.cpp
 * @brief 本文件主要进行数据的定时更新及发送
 * @author Lei.Zhang
 * @email zhanglei@radiantpv.com
 * @version V0.0.0
 * @date 2022-06-18
 * @license 2014-2022,Radiant Solar Technology Co., Ltd.
**/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
#include "rdn_api.h"
#include "curl.h"
#include "json.h"
#include "md5.h"
#include "node.h"

#define HTTP_GET_RCV_FILE 	"/tmp/get_rcv_file.txt"
#define HTTP_POST_RCV_FILE 	"/tmp/post_rcv_file.txt"
#define POST_JSON_FILE		"/tmp/web.json"
#define UPLOAD_FLAG_FILE	"/tmp/upload"
#define FILE_UPLOAD_CFG		"/userdata/cfg/upload.cfg"

char http_get_url[64] = "192.168.4.222:9022/users?u=admin&m=s";
char http_post_url[64] = "192.168.4.226:9022/robot";
int cycle_time = 5;

size_t receive_data(void *buffer, size_t size, size_t nmemb, FILE *file) 
{
	size_t r_size = fwrite(buffer, size, nmemb, file);
	return r_size;
}

void genMD5(unsigned char* output_md5, const char* msg)
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, msg, strlen(msg));
	MD5_Final(output_md5, &ctx);
}

void md5test(void)
{
	//128bit = 16char = 32位16进制数
	unsigned char output_md5[16]; 
	memset(output_md5,0,sizeof(output_md5));
	const char * msg = "hello";	

	genMD5(output_md5, msg);

	int i = 0;
	for(i = 0; i < 16; i++)
	{
		printf("%02X",output_md5[i]);
	}
	printf("\n");

	return;
}

int get_md5_token(char* md5_token_str, int size)
{
	int i = 0;
	int len = 0;
	FILE* fp = NULL;
	const char* salt_data = NULL;
	char* username = "admin";
	char* psw = "1234";
	char token[64] = {0};
	unsigned char md5_token[16] = {0};
	unsigned char md5_psw[16] = {0}; 
	struct json_object* json_obj = NULL;
	struct json_object* obj = NULL;

	if(md5_token_str == NULL)
	{
		return -1;
	}
	
	json_obj = json_object_from_file(HTTP_GET_RCV_FILE);

	if(json_obj == NULL)
	{
		LOG_LOOP("Get json obj from file[%s] faild====>\n",HTTP_GET_RCV_FILE);
		return -1;
	}
	obj = json_object_object_get(json_obj, "data");
	if(obj)
	{	
		salt_data = json_object_get_string(obj);
	}
	else
	{
		LOG_ERROR("Get salt data json obj faild====>\n");
		json_object_put(json_obj);
		return -1;
	}

	/*encrypt psw*/
	memset(md5_psw, 0, sizeof(md5_psw));	
	genMD5(md5_psw, psw);

	/*Create token string */ 
	len += snprintf(token+len, sizeof(token) - len, "%s", username);
	for(i = 0; i < 16; i++)
	{
		len += snprintf(token + len, sizeof(token) - len, "%02X", md5_psw[i]);
	}
	len += snprintf(token + len, sizeof(token) - len, "%s", salt_data);
	LOG_LOOP("token_str=%s====>\n", token);

	/*encrypt token*/
	memset(md5_token, 0, sizeof(md5_token));	
	genMD5(md5_token, token);

	/*Get encrypt token string*/
	len = 0;
	for(i = 0; i < 16; i++)
	{
		len += snprintf(md5_token_str + len, size - len, "%02X", md5_token[i]);
	}

	LOG_LOOP("md5_token_str=%s====>\n", md5_token_str);
	json_object_put(json_obj);

	return 0;
}

void http_get(void) 
{  
	FILE *fp = NULL;
	CURL *curl = NULL;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	unlink(HTTP_GET_RCV_FILE);
	fp = fopen(HTTP_GET_RCV_FILE, "w");
	if(curl && fp)
	{
		LOG_LOOP("curl init ok====>\n");
		curl_easy_setopt( curl, CURLOPT_TIMEOUT, 5);
		curl_easy_setopt(curl, CURLOPT_URL, http_get_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
	curl_global_cleanup();

	LOG_LOOP("return====>\n");
	return; 
}  

void http_post(char* md5_token)
{
	CURL *curl = NULL;
	struct curl_slist *plist = NULL;
	const char* send_data = NULL;
	struct json_object* obj = NULL;
    FILE *fp = NULL;

	if(md5_token == NULL)
	{
		return;
	}

	if(0 != access(POST_JSON_FILE, F_OK))
	{
		LOG_ERROR("[%s] is not exist,return====>\n", POST_JSON_FILE);
		return;
	}
	
	obj = json_object_from_file(POST_JSON_FILE);
	json_object_object_add(obj, "user", json_object_new_string("admin"));
	json_object_object_add(obj, "token", json_object_new_string(md5_token));
	send_data = json_object_to_json_string(obj);

	fp = fopen(HTTP_POST_RCV_FILE, "w");
	if(fp == NULL)
	{
		LOG_ERROR("open %s fail,return====>\n", HTTP_POST_RCV_FILE);
		return;		
	}

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, http_post_url);
		curl_easy_setopt(curl, CURLOPT_POST, 1L); 
		
		// 设置处理 response 的回调函数
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);	
		
		// 设置存储返回的结果
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);		

		plist = curl_slist_append(plist, "Content-Type:application/json"); 
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
		// 要发送的数据
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, send_data);			
		curl_easy_perform(curl);
		

		curl_slist_free_all(plist);
		curl_easy_cleanup(curl);
	}

	fclose(fp);
	json_object_put(obj);
	curl_global_cleanup();
	
	return;
}

void initReqParameter(void)
{
	char ip[16] = {0};
	char port[8] = {0};
	char get_url[32] = {0};
	char post_url[32] = {0};
	char cycle[4] = {0};

	rdn_get(CLOUD_NODE, "ip", ip, sizeof(ip));
	rdn_get(CLOUD_NODE, "port", port, sizeof(port));
	rdn_get(CLOUD_NODE, "url_get", get_url, sizeof(get_url));
	rdn_get(CLOUD_NODE, "url_post", post_url, sizeof(post_url));
	rdn_get(CLOUD_NODE, "cycle", cycle, sizeof(cycle));
	
	snprintf(http_get_url,sizeof(http_get_url), "%s:%s/%s", ip, port, get_url);
	snprintf(http_post_url, sizeof(http_post_url), "%s:%s/%s", ip, port, post_url );
	cycle_time = atoi(cycle);
	
	LOG_WARN("http_get_url=[%s]====>\n", http_get_url);
	LOG_WARN("http_post_url=[%s]====>\n", http_post_url);
	LOG_WARN("cycle_time=[%s]====>\n", cycle);
	
	return;
}

/**
 * @brief 更新云平台数据
 */
int UpdateCloudData(void)
{
	char cmd[64] = {0};
	const char* value_ptr = NULL;
	struct json_object* value_obj = NULL;	
	struct json_object* robot_obj = NULL;	
	struct json_object* ver_obj = NULL;
	struct json_object* panel_obj = NULL;
	struct json_object* time_obj = NULL;
	struct json_object* bat_obj = NULL;
	struct json_object* trace_obj = NULL;
	struct json_object* web_obj = NULL;
	int error_code = 0;
	char buf[32] = {0};
	char value[32] = {0};

	/***************************************************************************************/
	robot_obj = json_object_new_object();

	memset(value, 0, sizeof(value));
	rdn_get(ROBOT_NODE, "name", value, sizeof(value));
	value_ptr = value;
    json_object_object_add(robot_obj, "name", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(ROBOT_NODE, "sn", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(robot_obj, "sn", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(ROBOT_NODE, "cpu_usage", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(robot_obj, "cpu_usage", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(ROBOT_NODE, "cpu_temp1", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(robot_obj, "cpu_temp", json_object_new_string(value_ptr));

	value_ptr = "39";
	json_object_object_add(robot_obj, "mcu_temp", json_object_new_string(value_ptr));

	value_ptr = "0";
	json_object_object_add(robot_obj, "error_code", json_object_new_string(value_ptr));
	
	/***************************************************************************************/
	panel_obj = json_object_new_object();
	
	memset(value, 0, sizeof(value));
	rdn_get(PANEL_NODE, "width", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(panel_obj, "panel_width", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(PANEL_NODE, "heigh", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(panel_obj, "panel_height", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(PANEL_NODE, "angle", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(panel_obj, "panel_angle", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(PANEL_NODE, "mode", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(panel_obj, "panel_type", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(SENSOR_TEMP_NODE, "value", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(panel_obj, "panel_temp", json_object_new_string(value_ptr));

	/***************************************************************************************/
	time_obj = json_object_new_object();
	
	memset(value, 0, sizeof(value));
	rdn_get(ROBOT_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "robot", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(MAINTAIN_WP_L_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "water_pump", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(MAINTAIN_TIRE_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "tire", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(MAINTAIN_BRUSH_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "brush", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(MAINTAIN_SCP_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "scraper", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(MAINTAIN_AB_L_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "left_adsorption", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(MAINTAIN_AB_R_NODE, "timestring", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(time_obj, "right_adsorption", json_object_new_string(value_ptr));

	/***************************************************************************************/
	bat_obj = json_object_new_object();

	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "bat_t1", value, sizeof(value));
    snprintf(buf, sizeof(buf), "%d", atoi(value)/100);
    value_ptr = buf;
	json_object_object_add(bat_obj, "bat_temp", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "rem_cap", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(bat_obj, "remain_mah", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "percent_cap", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(bat_obj, "remain_percent", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "health", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(bat_obj, "health_factor", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "cycle_count", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(bat_obj, "cycle_count", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "vt", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(bat_obj, "voltage", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(BATTERY_NODE, "current", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(bat_obj, "current", json_object_new_string(value_ptr));

	/***************************************************************************************/
	trace_obj = json_object_new_object();

	memset(value, 0, sizeof(value));
	rdn_get(TRACE_NODE, "knowmap", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "knownmap_flag", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(TRACE_NODE, "isleft", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "is_left", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(TRACE_NODE, "turncount", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "turn_count", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(TRACE_NODE, "proportion", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "proportion", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(TRACE_NODE, "x", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "x", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(TRACE_NODE, "y", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "y", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(PANEL_NODE, "max_x", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "max_x", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(PANEL_NODE, "max_y", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(trace_obj, "max_y", json_object_new_string(value_ptr));

	/***************************************************************************************/
	ver_obj = json_object_new_object();

	memset(value, 0, sizeof(value));
	rdn_get(VERSION_NODE, "sysver", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(ver_obj, "system", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(VERSION_NODE, "appver", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(ver_obj, "app", json_object_new_string(value_ptr));
	
	memset(value, 0, sizeof(value));
	rdn_get(VERSION_NODE, "mcuver", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(ver_obj, "mcu", json_object_new_string(value_ptr));

	memset(value, 0, sizeof(value));
	rdn_get(VERSION_NODE, "cfgver", value, sizeof(value));
	value_ptr = value;
	json_object_object_add(ver_obj, "cfg", json_object_new_string(value_ptr));

	/***************************************************************************************/
	web_obj = json_object_new_object();
	json_object_object_add(web_obj, "Robot", robot_obj);
	json_object_object_add(web_obj, "Panel", panel_obj);
	json_object_object_add(web_obj, "Time", time_obj);
	json_object_object_add(web_obj, "Battery", bat_obj);
	json_object_object_add(web_obj, "Trace", trace_obj);
	json_object_object_add(web_obj, "Version", ver_obj);

	json_object_to_file(POST_JSON_FILE, web_obj);
	json_object_put(web_obj);

	return 0;
}

int main(int argc, char* argv[])
{
	char enable[4] = {0};
	char md5_token[64] = {0};
	
	initReqParameter();
	
	while(1)
	{
		sleep(cycle_time);
		rdn_get(ENABLE_NODE, "cloud", enable, sizeof(enable));
		if(atoi(enable) == 0)
		{
			//LOG_LOOP("rdncloud Disabled====>\n");
			continue;
		}
		UpdateCloudData();
		http_get();
		if(0 == get_md5_token(md5_token, sizeof(md5_token)))
		{
			http_post(md5_token);
		}
	}
	
	return 0;
}

