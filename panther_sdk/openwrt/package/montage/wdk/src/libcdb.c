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
#include "mtutils.h"

#include <whitedb/dbapi.h>
#include <whitedb/indexapi.h>

#if defined(CDB_IPC_LOCK)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#define CDB_IPC_LOCK_FAILURE        0
#define CDB_IPC_LOCK_SUCCESS        1

#define wg_start_read(db)           cdb_ipc_lock()
#define wg_end_read(db, lockid)     cdb_ipc_unlock()

#define wg_start_write(db)          cdb_ipc_lock()
#define wg_end_write(db, lockid)    cdb_ipc_unlock()

static int cdb_semid = -1;
static volatile int *cdb_lock_pid = NULL;
static int *cdb_op_code;

#if defined(CDB_IPC_LOCK_DEBUG)
static char *working_process_name;
static char *working_variable_name;

static char* get_full_process_name(void)
{
    size_t linknamelen;
    char file[256];
    static char cmdline[256] = {0};

    sprintf(file, "/proc/%d/exe", getpid());
    linknamelen = readlink(file, cmdline, sizeof(cmdline) / sizeof(*cmdline) - 1);
    cmdline[linknamelen + 1] = 0;
    return cmdline;
}
#endif

int cdb_ipc_connect(void)
{
    key_t key;
    int cdb_shmid;
    unsigned char *cdb_shm;

    if(cdb_lock_pid)
        return 0;

    key = ftok("/sbin/cdb", 88);

    if ((cdb_semid = semget(key, 1, IPC_CREAT | 0666)) < 0)
        goto error;

    if ((cdb_shmid = shmget(key, CDB_LOCK_SHMSZ, IPC_CREAT | 0666)) < 0)
        goto error;

    if ((cdb_shm = shmat(cdb_shmid, NULL, 0)) == (unsigned char *) -1)
        goto error;

    cdb_lock_pid = (int *) cdb_shm;
    cdb_op_code = (int *) &cdb_lock_pid[1];

#if defined(CDB_IPC_LOCK_DEBUG)
    working_variable_name = (char *) &cdb_op_code[1];
    working_process_name = (char *) &working_variable_name[128];
#endif

    return 0;

error:
    return CDB_IPC_SEMGET;
}

void cdb_ipc_init(void)
{
    if(cdb_semid>=0)
        semctl(cdb_semid, 0, SETVAL, 1);

    if(cdb_lock_pid)
        *cdb_lock_pid = 0;

    if(cdb_op_code)
        *cdb_op_code = 0;

#if defined(CDB_IPC_LOCK_DEBUG)
    working_variable_name[0] = '\0';
    working_process_name[0] = '\0';
#endif
}

static int cdb_ipc_unlock(void)
{
    int ret;
    struct sembuf sops[1];

    if(0>(ret=cdb_ipc_connect()))
        return ret;

    sops[0].sem_num = 0;        /* Operate on semaphore 0 */
    sops[0].sem_op = 1;         /* Increment value by one */
    sops[0].sem_flg = 0;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_process_name, "U ");
    strcpy(&working_process_name[2], get_full_process_name());
#endif

    *cdb_lock_pid = 0;

retry:
    if (semop(cdb_semid, sops, 1) < 0)
    {
       if((errno==EAGAIN)||(errno==EINTR))
           goto retry;
    }

    return CDB_IPC_LOCK_SUCCESS;
}

static int cdb_ipc_trylock(void)
{
    struct sembuf sops[1];

    sops[0].sem_num = 0;        /* Operate on semaphore 0 */
    sops[0].sem_op = -1;
    sops[0].sem_flg = IPC_NOWAIT;

    if (semop(cdb_semid, sops, 1) < 0)
    {
       if((errno==EAGAIN)||(errno==EINTR))
           return 1;
       else
           return -1;
    }

    *cdb_lock_pid = getpid();
#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_process_name, get_full_process_name());
#endif

    return 0;
}

/* return CDB_IPC_LOCK_FAILURE(0) on failure */
static int cdb_ipc_lock(void)
{
    int ret;
    int max_try_count = (CDB_IPC_LOCK_TIMEOUT/CDB_IPC_LOCK_RETRY_DELAY);

    if(0>(ret=cdb_ipc_connect()))
        return CDB_IPC_LOCK_FAILURE;

    while(1)
    {
        ret = cdb_ipc_trylock();
        if(0==ret)
            break;

        if(0>ret)
            return CDB_IPC_LOCK_FAILURE;

        //if(--max_try_count <= 0)
        //    break;

        usleep(5 * 1000);
    }

    return CDB_IPC_LOCK_SUCCESS;
}

int cdb_debug_get_lock_holder(void)
{
    int ret;

    if(0>(ret=cdb_ipc_connect()))
        return ret;

    return *cdb_lock_pid;
}

int cdb_debug_get_op_code(void)
{
    int ret;

    if(0>(ret=cdb_ipc_connect()))
        return ret;

    return *cdb_op_code;
}

