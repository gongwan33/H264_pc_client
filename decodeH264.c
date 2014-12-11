#include <math.h>

#include <cv.h>
#include <highgui.h>
#include <cxcore.h>

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <pthread.h>

#include <r_cmd.h>

#define FILE_READING_BUFFER (1*1024*1024)  

static unsigned char *buffer; 
static AVCodec *codec;  
static AVCodecContext *c= NULL;  
static int frame, got_picture, len;  
static AVFrame *picture;  
//static uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];  
static char buf[1024];  
static AVPacket avpkt;  
static AVFrame frameRGB;  

pthread_t h264tid;
pthread_mutex_t h264lock;
int isnew = 0;

//通过查找0x000001或者0x00000001找到下一个数据包的头部  
static int _find_head(unsigned char *buffer, int len)  
{  
    int i;  
   
    for(i = 512;i < len;i++)  
    {  
        if(buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+2] == 0 && buffer[i+3] == 1)  
            break;  
        if(buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+2] == 1)  
            break;  
    }  
    if (i == len)  
        return 0;
  
    if (i == 512)  
        return 0;

    return i;  
}  

//将文件中的一个数据包转换成AVPacket类型以便ffmpeg进行解码  
static void build_avpkt(AVPacket *avpkt, char *data, int dataLen)  
{  
    static int readptr = 0;  
    static int writeptr = 0;  
    int len,toread;  
   
    int nexthead;  
   
    if(writeptr- readptr < FILE_READING_BUFFER)  
    {  
        memmove(buffer, &buffer[readptr], writeptr - readptr);  
        writeptr -= readptr;  
        readptr = 0;  
        toread = FILE_READING_BUFFER - writeptr; 
	    len = dataLen;
	    memcpy(&buffer[writeptr], data, len);	
        writeptr += len;  
    }
	else
		printf("video buffer full!!!!\n");

    nexthead = _find_head(&buffer[readptr], writeptr-readptr);  
    if (nexthead == 0)  
    {  
        printf("failedfind next head...\n");  
        nexthead = writeptr - readptr;  
    }  
   
    avpkt->size = nexthead;  
    avpkt->data = &buffer[readptr];  
    readptr += nexthead; 
}

int initVideo()
{
	buffer = (unsigned char *)malloc(FILE_READING_BUFFER);
	if(buffer == NULL)
	{
		return -1;
	}
    avcodec_register_all();
    av_init_packet(&avpkt);  
   
    /* find the h264video decoder */  
    codec = avcodec_find_decoder(CODEC_ID_H264);  
    if (!codec){  
        printf("codecnot found\n");  
        return ;  
    }  
   
    c = avcodec_alloc_context3(codec);  
    picture= avcodec_alloc_frame();  
   
    if(codec->capabilities&CODEC_CAP_TRUNCATED)  
    c->flags|= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */  
   
    /* For somecodecs, such as msmpeg4 and mpeg4, width and height 
    MUST be initialized there because thisinformation is not 
    available in the bitstream. */  
   
    /* open it */  
    if(avcodec_open2(c, codec, NULL) < 0) {  
        printf("couldnot open codec\n");  
        exit(1);  
    }  
   

     //解码与显示需要的辅助的数据结构，需要注意的是，AVFrame必须经过alloc才能使用，不然其内存的缓存空间指针是空的，程序会崩溃  
    cvNamedWindow("decode", CV_WINDOW_AUTOSIZE);  
    frame = 0;  

	return 0;
}

void closeVideo()
{
	avcodec_close(c);  
    av_free(c);  
    av_free(picture);  

	free(buffer);
}


void video_decode(char *frameData, int len)
{

	build_avpkt(&avpkt, frameData, len);  

	if(avpkt.size == 0)  
		return;  

	while(avpkt.size > 0) {  
		len = avcodec_decode_video2(c,picture, &got_picture, &avpkt);//解码每一帧  

		IplImage *showImage = cvCreateImage(cvSize(picture->width, picture->height), 8, 3);  
		avpicture_alloc((AVPicture*)&frameRGB, PIX_FMT_RGB24, picture->width, picture->height);  

		if(len < 0) {  
			printf("Error while decoding frame %d\n",frame);  
			break;  
		}  
		if(got_picture) {  
			/* thepicture is allocated by the decoder. no need to free it */  
			//将YUV420格式的图像转换成RGB格式所需要的转换上下文  
			struct SwsContext * scxt = (struct SwsContext *)sws_getContext(picture->width, picture->height, PIX_FMT_YUV420P,  
					picture->width, picture->height, PIX_FMT_RGB24,  
					2,NULL,NULL,NULL);  
			if(scxt != NULL)  
			{  
				sws_scale(scxt, picture->data, picture->linesize, 0, c->height, frameRGB.data, frameRGB.linesize);//图像格式转换  
				showImage->imageSize = frameRGB.linesize[0];//指针赋值给要显示的图像  
				showImage->imageData = (char *)frameRGB.data[0];  
				cvShowImage("decode", showImage);//显示 
				cvWaitKey(50);//设置0.5s显示一帧，如果不设置由于这是个循环，会导致看不到显示出来的图像  
			}
			sws_freeContext(scxt);
			frame++;  
		}  
		avpkt.size -= len;  
		avpkt.data += len;  
	}  


}

void *videoThread(void *argc)
{
	initVideo();
	while(connected)
	{
		pthread_mutex_lock(&h264lock);
		if(isnew == 1)
		{
		    video_decode(bArrayImage, bArrayLen);
			isnew = 0;
		}
		else
			usleep(1000);
		pthread_mutex_unlock(&h264lock);
	}
	closeVideo();
}
