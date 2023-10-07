#include <stdio.h>
#include <unistd.h>
#include "rdn_info.h"

/**
 * @brief 主函数 创建各个线程
 */
int main(void)
{
    pthread_create(&pannel_temp_th, NULL, UpdatePanelTempThread, NULL);
    pthread_create(&uart_bat_th, NULL, UpdateBatteryInfoThread, NULL);
    pthread_create(&rebot_info_th, NULL, UpdateRebotInfoThread, NULL);
    pthread_create(&rebot_gps_th, NULL, UpdateRebotGPSThread,NULL);

    while(1)
    {
        sleep(1);
    }
    return 0;
}
