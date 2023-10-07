#include <stdio.h>  
#include <stdlib.h> 
#include <string.h>  
#include <unistd.h>   
#include <fcntl.h>   
#include <limits.h>   
#include <sys/types.h>   
#include <sys/wait.h>   
#include <stdio.h>
#include "rdn_api.h"
#include "debug.h"
#include "node.h"

#define RDN_RK3399_PROCESS 	"rdn-rk3399"

int isProcessExist(char* process)
{   
	int pid = 0;   
	FILE* fp = NULL;
	char pid_file[32] = {0};
	char buf[64] = {0};
	char cmd[64] = {0}; 

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/tmp/%s.pid", process);	

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "pidof %s > %s", process, pid_file);	
	system(cmd);

	fp = fopen(pid_file, "r");
	if(fp)
	{
		memset(buf, 0, sizeof(buf));
		if(fgets(buf, sizeof(buf)-1, fp))
		{ 
			pid = atoi(buf);   	  
		} 
		fclose(fp); 
	}
	  
	return pid;
}

int main()
{
	int i = 0;
	int cycle = 0;
	char buf[32] = {0};
	char process[32] = {0};
	
	sleep(10);
	memset(buf, 0, sizeof(buf));
	rdn_get(MONITOR_NODE, "cycle", buf, sizeof(buf));
	cycle = atoi(buf);
	
	while(1)
	{
		sleep(cycle);
		memset(buf, 0, sizeof(buf));
		rdn_get(ENABLE_NODE, "monitor", buf, sizeof(buf));
		if(atoi(buf) == 0)
		{
			//LOG_WARN("Disabled====>\n");
			continue;
		}
		
		for(i=1; i<=5; i++)
		{
			memset(process, 0, sizeof(process));
			memset(buf, 0, sizeof(buf));
			snprintf(process, sizeof(process), "process%d", i);
			if(rdn_get(MONITOR_NODE, process, buf, sizeof(buf)) > 0)
			{
				if(!isProcessExist(buf))
				{
					LOG_WARN("[%s] is not exist, will start it again====>\n",buf);
					if(0 == strcmp(buf, RDN_RK3399_PROCESS))
					{		
						chdir("/userdata/app/");
						system("./start_rk3399.sh");
						system("echo 1 > /tmp/rdn_reboot");
					}
					else
					{
						memset(process, 0, sizeof(process));
						snprintf(process, sizeof(process), "%s &", buf);
						system(process);
					}
				}
			}
		}
	}

	return 0;
}


