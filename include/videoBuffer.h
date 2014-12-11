#ifndef VIDEO_BUFFER_H
#define VIDEO_BUFFER_H

#define V_LIST_LEN 20

typedef struct vs_Buffer 
{
    unsigned char *data;
	long timestamp;
	unsigned int len;
	struct vs_Buffer *next;
} videoBuffer;

typedef struct
{
  videoBuffer *head;
  videoBuffer *tail;
  int length;
} videoList;

void initvideoList(videoList *list);
void clearvideoList(videoList *list);
int putvideoBuffer(videoList *list, unsigned char *dat, unsigned int len);
int getvideoBuffer(videoList *list, unsigned char *outdata, unsigned int *len);



#endif
