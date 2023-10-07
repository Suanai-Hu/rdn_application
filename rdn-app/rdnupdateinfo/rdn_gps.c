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

pthread_t rebot_gps_th;

// 从EC20获取GPS定位数据
void UpdateGPS(void)
{
	int fd = -1;
	int ret = 0;
	char buf[1024] = {0};
	char cmd1[32] = "AT+QGPS=1\r\n";
	char cmd2[32] = "AT+QGPSLOC=2\r\n";
	char cmd3[32] = "AT+QGPSEND\r\n";
	char gpsdata[32] = {0};
	char * response = NULL;
	char * latitude = NULL;
	char * longitude = NULL;
    char * token = NULL;

    // 打开GPS设备的串口设备文件
    fd = open(GPS_DEVICE, O_RDWR | O_NOCTTY, 0777);
    if(fd < 0)
    {
        return;
    }

	ret = InitSerial(fd, 115200, 0, 8, 1, 'N');
	if(ret < 0)
	{
		LOG_LOOP("Init GPS device failed====>\n");
		close(fd);
		return;
	}

	write(fd, cmd1, strlen(cmd1));
	sleep(5);
	write(fd, cmd2, strlen(cmd2));
	
	tcflush(fd, TCIOFLUSH);

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	struct timeval tv = {5, 0};
	ret = select(fd + 1, &rfds, NULL, NULL, &tv);
	if(ret < 0)
	{
		LOG_LOOP("Failed to wait for GPS data====>\n");
		close(fd);
		return;
	}
	else if(ret == 0)
	{
		LOG_LOOP("Timeout waiting for GPS data====>\n");
		close(fd);
		return;
	}

	ret = read(fd, buf, sizeof(buf) - 1);

	if(ret > 0)
	{
		buf[ret] = '\0';
		response = buf;
		LOG_LOOP("response = %s\n", response);
	}
	else
	{
		LOG_LOOP("Failed to read====>\n");
		write(fd, cmd3, strlen(cmd3));
		close(fd);
		return;
	}

	if(strstr(response, "+QGPSLOC"))
	{
		token = strtok(response, ",");
		for(int i = 0; i < 12; i++)
		{
			token = strtok(NULL, ",");
			if(i == 0)
			{
				latitude = token;			// 经度
			}
			else if(i == 1)
			{
				longitude = token;			// 纬度
			}
		}

		LOG_LOOP("latitude = %s, longitude = %s \n", latitude, longitude);
		snprintf(gpsdata, sizeof(gpsdata), "%s,%s", latitude, longitude);
		rdn_set(WL_G4_NODE, "GPS", gpsdata);
	}

    close(fd);
    return;
}

void* UpdateRebotGPSThread(void* arg)
{
	LOG_WARN("Enter====>\n");
	sleep(30);
	while(1)
	{
		UpdateGPS();
		sleep(20);
	}

	return NULL;
}