char *cdb_debug_get_variable_name(void)
{
#if defined(CDB_IPC_LOCK_DEBUG)
    if(0>cdb_ipc_connect())
        return NULL;

    return working_variable_name;
#else
    return NULL;
#endif
}

char *cdb_debug_get_process_name(void)
{
#if defined(CDB_IPC_LOCK_DEBUG)
    if(0>cdb_ipc_connect())
        return NULL;

    return working_process_name;
#else
    return NULL;
#endif
}

void cdb_debug_force_unlock(void)
{
    cdb_ipc_unlock();
}

#endif

static void *cdb;
static void *rdb;

int cdb_delete_db(void)
{
    if(cdb)
    {
        wg_detach_database(cdb);
        cdb = NULL;
    }

    if(rdb)
    {
        wg_detach_database(rdb);
        rdb = NULL;
    }

    wg_delete_database(DBNAME_CDB);
    wg_delete_database(DBNAME_RDB);

    return CDB_RET_SUCCESS;
}

static void cdb_create_index(void)
{
    if(cdb && (wg_column_to_index_id(cdb, 0, WG_INDEX_TYPE_TTREE, NULL, 0) == -1))
    {
        if(wg_create_index(cdb, 0, WG_INDEX_TYPE_TTREE, NULL, 0))
        {
            printf("CDB index creation failed.\n");
        }
    }
    
    if(rdb && (wg_column_to_index_id(rdb, 0, WG_INDEX_TYPE_TTREE, NULL, 0) == -1))
    {
        if(wg_create_index(rdb, 0, WG_INDEX_TYPE_TTREE, NULL, 0))
        {
            printf("RDB index creation failed.\n");
        }
    }
}

static int cdb_create_db(void)
{
    if(cdb==NULL)
        cdb = wg_attach_database(DBNAME_CDB, DBSIZE_CDB);

    if(rdb==NULL)
        rdb = wg_attach_database(DBNAME_RDB, DBSIZE_RDB);

    if(cdb && rdb)
        return CDB_RET_SUCCESS;

    if((NULL==cdb) && (NULL==rdb))
        return CDB_ERR_CDB_RDB_CREATE;

    if(NULL==cdb)
        return CDB_ERR_CDB_CREATE;

    if(NULL==rdb)
        return CDB_ERR_RDB_CREATE;

    return CDB_ERR_GENERIC;
}

int cdb_attach_db(void)
{
    if(cdb && rdb)
        return 0;

    if(cdb==NULL)
        cdb = wg_attach_existing_database(DBNAME_CDB);

    if(rdb==NULL)
        rdb = wg_attach_existing_database(DBNAME_RDB);

    if(cdb && rdb)
        return CDB_RET_SUCCESS;

    if((NULL==cdb) && (NULL==rdb))
        return CDB_ERR_CDB_RDB_ATTACH;

    if(NULL==cdb)
        return CDB_ERR_CDB_ATTACH;

    if(NULL==rdb)
        return CDB_ERR_RDB_ATTACH;

    return CDB_ERR_GENERIC;
}

static int __rdb_write_int(char *name, int value, int uselock)
{
    void *rec;
    wg_int lock_id;
    void *target_db = rdb;

    if(NULL==name)
        return CDB_ERR_ARG;

    if(uselock)
    {
        lock_id = wg_start_write(target_db);
        if(!lock_id)
            return CDB_ERR_RDB_WLOCK;
    }

    rec = wg_find_record_str(target_db, 0, WG_COND_EQUAL, name, NULL);
    if(rec==NULL)
    {
        /* record not found, create it */
        rec = wg_create_record(target_db, 2);

        if(rec)
            wg_set_field(target_db, rec, 0, wg_encode_str(target_db, name, NULL));
    }

    if(rec)
    {
        wg_set_field(target_db, rec, 1, wg_encode_int(target_db, value));

        if(uselock)
            wg_end_write(target_db, lock_id);
        return CDB_RET_SUCCESS;
    }
    else
    {
        if(uselock)
            wg_end_write(target_db, lock_id);
        return CDB_ERR_NO_REC;
    }
}

static void cdb_mark_dirty(void)
{
    __rdb_write_int(CDB_DIRTY_MARK_STR, 1, 1);
}

static void cdb_mark_clean(void)
{
    __rdb_write_int(CDB_DIRTY_MARK_STR, 0, 1);
}

static int __rdb_read_int(char *name, int *value, int uselock)
{
    void *rec;
    wg_int lock_id;
    void *target_db = rdb;

    if(value)
        *value = 0;
    else
        return CDB_ERR_ARG;

    if(uselock)
    {
        lock_id = wg_start_read(target_db);
        if(!lock_id)
            return CDB_ERR_RDB_RLOCK;
    }

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 5;

    rec = wg_find_record_str(target_db, 0, WG_COND_EQUAL, name, NULL);
    if(rec==NULL)
    {
        if(uselock)
            wg_end_read(target_db, lock_id);
        return CDB_RET_SUCCESS;   /* intented to be success */
    }

    *value = wg_decode_int(target_db, wg_get_field(target_db, rec, 1));

    if(uselock)
        wg_end_read(target_db, lock_id);
    return CDB_RET_SUCCESS;
}

