#ifndef _RDN_INFO_H_
#define _RDN_INFO_H_

#include <pthread.h>
#include <sys/ioctl.h>
 
typedef	unsigned short	uint16;
typedef	unsigned int	uint32;
typedef unsigned char	uint8;
 
#define VENDOR_REQ_TAG      0x56524551
#define VENDOR_READ_IO      _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO     _IOW('v', 0x02, unsigned int)

#define VENDOR_LENTH 32

#define GPS_DEVICE "/dev/ttyUSB2"   // GPS设备的串口设备名称
#define GPS_BAUDRATE B9600       	// GPS设备的波特率


enum VENDOR_ID 
{
	VENDOR_SN_ID 					= 1,
	VENDOR_WIFI_MAC_ID 				= 2,
	VENDOR_LAN_MAC_ID 				= 3,
	VENDOR_BT_MAC_ID 				= 4,
	VENDOR_HDCP_14_HDMI_ID 			= 5,
	VENDOR_HDCP_14_DP_ID 			= 6,
	VENDOR_HDCP_2x_ID 				= 7,
	VENDOR_DRM_KEY_ID 				= 8,
	VENDOR_PLAYREADY_Cert_ID 		= 9,
	VENDOR_ATTENTION_KEY_ID			= 10,
	VENDOR_PLAYREADY_ROOT_KEY_0_ID	= 11,
	VENDOR_PLAYREADY_ROOT_KEY_1_ID	= 12,
	VENDOR_SENSOR_CALIBRATION_ID	= 13,
	VENODR_RESERVE_ID_14			= 14,
	VENDOR_IMEI_ID					= 15,
	VENDOR_CUSTOM_ID_10				= 16,
	VENDOR_CUSTOM_ID_11				= 17,
	VENDOR_CUSTOM_ID_12				= 18,
	VENDOR_CUSTOM_ID_13				= 19,
	VENDOR_CUSTOM_ID_14				= 20,
	VENDOR_CUSTOM_ID_15				= 21,
	VENDOR_CUSTOM_ID_16				= 22,
	VENDOR_CUSTOM_ID_17				= 23,
	VENDOR_CUSTOM_ID_18				= 24,
	VENDOR_CUSTOM_ID_19				= 25,
	VENDOR_CUSTOM_ID_1A				= 26,
	VENDOR_CUSTOM_ID_1B				= 27,
	VENDOR_CUSTOM_ID_1C				= 28,
	VENDOR_CUSTOM_ID_1D				= 29,
	VENDOR_CUSTOM_ID_1E				= 30,
	//VENDOR_CUSTOM_ID_XX			= xx
	VENDOR_CUSTOM_ID_FF				= 255,
};
 
struct rk_vendor_req {
    uint32 tag;
    uint16 id;
    uint16 len;
    uint8 data[1];
};

typedef enum{
	BAT_CMD_NONE=0,
	BAT_CELL1_VOL=1,
	BAT_CELL2_VOL=2,
	BAT_CELL3_VOL=3,
	BAT_CELL4_VOL=4,
	BAT_CELL5_VOL=5,
	BAT_CELL6_VOL=6,
	BAT_CELL7_VOL=7,
	BAT_CELL8_VOL=8,
	BAT_CELL9_VOL=9,
	BAT_CELL10_VOL=10,
	BAT_TOTAL_VOL=11,
	BAT_EXTER_TEMP1=12,
	BAT_EXTER_TEMP2=13,
	BAT_IC_TEMP1=14,
	BAT_IC_TEMP2=15,
	BAT_CUR_CADC=16,
	BAT_FULL_CAP=17,
	BAT_REM_CAP=18,
	BAT_RSOC_CAP=19,
	BAT_CYCLE_COUNT=20,
	BAT_PACK_STATUS=21,
	BAT_PROTECT_STATUS=22,
	BAT_PACK_CONFIG=23,
	BAT_MAKE_INFO=24
}BatteryDataType;

enum timeNum
{
	YEAR=0,
	MONTH=1,
	DAY=2,
	HOUR=3,
	MINUTE=4,
	SECOND=5
};

