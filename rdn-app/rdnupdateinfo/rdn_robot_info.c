#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include "rdn_info.h"
#include "rdn_api.h"
#include "debug.h"
#include "node.h"

pthread_t rebot_info_th;

static volatile int cur_task_num = 0;
static running_queue_def running_queue[MAX_TASK_NUM] = {0};

/**
 * @brief 创建task
*/
int CreateTask(const void (*fun)(), int period)
{
	if(cur_task_num >= MAX_TASK_NUM)
	{
		LOG_ERROR("over limit max_tasks====>\n");
		return -1;
	}
    running_queue[cur_task_num].fun = fun;
    running_queue[cur_task_num].period = period;
    running_queue[cur_task_num].cnt = 0;
    cur_task_num++;
    return 0;
}

/**
 * @brief 运行调度器
*/
int RunScheduler()
{
    unsigned int i = 0;
    for(i = 0; i < cur_task_num; i++)
    {
        if(running_queue[i].cnt > 0)
        {
            running_queue[i].cnt--;
        }
        else
        {
            running_queue[i].cnt = running_queue[i].period;
            running_queue[i].fun();
        }
    }

    return 0;
}

/**
 * @brief 更新Debug开关状态
*/
void UpdateDbgEnable(void)
{
	char enable[8] = "0";
	char cmd[32] = {0};
	if(0 < rdn_get(ENABLE_NODE, "dbg", enable, sizeof(enable)))
	{
		snprintf(cmd, sizeof(cmd), "echo %s > /tmp/dbg.txt", enable);
		system(cmd);
	}

	return;
}

/**
 * @brief 获取cpu温度并设置到节点中
*/
void UpdateCpuUsage(void)
{
	int i = 0;
    FILE * fp = NULL;
    char cmdbuf[BUF_LEN_64] = {0};
	char usagebuf[BUF_LEN_128] = {0};
	char cpu_usage[11][32] = {0};

    memset(cmdbuf, 0, sizeof(cmdbuf));
	snprintf(cmdbuf, sizeof(cmdbuf), "mpstat -P ALL > %s", FILE_CPU_USAGE_RATE);
	system(cmdbuf);

	fp = fopen(FILE_CPU_USAGE_RATE, "r");
	if(!fp)
	{
		LOG_ERROR("open [%s] fail====>\n", FILE_CPU_USAGE_RATE);
		return;
	}

	memset(usagebuf, 0, sizeof(usagebuf));
	memset(cpu_usage, 0, sizeof(cpu_usage));
	while(fgets(usagebuf, BUF_LEN_128, fp))
	{
		if(i == 3)
		{
			sscanf(usagebuf, "%s %s %s %s %s %s %s %s %s %s %s",cpu_usage[0],cpu_usage[1],cpu_usage[2],cpu_usage[3],
				cpu_usage[4],cpu_usage[5],cpu_usage[6],cpu_usage[7],cpu_usage[8],cpu_usage[9],cpu_usage[10]);
            
            snprintf(usagebuf, sizeof(usagebuf), "%d", 100 - (int)atof(cpu_usage[10]));
            rdn_set(ROBOT_NODE, "cpu_usage", usagebuf);
			break;
		}
		i++;
	}

	fclose(fp);
	unlink(FILE_CPU_USAGE_RATE);

	return;
}

/**
 * @brief 将秒数转换成年/月/日/时/分/秒的格式
 * @param sec 秒数
 * @param mytime 存放转换后的数据（年/月/日/时/分/秒）
 * @param flag 转换最大单位的标志
*/
void GetMyTimeBySecond(int sec, MyTimeType* mytime, int flag)
{
	if(flag == HOUR)
	{
		mytime->year = 0;
		mytime->month = 0;
		mytime->day = 0;
		mytime->hour = sec/SEC_PER_HOUR;
		mytime->minute = sec%SEC_PER_HOUR/SEC_PER_MINUTE;
		mytime->second = sec%SEC_PER_HOUR%SEC_PER_MINUTE;
	}
	else if(flag == DAY)
	{
		mytime->year = 0;
		mytime->month = 0;
		mytime->day = sec/SEC_PER_DAY;
		mytime->hour = sec%SEC_PER_DAY/SEC_PER_HOUR;
		mytime->minute = sec%SEC_PER_DAY%SEC_PER_HOUR/SEC_PER_MINUTE;
		mytime->second = sec%SEC_PER_DAY%SEC_PER_HOUR%SEC_PER_MINUTE;
	}
	else if(flag == YEAR)
	{
		mytime->year = sec/SEC_PER_YEAR;
		mytime->month = sec%SEC_PER_YEAR/SEC_PER_MONTH;
		mytime->day = sec%SEC_PER_YEAR%SEC_PER_MONTH/SEC_PER_DAY;
		mytime->hour = sec%SEC_PER_YEAR%SEC_PER_MONTH%SEC_PER_DAY/SEC_PER_HOUR;
		mytime->minute = sec%SEC_PER_YEAR%SEC_PER_MONTH%SEC_PER_DAY%SEC_PER_HOUR/SEC_PER_MINUTE;
		mytime->second = sec%SEC_PER_YEAR%SEC_PER_MONTH%SEC_PER_DAY%SEC_PER_HOUR%SEC_PER_MINUTE;
	}
	
	return;
}