int cdb_unset(const char *name)
{
    int i=0;
    int ret;
    char prefix[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int lock_id;

    if((NULL==name)||(name[0]!='$'))
        return CDB_ERR_ARG;

    if(0>(ret=cdb_attach_db()))
        return ret;

    while((name[i]!='_')&&(name[i]!='\0'))
    {
        prefix[i] = name[i];
        i++;
    }
    prefix[i] = '\0';

    /* find it in CDB first */
    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_ERR_CDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 6;

    rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, (char *) name, NULL);
    if(rec)
    {
        wg_delete_record(cdb, rec);

        rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, &prefix[1], NULL);
        if(rec)
            wg_set_field(cdb, rec, 2, wg_encode_int(cdb, 1));

        wg_end_write(cdb, lock_id);

        cdb_mark_dirty();

        return CDB_RET_SUCCESS;
    }
    else
    {
        wg_end_write(cdb, lock_id);
    }


    /* then, find it in RDB */
    lock_id = wg_start_write(rdb);
    if(!lock_id)
        return CDB_ERR_RDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 7;

    rec = wg_find_record_str(rdb, 0, WG_COND_EQUAL, (char *) name, NULL);
    if(rec)
    {
        wg_delete_record(rdb, rec);
        wg_end_write(rdb, lock_id);
        return CDB_RET_SUCCESS;
    }
    else
    {
        wg_end_write(rdb, lock_id);
        return CDB_ERR_FIND_REC;
    }
}

static int _cdb_set(const char *name, void *value)
{
    int i=0;
    int ret;
    char prefix[CDB_NAME_MAX_LENGTH];
    char buf[CDB_DATA_MAX_LENGTH];
    void *rec;
    void *target_db;
    wg_int lock_id;

#if 1 /* get before set to find if actually the same value is set */ 
    if(CDB_RET_SUCCESS==cdb_get(name, buf))
    {
        if(!strcmp(value,buf))
            return CDB_RET_SUCCESS;
    }
#endif

    if((NULL==name)||(name[0]!='$'))
        return CDB_ERR_ARG;

    if(NULL==value)
        return CDB_ERR_ARG;

    if(0>(ret=cdb_attach_db()))
        return ret;

    while((name[i]!='_')&&(name[i]!='\0'))
    {
        prefix[i] = name[i];
        i++;
    }
    prefix[i] = '\0';

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_ERR_CDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 8;

    rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, &prefix[1], NULL);
    if(rec)
    {
        wg_set_field(cdb, rec, 2, wg_encode_int(cdb, 1));
        target_db = cdb;
    }
    else
    {
        target_db = rdb;
    }
    wg_end_write(cdb, lock_id);

    if(target_db == cdb)
        cdb_mark_dirty();

    lock_id = wg_start_write(target_db);
    if(!lock_id)
    {
        if(target_db==cdb)
            return CDB_ERR_CDB_WLOCK;
        else
            return CDB_ERR_RDB_WLOCK;
    }

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 9;

    rec = wg_find_record_str(target_db, 0, WG_COND_EQUAL, (char *) name, NULL);
    if(rec==NULL)
    {
        /* record not found, create it */
        rec = wg_create_record(target_db, 2);
        if(rec)
            wg_set_field(target_db, rec, 0, wg_encode_str(target_db, (char *) name, NULL));
    }

    if(rec)
    {
        /* update value */
        wg_set_field(target_db, rec, 1, wg_encode_str(target_db, value, NULL));

        wg_end_write(target_db, lock_id);
        return CDB_RET_SUCCESS;
    }
    else
    {
        wg_end_write(target_db, lock_id);
        return CDB_ERR_NO_REC;
    }
}

int cdb_set(const char *name, void *value)
{
    int ret, n;
    char buf[64];

    ret = _cdb_set(name, value);
    if(ret)
    {
        if(NULL == name)
        {
            n = snprintf(buf, 64, "cdb_set fail name=NULL (%d)\n", ret);
        }
        else
        {
            if(value)
            {
                n = snprintf(buf, 64, "cdb_set fail name=%s (%d)\n", name, ret);
            }
            else
            {
                n = snprintf(buf, 64, "cdb_set fail name=%s value=NULL (%d)\n", name, ret);
            }
        }
        mt_panic(buf, n);
    }

    return ret;
}

int cdb_set_int(const char *name, int value)
{
    char buf[32];

#if 0 /* get before set to find if actually the same value is set */
    int org_value;

    if(CDB_RET_SUCCESS==cdb_get_int(name, &org_value))
    {
        if(value == org_value)
            return CDB_RET_SUCCESS;
    }
#endif

    snprintf(buf, 32, "%d", value);

    return cdb_set(name, buf);
}

