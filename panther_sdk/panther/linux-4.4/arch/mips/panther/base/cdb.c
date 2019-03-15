/*
 *  (c) Copyright 2013 Montage Inc., All rights reserved.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/kmod.h>
#include <linux/slab.h>

#define CDB_PROC_NAME	"cdb"

#if 0
#define CDB_STR_DEF_SIZE    32
#define CDB_IP_DEF_SIZE     16
#define CDB_MAC_DEF_SIZE    18
#define CDB_INT_DEF_SIZE	4

/* we need to customize seq_file buffer size to accomodate cdb_dump() result */
#define SEQ_FILE_BUFSIZE    (8 * PAGE_SIZE)

#define SERVICE_NAME_MAX	32
#define CDB_NAME_MAX    	32

enum
{
	CDB_NONE = 0,
	CDB_INT,
	CDB_STR,
	CDB_IP,
	CDB_MAC,
	CDB_IPV6,
};

enum
{
	CDB_SERVICE = 0,
	RDB_SERVICE,
};

struct cdb_service_table
{
    struct list_head service_list;
	struct list_head hash_head;		/* CDB with same hash key(prefix) */
	short entries;
	char type;
	char changed;
	char *action;
	char min_cdb_name_len;
	char max_cdb_name_len;
    char name_len;
	char name[0];
};

struct cdb_entry
{
	struct list_head list;
	struct cdb_service_table *sv;
	char type;
    char valid;          /* the value is valid */
    char array;			/* this entry is array name */
	short length;		/* total length of name and value */
	char name_len;		/* CDB name's length */
	char name[0];		/* name + value */
};

#define CDB_ARRAY(x)    ((x) & 0x80)
#define CDB_TYPE(x)     ((x) & 0x7F)


#define MAC_CDB_CMD_QUEUE_DEPTH		8
#define MAX_CDB_CMD_STRING_LENGTH	512
#define MAX_CDB_CMD_BUFFER_SIZE     (MAX_CDB_CMD_STRING_LENGTH + 64)
struct cdb_command_queue
{
    pid_t pid;
    char command[MAX_CDB_CMD_STRING_LENGTH];
} cdb_cmd_queue[MAC_CDB_CMD_QUEUE_DEPTH];


struct kernel_cdb
{
    struct mutex cdb_mutex;
    int total_entries;
	struct list_head service_list;
} kcdb;


struct cdb_service_table *add_service(char *name, char type, char *action);


static void kcdb_lock(void)
{
    mutex_lock(&kcdb.cdb_mutex);
}

static void kcdb_unlock(void)
{
    mutex_unlock(&kcdb.cdb_mutex);
}

void cdb_free(void *addr)
{
    if(addr)
        kfree(addr);
}

void *cdb_zalloc(int size)
{
    void *p = kmalloc(size, GFP_KERNEL);

    if(p)
        memset(p, 0, size);

    return p;
}


static int cdb_init(void)
{
    int ret = 0;
	char default_service_name='\0';

    kcdb_lock();
	kcdb.total_entries = 0;
	INIT_LIST_HEAD(&kcdb.service_list);

	if(NULL == add_service(&default_service_name, CDB_SERVICE, 0))
		ret = -ENOMEM;

    kcdb_unlock();

    return ret;
}

void cdb_exit(void)
{
    struct cdb_entry *cdbe, *tmp_c;
	struct cdb_service_table *hash, *tmp_h;

    kcdb_lock();

	list_for_each_entry_safe(hash, tmp_h, &kcdb.service_list, service_list)
	{
		list_for_each_entry_safe(cdbe, tmp_c, &hash->hash_head, list)
		{
			list_del(&cdbe->list);
			cdb_free(cdbe);
		}

		list_del(&hash->service_list);
		cdb_free(hash);
	}

    kcdb.total_entries = 0;

    kcdb_unlock();
}

struct cdb_service_table *sv_search(char *str, int len)
{
	struct cdb_service_table *sv, *default_sv=0;

	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		if(sv->name_len == 0)
			default_sv = sv;
		else if(len < sv->name_len)
			continue;
		else if(((len == sv->name_len) || (str[(int)(sv->name_len)] == '_')) &&
				(strncmp(str, sv->name, sv->name_len) == 0))
		{
			/* Move common service to first element */
			list_move(&sv->service_list, &kcdb.service_list);
			return sv;
		}
	}

	return default_sv;
}

