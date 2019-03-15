#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define HAVE_DBG
#ifdef HAVE_DBG
#define CGI_DEBUG_LOG           "/tmp/cgi.log"
#define CGI_DEBUG_LOG_1         "/tmp/cgi.log.1"
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
//#define DBG(fmt, ...) printf("%s:%d " fmt, __func__, __LINE__, ## __VA_ARGS__)
#else
#define DBG(args...) 
#endif

void cdb_close(void);

char delimiter[128];

#define INTERFACE_RULE_FILENAME     "/tmp/rule"
struct netstatus_t
{
	char idx;
	char *name;
	char *rdb_name;
};
enum {
    IDX_WAN_STATE,
    IDX_WAN_MAC,
    IDX_WAN_IP,
    IDX_WAN_MASK,
    IDX_WAN_GW,
    IDX_WAN_DOMAIN,
    IDX_WAN_CONNTIME,
    IDX_WAN_CONNTIMES,
    IDX_WAN_DNS1,
    IDX_WAN_DNS2,
    IDX_LAN_MAC,
    IDX_LAN_IP,
    IDX_WLAN_MAC,
    IDX_USB_TELECOM,
    IDX_COUNT
};
static struct netstatus_t netstatus_tab[IDX_COUNT]={
	{ IDX_WAN_STATE,     "wan_state",  "$wanif_state"     },
	{ IDX_WAN_MAC,       "wan_mac",    "$wanif_mac"       },
	{ IDX_WAN_IP,        "wan_ip",     "$wanif_ip"        },
	{ IDX_WAN_MASK,      "wan_mask",   "$wanif_msk"       },
	{ IDX_WAN_GW,        "wan_gw",     "$wanif_gw"        },
	{ IDX_WAN_DOMAIN,    "wan_domain", "$wanif_domain"    }, 
	{ IDX_WAN_CONNTIME,  "conntime",   "$wanif_conntime"  },
	{ IDX_WAN_CONNTIMES, "conntimes",  "$wanif_conntimes" },
	{ IDX_WAN_DNS1,      "dns1",       "$wanif_dns1"      },
	{ IDX_WAN_DNS2,      "dns2",       "$wanif_dns2"      },
	{ IDX_LAN_MAC,       "lan_mac",    "$lanif_mac"       },
	{ IDX_LAN_IP,        "lan_ip",     "$lanif_ip"        },
	{ IDX_WLAN_MAC,      "wl_mac",     "$wlanif_mac"      },
	{ IDX_USB_TELECOM,   "mob_telecom","$usbif_telecom"   },
};


#define MAX_CMD_SIZE 512
#define MAX_BUF_SIZE 2048

void unescape_string(char *to, char *from)
{
    char *dst = to;
    char *src = from;

#define hex(x) \
	(((x) <= '9') ? ((x) - '0') : \
		(((x) <= 'F') ? ((x) - 'A' + 10) : \
			((x) - 'a' + 10)))

    while(*src != '\0')
    {
        if(src[0] == '%') {
            if( isxdigit(src[1]) && isxdigit(src[2]) ) {
				*dst = (char)(16 * hex(src[1]) + hex(src[2]));

                dst++;
                src+=3;

                continue;
            }
        }
        *dst = *src;

        dst++;
        src++;
    }

    *dst = '\0';

    return;
}

void first_arg(unsigned char *to, unsigned char *from)
{
    return;
}

int cmd_echo(char *cmd)
{
    char *p;

    if((p=strstr(cmd,"echo ")))
    {
        p += 5;
    }
    else
    {
        return -1;
    }

    printf("%s\n", p);
    DBG("%s:%d %s\n", __func__, __LINE__, p);

    return 0;
}

int find_status_idx(char *input)
{
    int idx;

	for(idx=0;idx<IDX_COUNT;idx++)
	{
    	if( strstr(input, netstatus_tab[idx].name) )
	    {
			return idx;
	    }
	}
    return -1;
}