/**
 * @brief 获取cpu温度并设置到节点中
*/
void UpdateCpuTemp(void)
{
	FILE* fp = NULL;
	char cmdbuf[BUF_LEN_128] = {0};
	char tempbuf[32] = {0};
    int zone0_temp = 0, zone1_temp = 0;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	snprintf(cmdbuf, sizeof(cmdbuf), "cat %s > %s", FILE_CPU0_SYS_TEMP, FILE_CPU0_TEMP);
	system(cmdbuf);

	memset(cmdbuf, 0, sizeof(cmdbuf));
	snprintf(cmdbuf, sizeof(cmdbuf), "cat %s > %s", FILE_CPU1_SYS_TEMP, FILE_CPU1_TEMP);
	system(cmdbuf);

	memset(tempbuf, 0, sizeof(tempbuf));
	fp = fopen(FILE_CPU0_TEMP, "r");
	if(fp)
	{
		if(fgets(tempbuf, BUF_LEN_64, fp))
		{
			zone0_temp = atoi(tempbuf)/1000;
            snprintf(tempbuf, sizeof(tempbuf), "%d", zone0_temp);
            rdn_set(ROBOT_NODE, "cpu_temp1", tempbuf);
		}
		fclose(fp);
	}

	memset(tempbuf, 0, sizeof(tempbuf));
	fp = fopen(FILE_CPU1_TEMP, "r");
	if(fp)
	{
		if(fgets(tempbuf, BUF_LEN_64, fp))
		{
			zone1_temp = atoi(tempbuf)/1000;
            snprintf(tempbuf, sizeof(tempbuf), "%d", zone1_temp);
            rdn_set(ROBOT_NODE, "cpu_temp2", tempbuf);
		}
		fclose(fp);
	}

	unlink(FILE_CPU0_TEMP);
	unlink(FILE_CPU1_TEMP);

	return;
}

void SetLastUsingTimeByNode(char* node)
{
	char buf[32] = {0};
	
	memset(buf, 0, sizeof(buf));
	rdn_get(node, "usingtime", buf, sizeof(buf));
	rdn_set(node, "lasttime", buf);

	return;
}

void SetLastUsingTime(void)
{
	SetLastUsingTimeByNode(ROBOT_NODE);
	SetLastUsingTimeByNode(MAINTAIN_BRUSH_NODE);
	SetLastUsingTimeByNode(MAINTAIN_TIRE_L_NODE);
	SetLastUsingTimeByNode(MAINTAIN_TIRE_R_NODE);
	SetLastUsingTimeByNode(MAINTAIN_SCP_NODE);
	
	SetLastUsingTimeByNode(MAINTAIN_ULT_LF_NODE);
	SetLastUsingTimeByNode(MAINTAIN_ULT_RF_NODE);
	SetLastUsingTimeByNode(MAINTAIN_ULT_L_NODE);
	SetLastUsingTimeByNode(MAINTAIN_ULT_R_NODE);
	
	SetLastUsingTimeByNode(MAINTAIN_WLM_L_NODE);
	SetLastUsingTimeByNode(MAINTAIN_WLM_R_NODE);
	SetLastUsingTimeByNode(MAINTAIN_WLP_L_NODE);
	SetLastUsingTimeByNode(MAINTAIN_WLP_R_NODE);

	SetLastUsingTimeByNode(MAINTAIN_SCPM_NODE);
	SetLastUsingTimeByNode(MAINTAIN_BRM_NODE);
	SetLastUsingTimeByNode(MAINTAIN_BRP_NODE);
	SetLastUsingTimeByNode(MAINTAIN_ABM_NODE);

	SetLastUsingTimeByNode(MAINTAIN_PRES_NODE);
	SetLastUsingTimeByNode(MAINTAIN_LFM_NODE);
	SetLastUsingTimeByNode(MAINTAIN_REMOTE_NODE);
	SetLastUsingTimeByNode(MAINTAIN_CAM_L_NODE);
	SetLastUsingTimeByNode(MAINTAIN_CAM_R_NODE);
	
	return;	
}