struct cdb_entry* cdb_search(char *str, int len)
{
    struct cdb_entry *cdbe;
	struct cdb_service_table *sv;

	sv = sv_search(str, len);

	/* list is sorted by name's length, so we can decide to use forward or
		backward search. */
	if((len - sv->min_cdb_name_len) <= (sv->max_cdb_name_len - len))
	{
		/* forward seacrch */
		list_for_each_entry(cdbe, &sv->hash_head, list)
		{
			if(cdbe->name_len == len)
			{
				if(strncmp(str, cdbe->name, len) == 0)
				{
					return cdbe;
				}
			}
			else if(cdbe->name_len > len)
				break;
		}
	}
	else
	{
		/* backward search */
		list_for_each_entry_reverse(cdbe, &sv->hash_head, list)
		{
			if(cdbe->name_len == len)
			{
				if(strncmp(str, cdbe->name, len) == 0)
				{
					return cdbe;
				}
			}
			else if(cdbe->name_len < len)
				break;
		}
	}

    return 0;
}

struct cdb_service_table *add_service(char *name, char type, char *action)
{
	struct cdb_service_table *sv;
	int action_len, name_len = strlen(name);

	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		if((sv->name_len == name_len) &&
			(strncmp(sv->name, name, name_len) == 0))
		{
			return sv;
		}
	}

	action_len = 0;
	if(action)
		action_len = strlen(action)+1;

	sv = cdb_zalloc(sizeof(struct cdb_service_table) + name_len+1+action_len);
	if(NULL == sv)
		return 0;

	strncpy(sv->name, name, name_len);
	sv->name_len = name_len;
	sv->name[name_len] = '\0';
	if(!action)
		sv->action = 0;
	else
	{
		sv->action = &sv->name[name_len+1];
		memcpy(sv->action, action, action_len);
	}
	sv->type = type;

	INIT_LIST_HEAD(&sv->hash_head);

	list_add(&sv->service_list, &kcdb.service_list);
	return sv;
}

struct cdb_entry* cdb_add_entry(char *name, char type, char array, struct cdb_service_table *sv)
{
    struct cdb_entry *new_entry, *cdbe;
	int name_len, extra_len;
	struct cdb_service_table *sv1;

	name_len = strlen(name);
	if(name_len > (CDB_NAME_MAX - 1))
	{
		return 0;
	}

	if(array)
		extra_len = name_len + 1; /* name + '\0'*/
	else
	{
		switch(type)
		{
			case CDB_INT:
				extra_len = name_len + 1 + CDB_INT_DEF_SIZE; /* name + '\0' + INT */
				break;
			case CDB_IP:
				extra_len = name_len + 1 + CDB_IP_DEF_SIZE; /* name + '\0' + IP */
				break;
			case CDB_MAC:
				extra_len = name_len + 1 + CDB_MAC_DEF_SIZE; /* name + '\0' + MAC */
				break;
			case CDB_STR:
				extra_len = name_len + 1 + CDB_STR_DEF_SIZE; /* name + '\0' + STR */
				break;
			default:
				extra_len = name_len + 1; /* name + '\0'*/
				break;
		}
	}

	new_entry = cdb_zalloc(sizeof(struct cdb_entry) + extra_len);
	if(0 == new_entry)
		return 0;

	sv1 = sv_search(name, name_len);

	new_entry->sv = sv;
    new_entry->array = array;
    new_entry->type = type;
    new_entry->length = extra_len;
    strncpy(new_entry->name, name, name_len);
	new_entry->name_len = name_len;

	list_for_each_entry(cdbe, &sv1->hash_head, list)
	{
		if(cdbe->name_len > name_len)
		{
			list_add(&new_entry->list, cdbe->list.prev);
			goto success;
		}
	}

	list_add_tail(&new_entry->list, &sv1->hash_head);
	sv1->max_cdb_name_len = name_len;

success:
	if(new_entry == list_first_entry(&sv1->hash_head, struct cdb_entry, list))
		sv1->min_cdb_name_len = name_len;

	sv->entries++;
    kcdb.total_entries++;

    return new_entry;
}


