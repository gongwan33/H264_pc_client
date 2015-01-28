#ifndef UNITBUF_H
#define UNITBUF_H

#include <stdio.h>
#include <stdlib.h>

#define UNITBUF_SIZE 1000*100
#define UNITLIST_SIZE 100

struct unitBuf {
    u_int32_t index;
	char *buf;
	u_int32_t len;
};

static struct unitBuf unitList[UNITLIST_SIZE];
static unitListP = -1;

void initUnitList()
{
	memset(unitList, -1, sizeof(unitList));
	printf("unit list initialized %d\n", sizeof(unitList));
}

int findUnitIndex(u_int32_t index)
{
	int i = 0;
	for(i = 0; i <= UNITLIST_SIZE; i++)
	{
		if(unitList[i].index == index)
			return i;
	}
	return -1;
}

int addUnit(char *data, int len, u_int32_t index, u_int32_t address)
{
	int pos = findUnitIndex(index);
	if(-1 == pos)
	{
		if(unitListP < UNITLIST_SIZE - 1)
		unitListP++;
		else
		{
			printf("unit buf pointer restart\n");
			unitListP = 0;
		}

		unitList[unitListP].buf = (char *)calloc(UNITBUF_SIZE, sizeof(char));
		unitList[unitListP].index = index;
		unitList[unitListP].len = address + len; 
		if(unitList[unitListP].len < UNITBUF_SIZE)
			memcpy(unitList[unitListP].buf + address, data, len);
		else
		{
			printf("erro!! unitbuf buf over flow!!\n");
			return -1;
		}
			
	}
	else
	{
		unitList[pos].len = address + len;
		if(unitList[unitListP].len < UNITBUF_SIZE)
			memcpy(unitList[pos].buf + address, data, len);
		else
		{
			printf("erro!! unitbuf buf over flow!!\n");
			return -1;
		}
	}
	return 0;
}

int getUnit(u_int32_t index, char **data, int *len)
{
	int pos = findUnitIndex(index);
	if(pos != -1)
	{
		*data = unitList[pos].buf;
		*len = unitList[pos].len;
		return 0;
	}
	else
		return -1;
}

int releaseUnit(u_int32_t index)
{
	int pos = findUnitIndex(index);
	if(pos != -1)
	{
		unitList[pos].index = -1;
		free(unitList[pos].buf);
		return 0;
	}
	else
	{
		return -1;
	}
}


#endif
