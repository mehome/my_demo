#include <fvad.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "myutils/debug.h"


Fvad *gVad = NULL;


void vad_init(int samplerate)
{
	if(gVad== NULL) {
		gVad = fvad_new();
		 if (!gVad) {
	        fprintf(stderr, "out of memory\n");
	        exit(-1);
	    }
		fvad_set_sample_rate(gVad, samplerate);
	
	}
}
void vad_exit()
{
	if(gVad)
		fvad_free(gVad);
}
#define BIG_ENDIAN 1

int vad(char  *  buf, int framelen, void *wav)
{
#if BIG_ENDIAN
  	unsigned char *p;
  	unsigned char temp;
#endif
	int vadres = 0;
	int16_t *buf1 = NULL;
	int16_t *buf2 = NULL;
	size_t i;

	/*if (framelen > SIZE_MAX / sizeof (double)
			|| !(buf1 = malloc(framelen * sizeof *buf1))) {
				fprintf(stderr, "failed to allocate buffers\n");

			goto end;
	}*/

	buf1 = calloc(1, framelen * (sizeof *buf1));
	if(buf1 == NULL) {
		fprintf(stderr, "failed to allocate buffers\n");
		goto end;
	}
	buf2 = calloc(1, framelen * (sizeof *buf1));
	if(buf2 == NULL) {
		fprintf(stderr, "failed to allocate buffers\n");
		goto end;
	}
	for(i = 0; i < framelen; i++) {
		buf1[i] = (buf[(2*i)+0] << 8) | buf[(2*i)+1];
	}
	for (i = 0; i < framelen; i++)
            buf2[i] = buf1[i] * INT16_MAX;
	
	wav_write_data(wav, buf2, framelen);


	#if 0
	for ( i = 0; i < framelen; i++) {
#if BIG_ENDIAN
			p = &buf[i];
			temp = p[0];
			p[0] = p[1];
			p[1] = temp;
#endif
			buf1[i] = buf[i] * INT16_MAX;
	}
	#endif
	printf("fvad_process\n");
	vadres = fvad_process(gVad, buf2, framelen);
	if (vadres < 0) {
		   fprintf(stderr, "VAD processing failed\n");
		   return -1;
	}
	printf("fvad_process done\n");
end:
	if (buf1) free(buf1);
	if (buf2) free(buf2);
	return vadres;
}








