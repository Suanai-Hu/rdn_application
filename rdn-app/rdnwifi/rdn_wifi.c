/**
 * @file wificonnect.cpp
 * @brief 本文件涉及自动连接wifi相关的代码
 * @author Lei.Zhang
 * @email zhanglei@radiantpv.com
 * @version V0.0.0
 * @date 2021-12-21
 * @license 2014-2021,Radiant Solar Technology Co., Ltd.
**/ 
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include "debug.h"
#include "node.h"
#include "rdn_wifi.h"
#include "rdn_api.h"

WifiConnctState* CurrentWifiState = NULL;
wifiConnAction 	CurrentWifiAction = WIFI_CHECK;


WifiConnctState conn_wifi_auto 		= {"conn_wifi_auto",		AutoConnect,	NULL					};
WifiConnctState conning_wifi_auto 	= {"conning_wifi_auto",		NULL,			CheckAutoConningState	};	
WifiConnctState conn_wifi_manual	= {"conn_wifi_manual",		ManualConnect,	NULL					};
WifiConnctState conning_wifi_manual	= {"conning_wifi_manual",	NULL,			CheckManualConningState	};
WifiConnctState conn_wifi_idle 		= {"conn_wifi_idle",		NULL, 			CheckIdleState			};
WifiConnctState conn_wifi_none 		= {"NULL",					NULL, 			NULL					};

void SetAckNode(char* acknum)
{
	rdn_set(GLOBAL_NODE, "acknum", acknum);
	return;
}

void AutoConnect(void)
{
	int i = 0;
	int ret = 0;
	char ssid[32] = {0};
	char psw[32] = {0};
	char ssid_buf[32] = {0};
	char psw_buf[32] = {0};
	char value[8] = {0};
	int retry_times = 0;

	LOG_DBG("enter====>\n");
	rdn_set(WL_WIFI_NODE, "state", START_STATE);
	rdn_set(WL_WIFI_NODE, "loging_ssid", "");
	rdn_set(WL_WIFI_NODE, "loging_psw", "");
	rdn_set(WL_WIFI_NODE, "ip", "");
	rdn_set(WL_WIFI_NODE, "gateway", "");
	
	while(1)
	{
		sleep(5);
		retry_times++;

		memset(value, 0, sizeof(value));
		rdn_get(WL_WIFI_NODE, "retrytimes", value, sizeof(value));
		if(retry_times >= atoi(value))
		{
			LOG_DBG("Retry times =[%d],continue====>\n",retry_times);
			SwitchCurrentState(&conn_wifi_none, WIFI_NONE);
			continue;
		}
		
		for(i=0; i<MAX_WIFI_CACHE_NUM; i++)
		{
			memset(ssid_buf, 0, sizeof(ssid_buf));
			memset(psw_buf, 0, sizeof(psw_buf));
			memset(ssid, 0, sizeof(ssid));
			memset(psw, 0, sizeof(psw));
			snprintf(ssid_buf, sizeof(ssid_buf), "ssid%d", i);
			snprintf(psw_buf, sizeof(psw_buf), "psw%d", i);
			rdn_get(WL_WIFI_NODE, ssid_buf, ssid, sizeof(ssid));
			rdn_get(WL_WIFI_NODE, psw_buf, psw, sizeof(psw));
			LOG_LOOP("root.wl.wifi.ssid[%d]=%s,psw[%d]=%s====>\n",i, ssid, i, psw);
			if(ssid[0] != '\0')
			{
				/* If ssid is not "0",means wifi login cache is exist */
				ret = SearchSsidFromWifiList(ssid);
				if(1 == ret)
				{	
					LOG_DBG("Try connect wifi,ssid=%s,psw=%s====>\n",ssid, psw);
					/* wpa certification process */
					rdn_set(WL_WIFI_NODE, "state", WPA_STATE);
					ret = StartWpaSupplicant(ssid, psw);
					if(ret < 0)
					{
						rdn_set(WL_WIFI_NODE, "state", PSW_ERROR_STATE);
						SwitchCurrentState(&conn_wifi_none, WIFI_NONE);
						return;
					}
					rdn_set(WL_WIFI_NODE, "loging_ssid", ssid);
					rdn_set(WL_WIFI_NODE, "loging_psw", psw);
					SwitchCurrentState(&conning_wifi_auto, WIFI_CHECK);
					return;
				}
			}
		}
	}

	return;
}

