#ifndef R_CMD_H
#define R_CMD_H

#define RECV_BUFFER_SIZE 1024*1024
#define SERVER_IP "192.168.1.44"

enum STATUS_CONNECTION
{
	STATE_DISCONNECTED,
	STATE_REQ_SEND,
	STATE_RESP_RECEIVE,
	STATE_VERI_SEND,
	STATE_VERI_RECEIVE,
	STATE_CONNECTED
};

enum AV_OP_CODE
{
	Video_Data = 1,
	Audio_Data,
	Talk_Data,
	Resume,
	Stop,
	set_volume_Req
};

extern int AuNum[4];
extern int connectdeep;
extern int looseconnection;
extern int pingconnect;
extern char *bBuffer;
extern char *AVbBuffer;
extern char *bufInput;
extern int bufInputP;
extern char *AVbufInput;
extern int AVbufInputP;
extern int iHeaderLen;
extern int iVideoLinkID;
extern int iAudioLinkID;
extern int avfd;
extern pthread_t avtid;
extern char *bArrayImage;
extern char *bAudio;

extern int connected;
extern int iStatus;

void *receiveThread(void *agrc);
void *AVReceiver(void *argc);

#endif
