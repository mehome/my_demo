/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	mpg123 defines 
	used source: musicout.h from mpegaudio package
*/

#ifndef MPG123_H
#define MPG123_H
#include "httpget.h"
#include "true.h"

extern struct httpdata htd;
extern off_t framenum;
extern int intflag;


/* why extern? */
extern int play_frame(void);

extern int control_generic(mpg123_handle *fr);



int   open_track(char *fname);

void close_track(void);

#endif 
