#include <stdio.h>
#include <stdlib.h>
#include "uuid.h"

#define LEN 37

void getuuidString(char *pDate)  
{  
    int flag, i;
	static unsigned int n = 0;
    srand((unsigned) time(NULL )+n);  
    for (i = 0; i < LEN - 1; i++)  
    {  
        flag = rand() % 2;  
        switch (flag)  
        {  
            case 0:  
                pDate[i] = 'a' + rand() % 6;  
                break;  
            case 1:  
                pDate[i] = '0' + rand() % 10;  
                break;  
            default:  
                pDate[i] = 'x';  
                break;  
        }  
	if(7 == i || 12 == i || 17 == i || 22 == i)
		pDate[++i] = '-';
    }  
    	pDate[LEN - 1] = '\0';  
	n++;
}  

