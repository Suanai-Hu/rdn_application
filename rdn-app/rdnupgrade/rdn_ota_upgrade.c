#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "json.h"
#include "rdn_upgrade.h"

pthread_t ota_upgrade_th;
pthread_t ota_download_th;
pthread_t ota_unzip_th;

ota_upgrade_t ota_info=
{
	.ota_cmd = "",
	.ver_type = "",
	.ver_name = "",
	.ver_size = 0,
	.download_percent = 0,
	.unzip_ok = 0
};

int downloading_flag = 0;
int unziping_flag = 0;


otaState* pCurrOtaState = NULL;

/*Define some OTA update state & handle function */
otaState ota_idle =			{NULL, 				OtaDownLoadVer,			OtaCheckLatestVer};
otaState ota_downloading = 	{OtaCancelDownLoad,	OtaCheckDownLoadProcess,NULL};	
otaState ota_unzip = 		{NULL,				OtaUnzipVer,			NULL};
otaState ota_unziping = 	{OtaCancelUnzip,	OtaCheckUnzipProcess,	NULL};
otaState ota_update = 		{OtaCancelUpdate,	OtaUpdateVer,			NULL};
otaState ota_updating = 	{OtaCancelUpdate,	NULL,					NULL};
otaState ota = 				{DoOtaCancel, 		DoOtaConfirm,			DoOtaCheck};

/**
 * @brief 检查机器是否可以访问外网
 * @return 是否可以访问外网
 * @retval 1表示可以，0表示不可以
*/
int IsCanAccessInternet(void)
{
	char *p = NULL;
	FILE* fp = NULL;
	char buf[BUF_LEN_256] = {0};
	
	unlink(FILE_PING_RESULT);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "ping 8.8.8.8 -c 3  -W 2 > %s", FILE_PING_RESULT);
	system(buf);
	
	/*
		================Can access internet===========================
		PING 8.8.8.8 (8.8.8.8): 56 data bytes
		64 bytes from 8.8.8.8: seq=0 ttl=113 time=72.926 ms
		64 bytes from 8.8.8.8: seq=1 ttl=113 time=65.679 ms
		64 bytes from 8.8.8.8: seq=2 ttl=113 time=75.545 ms
		
		--- 8.8.8.8 ping statistics ---
		3 packets transmitted, 3 packets received, 0% packet loss
		round-trip min/avg/max = 65.679/71.383/75.545 ms
		
		=================Can't access internet========================
		PING 1.1.1.1 (1.1.1.1): 56 data bytes

		--- 1.1.1.1 ping statistics ---
		3 packets transmitted, 0 packets received, 100% packet loss
	*/

	fp = fopen(FILE_PING_RESULT, "r");
	if(fp)
	{
		while(fgets(buf, BUF_LEN_256, fp))
		{
			if( p=strstr(buf, "packets received"))
			{
				/* p-2=[0 packets received, 100% packet loss]*/
			 	if(atoi(p-2) >= 2) 
			 	{
					return 1;
				}
			}	
		}
		fclose(fp);
	}

	return 0;	
}

/**
 * @brief 根据类型的不同，获取最新的版本号
 * @return 是否成功获取版本号
 * @retval 0，0表示成功
*/
int GetLatestVerInfo(char* ver_name, int len, const char* ver_type)
{  
	struct json_object* json_obj = NULL;
	struct json_object* obj = NULL;
	const char* latest_ver_name = NULL;
	const char* latest_ver_size = NULL;
    char current_ver[32] = {0};
	char* ver_type_ptr = NULL;
	char buf[BUF_LEN_256] = {0};
    char value[32] = {0};

	memset(value, 0, sizeof(value));	
	if(0 == strcmp(ver_type, "App"))
	{
		rdn_get(VERSION_NODE, "appver", current_ver, sizeof(current_ver));
	}
	else if(0 == strcmp(ver_type, "System"))
	{
		rdn_get(VERSION_NODE, "sysver", current_ver, sizeof(current_ver));
	}
	else if(0 == strcmp(ver_type, "Mcu"))
	{
		rdn_get(VERSION_NODE, "mcuver", current_ver, sizeof(current_ver));
	}
	
	unlink(FILE_LATEST_VER);
	/*执行脚本获取最新的版本号 */
#if 0
	unlink(FILE_GET_LATEST_VER_PHP);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), CMD_GET_LATEST_VER_PHP, ver_type);
	LOG_WARN("buf=[%s]====>\n", buf);
	system(buf);
	sleep(2);
