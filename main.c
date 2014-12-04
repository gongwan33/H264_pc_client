#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

struct cmd{
  char head[5];
  char opcode;
  char packet[17];
} command;

int main()
{
	int cfd;
	int writebytes;
	int sin_size;
	struct sockaddr_in s_add,c_add;
	unsigned short portnum=80; 

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
	printf("connect ok !\r\n");

	memset(&command, 0, sizeof(command));
	memcpy(&command.head, "MO_O", 5);
	command.opcode = 0;
	printf("command = %s\n", (char *)&command);

	if(-1 == (writebytes = send(cfd, (char *)&command, sizeof(command), 0)))
	{
		    printf("write data fail !\r\n");
			    return -1;
	}

	printf("%d\r\n", writebytes);

	getchar();
	close(cfd);
	return 0;
}
