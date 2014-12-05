#ifndef BLOWFISH_H
#define BLOWFISH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOWFISH_KEY "-shanghai-hangzhou"
extern void BlowfishKeyInit(char* strKey,int nLen);
extern void BlowfishEncrption(unsigned long * ,int len);
extern void BlowfishDecrption(unsigned long * ,int len);

#endif

