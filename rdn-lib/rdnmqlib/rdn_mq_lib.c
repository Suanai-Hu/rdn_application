#include "rdn_mq.h"

/// @brief 创建消息队列
/// @param mq_key 消息队列的KEY值，自定义
/// @return 成功则返回消息队列ID， 失败则返回-1
int rdn_mq_create(key_t mq_key)
{
	int mq_id = -1;

	LOG_DBG("Enter====>\n");
	mq_id = msgget(mq_key, 0666 | IPC_CREAT);
	if(mq_id == -1)
	{		
		LOG_WARN("msgget failed ====>\n");
	}

	return mq_id;
}

/// @brief 销毁消息队列
/// @param mq_key 消息队列的KEY值，自定义
/// @return 预留
int rdn_mq_destroy(key_t mq_key)
{
	return 0;
}

/// @brief 判断消息队列是否为空
/// @param mq_id 消息队列的ID
/// @return 非空则返回消息的个数，消息为空则返回0
int rdn_mq_empty(int mq_id)
{
	int msg_qnum = 0;
	struct msqid_ds msg_queue_attr;

	LOG_WARN("Enter====>\n");
	//建立消息队列	

	memset(&msg_queue_attr, 0, sizeof(msg_queue_attr));
	if(0 == msgctl(mq_id, IPC_STAT, &msg_queue_attr))
	{
		msg_qnum = msg_queue_attr.msg_qnum;
	}

	return msg_qnum;
}

/// @brief 向消息队列中添加消息
/// @param msg_key 消息队列的key值
/// @param msg_type 消息类型，大于0
/// @param msg_type 消息内容，如无可填NULL，发送和接收方协商好数据结构
/// @return 发送成功返回0
int rdn_mq_send(key_t msg_key, int msg_type, char* msg)
{
	int mq_id = -1;
	msg_data_t msg_data;

	mq_id = rdn_mq_create(msg_key);
	if(mq_id == -1)
	{
		return -1;
	}

	memset(&msg_data, 0, sizeof(msg_data_t));
	msg_data.msg_type = msg_type;
	if(msg)
	{
		strncpy(msg_data.msg_text, msg, sizeof(msg_data.msg_text)-1);
	}

	if(msgsnd(mq_id, (void*)&msg_data, sizeof(msg_data_t), 0) == -1) 
	{	   
		LOG_ERROR("msgsnd failed====>\n");    
		return -1;	  
	}

	LOG_WARN("Send msg type [%d] to queue====>\n", msg_type); 
	return 0;
}

/// @brief 从消息队列中读取消息
/// @param mq_id 消息队列的ID
/// @param msg_type 消息类型，大于0
/// @param msg_data 读取到的消息的内容
/// @return 返回读取的字节数
int rdn_mq_read(int mq_id, int msg_type, msg_data_t* msg_data)
{
	return msgrcv(mq_id, (void*)msg_data, sizeof(msg_data_t), msg_type, IPC_NOWAIT); 
}
 
