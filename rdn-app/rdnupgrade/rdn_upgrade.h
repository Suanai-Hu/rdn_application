#ifndef __RDN_UPGRADE_H__
#define __RDN_UPGRADE_H__

#include <pthread.h>

extern pthread_t local_upgrade_th;
extern pthread_t ota_upgrade_th;
extern pthread_t ota_download_th;
extern pthread_t ota_unzip_th;

#define MSG_QUEUE_KEY		5858
#define MSG_TYPE_UPGRADE		1
#define MSG_TYPE_SET			2
#define MSG_TYPE_REQ			3
#define MSG_TYPE_SAVE			4
#define MSG_TYPE_UPGRADE_MCU	5


typedef struct msg_data_s{
	long msg_type; 		/* 消息类型，必须 > 0 */
	char msg_text[1024]; 	/* 消息文本，可以是其他类型 */
}msg_data_t;

typedef struct upgrade_s{
	char cmd[32];
	void (*handle_fun)(void);
}upgrade_t;

typedef struct ota_upgrade_s
{
	char ota_cmd[16];
	char ver_type[16];
	char ver_name[64];
	int ver_size;
	int download_percent;
	int unzip_ok;
}ota_upgrade_t;

#define	BUF_LEN_64 			64
#define	BUF_LEN_128 		128
#define BUF_LEN_256			256
#define BUF_LEN_512			512

#define LINUX_TAR_NAME			"linux.tar.gz"
#define LINUX_BIN_NAME			"linux.img"
#define APP_BIN_NAME			"rdn-rk3399"
#define MCU_BIN_NAME			"mcu.bin"
#define COMMON_CONFIG_FILE		"configure.yml"
#define BACKUP_FILE				"userdata.tar.gz"
#define LEFTCAM_CONFIG_FILE		"leftcamera.yml"
#define RIGHTCAM_CONFIG_FILE	"rightcamera.yml"
#define LEN_ACTIVE_CODE			17

#define FILE_PING_RESULT			"/tmp/ping.txt"
#define FILE_CURL_LOG				"/tmp/curl.log"
#define FILE_VERSION_PATH			"/userdata/ota/%s"
#define FILE_GET_LATEST_VER_PHP		"/tmp/get_latest_version.php"
#define FILE_LATEST_VER				"/tmp/latest_version.txt"

#define CMD_GET_LATEST_VER_PHP		"curl -C - http://117.80.229.23:4234/Robot/%s/get_latest_version.php -o /tmp/get_latest_version.php"
#define CMD_GET_LATEST_VER_NO		"curl -C - http://117.80.229.23:4234/Robot/%s/latest_version.txt -o /tmp/latest_version.txt"
#define CMD_GET_LATEST_VER 			"curl -C - http://117.80.229.23:4234/Robot/%s/%s -o /userdata/ota/%s 2>&1 &> /tmp/curl.log  &"
#define CMD_UNZIP_VER				"tar -zxvf /userdata/ota/%s -C /userdata/ota/"

typedef struct otaState{
  void (*cancel)();
  void (*confirm)();
  void (*check)();
}otaState;

typedef struct otaTestCmd{
	char ota_action[16];
	char ota_type[16];
}OtaTestCmd;

void OtaInit(void);
void OtaCheckLatestVer(void);
void OtaDownLoadVer(void);
void OtaCheckDownLoadProcess(void);
void OtaUnzipVer(void);
void OtaCheckUnzipProcess(void);
void OtaUpdateVer(void);
void OtaCancelDownLoad(void);
void OtaCancelUnzip(void);
void OtaCancelUpdate(void);
void DoOtaCancel(void);
void DoOtaConfirm(void);
void DoOtaCheck(void);
void DoOtaUpdateSystem(void);
void DoOtaUpdateMcu(void);
void DoOtaUpdateApp(void);
void CheckOtaState(void);

void* OtaUpgradeThread(void* p);
void* LocalUpgradeThread(void* p);
void* OtaDownloadVerThread(void* P);
void* OtaUnzipVerThread(void* P);



int IsCanAccessInternet(void);
int GetLatestVerInfo(char* ver_name, int len, const char* ver_type);
void SetOtaUpdateInfo(char* ver_name, int ver_size, const char* ver_type, const char* ota_cmd);
void SetAckNode(char* acknum);
int CheckFileSize(char* filename, int rcvsize);

#endif
