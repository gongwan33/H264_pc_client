#include <stdio.h>
#include <stdlib.h>
#include <blowfish.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <s_cmd.h>
#include <r_cmd.h>
#include <client.h>
#include <pthread.h>
#include <adpcm.h>
#include <playback.h>
#include <buffer.h>
#include <decodeH264.h>

#define BTS_BUFFER 100
//#define SAVE_VIDEO 

#ifdef SAVE_VIDEO
FILE *fp;
#endif

int AuNum[4];
int connectdeep = 0;
int looseconnection = 0;
int pingconnect = 0;
unsigned char *bBuffer = NULL;
unsigned char *AVbBuffer = NULL;
unsigned char *bufInput = NULL;
unsigned char *AVbufInput = NULL;
int bufInputP = 0;
int AVbufInputP = 0;
int iHeaderLen = 23;
int iVideoLinkID = 0;
int iAudioLinkID = 0;
int avTimeStamp = 0;
pthread_t avtid;
unsigned char *bArrayImage = NULL;
int bArrayLen = 0;
List audioList;

long byteArrayToLong(unsigned char *inByteArray, int iOffset, int iLen) {
	long iResult = 0;
    int x = 0;
	for (x = 0; x < iLen; x++) {
		if ((x == 0) && (inByteArray[iOffset + (iLen - 1) - x] < 0))
			iResult = iResult
				| (0xffffffff & inByteArray[iOffset + (iLen - 1) - x]);
		else
			iResult = iResult
				| (0x000000ff & inByteArray[iOffset + (iLen - 1) - x]);
		if (x < (iLen - 1))
			iResult = iResult << 8;
	}

	return iResult;
}


int byteArrayToInt(unsigned char *b, int offset) 
{
	return (b[3 + offset] << 24) 
		+ ((b[2 + offset] & 0xFF) << 16)
		+ ((b[1 + offset] & 0xFF) << 8)
		+ (b[0 + offset] & 0xFF);
}

int byteArrayToIntLen(unsigned char *inByteArray, int iOffset, int iLen)
{
	int iResult = 0;
	int x = 0;

	for (x = 0; x < iLen; x++)
	{
		if ((x == 0) && (inByteArray[iOffset + (iLen - 1) - x] < 0))
			iResult = iResult | (0xffffffff & inByteArray[iOffset + (iLen - 1) - x]);
		else
			iResult = iResult | (0x000000ff & inByteArray[iOffset + (iLen - 1) - x]);
		if (x < (iLen - 1))
			iResult = iResult << 8;
	}

	return iResult;
}

char *byteArrayToString(unsigned char *inByteArray, int iOffset, int iLen)
{
	int iResult = 0;
    char ch[iLen];
    int x = 0;
    char *bToS = NULL;

	bToS = (unsigned char *)calloc(BTS_BUFFER, sizeof(unsigned char));
	if(bToS == NULL)
	{
		printf("Memory error!\n");
		return NULL;
	}
	for (x = 0; x < iLen; x++) {
		ch[x] = inByteArray[x + iOffset];
	}

	if(iLen < BTS_BUFFER)
	{
		memcpy(bToS, ch, iLen);
        bToS[iLen] = '\0';
	}
	else
	{
		memcpy(bToS, ch, BTS_BUFFER);
		bToS[BTS_BUFFER] = '\0';
	}

	return bToS;
}

int videoEnable()
{
	printf("video start====>\n");
	sendCommand(Video_Start_Req);
}

int audioEnable()
{
    printf("audio start====>\n");
	sendCommand(Audio_Start_Req);
}

int startAVReceive(int *fd)
{
	struct sockaddr_in s_add,c_add;
	unsigned short portnum=80;

   	*fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == *fd)
	{
		printf("avfd: socket fail ! \r\n");
		return -1;
	}
	printf("avfd: socket ok !\r\n");

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr(SERVER_IP);
	s_add.sin_port=htons(portnum);

	if(-1 == connect(*fd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
		printf("avfd: connect fail !\r\n");
		return -1;
	}

    pthread_create(&avtid, NULL, AVReceiver, NULL);

    return 0;
}