static int ___cdb_set_value(struct cdb_entry* cdbe, void *value)
{
    int ret = 0, tmp;
    int changed = 1;
    void *val = NULL;

    if(cdbe->array)
        return -ENOENT;


    if((cdbe->type == CDB_STR) &&
        ((cdbe->length - cdbe->name_len - 1) < (strlen((char *)value) + 1)))
    {
        struct cdb_entry* new_cdbe;
        int length;

        length = strlen((char *)value) + 1 + cdbe->name_len + 1;
        new_cdbe = cdb_zalloc(length + sizeof(struct cdb_entry));
        if(NULL == new_cdbe)
        {
            ret = -ENOMEM;
            goto Exit;
        }

        /* replace old */
        memcpy(new_cdbe, cdbe, sizeof(struct cdb_entry) + cdbe->name_len + 1);
        new_cdbe->length = length;
        list_replace(&cdbe->list, &new_cdbe->list);
        cdb_free(cdbe);

        cdbe = new_cdbe;
    }
    else if(0 == cdbe->length)
    {
        ret = -EINVAL;
        goto Exit;
    }

    if(cdbe->sv)
    {
        changed = cdbe->sv->changed;
    }

    val = cdbe->name + cdbe->name_len + 1;

    switch(cdbe->type)
    {
        case CDB_NONE:
            break;
        case CDB_INT:
            tmp = simple_strtoul((char *)value, NULL, 10);
            if (!changed && memcmp(val, (void *)&tmp, 4))
            {
                changed = 1;
            }
            memcpy(val, (void *)&tmp, 4);
            break;
        case CDB_STR:
            if (!changed && strcmp((char *)val, value))
            {
                changed = 1;
            }
            strcpy((char *)val, value);
            break;
        case CDB_IP:
            if (!changed && memcmp(val, value, CDB_IP_DEF_SIZE-1))
            {
                changed = 1;
            }
            strncpy((char *)val, value, CDB_IP_DEF_SIZE-1);
            break;
        case CDB_MAC:
            if (!changed && memcmp(val, value, CDB_MAC_DEF_SIZE-1))
            {
                changed = 1;
            }
            strncpy((char *)val, value, CDB_MAC_DEF_SIZE-1);
            break;
        default:
            ret = -ENOENT;
            break;
    }
    cdbe->valid = 1;
    if(cdbe->sv && changed)
    {
        cdbe->sv->changed = 1;
    }

Exit:
    return ret;
}

static void __cdb_return_default_value(struct cdb_entry* cdbe, void *value)
{
	/* always return "" for default value, excepting CDB_INT */
    switch(cdbe->type)
    {
        case CDB_INT:
            sprintf(value, "0");
            break;
        case CDB_IP:
//            sprintf(value, "0.0.0.0");
//            break;
        case CDB_MAC:
//            sprintf(value, "00:00:00:00:00:00");
//            break;
        case CDB_NONE:
        case CDB_STR:
        default:
            *(char *) value = '\0';
            break;
    }
}

static int convert_type_string(char *str)
{
	if(strcmp("INT", str) == 0)
		return CDB_INT;
	else if(strcmp("IP", str) == 0)
		return CDB_IP;
	else if(strcmp("MAC", str) == 0)
		return CDB_MAC;
	else if(strcmp("STR", str) == 0)
		return CDB_STR;
	else
		return CDB_NONE;
}

static int __cdb_get_value(struct cdb_entry* cdbe, void *value)
{
    void *val = NULL;
	int tmp;

    if(cdbe->array)
        return -ENOENT;

    /* return 1 on invalid value */
    if(!cdbe->valid)
    {
        __cdb_return_default_value(cdbe, value);
        return -EINVAL;
    }

    if(cdbe->name)
        val = (void *)cdbe->name + cdbe->name_len + 1;;

    switch(cdbe->type)
    {
        case CDB_NONE:
            break;
        case CDB_INT:
			memcpy((void *)&tmp, val, 4);
            sprintf(value, "%d", tmp);
            break;
        case CDB_STR:
        case CDB_IP:
        case CDB_MAC:
            if(val)
                strcpy(value, val);
            else
                *(char *) value = '\0';
            break;
        default:
            break;
    }

    return 0;
}

