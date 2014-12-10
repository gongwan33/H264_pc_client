#ifndef LIST_H
#define LIST_H

#define LIST_SPACE 2048*20
#define ITEM_SIZE 2048

typedef char *(*getFunc_t)(void);
typedef void (*addFunc_t)(char *);
typedef void (*clearFunc_t)(void); 
typedef void (*initFunc_t)(void); 

struct List
{
    char *data;
    getFunc_t getFunc;
	addFunc_t addFunc;
	clearFunc_t clearFunc;
};

extern struct List bAudio;

void initList();
#endif
