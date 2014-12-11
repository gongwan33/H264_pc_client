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

int iStatus = STATE_DISCONNECTED; 
int connected = 0;
int cfd = -1;
int avfd = -1;
pthread_t tid;

int main()
{
	struct sockaddr_in s_add,c_add;
	unsigned short portnum=80;

	printf("Hello,welcome to client !\r\n");

    BlowfishKeyInit(BLOWFISH_KEY, strlen(BLOWFISH_KEY));

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == cfd)
	{
		printf("socket fail ! \r\n");
		return -1;
	}
	printf("socket ok !\r\n");

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr(SERVER_IP);
	s_add.sin_port=htons(portnum);
	printf("s_addr = %#x ,port : %#x\r\n",s_add.sin_addr.s_addr,s_add.sin_port);

	if(-1 == connect(cfd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
		printf("connect fail !\r\n");
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
	if(avfd != -1)
		close(avfd);
	close(cfd);
	return 0;
}
