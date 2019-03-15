#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <wdk/cdb.h>
#include <libutils/shutils.h>

#define KEY_SIZE    CDB_NAME_MAX_LENGTH
#define VAL_SIZE    CDB_DATA_MAX_LENGTH
#define AP_MAX      10

/*SEC TYPE*/
#define WEP "wep"
#define WPA "wpa"

/*CIPHER TYPE*/
#define TKIP "TKIP"
#define CCMP "CCMP"
#define TKIP_CCMP "TKIP CCMP"

#define SSID 1
#define PASS 2

char debug;

static void ap_list_first_rule(void)
{
    char buf[VAL_SIZE] = {0};
    char ssid[128] = {0};
    char pass[128] = {0};
    char bssid[32] = {0};
    char sec[20] = {0};
    char cipher[32] = {0};
    char key[256] = {0};
    char val[4];
    int t = 0;
    int c = 0;
    int mode;
    int v;
    int wpa;
    int wpa2;
    int wep;

#if 0
    /*Clear First Rule*/
    cdb_unset("$wl_aplist_info1");
    cdb_unset("$wl_aplist_pass1");
#endif

    cdb_get("$wl_bss_ssid2", ssid);
    cdb_get("$wl_bss_bssid2", bssid);

    cdb_get("$wl_bss_sec_type2", val);

    v = t = atoi(val);
    if (v != 0)
    {
        wep = v&4;
        wpa = v&8; 
        wpa2 = v&16; 

        if (wep > 0)
        {
            cdb_get("$wl_bss_wep_index2", val);
            v = atoi(val);
            sprintf(key, "$wl_bss_wep_%dkey2", ++v);

            cdb_get(key,pass);
            strcpy(sec, WEP);
        } else if (wpa || wpa2)
        {
            strcpy(sec, WPA);
            cdb_get("$wl_bss_wpa_psk2",pass);

            cdb_get("$wl_bss_cipher2",val);
            v = c = atoi(val);

            if ((v & 4) && (v & 8))
            {
                strcpy(cipher, TKIP_CCMP);
            } else if (v & 4)
            {
                strcpy(cipher, TKIP);
            } else if (v & 8)
            {
                strcpy(cipher, CCMP);
            }
        } else
        {
            strcpy(sec, "none");
        }
    } else
    {
        strcpy(sec, "none");
    }

    mode = cdb_get_int("$op_work_mode", 3);
    sprintf(buf, "bssid=%s&sec=%s&cipher=%s&last=1&mode=%d&tc=%d:%d&ssid=%s", bssid, sec, cipher, mode, t, c, ssid);
    cdb_set("$wl_aplist_info1", buf);

    memset(buf, 0, VAL_SIZE);
    sprintf(buf, "%s",pass);
    cdb_set("$wl_aplist_pass1", buf);
}

static int aplist_info_parser(char *buf, char **argv)
{
    int num = str2argv(buf, argv, '&');

    if (num < 7)
    {
        num = 0;
    }
    if (!str_arg(argv, "bssid=")  || 
        !str_arg(argv, "sec=")    || 
        !str_arg(argv, "cipher=") || 
        !str_arg(argv, "last=")   ||
        !str_arg(argv, "mode=")   ||
        !str_arg(argv, "tc=")     ||
        !str_arg(argv, "ssid="))
    {
        num = 0;
    }

    return num;
}

static int aplist_get_info(int idx, char *buf, int buflen, char **argv)
{
    memset(buf, 0, buflen);
    if (cdb_get_array_str("$wl_aplist_info", idx, buf, buflen, NULL) != NULL)
    {
        return aplist_info_parser(buf, argv);
    }
    return 0;
}

