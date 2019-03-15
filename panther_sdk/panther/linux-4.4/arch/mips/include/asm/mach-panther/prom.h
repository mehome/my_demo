/* 
 *  Copyright (c) 2008, 2009    Montage Technology Group Limited     All rights reserved.
 *
 *  bootprom interface
 */
#ifndef _PANTHER_PROM_H
#define _PANTHER_PROM_H

extern char *prom_getcmdline(void);
extern char *prom_getenv(char *name);
extern void prom_init_cmdline(void);
extern void prom_meminit(void);

#endif /* !(_PANTHER_PROM_H) */

