#ifndef __MAD_DECODER_H__
#define __MAD_DECODER_H__

extern unsigned long get_file_size(const char *path);
extern int decode(unsigned char const *start, unsigned long length);
extern int safe_open(const char *name);


#endif