static void aplist_restructure(void)
{
    char key[KEY_SIZE];
    char buf[VAL_SIZE];
    char *argv[10];
    int idx;
    int num;

    for (idx=1; idx <= AP_MAX; idx++)
    {
        if ((num = aplist_get_info(idx, buf, VAL_SIZE, argv)) == 0)
        {
            // item is broken
            if (idx < AP_MAX)
            {
                sprintf(key, "$wl_aplist_info%d", idx);
                cdb_get_array("$wl_aplist_info", idx + 1, buf);
                cdb_set(key, buf);
                sprintf(key, "$wl_aplist_pass%d", idx);
                cdb_get_array("$wl_aplist_pass", idx + 1, buf);
                cdb_set(key, buf);

                sprintf(key, "$wl_aplist_info%d", idx + 1);
                cdb_unset(key);
            } else
            {
                sprintf(key, "$wl_aplist_info%d", idx);
                cdb_unset(key);
                sprintf(key, "$wl_aplist_pass%d", idx);
                cdb_unset(key);
            }
        }
    }
}

static int aplist_total(void)
{
    int idx;
    char buf[VAL_SIZE];

    for (idx=1; idx <= AP_MAX; idx++)
    {
        memset(buf, 0, VAL_SIZE);
        if (cdb_get_array_str("$wl_aplist_info", idx, buf, VAL_SIZE, NULL) == NULL)
        {
            break;
        }
        if (!strlen(buf))
        {
            break;
        }
    }
    return idx-1;
}

static void aplist_getitem(int idx, int type)
{
    char buf[VAL_SIZE];
    char *argv[10];
    char *p;
    char *ptr;
    char *ssid;
    char *sec;
    char *cipher;
    char *bssid;
    unsigned int len = 0;
    char num;

    if (idx > AP_MAX || idx <= 0)
    {
        return;
    }

    memset(buf, 0, VAL_SIZE);
    if (type == SSID)
    {
        if (cdb_get_array_str("$wl_aplist_info", idx, buf, VAL_SIZE, NULL) == NULL)
        {
            return;
        }
        if (!strlen(buf))
        {
            return;
        }
        num = str2argv(buf, argv, '&');

        if (num < 7)
        {
            return;
        }
        if (!str_arg(argv, "bssid=")  || 
            !str_arg(argv, "sec=")    || 
            !str_arg(argv, "cipher=") || 
            !str_arg(argv, "last=")   ||
            !str_arg(argv, "mode=")   ||
            !str_arg(argv, "tc=")     ||
            !str_arg(argv, "ssid="))
        {
            return;
        }

        ssid  = (char *)str_arg(argv, "ssid=");
        printf("%s\n",ssid);
    } else if (type == PASS)
    {
        cdb_get_array("$wl_aplist_pass", idx, buf);
        printf("%s\n",buf);
    }
}

static void aplist_show(void)
{
    int idx;
    char buf[VAL_SIZE];
    char *argv[10];
    char *p;
    char *ptr;
    char *ssid;
    char *sec;
    char *cipher;
    char *bssid;
    unsigned int len = 0;
    char num;

    for (idx=1; idx <= AP_MAX; idx++)
    {
        memset(buf, 0, VAL_SIZE);
        if (cdb_get_array_str("$wl_aplist_info", idx, buf, VAL_SIZE, NULL) == NULL)
        {
            break;
        }
        printf("buf:%s\n",buf);
        if (!strlen(buf))
        {
            break;
        }

        num = str2argv(buf, argv, '&');

        if (num < 7)
        {
            continue;
        }
        if (!str_arg(argv, "bssid=") || 
            !str_arg(argv, "sec=") || 
            !str_arg(argv, "cipher=") || 
            !str_arg(argv, "last=") ||
            !str_arg(argv, "mode=")   ||
            !str_arg(argv, "tc=")     ||
            !str_arg(argv, "ssid="))
        {
            continue;
        }

        bssid = (char *)str_arg(argv, "bssid=");
        sec   = (char *)str_arg(argv, "sec=");
        cipher= (char *)str_arg(argv, "cipher=");
        ssid  = (char *)str_arg(argv, "ssid=");

        printf("idx:%d\n",idx);
        printf("ssid=%s\n",ssid);
        if (bssid==NULL)
        {
            printf("NO bssid info\n");
        } else
        {
            printf("bssid=%s\n",bssid);
        }

        if (!strcmp(sec,"none"))
        {
            printf("No security\n\n");
        } else
        {
            printf("sec:%s\n",sec);
            printf("cipher:%s\n",cipher);
            cdb_get_array("$wl_aplist_pass", idx, buf);
            printf("pass:%s\n\n",buf);
        }
    }
}

