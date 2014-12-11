#ifndef DECODEH264_H
#define DECODEH264_H

int initVideo();
void closeVideo();
void video_decode();

void *videoThread(void *argc);

extern pthread_t h264tid;
extern pthread_mutex_t h264lock;
#endif
