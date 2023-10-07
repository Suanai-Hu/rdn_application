#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include "../include/rdn_api.h"

/// @brief 分割节点函数
/// @param node_path 节点路径如root.map.map0.rect0
/// @param node_name 储存分割后的节点路径
/// @return	返回分割后的数量
static int SplitNodePath(char* node_path, char node_name[][32])
{    
	int i = 0;
	char* s = strtok(node_path, ".");
	while (s != NULL)
	{	
		strncpy(node_name[i], s, 31);
		s = strtok(NULL, ".");
		i++;
	}

	return i;
}

/// @brief 获取节点属性值
/// @param shmid 节点第一个属性的共享内存ID
/// @param attr 属性名
/// @param value 存储属性值
/// @param len 存储属性值的数组长度
/// @return 成功则返回属性值的字节个数，失败返回-1
static int GetNodeAttr(int shmid,char* attr, char* value, int len)
{
	attrNode* node = NULL;
	
	if(attr == NULL || value == NULL)
	{
		printf("attr/value is NULL!!!\n");
		return -1;
	}
	
	while(shmid)
	{
		node = (attrNode*)shmat(shmid, NULL, 0);
		if(0 == strcmp(node->name, attr))
		{
			strncpy(value, node->value, len-1);
			shmdt(node);
			return strlen(value);
		}
		shmid = node->next;
		shmdt(node);
	}

	printf("Attribute:[%s] is not exist!!!\n", attr);
	return -1;
}

/// @brief 添加新的属性到共享内存中
/// @param attr_shmid 节点最后一个属性的共享内存ID
/// @param attr_name 属性名称
/// @param value 属性值
/// @return 返回新的属性的共享内存ID
static int CreateAttrShmNode(int attr_shmid, char* attr_name, char* value)
{
	int shmid = 0;
	attrNode* node = NULL;
	attrNode* attr_node = NULL;
	
	shmid = shmget(0, sizeof(attrNode), IPC_CREAT|0666);
	if(shmid <= 0)
	{
		printf("Create shmid fail====>");
		return -1;
	}
	node = (attrNode*)shmat(shmid, NULL, 0);
	strncpy(node->name, attr_name, sizeof(node->name)-1);
	strncpy(node->value, value, sizeof(node->value)-1);
	node->next = 0;
	node->prev = attr_shmid;

	if(attr_shmid)
	{
		attr_node = (attrNode*)shmat(attr_shmid, NULL, 0);
		attr_node->next = shmid;
		shmdt(attr_node);
	}
	
	shmdt(node);
	
	return shmid;
}

/// @brief 设置节点属性
/// @param shmid 节点第一个属性的共享内存ID
/// @param attr 属性名称
/// @param value 属性值
/// @return 成功返回0，失败返回-1
static int SetNodeAttr(int shmid, char* attr, char* value)
{
	attrNode* node = NULL;
	int last_attr_shmid = 0;

	if(attr == NULL || value == NULL)
	{
		return -1;
	}

	while(shmid)
	{
		node = (attrNode*)shmat(shmid, NULL, 0);
		if(0 == strcmp(node->name, attr))
		{
			memset(node->value, 0, sizeof(node->value));
			strncpy(node->value, value, sizeof(node->value)-1);
			shmdt(node);
			return 0;
		}
		last_attr_shmid = shmid;
		shmid = node->next;
		shmdt(node);
	}

	//CreateAttrShmNode(last_attr_shmid, attr, value);
	printf("Attribute:[%s] is not exist,plaease config it in xml first!!!\n", attr);
	return -1;
	
}

static void PrintAttrInfo(int attr_shmid, char* format)
{
	attrNode* attr_node = NULL;
	while(attr_shmid)
	{
		attr_node = (attrNode*)shmat(attr_shmid, NULL, 0);
		printf("%s%s = %s\n", format, attr_node->name, attr_node->value);
		attr_shmid = attr_node->next;
		shmdt(attr_node);
	}
	
	return;
}

/// @brief 查询节点
/// @param path 节点路径如root.map.map0.rect0
/// @return 成功返回查询到的节点的共享内存ID，失败返回0
int rdn_query(char* path)
{
	cfgNode* init_node = NULL;
	cfgNode* node = NULL;
	cfgNode* child_node = NULL;
	cfgNode* grandson_node = NULL;
	int shmid = 0;
	int child_shmid = 0;
	int grandson_shmid = 0;
	char name[64] = {0};
	char node_name[4][32] = {0};

	if(path == NULL || path[0] == '\0')
	{
		printf("path is NULL!!!\n");
		return 0;
	}
	
	strncpy(name, path, sizeof(name)-1);
	int len = SplitNodePath(name, node_name);

	int init_shmid = shmget(KEY, sizeof(cfgNode), IPC_CREAT|0666);
	init_node = (cfgNode*)shmat(init_shmid, NULL, 0);
	shmid = init_node->next;
	shmdt(init_node);
	
	if(0 == strcmp(path, "root"))
	{
		return shmid;
	}
	
	while(shmid)
	{
		node = (cfgNode*)shmat(shmid, NULL, 0);
		if(0 == strcmp(node->name, node_name[1]))
		{
			if(len == 2)
			{
				shmdt(node);
				return shmid;
			}
			else if(len >= 3)
			{
				child_shmid = node->children;
				while(child_shmid)
				{
					child_node = (cfgNode*)shmat(child_shmid, NULL, 0);
					if(0 == strcmp(child_node->name, node_name[2]))
					{
						if(len == 3)
						{
							shmdt(child_node);
							shmdt(node);
							return child_shmid;
						}
						else if(len == 4)
						{
							grandson_shmid = child_node->children;
							while(grandson_shmid)
							{
								grandson_node = (cfgNode*)shmat(grandson_shmid, NULL, 0);
								if(0 == strcmp(grandson_node->name, node_name[3]))
								{
									shmdt(grandson_node);
									shmdt(child_node);
									shmdt(node);
									return grandson_shmid;
								}
								grandson_shmid = grandson_node->next;
								shmdt(grandson_node);
							}
						}
					}
					child_shmid = child_node->next;
					shmdt(child_node);
				}
			}
		}
		shmid = node->next;
		shmdt(node);
	}

	return 0;	
}