void Parse_Packet(int inCode, char *inPacket, int len)
{
    int Video_Start_Resp_iResult = 0;
	int Video_Start_Resp_iLinkID = 0;
    int Audio_Start_Resp_iResult = 0;
    int Audio_Start_Resp_iLinkID = 0;

    switch (inCode)
	{
		case Login_Resp:
			printf("==============Login_Resp====================\n");
			int Login_Resp_iResult = byteArrayToIntLen(inPacket, 0, 2);
			if (Login_Resp_iResult == 0)
			{
				//Result == OK
					if (STATE_RESP_RECEIVE > iStatus)
					{
						iStatus = STATE_RESP_RECEIVE;
					}
				// Get ID
				char chCameraID[16];
				int x;
				for (x = 0; x < 16; x++)
					chCameraID[x] = (char) inPacket[2 + x];

/*				
 				// Get firmware version
				int iCameraVer[4];
				for (int x = 0; x < 4; x++)
				{
					iCameraVer[x] = byteArrayToInt(inPacket, 26 + x, 1);
				}
				targetCameraVer = iCameraVer[0] + "." + iCameraVer[1] + "." + iCameraVer[2] + "." + iCameraVer[3];
*/
				char byteAuNum[4];
				int y = 0;

				for (y = 0; y < 4; y++)
				{
					for (x = 0; x < 4; x++)
					{
						byteAuNum[x] = inPacket[30 + y * 4 + x];
					}
					AuNum[y] = byteArrayToIntLen(byteAuNum, 0, 4);
				}
				// verify
				for (x = 0; x < 4; x++)
				{
					if (AuNum[x] == ChNum[x])
					{
						continue;
					} else
					{
						// error !!!
						printf("Login_Resp:Error !!! AuNum[%d]=%d, ChNum[%d]=%d\n", x, AuNum[x], x, ChNum[x]);
						break;
					}
				}
				for (x = 0; x < 4; x++)
				{
					AuNum[x] = byteArrayToIntLen(inPacket, 46 + x * 4, 4);
					// Log.e("AuNum", "AuNum["+x+"]is "+AuNum[x]);
				}
			} 
			else if (Login_Resp_iResult == 2)
			{
				// MAX connections
			}
			sendCommand(Verify_Req);
			break;

		case Verify_Resp:
			printf( "=======Verify_Resp=========\n");
			int Verify_Resp_iResult = byteArrayToIntLen(inPacket, 0, 2);
			if (Verify_Resp_iResult == 0)
			{
				// Result = OK
				if (STATE_VERI_RECEIVE > iStatus)
				{
					iStatus = STATE_VERI_RECEIVE;
				}
				videoEnable();
			} 
			else if (Verify_Resp_iResult == 1)
			{
				// Result = user error
				printf("Verify_Resp Result = user error\n");
			} 
			else if (Verify_Resp_iResult == 5)
			{
				// password error
				printf("Verify_Resp Result = password error\n");
			}
		   	else
			{
				// unknown error
				printf("Verify_Resp Result = unknown error\n");
			}
			break;

		case Alive_Resp:
			looseconnection = 0;
			pingconnect = 1;
			break;

		case Video_Start_Resp:
			Video_Start_Resp_iResult = byteArrayToIntLen(inPacket, 0, 2);
			Video_Start_Resp_iLinkID = byteArrayToIntLen(inPacket, 2, 4);
			// "=======video start resp========="+Video_Start_Resp_iLinkID);
			if (Video_Start_Resp_iResult == 0)
			{
				// result = agree
				iVideoLinkID = Video_Start_Resp_iLinkID;
			} else if (Video_Start_Resp_iResult == 2)
			{
				// MAX
			} else
			{
				// unknown
			}
	        printf("iVideoLinkID = %d\n", iVideoLinkID);
			if(iVideoLinkID != 0)
			{
				if(0 != startAVReceive(&avfd))
					printf("AVReceiver start failed!\n");
			}
			else
				printf("VideoLinkID is incorrect!\n");

	        audioEnable();
#ifndef SAVE_VIDEO
			pthread_create(&h264tid, NULL, videoThread, NULL);
#endif

#ifdef SAVE_VIDEO
		    if( (fp = fopen("RecordH264.h264","a+")) == NULL)
			    return 0;	  
#endif
			break;

		case Audio_Start_Resp:
			// Log.e(TAG, "=======audio start resp=========");
			Audio_Start_Resp_iResult = byteArrayToIntLen(inPacket, 0, 2);
			// Parse ID only if the length over 6 bytes
			Audio_Start_Resp_iLinkID = 0;
			if (len == 6)
				Audio_Start_Resp_iLinkID = byteArrayToIntLen(inPacket, 2, 4);
			if (Audio_Start_Resp_iResult == 0)
			{
				// result = agree
				// If link ID is 0, use existed Video ID
				if (Audio_Start_Resp_iLinkID == 0)
					iAudioLinkID = iVideoLinkID;
				else
					iAudioLinkID = Audio_Start_Resp_iLinkID;
			} else if (Audio_Start_Resp_iResult == 2)
			{
				// MAX
			} else
			{
				// unknown
			}

			initList(&audioList);
			pthread_create(&playtid, NULL, playThread, NULL);

			break;

		default:
			printf("Parse_Packet Unsported Command !!!!  inCode is %d\n", inCode);
			break;
	}
}


