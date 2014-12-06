#ifndef ADPCM_H
#define ADPCM_H

struct adpcm_state { 
	short      valprev;        /* Previous output value */ 
	char       index;          /* Index into stepsize table */ 
}__attribute__((packed)); 

typedef struct adpcm_state adpcm_state_t;

void adpcm_decoder(char [], int [], int, struct adpcm_state *);

#endif
