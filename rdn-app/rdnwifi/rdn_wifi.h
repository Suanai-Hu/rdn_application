/**
 * @file wificonnect.h
 * @author Lei.Zhang
 * @email zhanglei@radiantpv.com
 * @version V0.0.0
 * @date 2022-3-9
 * @license 2014-2021,Radiant Solar Technology Co., Ltd.
**/ 
#ifndef _WIFICONNECT_H_
#define _WIFICONNECT_H_

#define SHM_ID_WIFI				1234
#define MAX_WIFI_CACHE_NUM		5
#define	BUF_LEN_64 				64
#define	BUF_LEN_128 			128
#define BUF_LEN_256				256
#define MAX_SSID_NUM			32
#define MAX_SSID_LEN			32
#define MAX_IPV4_LEN			15
#define	MAX_MAC_LEN				17
#define MAX_RETRY_TIMES			5

#define FILE_NETWORK_CFG		"/userdata/cfg/wificache.cfg"
#define FILE_SCAN_LIST			"/tmp/scanlist.txt"	
#define FILE_SSID_LIST			"/tmp/ssidlist.txt"	
#define FILE_WPA_SUPPLI_IF 		"/var/run/wpa_supplicant"
#define FILE_WPA_SUPPLI_CONF	"/etc/wpa_supplicant.conf"
#define FILE_WLAN0_STATUS_1		"/tmp/wlan0State1.txt"
#define FILE_IFCONFIG_WLAN0		"/tmp/wlan0Info"
#define FILE_PING_RESULT		"/tmp/ping.txt"

#define PROCESS_WPA_SUPPLICANT	"wpa_supplicant"
#define CMD_GET_WIFI_LIST		"iwlist wlan0 scan" 

#define STR_WIFI_CACHE_SSID 	"cache_ssid_%d="
#define STR_WIFI_CACHE_PSW 		"cache_psw_%d="
#define STR_lOGIN_SSID			"login_ssid="
#define STR_LOGIN_PSW			"login_psw="
#define STR_LOGED_SSID			"loged_ssid="
#define STR_LOGED_PSW			"loged_psw="

enum wlan0Status
{
	NO_CONNECT=0,
	WPA_OK=1,
	IP_OK=2
};

typedef enum
{
	WIFI_NONE=0,
	WIFI_CHECK=1,
	WIFI_CONNECT=2
}wifiConnAction;

typedef struct {
	char state[32];
	void (*connect)();
	void (*check)();
}WifiConnctState;


#define NONE_STATE				"NONE"
#define INIT_STATE				"INIT"
#define START_STATE				"START"
#define WPA_STATE				"WPA"
#define DHCP_STATE				"DHCP"
#define OK_STATE				"SUCCESS"
#define IP_ERROR_STATE			"IP_ERROR"
#define PSW_ERROR_STATE			"PSW_ERROR"

enum replyResultNum{
	ACK_APP_OK				=0,
	ACK_APP_FAIL_NAME		=1,
	ACK_APP_FAIL_SIZE		=2,
	ACK_APP_FAIL_FORMAT		=3,
	ACK_SYS_OK				=4,
	ACK_SYS_FAIL_NAME		=5,
	ACK_SYS_FAIL_SIZE		=6,
	ACK_SYS_FAIL_FORMAT		=7,
	ACK_MCU_OK				=8,
	ACK_MCU_FAIL_NAME		=9,
	ACK_MCU_FAIL_SIZE		=10,
	ACK_MCU_FAIL_FORMAT		=11,
	ACK_DIA_FAIL_TIMEOUT	=12,
	ACK_SAVE_OK				=13,
	ACK_CAM_OK				=14,
	ACK_CAM_FAIL_TIMEOUT	=15,
	ACK_WIFI_OK				=16,
	ACK_WIFI_FAIL_PSW		=17,
	ACK_WIFI_FAIL_IP		=18,
	ACK_MAP_OK				=19,
	ACK_MAX
};

void SetAckNode(char* acknum);
int IsConnectWifi(const char* wifiname);
int GetPingResult(char* ip);
int GetNetworkIp(char* ip, char* mask, char* network_ip, int size);
int GetIpMask(char ip_addr[], int ip_size, char mask_addr[], int mask_size);
int StartUdhcpc(void);
int CheckWlan0Status(char* filepath);
int SearchSsidFromWifiList(const char* ssid);
int StartWpaSupplicant(const char* ssid, const char* psw);
int GetNeighborWifiInfo(char ssidlist[][MAX_SSID_LEN], int max_ssid_sum);
void SetNeighborWifiNode(void);
int UpdateLoginCache(char* ssid, char* psw, char* ip);


void AutoConnect(void);
void ManualConnect(void);
void CheckAutoConningState(void);
void CheckManualConningState(void);

void CheckIdleState(void);
void SwitchCurrentState(WifiConnctState* state, wifiConnAction action);

#endif // _WIFICONNECT_H_