static int _cdb_get(const char *name, void *value)
{
    int ret;
    void *rec;
    wg_int enc2;
    wg_int lock_id;

    if((NULL==name)||(name[0]!='$'))
        return CDB_ERR_ARG;

    if(value)
        ((char *) value)[0] = '\0';
    else
        return CDB_ERR_ARG;

    if(0>(ret=cdb_attach_db()))
        return ret;

    /* find it in CDB first */
    lock_id = wg_start_read(cdb);
    if(!lock_id)
        return CDB_ERR_CDB_RLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 4;

    rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, (char *) name, NULL);
    if(rec)
    {
        enc2 = wg_get_field(cdb, rec, 1);
        strcpy((char *) value, wg_decode_str(cdb, enc2));

        wg_end_read(cdb, lock_id);
        return CDB_RET_SUCCESS;
    }
    else
    {
        wg_end_read(cdb, lock_id);
    }


    /* then, find it in RDB */
    lock_id = wg_start_read(rdb);
    if(!lock_id)
        return CDB_ERR_RDB_RLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, name);
#endif
    *cdb_op_code = 1;

    rec = wg_find_record_str(rdb, 0, WG_COND_EQUAL, (char *) name, NULL);
    if(rec)
    {
        enc2 = wg_get_field(rdb, rec, 1);
        strcpy((char *) value, wg_decode_str(rdb, enc2));

        wg_end_read(rdb, lock_id);
        return CDB_RET_SUCCESS;
    }
    else
    {
        wg_end_read(rdb, lock_id);
        return CDB_ERR_FIND_REC;
    }
}

int cdb_get(const char *name, void *value)
{
    int ret, n;
    char buf[64];

    ret = _cdb_get(name, value);
    if(ret && (ret != CDB_ERR_FIND_REC))
    {
        if(NULL == name)
        {
            n = snprintf(buf, 64, "cdb_get fail name=NULL (%d)\n", ret);
        }
        else
        {
            if(value)
            {
                n = snprintf(buf, 64, "cdb_get fail name=%s (%d)\n", name, ret);
            }
            else
            {
                n = snprintf(buf, 64, "cdb_get fail name=%s value=NULL (%d)\n", name, ret);
            }
        }
        mt_panic(buf, n);
    }

    return ret;
}

int boot_cdb_get(char *name, char *val)
{
    char key[CDB_NAME_MAX_LENGTH];

    if(name && (strlen(name)>0))
    {
        snprintf(key, CDB_NAME_MAX_LENGTH, "$boot_%s", name);
        return cdb_get(key, val);
    }
    else
    {
        if(val)
            val[0] = '\0';

        return CDB_ERR_ARG;
    }
}

int cdb_get_array(const char *name, unsigned short array_num, void *value)
{
    char key[CDB_NAME_MAX_LENGTH];

    snprintf(key, CDB_NAME_MAX_LENGTH, "%s%d", name, array_num);

    return cdb_get(key, value);
}

int cdb_get_int(const char *name, int default_value)
{
    char buf[CDB_DATA_MAX_LENGTH];
    int value = default_value;

    if ((cdb_get(name, buf) == CDB_RET_SUCCESS) && isxdigit(buf[0])) {
        value = strtol(buf, NULL, 0);
    }
    
    return value;
}

int cdb_get_array_int(const char *name, unsigned short array_num, int default_value)
{
    char key[CDB_NAME_MAX_LENGTH];

    snprintf(key, CDB_NAME_MAX_LENGTH, "%s%d", name, array_num);

    return cdb_get_int(key, default_value);
}

char *cdb_get_str(const char *name, char *value, int vlen, const char *default_value)
{
    char buf[CDB_DATA_MAX_LENGTH];
    char *ptr = NULL;

    if ((cdb_get(name, buf) == CDB_RET_SUCCESS) && value) {
        snprintf(value, vlen, "%s", buf);
        ptr = value;
    }
    else if (default_value) {
        snprintf(value, vlen, "%s", default_value);
        ptr = value;
    }
    
    return ptr;
}

char *cdb_get_array_str(const char *name, unsigned short array_num, char *value, int vlen, const char *default_value)
{
    char key[CDB_NAME_MAX_LENGTH];

    snprintf(key, CDB_NAME_MAX_LENGTH, "%s%d", name, array_num);

    return cdb_get_str(key, value, vlen, default_value);
}

