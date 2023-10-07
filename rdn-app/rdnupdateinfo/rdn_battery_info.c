#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <termios.h>        // 串口操作库
#include "rdn_info.h"
#include "rdn_api.h"
#include "debug.h"
#include "node.h"

pthread_t uart_bat_th;

///电池包数据的指令格式
unsigned char battery_cmd[][3]={
	{0x0a, 0xa0, 0x00}, 	/*0：	握手*/
	{0x0a, 0x01, 0x02},		/*1：	电芯1的电压值(mV)*/
	{0x0a, 0x02, 0x02}, 	/*2：	电芯2的电压值(mV)*/
	{0x0a, 0x03, 0x02}, 	/*3：	电芯3的电压值(mV)*/
	{0x0a, 0x04, 0x02}, 	/*4：	电芯4的电压值(mV)*/
	{0x0a, 0x05, 0x02}, 	/*5：	电芯5的电压值(mV)*/
	{0x0a, 0x06, 0x02}, 	/*6：	电芯6的电压值(mV)*/
	{0x0a, 0x07, 0x02}, 	/*7：	电芯7的电压值(mV)*/
	{0x0a, 0x08, 0x02}, 	/*8：	电芯8的电压值(mV)*/
	{0x0a, 0x09, 0x02}, 	/*9：	电芯9的电压值(mV)*/
	{0x0a, 0x0a, 0x02}, 	/*10：	电芯10的电压值(mV)*/
	{0x0a, 0x0b, 0x02}, 	/*11：	电芯总电压值(mV)*/
	{0x0a, 0x0c, 0x02},		/*12：	电芯温度1(℃)*/
	{0x0a, 0x0d, 0x02},		/*13：	电芯温度2(℃)*/
	{0x0a, 0x0e, 0x02},		/*14：	芯片内部温度1(℃)*/
	{0x0a, 0x0f, 0x02},		/*15：	芯片内部温度2(℃)*/
	{0x0a, 0x10, 0x04},		/*16：	实时电流值(mA)*/
	{0x0a, 0x11, 0x04},		/*17：	系统满充容量（mAH）*/
	{0x0a, 0x12, 0x04},		/*18：	电池包当前剩余电量（mAH）*/
	{0x0a, 0x13, 0x02},		/*19：	电池包剩余电量百分比（%）*/
	{0x0a, 0x14, 0x02},		/*20：	循环放电次数*/
	{0x0a, 0x15, 0x02},		/*21：	系统状态*/
	{0x0a, 0x16, 0x02},		/*22：	电池保护状态*/
	{0x0a, 0x17, 0x02},		/*23：	系统配置参数*/
	{0x0a, 0x78, 0x17},		/*24：	制造信息*/
};


/**
 * @brief 初始化串口
 * @param fd 串口文件描述符
 * @param speed 串口速度
 * @param flow_ctrl  数据流控制
 * @param databits 数据位 取值为 7 或者8
 * @param stopbits 停止位   取值为 1 或者2
 * @param parity 效验类型 取值为N,E,O,,S
 * @return 正确返回为1，错误返回为0
 */