void CheckAutoConningState(void)
{
	int ret = 0;
	int flag = 0;
	int connect_flag = 0;
	char ip[32] = {0};
	char subnetmask[32] = {0};
	char network_ip[32] = {0};
	
	while(1)
	{
		if(NO_CONNECT != CheckWlan0Status(FILE_WLAN0_STATUS_1))
		{
			LOG_DBG("WPA OK,using [%d]s====>\n",flag);
			break;
		}
		sleep(1);
		flag++;
		if(flag == 30)
		{
			LOG_DBG("WPA timeout 30s====>\n");
			rdn_set(WL_WIFI_NODE, "state", PSW_ERROR_STATE);
			SwitchCurrentState(&conning_wifi_auto, WIFI_CONNECT);
			return;
		}
	}

	/* Get wlan0 ip using udhcpc */
	rdn_set(WL_WIFI_NODE, "state", DHCP_STATE);
	ret = StartUdhcpc();
	if(0 == ret)
	{
		GetIpMask(ip, sizeof(ip), subnetmask, sizeof(subnetmask));
		GetNetworkIp(ip, subnetmask, network_ip,  sizeof(network_ip));
		rdn_set(WL_WIFI_NODE, "ip", ip);
		rdn_set(WL_WIFI_NODE, "gateway", network_ip);
		rdn_set(WL_WIFI_NODE, "state", OK_STATE);
		SwitchCurrentState(&conn_wifi_idle, WIFI_CHECK);
	}
	else
	{
		SwitchCurrentState(&conn_wifi_auto, WIFI_CONNECT);
	}
}

void ManualConnect(void)
{
	int ret = 0;
	char ip[32] = {0};
	char mask[32] = {0};
	char login_ssid[32] = {0};
	char login_psw[32] = {0};
	char loging_psw[32] = {0};
	char subnetmask[32] = {0};
	char network_ip[32] = {0};

	rdn_set(WL_WIFI_NODE, "state", START_STATE);
	rdn_get(WL_WIFI_NODE, "login_ssid", login_ssid, sizeof(login_ssid));
	rdn_get(WL_WIFI_NODE, "login_psw", login_psw, sizeof(login_psw));
	
	if(IsConnectWifi(login_ssid))
	{
		LOG_DBG("Have connect wifi [%s]====>\n",login_ssid);
		rdn_get(WL_WIFI_NODE, "loging_psw", loging_psw, sizeof(loging_psw));
		if(strcmp(login_psw, loging_psw))
		{
			SetAckNode("ACK_WIFI_FAIL_PSW");
			SwitchCurrentState(&conn_wifi_none, WIFI_NONE);
			rdn_set(WL_WIFI_NODE, "state", PSW_ERROR_STATE);
		}
		else
		{
			GetIpMask(ip, sizeof(ip), mask, sizeof(mask));
			GetNetworkIp(ip, subnetmask, network_ip,  sizeof(network_ip));
			UpdateLoginCache(login_ssid, login_psw, ip);
			SetAckNode("ACK_WIFI_OK");
			SwitchCurrentState(&conn_wifi_idle, WIFI_CHECK);
			rdn_set(WL_WIFI_NODE, "state", OK_STATE);
			rdn_set(WL_WIFI_NODE, "loging_ssid", login_ssid);
			rdn_set(WL_WIFI_NODE, "loging_psw", login_psw);
			rdn_set(WL_WIFI_NODE, "ip", ip);
			rdn_set(WL_WIFI_NODE, "gateway", network_ip);
			//rdn_set(WL_WIFI_NODE, "mode", "auto");
		}
		return;
	}

	//clear some wifi info and connect again.
	rdn_set(WL_WIFI_NODE, "ip", "");
	rdn_set(WL_WIFI_NODE, "gateway", "");
	rdn_set(WL_WIFI_NODE, "loging_ssid", "");
	rdn_set(WL_WIFI_NODE, "loging_psw", "");
	
	rdn_set(WL_WIFI_NODE, "state", WPA_STATE);
	ret = StartWpaSupplicant(login_ssid, login_psw);
	if(ret < 0)
	{
		SetAckNode("ACK_WIFI_FAIL_PSW");
		rdn_set(WL_WIFI_NODE, "state", PSW_ERROR_STATE);
		SwitchCurrentState(&conn_wifi_none, WIFI_NONE);
		return;
	}

	rdn_set(WL_WIFI_NODE, "loging_ssid", login_ssid);
	rdn_set(WL_WIFI_NODE, "loging_psw", login_psw);
	SwitchCurrentState(&conning_wifi_manual, WIFI_CHECK);
	
	return;
}

