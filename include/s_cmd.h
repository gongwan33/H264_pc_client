#ifndef S_CMD_H
#define S_CMD_H

enum op{
	Login_Req = 0,
	Login_Resp,
    Verify_Req,
    Verify_Resp,
    Video_Start_Req, 
    Video_Start_Resp,
    Video_End,
    Change_Fps,
    Audio_Start_Req,
    Audio_Start_Resp,
    Audio_End,
    Talk_Start_Req,
    Talk_Start_Resp,
    Talk_End,
    Decoder_Control_Req,
    Camera_Params_Set_Req = 19,
    voice_conrol = 23,
    Device_Control_Req = 250,
    Alive_Resp = 254,
    Keep_Alive
} CMD_OP_CODE;

int sendCommand(int cmd);

extern char fps;
extern int ChNum[4];
extern int cfd;


#endif
