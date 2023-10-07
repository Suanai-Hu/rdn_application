#include <stdio.h>
#include <string.h>
#include "libxml/parser.h"
#include "libxml/tree.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include "rdn_api.h"

/// @brief共享内存链表的头节点ID 
int init_shmid = -1;

/// @brief 解析XML格式的文件
/// @param docname xml文件路径
/// @return 返回解析后的根节点指针
static xmlNodePtr parseDoc(char *docname) 
{
    xmlDocPtr doc;
    xmlNodePtr cur;
	
    xmlKeepBlanksDefault(0);
    doc = xmlParseFile(docname);
    
    if (doc == NULL)
	{
        fprintf(stderr,"Document not parsed successfully. \n");
        return NULL;
    }
    
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) 
	{
        fprintf(stderr,"empty document\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    
    return cur;
}

/// @brief 初始化头节点
static void InitCfgNodeList(void)
{
	cfgNode* node = NULL;
	
	init_shmid = shmget(KEY, sizeof(cfgNode), IPC_CREAT|0666);
	if(init_shmid >= 0)
	{
		node = (cfgNode*)shmat(init_shmid, NULL, 0);
		node->next = 0;
		node->prev = 0;
		node->attr = 0;
		node->children = 0;
	}
	
	printf("[%s:%d]init_shmid=%d====>\n", __FUNCTION__, __LINE__, init_shmid);
	return;
}

/// @brief 设置共享内存节点的值
/// @param shmid 共享内存ID
/// @param node_name 节点名称
/// @param attr 属性的共享内存ID
/// @param next 下一个节点的共享内存ID
/// @param prev 上一个节点的共享内存ID
/// @param children 子节点的共享内存ID
static void SetCfgShmNodeValue(int shmid, char* node_name, int attr, int next, int prev, int children)
{
	cfgNode* node = (cfgNode*)shmat(shmid, NULL, 0);
	if(node_name)
	{
		strncpy(node->name, node_name, sizeof(node->name)-1);
	}

	if(attr)
	{
		node->attr = attr;
	}

	if(next)
	{
		node->next = next;
	}
	
	if(prev)
	{
		node->prev = prev;
	}

	if(children)
	{
		node->children = children;
	}
	
	shmdt(node);
	return;
}

/// @brief 创建下一个共享内存节点
/// @param cfg_shmid 当前最后一个共享内存节点
/// @param node_name 节点名称
/// @return 返回新创建的节点的共享内存ID
static int CreateCfgShmNode(int cfg_shmid,char* node_name)
{
	int shmid = 0;
	cfgNode* node = NULL;
	cfgNode* cfg_node = NULL;
	
	shmid = shmget(0, sizeof(cfgNode), IPC_CREAT|0666);
	if(shmid <= 0)
	{
		printf("Create shmid fail====>");
		return -1;
	}
	node = (cfgNode*)shmat(shmid, NULL, 0);
	strncpy(node->name, node_name, sizeof(node->name)-1);
	node->next = 0;
	node->prev = cfg_shmid;
	node->children = 0;
	node->attr = 0;

	if(cfg_shmid)
	{
		cfg_node = (cfgNode*)shmat(cfg_shmid, NULL, 0);
		cfg_node->next = shmid;
		shmdt(cfg_node);
	}
	
	shmdt(node);

	return shmid;
}

/// @brief 创建下一个属性的共享内存节点
/// @param attr_shmid 当前最后一个属性的共享内存ID
/// @param attr_name 属性名
/// @param value 属性值
/// @return 返回新创建的共享内存ID
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

/// @brief 将节点加入到共享内存链表中
/// @param shmid 节点的共享内存ID
static void InsertCfgNodeList(int shmid)
{	
	cfgNode* init_node = NULL;

	init_node = (cfgNode*)shmat(init_shmid, NULL, 0);
	if(init_node->next == 0)
	{
		init_node->next = shmid;
		init_node->prev = shmid;
	}
	
	shmdt(init_node);

	return;
}

/// @brief 加载xml文件到共享内存中
/// @param file_name 
static void LoadXmlFileToMemory(char* file_name)
{
	xmlAttrPtr attrPtr = NULL;
	xmlChar* szAttr = NULL;
	xmlNodePtr curnode = NULL;
	xmlNodePtr childnode = NULL;
	xmlNodePtr grandsonnode = NULL;
	xmlNodePtr root = NULL;

	int flag = 0;
	int shmid = 0;
	cfgNode* cfg_node = NULL;
	int cfg_shmid = 0;
	int attr_shmid = 0;
	int child_cfg_shmid = 0;
	int child_attr_shmid = 0;
	int grandson_cfg_shmid = 0;
	int grandson_attr_shmid = 0;
	int last_child_cfg_shmid = 0;
	int last_grandson_cfg_shmid = 0;

	if(file_name == NULL || file_name[0] == '\0')
	{
		return;
	}
	
	root = parseDoc(file_name);	
	curnode = root->xmlChildrenNode;
    if(curnode == NULL)
    {
		printf("curnode is NULL====>\n");
		return;
	}

	if(init_shmid < 0)
	{
		printf("init_shmid < 0====>");
		return;
	}

	//解析第一层节点如root.map
    while(curnode)
	{
		flag = 0;
		//cfg_shmid = 0;
		attr_shmid = 0;
		//printf("[%s]:\n", curnode->name);
        attrPtr = curnode->properties;
		//printf("cfg_shmid=%d,nodename=%s====>\n",cfg_shmid,(char*)(curnode->name));
		cfg_shmid = CreateCfgShmNode(cfg_shmid, (char*)(curnode->name));
		//printf("cfg_shmid=%d,nodename=%s====>\n",cfg_shmid,(char*)(curnode->name));
		//Create & init new node
		while(attrPtr)
		{
            szAttr = xmlGetProp(curnode,BAD_CAST attrPtr->name);
			//printf("\t[%s = %s]\n", attrPtr->name, szAttr);
			//printf("attr_shmid=%d,attrname=%s====>\n",attr_shmid,(char*)(attrPtr->name));
			attr_shmid = CreateAttrShmNode(attr_shmid, (char*)attrPtr->name, szAttr);
			//printf("attr_shmid=%d,attrname=%s====>\n",attr_shmid,(char*)(attrPtr->name));
			if(flag == 0)
			{
				SetCfgShmNodeValue(cfg_shmid, NULL, attr_shmid, 0, 0, 0);
				flag = 1;
			}
			
            xmlFree(szAttr);
    		attrPtr = attrPtr->next;
		}

		flag = 0;
		shmid = 0;
		child_cfg_shmid = 0;
		child_attr_shmid = 0;
		last_child_cfg_shmid = 0;
		//解析第二层节点如root.map.map1
		childnode = curnode->xmlChildrenNode;
		while(childnode)
		{
			//printf("\t[%s]:\n", childnode->name);
            attrPtr = childnode->properties;
			//printf("child_cfg_shmid=%d,nodename=%s====>\n",child_cfg_shmid,(char*)(childnode->name));
			shmid = child_cfg_shmid;
			child_cfg_shmid = CreateCfgShmNode(child_cfg_shmid, (char*)childnode->name);
			//printf("child_cfg_shmid=%d,nodename=%s====>\n",child_cfg_shmid,(char*)(childnode->name));
			if(shmid == 0)
			{
				SetCfgShmNodeValue(cfg_shmid, NULL, 0, 0, 0, child_cfg_shmid);
				shmid = child_cfg_shmid;
			}

			child_attr_shmid = 0;
			flag = 0;
    		while(attrPtr)
			{
	            szAttr = xmlGetProp(childnode,BAD_CAST attrPtr->name);
				//printf("child_attr_shmid=%d,attrname=%s====>\n",child_attr_shmid,(char*)(attrPtr->name));
				child_attr_shmid = CreateAttrShmNode(child_attr_shmid, (char*)attrPtr->name, szAttr);
				//printf("child_attr_shmid=%d,attrname=%s====>\n",child_attr_shmid,(char*)(attrPtr->name));
	            //printf("\t\t[%s = %s]\n", attrPtr->name, szAttr);
				if(flag == 0)
				{
					SetCfgShmNodeValue(child_cfg_shmid, NULL, child_attr_shmid, 0, 0, 0);
					flag = 1;
				}
	            xmlFree(szAttr);
        		attrPtr = attrPtr->next;
    		}
			
			flag = 0;
			shmid = 0;
			grandson_cfg_shmid = 0;
			//解析第三层节点如root.map.map1.rect0
			grandsonnode = childnode->xmlChildrenNode;
			while(grandsonnode)
			{
				//printf("\t\t[%s]:\n", grandsonnode->name);
	            attrPtr = grandsonnode->properties;
				//printf("grandson_cfg_shmid=%d,nodename=%s====>\n",grandson_cfg_shmid,(char*)(grandsonnode->name));
				shmid = grandson_cfg_shmid;
				grandson_cfg_shmid = CreateCfgShmNode(grandson_cfg_shmid, (char*)grandsonnode->name);
				//printf("grandson_cfg_shmid=%d,nodename=%s====>\n",grandson_cfg_shmid,(char*)(grandsonnode->name));
				if(shmid == 0)
				{	
					SetCfgShmNodeValue(child_cfg_shmid, NULL, 0, 0, 0, grandson_cfg_shmid);
					shmid = grandson_cfg_shmid;
				}
				grandson_attr_shmid = 0;
				flag = 0;
	    		while(attrPtr)
				{
		            szAttr = xmlGetProp(grandsonnode,BAD_CAST attrPtr->name);
					//printf("grandson_attr_shmid=%d,attrname=%s====>\n",grandson_attr_shmid,(char*)(attrPtr->name));
					grandson_attr_shmid = CreateAttrShmNode(grandson_attr_shmid, (char*)attrPtr->name, szAttr);
					//printf("grandson_attr_shmid=%d,attrname=%s====>\n",grandson_attr_shmid,(char*)(attrPtr->name));
		            //printf("\t\t\t[%s = %s]\n", attrPtr->name, szAttr);
					if(flag == 0)
					{
						SetCfgShmNodeValue(grandson_cfg_shmid, NULL, grandson_attr_shmid, 0, 0, 0);
						flag = 1;
					}
		            xmlFree(szAttr);
	        		attrPtr = attrPtr->next;
	    		}
				grandsonnode = grandsonnode->next;

			}
			childnode = childnode->next;
		}

		InsertCfgNodeList(cfg_shmid);
		//printf("\n=================================================================================\n");

        curnode = curnode->next;
    } 
	
    return;
}


int main(int argc, char **argv)
{
	char* file_path = "/userdata/cfg/init.xml";
	if(argc == 2)
	{
		file_path = argv[1];
	}
	if((access(file_path, F_OK)) == -1)
	{
		printf("XML config file is not exist====>\n");
		return -1;
	}
	
	InitCfgNodeList();
   	LoadXmlFileToMemory(file_path);
	
    return 0;
}