void CheckManualConningState(void)
{
	int ret = 0;
	int flag = 0;
	int connect_flag = 0;
	char ip[32] = {0};
	char subnetmask[32] = {0};
	char network_ip[32] = {0};
	char loging_ssid[32] = {0};
	char loging_psw[32] = {0};
	
	while(1)
	{
		if(NO_CONNECT != CheckWlan0Status(FILE_WLAN0_STATUS_1))
		{
			LOG_DBG("WPA OK,using [%d]s====>\n",flag);
			break;
		}
		sleep(1);
		flag++;
		if(flag == 30)
		{
			LOG_DBG("WPA timeout 30s====>\n");			
			SwitchCurrentState(&conn_wifi_none, WIFI_NONE);
			SetAckNode("ACK_WIFI_FAIL_PSW");
			rdn_set(WL_WIFI_NODE, "state", PSW_ERROR_STATE);
			return;
		}
	}

	rdn_set(WL_WIFI_NODE, "state", DHCP_STATE);
	/* Get wlan0 ip using udhcpc */
	ret = StartUdhcpc();
	if(0 == ret)
	{
		GetIpMask(ip, sizeof(ip), subnetmask, sizeof(subnetmask));
		GetNetworkIp(ip, subnetmask, network_ip,  sizeof(network_ip));
		rdn_get(WL_WIFI_NODE, "loging_ssid", loging_ssid, sizeof(loging_ssid));
		rdn_get(WL_WIFI_NODE, "loging_psw", loging_psw, sizeof(loging_psw));
		UpdateLoginCache(loging_ssid, loging_psw, ip);
		rdn_set(WL_WIFI_NODE, "ip", ip);
		rdn_set(WL_WIFI_NODE, "gateway", network_ip);
		rdn_set(WL_WIFI_NODE, "state", OK_STATE);
		SwitchCurrentState(&conn_wifi_idle, WIFI_CHECK);
		SetAckNode("ACK_WIFI_OK");
	}
	else
	{
		rdn_set(WL_WIFI_NODE, "state", IP_ERROR_STATE);
		SwitchCurrentState(&conn_wifi_none, WIFI_NONE);
		SetAckNode("ACK_WIFI_FAIL_IP");
	}

	return;
}

void CheckIdleState(void)
{
	int ret = 0;
	int count = 0;
	char gateway[16] = {0};
	char mode[16] = {0};
	char state[16] = {0};
	
	rdn_get(WL_WIFI_NODE, "gateway", gateway, sizeof(gateway));
	while(1)
	{
		ret = GetPingResult(gateway);
		if(ret == 0)
		{
			count++;
			LOG_DBG("Ping gateway fail times=[%d/3],====>\n",count);
			if(count == 3)
			{
				SwitchCurrentState(&conn_wifi_auto, WIFI_CONNECT);
				break;
			}					
		}
		else
		{
			rdn_get(WL_WIFI_NODE, "mode", mode, sizeof(mode));
			rdn_get(WL_WIFI_NODE, "state", state, sizeof(state));
			if(strcmp(mode, "manual") == 0 && strcmp(state, INIT_STATE) == 0)
			{
				SwitchCurrentState(&conn_wifi_manual, WIFI_CONNECT);	
				return;
			}
			count = 0;
		}
		sleep(10);
	}
	
	return;
}

void SwitchCurrentState(WifiConnctState* state, wifiConnAction action)
{
	char* action_str[4] = {"NONE", "CHECK", "CONNECT"};
	
	CurrentWifiState = state;
	CurrentWifiAction = action;
	LOG_DBG("Current state=[%s],action=[%s]====>\n",CurrentWifiState->state, action_str[action]);
}

