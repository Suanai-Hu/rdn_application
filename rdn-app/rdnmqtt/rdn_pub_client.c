#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include "debug.h"
#include "rdn_api.h"
#include "node.h"
#include "rdn_mqtt.h"

void* RdnMqttPubClient(void* p)
{
	while(1)
	{
		HandleErrorMsg();
		sleep(5);
	}

}


