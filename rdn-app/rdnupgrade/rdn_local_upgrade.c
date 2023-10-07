#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "json.h"
#include "rdn_upgrade.h"

pthread_t local_upgrade_th;

void SetAckNode(char* acknum)
{
	rdn_set(GLOBAL_NODE, "acknum", acknum);
	return;
}

int SendMsgToQueue(int msg_type, char* msg)
{  
	int msgid = -1;
	msg_data_t msg_data;

	memset(&msg_data, 0, sizeof(msg_data_t));
	//建立消息队列
	msgid = msgget((key_t)MSG_QUEUE_KEY, 0666 | IPC_CREAT);
	if(msgid == -1)
	{  
		LOG_ERROR("msgget failed,return -1====>\n");  
		return -1;
	}
	
	LOG_WARN("msgid=%d====>\n", msgid);			
	msg_data.msg_type = msg_type;
	if(msg)
	{
		strncpy(msg_data.msg_text, msg, sizeof(msg_data.msg_text)-1);
	}
	//向队列发送数据  
	if(msgsnd(msgid, (void*)&msg_data, sizeof(msg_data_t), 0) == -1) 
	{	   
		LOG_ERROR("msgsnd failed====>\n");    
		return -1;	  
	}
	
	LOG_WARN("Send msg:[%d] to queue====>\n", msg_type);   
	return 0;
}

/**
 * @brief 检查文件的字节数，并和收到的字节数对比
 * @param filename 文件名
 * @param rcvsize 收到的字节大小
 * @return 返回对比的结果
 * @retval 对比结果一致返回1，不一致返回0
*/
int CheckFileSize(char* filename, int rcvsize)
{
	int count = 0;
	int size = 0;
    struct stat statbuf;

	while(1)
	{
		if((access(filename, F_OK)) == -1)
		{
			continue;
		}

		memset(&statbuf, 0, sizeof(statbuf));
		stat(filename, &statbuf);
		size = statbuf.st_size;
		if(size > 0 && size == rcvsize)
		{
			LOG_DBG("Check file size ok, loop time is [%d]====>\n", count);
			return 1;
		}
		
		sleep(1);
		count++;
		if(count == 30)
		{
			LOG_DBG("size of [%s] is [%d],rcv size is [%d]====>\n",filename, size, rcvsize);
			return 0;
		}
	}

	return 0;
}

void SendSignalToRk3399Process(void)
{
	int pid = 0;   
	FILE* fp = NULL;
	char buf[64] = {0};
	union sigval value;
	char* pid_file = "/tmp/rk3399.pid";

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "pidof rdn-rk3399 > %s", pid_file);	
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
	}
	
	value.sival_int = 1;	
	sigqueue(pid, SIGUSR1, value);
	
	LOG_WARN("Send signal to rdn-rk3399====>\n");
	return;
}

void HandleUpgradeOta(void)
{
	char ota_action[32] = {0};
	char ota_type[32] = {0};

	memset(ota_action, 0, sizeof(ota_action));
	rdn_get(UPGRADE_NODE, "ota_action", ota_action, sizeof(ota_action));
	memset(ota_type, 0, sizeof(ota_type));
	rdn_get(UPGRADE_NODE, "ota_type", ota_type, sizeof(ota_type));

	LOG_WARN("ver_type=[%s],action=[%s]====>\n", ota_type, ota_action);

	/*设置OTA升级相关的参数*/
	SetOtaUpdateInfo(NULL, 0, ota_type, ota_action);	
		
	LOG_DBG("return====>\n");
	return;
}

void HandleUpgradeMcu(void)
{
	char filepath[64] = {0};
	char filename[64] = {0};
	char filesize[16] = {0};
	char buf[BUF_LEN_128] = {0};

	memset(filename, 0, sizeof(filename));
	rdn_get(UPGRADE_NODE, "filename", filename, sizeof(filename));
	memset(filesize, 0, sizeof(filesize));
	rdn_get(UPGRADE_NODE, "filesize", filesize, sizeof(filesize));
	
	memset(filepath, 0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath), "/userdata/ota/%s", filename);
	if(0 == CheckFileSize(filepath, atoi(filesize)))
	{
		LOG_ERROR("The size of [%s] is incorrect====>\n", filepath);
		SetAckNode("ACK_MCU_FAIL_SIZE");
		return;
	}

	if(!strstr(filename, MCU_BIN_NAME))
	{
		LOG_ERROR("The fileName:[%s] is incorrect,must be V0.0.1234.19700101.dbg-mcu.bin====>\n", filename);
		SetAckNode("ACK_MCU_FAIL_NAME");
		return;
	}

	SetAckNode("ACK_MCU_FILE_OK");
	SendMsgToQueue(MSG_TYPE_UPGRADE_MCU, NULL);

	return;
}

