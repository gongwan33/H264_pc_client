#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <s_cmd.h>
#include <r_cmd.h>
#include <pthread.h>

int iStatus = STATE_DISCONNECTED; 
int connected = 0;

int main()
{
	int cfd = -1;
	int writebytes;
	int sin_size;
	struct sockaddr_in s_add,c_add;
	unsigned short portnum=80;
	pthread_t tid;

	printf("Hello,welcome to client !\r\n");

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == cfd)
	{
		printf("socket fail ! \r\n");
		return -1;
	}
	printf("socket ok !\r\n");

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr("192.168.1.44");
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
    pthread_create(&tid, NULL, receiveThread, (void *)&cfd);
	sendCommand(Login_Req, cfd);

	getchar();
	connected = 0;
	pthread_join(tid, NULL);
	close(cfd);
	return 0;
}
