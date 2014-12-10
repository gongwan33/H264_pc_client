#include <stdlib.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <r_cmd.h>
#include <list.h>

static snd_pcm_t *handle;

int initPlayback(int channels, int rate)
{
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	int rc = 0;

	/* Open PCM device for playback. */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0) {
		printf("unable to open pcm device: %s/n", snd_strerror(rc));
		return -1;
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, channels);

	/* 44100 bits/second sampling rate (CD quality) */
	val = rate;
	snd_pcm_hw_params_set_rate_near(handle, params,
			&val, &dir);

	/* Set period size to 32 frames. */
	frames = 1024;
	snd_pcm_hw_params_set_period_size_near(handle,
			params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		printf("unable to set hw parameters: %s/n",	snd_strerror(rc));
		return -1;
	}
    
	return 0;
}

int closePlayback()
{
     snd_pcm_drain(handle);
	 snd_pcm_close(handle);
	 return 0;
}

int playback(char *buffer, int frames)
{
	int rc = 0;

	rc = snd_pcm_writei(handle, buffer, frames);

	if (rc == -EPIPE) {
		/* EPIPE means underrun */
		printf("underrun occurred\n");
		snd_pcm_prepare(handle);
	} else if (rc < 0) {
		printf("error from writei: %s\n", snd_strerror(rc));
	}  else if (rc != (int)frames) {
		printf("short write, write %d frames\n", rc);
	}

	return 0;
}

void *playbackThread(void *argc)
{
     initPlayback(1, 16000);
	 struct pcmAudio *audio = NULL;

	 while(connected)
	 {
		 char *buf = bAudio.getFunc();

		 if(buf != NULL)
			playback(buf, 1024);
		 else
		 {
			usleep(64000);
		 }
	 }

	 closePlayback();
}