int InitSerial(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
    unsigned int i;
    int speed_arr[] = {B115200, B57600, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arr[] = {115200, 57600, 19200, 9600, 4800, 2400, 1200, 300};
    struct termios options;
    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.
    */
    if(tcgetattr( fd,&options)!=0) {
        perror("SetupSerial 1");
        return(FALSE);
    }
    //设置串口输入波特率和输出波特率
    for (i= 0;i < sizeof(speed_arr)/sizeof(int);i++) {
        if(speed == name_arr[i]) {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }
    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;
    //设置数据流控制
    switch(flow_ctrl) {
    case 0://不使用流控制
        options.c_iflag &= ~(ICRNL | IXON);
        options.c_cflag &= ~CRTSCTS;
        break;
    case 1://使用硬件流控制
        options.c_cflag |= CRTSCTS;
        break;
    case 2://使用软件流控制
        options.c_cflag |= IXON | IXOFF | IXANY;
        break;
    default:
        fprintf(stderr,"Unsupported flow_ctrl\n");
        return FALSE;
        break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits) {
    case 5:
        options.c_cflag |= CS5;
        break;
    case 6:
        options.c_cflag |= CS6;
        break;
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr,"Unsupported data size\n");
        return FALSE;
    }
    //设置校验位
    switch (parity) {
    case 'n':
    case 'N': //无奇偶校验位。
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O'://设置为奇校验
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E'://设置为偶校验
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
        break;
    case 's':
    case 'S': //设置为空格
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        fprintf(stderr,"Unsupported parity\n");
        return FALSE;
    }
    // 设置停止位
    switch (stopbits) {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        fprintf(stderr,"Unsupported stop bits\n");
        return FALSE;
    }
    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//我加的
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */
    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    //激活配置 (将修改后的termios数据设置到串口中）
    if(tcsetattr(fd,TCSANOW,&options) != 0) {
        perror("com set error!\n");
        return FALSE;
    }
    tcflush(fd,TCIFLUSH);
    return TRUE;
}


/**
 * @brief 打开电池包串口
 * @return 成功打开返回设备描述符
 * @retval 大于0表示成功，-1表示失败
*/
int OpenBatteryUart(void)
{
	int ret = 0;
	int fd = -1;

    fd = open(UART_BAT_DEV_PATH, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK, 0777);
	if(fd < 0)
	{
		LOG_ERROR("open bat serial com fail====>\n");
		return -1;
	}
    ret = InitSerial(fd, 9600, 0, 8, 1, 'N');
	if(ret < 0)
	{
		LOG_ERROR("init bat serial com fail====>\n");
		close(fd);
		return -1;
	}

	return fd;
}

void ParseBatMakeInfo(unsigned char* buf)
{
	char tmp[32] = {0};

	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "V%x.%x", buf[0], buf[1]);
	LOG_DBG("SW=%s====>\n",tmp);
	rdn_set(BATTERY_NODE, "sw_ver", tmp);
	
	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "V%x.%x", buf[2], buf[3]);
	rdn_set(BATTERY_NODE, "hw_ver", tmp);
	LOG_DBG("HW=%s====>\n",tmp);
	
	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "%x%x-%x-%x", buf[17], buf[18], buf[19], buf[20]);
	rdn_set(BATTERY_NODE, "make_date", tmp);
	LOG_DBG("DATE=%s====>\n",tmp);

	rdn_set(BATTERY_NODE, "maker", "CZXH");
	return;
}

/**
 * @brief 获取电池包的特定数据
 * @param fd 串口描述符
 * @param type 指令类型
 * @return 返回获取到的值
*/
int GetBatValue(int fd, int type)
{
	int ret_w = -1;
	int ret_r = -1;
	unsigned char buf[32] = {0};
	int value = 0;
	int count = 0;
	unsigned char bat_makeinfo_cmd[5] = {0x0B, 0x77, 0x01, 0x01, 0xE9};
	
	if(type == BAT_MAKE_INFO)
	{
		write(fd, bat_makeinfo_cmd, sizeof(bat_makeinfo_cmd));
		while(ret_r < 0)
		{	
			count++;
			usleep(1000*10);
			memset(buf, 0, sizeof(buf));
			ret_r = read(fd, buf, sizeof(buf));
			if(count >= 100)
			{
				LOG_DBG("Try read 0x5a 100 times,fail====>\n");
				return -1;
			}
		}
		if(buf[0] != 0x5A)
		{
			LOG_DBG("Get bat make info fail====>\n");
			return -1;
		}
	}
	
	ret_w = write(fd, battery_cmd[type], sizeof(battery_cmd[type]));
	if(ret_w > 0)
	{
		count = 0;
		ret_r = -1;
		memset(buf, 0, sizeof(buf));
		while(ret_r < 0)
		{	
			count++;
			sleep(1);
			ret_r = read(fd, buf, sizeof(buf));
			if(count >= 20)
			{
				return -1;
			}
		}
		if(type == BAT_MAKE_INFO)
		{
			ParseBatMakeInfo(buf);
		}
		else
		{
			if(ret_r > 4)
			{		
	            value = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | (buf[3]<<0);
			}
			else
			{
	            value = (buf[0]<<8) | (buf[1]<<0);
			}
		}
		
	}

	return value;
}

