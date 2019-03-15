/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>


#include "debug.h"

int LogInit(int argc , char **argv)
{
	char *showLog = NULL;
	char *logLevel = NULL;
	log_set_quiet(1);
	log_set_level(LOG_FATAL);
	showLog = ConfigParserGetValue("showLog");
	if(showLog) 
	{
		if(strcmp(showLog, "true") == 0) 
		{
			log_set_level(LOG_ERROR);
			log_set_quiet(0);
		}
		else if(strcmp(showLog, "false") == 0) 
		{
			log_set_quiet(1);
		}
		logLevel  = ConfigParserGetValue("logLevel");
		if(logLevel)
		{
			if(strcmp(logLevel, "error") == 0) {
				log_set_level(LOG_ERROR);	
			} else if(strcmp(logLevel, "warn") == 0) {
				log_set_level(LOG_WARN);	
			} else if(strcmp(logLevel, "info") == 0) {
				log_set_level(LOG_INFO);
			}else if(strcmp(logLevel,  "fatal") == 0) {
				log_set_level(LOG_FATAL);
			}else if(strcmp(logLevel,  "debug") == 0) {
				log_set_level(LOG_DEBUG);
			}
			
			free(logLevel);
		}
		free(showLog);
	}

	if(argc >= 2) 
	{
		if(strcmp(argv[1], "showlog") == 0)
		{
			 log_set_quiet(0);
			 if(argc == 3) {
				if(strcmp(argv[2], "error") == 0) {
					log_set_level(LOG_ERROR);	
				} else if(strcmp(argv[2], "warn") == 0) {
					log_set_level(LOG_WARN);	
				} else if(strcmp(argv[2], "info") == 0) {
					log_set_level(LOG_INFO);
				}else if(strcmp(argv[2],  "fatal") == 0) {
					log_set_level(LOG_FATAL);
				}else if(strcmp(argv[2],  "debug") == 0) {
					log_set_level(LOG_DEBUG);
				}
			}
		}
		else if(strcmp(argv[1], "file") == 0) 
		{
			if(argc == 3) {
				filelog_init(argv[3]);
			} else {
				filelog_init("/tmp/iot.log");
			}
		}
	}	

}