/* public functions */
int debug_dump(void)
{
	struct cdb_service_table *sv;
    struct cdb_entry* cdbe;

    kcdb_lock();

	printk("kcdb.service_list.next = %x, kcdb.service_list.prev = %x\n",
		(int)(kcdb.service_list.next), (int)(kcdb.service_list.prev));

	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		printk("[%s(%p)], name_len=%d, type=%d, minlen=%d, maxlen=%d, num=%d, changed=%d, action=%s\n",
			sv->name, sv, sv->name_len, sv->type, sv->min_cdb_name_len, sv->max_cdb_name_len, sv->entries, sv->changed, sv->action?sv->action:"NULL");
		list_for_each_entry(cdbe, &sv->hash_head, list)
		{
			printk("     %s, name_len=%d, len=%d, type=%d, valid=%d, array=%d, sv=%s\n",
				cdbe->name, cdbe->name_len, cdbe->length, cdbe->type, cdbe->valid, cdbe->array, cdbe->sv?cdbe->sv->name:"NULL");
		}
	}

    kcdb_unlock();

    return 0;
}

int cdb_dump_with_spec_name(struct seq_file *s, char *name)
{
	struct cdb_service_table *sv;
    struct cdb_entry* cdbe;
    char value[MAX_CDB_CMD_BUFFER_SIZE];
	char aflg=0;
	char start=0;

	if(!strcmp(name, "all"))
		aflg=1;

	kcdb_lock();

	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		list_for_each_entry(cdbe, &sv->hash_head, list)
		{
			if(!aflg) {
				if(!strncmp(name, cdbe->name, strlen(name))) {
					start = 1;
					goto dump;
				}
				else
					if(start)
						break;
					else
						continue;
			}
dump:
			if(__cdb_get_value(cdbe, value) == 0) {
				if((cdbe->type==CDB_INT)||(cdbe->type==CDB_NONE))
					seq_printf(s, "%s=%s\n", cdbe->name, value);
				else
					seq_printf(s, "%s='%s'\n", cdbe->name, value);
			}
		}
	}

    kcdb_unlock();

    return 0;

}

int cdb_dump_with_commit(struct seq_file *s, char commit)
{
	struct cdb_service_table *sv;
    struct cdb_entry* cdbe;
    char value[MAX_CDB_CMD_BUFFER_SIZE];

    kcdb_lock();

	seq_printf(s, "#CDB\n");
	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		if(commit)
		{
			sv->changed = 0;
			if(sv->type != CDB_SERVICE)
				continue;
		}

		list_for_each_entry(cdbe, &sv->hash_head, list)
		{
			if(commit)
			{
				if(cdbe->sv)
					if(cdbe->sv->type != CDB_SERVICE)
						continue;
			}

			if(__cdb_get_value(cdbe, value) == 0)
			{
				if((cdbe->type==CDB_INT)||(cdbe->type==CDB_NONE))
					seq_printf(s, "$%s=%s\n", cdbe->name, value);
				else
					seq_printf(s, "$%s='%s'\n", cdbe->name, value);
			}
		}
	}


    kcdb_unlock();

    return 0;
}

int rdb_commit(struct seq_file *s)
{
	struct cdb_service_table *sv;

    kcdb_lock();

	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		if(sv->type == CDB_SERVICE)
			continue;
		if(sv->changed)
		{
			if(sv->action)
			{
				char *argv[3];
				char *envp[]={"HOME=/", "PATH=/bin:/sbin:/user/bin:/user/sbin", NULL};

				/* call user space action to handle changes */
				argv[0] = "/bin/sh";
				argv[1] = sv->action;
				argv[2] = NULL;
				call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
			}

			sv->changed = 0;
		}
	}


    kcdb_unlock();

    return 0;
}

int cdb_show_changes(struct seq_file *s)
{
	struct cdb_service_table *sv;

    kcdb_lock();

	list_for_each_entry(sv, &kcdb.service_list, service_list)
	{
		if(sv->type != CDB_SERVICE)
			continue;
		if(sv->name_len && sv->changed)
		{
			seq_printf(s, "%s ", sv->name);
		}
	}

    kcdb_unlock();
    return 0;
}


static inline int isdigit(char ch)
{
    return(ch >= '0') && (ch <= '9');
}

struct cdb_entry *greedy_search(char *name, int name_len)
{
	struct cdb_entry *cdbe;

    cdbe = cdb_search(name, name_len);

    if(cdbe)
    {
		return cdbe;
    }
    else
    {
		while((name_len > 1) && isdigit(name[name_len-1]))
		{
			name[name_len-1] = '\0';
			name_len--;
		}

		cdbe = cdb_search(name, name_len);
		if((cdbe) && (cdbe->array))
		{
			return cdbe;
        }
    }
    return NULL;
}