typedef struct{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}MyTimeType;

#define UPDATE_CYCLE_5S			5
#define UPDATE_CYCLE_10S		10
#define UPDATE_CYCLE_15S		15
#define UPDATE_CYCLE_30S		30
#define UPDATE_CYCLE_45S		45
#define UPDATE_CYCLE_60S		60


#define SEC_PER_YEAR		(365*24*60*60)
#define SEC_PER_MONTH		(30*24*60*60)
#define	SEC_PER_DAY			(24*60*60)
#define SEC_PER_HOUR		(60*60)
#define	SEC_PER_MINUTE		(60)

enum SysErrorCode
{
    SYS_NONE_ERR  		=0,
    BAT_COMM_ERR  		=101,
    CAM_NO_CALIBRATE 	=102,
    SYS_OVER_TEMP  		=103,
    LEFT_CAM_NO_DEV  	=104,
    RIGHT_CAM_NO_DEV 	=105,
    LEFT_CAM_IMAGE_ERR 	=106,
    RIGHT_CAM_IMAGE_ERR =107,
    LEFT_CAM_OFFLINE 	=108,
    RIGHT_CAM_OFFLINE 	=109,
    LEFT_CAM_OPEN_ERR 	=110,
    RIGHT_CAM_OPEN_ERR 	=111,
    BAT_LOWER_ERR  		=112,
    TEMP_SENSOR_ERR  	=113,
    PANEL_TEMP_LOW  	=114,
    PANEL_TEMP_HIGH  	=115,
    SYS_OVER_USAGE  	=116,
    RDN21_APP_HANGS  	=117
};

typedef struct 
{
    void (*fun)();
    int period;
    int cnt;
}running_queue_def;

#define FALSE  -1
#define TRUE   0

#define	BUF_LEN_64 			64
#define	BUF_LEN_128 		128

#define MAX_TASK_NUM		10
#define FILE_SYS_VER			"/etc/sys_version"

#define DEFAULT_ROBOT_SN	"RDN21-0000"
#define DEFAULT_ROBOT_NAME	"RDN21-0000"
#define DEFAULT_AP_NAME		"RDN21-0000"

#define FILE_CPU0_SYS_TEMP		"/sys/class/thermal/thermal_zone0/temp"
#define FILE_CPU1_SYS_TEMP		"/sys/class/thermal/thermal_zone1/temp"
#define FILE_CPU0_TEMP			"/tmp/cpu0Temp"
#define FILE_CPU1_TEMP			"/tmp/cpu1Temp"
#define FILE_CPU_USAGE_RATE		"/tmp/cpuUsageRate"
#define UART_BAT_DEV_PATH		"/dev/ttyS7"
#define FILE_SYS_TIME			"/proc/uptime"

#define I2C_DEFAULT_TIMEOUT     10000  /* jiffies */
#define I2C_DEFAULT_RETRY       30
#define I2C_TEMP_SLAVE_ADR		0x5a
#define I2C_TEMP_RAM_ACCESS		0x00 	/*RAM access command */
#define I2C_TEMP_REG_AMBIENT   	0x06 	
#define I2C_TEMP_REG_OBJ1      	0x07
#define I2C_TEMP_REG_OBJ2      	0x08
#define I2C_TEMP_DEV_PATH		"/dev/i2c-4"
#define USE_NEARDI_SYS

extern pthread_t uart_bat_th;
extern pthread_t pannel_temp_th;
extern pthread_t rebot_info_th;
extern pthread_t rebot_gps_th;

int InitSerial(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
int ReadVendor(enum VENDOR_ID vid,char* buf,int len);
int Writevendor(enum VENDOR_ID vid,char* buf,int len);
int CreateTask(const void(*fun)(), int period);
int RunScheduler(void);

void* UpdateRebotInfoThread(void* arg);
void* UpdateBatteryInfoThread(void* arg);
void* UpdatePanelTempThread(void* arg);
void* UpdateRebotGPSThread(void* arg);
void CheckErrorInfo(void);

#endif