/**
 * @brief 根据key值从指定的文件中获取到value值
 * @param filePath 文件路径
 * @param key 属性字符串
 * @param value 存放获取到的值
 * @param size value可以存储的最大长度
 * @return 是否成功
 * @retval 0表示成功，-1表示失败
*/
int GetValueFromCfgFile(char* filepath, char* key, char* value, int size)
{
	FILE* fp = NULL;
	char* p = NULL;
	char buf[BUF_LEN_128] = {0};
	char keyname[64] = {0};
	
	if(!filepath || !key || !value)
	{
		LOG_ERROR("filepath/key/value is NULL====>\n");
		return -1;
	}
	fp = fopen(filepath, "r");
	if(!fp)
	{
		LOG_ERROR("open [%s] fail====>\n",filepath);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	while(fgets(buf, BUF_LEN_128, fp) != NULL)
	{
		if(p = strstr(buf, key))
		{
			/*
				bssid=50:64:2b:9e:e9:7d
				ssid=app-test
				wpa_state=COMPLETED
				ip_address=192.168.31.128
			*/
			
			sscanf(buf,"%[^=]",keyname);
            if(strncmp(keyname, key, strlen(key)-1))
			{
				continue;
			}
			memset(value, 0, size);
			strncpy(value, p + strlen(key), size-1);
			value[strlen(value)-1] = '\0';
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	return -1;
}


/**
 * @brief 获取系统板周边wifi信息
 * @param ssidlist  输出参数，存放周边wifi信息
 * @param max_ssid_sum 允许存放的周边wifi信息的最大条数
 * @return 是否成功
 * @retval 0表示成功，-1表示失败
*/

int GetNeighborWifiInfo(char ssidlist[][MAX_SSID_LEN], int max_ssid_sum)
{
	int i = 0;
	int len = 0;
	int lenth = 0;
	FILE* fp = NULL;
	char* p = NULL;
	char cmd[BUF_LEN_128] = {0};
	char buf[BUF_LEN_128]; 	
	int flag = 1;

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "%s > %s", CMD_GET_WIFI_LIST, FILE_SCAN_LIST);
	system(cmd);

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "grep -E \"ESSID\" %s > %s", FILE_SCAN_LIST, FILE_SSID_LIST);
	system(cmd);

	fp = fopen(FILE_SSID_LIST, "r");
	if(!fp)
	{
		LOG_ERROR("open [%s] fail====>\n", FILE_SSID_LIST);
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	while(fgets(buf, BUF_LEN_128, fp) != NULL)
	{
		len = strlen(buf);
		buf[len-1] = '\0';  
		if(p = strstr(buf, "ESSID:"))
		{
			if(strlen(p) > 30 || strstr(p, " "))
			{
				/* ESSID:"360\xE8\xA1\x8C\xE8\xBD\xA6\xE8\xAE\xB0\xE5\xBD\x95\xE4\xBB\xAA-2105" */
				/* ESSID:"Mi 10 Lite Zoom" */

				flag = 0;
				continue;
			}
			else
			{
				if(i < max_ssid_sum)
				{
					/* ESSID:"testWifi" */
					flag = 1;

					strncpy(ssidlist[i], p + strlen("ESSID:\""), MAX_SSID_LEN-1);
					lenth = strlen(ssidlist[i]); 
					ssidlist[i][lenth-1] = '\0';  /*delete "," */
					LOG_LOOP("ssidList[%d]=%s====>\n", i, ssidlist[i]);
	                i++;
				}
			}
		}
		memset(buf, 0, sizeof(buf));
	}	
	
	fclose(fp);
	
	return 0;
}

int GetNodeIndex(char* ssid)
{
	int i = 0;
	char attr[32] = {0};
	char value[32] = {0};
	int idx = -1;

	if(ssid == NULL || ssid[0] == '\0')
	{
		return -1;
	}
	
	for(i=0; i<MAX_SSID_NUM; i++)
	{
		memset(attr, 0, sizeof(attr));
		memset(value, 0, sizeof(value));
		snprintf(attr, sizeof(attr), "ssid%d", i);
		rdn_get(WL_NB_WIFI_NODE, attr, value, sizeof(value));	
		if(0 == strcmp(ssid, value))
		{
			return -1;
		}
		if(idx == -1 && value[0] == '\0')
		{
			idx = i;	
		}	
	}

	return idx;
}

void SetNeighborWifiNode(void)
{
	int i = 0;
	int idx = -1;
	char attr[32] = {0};
	char ssidlist[MAX_SSID_NUM][MAX_SSID_LEN] = {0};
	char value[32] = {0};
	
	GetNeighborWifiInfo(ssidlist, MAX_SSID_NUM);
	for(i=0; i<MAX_SSID_NUM; i++)
	{
		memset(attr, 0, sizeof(attr));
		snprintf(attr, sizeof(attr), "ssid%d", i);	
		rdn_set(WL_NB_WIFI_NODE, attr, "");
	}

	for(i=0; i<MAX_SSID_NUM; i++)
	{
		idx = GetNodeIndex(ssidlist[i]);
		if(idx >= 0)
		{
			snprintf(attr, sizeof(attr), "ssid%d", idx);	
			rdn_set(WL_NB_WIFI_NODE, attr, ssidlist[i]);
			LOG_DBG("neighbor wifi:ssid[%d]=%s====>\n", idx, ssidlist[i]);
		}
	}
	
	return;
}

/**
 * @brief 从wifi列表中查找特定的wifi名称
 * @param ssid 查找的wifi名称
 * @param ssidlist wifi列表
 * @return 是否找到
 * @retval 未找到返回0，找到返回1
*/

int SearchSsidFromWifiList(const char* ssid)
{
	int i = 0;
	char attr[32] = {0};
	char value[32] = {0};
	
	if(ssid == NULL || strlen(ssid) == 0)
	{
		return -1;
	}
	
	for(i=0; i<MAX_SSID_NUM; i++)
	{
		snprintf(attr, sizeof(attr), "ssid%d", i);
		rdn_get(WL_NB_WIFI_NODE, attr, value, sizeof(value));
		if(0 == strcmp(ssid, value))
		{
			return 1;
		}
	}

	return 0;
}

/**
 * @brief 检查进程是否存在
 * @param processname 进程名称
 * @return 是否成功
 * @retval -1表示失败，成功则返回进程ID
*/

int IsProcessExist(char* processname)
{   
	int pid = 0;   
	FILE* fp = NULL;
	char pid_file[32] = {0};
	char buf[BUF_LEN_64] = {0};
	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/tmp/%s.pid", processname);	

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "pidof %s > %s", processname, pid_file);	
	system(buf);

	fp = fopen(pid_file, "r");
	if(fp)
	{
		memset(buf, 0, sizeof(buf));
		if(fgets(buf, sizeof(buf)-1, fp))
		{ 
			pid = atoi(buf);   	  
		} 
		fclose(fp); 
		return pid;
	}

	return -1;
}

/**
 * @brief 检查算法板连接外部wifi的状态
 * @param filepath wifi状态结果存放文件
 * @return 返回WIFI连接状态
 * @retval -1表示失败，1表示密码验证成功，2表示获取IP成功
*/

int CheckWlan0Status(char* filepath)
{
	char state[32] = {0};
	char ip[16] = {0};
	char buf[BUF_LEN_128] = {0};
	int ret = -1;

	if(!filepath || !strlen(filepath))
	{
		LOG_ERROR("filePath is NULL====>\n");
		return -1;
	}

	if(IsProcessExist(PROCESS_WPA_SUPPLICANT) <= 0)
	{
		LOG_LOOP("wpa_supplicant process is not exist====>\n");
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "wpa_cli -i wlan0 status > %s", filepath);
	system(buf);

	memset(state, 0, sizeof(state));
	memset(ip, 0, sizeof(ip));
	GetValueFromCfgFile(filepath, "wpa_state=", state, sizeof(state));
	GetValueFromCfgFile(filepath, "ip_address=", ip, sizeof(ip));

	LOG_DBG("wpa_state=%s,ip=%s====>\n",state, ip);

	if(0 == strcmp(state, "COMPLETED") && ip[0] != '\0')
	{
		ret = IP_OK;
	}
	else if(0 == strcmp(state, "COMPLETED") && ip[0] == '\0')
	{
		ret = WPA_OK;
	}
	else
	{
		ret = NO_CONNECT;
	}
	
	LOG_DBG("wlan0 status=%d [0:NO_CONNECT, 1:WPA_OK, 2:IP_OK]====>\n", ret);
	return ret;
}

/**
 * @brief 启动udhcpc进程获取IP地址
 * @return 是否成功
 * @retval 0表示成功，-1表示失败
*/

int StartUdhcpc(void)
{
	int flag = 0;
	
	system("killall udhcpc");
	system("udhcpc -b -i wlan0 -R"); 
	/*-R:if udhcpc process is not exist, IP will be released */
	
	while(1)	
	{
		if(IP_OK == CheckWlan0Status(FILE_WLAN0_STATUS_1))
		{
			break;
		}
		sleep(1);
		flag++;
		if(flag == 10)
		{
			LOG_ERROR("Get ip fail,timeout 10s,return -1====>\n");
			return -1;
		}
	}

	return 0;
}

/**
 * @brief 启动连接wifi的进程
 * @param ssid wifi名称
 * @param psw wifi密码
 * @return 是否成功
 * @retval 0表示成功，-1表示失败
*/

int StartWpaSupplicant(const char* ssid, const char* psw)
{
	int ret = 0;
	int flag = 0;
	char buf[128] = {0};
	if(ssid==NULL || psw==NULL || strlen(ssid)==0 || strlen(psw)=='\0')
	{
		LOG_ERROR("ssid or psw is NULL,return -1====>\n");
		return -1;
	}

#ifdef USE_NEARDI_SYS
	//system("ifconfig wlan0 down");
#endif
	system("killall wpa_supplicant");
	unlink(FILE_WPA_SUPPLI_CONF);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "echo ctrl_interface=%s >> %s", FILE_WPA_SUPPLI_IF, FILE_WPA_SUPPLI_CONF);
	system(buf);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "echo ap_scan=1 >> %s", FILE_WPA_SUPPLI_CONF);
	system(buf);
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "wpa_passphrase %s %s >> %s", ssid, psw, FILE_WPA_SUPPLI_CONF);
	ret = system(buf);
	if(ret != 0)
	{
		LOG_ERROR("wpa_passphrase fail,password must be 8-63 characters====>\n");
		return -1;
	}

	/*The following cmd will make the board to connet the wifi with the config file. */

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "wpa_supplicant -B -i wlan0 -c %s", FILE_WPA_SUPPLI_CONF);
	system(buf);

	return 0;
}