/// @brief 打印节点下的所有属性信息，包括子节点的属性
/// @param path 
void rdn_show(char* path)
{
	int attr_shmid = 0;
	int cfg_shmid = 0;
	int cfg_son_shmid = 0;
	int cfg_grandson_shmid = 0;
	
	attrNode* attr_node = NULL;
	cfgNode* cfg_node = NULL;
	cfgNode* cfg_son_node = NULL;
	cfgNode* cfg_grandson_node = NULL;

	if(path == NULL || path[0] == '\0')
	{
		printf("path is NULL!!!\n");
		return;
	}
	
	cfg_shmid = rdn_query(path);
	if(cfg_shmid == 0)
	{
		printf("[%s] is not exist!!!\n", path);
		return;
	}

	while(cfg_shmid)
	{
		cfg_node = (cfgNode*)shmat(cfg_shmid, NULL, 0);
		printf("[%s:]\n", cfg_node->name);
		attr_shmid = cfg_node->attr;
		PrintAttrInfo(attr_shmid, "\t");
		cfg_son_shmid = cfg_node->children;
		while(cfg_son_shmid)
		{
			cfg_son_node = (cfgNode*)shmat(cfg_son_shmid, NULL, 0);
			printf("\t[%s:]\n", cfg_son_node->name);
			attr_shmid = cfg_son_node->attr;
			PrintAttrInfo(attr_shmid, "\t\t");
			cfg_grandson_shmid = cfg_son_node->children;
			while(cfg_grandson_shmid)
			{
				cfg_grandson_node = (cfgNode*)shmat(cfg_grandson_shmid, NULL, 0);
				printf("\t\t[%s:]\n", cfg_grandson_node->name);
				attr_shmid = cfg_grandson_node->attr;
				PrintAttrInfo(attr_shmid, "\t\t\t");
				cfg_grandson_shmid = cfg_grandson_node->next;
				shmdt(cfg_grandson_node);
			}
			cfg_son_shmid = cfg_son_node->next;
			shmdt(cfg_son_node);
		}
		if(0 == strcmp(path, "root"))
		{
			cfg_shmid = cfg_node->next;
			shmdt(cfg_node);
		}
		else
		{	shmdt(cfg_node);
			break;
		}
		
	}
	
	return;
}

/// @brief 获取节点的属性值，此API提供给外部使用
/// @param path 节点路径，如root.map.map0.rect0
/// @param attr 属性名称
/// @param value 存储获取到的值
/// @param lenth value的长度
/// @return 成功则返回获取到的字节数，失败返回-1
int rdn_get(char* path, char* attr, char* value, int lenth)
{
	cfgNode* node = NULL;
	int node_shmid = 0;
	int attr_shmid = 0;

	if(path == NULL || attr == NULL || value == NULL)
	{
		printf("path/attr/value is NULL!!!\n");
		return -1;
	}
	
	if(node_shmid = rdn_query(path))
	{
		node = (cfgNode*)shmat(node_shmid, NULL, 0);
		attr_shmid = node->attr;
		shmdt(node);
		return GetNodeAttr(attr_shmid, attr, value, lenth);
	}

	printf("[%s] is not exist!!!\n", path);
	return -1;
}

/// @brief 设置节点的属性值
/// @param path 节点路径，如root.map.map0.rect0
/// @param attr 属性名称
/// @param value 要设置的值
/// @return 成功则返回0， 失败则返回-1
int rdn_set(char* path, char* attr, char* value)
{
	cfgNode* node = NULL;
	int node_shmid = 0;
	int attr_shmid = 0;

	if(path == NULL || attr == NULL || value == NULL)
	{
		printf("path/attr/value is NULL!!!\n");
		return -1;
	}
	
	if(node_shmid = rdn_query(path))
	{
		node = (cfgNode*)shmat(node_shmid, NULL, 0);
		attr_shmid = node->attr;
		if(attr_shmid)
		{
			shmdt(node);
			return SetNodeAttr(attr_shmid, attr, value);	
		}
		else
		{
//			attr_shmid = CreateAttrShmNode(attr_shmid, attr, value);
//			node->attr = attr_shmid;
//			shmdt(node);
			printf("Attribute:[%s] is not exist,plaease config it in xml first!!!\n", attr);
			return -1;
		}
	}

	printf("[%s] is not exist!!!\n", path);
	return -1;
}

/// @brief 提交修改后的节点数据
/// @param  节点路径，如root.wifi
/// @return
int rdn_commit(char* path)
{
	printf("committing=>\n");
}

