#ifndef _BUFFER_H
#define _BUFFER_H

#define DATA_LEN 2048
#define LIST_LEN 200

typedef struct s_Buffer 
{
    char data[DATA_LEN];
	long timestamp;
	struct s_Buffer *next;
} Buffer;

typedef struct
{
  Buffer *head;
  Buffer *tail;
  int length;
} List;

void initList(List *list);
void clearList(List *list);
int putBuffer(List *list, char *dat);
int getBuffer(List *list, char *outdata);



#endif
