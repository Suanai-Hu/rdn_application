#include <stdio.h>
#include <string.h>
#include "libxml/parser.h"
#include "libxml/tree.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include "rdn_api.h"

/// @brief 保存共享内存中节点信息到xml文件
/// @param  
/// @return 
int main(int argc, char** argv)
{
	int ret = 0;
	int attr_shmid = 0;
	int cfg_shmid = 0;
	int cfg_son_shmid = 0;
	int cfg_grandson_shmid = 0;

	char* file_path = "/oem/romfile/.config.xml";
	if(argc == 2)
	{
		file_path = argv[1];
	}
	
	attrNode* attr_node = NULL;
	cfgNode* cfg_node = NULL;
	cfgNode* cfg_son_node = NULL;
	cfgNode* cfg_grandson_node = NULL;

	xmlNodePtr xml_node = NULL;
	xmlNodePtr xml_son_node = NULL;
	xmlNodePtr xml_grandson_node = NULL;
	
	/*定义文档*/
	xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
	/*定义根节点*/
	xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "root");
	/*设置根节点到文档*/	
	xmlDocSetRootElement(doc, root);	

	cfg_shmid = shmget(KEY, sizeof(cfgNode), IPC_CREAT|0666);
	cfg_node = (cfgNode*)shmat(cfg_shmid, NULL, 0);
	cfg_shmid = cfg_node->next;
	shmdt(cfg_node);
	
	while(cfg_shmid)
	{
		cfg_node = (cfgNode*)shmat(cfg_shmid, NULL, 0);
		xml_node = xmlNewNode(NULL,BAD_CAST cfg_node->name);
		xmlAddChild(root, xml_node);
		attr_shmid = cfg_node->attr;
		while(attr_shmid)
		{
			attr_node = (attrNode*)shmat(attr_shmid, NULL, 0);
			xmlNewProp(xml_node, BAD_CAST attr_node->name, BAD_CAST attr_node->value);
			attr_shmid = attr_node->next;
			shmdt(attr_node);
		}

		cfg_son_shmid = cfg_node->children;
		while(cfg_son_shmid)
		{
			cfg_son_node = (cfgNode*)shmat(cfg_son_shmid, NULL, 0);
			xml_son_node = xmlNewNode(NULL,BAD_CAST cfg_son_node->name);
			xmlAddChild(xml_node, xml_son_node);
			attr_shmid = cfg_son_node->attr;
			while(attr_shmid)
			{
				attr_node = (attrNode*)shmat(attr_shmid, NULL, 0);
				xmlNewProp(xml_son_node, BAD_CAST attr_node->name, BAD_CAST attr_node->value);
				attr_shmid = attr_node->next;
				shmdt(attr_node);
			}

			cfg_grandson_shmid = cfg_son_node->children;
			while(cfg_grandson_shmid)
			{
				cfg_grandson_node = (cfgNode*)shmat(cfg_grandson_shmid, NULL, 0);
				xml_grandson_node = xmlNewNode(NULL,BAD_CAST cfg_grandson_node->name);
				xmlAddChild(xml_son_node, xml_grandson_node);
				attr_shmid = cfg_grandson_node->attr;
				while(attr_shmid)
				{
					attr_node = (attrNode*)shmat(attr_shmid, NULL, 0);
					xmlNewProp(xml_grandson_node, BAD_CAST attr_node->name, BAD_CAST attr_node->value);
					attr_shmid = attr_node->next;
					shmdt(attr_node);
				}
				cfg_grandson_shmid = cfg_grandson_node->next;
				shmdt(cfg_grandson_node);
			}
			cfg_son_shmid = cfg_son_node->next;
			shmdt(cfg_son_node);
		}
		cfg_shmid = cfg_node->next;
		shmdt(cfg_node);
	}

	ret = xmlSaveFile(file_path, doc);
	if (ret != -1)
	{
		printf("Save xml to [%s]，Write [%d] bytes!\n", file_path, ret);
	}

	xmlFreeDoc(doc);
	system("sync");
	return 0;
}