static void aplist_modify(unsigned int index) 
{
    char ssidval[33] = {0};
    char bssidval[20] = {0};
    char key[KEY_SIZE] = {0};
    unsigned int idx = 0;
    char buf[VAL_SIZE] = {0};
    char *p;
    char *ptr;
    char *ssid;
    char *pass;
    char *bssid;
    char matchidx = 0, curmax = 0;
    unsigned int len = 0;

    /*compare and modify*/
    if (index == 0)
    {
        if (cdb_get("$wl_bss_ssid2", ssidval) < 0)
        {
            if (debug)
            {
                goto debug;
            }
            return;
        }
        cdb_get("$wl_bss_bssid2", bssidval);

        for (idx=1; idx <= AP_MAX; idx++)
        {
            if (cdb_get_array("$wl_aplist_info", idx, buf) < 0)
            {
                if (debug)
                {
                    printf("idx:%d has no data\n",idx);
                }
                break;
            }
            if (!strlen(buf))
            {
                break;
            }
            p = buf;
            len = 0;
            bssid = cdb_get_array_val("bssid", 2, CDB_GENR_DELIM, p+=len, &len, NULL);
            ssid = cdb_get_array_val("ssid", 2, CDB_LAST_DELIM, p+=len, &len, NULL);

            if (!strcmp(ssid, ssidval)) //ssid match
            {
                if (strlen(bssidval) == 0) // omnicfg
                {
                    matchidx = idx;
                } else if (!strcmp(bssid, bssidval))
                {
                    matchidx = idx;
                }
            }
            memset(buf, 0, VAL_SIZE);
        }

        curmax = (idx>AP_MAX)? AP_MAX:idx;

        //no match
        if (!matchidx)
        {
            /*
                Move current rule to next rule
                From end to start
            */
            for (idx = curmax; idx > 0; idx--)
            {
                if (curmax == AP_MAX)
                {
                    continue;
                }

                memset(buf, 0, VAL_SIZE);
                cdb_get_array("$wl_aplist_info", idx, buf);
                if (!strlen(buf))
                {
                    continue;
                }

                if (idx == 1)
                {
                    ptr = strstr(buf, "&last=");
                    if (ptr)
                    {
                        ptr+=6;
                        ptr[0]='0';
                    }
                }
                /*bss info*/
                memset(key, 0, KEY_SIZE);
                sprintf(key, "%s%d", "$wl_aplist_info", idx+1);
                cdb_set(key, buf);

                /*bss passwd*/
                memset(key, 0, KEY_SIZE);
                memset(buf, 0, VAL_SIZE);
                cdb_get_array("$wl_aplist_pass", idx, buf);
                /*
                    Whether passwd exist or not we must set 
                    coz its may have the prev but need to remove.
                */
                sprintf(key, "%s%d", "$wl_aplist_pass", idx+1);
                cdb_set(key, buf);
            }
        } else
        {
            for (idx = matchidx-1; idx > 0; idx--)
            {
                memset(buf, 0, VAL_SIZE);
                cdb_get_array("$wl_aplist_info", idx, buf);

                if (!strlen(buf))
                {
                    printf("It is strange, why the prev rule is empty.\n");
                    continue;
                }

                if (idx == 1)
                {
                    ptr = strstr(buf, "&last=");
                    if (ptr)
                    {
                        ptr+=6;
                        ptr[0]='0';
                    }
                }

                memset(key, 0, KEY_SIZE);
                sprintf(key, "%s%d", "$wl_aplist_info", idx+1);
                cdb_set(key, buf);

                /*bss passwd*/
                memset(key, 0, KEY_SIZE);
                memset(buf, 0, VAL_SIZE);
                cdb_get_array("$wl_aplist_pass", idx, buf);
                /*
                    Whether passwd exist or not we must set 
                    coz its may have the prev but need to remove.
                */
                sprintf(key, "%s%d", "$wl_aplist_pass", idx+1);
                cdb_set(key, buf);
            }
        }

        ap_list_first_rule();
        debug:  
        if (debug)
        {
            if (matchidx > 1)
            {
                printf("Need to move %d to high priority\n",matchidx);
            } else if (matchidx == 0)
            {
                printf("New AP join\n");
            }
        }

        return;
    }
    /*Delete index info*/
    else
    {
        for (idx=1; idx <= AP_MAX; idx++)
        {
            if (cdb_get_array("$wl_aplist_info", idx, buf) < 0)
            {
                break;
            }
            if (!strlen(buf))
            {
                break;
            }
        }

        curmax = (idx>AP_MAX)? AP_MAX:idx;

        for (idx = index; idx <= curmax; idx++)
        {
            char key2[KEY_SIZE] = {0};

            memset(buf, 0, VAL_SIZE);
            /*Remove index*/
            sprintf(key, "%s%d", "$wl_aplist_info", idx);
            cdb_unset(key);

            sprintf(key2, "%s%d", "$wl_aplist_info", idx+1); //next index info
            if (cdb_get(key2, buf) < 0)
            {
                /*No next rule exist*/
                cdb_unset(key);
                memset(key, 0, KEY_SIZE);
                sprintf(key, "%s%d", "$wl_aplist_pass", idx);
                cdb_unset(key);
                return;
            }

            cdb_set(key, buf);

            /*Clear passwd*/
            memset(key, 0, KEY_SIZE);
            sprintf(key, "%s%d", "$wl_aplist_pass", idx);
            cdb_unset(key);

            memset(key2, 0, KEY_SIZE);
            sprintf(key2, "%s%d", "$wl_aplist_pass", idx+1);

            memset(buf, 0, VAL_SIZE);
            cdb_get(key2, buf);
            cdb_set(key, buf);

            if (index == 1)
            {
                cdb_unset("$wl_bss_ssid2");
                cdb_unset("$wl_bss_bssid2");
            }
        }
    }

    return;
}

