#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_pppox.h>
#include <linux/sockios.h>
#include "rdn_api.h"
#include "node.h"

int main(void)
{
    int fd;
	char enable[4] = {0};
	struct stat buffer;
    struct ifreq ifr;
    struct rtentry rt;
	struct sockaddr_in *sin;

    while(1) 
    {
        sleep(3);
    	rdn_get(ENABLE_NODE, "G4", enable, sizeof(enable));
        if(atoi(enable) == 0)
        {
			continue;
		}
        // 如果两个文件都存在，则退出循环并发送pppd命令
        if (access("/dev/ttyUSB0", F_OK) != -1 && stat("/etc/ppp/peers/quectel-ppp", &buffer) == 0) 
        {
            LOG_DBG("Both files exist. Exiting loop.\n");
            system("pppd call quectel-ppp &");
            break;
        }
		sleep(5);
    }

    while(1)
    {
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, "ppp0", IFNAMSIZ-1);
        fd = socket(AF_INET, SOCK_DGRAM, 0);

        // 查看socket是否正常
        if (fd < 0)
        {
            LOG_DBG("socket error! \n");
            return -1;
        }

        // 检查ppp0是否拿到IP
        if(ioctl(fd, SIOCGIFADDR, &ifr) == 0)
        {
            LOG_DBG("get ip success\n");
            close(fd);

            system("route add default dev ppp0");

			// 显示获取到的IP地址
			sin = (struct sockaddr_in *)&ifr.ifr_addr;
			LOG_DBG("ppp0 IP address is %s\n", inet_ntoa(sin->sin_addr));
            break;
        }

        close(fd);
    }

    return 0;
}