char *cdb_get_array_val(char *key, unsigned char type, char delimtype, char *value, unsigned int *len, char *val)
{
    char *pstart;
    char *pend = NULL;

    pstart = strstr(value,key);

    if(pstart == NULL)
        return NULL;
    
    if(strncmp(key, value, strlen(key)))
        *len = (unsigned int)(pstart - value);
    else
        *len = 0;

    if(delimtype == CDB_GENR_DELIM)
        pend = strstr(pstart,CDB_DELIM);
    else if(delimtype == CDB_LAST_DELIM)
        ;
    else
        return NULL;

    if(pend != NULL) {
        *len += (unsigned int)(pend - pstart);
        (*len)++;
        *pend='\0';
    }

    pstart += strlen(key) + 1;

    if(pstart == pend) {
        return NULL;
    }
    
    switch(type) {
        case CDB_STR:
            return pstart;
        case CDB_MAC:
            if(NULL == val)
                return NULL;
            sscanf(pstart, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
            return 0;
        default:
            printf("DEF\n");
            return 0;
    }
}

static int cdb_add_record(char *text_record, int uselock, int overwrite)
{
    int i=0;
    char prefix[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int enc, enc2;
    char *name = NULL;
    char *value = NULL;
    int length = strlen(text_record);
    wg_int lock_id = 0;

    if(NULL==text_record)
        return CDB_ERR_ARG;

    while(text_record[i]!='_')
    {
        prefix[i] = text_record[i];
        i++;
        if(i>=length)
            return CDB_ERR_ARG;
    }
    prefix[i] = '\0';

    while(text_record[i]!='=')
    {
        i++;
        if(i>=length)
            return CDB_ERR_ARG;
    }
    text_record[i] = '\0';

    name = text_record;
    if((i+1)<length)
        value = &text_record[i+1];

    length = strlen(value);
    if((length>=2) && (value[0]=='\'') && (value[length-1]=='\''))
    {
        value[--length] = '\0';
        value++;
        length--;
    }

    //printf("(%s %s)\n", name, value);

    if(uselock)
    {
        lock_id = wg_start_write(cdb);
        if(!lock_id)
            return CDB_ERR_CDB_WLOCK;
    }

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, "cdb_add_record");
#endif
    *cdb_op_code = 10;

    rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, &prefix[1], NULL);
    if(NULL==rec)
    {
        rec = wg_create_record(cdb, 3);
        if(rec)
        {
            enc = wg_encode_str(cdb, &prefix[1], NULL);
            wg_set_field(cdb, rec, 0, enc);
            wg_set_field(cdb, rec, 2, wg_encode_int(cdb, 0));
        }
    }

    rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, name, NULL);
    if(rec)
    {
        /* already exist */
        if(overwrite)
            wg_set_field(cdb, rec, 1, wg_encode_str(cdb, value, NULL));

        if(uselock && lock_id)
            wg_end_write(cdb, lock_id);

        return CDB_RET_SUCCESS;
    }

    rec = wg_create_record(cdb, 2);
    if(rec)
    {
        /* update value */
        enc = wg_encode_str(cdb, name, NULL);
        enc2 = wg_encode_str(cdb, value, NULL);
        wg_set_field(cdb, rec, 0, enc);
        wg_set_field(cdb, rec, 1, enc2);
    }

    if(uselock && lock_id)
        wg_end_write(cdb, lock_id);

    return CDB_RET_SUCCESS;
}

static void cdb_tmpfs_cleanup(void)
{
    unlink(CDB_BINARY_DB_TEMP_LZO);
    unlink(CDB_BINARY_DB_TEMP);
}

static int file_exists(char *path)
{
    struct stat st;

    memset(&st, 0 , sizeof(st));
    return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
}

static int cdb_import_lzo(void)
{
    int ret;

    ret = system("/bin/lzop -d " CDB_BINARY_DB_TEMP_LZO REDIRECT_STDOUT_STDERR);
    if(ret==0)
    {
        ret = wg_import_dump(cdb, CDB_BINARY_DB_TEMP);
    }
    else
    {
        ret = CDB_ERR_LZOP_DECOMPRESS;
    }

    cdb_tmpfs_cleanup();

    return ret;
}

static int cdb_dump_lock(void)
{
    int ret;
    int cdb_lock_flag = 0;
    int try_count = 0;
    wg_int lock_id;

    do
    {
        lock_id = wg_start_write(cdb);
        if(!lock_id)
            return CDB_ERR_CDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, "cdb_dump_lock");
#endif
    *cdb_op_code = 11;

#if defined(CDB_IPC_LOCK)
        if(0>(ret=__rdb_read_int(CDB_DUMP_LOCK_STR, &cdb_lock_flag, 0)))
#else
        if(0>(ret=__rdb_read_int(CDB_DUMP_LOCK_STR, &cdb_lock_flag, 1)))
#endif
        {
            wg_end_write(cdb, lock_id);
            return ret;
        }
    
        wg_end_write(cdb, lock_id);

        try_count++;
        if(try_count > CDB_DUMP_LOCK_MAX_TRY_COUNT)
            return CDB_ERR_RETRY_LIMIT;

        usleep(100 * 1000);
    } while(cdb_lock_flag);

    return CDB_RET_SUCCESS;
}

static int cdb_dump_unlock(void)
{
    wg_int lock_id;

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_ERR_CDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, "cdb_dump_unlock");
#endif
    *cdb_op_code = 12;

#if defined(CDB_IPC_LOCK)
    __rdb_write_int(CDB_DUMP_LOCK_STR, 0, 0);
#else
    __rdb_write_int(CDB_DUMP_LOCK_STR, 0, 1);
#endif

    wg_end_write(cdb, lock_id);

    return CDB_RET_SUCCESS;
}