int cdb_set(char *name, void* value)
{
    int ret = -1, name_len;
    char temp_name[CDB_NAME_MAX];
    struct cdb_entry *cdbe, *cdbe1;


    kcdb_lock();

	name_len = strlen(name);
	if(name_len > (CDB_NAME_MAX -1))
	{
		ret = -ENOSPC;
		goto fail;
	}

	strncpy(temp_name, name, name_len);
	temp_name[name_len] = '\0';

	cdbe = greedy_search(temp_name, name_len);

	if(cdbe)
	{
		if(cdbe->array && (cdbe->name_len < name_len))
		{
			cdbe1 = cdb_add_entry(name, cdbe->type, 0, cdbe->sv);
			if(cdbe1)
				ret = ___cdb_set_value(cdbe1, value);
			else
				ret = -ENOSPC;
		}
		else
			ret = ___cdb_set_value(cdbe, value);
	}

fail:
    kcdb_unlock();

    return ret;
}


int cdb_get(char *name, void *value)
{
    int ret = -ENOENT, name_len;
    struct cdb_entry *cdbe;

    kcdb_lock();

	name_len = strlen(name);
	cdbe = greedy_search(name, name_len);

	if(cdbe)
	{
		if(cdbe->name_len < name_len)
		{
			__cdb_return_default_value(cdbe, value);
			ret = 1;
		}
		else
			ret = __cdb_get_value(cdbe, value);
	}

    kcdb_unlock();

    return ret;
}

int cdb_command(struct seq_file *s, int argc, char *argv[])
{
    int ret;

    char value[MAX_CDB_CMD_BUFFER_SIZE];
    char *p;
    int len;

    #if 0
    int i;
    printk("#argc %d#\n", argc);
    for(i=0;i<argc;i++)
    {
        printk("#%d <%s>#\n", i, argv[i]);
    }
    #endif

	if(strlen(argv[0])==0)
		goto Exit;

	if(argv[0][0]=='$')
	{
		char *start = argv[0];
		char *ncmd;

		if(argv[0][1] == '\0')
			cdb_dump_with_commit(s, 0);

		do
		{
			/* '\xff' means multiple command */
			if((ncmd = strstr(start, "\xff")) != NULL)
			{
				*ncmd = '\0';
				ncmd += 1;
			}

			start += 1;
			if((p=strstr(start, "=")))
			{
				*p = '\0';
				p++;

				/* strip ' */
				len = strlen(p);
				if((*p=='\'') && (p[len-1]=='\''))
				{
					p++;

					len = strlen(p);
					if(len>0)
					{
						if(p[len-1]=='\'')
							p[len-1] = '\0';
					}
				}

				ret = cdb_set(start, p);
				if(ret>=0)
				{
					seq_printf(s, "!OK(%d)\n", ret);
				}
				else
				{
					seq_printf(s, "!ERR\n");
				}
			}
			else
			{
				ret = cdb_get(start, value);
				if(ret>=0)
				{
					seq_printf(s, "%s\n", (char *) value);
				}
				else
				{
					seq_printf(s, "!ERR\n");
				}

			}

		}while((ncmd!=NULL) && (start = ncmd));
	}
	else if(argv[0][0]=='@') // set the raw data to cdb when use config_setraw
	{
		char *start = argv[0];

		start += 1;
		if((p=strstr(start, "=")))
		{
			*p = '\0';
			p++;

			ret = cdb_set(start, p);
			if(ret>=0)
			{
				seq_printf(s, "!OK(%d)\n", ret);
			}
			else
			{
				seq_printf(s, "!ERR\n");
			}
		}
		else
		{
			seq_printf(s, "!ERR\n");
		}
	}
	else if(argv[0][0]=='#')
		goto Exit;
	else if(!strcmp(argv[0], "get"))
	{
		if(argc>1)
		{
			ret = cdb_get(argv[1], value);
			if(ret>=0)
			{
				seq_printf(s, "%s\n", (char *) value);
			}
			else
			{
				seq_printf(s, "!ERR\n");
			}
		}
		else
		{
			goto Error;
		}
	}
	else if(!strcmp(argv[0], "set"))
	{
		if(argc>2)
		{
			p = argv[2];
			/* strip ' */
			if(*p=='\'')
			{
				p++;

				len = strlen(p);
				if(len>0)
				{
					if(p[len-1]=='\'')
						p[len-1] = '\0';
				}
			}

			ret = cdb_set(argv[1], p);
			if(ret>=0)
			{
				seq_printf(s, "!OK(%d)\n", ret);
			}
			else
			{
				seq_printf(s, "!ERR\n");
			}
		}
		else
		{
			goto Error;
		}
	}
	else if(!strcmp(argv[0], "debug"))
	{
		debug_dump();
	}
	else if(!strcmp(argv[0], "changes"))
	{
		cdb_show_changes(s);
	}
	else if(!strcmp(argv[0], "commit"))
	{
		cdb_dump_with_commit(s, 1);
	}
	else if((!strcmp(argv[0], "dump"))||(!strcmp(argv[0], "-d"))||(!strcmp(argv[0], "$")))
	{
		cdb_dump_with_commit(s, 0);
	}
	else if((!strcmp(argv[0], "-s")))
	{
		if(argc >= 2)
			cdb_dump_with_spec_name(s, argv[1]);
		else
			goto Error;
	}
	else if(!strcmp(argv[0], "rcommit"))
	{
		rdb_commit(s);
	}
	else if(!strcmp(argv[0], "add"))
	{
		/* add name[] type service */
		struct cdb_service_table *sv;
		char type, array = 0;

		if(argc < 3)
		{
			goto Error;
		}


		if(argc == 4)
			sv = sv_search(argv[3], strlen(argv[3]));
		else
			sv = sv_search('\0', 0);

		if((p=strstr(argv[1], "[]")))
		{
			array = 1;
			*p = '\0';
		}

		if((type = convert_type_string(argv[2])) == 0)
			goto Error;

		cdb_add_entry(argv[1], type, array, sv);

		seq_printf(s, "\n");
	}
	else if(!strcmp(argv[0], "add_cdb_service"))
	{
		if(argc < 2)
		{
			goto Error;
		}

		if(argc == 3)
			add_service(argv[1], CDB_SERVICE, argv[2]);
		else
			add_service(argv[1], CDB_SERVICE, 0);
		seq_printf(s, "\n");
	}
	else if(!strcmp(argv[0], "add_rdb_service"))
	{
		if(argc < 2)
		{
			goto Error;
		}

		if(argc == 3)
			add_service(argv[1], RDB_SERVICE, argv[2]);
		else
			add_service(argv[1], RDB_SERVICE, 0);
		seq_printf(s, "\n");
	}
	else
	{
		seq_printf(s, "Ignore '%s'\n", argv[0]);
	}

Exit:
	//seq_printf(s, "\n");

    return 0;

Error:
    seq_printf(s, "!ERR\n");

    return 0;
}