#endif
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), CMD_GET_LATEST_VER_NO, ver_type);
	LOG_WARN("buf=[%s]====>\n", buf);
	system(buf);
	sleep(1);
	
	json_obj = json_object_from_file(FILE_LATEST_VER);
	if(json_obj == NULL)
	{
		LOG_ERROR("Get json obj from file[%s] faild====>\n",FILE_LATEST_VER);
		return -1;
	}
	obj = json_object_object_get(json_obj, "version");
	if(obj)
	{	
		latest_ver_name = json_object_get_string(obj);
	}
	else
	{
		LOG_ERROR("Get latest version name failed====>\n");
		json_object_put(json_obj);
		return -1;
	}

	obj = json_object_object_get(json_obj, "size");
	if(obj)
	{	
		latest_ver_size = json_object_get_string(obj);
	}
	else
	{
		LOG_ERROR("Get latest version size failed====>\n");
		json_object_put(json_obj);
		return -1;
	}

	if(0 <= strncmp(current_ver, latest_ver_name, strlen(current_ver)))
	{
		LOG_ERROR("Have been latest version,[%s]====>\n",current_ver);
		json_object_put(json_obj);
		return -2;
	}
	strncpy(ver_name, latest_ver_name, len-1);
	
	json_object_put(json_obj);
	return atoi(latest_ver_size); 
}

/**
 * @brief 根据版本类型的不同，下载版本文件
*/
void SetOtaUpdateInfo(char* ver_name, int ver_size, const char* ver_type, const char* ota_cmd)
{
	LOG_WARN("enter====>\n");
	ota_info.download_percent = 0;
	if(ver_name)
	{
		strncpy(ota_info.ver_name, ver_name, sizeof(ota_info.ver_name)-1);
	}

	if(ver_size)
	{
		ota_info.ver_size = ver_size;
	}

	if(ver_type)
	{
		strncpy(ota_info.ver_type, ver_type, sizeof(ota_info.ver_type)-1);
	}
	
	if(ota_cmd)
	{
		strncpy(ota_info.ota_cmd, ota_cmd, sizeof(ota_info.ota_cmd)-1);
	}

	return;
}

/**
 * @brief 从云平台下载版本，并切换状态为正在下载状态
*/
void OtaDownLoadVer(void)
{	
	pCurrOtaState = &ota_downloading;

	return;
}


void OtaCheckLatestVer(void)
{
	int ret = 0;
	char ver_name[64] = {0};
	int ver_size = 0;
	char buf[64] = {0};
   
	pCurrOtaState = &ota_idle;
	
	ret = IsCanAccessInternet();
	if(0 == ret)
	{
		LOG_ERROR("Not Connect Internet====>\n");
		SetAckNode("ACK_OTA_FAIL_NO_INTERNET");
		SetOtaUpdateInfo(NULL, 0, NULL, "none");
		return;
	}

	/*获取最新的版本号 */
	ver_size = GetLatestVerInfo(ver_name, sizeof(ver_name), ota_info.ver_type);
	if(-1 == ver_size)
	{
		LOG_ERROR("Failed to get version====>\n");
		SetAckNode("ACK_OTA_FAIL_GET_VER");
		SetOtaUpdateInfo(NULL, 0, NULL, "none");
	}
	else if(-2 == ver_size)
	{
		LOG_ERROR("Version have been latest, no need update====>\n");
		SetAckNode("ACK_OTA_FAIL_LATEST_VER");
		SetOtaUpdateInfo(NULL, 0, NULL, "none");
	}
	else
	{
		memset(buf,0 ,sizeof(buf));
		snprintf(buf, sizeof(buf), "%s", ver_name);
		rdn_set(UPGRADE_NODE, "ota_ver", buf);
		
		SetAckNode("ACK_OTA_VERSION");
		LOG_DBG("Latest version is [%s]====>\n",ver_name);
		SetOtaUpdateInfo(ver_name, ver_size, NULL, "none");
	}

	return;
}