static int cdb_export_lzo(void)
{
    int ret;
#if defined(CDB_IPC_LOCK)
    int ipc_locked = 0;
#endif

    if(0>(ret=cdb_dump_lock()))
        return ret;

    cdb_tmpfs_cleanup();

#if defined(CDB_IPC_LOCK)
    ipc_locked = cdb_ipc_lock();
    if(!ipc_locked)
    {
        ret = CDB_ERR_CDB_WLOCK;
        goto ERR;
    }
#endif

    ret = wg_dump(cdb, CDB_BINARY_DB_TEMP);
    if(0!=ret)
    {
        ret = CDB_ERR_WG_DUMP;
        goto ERR;
    }

#if defined(CDB_IPC_LOCK)
    cdb_ipc_unlock();
    ipc_locked = 0;
#endif

    ret = system("/bin/lzop -C " CDB_BINARY_DB_TEMP REDIRECT_STDOUT_STDERR);
    if(0!=ret)
    {
        ret = CDB_ERR_LZOP_COMPRESS;
        goto ERR;
    }

    system("/bin/cp -f " CDB_BINARY_DB_TEMP_LZO " " CDB_BINARY_DB_BAK REDIRECT_STDOUT_STDERR);
    sync();

    system("/bin/mv -f " CDB_BINARY_DB_TEMP_LZO " " CDB_BINARY_DB REDIRECT_STDOUT_STDERR);
    sync();

    system("/bin/dd if=" CDB_BINARY_DB " of=" CDB_MTD_BLOCK_DEV REDIRECT_STDOUT_STDERR);
    sync();

    cdb_tmpfs_cleanup();

    cdb_dump_unlock();
    return CDB_RET_SUCCESS;

ERR:
#if defined(CDB_IPC_LOCK)
    if(ipc_locked)
        cdb_ipc_unlock();
#endif

    cdb_tmpfs_cleanup();

    cdb_dump_unlock();
    return ret;
}

int cdb_reset(void)
{
    int ret;

    if(0>(ret=cdb_attach_db()))
        return ret;

    if(0>(ret=cdb_dump_lock()))
        return ret;

    cdb_mark_clean();

    unlink(CDB_BINARY_DB_BAK);
    unlink(CDB_BINARY_DB);
    system("/bin/dd if=/dev/zero of=" CDB_MTD_BLOCK_DEV REDIRECT_STDOUT_STDERR);
    sync();

    cdb_dump_unlock();

    return CDB_RET_SUCCESS;
}

static int __cdb_save(int force)
{
    int cdb_dirty = 1;
    int ret;

    if(0>(ret=cdb_attach_db()))
        return ret;

    if(0>(ret=__rdb_read_int(CDB_DIRTY_MARK_STR, &cdb_dirty, 1)))
        return ret;

    if(force || cdb_dirty)
    {
        cdb_mark_clean();
        ret = cdb_export_lzo();

        if(CDB_RET_SUCCESS==ret)
        {
            printf("Saving CDB (OK).\n");
        }
        else
        {
            cdb_mark_dirty();
            printf("Saving CDB (Failed).\n");
        }
        return ret;
    }
    else
    {
        printf("Saving CDB (Skipped/Clean).\n");
        return CDB_RET_SUCCESS;
    }
}

int cdb_save(void)
{
    return __cdb_save(0);
}

int cdb_force_save(void)
{
    return __cdb_save(1);
}

static int cdb_load_binary_db(void)
{
    int ret = -1;

    if(file_exists(CDB_BINARY_DB))
    {
        cdb_tmpfs_cleanup();
        system("/bin/cp -f " CDB_BINARY_DB " " CDB_BINARY_DB_TEMP_LZO REDIRECT_STDOUT_STDERR);
        ret = cdb_import_lzo();
        if(ret==0)
            return CDB_RET_LOAD_MAIN;
        unlink(CDB_BINARY_DB);
    }

    if(file_exists(CDB_BINARY_DB_BAK))
    {
        cdb_tmpfs_cleanup();
        system("/bin/cp -f " CDB_BINARY_DB_BAK " " CDB_BINARY_DB_TEMP_LZO REDIRECT_STDOUT_STDERR);
        ret = cdb_import_lzo();
        if(ret==0)
            return CDB_RET_LOAD_BAK;
        unlink(CDB_BINARY_DB_BAK);
    }

    cdb_tmpfs_cleanup();
    system("/bin/dd if=" CDB_MTD_BLOCK_DEV " of=" CDB_BINARY_DB_TEMP_LZO " bs=65536 count=2" REDIRECT_STDOUT_STDERR);
    ret = cdb_import_lzo();
    if(ret==0)
        return CDB_RET_LOAD_MTD;

    return CDB_ERR_GENERIC;
}

/* remove CR/LF/space on the tail */
static void buf_pre_process(char *buf)
{
    int i;
    int length;

    length = strlen(buf);

    for(i=length-1;i>=0;i--)
    {
        if((buf[i]==0x0a)||(buf[i]==0x0d)||(buf[i]==' '))
            buf[i] = '\0';
        else
            break;
    }
}