static int proc_cdb_show(struct seq_file *s, void *priv)
{
    int i;
    pid_t curr_proc_id = current->pid;
    int command_queue_index = -1;
    char *p;
    int __argc = 0;
    char*  __argv[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,};

    kcdb_lock();

    for(i=0;i<MAC_CDB_CMD_QUEUE_DEPTH;i++)
    {
        if(curr_proc_id == cdb_cmd_queue[i].pid)
        {
            command_queue_index = i;
            break;
        }
    }

    kcdb_unlock();

    if(0 > command_queue_index)
    {
        seq_printf(s, "!ERR\n");
    }
    else
    {
        //printk("<%d:%s>\n", command_queue_index, cdb_cmd_queue[command_queue_index].command);
        p = cdb_cmd_queue[command_queue_index].command;

        __argv[__argc] = p;
        __argc++;

        if(*p=='$' || *p=='@') // only one cdb value
            while(*p!='\0') p++;

        i = 1; // assume no '"' in command
        while(*p!='\0')
        {
            if((((*p==' ')||(*p=='\t'))&&(i==1)) || ((*p=='"')&&(i==0)))
            {
                if(*p=='"')
                    i = 1;
                *p = '\0';
                p++;

                while((*p==' ')||(*p=='\t')||(*p=='"'))
                {
                    if(*p=='"')
                        i = 0;
                    p++;
                }
                if(*p=='\0')
                    continue;

                __argv[__argc] = p;
                __argc++;
                continue;
            }
            p++;
        }

        cdb_command(s, __argc, __argv);
    }

    kcdb_lock();
    cdb_cmd_queue[command_queue_index].pid = 0;
    kcdb_unlock();

    return 0;
}