void InitBatteryNode(void)
{
	rdn_set(BATTERY_NODE, "state", "1");
	rdn_set(BATTERY_NODE, "v1", "-1");
	rdn_set(BATTERY_NODE, "v2", "-1");
	rdn_set(BATTERY_NODE, "v3", "-1");
	rdn_set(BATTERY_NODE, "v4", "-1");
	rdn_set(BATTERY_NODE, "v5", "-1");
	rdn_set(BATTERY_NODE, "v6", "-1");
	rdn_set(BATTERY_NODE, "v7", "-1");
	rdn_set(BATTERY_NODE, "v8", "-1");
	rdn_set(BATTERY_NODE, "v9", "-1");
	rdn_set(BATTERY_NODE, "v10", "-1");
	rdn_set(BATTERY_NODE, "vt", "-1");
	rdn_set(BATTERY_NODE, "bat_t1", "-274");
	rdn_set(BATTERY_NODE, "bat_t2", "-274");
	rdn_set(BATTERY_NODE, "ic_t1", "-274");
	rdn_set(BATTERY_NODE, "ic_t2", "-274");
	rdn_set(BATTERY_NODE, "current", "-1");
	rdn_set(BATTERY_NODE, "full_cap", "-1");
	rdn_set(BATTERY_NODE, "rem_cap", "-1");
	rdn_set(BATTERY_NODE, "percent_cap", "-1");
	rdn_set(BATTERY_NODE, "cycle_count", "-1");
	rdn_set(BATTERY_NODE, "sys_state", "-1");
	rdn_set(BATTERY_NODE, "protect_state", "-1");
	rdn_set(BATTERY_NODE, "sys_config", "-1");
	rdn_set(BATTERY_NODE, "sw_ver", "V0.0");
	rdn_set(BATTERY_NODE, "hw_ver", "V0.0");
	rdn_set(BATTERY_NODE, "maker", "CZXH");
	rdn_set(BATTERY_NODE, "make_date", "2023-01-01");
	rdn_set(BATTERY_NODE, "health", "100");
}

/**
 * @brief 更新电池包的数据信息
 * @param batvalue_buf 获取的指定电池包数据
 * @param battery_buf  节点数据存储
*/
void* UpdateBatteryInfoThread(void* arg)
{
	int ret = -1;
	int fd = -1;
	int count = 0;
	int batvalue_buf = 0;
    char battery_buf[32] = {0};

	//初始化电池包信息
	InitBatteryNode();
    
	LOG_WARN("enter====>\n");
	while(fd < 0)
	{
		count++;
		fd = OpenBatteryUart();
		if(fd > 0)
		{
			LOG_WARN("open bat serial com OK,fd=%d====>\n",fd);
			break;
		}
		if(count >= 10)
		{
			LOG_WARN("open bat serial com fail,return NULL====>\n");
			rdn_set(BATTERY_NODE, "state", "0");
			return NULL;
		}
		sleep(3);
	}
	
	while(1)
	{
		sleep(10);
		ret = GetBatValue(fd, BAT_CELL1_VOL);
		if(ret < 0)
		{
			LOG_WARN("Get bat value fail,return NULL====>\n");
			rdn_set(BATTERY_NODE, "state", "0");
			close(fd);
			return NULL;
		}

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = ret;
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v1", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CELL2_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v2", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CELL3_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v3", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CELL4_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v4", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CELL5_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v5", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CELL6_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v6", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CELL7_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "v7", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_TOTAL_VOL);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "vt", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_EXTER_TEMP1) - 273;
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "bat_t1", battery_buf);
        
        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_EXTER_TEMP2) - 273;
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "bat_t2", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_IC_TEMP1) - 273;
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "ic_t1", battery_buf);
        
        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_IC_TEMP2) - 273;
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "ic_t2", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CUR_CADC);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "current", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_FULL_CAP);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "full_cap", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_REM_CAP);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "rem_cap", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_RSOC_CAP);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "percent_cap", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_CYCLE_COUNT);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "cycle_count", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_PACK_STATUS);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "sys_state", battery_buf);
        
        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_PROTECT_STATUS);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "protect_state", battery_buf);

        memset(battery_buf, 0, sizeof(battery_buf));
        batvalue_buf = GetBatValue(fd, BAT_PACK_CONFIG);
        snprintf(battery_buf, sizeof(battery_buf), "%d", batvalue_buf);
        rdn_set(BATTERY_NODE, "sys_config", battery_buf);

		GetBatValue(fd, BAT_MAKE_INFO);
	}

	close(fd);
	return NULL;
}