/**
 * @brief 获取某个字符串中特定字符后面的值
 * @param str 输入字符串 
 * @param s 字符串中特定字符
 * @return 返回特定字符后面的字符串
 * @retval 失败返回NULL,成功返回获取到的字符串
*/

char* GetValueBySymbol(char* str, char* s)
{
	char* p = NULL;
	
    if(!str || !s || !strlen(str) || !strlen(s))
	{
		LOG_ERROR("str/symbol is NULL====>\n");
		return NULL;
	}

	if(p = strstr(str, s))
	{
		return (p+1);
	}

	return NULL;
}

/**
 * @brief 获取wlan0(wifi接口)的IP地址和子网掩码
 * @param ip_addr 输出参数，存储wlan0的IP地址
 * @param ip_size IP地址的最大存储长度
 * @param mask_addr 输出参数，存储子网掩码
 * @param mask_size 子网掩码的最大存储长度
 * @return 是否成功
 * @retval 0表示成功，-1表示失败
*/

int GetIpMask(char ip_addr[], int ip_size, char mask_addr[], int mask_size)
{
	int i = 0;
	FILE* fp = NULL;
	char buf[BUF_LEN_128] = {0};
	char inet_buf[16] = {0};
	char ip_buf[32] = {0};
	char bcast_buf[32] = {0};
	char mask_buf[32] = {0};
	char* p=NULL;
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "ifconfig wlan0 > %s", FILE_IFCONFIG_WLAN0);
	system(buf);

	fp = fopen(FILE_IFCONFIG_WLAN0, "r");
	if(!fp)
	{
		LOG_ERROR("open %s fail====>\n", FILE_IFCONFIG_WLAN0);
		return -1;
	}

	/*inet addr:192.168.31.169  Bcast:192.168.31.255  Mask:255.255.255.0*/

	while(fgets(buf, BUF_LEN_128, fp))
	{
		if(i == 1)
		{
			sscanf(buf,"%s %s %s %s", inet_buf, ip_buf, bcast_buf, mask_buf);
			LOG_DBG("ip=%s, mask=%s ====>\n", ip_buf, mask_buf);
			if(ip_buf[0] != '\0')
			{
				if(p = GetValueBySymbol(ip_buf, ":"))
				{
					strncpy(ip_addr, p, ip_size-1);
				}
			}
			if(mask_buf[0] != '\0')
			{
				if(p = GetValueBySymbol(mask_buf, ":"))
				{
					strncpy(mask_addr, p, mask_size-1);
				}
			}
			break;
		}
		i++;
	}

	fclose(fp);
	return 0;
}