//wl_aplist_info=bssid=xxx&sec=xxx&cipher=&last=&mode=&tc=&ssid=
//wl_aplist_pass=xxx 
int main(int argc, char *argv[])
{
    unsigned int idx = 0;

    //aplist_restructure();

    if (argc >= 2)
    {
        if (!strcmp(argv[1], "d"))
        {
            debug = 1;
            aplist_modify(idx);
            cdb_service_clear_change("wl");
            cdb_save();
        } else if (!strcmp(argv[1], "s"))
        {
            aplist_show();
        } else if (!strcmp(argv[1], "t"))
        {
            aplist_total();
        } else if (!strcmp(argv[1], "g"))
        {
            if (argc == 4)
            {
                idx = atoi(argv[2]);
                if (!strcmp(argv[3], "s"))
                {
                    aplist_getitem(idx, SSID);
                } else if (!strcmp(argv[3], "p"))
                {
                    aplist_getitem(idx, PASS);
                }
            }
        } else if (!strcmp(argv[1], "r"))
        {
            if (argc == 3)
            {
                idx = atoi(argv[2]);
                if (idx > 0 && idx <= AP_MAX)
                {
                    aplist_modify(idx);
                    cdb_service_clear_change("wl");
                    cdb_save();
                }
            }
        }
    } else
    {
        aplist_modify(idx);
        cdb_service_clear_change("wl");
        cdb_save();
    }

    return 0;
}
