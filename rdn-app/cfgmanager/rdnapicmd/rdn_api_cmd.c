#include <stdio.h>
#include <string.h>
#include "rdn_api.h"

void usage(void)
{
	printf("Usage:");
	printf("\t[1]. rdn get	{root.node1.node2.node3} {attribute}\n");
	printf("\t[2]. rdn set 	{root.node1.node2.node3} {attribute} {value}\n");
	printf("\t[3]. rdn show	{root.node1.node2.node3}\n");
	
}

int main(int argc, char*argv[])
{
	char value[64] = {0};

	if(argc < 2 || argc > 5)
	{
		goto help;
	}
	
	if(0 == strcmp(argv[1], "get"))
	{
		if(rdn_get(argv[2], argv[3], value, sizeof(value)) > 0)
		{
			printf("%s\n",value);
		}
		return 0;
	}
	else if(0 == strcmp(argv[1], "set"))
	{
		if(0 == rdn_set(argv[2], argv[3], argv[4]))
		{
			printf("\tSUCCESS!!!\n");
		}
		else
		{
			printf("\tFAIL!!!\n");
		}
		return 0;
	}
	else if(0 == strcmp(argv[1], "show"))
	{
		rdn_show(argv[2]);
		return 0;
	}
	else
	{
		goto help;
	}

help:
	usage();

	return 0;	
}
