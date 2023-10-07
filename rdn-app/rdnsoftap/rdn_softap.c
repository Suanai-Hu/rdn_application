#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include <stdlib.h>

#define WL_AP_NODE 	"root.wl.ap"

void WriteKeyValueToFile(FILE* fp, char* key)
{
	char value[32] = {0};
	char buf[64] = {0};
	
	memset(value, 0, sizeof(value));
	rdn_get(WL_AP_NODE, key, value, sizeof(value));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%s=%s\n", key, value);
	fwrite(buf, strlen(buf), sizeof(char), fp);
	
	return;
}

int CreateHostapdCfgFile(void)
{
	char buf[64] = {0};
	FILE* fp = NULL;
	char cmd[256] = {0};
	char* conf_file= "/etc/hostapd.conf";
	
	unlink(conf_file);
	fp = fopen(conf_file, "a+");
	if(fp)
    {
    /*
		interface=p2p0
		ctrl_interface=/var/run/hostapd
		driver=nl80211
		ssid=RDN21-LEI
		channel=6
		hw_mode=g
		ieee80211n=1
		ignore_broadcast_ssid=0
		auth_algs=1
		wpa=2
		wpa_passphrase=12345678
		wpa_key_mgmt=WPA-PSK
		wpa_pairwise=TKIP
		rsn_pairwise=CCMP
	*/
     	WriteKeyValueToFile(fp, "interface");
		WriteKeyValueToFile(fp, "ctrl_interface");
		WriteKeyValueToFile(fp, "driver");
		WriteKeyValueToFile(fp, "ssid");
		WriteKeyValueToFile(fp, "channel");
		WriteKeyValueToFile(fp, "hw_mode");
		WriteKeyValueToFile(fp, "ieee80211n");
		WriteKeyValueToFile(fp, "ignore_broadcast_ssid");
		WriteKeyValueToFile(fp, "auth_algs");
		WriteKeyValueToFile(fp, "wpa");
		WriteKeyValueToFile(fp, "wpa_passphrase");
		WriteKeyValueToFile(fp, "wpa_key_mgmt");
		WriteKeyValueToFile(fp, "wpa_pairwise");
		WriteKeyValueToFile(fp, "rsn_pairwise");
		fclose(fp);	
    }

    return 0;
}

void StartWlanAccesspoint(void)
{
	char cmd[128] = {0};
	char ip[16] = {0};

	system("killall -9 dnsmasq");
	system("killall -9 hostapd");
	system("ifconfig p2p0 down");  

	rdn_set(WL_AP_NODE, "state", "off");
	rdn_get(WL_AP_NODE, "ip", ip, sizeof(ip));

	system("ifconfig p2p0 up");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "ifconfig p2p0 %s netmask 255.255.255.0", ip);
	system(cmd);

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "route add default gw %s p2p0", ip);
	system(cmd);

	system("/usr/sbin/dnsmasq &");

	CreateHostapdCfgFile();
	system("hostapd /etc/hostapd.conf &");
	rdn_set(WL_AP_NODE, "state", "on");

	return;
}

int main(int argc, char *argv[])
{	
	StartWlanAccesspoint();
	return 0;
}