/**
 * @brief 根据IP地址和子网掩码获取网络号
 * @param ip IP地址
 * @param mask 子网掩码
 * @param network_ip 存放网络号
 * @param size 存放的网络号的最大长度
*/
int GetNetworkIp(char* ip, char* mask, char* network_ip, int size)
{
	int i = 0;
	int ip_len = 0;
	int mask_len = 0;
	int tmp_len1 = 0;
	int tmp_len2 = 0;
    int ip_array[4] = {0};
    int mask_array[4] = {0};
    int net_ip_array[4] = {0};
    char tmp[4] = {0};
    char* p_ip = ip;
    char* p_mask = mask;
    char* p = NULL;
	/*Split IP */
    for(i = 0; i < 4; i++)
    {
        ip_len = strlen(p_ip);
        if(i != 3)
		{
            p = strchr(p_ip, '.');
            if(NULL == p)
            {
                return 0;
            }
        }
        else
        {
            p = p_ip;
        }
        tmp_len1 = strlen(p);
		
        if(i != 3)
        {
            tmp_len2 = ip_len - tmp_len1;
        }
        else
        {
            tmp_len2 = tmp_len1;
        }
        
        memset(tmp, 0, sizeof(tmp));
        strncpy(tmp, p_ip, tmp_len2);
        ip_array[i] = atoi(tmp);
        if(i != 3)
        {
            p_ip += (tmp_len2 + 1);
        }
    }

    p = NULL;
    tmp_len1 = 0;
    tmp_len2 = 0;
	
	/*Split mask */
    for(i = 0; i < 4; i++)
    {	
        mask_len = strlen(p_mask);
        if(i != 3)
        {
            p = strchr(p_mask, '.');
            if(NULL == p)
            {
                return 0;
            }
        }
        else
        {
            p = p_mask;
        } 
        tmp_len1 = strlen(p);
        
        if(i != 3)
        {
            tmp_len2 = mask_len - tmp_len1;
        }
        else
        {
            tmp_len2 = tmp_len1;
        }
        
        memset(tmp, 0, sizeof(tmp));
        strncpy(tmp, p_mask, tmp_len2);
        mask_array[i] = atoi(tmp);
        if(i != 3)
        {
            p_mask += (tmp_len2 + 1);
        }
    }

    p = NULL;

    /* Calculate network IP */
    for(i = 0; i < 4; i++)
    {
        net_ip_array[i] = ip_array[i] & mask_array[i];
    }
    
   /*Splicing network number */
	net_ip_array[3]=1;
    snprintf(network_ip, size, "%d.%d.%d.%d", net_ip_array[0], net_ip_array[1], net_ip_array[2], net_ip_array[3]);
	
    return 0;
}

