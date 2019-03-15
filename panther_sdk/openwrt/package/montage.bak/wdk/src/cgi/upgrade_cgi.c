#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <errno.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))

#define WDK_UPGRADE_SCRIPT        "/lib/wdk/sysupgrade"
#define WDK_CDBUPGRADE_SCRIPT     "/lib/wdk/cdbupload"

FILE *debugfp = NULL;

#define CDB_COMMAND_SIZE          512
#define CDB_PROCFS_FILENAME       "/proc/cdb"
#define TEMP_FIRMWARE_FILENAME    "/tmp/firmware"
#define TEMP_CONFIG_CDB_FILENAME  "/tmp/config.cdb"
#define DISK_INFO_FILENAME        "/var/diskinfo.txt"

#define CGI_DEBUG_LOG             "/tmp/upgrade-cgi.log"
#define CGI_DEBUG_LOG_1           "/tmp/upgrade-cgi.log.1"
#define CGI_DEBUG_LOG_MAXSIZE     4096
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

#define EXEC_ERR_NOIMPL     -1
#define EXEC_ERR_INTERNAL   -2
#define EXEC_ERR_EXECUTION  -3

int exec_sysupgrade(char *firmware_pathname)
{
    int fd;
    pid_t child;

    if((fd=open("/dev/console", O_RDWR))<0)
        return EXEC_ERR_INTERNAL;

	switch( (child = fork()) )
    {
		/* oops */
        case -1:
            DBG("fork failure\n");
            close(fd);
			return EXEC_ERR_INTERNAL;
            break;

		/* exec child */
        case 0:
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            setsid();
            execl("/sbin/sysupgrade", "-v",  firmware_pathname, NULL);
            exit(EXEC_ERR_INTERNAL);

            return EXEC_ERR_INTERNAL;
            break;

        default:
            close(fd);
            break;
    }

    return 0;
}
int exec_script(char *cmd)
{
    char *p = cmd;
    struct stat sb;
    int fd;
    pid_t child;

    int __argc = 0;
    char*  __argv[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };

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

    if(stat(cmd, &sb) == -1)
        return EXEC_ERR_NOIMPL;

    if((fd=open("/dev/null", O_RDWR))<0)
        return EXEC_ERR_INTERNAL;

	switch( (child = fork()) )
    {
		/* oops */
        case -1:
            DBG("fork failure\n");
            close(fd);
			return EXEC_ERR_INTERNAL;
            break;

		/* exec child */
        case 0:
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            setsid();
            execv(cmd, (char **const) &__argv);
            exit(EXEC_ERR_INTERNAL);

            return EXEC_ERR_INTERNAL;
            break;

        default:
            close(fd);
            break;
    }

    return 0;
}

#define READ_BUFFER_SIZE    4096
#define BOUNDRY_STRING      "boundary="
#define FILETYPE_STRING     "filetype"
#define FILEID_STRING       "fileid"
#define FILEPATH_STRING     "filepath"
#define FILENAME_STRING     "filename"

#define FILEID_MAX          8+1

#define CHAR_LF 0xa
#define CHAR_CR 0xd

#define FT_CDB      '0'
#define FT_FIRMWARE '1'
#define FT_REGULAR  '2'

#define WEB_STATE_DEFAULT 0
#define WEB_STATE_ONGOING 1
#define WEB_STATE_SUCCESS 2
#define WEB_STATE_FAILURE 3

#define WEB_UPLOAD_STATE "/tmp/upload_state"

#if 0
int cdb_set(char *name, void *value)
{
    char command[CDB_COMMAND_SIZE];
    FILE *cdb_fp = NULL;
    int ret = -1;

    if(!(cdb_fp = fopen(CDB_PROCFS_FILENAME, "r+")))
        goto Exit;

    fseek(cdb_fp, 0L, SEEK_SET);
    snprintf(command, CDB_COMMAND_SIZE, "%s=%s", name, (char *)value);
    command[CDB_COMMAND_SIZE-1] = '\0';

    ret = fwrite(command, 1, strlen(command), cdb_fp);

    while(fgets(command, CDB_COMMAND_SIZE, cdb_fp)) ;

Exit:
    if(cdb_fp)
        fclose(cdb_fp);

    return (ret < 0) ? -1 : 0;
}
void update_web_state(int state)
{
	char value[5];
	snprintf(value, sizeof(value), "%d", state);
	value[sizeof(value)-1] = '\0';
	cdb_set("$web_state", value);
}
#else
void update_upload_state(int state, char *ip, char *id)
{
    FILE *fp = NULL;
    if( (fp = fopen(WEB_UPLOAD_STATE, "a+")) )
    {
        fprintf(fp, "%s %s %u %u\n", ip, id, state, time(NULL));
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
        DBG("%s: state=%d ip=%s id=%s\n", __func__, state, ip, id);
    }
}
#endif

