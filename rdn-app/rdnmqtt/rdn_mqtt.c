#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h> 
#include <json-c/json.h>
#include "rdn_api.h"
#include "debug.h"
#include "rdn_mqtt.h"

char host[32] = "";
char port[16] = "";
char account[32] = "";
char passwd[32] = "";
char keepalive[8] = "";
char qos[4] = "";
char sn[32] = "";

void InitMqttPara(void)
{
	rdn_get(MQTT_NODE, "host", host, sizeof(host));
	rdn_get(MQTT_NODE, "port", port, sizeof(port));
	rdn_get(MQTT_NODE, "account", account, sizeof(account));
	rdn_get(MQTT_NODE, "password", passwd, sizeof(passwd));	
	rdn_get(MQTT_NODE, "keepalive", keepalive, sizeof(keepalive));
	rdn_get(MQTT_NODE, "qos", qos, sizeof(qos));
	rdn_get(ROBOT_NODE, "sn", sn, sizeof(sn));
    printf("%s, %s, %s, %s, %s, %s, %s\n", host, port, account, passwd, keepalive, qos, sn);
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    char topic[64] = "0";
	
    printf("Connected to MQTT server with code %d.\n", rc);
 
    memset(topic, 0, sizeof(topic));
    snprintf(topic, sizeof(topic), "ATS21/%s", sn);

    printf("topic1 = %s\n", topic);
    if (rc == 0) 
	{
        mosquitto_subscribe(mosq, NULL, topic, atoi(qos));
    }
	return;
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("Disconnected from MQTT server with code %d.\n", rc);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("Received message on topic %s: %s\n", msg->topic, (char *)msg->payload);
	RdnHandleMsg(mosq, (char *)msg->payload);
}

int main(int argc, char **argv)
{
	char topic[64] = "0";
    struct mosquitto *mosq = NULL;

    mosquitto_lib_init();
	InitMqttPara();

    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Create mosquitto failed.\n");
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_username_pw_set(mosq, account, passwd); 
    mosquitto_connect(mosq, host, atoi(port), atoi(keepalive));

    memset(topic, 0, sizeof(topic));
    snprintf(topic, sizeof(topic), "ATS21/%s", sn);
    printf("topic2 = %s\n", topic);

    mosquitto_subscribe(mosq, NULL, topic, atoi(qos));
    mosquitto_loop_forever(mosq, -1, 1);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
