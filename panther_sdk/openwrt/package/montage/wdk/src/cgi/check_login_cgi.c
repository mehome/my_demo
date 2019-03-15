/*=============================================================================+
|                                                                              |
| Copyright 2013                                                               |
| Montage Inc. All right reserved.                                           |
|                                                                              |
+=============================================================================*/
/*! 
*   \file check_login_cgi.c
*   \brief
*   \author
*/

/*=============================================================================+
| Included Files							       							   |
+=============================================================================*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

/*=============================================================================+
| defines						                       						   |
+=============================================================================*/
#define AUTH_FILENAME           "/tmp/httpd.conf"
#define COOKIE_NAME             "CID"
#define COOKIE_VALUE            "CaltRtr"
#define COOKIE_ID_LENGTH        12

#define CGI_DEBUG_LOG           "/tmp/debug-cgi.log"
#define CGI_DEBUG_LOG_1         "/tmp/debug-cgi.log.1"
#define CGI_DEBUG_LOG_MAXSIZE   4096
#define DBG(args...) do {                           \
    FILE *debugfp;                                  \
    struct stat sb;                                 \
    if (stat(CGI_DEBUG_LOG, &sb) == 0) {            \
        if (sb.st_size >= CGI_DEBUG_LOG_MAXSIZE) {  \
            rename(CGI_DEBUG_LOG, CGI_DEBUG_LOG_1); \
        }                                           \
    }                                               \
    if ((debugfp = fopen(CGI_DEBUG_LOG, "aw+"))) {  \
        fprintf(debugfp, ##args);                   \
        fflush(debugfp);                            \
        fsync(fileno(debugfp));                     \
        fclose(debugfp);                            \
    }                                               \
} while(0)