void truncate_file(char *path, int writelen)
{
    FILE *fp = NULL;
    if( (fp = fopen(path, "r+")) )
    {
        ftruncate(fileno(fp), writelen);
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
        DBG("truncate %s\n", path);
    }
}

int check_permission(char *path)
{
    FILE *fp = NULL;
    char line[2048];
    int ret = -1;
    int len;

    DBG("check permission for file [%s] request\n", path);
    if( (fp = fopen(DISK_INFO_FILENAME, "r")) )
    {
        while(1)
        {
            if(fgets(line, sizeof(line), fp) == NULL) break; /* end-of-file */
            len = strlen(line);
            if(line[len - 1] == '\n')
                line[--len] = '\0';
            DBG("disk [%s]\n", line);
            if(!strncmp(path, line, len))
            {
                ret = 0;
                break;
            }
        }
        fclose(fp);
    }
    if(!ret)
        DBG("check PASS\n");
    else
        DBG("check FAIL\n");
    return ret;
}

#define HTTPD_QUERYVAR_MAX 10
struct request_info {
    char file_type;
    char file_id[FILEID_MAX];
    char path_info[PATH_MAX];
    char name_info[PATH_MAX];
    char path_phys[PATH_MAX];

    char root_path[PATH_MAX];
    char remote_ip[INET6_ADDRSTRLEN];
    char boundary[PATH_MAX];
    unsigned long blen;
    unsigned long len;
    struct http_va {
        unsigned char *name;
        unsigned char *val;
    } http_va[HTTPD_QUERYVAR_MAX];
};

char *httpd_check_pattern(char *data, int len, char *pattern)
{
    int offset=0;

    while(offset < len)
    {  
        if(memcmp( data+offset, pattern, strlen(pattern)) == 0)
            return data+offset;
        else
            offset++;
    }
    return 0;

}

void httpd_set_var(char *name, char *val, struct request_info *info)
{
    int i;
    struct http_va *var;
    
    for(var = &info->http_va[0], i=0; var->name && (i < HTTPD_QUERYVAR_MAX); var++, i++);
    
    if(i >= HTTPD_QUERYVAR_MAX)
    {   
        printf("Query table is not enough!");
        return;
    }   
     
    var->name = name;
    var->val = val;
}

char * httpd_process_multipart(char *buf, int len, struct request_info *info)
{
    char null_line[] = { CHAR_LF, CHAR_LF, 0 };
    char null_line1[] = { CHAR_CR, CHAR_LF, CHAR_CR, CHAR_LF, 0 };
    char *ptr = 0;
    char *start = buf;
    char *next;
    char *var;

    while (len > 0) {
        if(!(next = httpd_check_pattern(start, len, info->boundary)))
            break;

        if(next-4 > buf) {
            *(next-4) = '\0';
        }

        next += info->blen;
        if((*next == '-') && *(next+1)=='-')
            break;

        next += 2;
        len -= (next - start);
        start = next;

        if (!strncasecmp(start, "Content-Disposition:", 20) ) {
            if((var = httpd_check_pattern(start, len, "name=\"")) != NULL ) {
                if((ptr = httpd_check_pattern(start, len, null_line1)) != NULL) 
                    ptr = ptr + strlen(null_line1);
                else if((ptr = httpd_check_pattern(start, len, null_line)) != NULL) 
                    ptr = ptr + strlen(null_line);
                else {
                    DBG("ERR: no null line in multipart/form-data\n");
                    return 0;
                }

                var += 6;
                if((next = strchr( var, '"')) != 0) {
                    *next = '\0';
                    httpd_set_var(var, ptr, info);
                }
            }
        }
    }

    return ptr;
}