void SetUsingTimeByNode(char* node, int sys_time)
{
	char buf[32] = {0};
	char value[32] = {0};
	char timestring[32] = {0};
	MyTimeType mytime;

	memset(buf, 0, sizeof(buf));
	memset(value, 0, sizeof(value));
	memset(timestring, 0, sizeof(timestring));
	memset(&mytime, 0, sizeof(mytime));
	
	rdn_get(node, "lasttime", buf, sizeof(buf));
	snprintf(value, sizeof(value), "%d", atoi(buf) + sys_time);
	rdn_set(node, "usingtime", value);
	GetMyTimeBySecond(atoi(value), &mytime, DAY);
	snprintf(timestring, sizeof(timestring), "%d-%d-%d", mytime.day, mytime.hour, mytime.minute);
	rdn_set(node, "timestring", timestring);

	return;
}

void SetUsingTime(int sys_time)
{
	SetUsingTimeByNode(ROBOT_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_BRUSH_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_TIRE_L_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_TIRE_R_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_SCP_NODE, sys_time);
	
	SetUsingTimeByNode(MAINTAIN_ULT_LF_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_ULT_RF_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_ULT_L_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_ULT_R_NODE, sys_time);
	
	SetUsingTimeByNode(MAINTAIN_WLM_L_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_WLM_R_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_WLP_L_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_WLP_R_NODE, sys_time);

	SetUsingTimeByNode(MAINTAIN_SCPM_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_BRM_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_BRP_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_ABM_NODE, sys_time);

	SetUsingTimeByNode(MAINTAIN_PRES_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_LFM_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_REMOTE_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_CAM_L_NODE, sys_time);
	SetUsingTimeByNode(MAINTAIN_CAM_R_NODE, sys_time);

	return;	
}

void SetActiveTime(int sys_time)
{
	char buf[32] = {0};
	char locked[32] = {0};
	char active_time[32] = {0};
	
	rdn_get(ROBOT_NODE, "locked", locked, sizeof(locked));
	rdn_get(ROBOT_NODE, "activetime", active_time, sizeof(active_time));
	if(atoi(locked) == 0)
	{
		/* active status,calculate remaining active time */
		snprintf(buf, sizeof(buf), "%d", atoi(active_time) - UPDATE_CYCLE_45S);
		rdn_set(ROBOT_NODE, "activetime", buf);		

		if(atoi(buf) <= 60)
		{
			rdn_set(ROBOT_NODE, "locked", "1");
			system("rdnsave");
			LOG_WARN("Robot will be locked after reboot，please active it====>\n");
		}
	}

	return;
}

void SetCurrTime(int sys_time)
{
	char buf[32] = {0};
	
	snprintf(buf, sizeof(buf), "%d", sys_time);
	rdn_set(ROBOT_NODE, "currtime", buf);

	return;
}

/***
 * 
 * 单位时间清扫面积
 * currarea(当前清扫面积)/currtime(当前使用时间)*3600 
 * 
*/
void SetUnitArea(void)
{
	char currarea[32] = {0};
	char currtime[32] = {0};
	int unitarea = 0;
	char buf[32] = {0};

	rdn_get(ROBOT_NODE, "currarea", currarea, sizeof(currarea));
	rdn_get(ROBOT_NODE, "currtime", currtime, sizeof(currtime));

	unitarea = atoi(currarea) / atoi(currtime) * 3600;
	snprintf(buf, sizeof(buf), "%d", unitarea);
	rdn_set(ROBOT_NODE, "unitarea", buf);

	return;
}

/**
 * @brief 获取机器使用时间
*/
void UpdateUsingTime(void)
{
	FILE* fp = NULL;
	char buf[32] = {0};
	char sys_time_s[32] = {0};
	int  sys_time_i = 0;

	fp = fopen(FILE_SYS_TIME, "r");
	if(fp)
	{
		if(fgets(buf, 32, fp))
		{
			memset(sys_time_s, 0, sizeof(sys_time_s));
			sscanf(buf, "%s", sys_time_s);
			sys_time_i = atoi(sys_time_s);
		}
		fclose(fp);
	}

	/* update the sys using time */
	SetUsingTime(sys_time_i);
	SetActiveTime(sys_time_i);
	SetCurrTime(sys_time_i);

	return;
}

void InitRobotNode(void)
{
	//rdn_set(ROBOT_NODE, "active", "1");
}

/**
 * @brief 保存数据到XML文件
*/
void SaveXmlConfig(void)
{
	system("rdnsave");
}