char * get_byte_count(char *fname)
{
    static char line[20];
    static char err[]="!ERR";
    char filename[50] = "/sys/class/net/";
    char pre_fname[] = "/statistics/";
    char *p;
    FILE *c;
    int ret = 0;

    memset(line, 0, sizeof(line));
    if( (c = fopen(INTERFACE_RULE_FILENAME, "r")) != NULL )
    {
        while( fgets(line, sizeof(line), c) )
        {
            p = &line[strlen(line) - 1];
            if(*p == '\n')
                *p = '\0';
            if((p=strstr(line,"WAN=")))
            {
                p += 4;
                strcat(filename, p); // carefully buffer overflow
                strcat(filename, pre_fname);
                strcat(filename, fname);

                ret = 1;
                break;
            }
        }
        fclose(c);

        DBG("%s:%d byte_count:[%s]\n", __func__, __LINE__, filename);
        if( !ret )
            goto Exit;

        if( (c = fopen(filename, "r")) != NULL )
        {
            fgets(line, sizeof(line) - 1, c);
            p = &line[strlen(line) - 1];
            if(*p == '\n')
                *p = '\0';
            fclose(c);
            DBG("%s:%d byte_count:[%s]\n", __func__, __LINE__, line);
        }
    }

Exit:
    if (ret)
	    return line;
    else
        return err;
}

int cmd_status(char *cmd)
{
    char *p;

    if((p=strstr(cmd,"status ")))
    {
        p += 7;
    }
    else
    {
        return -1;
    }

    if( !strcmp(p, "wan_rx_bytes") )
    {
        printf("%s\n", get_byte_count("rx_bytes"));
    }
    else if( !strcmp(p, "wan_tx_bytes") )
    {
        printf("%s\n", get_byte_count("tx_bytes"));
    }
    else
    {
        return -1;
    }

    return 0;
}

#define CDB_PROCFS_FILENAME     "/proc/cdb"
#define BUF_SIZE 512

FILE *cdb_fp = NULL;