int cdb_load_text_db(char *filename, int overwrite)
{
    char buf[CDB_LINE_BUF_SIZE];
    FILE *fp;

    fp = fopen(filename, "r");
    if(fp)
    {
        while(1)
        {
            buf[0] = 0;
            fgets(buf, sizeof(buf), fp);

            if(buf[0] == 0)
                break;

            if(buf[0]!='$')
                continue;

#if 0
            if(buf[0]=='#' || buf[0]=='\n' || buf[0]=='\0')
                continue;
#endif

            buf_pre_process(buf);
            if(overwrite)
                cdb_add_record(buf, 1, 1);
            else
                cdb_add_record(buf, 0, 0);
        }

        fclose(fp);
        return CDB_RET_SUCCESS;
    }
    else
    {
        return CDB_ERR_GENERIC;
    }
}

static inline int cdb_strcat_buf(char *buf, int buf_maxlen, int offset, char *output_str)
{
    int len = strlen(output_str);

    if((len + offset + 1) >= buf_maxlen)
        return offset;

    memcpy(&buf[offset], output_str, len);
    buf[offset+len] = '\0';

    return (offset + len);
}

int cdb_dump_text_db(char *filename)
{
    int ret;
    char var_name[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int enc;
    wg_int lock_id;
    char *buf = NULL;
    int offset = 0;

    if(0>(ret=cdb_attach_db()))
        return ret;

    buf = malloc(CDB_TXT_DB_MAX_SIZE);
    if(buf==NULL)
        return CDB_ERR_NOMEM;

    lock_id = wg_start_read(cdb);
    if(lock_id)
    {
#if defined(CDB_IPC_LOCK_DEBUG)
        strcpy(working_variable_name, "cdb_dump_text_db");
#endif
        *cdb_op_code = 2;

        offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "#CDB\n");

        rec = wg_find_record_str(cdb, 0, WG_COND_GREATER, "$", NULL);
        while(rec)
        {
            enc = wg_get_field(cdb, rec, 0);
            strcpy(var_name, wg_decode_str(cdb, enc));
            if(var_name[0]=='$')
            {
                //printf("%s\n", var_name);
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, var_name);
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "='");
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, wg_decode_str(cdb, wg_get_field(cdb, rec, 1)));
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "'\n");
            }
            rec = wg_find_record_str(cdb, 0, WG_COND_GREATER, "$", rec);
        }
        wg_end_read(cdb, lock_id);
    }
    else
    {
        free(buf);
        return CDB_ERR_CDB_RLOCK;
    }

    lock_id = wg_start_read(rdb);
    if(lock_id)
    {
#if defined(CDB_IPC_LOCK_DEBUG)
        strcpy(working_variable_name, "cdb_dump_text_db_r");
#endif
        *cdb_op_code = 3;

        offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "\n #RDB\n");

        rec = wg_find_record_str(rdb, 0, WG_COND_GREATER, "$", NULL);
        while(rec)
        {
            enc = wg_get_field(rdb, rec, 0);
            strcpy(var_name, wg_decode_str(rdb, enc));
            if(var_name[0]=='$')
            {
                //printf(" %s\n", var_name);
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, " ");
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, var_name);
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "='");
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, wg_decode_str(rdb, wg_get_field(rdb, rec, 1)));
                offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "'\n");
            }
            rec = wg_find_record_str(rdb, 0, WG_COND_GREATER, "$", rec);
        }
        offset = cdb_strcat_buf(buf, CDB_TXT_DB_MAX_SIZE, offset, "\n");
        wg_end_read(rdb, lock_id);
    }
    else
    {
        free(buf);
        return CDB_ERR_RDB_RLOCK;
    }

    if(filename==NULL)
        printf("%s", buf);

    free(buf);
    return CDB_RET_SUCCESS;
}

int cdb_merge(char *filename)
{
    int ret;

    if(0>(ret=cdb_attach_db()))
        return ret;

    return cdb_load_text_db(filename, 0);
}

static int __cdb_find_change(char *change_str, int commit)
{
    int ret;
    char name[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int enc;
    wg_int lock_id;

    if(0>(ret=cdb_attach_db()))
        return ret;

    if(change_str)
        change_str[0] = '\0';

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_ERR_CDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, "__cdb_find_change");
#endif
    *cdb_op_code = 13;

    rec = wg_find_record_int(cdb, 2, WG_COND_EQUAL, 1, NULL);
    while(rec)
    {
        enc = wg_get_field(cdb, rec, 0);
        strcpy(name, wg_decode_str(cdb, enc));

        if(commit)
            wg_set_field(cdb, rec, 2, wg_encode_int(cdb, 0));

        if(change_str)
        {
            if(strlen(name))
            {
                strcat(change_str, name);
                strcat(change_str, " ");
            }
        }
        rec = wg_find_record_int(cdb, 2, WG_COND_EQUAL, 1, rec);
    }

    wg_end_write(cdb, lock_id);

    return CDB_RET_SUCCESS;
}

int cdb_service_commit_change(char *change_str)
{
    return __cdb_find_change(change_str, 1);
}

