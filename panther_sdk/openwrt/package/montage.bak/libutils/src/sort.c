#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>		/* AhMan March 18 2005 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wait.h>
#include <net/route.h>		/* AhMan March 18 2005 */
#include <sys/types.h>
#include <signal.h>

/* This is a NOEXEC applet. Be very careful! */

enum {
	FLAG_n  = 1,            /* Numeric sort */
	FLAG_g  = 2,            /* Sort using strtod() */
	FLAG_M  = 4,            /* Sort date */
/* ucsz apply to root level only, not keys.  b at root level implies bb */
	FLAG_u  = 8,            /* Unique */
	FLAG_c  = 0x10,         /* Check: no output, exit(!ordered) */
	FLAG_s  = 0x20,         /* Stable sort, no ascii fallback at end */
	FLAG_z  = 0x40,         /* Input and output is NUL terminated, not \n */
/* These can be applied to search keys, the previous four can't */
	FLAG_b  = 0x80,         /* Ignore leading blanks */
	FLAG_r  = 0x100,        /* Reverse */
	FLAG_d  = 0x200,        /* Ignore !(isalnum()|isspace()) */
	FLAG_f  = 0x400,        /* Force uppercase */
	FLAG_i  = 0x800,        /* Ignore !isprint() */
	FLAG_m  = 0x1000,       /* ignored: merge already sorted files; do not sort */
	FLAG_S  = 0x2000,       /* ignored: -S, --buffer-size=SIZE */
	FLAG_T  = 0x4000,       /* ignored: -T, --temporary-directory=DIR */
	FLAG_o  = 0x8000,
	FLAG_k  = 0x10000,
	FLAG_t  = 0x20000,
	FLAG_bb = 0x80000000,   /* Ignore trailing blanks  */
};

static
char* get_chunk_with_continuation(FILE *file, int *end, int *lineno)
{
    int ch = 0;
    int idx = 0;
    char *linebuf = NULL;
    int linebufsz = 0;

    while ((ch = getc(file)) != EOF) {
        /* grow the line buffer as necessary */

        if (idx >= linebufsz) {
            linebufsz += 256;
            linebuf = realloc(linebuf, linebufsz);
        }
        linebuf[idx++] = (char) ch;
        if (!ch)
            break;
        if (end && ch == '\n') {
            if (lineno == NULL)
                break;
            (*lineno)++;
            if (idx < 2 || linebuf[idx-2] != '\\')
                break;
            idx -= 2;
        }
    }
    if (end)
        *end = idx;
    if (linebuf) {
        // huh, does fgets discard prior data on error like this?
        // I don't think so....
        //if (ferror(file)) {
        //  free(linebuf);
        //  return NULL;
        //}
        linebuf = realloc(linebuf, idx + 1);
        linebuf[idx] = '\0';
    }
    return linebuf;
}


char* xmalloc_fgetline(FILE *file)
{
    int i = 0;
    char *c = get_chunk_with_continuation(file, &i, NULL);

    if ((i > 0) && c[--i] == '\n')
        c[i] = '\0';
		
    return c;
}

void* xrealloc_vector(void *vector, unsigned sizeof_and_shift, int idx)
{
    int mask = 1 << (uint8_t)sizeof_and_shift;
		
    if (!(idx & (mask - 1))) {
        //sizeof_and_shift >>= 8; /* sizeof(vector[0]) */
        
        vector = realloc(vector, sizeof_and_shift * (idx + mask + 1) );
        memset((char*)vector + (sizeof_and_shift * idx), 0, sizeof_and_shift * (mask + 1));
    }
    
    return vector;
}


/* Iterate through keys list and perform comparisons */
static int compare_keys(const void *xarg, const void *yarg)
{
	int flags = FLAG_s, retval = 0;
	char *x, *y;


	/* This curly bracket serves no purpose but to match the nesting
	   level of the for () loop we're not using */
	{
		x = *(char **)xarg;
		y = *(char **)yarg;

		/* Perform actual comparison */
		switch (flags & 7) {
		default:
			printf("unknown sort type\n");
			break;
		/* Ascii sort */
		case 0:
#if ENABLE_LOCALE_SUPPORT
			retval = strcoll(x, y);
#else
			retval = strcmp(x, y);
#endif
			break;

		/* Integer version of -n for tiny systems */
		case FLAG_n:
			retval = atoi(x) - atoi(y);
			break;
		} /* switch */
	} /* for */
	return retval;
}

int sort_file(char *infile, char *outfile)
{
	FILE *fp, *outfp = NULL;
	char *line = NULL, **lines = NULL;
	int linecount = 0, i = 0;

	if( !infile || !outfile ) {
		return -1;
	}
	outfp = fopen(outfile, "w");
	fp = fopen(infile, "r");
	
	if( fp && outfp ) {
			/* Open input files and read data */
			while( (line = xmalloc_fgetline(fp)) ) {
				
				/* coreutils 6.9 compat: abort on first open error,
				 * do not continue to next file: */
				lines = xrealloc_vector(lines, 6, linecount);
				lines[linecount++] = line;
			}
			fclose(fp);

			/* Perform the actual sort */
			qsort(lines, linecount, sizeof(char *), compare_keys);
			
			for (i = 0; i < linecount; i++) {
				fprintf(outfp, "%s\n", lines[i]);
				free(lines[i]);
			}
			fclose(outfp);
	}
	return 0;
}
