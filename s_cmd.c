#include <stdio.h>
#include <stdlib.h>
#include <blowfish.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <s_cmd.h>
#include <client.h>
#include <r_cmd.h>

int ChNum[] =	{ 1, 2, 3, 4 };
char bOut[50];
char int32Arr[4];
char fps = 5;

char* int32ToByteArray(int num)
{
	int i = 0;
	for (i = 0; i < 4; i++)
	{
		int offset = i * 8;
		int32Arr[i] = ((num >> offset) & 0xFF);
	}
	return int32Arr;
}	

int sendCommand(int inCommand, int cfd)
{
	int bRet = -2;
	char szHeader[5] = "MO_O";
	int cX = 0;
	int length = 0;
	int writebytes = 0;

	if(connected != 1)
		return -1;

	switch (inCommand)
	{
		case Change_Fps:
			// 0 ~ 3 is "MO_O"
			memcpy(bOut, szHeader, 4);
			length = 4;
			// 4 ~ 7 is command
			memcpy(bOut + length, int32ToByteArray((int)Change_Fps), 4);
			length = 8;
			// 8 ~ 14 set to 0
			memset(bOut + length, 0, 7);
			length = 15;
			// 15 ~ 18 is the length = 1
			memcpy(bOut + length, int32ToByteArray(1), 4);
			length = 19;
			// 19 ~ 22 set to 0
			memset(bOut + length, 0, 4);
			length = 23;
			// 23 set to reserve = 1
			memset(bOut + length, fps, 1);
			length = 24;
			break;
		case Login_Req:
			// 0 ~ 3 is "MO_O"
			// Log.e("send login_req","");
			memcpy(bOut, szHeader, 4);
			length = 4;
			// 4 ~ 7 is command
			memcpy(bOut + length, int32ToByteArray((int)Login_Req), 4);
			length = 8;
			// 8 ~ 14 set to 0
			memset(bOut + length, 0, 7);
			length = 15;
			// 15 ~ 18 is the length = 16
			memcpy(bOut + length, int32ToByteArray(16), 4);
			length = 19;

			memset(bOut + length, 0, 4);
			length = 23;

			for (cX = 0; cX < 4; cX++) {
				// send 4 ch_num
				memcpy(bOut + length, int32ToByteArray(ChNum[cX]), sizeof(int));
				length = length + 4;
			}

			BlowfishEncrption((unsigned long *)ChNum, sizeof(ChNum)/sizeof(int));
			break;

		case Verify_Req:
			// 0 ~ 3 is "MO_O"
			memcpy(bOut, szHeader, 4);
			length = 4;
			// 4 ~ 7 is command
			memcpy(bOut + length, int32ToByteArray((int)Verify_Req), 4);
			length = 8;

			// 8 ~ 14 set to 0
			memset(bOut + length, 0, 7);
			length = 15;

			// 15 ~ 18 is the length = 16
			memcpy(bOut + length, int32ToByteArray(16), 4);
			length = 19;

			// 19 ~ 22 set to 0
			memset(bOut + length, 0, 4);
			length = 23;

			BlowfishEncrption((unsigned long *)AuNum, sizeof(AuNum)/sizeof(int));
			for (cX = 0; cX < 4; cX++)
			{
				memcpy(bOut + length, int32ToByteArray(AuNum[cX]), sizeof(int));
				length = length + 4;
			}
			break;

		case Video_Start_Req:
			// 0 ~ 3 is "MO_O"
			memcpy(bOut, szHeader, 4);
			length = 4;
			// 4 ~ 7 is command
			memcpy(bOut + length, int32ToByteArray((int)Video_Start_Req), 4);
			length = 8;

			// 8 ~ 14 set to 0
			memset(bOut + length, 0, 7);
			length = 15;

			// 15 ~ 18 is the length = 1
			memcpy(bOut + length, int32ToByteArray(1), 4);
			length = 19;

			// 19 ~ 22 set to 0
			memset(bOut + length, 0, 4);
			length = 23;

			// 23 set to reserve = 1
			*(bOut + length) = 1;
			length = 24;
			break;

/*		case CMD_OP_CODE.Video_End:
			// 0 ~ 3 is "MO_O"
			bOut.write(szHeader.getBytes(), 0, 4);
			// 4 ~ 7 is command
			try
			{
				bOut.write(int32ToByteArray(CMD_OP_CODE.Video_End));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 8 ~ 14 set to 0
			for (cX = 8; cX < 15; cX++)
				bOut.write(0x00);
			// 15 ~ 18 is the length = 0
			try
			{
				bOut.write(int32ToByteArray(0));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 19 ~ 22 set to 0
			for (cX = 19; cX < 23; cX++)
				bOut.write(0x00);
			break;

		case CMD_OP_CODE.Audio_Start_Req:
			audiosending = true;
			// 0 ~ 3 is "MO_O"
			bOut.write(szHeader.getBytes(), 0, 4);
			// 4 ~ 7 is command
			try
			{
				bOut.write(int32ToByteArray(CMD_OP_CODE.Audio_Start_Req));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 8 ~ 14 set to 0
			for (cX = 8; cX < 15; cX++)
				bOut.write(0x00);
			// 15 ~ 18 is the length = 1
			try
			{
				bOut.write(int32ToByteArray(1));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 19 ~ 22 set to 0
			for (cX = 19; cX < 23; cX++)
				bOut.write(0x00);
			// 23 set to reserve = 1
			try
			{
				bOut.write(int8ToByteArray(1));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			break;

		case CMD_OP_CODE.Audio_End:
			audiosending = false;
			try
			{
				// 0 ~ 3 is "MO_O"
				bOut.write(szHeader.getBytes(), 0, 4);
				// 4 ~ 7 is command
				bOut.write(int32ToByteArray(CMD_OP_CODE.Audio_End));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 8 ~ 14 set to 0
			for (cX = 8; cX < 15; cX++)
				bOut.write(0x00);
			// 15 ~ 18 is the length = 0
			try
			{
				bOut.write(int32ToByteArray(0));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 19 ~ 22 set to 0
			for (cX = 19; cX < 23; cX++)
				bOut.write(0x00);
			break;

		case CMD_OP_CODE.Camera_Params_Set_Req:
			try
			{
				// 0 ~ 3 is "MO_O"
				bOut.write(szHeader.getBytes(), 0, 4);
				// 4 ~ 7 is command
				bOut.write(int32ToByteArray(CMD_OP_CODE.Camera_Params_Set_Req));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 8 ~ 14 set to 0
			for (cX = 8; cX < 15; cX++)
				bOut.write(0x00);
			// 15 ~ 18 is the length = 2
			try
			{
				bOut.write(int32ToByteArray(2));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 19 ~ 22 set to 0
			for (cX = 19; cX < 23; cX++)
				bOut.write(0x00);
			// 23 set to Type = 0
			try
			{
				bOut.write(int8ToByteArray(0));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 24 set to Value = 8
			try
			{
				bOut.write(int8ToByteArray(8));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			break;

		case CMD_OP_CODE.Decoder_Control_Req:

			break;

		case CMD_OP_CODE.Keep_Alive:
			// 0 ~ 3 is "MO_O"
			bOut.write(szHeader.getBytes(), 0, 4);
			// 4 ~ 7 is command
			try
			{
				bOut.write(int32ToByteArray(CMD_OP_CODE.Keep_Alive));
			} catch (IOException e)
			{
				e.printStackTrace();
			}
			// 8 ~ 22 set to 0
			for (cX = 8; cX < 23; cX++)
				bOut.write(0x00);
			break;
			*/
		default:
			printf("opcode erro: NOT FOUND\n");
	}

	if (length > 0)
	{
		if(-1 == (writebytes = send(cfd, bOut, length, 0)))
		{
			printf("write data fail !\r\n");
			bRet = -2;
			return bRet;
		}

		printf("writebytes = %d\r\n", writebytes);

		bRet = 0;
	}

	return bRet;
}

