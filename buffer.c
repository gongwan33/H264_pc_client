#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <buffer.h>

pthread_mutex_t listLock;

void initList(List *list)
{
	list->head = NULL;
    list->tail = NULL;
    list->length = 0;	
}

void clearList(List *list)
{
	Buffer *p = NULL;
	pthread_mutex_lock(&listLock);
    for(p = list->head; p != NULL; )
	{
		Buffer *buf = p;
		p = p->next;
		free(buf);
	}
	list->length = 0;
	pthread_mutex_unlock(&listLock);
}

int putBuffer(List *list, char *dat)
{
    if(list->length >= LIST_LEN)
	{
       clearList(list); 
	}

	Buffer *buf = (Buffer *)malloc(sizeof(Buffer));
	if(buf == NULL)
	{
		printf("Buffer.c: Memory not enough\n");
		return -1;
	}
	
    buf->next = NULL;
	memcpy(buf->data, dat, DATA_LEN);
	buf->timestamp = 0;
	
	pthread_mutex_lock(&listLock);
	if(list->tail != NULL)
		list->tail->next = buf;
	list->tail = buf;
	if(list->length == 0)
		list->head = buf;
	list->length++;
	pthread_mutex_unlock(&listLock);
   return 0; 
}

int getBuffer(List *list, char *outdata)
{
    if(list->length <= 0)
		return -1;
     
	pthread_mutex_lock(&listLock);
	if(list->head != NULL)
	{
        memcpy(outdata, list->head->data, DATA_LEN);

		if(list->head != list->tail)
		{
		    list->head = list->head->next;
			list->length--;
		}
		else
		{
			list->head = NULL;
			list->tail = NULL;
			list->length = 0;
		}
	}
	else
	{
	    pthread_mutex_unlock(&listLock);
		return -1;
	}
	pthread_mutex_unlock(&listLock);
    return 0;
}

/*========================test==================================*/
/*
void main()
{
	char test1[DATA_LEN];
	char test2[DATA_LEN];
	char test3[DATA_LEN];
	char outdat[DATA_LEN];

	test1[0] = 'h';
	test1[1000] = 'm';
	test1[2047] = 'q';

	test2[0] = 'x';
	test2[1000] = 'y';
	test2[2047] = 'z';

	test3[0] = 'a';
	test3[1000] = 'b';
	test3[2047] = 'c';

	List list;

	initList(&list);

	putBuffer(&list, test1);
	putBuffer(&list, test2);
	putBuffer(&list, test3);

	getBuffer(&list, outdat);
	printf("out = %c %c %c\n", outdat[0], outdat[1000], outdat[2047]);
	getBuffer(&list, outdat);
	printf("out = %c %c %c\n", outdat[0], outdat[1000], outdat[2047]);
	getBuffer(&list, outdat);
	printf("out = %c %c %c\n", outdat[0], outdat[1000], outdat[2047]);
	memset(outdat, 0, DATA_LEN);
	getBuffer(&list, outdat);
	printf("out = %c %c %c\n", outdat[0], outdat[1000], outdat[2047]);

	clearList(&list);
}
*/
