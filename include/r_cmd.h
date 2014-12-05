#ifndef R_CMD_H
#define R_CMD_H

#define RECV_BUFFER_SIZE 1024*1024

enum STATUS_CONNECTION
{
	STATE_DISCONNECTED,
	STATE_REQ_SEND,
	STATE_RESP_RECEIVE,
	STATE_VERI_SEND,
	STATE_VERI_RECEIVE,
	STATE_CONNECTED
};

extern int AuNum[4];
extern int connectdeep;
extern int looseconnection;
extern int pingconnect;
extern char *bBuffer;
extern char *bufInput;
extern int bufInputP;
extern int iHeaderLen;
extern int iVideoLinkID;
extern int iAudioLinkID;

extern int connected;
extern int iStatus;

void *receiveThread(void *agrc);

#endif