/*=============================================================================+
| Extern Function/Variables						       						   |
+=============================================================================*/
static char letter_tb[26] = 
	{'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};
static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';
int uh_auth_error;
extern int MD5String (unsigned char* string, unsigned int nLen, unsigned char* digest);

FILE *debugfp = NULL;

/*=============================================================================+
| Functions  						   	 	      							   |
+=============================================================================*/
/* skips all whitespace anywhere.
   converts characters, four at a time, starting at (or after)
   src from base - 64 numbers into three 8 bit bytes in the target area.
   it returns the number of data bytes stored at the target, or -1 on error.
 */
int
b64_pton(char const *src, u_char *target, size_t targsize)
{
	int tarindex, state, ch;
	char *pos;

	state = 0;
	tarindex = 0;

	while ((ch = *src++) != '\0') {
		if (isspace(ch))	/* Skip whitespace anywhere. */
			continue;

		if (ch == Pad64)
			break;

		pos = strchr(Base64, ch);
		if (pos == 0) 		/* A non-base64 character. */
			return (-1);

		switch (state) {
		case 0:
			if (target) {
				if (tarindex >= targsize)
					return (-1);
				target[tarindex] = (pos - Base64) << 2;
			}
			state = 1;
			break;
		case 1:
			if (target) {
				if (tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 4;
				target[tarindex+1]  = ((pos - Base64) & 0x0f)
							<< 4 ;
			}
			tarindex++;
			state = 2;
			break;
		case 2:
			if (target) {
				if (tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 2;
				target[tarindex+1]  = ((pos - Base64) & 0x03)
							<< 6;
			}
			tarindex++;
			state = 3;
			break;
		case 3:
			if (target) {
				if (tarindex >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base64);
			}
			tarindex++;
			state = 0;
			break;
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad64) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for (; ch != '\0'; ch = *src++)
				if (!isspace(ch))
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != Pad64)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for (; ch != '\0'; ch = *src++)
				if (!isspace(ch))
					return (-1);

			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (target && target[tarindex] != 0)
				return (-1);
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}

int hex2str(char *pszHex, int nHexLen, char *pszString)
{
	int i;
	char ch;
	for (i = 0; i < nHexLen; i++)
	{
		ch = (pszHex[i] & 0xF0) >> 4;
		pszString[i * 2] = (ch > 9) ? (ch + 0x41 - 0x0A) : (ch + 0x30);
		ch = pszHex[i] & 0x0F;
		pszString[i * 2 + 1] = (ch > 9) ? (ch + 0x41 - 0x0A) : (ch + 0x30);
	}
	pszString[i * 2] = 0;
	return 0;
}

static int uh_auth_compare(char *usr, char *pwd)
{
	FILE *c;
	char line[512];
	char *user = NULL;
	char *pass = NULL;
	char *eol  = NULL;
	int auth_ok = 0;

  c = fopen(AUTH_FILENAME, "r");
  if(c != NULL)
  {
		memset(line, 0, sizeof(line));
		while( fgets(line, sizeof(line) - 1, c) )
		{
			if( (line[0] == '/') && (strchr(line, ':') != NULL) )
			{
				if( !(user = strchr(line, ':')) || (*user++ = 0) ||
				    !(pass = strchr(user, ':')) || (*pass++ = 0) ||
					!(eol = strchr(pass, '\n')) || (*eol++  = 0) )
						continue;
			}
			else
				break;
		}
		if(!strcmp(user,usr)&&!strcmp(pass,pwd))	auth_ok = 1;
	}
	return auth_ok;
}

#define BUFFER_SIZE    2048
#define USERID_STRING      "UserID="
#define CHAR_LF 0xa //\n
#define CHAR_CR 0xd //\r

void gen_page(const char *page, char *user)
{
	char s[BUFFER_SIZE];
	char cid[COOKIE_ID_LENGTH+1];
	int i;

	memset(cid, 0, sizeof(cid));
	if(!uh_auth_error)
	{
		for( i=0; i<COOKIE_ID_LENGTH; i++ )
		{
			cid[i] = letter_tb[rand()%26];	
		}
		snprintf(s, sizeof(s), "Set-Cookie: %s=%s;Version=1;%s%s\r\n\r\n", COOKIE_NAME, cid, USERID_STRING, user );
	}
	strcat(s,"<html><head><script>location='");
	strcat(s, page);
	strcat(s, "';</script></head></html>\n");
	printf(s);
}

int main(int argc, char **argv)
{
    char *buf, *buf1;
    int ret, i, index, len, readlen = 0;
    char buffer[BUFFER_SIZE], read_buf[BUFFER_SIZE+1];
    char *user = NULL, *pass = NULL;
    char md5_buf[32], md5_pass[32+1], authpass[BUFFER_SIZE];

    memset(read_buf, 0, sizeof(read_buf));
    memset(buffer, 0, sizeof(buffer));

    srand(time(NULL));

    buf = getenv("CONTENT_LENGTH");
    if(buf)
    {
        len = atoi(buf);
        DBG("POST %d\n", len);

        while(1)
        {
        	ret = fread(read_buf, BUFFER_SIZE, 1, stdin);
        	if(ret<0)
        	{
        	    DBG("read error %d\n", ret);
        	    goto Exit;
        	}

        	if(readlen==0)
        	{
        	    DBG("read_buf = %s\n", read_buf);
							if(NULL == (buf = strrchr(read_buf, '&')))	goto Exit;
        	    *buf++ = '\0';
        	    if(NULL == (user = strchr(read_buf, '=')))	goto Exit;
							user++;
							DBG("user = %s\n", user);
        	    if(NULL == (buf1 = strchr(buf, '=')))	goto Exit;
							buf1++;
							DBG("pass (encrypted) = %s\n",buf1);
							/* transform to plain text */
							index = b64_pton( buf1, authpass, sizeof(authpass) );
							authpass[index]=0;
							DBG("pass (plain text) = %s\n",authpass);
							MD5String((unsigned char *)authpass, strlen(authpass), (unsigned char *)md5_buf);
							hex2str(md5_buf,16,md5_pass);
							pass = md5_pass;
							DBG("pass (md5) = %s\n",pass);
        	}
        	readlen += BUFFER_SIZE;

        	if(ret==0)
        	    break;
				}
        DBG("total read %d\n", readlen);
        if(readlen < len)
            goto Exit;

				if(uh_auth_compare(user,pass))
				{
					DBG("--------- Congratulations! Authentication is successful! ---------\n");
					uh_auth_error = 0;
					gen_page("index.htm", user);
				}
				else
				{
					DBG("--------- Sorry! Authentication is failed! ---------\n");
					uh_auth_error = 1;
					gen_page("index_web_error.htm", user);
				}
    }
    return 0;

Exit:
    return -1;
}
