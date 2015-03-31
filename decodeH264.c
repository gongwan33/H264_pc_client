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
#include <videoBuffer.h>

#define FILE_READING_BUFFER (500*1024)  

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


//将文件中的一个数据包转换成AVPacket类型以便ffmpeg进行解码  
static int build_avpkt(AVPacket *avpkt)
{  
	int len;
    if(-1 == getVideoBuffer(&vList, buffer, &len))
		return -1;  
    avpkt->size = len;  
    avpkt->data = buffer;   
	return 0;
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


void video_decode()
{

	if(-1 == build_avpkt(&avpkt))
	{
		usleep(10000);
		return;  
	}

	if(avpkt.size == 0)  
		return;  

	while(avpkt.size > 0) {  
		len = avcodec_decode_video2(c,picture, &got_picture, &avpkt);//解码每一帧  

		printf("ffmpeg: len = %d got_picture = %d\n", len, got_picture);
		IplImage *showImage = cvCreateImage(cvSize(picture->width, picture->height), 8, 3);  
		avpicture_alloc((AVPicture *)&frameRGB, PIX_FMT_BGR24, picture->width, picture->height);  

		if(len < 0) {  
			printf("Error while decoding frame %d\n",frame);  
			break;  
		}  
		if(got_picture) {  
			/* thepicture is allocated by the decoder. no need to free it */  
			//将YUV420格式的图像转换成RGB格式所需要的转换上下文  
			struct SwsContext * scxt = (struct SwsContext *)sws_getContext(picture->width, picture->height, PIX_FMT_YUV420P,  
					picture->width, picture->height, PIX_FMT_BGR24, 2,NULL,NULL,NULL);  
			if(scxt != NULL)  
			{  
				sws_scale(scxt, picture->data, picture->linesize, 0, c->height, frameRGB.data, frameRGB.linesize);//图像格式转换  
				showImage->imageSize = frameRGB.linesize[0];//指针赋值给要显示的图像  
				showImage->imageData = (char *)frameRGB.data[0];  
				cvShowImage("decode", showImage);//显示 
				cvWaitKey(40);//设置显示一帧，如果不设置由于这是个循环，会导致看不到显示出来的图像  
			}
			avpicture_free((AVPicture *)&frameRGB);
			cvReleaseImage(&showImage);
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
	    video_decode();
	closeVideo();
}