int SetSysVersion(void)
{
	FILE* fp = NULL;
	char buf[BUF_LEN_64] = {0};
	char sys_version[BUF_LEN_64] = "V0.0.0";

	fp = fopen(FILE_SYS_VER, "r");
	if(!fp)
	{
		LOG_ERROR("open [%s] fail====>\n", FILE_SYS_VER);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	if(fgets(buf, BUF_LEN_64, fp))
	{
		strncpy(sys_version, buf, sizeof(sys_version)-1);
		sys_version[strlen(sys_version)-1] = '\0';
	}
	fclose(fp);

	rdn_set(VERSION_NODE, "sysver", sys_version);
	return 0;
}

/**
 * @brief 从vendor分区中读取数据
 * @param vid 读取的数据类型
 * @param buf 存放读取到的数据
 * @param len 最大允许读取的长度
*/
int ReadVendor(enum VENDOR_ID vid,char* buf,int len)
{
	int ret;
	uint8 p_buf[sizeof(struct rk_vendor_req)+64];
	struct rk_vendor_req *req;
	req = (struct rk_vendor_req *)p_buf;
	int sys_fd = open("/dev/vendor_storage", O_RDWR, 0); 
	if(sys_fd < 0)
	{
		LOG_ERROR("vendor storage open fail====>\n");
		return -1;
	}
	req->tag = VENDOR_REQ_TAG;
	req->id = vid;
	req->len = len;
	
    ret = ioctl(sys_fd, VENDOR_READ_IO, req);
    if (!ret) 
	{
        memcpy(buf, req->data, len);
    }
	else
	{
		LOG_ERROR("Vendor read error====>\n");
	}
	close(sys_fd);
	return ret;
}

/**
 * @brief 将数据写入vendor分区
 * @param vid 写入的数据类型
 * @param buf 写入的数据
 * @param len 最大允许写入的长度
*/
int WriteVendor(enum VENDOR_ID vid,char* buf,int len)
{
	int ret;
	uint8 p_buf[sizeof(struct rk_vendor_req)+64];
	struct rk_vendor_req *req;
	int i;
	int sys_fd = open("/dev/vendor_storage", O_RDWR, 0); 
	req = (struct rk_vendor_req *)p_buf;
	if(sys_fd < 0)
	{
		LOG_ERROR("Vendor storage open fail====>\n");
		return -1;
	}
	req->tag = VENDOR_REQ_TAG;
	req->id = vid;
	req->len = len;
	for (int i = 0; i < len; i++)
	{
		req->data[i] = buf[i];
	}
	ret = ioctl(sys_fd, VENDOR_WRITE_IO, req);
	if(ret) 
	{
		LOG_ERROR("Vendor write fail====>\n");
	}
	
	close(sys_fd);
	return ret;
}

int SetSnInfo(void)
{
	int ret = -1;
	char sn[32] = {0};
	char name[32] = {0};
	char apname[32] = {0};
	
	ret = ReadVendor(VENDOR_SN_ID, sn, VENDOR_LENTH);
	if(!ret)
	{
		LOG_DBG("SN=%s====>\n", sn);
		rdn_set(ROBOT_NODE, "sn", sn);
		rdn_get(ROBOT_NODE, "name", name, sizeof(name));
		if(0 == strcmp(name, DEFAULT_ROBOT_NAME))
		{
			rdn_set(ROBOT_NODE, "name", sn);
		}
		rdn_get(WL_AP_NODE, "ssid", apname, sizeof(apname));
		if(0 == strcmp(apname, DEFAULT_AP_NAME))
		{
			rdn_set(WL_AP_NODE, "ssid", sn);
		}

		system("rdnsave");	
	}
	
	return 0;
}

void* UpdateRebotInfoThread(void* arg)
{
	LOG_WARN("Enter====>\n");

	InitRobotNode();
	SetSysVersion();
	SetSnInfo();
	SetLastUsingTime();
	
	CreateTask(UpdateDbgEnable, UPDATE_CYCLE_5S);
	CreateTask(UpdateCpuTemp, UPDATE_CYCLE_15S);
	CreateTask(UpdateCpuUsage, UPDATE_CYCLE_30S);
	CreateTask(UpdateUsingTime, UPDATE_CYCLE_60S);
	CreateTask(SetUnitArea, UPDATE_CYCLE_60S);
	CreateTask(SaveXmlConfig, UPDATE_CYCLE_60S);
	CreateTask(CheckErrorInfo, UPDATE_CYCLE_5S);
	
	while(1)
	{
		RunScheduler();
		sleep(1);
	}

	return NULL;
}