void HandleUpgradeSys(void)
{
	char filepath[64] = {0};
	char filename[64] = {0};
	char filesize[16] = {0};
	char buf[BUF_LEN_128] = {0};
	
	LOG_WARN("Enter====>\n");

	memset(filename, 0, sizeof(filename));
	rdn_get(UPGRADE_NODE, "filename", filename, sizeof(filename));
	memset(filesize, 0, sizeof(filesize));
	rdn_get(UPGRADE_NODE, "filesize", filesize, sizeof(filesize));
	
	memset(filepath, 0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath), "/userdata/ota/%s", filename);
	if(0 == CheckFileSize(filepath, atoi(filesize)))
	{
		LOG_ERROR("The size of [%s] is incorrect====>\n", filepath);
		SetAckNode("ACK_SYS_FAIL_SIZE");
		return;
	}
	
	if(strstr(filename, LINUX_TAR_NAME))
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "tar -zxvf /userdata/ota/%s -C /userdata/ota/", filename);
		system(buf);
	}
	else
	{
		LOG_ERROR("The fileName:[%s] is incorrect,must be V0.0.1234.19700101.dbg-linux.tar.gz====>\n", filename);
		SetAckNode("ACK_SYS_FAIL_NAME");
		return;
	}

	SetAckNode("ACK_SYS_OK");

	sleep(5);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "update ota /userdata/ota/%s", LINUX_BIN_NAME);
	system(buf);

	LOG_DBG("Successfully====>\n");
	return;
}

void HandleUpgradeApp(void)
{
	char filepath[64] = {0};
	char filename[64] = {0};
	char filesize[16] = {0};
	char buf[BUF_LEN_128] = {0};
	
	LOG_WARN("Enter====>\n");

	memset(filename, 0, sizeof(filename));
	rdn_get(UPGRADE_NODE, "filename", filename, sizeof(filename));
	memset(filesize, 0, sizeof(filesize));
	rdn_get(UPGRADE_NODE, "filesize", filesize, sizeof(filesize));
	
	memset(filepath, 0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath), "/userdata/ota/%s", filename);
	if(0 == CheckFileSize(filepath, atoi(filesize)))
	{
		LOG_ERROR("The size of [%s] is incorrect====>\n", filepath);
		SetAckNode("ACK_APP_FAIL_SIZE");
		return;
	}
	
	memset(buf, 0, sizeof(buf));
	if(strstr(filename, APP_BIN_NAME))
	{
		snprintf(buf, sizeof(buf), "echo 1 > /userdata/cfg/update.cfg");
		LOG_DBG("cmdBuf=%s====>\n",buf);
		system(buf);	
	}
	else
	{
		LOG_ERROR("The fileName:[%s] is incorrect,must like V0.0.1234.19700101.dbg-rdn-rk3399====>\n", filename);
		SetAckNode("ACK_APP_FAIL_NAME");
		return;
	}
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "mv /userdata/ota/%s /userdata/ota/rdn-rk3399", filename);
	system(buf);

	SetAckNode("ACK_APP_OK");
	
	sleep(5);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "reboot");
	system(buf);

	LOG_DBG("Successfully====>\n");
	return;
}

