#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <videoBuffer.h>

pthread_mutex_t videoListLock;

void initVideoList(videoList *list)
{
	list->head = NULL;
    list->tail = NULL;
    list->length = 0;	
}

void clearVideoList(videoList *list)
{
	videoBuffer *p = NULL;
	pthread_mutex_lock(&videoListLock);
    for(p = list->head; p != NULL; )
	{
		videoBuffer *buf = p;
		p = p->next;
		free(buf->data);
		free(buf);
	}
	list->length = 0;
	pthread_mutex_unlock(&videoListLock);
}

int putVideoBuffer(videoList *list, unsigned char *dat, unsigned int len)
{
    if(list->length >= V_LIST_LEN)
	{
	    printf("buffer.c: length not enough overwrite!\n");
		clearVideoList(list); 
	}

	videoBuffer *buf = (videoBuffer *)malloc(sizeof(videoBuffer));
	if(buf == NULL)
	{
		printf("Buffer.c: Memory not enough\n");
		return -1;
	}
	
    buf->next = NULL;
	buf->data = (unsigned char *)malloc(len);
	buf->len = len;
	memcpy(buf->data, dat, len);
	buf->timestamp = 0;
	
	pthread_mutex_lock(&videoListLock);
	if(list->tail != NULL)
		list->tail->next = buf;
	list->tail = buf;
	if(list->length == 0)
		list->head = buf;
	list->length++;
	pthread_mutex_unlock(&videoListLock);
   return 0; 
}

int getVideoBuffer(videoList *list, unsigned char *outdata, unsigned int *len)
{
    if(list->length <= 0)
		return -1;
     
	pthread_mutex_lock(&videoListLock);
	if(list->head != NULL)
	{
        memcpy(outdata, list->head->data, list->head->len);
		*len = list->head->len;

		if(list->head != list->tail)
		{
			videoBuffer *temp = list->head;
		    list->head = list->head->next;
			list->length--;
			free(temp->data);
			free(temp);
		}
		else
		{
			free(list->head->data);
			free(list->head);
			list->head = NULL;
			list->tail = NULL;
			list->length = 0;
		}
	}
	else
	{
	    pthread_mutex_unlock(&videoListLock);
		return -1;
	}
	pthread_mutex_unlock(&videoListLock);
    return 0;
}