static int proc_cdb_open(struct inode *inode, struct file *file)
{
    int ret;
    struct seq_file *p;

    ret = single_open(file, proc_cdb_show, NULL);
    if(ret==0)
    {
        p = (struct seq_file *) file->private_data;

        if(p->buf==NULL)
        {
            p->buf = kmalloc(SEQ_FILE_BUFSIZE, GFP_KERNEL);

            if(p->buf)
                p->size = SEQ_FILE_BUFSIZE;
        }
    }

    return ret;
}

static ssize_t proc_cdb_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
    int i;
    pid_t curr_proc_id = current->pid;
    int copy_count;
    int command_queue_index = -1;

    kcdb_lock();
    for(i=0;i<MAC_CDB_CMD_QUEUE_DEPTH;i++)
    {
        if(curr_proc_id == cdb_cmd_queue[i].pid)
        {
            command_queue_index = i;
            break;
        }
    }

    if(0 > command_queue_index)
    {
        for(i=0;i<MAC_CDB_CMD_QUEUE_DEPTH;i++)
        {
            if(0 == cdb_cmd_queue[i].pid)
            {
                command_queue_index = i;
                cdb_cmd_queue[i].pid = curr_proc_id;
                break;
            }
        }
    }

    kcdb_unlock();

    if(0 > command_queue_index)
        return -ENOSPC;

    copy_count = (count > (MAX_CDB_CMD_STRING_LENGTH - 1)) ? (MAX_CDB_CMD_STRING_LENGTH - 1):count;

    if(0==copy_from_user(cdb_cmd_queue[command_queue_index].command, buffer, copy_count))
    {
        if(copy_count>0)
        {
            cdb_cmd_queue[command_queue_index].command[copy_count] = '\0';
            if(0xa==cdb_cmd_queue[command_queue_index].command[copy_count-1])
                cdb_cmd_queue[command_queue_index].command[copy_count-1] = '\0';
        }
        else
        {
            cdb_cmd_queue[command_queue_index].command[0] = '\0';
        }

    }
    else
    {
        cdb_cmd_queue[command_queue_index].pid = 0;
        return -EIO;
    }

    return copy_count;
}

static const struct file_operations cdb_proc_fops = {
    .owner      = THIS_MODULE,
    .open       = proc_cdb_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    //.release    = seq_release,
    .release    = single_release,
    .write      = proc_cdb_write,
};

static int __init cdb_init_module(void)
{
    printk(KERN_DEBUG "CDB kernel module init\n");

    memset(cdb_cmd_queue, 0, sizeof(cdb_cmd_queue));

    mutex_init(&kcdb.cdb_mutex);

    if(0>cdb_init())
        return -ENOSPC;

    if(NULL==proc_create(CDB_PROC_NAME, S_IWUSR, NULL, &cdb_proc_fops))
        return -EIO;

    return 0;
}

static void __exit cdb_cleanup_module(void)
{
    remove_proc_entry(CDB_PROC_NAME, NULL);

    cdb_exit();

    mutex_destroy(&kcdb.cdb_mutex);

    printk(KERN_DEBUG "CDB kernel module exit\n");
}
#endif

static int proc_cdb_open(struct inode *inode, struct file *file)
{
    dump_stack();
    panic("\n\n ##### Can not access /proc/cdb anymore #####\n\n");

    return 0;
}

static ssize_t proc_cdb_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
    dump_stack();
    panic("\n\n ##### Can not access /proc/cdb anymore #####\n\n");

    return 0;
}

static const struct file_operations cdb_proc_fops = {
    .owner      = THIS_MODULE,
    .open       = proc_cdb_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    //.release    = seq_release,
    .release    = single_release,
    .write      = proc_cdb_write,
};

static int __init cdb_init_module(void)
{
    printk(KERN_DEBUG "CDB kernel module init\n");

    if(NULL==proc_create(CDB_PROC_NAME, S_IWUSR, NULL, &cdb_proc_fops))
        return -EIO;

    return 0;
}

static void __exit cdb_cleanup_module(void)
{
    remove_proc_entry(CDB_PROC_NAME, NULL);
    printk(KERN_DEBUG "CDB kernel module exit\n");
}

module_init(cdb_init_module);
module_exit(cdb_cleanup_module);
MODULE_LICENSE("GPL");

