#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#include <s_cmd.h>
#include <r_cmd.h>
#include <blowfish.h>
#include <playback.h>
#include <decodeH264.h>
#include <p2p/JEAN_P2P.h>

#define server_port 61000
#define local_port 6788
#define server_ip "192.168.1.109"

int iStatus = STATE_DISCONNECTED; 
int connected = 0;
pthread_t tid;

int main()
{
	int ret = 0;

	printf("Hello,welcome to client !\r\n");

    BlowfishKeyInit(BLOWFISH_KEY, strlen(BLOWFISH_KEY));

	ret = JEAN_init_slave(server_port, local_port, server_ip);
	if( 0 > ret )
	{
		printf("init slave error");
		return -1;
	}

	ret = init_CMD_CHAN();
	if( 0 > ret )
	{
		printf("init cmd error");
		return -1;
	}

	connected = 1;
	printf("connect ok !\r\n");

	//connect completed! Start protocal!----------------------------------------------------------
    pthread_create(&tid, NULL, receiveThread, NULL);
	sendCommand(Login_Req);

	while(1);

	connected = 0;
	pthread_join(tid, NULL);
	pthread_join(avtid, NULL);
	pthread_join(playtid, NULL);
	pthread_join(h264tid, NULL);
	clearList(&audioList);
	clearVideoList(&vList);

	close_CMD_CHAN();
	JEAN_close_slave();

	return 0;
}
