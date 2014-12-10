#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include <pthread.h>

struct List bAudio;
int listP = 0;
int firstP = 0;
static pthread_mutex_t audioBufLock;

char *getItem(void)
{
	pthread_mutex_lock(&audioBufLock);
	if(listP > 0 && listP > firstP)
	{
		char *item = NULL;
        item =  bAudio.data + (firstP % LIST_SPACE/ITEM_SIZE) * ITEM_SIZE;
        firstP++;
		if(firstP == listP)
		{
			firstP = 0;
			listP = 0;
		}
	    pthread_mutex_unlock(&audioBufLock);
		return item;
	}
	else
	{
	    pthread_mutex_unlock(&audioBufLock);
		return NULL;
	}
}

void addItem(char *item)
{
	pthread_mutex_lock(&audioBufLock);
	if(item != NULL)
	{
		memcpy(bAudio.data + (listP % (LIST_SPACE/ITEM_SIZE)) * ITEM_SIZE, item, ITEM_SIZE);
		listP++;
	}
	else
		printf("Item should not be NULL!\n");
	pthread_mutex_unlock(&audioBufLock);
}

void clearList(void)
{
    free(bAudio.data);
	listP = 0;
	firstP = 0;
}

void initList(void)
{
    bAudio.data = (char *)calloc(LIST_SPACE, sizeof(char));
	bAudio.getFunc = getItem;
    bAudio.addFunc = addItem;
    bAudio.clearFunc = clearList;
}