void HandleActiveRobot(void)
{
	int i = 0; 
	int len = 0;
	int sum = 0;
	int result = 0;
	int limit_hour = 0;
	int expirate_date = 0;
	char limit_hour_str[32] = {0};	
	char expirate_date_str[32] = {0};
	char sum_str[32] = {0}; 
	char active_code[32] = {0};
	char active_date[32] = {0};
	char buf[32] = {0};
	char* sn_ptr = NULL;
	char sn[32] = {0};

	memset(sn, 0, sizeof(sn));
	rdn_get(ROBOT_NODE, "sn", sn, sizeof(sn));
	sn_ptr = sn;
	
	//-1：永久激活状态 0：激活状态（有时间限制，时间取决于m_active_time） 1：锁定状态
	memset(buf, 0, sizeof(buf));
	rdn_get(ROBOT_NODE, "locked", buf, sizeof(buf));
	if( atoi(buf) != 1)
	{
		LOG_DBG("Robot is active currently====>\n");
		SetAckNode("ACK_ACTIVE_REPEAT");
		return;
	}

	memset(active_code, 0, sizeof(active_code));
	rdn_get(UPGRADE_NODE, "active_code", active_code, sizeof(active_code));
	memset(active_code, 0, sizeof(active_code));
	rdn_get(UPGRADE_NODE, "active_date", active_date, sizeof(active_date));
	
	len = strlen(active_code);
	if(len != LEN_ACTIVE_CODE)
	{
		LOG_DBG("Active code lenth is not 17====>\n");
		SetAckNode("ACK_ACTIVE_FAIL");
		return;
	}
	
	for(i=0; i<LEN_ACTIVE_CODE; i++)
	{
		if(0 == isxdigit(*(active_code+i)))
		{
			LOG_DBG("Some character is not HEX====>\n");
			SetAckNode("ACK_ACTIVE_FAIL");
			return;
		}
	}

	/*
	*17位激活码如【0561 0fc18 fecb7598】:
	*前4位为机器序列号/机器激活可用时长/激活码失效日期,3个项目字符串对应的ascII码的总和（16进制）
	*中间5位为机器激活可用时长与0xFFFF异或之后的结果（16进制），99999小时（差不多10年）表示永久激活
	*后8位为激活码失效日期与0xFFFFFFFF异或后的结果(16进制)
	*/
	strncpy(sum_str, active_code, 4);
	strncpy(limit_hour_str, active_code + 4, 5);
	strncpy(expirate_date_str, active_code + 9, 8);

	LOG_DBG("sum_str=%s,limit_hour_str=%s,expirate_date_str=%s====>\n",sum_str,limit_hour_str,expirate_date_str);

	sscanf(sum_str, "%04x", &sum);
	sscanf(limit_hour_str, "%05x", &limit_hour);
	sscanf(expirate_date_str, "%08x", &expirate_date);
	LOG_DBG("sum=0x%x,limit_hour=0x%x,expirate_date=0x%x====>\n", sum, limit_hour, expirate_date);

	memset(limit_hour_str, 0, sizeof(limit_hour_str));
	memset(expirate_date_str, 0, sizeof(expirate_date_str));
	sum ^= 0XFFFF;
	snprintf(limit_hour_str, sizeof(limit_hour_str), "%d", (limit_hour ^ 0XFFFF));
	snprintf(expirate_date_str, sizeof(expirate_date_str), "%d", (expirate_date ^ 0XFFFFFFFF));
	LOG_DBG("sum=%d,limit_hour_str=%s,expirate_date_str=%s====>\n",sum,limit_hour_str,expirate_date_str);

	if(atoi(active_date) > atoi(expirate_date_str))
	{
		LOG_DBG("Activation code expired====>\n");
		SetAckNode("ACK_ACTIVE_CODE_EXPIRED");
		return;
	}

	for(i=0; i<strlen(sn_ptr); i++)
	{
		result += *(sn_ptr+i);
	}

	for(i=0; i<strlen(limit_hour_str); i++)
	{
		result += *(limit_hour_str+i);
	}

	for(i=0; i<strlen(expirate_date_str); i++)
	{
		result += *(expirate_date_str+i);
	}
	
	if(result == sum)
	{
		if(atoi(limit_hour_str) == 99999)
		{
			/*Active forever*/
			rdn_set(ROBOT_NODE, "locked", "-1");
			rdn_set(ROBOT_NODE, "activetime", "0");
		}
		else
		{
			rdn_set(ROBOT_NODE, "locked", "0");
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%d", atoi(limit_hour_str) * 60 * 60);
			rdn_set(ROBOT_NODE, "activetime", buf);
		}
		system("rdnsave");
		SetAckNode("ACK_ACTIVE_SUCCESS");
		LOG_DBG("Activated successfully,please reboot====>\n"); 
	}
	else
	{
		SetAckNode("ACK_ACTIVE_FAIL");
		LOG_DBG("Activated faild====>\n");	
	}
	
	LOG_DBG("Successfully====>\n");
	return ;
}


void HandleUpgradeCfg(void)
{
	char filepath[64] = {0};
	char filename[64] = {0};
	char filesize[16] = {0};
	char buf[BUF_LEN_128] = {0};

	memset(filename, 0, sizeof(filename));
	rdn_get(UPGRADE_NODE, "filename", filename, sizeof(filename));
	memset(filesize, 0, sizeof(filesize));
	rdn_get(UPGRADE_NODE, "filesize", filesize, sizeof(filesize));
	
	memset(filepath, 0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath), "/userdata/ota/%s", filename);
	if(0 == CheckFileSize(filepath, atoi(filesize)))
	{
		LOG_ERROR("The size of [%s] is incorrect====>\n", filepath);
		SetAckNode("ACK_CFG_FAIL_SIZE");
		return;
	}
	
	memset(buf, 0, sizeof(buf));
	if(strstr(filename, COMMON_CONFIG_FILE))
	{
		snprintf(buf, sizeof(buf), "echo 2 > /userdata/cfg/update.cfg");
		LOG_DBG("cmdBuf=%s====>\n",buf);
		system(buf);	
	}
	else
	{
		LOG_ERROR("The fileName:[%s] is incorrect,must like V1.0.1234.19700101.dbg-configure.yml====>\n", filename);
		SetAckNode("ACK_CFG_FAIL_NAME");
		return;
	}
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "mv /userdata/ota/%s /userdata/ota/configure.yml", filename);
	system(buf);
	
	SetAckNode("ACK_CFG_OK");
	sleep(5);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "reboot");
	system(buf);

	LOG_DBG("Successfully====>\n");
	return;
}