/**
 * @brief 检查下载进度百分比，如果下载进度为百分百，则切换为升级状态
*/
void OtaCheckDownLoadProcess(void)
{
	float size = 0;
	float file_size = 0;
	char file_name[64] = {0};
	struct stat statbuf;
	char buf[32] = {0};

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/userdata/ota/%s", ota_info.ver_name);
	file_size = (float)ota_info.ver_size;
	if(0 == access(file_name, F_OK))
	{
		memset(&statbuf, 0, sizeof(statbuf));
		stat(file_name, &statbuf);
		size = (float)(statbuf.st_size);
		ota_info.download_percent = (int)((size/file_size)*100);
		
		memset(buf,0 ,sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", ota_info.download_percent);
		rdn_set(UPGRADE_NODE, "ota_percent", buf);

		SetAckNode("ACK_OTA_PERCENT");
		LOG_DBG("Download [%s] %%%d====>\n", ota_info.ver_name, ota_info.download_percent);
		
		if(size == file_size)	
		{
			pCurrOtaState = &ota_unzip;
			downloading_flag = 0;
		}
		else
		{
			pCurrOtaState = &ota_downloading;
		}
	}

	return;
}

/**
 * @brief 解压版本（系统版本较大）
*/
void OtaUnzipVer(void)
{	
	/*Create temporary thread to unzip version. */
	pCurrOtaState = &ota_unziping;

	return;
}

/**
 * @brief 检查是否解压OK
*/
void OtaCheckUnzipProcess(void)
{
	static int count = 0;
	/*do somthing*/
	if(ota_info.unzip_ok)
	{
		pCurrOtaState = &ota_update;
	}
	LOG_WARN("unziping,use [%d]s====>\n", count++);

	return;
}

/**
 * @brief 根据版本的类型调用对应的升级处理函数，并切换为正在升级状态
*/
void OtaUpdateVer(void)
{
	if(pCurrOtaState == &ota_updating)
	{
		LOG_WARN("OTA updating====>\n");
		return;
	}
	pCurrOtaState = &ota_updating;
	
	if(strcmp(ota_info.ver_type, "System") == 0)
	{
		DoOtaUpdateSystem();
	}
	else if(strcmp(ota_info.ver_type, "App") == 0)
	{
		DoOtaUpdateApp();
	}
	else if(strcmp(ota_info.ver_type, "Mcu") == 0)
	{
		DoOtaUpdateMcu();
	}
	
	return;
}

/**
 * @brief 取消下载，并将状态切换为空置状态
*/
void OtaCancelDownLoad(void)
{
	char buf[64] = {0};

    snprintf(buf, sizeof(buf), FILE_VERSION_PATH, ota_info.ver_name);
	system("killall curl");
	unlink(FILE_GET_LATEST_VER_PHP);
	unlink(FILE_LATEST_VER);
	unlink(FILE_CURL_LOG);
	unlink(buf);
	SetAckNode("ACK_OTA_CANCLE_OK");
	SetOtaUpdateInfo(NULL, 0, NULL, "none");
	downloading_flag = 0;
	pCurrOtaState = &ota_idle;
	return;
}
/**
 * @brief 取消解压，并将状态切换为空置状态
*/
void OtaCancelUnzip(void)
{
	if(strcmp(ota_info.ver_type, "System") == 0)
	{
		system("killall tar");
		system("rm /userdata/ota/*-linux.tar.gz");
		system("rm /userdata/ota/linux.img");
		SetAckNode("ACK_OTA_CANCLE_OK");
	}
	SetOtaUpdateInfo(NULL, 0, NULL, "none");
	unziping_flag = 0;
	pCurrOtaState = &ota_idle;
	return;
}

/**
 * @brief 取消升级，并将状态切换为空置状态
*/
void OtaCancelUpdate(void)
{
	/* if updating ,can't cancel update */
	SetAckNode("ACK_OTA_FAIL_CANT_CANCLE");
	//pCurrOtaState = &ota_idle;
}

/**
 * @brief 取消操作，总函数入口
*/
void DoOtaCancel(void)
{
	if(pCurrOtaState->cancel)
	{
		pCurrOtaState->cancel();
	}
}

/**
 * @brief 确认操作，总函数入口
*/
void DoOtaConfirm(void)
{
	if(pCurrOtaState->confirm)
	{
		pCurrOtaState->confirm();
	}
}

/**
 * @brief 检查最新版本
*/
void DoOtaCheck(void)
{	
	if(pCurrOtaState->check)
	{
		pCurrOtaState->check();
	}
}

/**
 * @brief 初始化OTA状态为空置状态
*/
void OtaInit(void)
{
	//do something
	pCurrOtaState = &ota_idle;	
}

/**
 * @brief 系统升级处理函数
*/
void DoOtaUpdateSystem(void)
{
	char buf[BUF_LEN_128] = {0};

	LOG_WARN("Enter====>\n");

	/*Cant power off, 10min*/
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "update ota /userdata/ota/%s", LINUX_BIN_NAME);
	system(buf);

	LOG_DBG("return 0====>\n");
    return;
}

