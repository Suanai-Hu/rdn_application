#include <stdio.h>
#include <unistd.h>
#include "rdn_upgrade.h"

/**
 * @brief 主函数 创建各个线程
 */
int main(void)
{
    pthread_create(&local_upgrade_th, NULL, LocalUpgradeThread, NULL);
    pthread_create(&ota_upgrade_th, NULL, OtaUpgradeThread, NULL);
	pthread_create(&ota_download_th, NULL, OtaDownloadVerThread, NULL);
	pthread_create(&ota_unzip_th, NULL, OtaUnzipVerThread, NULL);
	
    while(1)
    {
        sleep(1);
    }
    return 0;
}