int GetPingResult(char* ip)
{
	int ret = 0;
	char* p = NULL;
	FILE* fp = NULL;
	char buf[BUF_LEN_256] = {0};
	char cmdbuf[BUF_LEN_64] = {0};
	
	if(ip == NULL || ip[0] == '\0')
	{
		return -1;
	}	
	memset(buf, 0, sizeof(buf));
   	//unlink(FILE_PING_RESULT);
	snprintf(cmdbuf, sizeof(cmdbuf), "ping %s -c 3 -W 2 > %s", ip, FILE_PING_RESULT);
	system(cmdbuf);

	/*
	PING 10.201.126.1 (10.201.126.1): 56 data bytes
	64 bytes from 10.201.126.1: seq=0 ttl=64 time=0.380 ms
	64 bytes from 10.201.126.1: seq=1 ttl=64 time=0.320 ms
	64 bytes from 10.201.126.1: seq=2 ttl=64 time=0.383 ms

	--- 10.201.126.1 ping statistics ---
	3 packets transmitted, 3 packets received, 0% packet loss
	*/
	
   	fp = fopen(FILE_PING_RESULT, "r");
   	if(fp)
	{
	   while(fgets(buf, BUF_LEN_256, fp))
	   {
	       if( p=strstr(buf, "received"))
	       {
				/* p-2=[0 packets received, 100% packet loss]*/
				if(atoi(p-2) >= 2) 
				{			 			      
					ret = 1;
				}
	       }
	   }
	   fclose(fp);	
	}	
	
	return ret;
}

/**
 * @brief 检查算法板是否已经连接指定的wifi
 * @param wifiName 将要连接的wifi名称
 * @return 返回是否已经成功连接
 * @retval 1表示已经成功连接，0表示没有成功连接
*/
int IsConnectWifi(const char* wifiname)
{
	char ssid[32] = {0};
	char state[32] = {0};
	char ip[16] = {0};
	char buf[BUF_LEN_128] = {0};

	if(!wifiname || !strlen(wifiname))
	{
		LOG_ERROR("ssid is NULL====>\n");
		return 0;
	}
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "wpa_cli -i wlan0 status > %s", FILE_WLAN0_STATUS_1);
	system(buf);

	memset(state, 0, sizeof(state));
	memset(ip, 0, sizeof(ip));
	GetValueFromCfgFile(FILE_WLAN0_STATUS_1, "ssid=", ssid, sizeof(ssid));
	GetValueFromCfgFile(FILE_WLAN0_STATUS_1, "wpa_state=", state, sizeof(state));
	GetValueFromCfgFile(FILE_WLAN0_STATUS_1, "ip_address=", ip, sizeof(ip));

	LOG_DBG("ssid=%s,wpa_state=%s,ip=%s====>\n",ssid, state, ip);
	if(!strcmp(ssid, wifiname) && !strcmp(state, "COMPLETED") && ip[0] != '\0')
	{
		return 1;
	}
	
	return 0;
}

