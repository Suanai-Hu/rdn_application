#ifndef __RDN_MQ_H__
#define __RDN_MQ_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "debug.h"

#define MSG_QUEUE_KEY   		5858
#define MSG_TYPE_MOVEMENT   	1
#define MSG_TYPE_REMOTE_PAIR  	2
#define MSG_TYPE_CALIBRATE   	3 
#define MSG_TYPE_CHARGING   	4
#define MSG_TYPE_UPGRADE_MCU  	5
#define MSG_TYPE_SYS_ERROR      6

typedef struct msg_data_s{
 long msg_type;			/* 消息类型，必须 > 0 */
 char msg_text[1024]; 	/* 消息文本，可以是其他类型 */
}msg_data_t; 

int rdn_mq_create(key_t mq_key);
int rdn_mq_destroy(key_t mq_key);
int rdn_mq_empty(int mq_id);
int rdn_mq_send(key_t msg_key, int msg_type, char* msg);
int rdn_mq_read(int mq_id, int msg_type, msg_data_t* data);

#endif