int cdb_multiple_command(char *cmd)
{
	int ret = -1;
    char buf[BUF_SIZE];
    char *p;

	if(NULL == cdb_fp) {
        cdb_fp = fopen(CDB_PROCFS_FILENAME, "r+");
        if(NULL == cdb_fp) {
			DBG("%s:%d fopen %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
            goto Exit;
        }
    }
    fseek(cdb_fp, 0L, SEEK_SET);
	
	ret = fwrite(cmd, strlen(cmd), 1, cdb_fp);
    if(0 > ret) {
		DBG("%s:%d fwrite %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
        goto Exit;
    }

    while((p = fgets(buf, BUF_SIZE, cdb_fp)))
    {
		printf("%s%s\n", buf, delimiter);
		DBG("%s:%d buf=[%s] del=[%s]\n", __func__, __LINE__, buf, delimiter);
    }

    cdb_close();

    return 0;
Exit:
	printf("!ERR\n");
	DBG("%s:%d !ERR(%d)%s\n", __func__, __LINE__, ret, cmd);

    cdb_close();

    return ret;

}

int __cdb_command(char *command, FILE *cdb_fp, char *value)
{
    int ret;
    char buf[BUF_SIZE];
    char *p;
    int len;

    if(value)
        *value = '\0';

    ret = fwrite(command, strlen(command), 1, cdb_fp);
    if(0 > ret) {
		DBG("%s:%d fwrite %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
        return ret;
    }

    while((p = fgets(buf, BUF_SIZE, cdb_fp)))
    {
        if(value)
        {
            sprintf(value, "%s", p);
        }
    }

    if(value)
    {
        len = strlen(value);
        if((len>0)&&(value[len-1]=='\n'))
        {
            value[len-1] = '\0';
        }
    }

    return 0;
}

int cdb_set(char *name, void* value)
{
    int ret = -1;
    char line_buf[BUF_SIZE];

    if(NULL == cdb_fp) {
        cdb_fp = fopen(CDB_PROCFS_FILENAME, "r+");
        if(NULL == cdb_fp) {
			DBG("%s:%d fopen %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
            goto Exit;
        }
    }
    fseek(cdb_fp, 0L, SEEK_SET);

    snprintf(line_buf, BUF_SIZE, "%s=%s", name, (char *)value);
    line_buf[BUF_SIZE-1] = '\0';
    ret = __cdb_command(line_buf, cdb_fp, NULL);
    if(0 > ret) {
		DBG("%s:%d __cdb_command %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
        goto Exit;
    }

    cdb_close();

    return 0;

Exit:
	DBG("%s:%d !ERR(%d)%s\n", __func__, __LINE__, ret, line_buf);

    cdb_close();

    return -1;
}

int cdb_get(char *name, void* value)
{
    int ret = -1;

    if(NULL == cdb_fp) {
        cdb_fp = fopen(CDB_PROCFS_FILENAME, "r+");
        if(NULL == cdb_fp) {
			DBG("%s:%d fopen %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
            goto Exit;
        }
    }
    fseek(cdb_fp, 0L, SEEK_SET);

    ret = __cdb_command(name, cdb_fp, value);
    if(0 > ret) {
		DBG("%s:%d __cdb_command %s failed\n", __func__, __LINE__, CDB_PROCFS_FILENAME);
        goto Exit;
    }

    cdb_close();

    return 0;

Exit:
	DBG("%s:%d !ERR(%d)%s\n", __func__, __LINE__, ret, name);

    cdb_close();

    return -1;
}

void cdb_close(void)
{
    if(cdb_fp)
        fclose(cdb_fp);
    cdb_fp = NULL;
}

#define EXEC_ERR_NOIMPL     -1
#define EXEC_ERR_INTERNAL   -2
#define EXEC_ERR_EXECUTION  -3

int exec_script(char *cmd)
{
    char *p = cmd;
    struct stat sb;

    int ret = EXEC_ERR_EXECUTION;

	int wfd[2] = { 0, 0 };
    pid_t child;

    int __argc = 0;
    char*  __argv[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };
    int status;

    int fd_max, buflen;
    fd_set reader;
    struct timeval timeout;
    char return_msg[MAX_BUF_SIZE];
    sigset_t ssn, sso;

    if (!strncmp(cmd, "/lib/wdk/stor", 13))
    {
        __argv[__argc++] = p;
        p += 13;
        *p = '\0';
        p++;
        __argv[__argc++] = p;

        while(*p!='\0')
        {
            if(*p==' ')
            {
                *p = '\0';
                p++;
                __argv[__argc++] = p;
                /* merge all args to one for cmd stor */
                break;
            }
            else
            {
                p++;
            }
        }
    }
    else
    {
        __argv[__argc] = p;
        __argc++;
        while(*p!='\0')
        {
            if(*p==' ')
            {
                *p = '\0';
                p++;

                while(*p==' ')
                    p++;

                __argv[__argc] = p;
                __argc++;
            }
            else
            {
                p++;
            }
        }
    }

    if(stat(cmd, &sb) == -1)
        return EXEC_ERR_NOIMPL;

    if(pipe(wfd) < 0)
    {
		if(wfd[0] > 0)
            close(wfd[0]);
		if(wfd[1] > 0)
            close(wfd[1]);
        return EXEC_ERR_INTERNAL;
    }

	switch( (child = fork()) )
    {
		/* oops */
        case -1:
            DBG("%s:%d fork failure\n", __func__, __LINE__);
			return EXEC_ERR_INTERNAL;
            break;

		/* exec child */
        case 0:
            /* UNBLOCK SIGCHLD in child thread */
            sigemptyset(&ssn);
            sigaddset(&ssn, SIGCHLD);
            sigprocmask(SIG_UNBLOCK, &ssn, &sso);

            dup2(wfd[1], 1);
            execv(cmd, (char **const) &__argv);

            close(wfd[1]);
            exit(EXEC_ERR_INTERNAL);

            return EXEC_ERR_INTERNAL;
            break;

        default:

            close(wfd[1]);
            fcntl(wfd[0], F_SETFL, O_NONBLOCK);
            ret = 0;
            fd_max = wfd[0] + 1;

            while( 1 )
            {
                /* leave if our child exit */
                if(0!=waitpid(child, &status, WNOHANG))
                    break;

                FD_ZERO(&reader);
                FD_SET(wfd[0], &reader);
                timeout.tv_sec=0;
                timeout.tv_usec=500000;

                if ( select(fd_max, &reader, 0, 0, &timeout) > 0) 
                {
                    if( FD_ISSET(wfd[0], &reader) )
                    {
                        while ((buflen = read(wfd[0], return_msg, MAX_BUF_SIZE)) > 0)
                        {
                            //fwrite(return_msg, buflen, 1, stdout);
							DBG("%s:%d exec: %s\n", __func__, __LINE__, return_msg);
							return_msg[buflen] = 0;
							printf("%s\n", return_msg);
                        }
                    }
                }
            }
            close(wfd[0]);

            break;
    }

    return ret;
}

int cli_process_single_cmd(char *cmd)
{
    //char *p;
    char system_exec_buf[MAX_CMD_SIZE];
    int ret;

    while(*cmd != '\0')
    {
        if(*cmd == ' ')
        {
            cmd++;
            continue;
        }
#if 0
        if(*cmd == '$')
        {
            //DBG("%s:%d cdb: %s\n", __func__, __LINE__, cmd);
            if(cmd[1]=='\0' || cmd[1]==' ')
            {
                cdb_command("$");
            }
            else
            {
                if((p=strstr(cmd, "=")))
                {
                    *p = '\0';
                    p++;
    
                    cmd_cdb_set(cmd, p);
                }
                else
                {
                    cmd_cdb_get(cmd);
                }
            }
            break;
        }
#endif
        if(!strncmp(cmd, "echo", 4))
        {
            //DBG("%s:%d echo: %s\n", __func__, __LINE__, cmd);
            cmd_echo(cmd);
            break;
        }

        if(!strncmp(cmd, "status", 6))
        {
            //DBG("%s:%d status: %s\n", __func__, __LINE__, cmd);
            if(!cmd_status(cmd))
                break;
        }

        DBG("%s:%d => script command: %s\n", __func__, __LINE__, cmd);

        snprintf(system_exec_buf, MAX_CMD_SIZE - 1, "/lib/wdk/%s", cmd);
        system_exec_buf[MAX_CMD_SIZE-1] = '\0';

        //DBG("%s:%d exec command: %s\n", __func__, __LINE__, system_exec_buf);
        ret = exec_script(system_exec_buf);
         if(ret==EXEC_ERR_NOIMPL)
        {
            printf("NO IMPL\n");
            DBG("%s:%d NO IMPL %s\n", __func__, __LINE__, cmd);
        }
        else if((ret==EXEC_ERR_INTERNAL)||(ret==EXEC_ERR_EXECUTION))
        {
            printf("EXEC ERR %d\n", ret);
            DBG("%s:%d EXEC ERR %s %d\n", __func__, __LINE__, cmd, ret);
        }
        else if(ret!=0)
        {
            printf("SCRIPT ERR %d\n", ret);
            DBG("%s:%d SCRIPT ERR %s %d\n", __func__, __LINE__, cmd, ret);
        }

        break;
    }

    return 0;
}

void aggr_cmd(char **acmd, char *cmd, int len, char del)
{
	if(del == '\xff')
	{
		**acmd = del;
		(*acmd)++;
	}
	strncpy(*acmd, cmd, len);
	*acmd += len;
	**acmd = '\0';
}

void do_aggr_cmd(char **acmd, char *cmd, int len, char *mcmd)
{
	if(len > ((mcmd + MAX_CMD_SIZE) - *acmd - 2))
	{
		/* Reminding buffer is enough to append command.*/
		cdb_multiple_command(mcmd);

		*acmd = mcmd;
		aggr_cmd(acmd, cmd, len, 0);
	}
	else
	{
		aggr_cmd(acmd, cmd, len, '\xff');
	}
}

int main(int argc, char **argv)
{
    char *buf=NULL, *nbuf, *start;
    char multiple_cmd[MAX_CMD_SIZE];
    char single_cmd[MAX_CMD_SIZE];
    char http_post_buf[MAX_BUF_SIZE];
    char *aggr_cmd_end;
    int i, len, buflen;
    int aggr_cdb = 0;
    int content_length = 0;

    memset(delimiter, 0, sizeof(delimiter));

    buf = getenv("CONTENT_LENGTH");
    if (buf && (atoi(buf) > 0))
    {
        len = atoi(buf);
        DBG("POST %d\n", len);
        fgets(http_post_buf, MAX_BUF_SIZE, stdin);
        buf = http_post_buf;
    }
    else
	{
        buf = getenv("QUERY_STRING");
	}

    if(NULL==buf)
       goto Exit;

    if((nbuf=strstr(buf, "cmd=")))
    {
        buf = nbuf + 4;

		do
        {
            if((nbuf = strstr(buf, "%;")) != NULL || (nbuf = strstr(buf, "\r\n")) != NULL)
            {
                *nbuf = '\0';
                nbuf += 2;
            }
            else if((nbuf = strstr(buf, "%25;")) != NULL || (nbuf = strstr(buf, "\r\n")) != NULL)
            {
                *nbuf = '\0';
                nbuf += 4;
            }

			unescape_string(single_cmd, buf);
			start = single_cmd;
			if(*start == ' ')
        	{
            	start++;
            	continue;
        	}

            len = strlen(start);
            if(len < 1)
                break;

			if(aggr_cdb)
			{
				if(!strncmp(start, "echo ", 5))
				{
					strcpy(delimiter, start+5);
					continue;
				}
				else if(*start == '$')
				{
					do_aggr_cmd(&aggr_cmd_end, start, len, multiple_cmd);
					continue;
				}
				else if((!strncmp(start, "status ", 7)) && ((i = find_status_idx(start+7)) >= 0))
				{
					len = strlen(netstatus_tab[i].rdb_name);
					do_aggr_cmd(&aggr_cmd_end, netstatus_tab[i].rdb_name, len, multiple_cmd);
					continue;
				}
				else if((!strncmp(start, "commit state", 13)))
				{
					len = 10;
					do_aggr_cmd(&aggr_cmd_end, "$web_state", len, multiple_cmd);
					continue;
				}
				else
				{
					cdb_multiple_command(multiple_cmd);
					aggr_cdb = 0;
				}
			}
			else
			{
				aggr_cmd_end = multiple_cmd;

				if(*start == '$')
				{
					aggr_cdb = 1;
					aggr_cmd(&aggr_cmd_end, start, len, 0);
					continue;
				}
				else if((!strncmp(start, "status ", 7)) && ((i = find_status_idx(start+7)) >= 0))
				{
					aggr_cdb = 1;
					len = strlen(netstatus_tab[i].rdb_name);
					aggr_cmd(&aggr_cmd_end, netstatus_tab[i].rdb_name, len, 0);
					continue;
				}
				else if((!strncmp(start, "commit state", 13)))
				{
					aggr_cdb = 1;
					len = 10;
					aggr_cmd(&aggr_cmd_end, "$web_state", len, 0);
					continue;
				}
			}

				
            //strncpy(cmd_buf, buf, MAX_CMD_SIZE);
            //cmd_buf[MAX_CMD_SIZE-1] = '\0';

            //DBG("%s:%d =>%s\n", __func__, __LINE__, cmd_buf);

            //unescape_string(single_cmd, cmd_buf);
            cli_process_single_cmd(single_cmd);

        }
		while((nbuf!=NULL) && (buf = nbuf));

		if(aggr_cdb)
		{
			cdb_multiple_command(multiple_cmd);
		}
    }
    
Exit:

    return 0;
}