int cdb_service_find_change(char *change_str)
{
    return __cdb_find_change(change_str, 0);
}

static int __cdb_set_change(char *service_name, int value)
{
    int ret;
    void *rec;
    wg_int lock_id;

    if(0>(ret=cdb_attach_db()))
        return ret;

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_ERR_CDB_WLOCK;

#if defined(CDB_IPC_LOCK_DEBUG)
    strcpy(working_variable_name, "__cdb_set_change");
#endif
    *cdb_op_code = 14;

    rec = wg_find_record_str(cdb, 0, WG_COND_EQUAL, service_name, NULL);
    if(rec)
        wg_set_field(cdb, rec, 2, wg_encode_int(cdb, value));

    wg_end_write(cdb, lock_id);

    return CDB_RET_SUCCESS;
}

int cdb_service_clear_change(char *service_name)
{
    return __cdb_set_change(service_name, 0);
}

int cdb_service_set_change(char *service_name)
{
    return __cdb_set_change(service_name, 1);
}

int cdb_init(void)
{
    int ret;

    if(0>(ret=cdb_create_db()))
        return ret;

    ret = cdb_load_binary_db();
    if(0>ret)
    {
       cdb_create_index();

       /* re-init with default CDB */
       printf("Loading default CDB\n");
       ret = cdb_load_text_db(CDB_DEFAULT_CONFIG, 0);
       cdb_mark_dirty();
    }
    else
    {
        if(CDB_RET_LOAD_MTD==ret)
        {
            printf("Loading CDB from MTD\n");
            /* data from mtd device, could due to firmware upgrade, merge it */
            cdb_merge(CDB_DEFAULT_CONFIG);
            cdb_mark_dirty();
        }
    }

    if(0>ret)
    {
        return ret;
    }
    else
    {
        cdb_create_index();
        cdb_service_commit_change(NULL);
        return CDB_RET_SUCCESS;
    }
}

void cdb_debug_dump(void)
{
    if(0>cdb_attach_db())
        return;

    if(cdb)
    {
        printf("===============CDB===============\n");
        wg_print_db(cdb);
        printf("===============END OF CDB========\n");
    }

    if(rdb)
    {
        printf("===============RDB===============\n");
        wg_print_db(rdb);
        printf("===============END OF RDB========\n");
    }
}

#if 0 // obsoleted
char *rule2var(const char *str, const char *name) {
    char *s = malloc(strlen(str) + 1);
    const char *delim1 = CDB_DELIM;
    const char *delim2 = "=";
    char *pch1, *pch2, *token1, *token2, *rc = NULL;
    strcpy(s, str);
    token1 = strtok_r(s, delim1, &pch1);
    while (token1 != NULL) {
        //printf ("%s\n",token1);
        token2 = strtok_r(token1, delim2, &pch2);
        token1 = strtok_r(NULL, delim1, &pch1);
        if (!strcmp(name, token2)) {
            token2 = strtok_r(NULL, delim2, &pch2);
            //printf("%s=%s\n",name,token2);
            rc = malloc(strlen(token2) + 1);
            strcpy(rc, token2);
            break;
        }
    }
    free(s);
    return rc;
}

int get_all_attributes(char *str, char **att, char **val)
{
    const char *delim1 = "&";
    const char *delim2 = "=";
    char *pch1, *pch2, *token1, *token2;
    int n = 0;

    token1 = strtok_r(str, delim1, &pch1);
    while (token1 != NULL) {
        //printf ("### %s\n",token1);
        token2 = strtok_r(token1, delim2, &pch2);
        if (token2 != NULL) {
            token2 = strtok_r(NULL, delim2, &pch2);
            //printf("@@@ %s=%s\n", token1, token2);
        }
        att[n] = token1;
        val[n] = token2;
        n++;
        token1 = strtok_r(NULL, delim1, &pch1);
    }
    //printf ("### n = %d\n",n);

    return n;
}

char *updaterulevar(const char *str, const char *name, const char *value)
{
    char *os = malloc(strlen(str) + 1);
    const char *delim1 = CDB_DELIM;
    const char *delim2 = "=";
    char *pch1, *pch2, *token1, *token2;
    char *ns = malloc(strlen(str) + strlen(value) + 2);
    ns[0] = '\0';
    strcpy(os, str);
    //printf("ostr:%s\n",os);
    token1 = strtok_r(os, delim1, &pch1);
    while (token1 != NULL) {
        //printf ("%s\n",token1);
        token2 = strtok_r(token1, delim2, &pch2);
        ns = strcat(ns, token1);
        ns = strcat(ns, delim2);
        token1 = strtok_r(NULL, delim1, &pch1);
        if (!strcmp(name, token2)) {
            ns = strcat(ns, value);
        }
        else {
            token2 = strtok_r(NULL, delim2, &pch2);
            ns = strcat(ns, token2);
        }
        if (token1)
            ns = strcat(ns, delim1);
    }
    free(os);
    //printf("nstr:%s\n",ns);
    return ns;
}
#endif

