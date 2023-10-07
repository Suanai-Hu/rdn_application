#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>    
#include "rdn_api.h"

int main()
{
	int i = 0;
	char value[16] = {0};
	struct timeval start, end;
    

#if 1
	rdn_get("root.map.map0.rect0", "c", value, sizeof(value));
	printf("root.map.map0.rect0 => c=%s====>\n",value);

	rdn_get("root.map.map0.rect4", "bb", value, sizeof(value));
	printf("root.map.map0.rect4 => bb=%s====>\n",value);

	rdn_get("root.map.map0", "mapname", value, sizeof(value));
	printf("root.map.map0 => mapname=%s====>\n",value);

	rdn_get("root.wifi", "ssid0", value, sizeof(value));
	printf("root.wifi => ssid0=%s====>\n",value);
	
	rdn_get("root.maintain.robot", "recoder", value, sizeof(value));
	printf("root.maintain.robot => recoder=%s====>\n",value);


	printf("=============================================\n");
	
	rdn_show("root.map.map0.rect2");

	printf("=============================================\n");
	
	rdn_show("root.maintain");
	
	printf("=============================================\n");

#if 1
	while(1)
	{
		memset(&start, 0, sizeof(start));
		memset(&end, 0, sizeof(end));
		gettimeofday(&start, NULL);
		rdn_get("root.maintain1.remote", "recoder", value, sizeof(value));
		gettimeofday(&end, NULL);
		printf("[root.maintain1.remote]=>[recoder = %s]=> [cost %d usec]====>\n",value,(int)(1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)));

		memset(&start, 0, sizeof(start));
		memset(&end, 0, sizeof(end));
		gettimeofday(&start, NULL);
		rdn_get("root.maintain30.remote", "recoder", value, sizeof(value));
		gettimeofday(&end, NULL);
		printf("[root.maintain30.remote]=>[recoder = %s]=> [cost %d usec]====>\n",value,(int)(1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)));


		memset(value, 0, sizeof(value));
		rdn_get("root.map.map4.rect4", "a", value, sizeof(value));
		printf("[Befort set]=>[root.map.map4.rect4] => [a = %s]====>\n",value);
		sprintf(value, "%d", i);
		rdn_set("root.map.map4.rect4", "a", value);
		memset(value, 0, sizeof(value));
		rdn_get("root.map.map4.rect4", "a", value, sizeof(value));
		printf("[After set]=>[root.map.map4.rect4] => [a = %s]====>\n",value);

		i++;
	
		printf("===================================================================================\n");

		sleep(2);
	}
#endif	
#endif

}