void HandleBackupData(void)
{
	char cmd[128] = {0};
	char sn[32] = {0};
	
	memset(sn, 0, sizeof(sn));
	rdn_get(ROBOT_NODE, "sn", sn, sizeof(sn));
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "/userdata/shell/backup.sh %s", sn);
	system(cmd);
	
	SetAckNode("ACK_BACKUP_OK");
	LOG_DBG("Successfully====>\n");
	return ;
}

void HandleImportData(void)
{
	char filepath[64] = {0};
	char filename[64] = {0};
	char filesize[16] = {0};
	char cmd[128] = {0};
	
	memset(filename, 0, sizeof(filename));
	rdn_get(UPGRADE_NODE, "filename", filename, sizeof(filename));
	memset(filesize, 0, sizeof(filesize));
	rdn_get(UPGRADE_NODE, "filesize", filesize, sizeof(filesize));
	
	memset(filepath, 0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath), "/userdata/ota/%s", filename);
	if(0 == CheckFileSize(filepath, atoi(filesize)))
	{
		LOG_ERROR("The size of [%s] is incorrect====>\n", filepath);
		SetAckNode("ACK_IMPORT_FAIL_SIZE");
		return;
	}
	
	if(!strstr(filename, BACKUP_FILE))
	{
		LOG_ERROR("The fileName:[%s] is incorrect,must like xxx_userdata.tar.gz====>\n", filename);
		SetAckNode("ACK_IMPORT_FAIL_NAME");
		return;
	}

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "/userdata/shell/import.sh %s", filename);
	system(cmd);
	
	SetAckNode("ACK_IMPORT_OK");

	sleep(5);
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "reboot");
	system(cmd);
	
	LOG_DBG("Successfully====>\n");
	return;
}

upgrade_t upgrade_table[]=
{
	{"ota", 		HandleUpgradeOta},
	{"UpdateMCU",	HandleUpgradeMcu},
	{"UpdateSYS", 	HandleUpgradeSys},
	{"UpdateAPP", 	HandleUpgradeApp},
	{"ActiveRobot", HandleActiveRobot},
	{"UpdateCFG", 	HandleUpgradeCfg},
	{"BackupData", 	HandleBackupData},
	{"ImportData", 	HandleImportData}
	
};

void HandleUpgrade(void)
{
	int i = 0;
	char cmd[32] = {0};	
	int size = sizeof(upgrade_table)/sizeof(upgrade_table[0]);

	LOG_WARN("Enter====>\n");
	
	rdn_get(UPGRADE_NODE, "cmd", cmd, sizeof(cmd));
	for(i=0; i<size; i++)
	{
		if(0 == strcmp(cmd, upgrade_table[i].cmd))
		{
			upgrade_table[i].handle_fun();
			break;
		}
	}

	return;
}

void* LocalUpgradeThread(void* p)
{
	int msg_qnum = 0;
	int msgid = -1;
    msg_data_t data;
	struct msqid_ds msg_queue_attr;
	long int msgtype = MSG_TYPE_UPGRADE;
   
	LOG_WARN("Enter====>\n");
	//建立消息队列   
    msgid = msgget((key_t)MSG_QUEUE_KEY, 0666 | IPC_CREAT); 
    if(msgid == -1)
    {      
        LOG_WARN("msgget failed ====>\n");
       	return NULL;
    }  

#if 0
	msgtype = 0 :接收第一个消息
	msgtype > 0 :接收类型等于msgtyp的第一个消息
	msgtype < 0 :接收类型等于或者小于msgtyp绝对值的第一个消息
#endif

	while(1)
	{
		memset(&msg_queue_attr, 0, sizeof(msg_queue_attr));
		if(0 == msgctl(msgid, IPC_STAT, &msg_queue_attr))
		{
			msg_qnum = msg_queue_attr.msg_qnum;
			if(msg_qnum > 0)
			{
				if(msgrcv(msgid, (void*)&data, sizeof(msg_data_t), msgtype, IPC_NOWAIT) > 0)  
				{  
					HandleUpgrade();
				}  
			}
		}
		sleep(1);
	}
		
	return NULL;
}
