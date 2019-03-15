#ifndef ___MYUTILS_MY_STRING_H__
#define ___MYUTILS_MY_STRING_H__

typedef struct mystring 
{
	void *buf;
	int write;
	int read;
	int len;
} mystring;

mystring *string_new();
void string_append(mystring *str, void *buf ,int len);
void *string_data(mystring *str, int *len);
void string_free(mystring *str);
int string_empty(mystring *str);
int string_length(mystring *str);
void string_zero(mystring *str);



#endif