int main(int argc, char **argv)
{
    char read_buf[READ_BUFFER_SIZE+1];
    struct request_info info;
    struct http_va *var;
    FILE *output_fp = NULL;
    char *filename = NULL;
    char *buf;
    unsigned long len;
    unsigned long blen;
    unsigned long readlen = 0;
    unsigned long writelen = 0;
    int short_file = 0;
    int i;

    read_buf[READ_BUFFER_SIZE] = '\0';

    unlink(TEMP_FIRMWARE_FILENAME);

    memset(&info, 0, sizeof(struct request_info));
    info.file_type = FT_CDB;
    buf = getenv("DOCUMENT_ROOT");
    if(buf) {
        blen = strlen(buf);
        memcpy(info.root_path, buf, min(blen, sizeof(info.root_path)-1));
        info.root_path[blen] = '/';
    }

    buf = getenv("CONTENT_LENGTH");
    if(buf)
    {
        info.len = atoi(buf);
        DBG("POST\n");
        DBG("CONTENT_LENGTH %d\n", info.len);

        buf = getenv("REMOTE_ADDR");
        if(buf == NULL)
            goto Exit;
        DBG("REMOTE_ADDR %s\n", buf);
        strncpy(info.remote_ip, buf, sizeof(info.remote_ip));

        buf = getenv("CONTENT_TYPE");
        if(buf == NULL)
            goto Exit;
        DBG("CONTENT_TYPE %s\n", buf);
        if(!strstr(buf, "multipart/form-data"))
            goto Exit;
        if(NULL == (buf = strstr(buf, BOUNDRY_STRING)))
            goto Exit;
        else
            buf += strlen(BOUNDRY_STRING);
        strcpy(info.boundary, buf);
        info.blen = strlen(buf);

        while(1)
        {
            len = fread(read_buf, 1, READ_BUFFER_SIZE, stdin);

            if(len < 0) {
                DBG("ERR: read error %d\n", len);
                goto Exit;
            }
            if(len == 0)
                break;

            if(readlen == 0)
            {
                /*
                 * IMPORTANT for multipart/form-data 
                 * Content-Disposition: form-data; name="files" must be arranged in the end part
                 * Otherwise, we can't dertermine what kind of FILE is uploading now
                 */
                if(NULL == (buf = httpd_process_multipart(read_buf, len, &info))) {
                    DBG("ERR: no multipart or failed to parse\n");
                    goto Exit;
                }

                for(var = &info.http_va[0], i=0; var->name && (i < HTTPD_QUERYVAR_MAX); var++, i++) {
                    if (!strcmp(var->name, FILETYPE_STRING)) {
                        info.file_type = var->val[0];
                    }
                    else if (!strcmp(var->name, FILEID_STRING)) {
                        strncpy(info.file_id, var->val, min(FILEID_MAX-1, strlen(var->val)));
                    }
                    else if (!strcmp(var->name, FILEPATH_STRING)) {
                        if (var->val[0] == '/')
                            var->val++;
                        strncpy(info.path_info, var->val, min(PATH_MAX-1, strlen(var->val)));
                    }
                    else if (!strcmp(var->name, FILENAME_STRING)) {
                        if (var->val[0] == '/')
                            var->val++;
                        strncpy(info.name_info, var->val, min(PATH_MAX-1, strlen(var->val)));
                    }
                    else {
                        continue;
                    }
                    
                    DBG("name = %s value = %s\n", var->name, var->val);
                }

                if(info.file_id[0] == 0) {
                    DBG("no file id, set to default '0'\n");
                    info.file_id[0] = '0';
                }
                setenv("FILE_ID", info.file_id, 1);
                if(info.file_type == FT_REGULAR) {
                    if(!info.path_info[0] || !info.name_info[0] || 
                      ((strlen(info.path_info) + strlen(info.name_info) + 2) > PATH_MAX)) {
                        DBG("ERR: no filename or filename too large\n");
                        info.path_info[0] = 0;
                        goto Exit;
                    }
                    filename = strrchr(info.name_info, '/');
                    if (filename) {
                        *filename++ = '\0';
                        strcat(info.path_info, "/");
                        strcat(info.path_info, info.name_info);
                    }
                    else
                        filename = info.name_info;
                    if (!realpath(info.path_info, info.path_phys)) {
                        DBG("ERR: get realpath from [%s]\n", info.path_info);
                        info.path_info[0] = 0;
                        goto Exit;
                    }
                    DBG("request filename [%s]:[%s]\n", info.path_info, filename);
                    DBG("real    filename [%s]:[%s]\n", info.path_phys, filename);
                    strcat(info.path_phys, "/");
                    strcat(info.path_phys, filename);
                    strcpy(info.path_info, info.path_phys);
                    if(check_permission(info.path_info)) {
                        info.path_info[0] = 0;
                        goto Exit;
                    }
                }
                else
                    strcpy(info.path_info, TEMP_FIRMWARE_FILENAME);

                if(NULL == (output_fp = fopen(info.path_info, "w"))) {
                    DBG("ERR: open temp. firmware failure\n");
                    goto Exit;
                }
                if(0 == fwrite(buf, len - (buf - read_buf), 1, output_fp)) {
                    DBG("ERR: write error\n");
                    goto Exit;
                }
                writelen += len - (buf - read_buf);

                update_upload_state(WEB_STATE_ONGOING, info.remote_ip, info.file_id);
            }
            else
            {
                if(0 == fwrite(read_buf, len, 1, output_fp)) {
                    DBG("ERR: write error\n");
                    goto Exit;
                }
                writelen += len;
            }
            readlen += len;
        }

        DBG("total read %d, write %d\n", readlen, writelen);
        if(readlen < info.len) {
            DBG("readlen(%d) < CONTENT_LENGTH(%d)\n", readlen, info.len);
            goto Exit;
        }

        fclose(output_fp);
        output_fp = NULL;


        /* trim the output file to proper boundary */
        if(NULL == (output_fp = fopen(info.path_info, "r"))) {
            DBG("ERR: open temp. firmware failure\n");
            goto Exit;
        }

        if(0 > fseek(output_fp, -READ_BUFFER_SIZE, SEEK_END)) {
            if(0 > fseek(output_fp, 0, SEEK_SET))
                goto Exit;
            else
                short_file = 1;
        }

        if(0 > (len = fread(read_buf, 1, READ_BUFFER_SIZE, output_fp))) {
            DBG("ERR: read error\n");
            goto Exit;
        }

        if(NULL == (buf = httpd_check_pattern(read_buf, len, info.boundary))) {
            DBG("ERR: no end boundary\n");
            goto Exit;
        }
        buf -= 4;

        if(short_file)
            writelen = (buf - read_buf);
        else
            writelen -= (READ_BUFFER_SIZE - (buf - read_buf));

        fclose(output_fp);
        output_fp = NULL;

        DBG("file_size %d\n", writelen);

        truncate_file(info.path_info, writelen);

        DBG("file_type %c\n", info.file_type);
        if(FT_CDB == info.file_type) {
            rename(TEMP_FIRMWARE_FILENAME, TEMP_CONFIG_CDB_FILENAME);
            DBG("call %s\n", WDK_CDBUPGRADE_SCRIPT);
            exec_script(WDK_CDBUPGRADE_SCRIPT);
        }
        else if(FT_FIRMWARE == info.file_type) {
            //DBG("call %s\n", WDK_UPGRADE_SCRIPT);
            //exec_script(WDK_UPGRADE_SCRIPT);  // Cheetah used, and will run /lib/wdk/sysupgrade

            exec_sysupgrade(TEMP_FIRMWARE_FILENAME);     // Panther used, running sysupgrade by scripts of user space
        }
        else if(FT_REGULAR == info.file_type) {
            update_upload_state(WEB_STATE_SUCCESS, info.remote_ip, info.file_id);
            DBG("OK: upload %s\n", info.path_info);
        }
        else {
            DBG("ERR: unknown file type\n");
            goto Exit;
        }
    }
    else {
        DBG("ERR: No CONTENT_LENGTH\n");
        goto Exit;
    }

    return 0;


Exit:
    if(output_fp)
        fclose(output_fp);

    if(info.path_info[0] != 0) {
        DBG("ERR: remove file: [%s]\n", info.path_info);
        unlink(info.path_info);
    }

    DBG("ERR: upgrade failed\n");

    if((info.remote_ip[0] != 0) && (info.file_id[0] != 0)) {
        update_upload_state(WEB_STATE_FAILURE, info.remote_ip, info.file_id);
    }

    return -1;
}


