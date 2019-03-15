#include "mystring.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

mystring *string_new()
{
	mystring *str = (mystring *)calloc(1, sizeof(mystring));
	if(str == NULL)
		return NULL;
	str->len 	= 0;
	str->read 	= 0;
	str->write 	= 0;	
	return str;
}
void string_append(mystring *str, void *buf ,int len)
{
	assert(str != NULL);
	//if( str->write == 0 ) {
	if( str->len == 0 ) {
		str->len += len;
		str->len += 10;
		str->buf = calloc(1, str->len);
	} else {
		if( (str->len - str->write) <= len) {
			str->len += len;
			str->len += 10;
			str->buf = realloc(str->buf, str->len);	
			memset(str->buf+str->write, 0, len + 10);
		}
	}
	memcpy(str->buf + str->write, buf, len);
	str->write += len;
}
void *string_data(mystring *str, int *len)
{
	assert(str != NULL);
	*len = str->write;
	return str->buf;
}
void string_zero(mystring *str)
{
	assert(str != NULL);
	str->write = 0;
	str->len = 0;
	free(str->buf);
	str->buf = NULL;
}


int string_empty(mystring *str)
{
	assert(str != NULL);
	return str->write == 0 ? 1 : 0;
}

int string_length(mystring *str)
{
	assert(str != NULL);
	return str->write;
}


void string_free(mystring *str)
{
	if(str) {
		if(str->buf) 
			free(str->buf);
		free(str);
		str = NULL;
	}
}


