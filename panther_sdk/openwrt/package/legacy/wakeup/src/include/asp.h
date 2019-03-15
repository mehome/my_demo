#ifndef __ASP_H__
#define __ASP_H__

int ASP_init(void);
char* ASP_acquire_data(int *samples, unsigned long long *ts);
void ASP_release_data(void);

#endif //__ASP_H__
