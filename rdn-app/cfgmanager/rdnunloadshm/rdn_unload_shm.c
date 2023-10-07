#include <stdio.h>
#include <string.h>
#include "libxml/parser.h"
#include "libxml/tree.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include "rdn_api.h"

/// @brief 释放属性节点的共享内存
/// @param  
/// @return 
void ReleaseAttrShm(int attr_shmid)
{
	int shmid = 0;
	int rm_shmid = 0;
	attrNode* attr_node = NULL;
	
	shmid = attr_shmid;
	while(shmid)
	{
		rm_shmid = shmid;
		attr_node = (attrNode*)shmat(shmid, NULL, 0);
		shmid = attr_node->next;
		//printf("Release attr=[%s]====>\n",attr_node->name);
		shmdt(attr_node);	
		shmctl(rm_shmid, IPC_RMID, 0);
	}
}

/// @brief 释放配置节点的共享内存
/// @param  
/// @return 
void ReleaseCfgShm(int cfg_shmid)
{
	int shmid = 0;
	int rm_shmid = 0;
	cfgNode* cfg_node = NULL;
	
	shmid = cfg_shmid;
	while(shmid)
	{
		rm_shmid = shmid;
		cfg_node = (cfgNode*)shmat(shmid, NULL, 0);
		shmid = cfg_node->next;
		//printf("Release cfg=[%s]====>\n",cfg_node->name);
		shmdt(cfg_node);	
		shmctl(rm_shmid, IPC_RMID, 0);
	}
}


/// @brief 保存共享内存中节点信息到xml文件
/// @param  
/// @return 
int main(int argc, char** argv)
{
	int attr_shmid = 0;
	
	int cfg_shmid = 0;
	int cfg_son_shmid = 0;
	int cfg_grandson_shmid = 0;

	int rm_cfg_shmid = 0;
	int rm_cfg_son_shmid = 0;
	int rm_cfg_grandson_shmid = 0;

	cfgNode* cfg_node = NULL;
	cfgNode* cfg_son_node = NULL;
	cfgNode* cfg_grandson_node = NULL;

	cfg_shmid = shmget(KEY, sizeof(cfgNode), IPC_CREAT|0666);
	cfg_node = (cfgNode*)shmat(cfg_shmid, NULL, 0);
	cfg_shmid = cfg_node->next;
	shmdt(cfg_node);

	rm_cfg_shmid = cfg_shmid;
	while(cfg_shmid)
	{
		cfg_node = (cfgNode*)shmat(cfg_shmid, NULL, 0);
		attr_shmid = cfg_node->attr;
		ReleaseAttrShm(attr_shmid);

		cfg_son_shmid = cfg_node->children;
		rm_cfg_son_shmid = cfg_son_shmid;
		while(cfg_son_shmid)
		{
			cfg_son_node = (cfgNode*)shmat(cfg_son_shmid, NULL, 0);
			attr_shmid = cfg_son_node->attr;
			ReleaseAttrShm(attr_shmid);
			cfg_grandson_shmid = cfg_son_node->children;
			rm_cfg_grandson_shmid = cfg_grandson_shmid;
			while(cfg_grandson_shmid)
			{
				cfg_grandson_node = (cfgNode*)shmat(cfg_grandson_shmid, NULL, 0);
				attr_shmid = cfg_grandson_node->attr;
				ReleaseAttrShm(attr_shmid);
				cfg_grandson_shmid = cfg_grandson_node->next;
				shmdt(cfg_grandson_node);
			}
			ReleaseCfgShm(rm_cfg_grandson_shmid);
			cfg_son_shmid = cfg_son_node->next;
			shmdt(cfg_son_node);
		}
		ReleaseCfgShm(rm_cfg_son_shmid);
		cfg_shmid = cfg_node->next;
		shmdt(cfg_node);
	}
	ReleaseCfgShm(rm_cfg_shmid);

	return 0;
}

