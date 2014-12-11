#ifndef PLAYBACK_H
#define PLAYBACK_H

int initPlayback(int channels, int rate);
int closePlayback();
int playback(char *buffer, int frames);

void *playThread(void *);

extern pthread_t playtid;

#endif
