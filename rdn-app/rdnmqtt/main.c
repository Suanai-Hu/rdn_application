#include <stdio.h>
#include <unistd.h>
#include "rdn_mqtt.h"

/**
 * @brief 主函数 创建各个线程
 */
int main(void)
{
    pthread_create(&rdn_mqtt_sub_th, NULL, RdnMqttSubClient, NULL);
    pthread_create(&rdn_mqtt_pub_th, NULL, RdnMqttPubClient, NULL);
	
    while(1)
    {
        sleep(1);
    }
    return 0;
}



