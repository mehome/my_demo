#include "include/mtdm_client.h"
#include "wdk.h"
#include "cdb.h"

#define UHTTPD_NAME     "uhttpd"
#define UHTTPD_CONFILE  "/tmp/httpd.conf"

/*

    Translate format  from:
        user=admin&pass=21232F297A57A5A743894A0E4A801FC3
    To:
        /:admin:21232F297A57A5A743894A0E4A801FC3

*/
static void translate_format(char *string_in, char *string_out)
{
    char *p1 = NULL;
    char *p2 = NULL;

    p1 = strstr(string_in, "user=");
    p2 = strstr(string_in, "pass=");
    *(p2 - 1) = 0;

    if ((p1 != NULL) && (p2 != NULL)) {
        strcat(string_out, "/:");
        strcat(string_out, p1 + 5);
        strcat(string_out, ":");
        strcat(string_out, p2 + 5);
    }
}



/*
    refresh user:pass to /tmp/httpd.conf
*/
int wdk_mangment(int argc, char **argv)
{
    char httpd_config[100] = {0};
    char cdb_config[100] = {0};
    char cdb_config_new[100] = {0};
    int n;


    if (argc != 0) {
        LOG("no argument needed");
        return -1;
    }

    if (0 == access(UHTTPD_CONFILE, 0)) {
        readline(UHTTPD_CONFILE, httpd_config, sizeof(httpd_config));
        n = strlen(httpd_config);
        if (httpd_config[n - 1] == '\n') {
            httpd_config[n - 1] = 0;
        }
        //LOG("From http.conf=[%s]", httpd_config);

        cdb_get("$sys_user1", cdb_config);
        //LOG("           cdb=[%s]", cdb_config);

        translate_format(cdb_config, cdb_config_new);
        //LOG("       cdb_new=[%s]", cdb_config_new);

       // LOG(" ");

        if (0 == strcmp(cdb_config_new, httpd_config)) {
            //LOG("Configuration are same. nothing to be done");
        } else {
            LOG("overriding http.conf...");
            writeline(UHTTPD_CONFILE, cdb_config_new);
            mtdm_client_simply_call(OPC_CMD_HTTP, OPC_CMD_ARG_RESTART, NULL);
        }
    } else {
        LOG("%s not found!!!", UHTTPD_CONFILE);
    }

    return 0;
}






