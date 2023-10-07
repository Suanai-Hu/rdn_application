#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"
#include "rdn_mq.h"

//cpu高温、cpu占用率过高报警
void CheckCpuError(void)
{
	char temp[10] = {0};
	char usage[10] = {0};
	char buf[8] = {0};

	rdn_get(ROBOT_NODE, "cpu_temp1", temp, sizeof(temp));
	rdn_get(ROBOT_NODE, "cpu_usage", usage, sizeof(usage));

	if(atoi(temp) >= 100)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", SYS_OVER_TEMP);
		rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_SYS_ERROR, buf);
	}

	else if(atoi(usage) >= 90)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", SYS_OVER_USAGE);
		rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_SYS_ERROR, buf);
	}
	
	return;
}

//电池电量过低报警
void CheckBatteryError(void)
{
	char buf[8] = {0};
	char percent[10] = {0};

	rdn_get(BATTERY_NODE, "percent_cap", percent, sizeof(percent));

	if(atoi(percent) <= 20)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", BAT_LOWER_ERR);
		rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_SYS_ERROR, buf);
	}

	//通讯异常
	
	return;
}

//面板温度过高/面板温度过低 报警
void CheckPanelTempError(void)
{
	char buf[8] = {0};
	char temp[10] = {0};

	rdn_get(SENSOR_TEMP_NODE, "value", temp, sizeof(temp));

	if(atoi(temp) <= 0)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", PANEL_TEMP_LOW);
		rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_SYS_ERROR, buf);
	}
	else if (atoi(temp) >= 50)
	{
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", PANEL_TEMP_HIGH);
		rdn_mq_send(MSG_QUEUE_KEY, MSG_TYPE_SYS_ERROR, buf);
	}

	//温度传感器异常

	return;
}

//程序挂掉 报警
void CheckProcessHangs(void)
{
	LOG_WARN("[%s:%d]:Enter======>\n",__FUNCTION__,__LINE__);
}

void CheckErrorInfo(void)
{
	CheckCpuError();
	CheckBatteryError();
	CheckPanelTempError();
	CheckProcessHangs();
}