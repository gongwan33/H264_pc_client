#ifndef R_CMD_H
#define R_CMD_H

#define RECV_BUFFER_SIZE 1024*1024
#define SERVER_IP "192.168.1.44"
#define AUDIO_BUFFER_SIZE 1024*20

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

struct pcmAudio
{
    char *data;
	int len;
};

extern int AuNum[4];
extern int connectdeep;
extern int looseconnection;
extern int pingconnect;
extern unsigned char *bBuffer;
extern unsigned char *AVbBuffer;
extern unsigned char *bufInput;
extern int bufInputP;
extern unsigned char *AVbufInput;
extern int AVbufInputP;
extern int iHeaderLen;
extern int iVideoLinkID;
extern int iAudioLinkID;
extern int avfd;
extern pthread_t avtid;
extern unsigned char *bArrayImage;
extern struct pcmAudio bAudio;

extern int connected;
extern int iStatus;
extern int cfd;

void *receiveThread(void *agrc);
void *AVReceiver(void *argc);

#endif