/**
 * @brief 更新算法板连接wifi的用户名和密码信息
 * @param ssid wifi名称
 * @param psw wifi密码
 * @param ip 连接当前wifi后获取到的ip地址
 * @return 是否成功
 * @retval 0表示成功，-1表示失败
*/
int UpdateLoginCache(char* ssid, char* psw, char* ip)
{
	int i = 0;
	int idx = -1;
	int flag = -1;
	char ssid_buf[32] = {0};
	char psw_buf[32] = {0};
	char ssid_value[32] = {0};
	char psw_value[32] = {0};

	if(ssid[0] == '\0' || psw[0] == '\0' || ip[0] == '\0')
	{
		return -1;
	}
	
	for(i=0; i<MAX_WIFI_CACHE_NUM; i++)
	{
		memset(ssid_buf, 0, sizeof(ssid_buf));
		memset(psw_buf, 0, sizeof(psw_buf));
		memset(ssid_value, 0, sizeof(ssid_value));
		memset(psw_value, 0, sizeof(psw_value));
		snprintf(ssid_buf, sizeof(ssid_buf), "ssid%d", i);
		snprintf(psw_buf, sizeof(psw_buf), "psw%d", i);
		rdn_get(WL_WIFI_NODE, ssid_buf, ssid_value, sizeof(ssid_value));
		rdn_get(WL_WIFI_NODE, psw_buf, psw_value, sizeof(psw_value));
		if(0 == strcmp(ssid, ssid_value))
		{
			flag = 0;
			rdn_set(WL_WIFI_NODE, psw_buf, psw);
			LOG_DBG("Replace wifi psw to index [%d]====>\n", i);
			break;
		}
		else
		{
			if(strlen(ssid_value) == 0)
			{
				if(-1 == idx)
				{
					idx = i;
				}
			}
		}
	}

	if(-1 == flag)
	{
		if(-1 == idx)
		{
			idx = 0;
		}
		flag = 1;
		memset(ssid_buf, 0, sizeof(ssid_buf));
		memset(psw_buf, 0, sizeof(psw_buf));
		snprintf(ssid_buf, sizeof(ssid_buf), "ssid%d", idx);
		snprintf(psw_buf, sizeof(psw_buf), "psw%d", idx);
		rdn_set(WL_WIFI_NODE, ssid_buf, ssid);
		rdn_set(WL_WIFI_NODE, psw_buf, psw);
		LOG_DBG("Add ssid/psw to index [%d]====>\n", idx);
	}

	if(flag != -1)
	{
		system("rdnsave");
		system("sync");
		LOG_DBG("update wifi login cache successfully====>\n");
	}

	return 0;
}

/**
 * @brief wifi连接相关的进程主函数
*/
int main()
{
	char enable[16] = {0};
	char mode[16] = {0};
	char state[16] = {0};
	
	SwitchCurrentState(&conn_wifi_auto, WIFI_CONNECT);
	SetNeighborWifiNode();
	
	while(1)
	{
		sleep(3);
		
		memset(enable, 0, sizeof(enable));
		rdn_get(ENABLE_NODE, "wifi", enable, sizeof(enable));
		if(!atoi(enable))
		{
			//LOG_LOOP("enable=0,continue====>\n");
			continue;
		}

		memset(enable, 0, sizeof(enable));
		rdn_get(WL_WIFI_NODE, "state", state, sizeof(state));
		rdn_get(WL_WIFI_NODE, "mode", mode, sizeof(mode));
		if(strcmp(mode, "manual") == 0 && strcmp(state, INIT_STATE) == 0)
		{
			LOG_DBG("Call SwitchCurrentState====>\n");
			SwitchCurrentState(&conn_wifi_manual, WIFI_CONNECT);
		}
		
		if(CurrentWifiAction == WIFI_CHECK)
		{
			if(CurrentWifiState->check)
			{
				CurrentWifiState->check();
			}
		}
		else if(CurrentWifiAction == WIFI_CONNECT)
		{
			if(CurrentWifiState->connect)
			{
				CurrentWifiState->connect();
			}
		}
	}
	return 0;
}