void *receiveThread(void *argc)
{
	struct timeval tv;
	struct timezone tz;
	int lastcmdtime1 = 0;
	int curTime = 0;

	bBuffer = (unsigned char *)calloc(RECV_BUFFER_SIZE, sizeof(unsigned char));
	bufInput = (unsigned char *)calloc(RECV_BUFFER_SIZE * 2, sizeof(unsigned char));
	bufInputP = 0;

	if(bufInput == NULL || bBuffer == NULL)
	{
		if(bufInput != NULL)
			free(bufInput);
		if(bBuffer != NULL)
			free(bBuffer);

		printf("ERROR: MEMORY ERROR!\n");
		return;
	}

    gettimeofday(&tv, &tz);
	lastcmdtime1 = (tv.tv_sec*1000000 + tv.tv_usec)/1000;

	while (connected)
	{
		// Check if it does not send any command for 60s. If yes, send a
		// keep alive command.
        gettimeofday(&tv, &tz);
	    curTime = (tv.tv_sec*1000000 + tv.tv_usec)/1000;
		if ((curTime - lastcmdtime1) > 800 && connectdeep == 1)
		{
			lastcmdtime1 = curTime;

			sendCommand(Keep_Alive);
			if (pingconnect == 0)
			{
				looseconnection++;
				if (looseconnection >= 10)
				{
					looseconnection = 0;
					return;
				}
			} else
			{
				pingconnect = 0;
				looseconnection = 0;
			}
		}

		// Receive packet

		int iReadLen = recv(cfd, bBuffer, RECV_BUFFER_SIZE, 0);
		int x = 0;

		if(iReadLen + bufInputP <= RECV_BUFFER_SIZE*2)
		{
			memcpy(bufInput + bufInputP, bBuffer, iReadLen);
			bufInputP += iReadLen;
		}

		if (iReadLen >= iHeaderLen)
		{
			// Get a basic header
		    char bHeader[iHeaderLen];
			for (x = 0; x < iHeaderLen; x++)
				bHeader[x] = bufInput[x];
			// Parse header
			char *header = byteArrayToString(bHeader, 0, 4);
			int iOpCode = byteArrayToIntLen(bHeader, 4, 2);
			int iContentLen = byteArrayToIntLen(bHeader, 15, 4);

			if ((bufInputP >= (iHeaderLen + iContentLen)) && strcmp(header, "MO_O") == 0)
			{
				char bPacket[iContentLen];
				int i = 0;
				memcpy(bPacket, bufInput + iHeaderLen, iContentLen);

				for(i = 0; i < bufInputP - iHeaderLen - iContentLen; i++)
					bufInput[i] = bufInput[iHeaderLen + iContentLen + i];

				bufInputP = bufInputP - iHeaderLen - iContentLen;

				Parse_Packet(iOpCode, bPacket, iContentLen);
			}

			free(header);
		}
		else
			usleep(500000);
	}

	free(bBuffer);
	free(bufInput);
}

