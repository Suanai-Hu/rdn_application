#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "rdn_info.h"
#include "rdn_api.h"
#include "debug.h"
#include "node.h"

pthread_t pannel_temp_th;

/**
 * @brief 打开I2C设备
 * @param devicepath	i2c设备地址
 * @return 成功打开的i2c设备描述符
 * @retval 大于0表示成功，-1表示失败
*/
int I2cOpen(char* devicepath)
{
	int	fd = 0;

	if(devicepath == NULL || strlen(devicepath) == 0)
	{
		return -1;
	}
	fd= open(devicepath, O_RDWR);
	if(fd < 0 )
	{
		LOG_ERROR("open [%s] faild====>\n", devicepath);
		return -1;
	}	
	
	return fd;
}

/**
 * @brief 初始化i2c设备
 * @param fd i2c设备描述符
 * @param slaveaddr	从设备地址
 * @return 是否初始化成功
 * @retval 0表示成功，-1表示失败
*/
int I2cInit(int fd, int slaveaddr)
{
	if(fd < 0)
	{
		LOG_ERROR("fd is wrong====>\n");
		return -1;
	}
	
	if(ioctl(fd, I2C_SLAVE, slaveaddr) < 0)
	{
		LOG_ERROR("set I2C_SLAVE faild====>\n");
		return -1;
	}
	
    if (ioctl(fd, I2C_TIMEOUT, I2C_DEFAULT_TIMEOUT) < 0)
    {
    	LOG_ERROR("set I2C_TIMEOUT faild====>\n");
		return -1;		
	}
    if (ioctl(fd, I2C_RETRIES, I2C_DEFAULT_RETRY) < 0)
    {
    	LOG_ERROR("set I2C_RETRIES faild====>\n");
		return -1;
	}
	
	return 0;
}

/**
 * @brief 初始化i2c设备
 * @param fd   : File descriptor
 * @param addr : I2C device addr
 * @param reg  : Register address
 * @param val  : Save the data you will read
 * @param len  : Bytes number you will read
 * @return 是否初始化成功
 * @retval 0表示成功，-1表示失败
*/
int I2cReadBytes(int fd, unsigned char addr, unsigned char reg, unsigned char *value, int len)
{
	int ret = 0;
	unsigned char outbuf;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg msg[2];

	memset(msg, 0, sizeof(msg));

	/*Step1:Write the register addr that you will read */
	msg[0].addr = addr;			
	msg[0].flags = 0;  		/*write flag */
	msg[0].len = 1;        
	msg[0].buf = &outbuf; 		
	outbuf = reg;

	/*Step2:read the register data */
	msg[1].len = len;         	
	msg[1].addr = addr;       	
	msg[1].flags = I2C_M_RD;   /*Read flag */
	msg[1].buf = value;			

	packets.msgs = msg;
	packets.nmsgs = 2;           

	/*Step3: send i2c packets */ 
	ret = ioctl(fd, I2C_RDWR, (unsigned long)&packets); 

	return ret;
}

/*
** RN6854M处理AHD摄像头传输的数据
*/

void* HandleCamData(void* arg)
{
	int ret = 0;
	int fd = 0;

	LOG_WARN("Enter=====================>\n");
	
	fd = I2cOpen();
}

/*
void* UpdatePanelTempThread(void* arg)
{
	int ret = 0;
	int	fd = 0;
    int pannel_temperature = 0;
    char pannel_temp_buf[8] = {0};
	unsigned char value[3] = {0};
    float obj1_T = 0;

    LOG_WARN("Enter====>\n");
	
	fd = I2cOpen(I2C_TEMP_DEV_PATH);
	if(fd < 0)
	{
		LOG_ERROR("Open %s fail====>\n", I2C_TEMP_DEV_PATH);
		return NULL;
	}
	
	ret = I2cInit(fd, I2C_TEMP_SLAVE_ADR);
	if(ret < 0)
	{
		LOG_ERROR("Init i2c fail====>\n");
		return NULL;
	}
	
    while(1)
    {
        ret = I2cReadBytes(fd, I2C_TEMP_SLAVE_ADR, I2C_TEMP_REG_OBJ1, value, 3);
        if(ret > 0)
        {
            obj1_T = ((value[1]<<8) | (value[0])) * 0.02 - 273.15;
        }
        pannel_temperature = (int)obj1_T;
        memset(pannel_temp_buf, 0, sizeof(pannel_temp_buf));
        snprintf(pannel_temp_buf, sizeof(pannel_temp_buf), "%d", pannel_temperature);
        rdn_set(SENSOR_TEMP_NODE, "value", pannel_temp_buf);
        LOG_LOOP("temp=[%d]====>\n",pannel_temperature);
        sleep(3);
    }
	close(fd);
	return NULL;
}
*/
