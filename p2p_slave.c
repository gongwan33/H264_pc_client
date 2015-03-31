/*AUTHOR:WANGGONG, CHINA
 *VERSION:1.0
 *FUNCTION:SLAVE
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <p2p/JEANP2PPRO.h>
#include <p2p/commonkey.h>
#include <p2p/ring.h>
#include <p2p/DSet.h>
#include <p2p/unitBuf.h>

#define MAX_TRY 5
#define SEND_BUFF_SIZE 1024*3
#define MAX_RECEIVE 1024*1024
#define MAX_RECV_BUF 1024*1024*5
#define CONTROL_BUF_SIZE 2000 
#define MTU 1000

#define TURN_DATA_SIZE 1024*3
#define KEEP_CONNECT_PACK 0
//#define server_ip_1 "192.168.1.216"
//#define server_ip_1 "192.168.1.114"
//#define server_ip_1 "192.168.1.4"
//#define server_ip_1 "192.168.1.109"
#define server_ip_1 "103.226.127.42"

#define USERNAME "wang"
#define PASSWD "123456"

#define ACT_NETCARD "eth0"
#define server_port 61000
#define server_turn_port 61001
#define server_cmd_port 61002
#define server_control_port 61003
#define local_port 6788

static char recvSign;
static char controlSign;
static struct sockaddr_in servaddr1, local_addr, recv_sin, master_sin, host_sin, turnaddr;
static struct ifreq ifr, *pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd;
static int cmdfd;
static int controlfd;
static int port, sin_size, recv_sin_len;
static char mac[6], ip[4], buff[1024];
static pthread_t keep_connection;
static char pole_res;
static unsigned int sendIndex;
static unsigned int getNum;
static unsigned int sendNum;
static char* recvBuf;
static char* recvProcessBuf;
static char* recvProcessBackBuf;
static pthread_t recvDat_id;
static pthread_mutex_t recvBuf_lock;
static pthread_mutex_t synGetCount_lock;
static unsigned int recvBufP;
static unsigned int recvProcessBufP;
static unsigned int recvProcessBackBufP;
static char recvThreadRunning = 0;
struct elm elmPack[1000];
static int elmPackP = 0;	
static pthread_mutex_t recvProcessBuf_lock;
static pthread_t analyseRecvData_t;
static int analyseRecvDataRunning = 0;
static int scanP_g = 0;
static unsigned char windowLen = 0; //should not larger than 255

unsigned char connectionStatus = FAIL;

int JEAN_recv_timeout = 1000;//1s
int commonKey = 0;
static int controlChanThreadRunning = 0;
static pthread_t control_t;
static unsigned int lastAdd = 0;
static unsigned int lastSliceIndex = 0;
static unsigned int lastHeadLen = 0;
static unsigned int lastTotalLen = 0;
static unsigned int page = 0;
static unsigned int recvPage = 0;

int init_CONTROL_CHAN();
int close_CONTROL_CHAN();
int send_control(void *, int);
int recv_control(void *, int);
void* analyseRecvData(void *argc);

long timeDelta(struct timeval *tvs, struct timeval *tve)
{
	long s, e;
	s = ((long)tvs->tv_sec)*1000000 + (long)tvs->tv_usec; 
	e = ((long)tve->tv_sec)*1000000 + (long)tve->tv_usec; 
	return (e - s);
}

int local_net_init(int localPort){
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(localPort);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return -1;
	}

	if(bind(sockfd, (struct sockaddr *) (&local_addr),sizeof(local_addr)) == -1){
		printf ("Bind error: %s\a\n", strerror (errno));
		return -2;
	}

	return 0;
}

void init_host_sin(char * str_ip, int p){
	memset(&host_sin, 0, sizeof(host_sin));
	host_sin.sin_family = AF_INET;
	host_sin.sin_addr.s_addr = inet_addr(str_ip);
	host_sin.sin_port = htons(p);
}

void init_recv_sin(){
	memset(&recv_sin, 0, sizeof(recv_sin));
	recv_sin.sin_family = AF_INET;
	recv_sin.sin_addr.s_addr = inet_addr("1.1.1.1");
	recv_sin.sin_port = htons(10000);
	recv_sin_len = sizeof(recv_sin);
}

int get_local_ip_port(){
	int i = 0;
	sin_size = sizeof(local_addr);
	getsockname(sockfd, (struct sockaddr*)&local_addr, &sin_size);
	port = ntohs(local_addr.sin_port);

	memset(&ifr, 0, sizeof(ifr));
	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;

	if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0){
		printf("SIOCGIFCONF screwed up\n");
		return -1;
	}

	pifr = (struct ifreq *)(ifc.ifc_req);

	for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; pifr++){
		strcpy(ifr.ifr_name, pifr->ifr_name);    //eth0 eth1 ...

		if(strcmp(ACT_NETCARD, ifr.ifr_name) != 0)
			continue;

		if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0){
			printf("ip screwed up\n");
			return -2;
		}
		memcpy(ip, ifr.ifr_addr.sa_data+2, 4);
		//printf("%s\n",inet_ntoa(*(struct in_addr *)ip));
	}	

	printf("local ip is %s\n",inet_ntoa(*(struct in_addr *)ip));
	printf("local port is %d\n", port);

	init_host_sin(inet_ntoa(*(struct in_addr*)ip), port);

	return 0;
}

int set_ip1_struct(char * ip1, int port){
	memset(&servaddr1, 0, sizeof(servaddr1));
	servaddr1.sin_family = AF_INET;
	servaddr1.sin_port = htons(port);
	
	if( inet_pton(AF_INET, ip1, &servaddr1.sin_addr) <= 0){
		printf("inet_pton error for %s\n",ip1);
		return -1;
	}

	memset(&turnaddr, 0, sizeof(turnaddr));
	turnaddr.sin_family = AF_INET;
	turnaddr.sin_port = htons(server_turn_port);
	
	if( inet_pton(AF_INET, ip1, &turnaddr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",ip1);
		return -1;
	}

	return 0;
}

int set_rec_timeout(int usec, int sec){
	struct timeval tv_out;
    tv_out.tv_sec = sec;
    tv_out.tv_usec = usec;

	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
}

int Send_TURN(){
	struct p2p_head head;
	memcpy(&head.logo, "TRN", 3);

	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

int Send_CMDOPEN(){
	struct p2p_head head;
	memcpy(&head.logo, "CMD", 3);
	head.data[0] = 'S';

	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

int Send_CONTROLOPEN(){
	struct p2p_head head;
	memcpy(&head.logo, "CTL", 3);
	head.data[0] = 'S';

	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

int Send_VUAPS(){
	struct p2p_head head;
	memcpy(&head.logo, "SUP", 3);
	if(strlen(USERNAME) > 10 || strlen(PASSWD) > 10) 
		return -1;

	memcpy(head.data, USERNAME, 10);
	memcpy(head.data + 10, PASSWD, 10);
	memcpy(head.data + 20, &host_sin, sizeof(struct sockaddr_in));

	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

void Send_IP_REQ(){
	struct p2p_head head;
	memcpy(&head.logo, "MIP", 3);
	memcpy(head.data, USERNAME, 10);
	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_POL(char req,struct sockaddr_in * sock){
	struct p2p_head head;
	if(req == POL_SENT)
	{
		memcpy(&head.logo, "POL", 3);
		memcpy(head.data, USERNAME, 10);
		sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)sock, sizeof(struct sockaddr_in));
		return;
	}
	else
	{
		ip_info[0] = req;
		sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)sock, sizeof(struct sockaddr_in));
		return;
	}
}

void Send_CMD(char Ctls, char Res){
	struct p2p_head head;
	if(Ctls == GET_REQ)
		memcpy(head.logo, "GRQ", 3);
	else if(Ctls == KEEP_CON)
		memcpy(head.logo, "KEP", 3);
	head.data[0] = Res;
	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_CMD_TO_MASTER(char Ctls, char Res){
	ip_info[0] = Ctls;
	ip_info[1] = Res;
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&master_sin, sizeof(struct sockaddr_in));
}

void *Keep_con(){
	pthread_detach(pthread_self());
	while(1){
		Send_CMD(KEEP_CON, 0x01);
		printf("Send KEEP_CON!\n");
		sleep(10);
	}
}

void clean_rec_buff(){
	char tmp[50];
	int ret;
	set_rec_timeout(100000, 0);//(usec, sec)
	while(ret > 0){
		ret = recvfrom(sockfd, tmp, 10, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		printf("Clean recv buff %d.\n", ret);
	}
	set_rec_timeout(0, 1);//(usec, sec)
}

void sendGet(unsigned int index)
{
	struct get_head getSt;
	unsigned char * buf;
	unsigned char num[4];
	memcpy(&getSt, "GET", 3);
	buf = (unsigned char *)malloc(sizeof(struct get_head) + sizeof(u_int32_t));
	memcpy(buf, &getSt, sizeof(getSt));

	num[0] = (index & 0xff);
	num[1] = ((index>>8) & 0xff);
	num[2] = ((index>>16) & 0xff);
	num[3] = ((index>>24) & 0xff);

	memcpy(buf + sizeof(getSt), num, 4);
	
	send_control(buf, sizeof(struct get_head) + sizeof(u_int32_t));
	free(buf);
}

void sendWin(unsigned char winLen)
{
	struct win_head winSt;
	unsigned char * buf;
	memcpy(&winSt, "WIN", 3);
	buf = (char *)malloc(sizeof(struct win_head) + sizeof(unsigned char));
	memcpy(buf, &winSt, sizeof(struct win_head));
	buf[4] = winLen;
	
	send_control(buf, sizeof(struct win_head) + sizeof(unsigned char));
	free(buf);
}

void sendSok()
{
	struct sok_head sokSt;

	memcpy(&sokSt, "SOK", 3);

	send_control((void *)&sokSt, sizeof(struct sok_head));
}

int elmInPack(u_int32_t index, struct elm * list, int len)
{
	int i = 0;
	for(i = 0; i < len; i++)
	{
		if(list[i].index == index)
			return 1;
	}
	return 0;
}	

void changeElm(struct elm *list, int i, int j)
{
	u_int32_t tmpI = 0, tmpS = 0, tmpE = 0;
	tmpI = list[i].index;
	tmpS = list[i].start;
	tmpE = list[i].end;
	list[i].index = list[j].index;
	list[i].start = list[j].start;
	list[i].end = list[j].end;
	list[j].index = tmpI;
	list[j].start = tmpS;
	list[j].end = tmpE;
}	

void elmSort(struct elm * list, int len)
{
	int i = 0, j = 0;
	for(i = 0; i <= len; i++)
	 for(j = i + 1; j <= len; j++)
		if(list[i].index > list[j].index)
			changeElm(list, i, j);
}

int findElm(unsigned int index)
{
	int i = 0;
	for(i = 0; i <= elmPackP; i++)
		if(elmPack[elmPackP].index == index)
			return i;
	return -1;
}

int tidyInBuf(char *buf, int len, char *dest, int *destlen)
{
	int scanP = 0;
	struct load_head head;
	int resLen = 0;
	char *tempBuf;
	elmPackP = 0;
	unsigned int lastIndex = 0;

	tempBuf = (char *)malloc(len);	

	while(scanP + sizeof(struct get_head) < len)
	{
		if(scanP + sizeof(struct load_head) < len && (buf[scanP] == 'J' && buf[scanP + 1] == 'E' && buf[scanP + 2] == 'A' && buf[scanP + 3] == 'N'))
		{
			memcpy(&head, buf + scanP, sizeof(struct load_head));

			if(len - scanP - sizeof(struct load_head) >= head.length)
			{
				if(elmPackP == 0 || !elmInPack(head.index, elmPack, elmPackP))
				{
					if(elmPackP < 999)
						elmPackP++;
					else
					{
						printf("elmPackP over flow!\n");
						exit(0);
						break;
					}

					elmPack[elmPackP].index = head.index;
					elmPack[elmPackP].start = scanP;
					elmPack[elmPackP].end = scanP + head.length + sizeof(struct load_head);
				}

				scanP = scanP + sizeof(struct load_head) + head.length;
			}
			else
				break;
		}
		else
			scanP++;
	}

	if(elmPackP >= 0)
	{
		elmSort(elmPack, elmPackP);

		int i = 0;
		for(i = 0; i <= elmPackP; i++)
		{
			memcpy(tempBuf + resLen, buf + elmPack[i].start, elmPack[i].end - elmPack[i].start); 
			resLen = resLen + elmPack[i].end - elmPack[i].start;
		}

		memcpy(dest + *destlen, tempBuf, resLen);
		*destlen += resLen;
	}

	if(scanP < len)
	{
		printf("broken frame\n");
		memcpy(dest + *destlen, buf + scanP, len - scanP);
		*destlen += (len - scanP);
	}

	free(tempBuf);
	return 0;
}

int init_CMD_CHAN()
{
	struct sockaddr_in pin;

	bzero(&pin,sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = inet_addr(server_ip_1);
	pin.sin_port = htons(server_cmd_port);

	if((cmdfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error opening socket \n");
		return -1;
	}

	if(connect(cmdfd, (void *)&pin, sizeof(pin)) == -1)
	{
		printf("Error connecting to socket \n");
		return -1;
	}

	return 0;
}

int close_CMD_CHAN()
{
	close(cmdfd);
}

int send_cmd(char *data, int len)
{
	int sendLen = 0;
	if(len < 0)
		return -1;
	sendLen = send(cmdfd, data, len, 0);
	if(sendLen == -1)
	{
		printf("Error in send\n");
		return -1;
	}

	return sendLen;
}

int recv_cmd(char *data, int len)
{
	int recvLen = 0;
	recvLen = recv(cmdfd, data, len, 0);
	if(recvLen == -1)
	{
		printf("Error in recv\n");
		return -1;
	}

	return recvLen;
}

int init_CONTROL_CHAN()
{
	struct sockaddr_in pin;

	bzero(&pin,sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = inet_addr(server_ip_1);
	pin.sin_port = htons(server_control_port);

	if((controlfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("control:Error opening socket \n");
		return -1;
	}

	if(connect(controlfd, (void *)&pin, sizeof(pin)) == -1)
	{
		printf("control:Error connecting to socket \n");
		return -1;
	}

	return 0;
}

int close_CONTROL_CHAN()
{
	close(controlfd);
}

int send_control(void *data, int len)
{
	int sendLen = 0;
	if(len < 0)
		return -1;
	sendLen = send(controlfd, data, len, 0);
	if(sendLen == -1)
	{
		printf("control:Error in send\n");
		return -1;
	}

	return sendLen;
}

int recv_control(void *data, int len)
{
	int recvLen = 0;
	recvLen = recv(controlfd, data, len, 0);
	if(recvLen == -1)
	{
		printf("control:Error in recv\n");
		return -1;
	}

	return recvLen;
}

void* controlChanThread(void *argc)
{
	controlChanThreadRunning = 1;

	pthread_detach(pthread_self());

	int recvLen = 0;
	unsigned char controlBuf[CONTROL_BUF_SIZE];
	int controlBufP = 0;

	while(controlSign == 1)
	{
		recvLen = recv_control(controlBuf, CONTROL_BUF_SIZE); 
		if(recvLen <= 0)
			continue;
		else
			controlBufP += recvLen;

		if(controlBufP >= sizeof(struct syn_head))
		{
			int scanP = 0;
			struct get_head get;

			while(scanP + sizeof(struct syn_head) <= controlBufP)
			{
				if(controlBuf[scanP] == 'G' && controlBuf[scanP + 1] == 'E' && controlBuf[scanP + 2] == 'T')
				{
		//			memcpy(&get, controlBuf + scanP, sizeof(struct get_head));
					
					if(controlBufP - scanP - sizeof(struct get_head) >= sizeof(u_int32_t))
					{
						unsigned int index = 0;
						index = ((controlBuf[scanP + 3]) | (controlBuf[scanP + 4]<<8) | (controlBuf[scanP + 5]<<16) | (controlBuf[scanP + 6]<<24));
						unreg_buff(index);
						scanP = scanP + sizeof(struct get_head) + sizeof(u_int32_t);
#if PRINT
						printf("get %d\n", index);
#endif
					}
					else 
						break;


				}
				else if(controlBuf[scanP] == 'S' && controlBuf[scanP + 1] == 'Y' && controlBuf[scanP + 2] == 'N')
				{
#if PRINT
					printf("syn\n");
#endif

#if PRINT_TIME
					struct timeval tvs, tve;
					gettimeofday(&tvs, NULL);
#endif
					pthread_mutex_lock(&synGetCount_lock);
					pthread_mutex_lock(&recvProcessBuf_lock);
					tidyInBuf(recvProcessBackBuf, recvProcessBackBufP, recvProcessBuf, &recvProcessBufP);
					pthread_mutex_unlock(&recvProcessBuf_lock);

					scanP_g = 0;
					recvProcessBackBufP = 0;

					recvPage++;
					sendSok();

#if PRINT_TIME
					gettimeofday(&tve, NULL);
					long delta = timeDelta(&tvs, &tve);
					printf("syn and sok use %d us\n", (int)delta);
#endif

#if PRINT
					printf("send Sok\n");
#endif
					pthread_mutex_unlock(&synGetCount_lock);

					if(analyseRecvDataRunning == 0)
					{
						analyseRecvDataRunning = 1;
						pthread_create(&analyseRecvData_t, NULL, analyseRecvData, NULL);
					}
					else
						printf("is already analysing!!!!!!!!\n");

					scanP = scanP + sizeof(struct syn_head);
				}
				else if(controlBuf[scanP] == 'S' && controlBuf[scanP + 1] == 'O' && controlBuf[scanP + 2] == 'K')
				{
#if PRINT
					printf("sok\n");
#endif
					page++;
					scanP = scanP + sizeof(struct sok_head);
				}
				else if(controlBuf[scanP] == 'W' && controlBuf[scanP + 1] == 'I' && controlBuf[scanP + 2] == 'N')
				{
					
					if(controlBufP - scanP - sizeof(struct win_head) >= sizeof(unsigned char))
					{
						windowLen = controlBuf[scanP + 3];

						scanP = scanP + sizeof(struct get_head) + sizeof(unsigned char);
#if PRINT
						printf("win %d\n", windowLen);
#endif
					}
					else 
						break;
				}
				else
					scanP++;
			}

			if(scanP == controlBufP)
			{
				controlBufP = 0;
				continue;
			}
			else if(scanP < controlBufP)
			{
				controlBufP -= scanP;
				memcpy(controlBuf, controlBuf + scanP, controlBufP);
			}

		}

	}

	controlChanThreadRunning = 0;
}

void* analyseRecvData(void *argc)
{
	analyseRecvDataRunning = 1;

	pthread_detach(pthread_self());
	int scanP = 0;

	pthread_mutex_lock(&recvProcessBuf_lock);
	if(recvProcessBufP >= sizeof(struct load_head))
	{

		while(scanP + sizeof(struct load_head) < recvProcessBufP)
		{
			if(recvProcessBuf[scanP] == 'J' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'A' && recvProcessBuf[scanP + 3] == 'N')
			{
				struct load_head head;

				memcpy(&head, recvProcessBuf + scanP, sizeof(struct load_head));

#if PRINT
				printf("load head: %c %d %d %d %d %d %d\n", head.logo[0], head.index, head.priority, (unsigned int)head.length, head.address, head.totalLen, head.sliceIndex);
#endif
				if(recvProcessBufP - scanP - sizeof(struct load_head) >= head.length)
				{

					if(head.length == 0)
					{
#if PRINT
						printf("Empty load!Actually lost.\n");
#endif
					}

					if(recvBufP + head.length > MAX_RECV_BUF)
					{
						printf("recv processed buf overflow!!\n");
						pthread_mutex_lock(&recvBuf_lock);
						recvBufP = 0;
						pthread_mutex_unlock(&recvBuf_lock);
					}

					if(head.sliceIndex == lastSliceIndex && head.address - lastAdd > lastHeadLen)
					{
						printf("lost case1\n");
						pthread_mutex_lock(&recvBuf_lock);
						memset(recvBuf + recvBufP, 0, head.address - lastAdd + lastHeadLen);
						recvBufP += (head.address - lastAdd + lastHeadLen);
						memcpy(recvBuf + recvBufP, recvProcessBuf + scanP + sizeof(struct load_head), head.length);
						recvBufP += head.length;
						pthread_mutex_unlock(&recvBuf_lock);
					}
					else if(lastSliceIndex - head.sliceIndex >= 1 && (head.address != 0 || lastTotalLen != lastAdd + lastHeadLen))
					{
						printf("lost case2\n");
						pthread_mutex_lock(&recvBuf_lock);
						memset(recvBuf + recvBufP, 0, head.address + lastTotalLen - lastAdd + lastHeadLen);
						recvBufP += (head.address + lastTotalLen - lastAdd + lastHeadLen);
						memcpy(recvBuf + recvBufP, recvProcessBuf + scanP + sizeof(struct load_head), head.length);
						recvBufP += head.length;
						pthread_mutex_unlock(&recvBuf_lock);
					}
					else
					{
						printf("normal case\n");
						pthread_mutex_lock(&recvBuf_lock);
						memcpy(recvBuf + recvBufP, recvProcessBuf + scanP + sizeof(struct load_head), head.length);
						recvBufP += head.length;
						pthread_mutex_unlock(&recvBuf_lock);
					}

					lastAdd = head.address;
					lastSliceIndex = head.sliceIndex;
					lastHeadLen = head.length;
					lastTotalLen = head.totalLen;

					scanP = scanP + sizeof(struct load_head) + head.length;

				}
				else
					break;
			}
			else
				scanP++;

		}

		if(scanP == recvProcessBufP)
		{
			recvProcessBufP = 0;
		}
		else if(scanP < recvProcessBufP)
		{
			recvProcessBufP -= scanP;
			memmove(recvProcessBuf, recvProcessBuf + scanP, recvProcessBufP);
		}


	}
	pthread_mutex_unlock(&recvProcessBuf_lock);

	analyseRecvDataRunning = 0;
}

void* recvData(void *argc)
{
	recvThreadRunning = 1;

	int recvLen = 0;
	int recv_size = 0;
	int err = 0;
	socklen_t optlen = 0;
	char *retryData;
	int pauseSign = 0;
	int recordLostNum = 0;
	int getLostNum = 0;
	unsigned int pauseIndex = -1;
	struct timeval tv;
    unsigned long lasttime = 0;
    unsigned long curtime = 0;
	struct load_head head;
	unsigned char * tempBuf = NULL;
	int tempBufLen = MTU*100;


	pthread_detach(pthread_self());

	recvBufP = 0;
	recvProcessBufP = 0;
	recvProcessBackBufP = 0;

	optlen = sizeof(recv_size); 
	err = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen); 
	if(err < 0)
	{ 
		printf("Fail to get recbuf size\n"); 
	} 

	initUnitList();

	tempBuf = (unsigned char *)malloc(tempBufLen);

	set_rec_timeout(0, 0);//(usec, sec)
	while(recvSign)
	{
		recvLen = 0;

		if(connectionStatus == P2P)
		{
			recvLen = recvfrom(sockfd, tempBuf, tempBufLen, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		}
		else if(connectionStatus == TURN)
		{
			recvLen = recvfrom(sockfd, tempBuf, tempBufLen, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		}
		else
		{	
			sleep(1);
			continue;
		}

		if(recvLen <= 0)
		{
			continue;
		}

		pthread_mutex_lock(&synGetCount_lock);

		if(recvProcessBackBufP > MAX_RECV_BUF - tempBufLen)
		{
			printf("recvBuf overflow!!\n");
			recvProcessBackBufP = 0;
		}

		memcpy(recvProcessBackBuf + recvProcessBackBufP, tempBuf, recvLen);

		getNum += recvLen;
		recvProcessBackBufP += recvLen;

		while(scanP_g + sizeof(struct load_head) < recvProcessBackBufP)
		{
			if(recvProcessBackBuf[scanP_g] == 'J' && recvProcessBackBuf[scanP_g + 1] == 'E' && recvProcessBackBuf[scanP_g + 2] == 'A' && recvProcessBackBuf[scanP_g + 3] == 'N')
			{
				memcpy(&head, recvProcessBackBuf + scanP_g, sizeof(struct load_head));
				if(head.priority > 0)
					sendGet(head.index);

				if(head.index/windowLen != recvPage)
				{
					recvProcessBackBufP -= recvLen;
#if PRINT
					printf("err: other page data mixed!!\n");
#endif
				}

				printf("index = %d\n", head.index);

				if(recvProcessBackBufP - scanP_g - sizeof(struct load_head) >= head.length)
					scanP_g = scanP_g + sizeof(struct load_head) + head.length;
				else
					break;
			}
			else
				scanP_g++;
		}
		pthread_mutex_unlock(&synGetCount_lock);

	}

	free(tempBuf);
	recvThreadRunning = 0;
}

int JEAN_init_slave(int setServerPort, int setLocalPort, char *setIp)
{
	int ret = 0;	
	int  i;
	char Ctl_Rec[50];
	char Rec_W;
	char Pole_ret = -1;

	recvSign = 1;
    recvBuf = (char*)malloc(MAX_RECV_BUF);
    recvProcessBuf = (char*)malloc(MAX_RECV_BUF);
    recvProcessBackBuf = (char*)malloc(MAX_RECV_BUF);

    initRing();	
	
    if (pthread_mutex_init(&recvBuf_lock, NULL) != 0) 
	{
		printf("mutex init error\n");
		return -1;
	}

    if (pthread_mutex_init(&synGetCount_lock, NULL) != 0) 
	{
		printf("mutex init error\n");
		return -1;
	}

    if (pthread_mutex_init(&recvProcessBuf_lock, NULL) != 0) 
	{
		printf("mutex init error\n");
		return -1;
	}

	ret = local_net_init(setLocalPort);
	if(ret < 0){
		printf("Local bind failed!!%d\n", ret);
		return ret;
	}

	ret = get_local_ip_port();
	if(ret < 0){
		printf("Geting local ip and port failed!!%d\n", ret);
		return ret;
	}

	ret = set_ip1_struct(setIp, setServerPort);
	if(ret < 0){
		printf("Set ip1 failed!!%d\n", ret);
		return ret;
	}

	printf("------------------- Connection and user name verifying ---------------------\n");
	for(i = 0; i < MAX_TRY; i++){
		Send_VUAPS();
		printf("Send uname and passwd\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		char result;
		result = Ctl_Rec[1];

		if(Ctl_Rec[0] == GET_REQ){
			printf("Receive ctl_w = %d result = %d\n", Rec_W, result);
			if(result == 4){
				printf("Verify and find node success!\n");
				break;
			}
			else if(result == 6){
				printf("Verify success but find node failed. Maybe node not exists!\n");
				return NO_NODE;
			}
			else if(result == 5){ 
				printf("Verify failed!\n");
				return WRONG_VERIFY;
			}
		}
	}

	if(i >= MAX_TRY) return OUT_TRY;

#if	KEEP_CONNECT_PACK
	ret = pthread_create(&keep_connection, NULL, Keep_con, NULL);
	if (ret != 0)
		printf("can't create thread: %s\n", strerror(ret));
#endif

	clean_rec_buff();
	printf("------------------ Request master IP!-------------------\n");

	for(i = 0; i < MAX_TRY; i++){
		memset(Ctl_Rec, 0, 50);
		Send_IP_REQ();
		printf("Send IP_REQ.\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		Rec_W = Ctl_Rec[0];

		if(Rec_W == RESP_M_IP){
			printf("Receive ctl_w = %d\n", Rec_W);
			memcpy(&master_sin, Ctl_Rec + 1, sizeof(struct sockaddr_in));
			printf("Get master IP: %s\n", inet_ntoa(master_sin.sin_addr));
			break;
		}

	}

	if(i >= MAX_TRY) return OUT_TRY;
	
	clean_rec_buff();
	printf("------------------ Establish connection!-------------------\n");
	for(i = 0; i < 2; i++)
		Send_POL(POL_REQ, &master_sin);

	clean_rec_buff();
	for(i = 0; i < MAX_TRY; i++){
		memset(Ctl_Rec, 0, 50);
		Send_POL(POL_SENT, &servaddr1);
		printf("Send POL_SENT.\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);

		//printf("OP = %d\n", Ctl_Rec[0]);

		if(Ctl_Rec[0] == GET_REQ){
			if(Ctl_Rec[1] == 0x0b){
				printf("Sever has got POL_SENT.\n");
				break;
			}
		}

	}

	if(i >= MAX_TRY) return OUT_TRY;

	while(Pole_ret == -1){
		set_rec_timeout(0, 0);//(usec, sec)
		memset(Ctl_Rec, 0, 50);
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		printf("OPCODE = %d\n", Ctl_Rec[0]);

		switch(Ctl_Rec[0]){
			case POL_REQ:
				Send_CMD_TO_MASTER(GET_REQ, 0x0a);
				printf("Receive GET_REQ\n");
				break;

			case CON_ESTAB:
				pole_res = Ctl_Rec[1];
//				commonKey = (Ctl_Rec[2] | (Ctl_Rec[3]<<8) | (Ctl_Rec[4]<<16) | (Ctl_Rec[5]<<24));
				Send_CMD(GET_REQ, 0x14);
//				printf("Pole result = %d, key = 0x%x.\n", pole_res, commonKey);
				Pole_ret = pole_res;
#ifndef TEST_TURN
				if(Pole_ret == 1)
					connectionStatus = P2P;
				else
#endif
				{
					int i = 0;
					connectionStatus = TURN;
					for(i = 0; i < MAX_TRY + 1 ; i++){
						printf("send turn \n");
						Send_TURN();
						char result = 0;

						recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
						if(Ctl_Rec[0] == GET_REQ) 
							break;
					}

					if(i >= MAX_TRY + 1) return OUT_TRY;

				}

				clean_rec_buff();
				for(i = 0; i < MAX_TRY + 1 ; i++){
					sleep(1);
					printf("require cmd channel open \n");
					Send_CMDOPEN();
					char result = 0;

					recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
					if(Ctl_Rec[0] == GET_REQ) 
						break;
				}

				if(i >= MAX_TRY + 1) return OUT_TRY;

				clean_rec_buff();
				for(i = 0; i < MAX_TRY + 1 ; i++){
					sleep(1);
					printf("require cmd channel open \n");
					Send_CONTROLOPEN();
					char result = 0;

					recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
					if(Ctl_Rec[0] == GET_REQ) 
						break;
				}

				if(i >= MAX_TRY + 1) return OUT_TRY;

				if(-1 == init_CONTROL_CHAN())
				{
					printf("control chan open failed\n");
					return -1;
				}

				controlSign = 1;

				if(controlChanThreadRunning == 0)
				{
					controlChanThreadRunning = 1;
					pthread_create(&control_t, NULL, controlChanThread, NULL);
				}

				if(recvThreadRunning == 0)
				{
					recvThreadRunning = 1;
					pthread_create(&recvDat_id, NULL, recvData, NULL);
				}

				break;

			case S_POL_REQ:
			printf("Get M_POL_REQ from server.\n");
			for(i = 0; i < MAX_TRY; i++){
				memset(Ctl_Rec, 0, 50);
				Send_POL(POL_REQ, &master_sin);
				printf("Send POL_REQ to master.\n");

				set_rec_timeout(0, 1);//(usec, sec)
				recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
				if(Ctl_Rec[0] == GET_REQ && Ctl_Rec[1] == 0x0a){
					printf("Pole ok! Connection established.\n");
					Send_CMD(GET_REQ, 0x10);
					break;
				}
			}

			if(i >= MAX_TRY){
				Send_CMD(GET_REQ, 0x11);
				printf("Pole failed! Requiring slave mode.");
			}

			break;	

		}
	}
	return 0;
}

int JEAN_send_slave(char *data, int len, unsigned char priority, unsigned char video_analyse, int video_head_len)
{
	int sendLen = 0;
    char *buffer;
	struct load_head lHead;

	buffer = (char *)malloc(len + sizeof(struct load_head));
	memcpy(lHead.logo, "JEAN", 4);
	lHead.index = sendIndex;
	lHead.priority = priority;
	lHead.length = len;
	lHead.direction = 0;

	memcpy(buffer, &lHead, sizeof(lHead));
	memcpy(buffer + sizeof(lHead), data, len);

#if TEST_LOST
	int rnd = 0;
	int lost_emt = LOST_PERCENT;
	rnd = rand()%100;
#ifdef LOST_PRINT
	if(rnd <= lost_emt)
	    printf("Lost!!: rnd = %d, lost_emt = %d, lost_posibility = %d percent\n", rnd, lost_emt, lost_emt);
#endif
	if(rnd > lost_emt)
	{
#endif

    if(connectionStatus == P2P)
	{
	    sendLen = sendto(sockfd, buffer, len + sizeof(lHead), 0, (struct sockaddr *)&master_sin, sizeof(struct sockaddr_in));
	}
	else if(connectionStatus == TURN)
	{
	    sendLen = sendto(sockfd, buffer, len + sizeof(lHead), 0, (struct sockaddr *)&turnaddr, sizeof(turnaddr));
	}
	else 
	{
		return NO_CONNECTION; 
	}

#if TEST_LOST
	}
#endif

	if(priority > 0)
		reg_buff(sendIndex, buffer, priority, len);
	sendIndex++;
    sendNum += sendLen;
    return sendLen;
}

int JEAN_recv_slave(char *data, int len, unsigned char priority, unsigned char video_analyse)
{
	int recvLen = 0;
    unsigned long int waitTime = 0;

	while(recvBufP == 0 && waitTime < JEAN_recv_timeout * 10)
	{
		usleep(10);
		waitTime += 10;
	}

	if(recvBufP == 0)
		return 0;

	pthread_mutex_lock(&recvBuf_lock);
	if(recvBufP < len)
	{
		memcpy(data, recvBuf, recvBufP);
		recvLen = recvBufP;
		recvBufP = 0;
	}
	else
	{
		memcpy(data, recvBuf, len);
		recvLen = len;
        memcpy(recvBuf, recvBuf + len, recvBufP - len);
		recvBufP -= len;
	}
	pthread_mutex_unlock(&recvBuf_lock);
    return recvLen;
}

int JEAN_close_slave()
{
	recvSign = 0;
	controlSign = 0;
	close_CONTROL_CHAN();
	close_CMD_CHAN();
	free(recvBuf);
	free(recvProcessBuf);
	free(recvProcessBackBuf);
	emptyRing();

	pthread_mutex_destroy(&recvBuf_lock);
	pthread_mutex_destroy(&synGetCount_lock);
	pthread_mutex_destroy(&recvProcessBuf_lock);
	close(sockfd);
	return 0;
}