/**
 * @brief rdn-rk3399升级处理函数
*/
void DoOtaUpdateApp(void)
{
	char buf[BUF_LEN_128] = {0};

	LOG_WARN("Enter====>\n");
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "mv /userdata/ota/%s /userdata/ota/rdn-rk3399", ota_info.ver_name);
	system(buf);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "echo 1 > /userdata/cfg/updateApp");
	LOG_DBG("cmdBuf=%s====>\n",buf);
	system(buf);	
		
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "reboot");
	system(buf);

	LOG_DBG("return====>\n");
	return;
}

/**
 * @brief mcu升级处理函数
*/
void DoOtaUpdateMcu(void)
{
	LOG_WARN("Enter====>\n");
	SetAckNode("ACK_OTA_UPDATE_OK");
	return;
}

/**
 * @brief 循环检查ota 升级的状态
*/
void CheckOtaState(void)
{
	if(0 == strcmp(ota_info.ota_cmd, "none"))
	{
		/*正常情况下直接return*/
		return;
	}

	if(0 == strcmp(ota_info.ota_cmd, "check"))
	{
		/*检查最新版本*/
		ota.check();
	}
	else if(0 == strcmp(ota_info.ota_cmd, "ok"))
	{
		/*升级操作*/
		ota.confirm();
	}
	else if(0 == strcmp(ota_info.ota_cmd, "cancel"))
	{
		/*取消升级操作*/
		ota.cancel();
	}

	return;
}

/**
 * @brief 版本下载，线程处理函数
*/
void* OtaDownloadVerThread(void* p)
{
	char buf[BUF_LEN_256] = {0};

	LOG_WARN("Enter====>\n");
	while(1)
	{
		sleep(3);
		if(pCurrOtaState != &ota_downloading || downloading_flag == 1)
		{
			continue;
		}
		downloading_flag = 1;
		system("rm /userdata/ota/V*");

		unlink(FILE_CURL_LOG);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), CMD_GET_LATEST_VER, ota_info.ver_type, ota_info.ver_name, ota_info.ver_name);
		LOG_WARN("buf=[%s]====>\n", buf);
		system(buf);
	}
	
	return NULL;
}

/**
 * @brief 解压版本，线程处理函数
*/
void* OtaUnzipVerThread(void* p)
{
	char buf[BUF_LEN_128] = {0};

	LOG_WARN("Enter====>\n");

	while(1)
	{
		sleep(3);
		if(pCurrOtaState != &ota_unziping || unziping_flag==1)
		{
			continue;
		}

		unziping_flag = 1;
		ota_info.unzip_ok = 0;
		if(strcmp(ota_info.ver_type, "System") == 0)
		{
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), CMD_UNZIP_VER, ota_info.ver_name);
			system(buf);
		}
		
		SetAckNode("ACK_OTA_UPDATE_OK");
		ota_info.unzip_ok = 1;
		unziping_flag = 0;

		sleep(5);
		LOG_WARN("unzip OK,will update & reboot====>\n");
	}

    return NULL;
}

void* OtaUpgradeThread(void* p)
{
	LOG_WARN("Enter====>\n");	

	OtaInit();
	
	while(1)
	{
		CheckOtaState();
		sleep(3);
	}

	return NULL;
}