void Parse_AVPacket(int inCode, char *inPacket, int headOffset)
{
    int Audio_Data_iAudioLen = 0;
	adpcm_state_t adpcmState;
	short *decodeBuffer = NULL;
	switch (inCode)
	{
		case Video_Data:
			connectdeep = 1;
			int Video_Data_iVideoLen = byteArrayToIntLen(inPacket, headOffset + 9, 4);
            if(Video_Data_iVideoLen <= 0)
				break;

			printf("recv video data, %d\n", Video_Data_iVideoLen);

			int timestamp = byteArrayToIntLen(inPacket, 0, 4);
			int frametime = byteArrayToIntLen(inPacket, headOffset + 4, 4);
			int preserve = byteArrayToIntLen(inPacket, headOffset + 8, 1);

			pthread_mutex_lock(&h264lock);

			bArrayImage = (char *)malloc(Video_Data_iVideoLen);
			memcpy(bArrayImage, inPacket + headOffset + 13,  Video_Data_iVideoLen);
			bArrayLen = Video_Data_iVideoLen;

#ifdef SAVE_VIDEO
			fwrite(inPacket + headOffset + 13, Video_Data_iVideoLen, 1, fp);
#endif
			isnew = 1;

			pthread_mutex_unlock(&h264lock);

			break;

		case Audio_Data:
			Audio_Data_iAudioLen = byteArrayToIntLen(inPacket, headOffset + 13, 4); 
			if(Audio_Data_iAudioLen <= 0)
				break;

			printf("recv audio data, %d\n", Audio_Data_iAudioLen);
            
			decodeBuffer = (short *)malloc(Audio_Data_iAudioLen * 4);

			int Audio_Data_paraSample = byteArrayToIntLen(inPacket, headOffset + 17 + Audio_Data_iAudioLen, 2);
			int Audio_Data_paraIndex = byteArrayToIntLen(inPacket, headOffset + 17 + Audio_Data_iAudioLen + 2, 1);

			int packetSeq = byteArrayToIntLen(inPacket, headOffset + 4, 4);
			int graspstamp = byteArrayToIntLen(inPacket, headOffset + 8, 4);
			int format = byteArrayToIntLen(inPacket, headOffset + 12, 1);

			adpcmState.valprev = (short) Audio_Data_paraSample;
			adpcmState.index = (char) Audio_Data_paraIndex;

		    adpcm_decoder(inPacket + headOffset + 17, decodeBuffer, Audio_Data_iAudioLen, &adpcmState);

			putBuffer(&audioList, (char*)decodeBuffer);
			free(decodeBuffer);

			break;
	}
}


void *AVReceiver(void *argc)
{
	int x = 0;
	AVbufInput = (unsigned char *)calloc(RECV_BUFFER_SIZE * 2, sizeof(unsigned char));
	AVbufInputP = 0;
	AVbBuffer = (unsigned char *)calloc(RECV_BUFFER_SIZE, sizeof(unsigned char));
	while (connected)
	{
		unsigned int iReadLen = recv(avfd, AVbBuffer, RECV_BUFFER_SIZE, 0);
		if(AVbufInputP + iReadLen <= RECV_BUFFER_SIZE * 2)
		{
			memcpy(AVbufInput + AVbufInputP, AVbBuffer, iReadLen);
			AVbufInputP += iReadLen;
		}

		//        printf("read AVpack, %d\n", iReadLen);

		//header scanner
		unsigned int skip_len = 0;
		unsigned int p = 0;

		if(AVbufInputP > iHeaderLen)
		{

			while((p < AVbufInputP - 3) && (AVbufInput[p] != 'M' || AVbufInput[p+1] != 'O' || AVbufInput[p+2] != '_' || AVbufInput[p+3] != 'V'))
				p++;

			if(p >= AVbufInputP - 3)
			{
				for(x = 0; x < 3; x++)
					AVbufInput[x] = AVbufInput[AVbufInputP - 3 + x];
				AVbufInputP = 3;
				continue;
			}
			else
			{
				AVbufInputP -= p;  
				for(x = 0; x < AVbufInputP; x++)
					AVbufInput[x] = AVbufInput[p + x];
			}

			while (AVbufInputP > iHeaderLen)
			{
				// Get a basic header
				unsigned char bHeader[iHeaderLen];
				int x = 0;
				for (x = 0; x < iHeaderLen; x++)
					bHeader[x] = AVbufInput[skip_len + x];

				unsigned int iOpCode = byteArrayToIntLen(bHeader, 4, 2);
				unsigned int iContentLen = byteArrayToIntLen(bHeader, 15, 4);

				if(iContentLen < 0)
				{
					skip_len += iHeaderLen;
					break;
				}

				if ((AVbufInputP >= (skip_len + iHeaderLen + iContentLen)))
				{
					avTimeStamp = byteArrayToLong(AVbufInput, skip_len + iHeaderLen, 4);
					Parse_AVPacket(iOpCode, AVbufInput, skip_len + iHeaderLen);
					skip_len += iContentLen + iHeaderLen;
				} 
				else
				{
					break;
				}

			}

			for(x = 0; x < AVbufInputP - skip_len; x++)
				AVbufInput[x] = AVbufInput[skip_len + x];
			AVbufInputP = AVbufInputP - skip_len;
		}
		else
			usleep(800);
	}

#ifdef SAVE_VIDEO
	fflush(fp);
	fclose(fp);
#endif
	free(AVbBuffer);
	free(AVbufInput);
}

