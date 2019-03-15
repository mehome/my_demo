#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include "include/cdb.h"

int main(int argc, char *argv[])
{
    char name[CDB_NAME_MAX_LENGTH];
    char _line_buf[CDB_LINE_BUF_SIZE+1];
    char *line_buf = &_line_buf[1];
    char buf[CDB_DATA_MAX_LENGTH];
    int ret;
    char *p;

    if(argc>1)
    {
        if(!strcmp(argv[1], "fload"))
        {
            cdb_init();
            return 0;
        }
        else if(!strcmp(argv[1], "fclear"))
        {
            cdb_delete_db();
            return 0;
        }
    }

    if(0>cdb_attach_db())
        return 0;

    _line_buf[0] = '$';
    line_buf[0] = '\0';

    if(argc>1)
    {
        if(!strcmp(argv[1], "dump"))
        {
            cdb_dump_text_db(NULL);
        }
        else if(!strcmp(argv[1], "get") && (argc >= 3))
        {
            snprintf(name, CDB_NAME_MAX_LENGTH, "$%s", argv[2]);
            ret = cdb_get(name, line_buf);
            if(0 > ret)
                printf("!ERR\n");
            else
                printf("%s\n", line_buf);
        }
        else if(!strcmp(argv[1], "unset") && (argc >= 3))
        {
            snprintf(name, CDB_NAME_MAX_LENGTH, "$%s", argv[2]);
            ret = cdb_unset(name);
            if(0 > ret)
                printf("!ERR\n");
            else
                printf("!OK(%d)\n", ret);
        }
        else if(!strcmp(argv[1], "set") && (argc >= 4))
        {
            sprintf(name, "$%s", argv[2]);
            ret = cdb_set(name, argv[3]);
            if(0 > ret)
                printf("!ERR\n");
            else
                printf("!OK(%d)\n", ret);
        }
        else if(!strcmp(argv[1], "commit"))
        {
            cdb_save();
        }
        else if(!strcmp(argv[1], "save"))
        {
            cdb_save();
        }
        else if(!strcmp(argv[1], "fsave"))
        {
            cdb_force_save();
        }
        else if(!strcmp(argv[1], "reset"))
        {
            if (!cdb_reset())
                printf("CDB Reset.\n");
        }
        else if(!strcmp(argv[1], "fsh"))
        {
            if(!cdb_service_find_change(buf))
                printf("List of service with configuration change:\n(%s)\n", buf);
        }
        else if(!strcmp(argv[1], "ssh") && (argc >= 3))
        {
            if(!cdb_service_set_change(argv[2]))
                printf("Set configuration change of service '%s'.\n", argv[2]);
        }
        else if(!strcmp(argv[1], "csh") && (argc >= 3))
        {
            if(!cdb_service_clear_change(argv[2]))
                printf("Clear configuration change of service '%s'.\n", argv[2]);
        }
        else if(!strcmp(argv[1], "ulck"))
        {
            cdb_debug_force_unlock();
            printf("Force unlock\n");
        }
        else if(!strcmp(argv[1], "dlck"))
        {
            printf("Lock information: %d(%s) %d(%s)\n",
                   cdb_debug_get_lock_holder(), cdb_debug_get_variable_name(),
                   cdb_debug_get_op_code(), cdb_debug_get_process_name());
        }
        else
        {
            /* print usage here */
            printf("Configuration/Runtime database maintenance utility\n"
                   "Usage:\n"
                   "         get <cdb variable>\n"
                   "         set <cdb variable> <value>\n"
                   "         unset <cdb variable>\n"
                   "         dump              - dump DB in text format\n"
                   "         save              - save DB to flash\n"
                   "         reset             - reset DB in flash\n"
                   "         fsave             - force DB save\n"
                   "         fclear            - force DB clear in memory\n"
                   "         fload             - force DB load from flash\n"
                   "         fsh               - find service change\n"
                   "         ssh <cdb service> - set service change\n"
                   "         csh <cdb service> - clear service change\n"
                   "         dlck              - dump lock information\n"
                   "         ulck              - force unlock\n");
        }

        return 0;
    }

    while(1)
    {
        printf("db>");

        if(NULL==fgets(line_buf, CDB_LINE_BUF_SIZE, stdin))
            break;

        line_buf[strlen(line_buf)-1] = '\0';

        if(strlen(line_buf)==0)
            break;

        //if((!strcmp(line_buf, "quit")) || (!strcmp(line_buf, "q")) || (!strcmp(line_buf, "exit")))
        //    break;

        if(line_buf[0]=='#')
            continue;

        if(!strcmp(line_buf,"$"))
        {
            cdb_dump_text_db(NULL);
            continue;
        }

        if((p = strstr(line_buf, "=")))
        {
            *p = '\0';
            p++;

            if(line_buf[0]!='$')
                ret = cdb_set(_line_buf, p);
            else
                ret = cdb_set(line_buf, p);

            if(0 > ret)
                printf("!ERR\n");
            else
                printf("!OK(%d)\n", ret);
        }
        else
        {
            if(line_buf[0]=='!')
            {
                line_buf[0] = '$';
                ret = cdb_unset(line_buf);

                if(0 > ret)
                    printf("!ERR\n");
                else
                    printf("%s removed\n", line_buf);
            }
            else
            {
                if(line_buf[0]!='$')
                    ret = cdb_get(_line_buf, buf);
                else
                    ret = cdb_get(line_buf, buf);

                if(0 > ret)
                    printf("!ERR\n");
                else
                    printf("%s\n", buf);
            }
        }
    }

    return 0;
}

