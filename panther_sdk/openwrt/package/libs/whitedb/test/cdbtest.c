#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <whitedb/dbapi.h>
#include <whitedb/indexapi.h>

#define DBNAME_CDB      "10000"
#define DBNAME_RDB      "20000"

#define DBSIZE_CDB      (130 * 1024)
#define DBSIZE_RDB      (130 * 1024)

#define CDB_TXT_DB_MAX_SIZE    (128 * 1024)

#define CDB_NAME_MAX_LENGTH 64
#define CDB_DATA_MAX_LENGTH 512
#define CDB_LINE_BUF_SIZE   (CDB_DATA_MAX_LENGTH + CDB_NAME_MAX_LENGTH + 4)

#define CDB_RET_SUCCESS      0
#define CDB_RET_FAILURE     -1

#define CDB_RET_LOAD_MAIN   1
#define CDB_RET_LOAD_BAK    2
#define CDB_RET_LOAD_MTD    3

#define CDB_DIRTY_MARK_STR   "whitedb_dirty"
#define CDB_DUMP_LOCK_STR    "whitedb_dump_lock"

#define CDB_DUMP_LOCK_MAX_TRY_COUNT 50

#define CDB_BINARY_DB           "/etc/config/cdb.dbz"
#define CDB_BINARY_DB_BAK       "/etc/config/cdb.dbz.1"
#define CDB_DEFAULT_CONFIG      "/etc/config/default.cdb"
#define CDB_MTD_BLOCK_DEV       "/dev/mtdblock2"
#define CDB_BINARY_DB_TEMP_LZO  "/tmp/.cdb.db.lzo"
#define CDB_BINARY_DB_TEMP      "/tmp/.cdb.db"
#define REDIRECT_STDOUT_STDERR  " > /dev/null 2>&1"

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

    return CDB_RET_FAILURE;
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

    return CDB_RET_FAILURE;
}

static int rdb_write_int(char *name, int value)
{
    void *rec;
    wg_int lock_id;
    void *target_db = rdb;

    lock_id = wg_start_write(target_db);
    if(!lock_id)
        return CDB_RET_FAILURE;

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

        wg_end_write(target_db, lock_id);
        return CDB_RET_SUCCESS;
    }
    else
    {
        wg_end_write(target_db, lock_id);
        return CDB_RET_FAILURE;
    }
}

static void cdb_mark_dirty(void)
{
    rdb_write_int(CDB_DIRTY_MARK_STR, 1);
}

static void cdb_mark_clean(void)
{
    rdb_write_int(CDB_DIRTY_MARK_STR, 0);
}

static int rdb_read_int(char *name, int *value)
{
    void *rec;
    wg_int lock_id;
    void *target_db = rdb;

    if(value)
        *value = 0;

    lock_id = wg_start_read(target_db);
    if(!lock_id)
        return CDB_RET_FAILURE;

    rec = wg_find_record_str(target_db, 0, WG_COND_EQUAL, name, NULL);
    if(rec==NULL)
    {
        wg_end_read(target_db, lock_id);
        return CDB_RET_SUCCESS; 
    }

    if(value)
        *value = wg_decode_int(target_db, wg_get_field(target_db, rec, 1));

    wg_end_read(target_db, lock_id);
    return CDB_RET_SUCCESS;
}

int cdb_set(const char *name, void *value)
{
    int i=0;
    char prefix[CDB_NAME_MAX_LENGTH];
    void *rec;
    void *target_db;
    wg_int lock_id;

    if((NULL==name)||(name[0]!='$'))
        return CDB_RET_FAILURE;

    if(NULL==value)
        return CDB_RET_FAILURE;

    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    while((name[i]!='_')&&(name[i]!='\0'))
    {
        prefix[i] = name[i];
        i++;
    }
    prefix[i] = '\0';

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_RET_FAILURE;

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
        return CDB_RET_FAILURE;

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
        return CDB_RET_FAILURE;
    }
}

int cdb_set_int(const char *name, int value)
{
    char buf[32];

    snprintf(buf, 32, "%d", value);

    return cdb_set(name, buf);
}

int cdb_get(const char *name, void *value)
{
    void *rec;
    wg_int enc2;
    wg_int lock_id;

    if((NULL==name)||(name[0]!='$'))
        return CDB_RET_FAILURE;

    if(value)
        ((char *) value)[0] = '\0';
    else
        return CDB_RET_FAILURE;

    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    /* find it in CDB first */
    lock_id = wg_start_read(cdb);
    if(!lock_id)
        return CDB_RET_FAILURE;

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
        return CDB_RET_FAILURE;

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
        return CDB_RET_FAILURE;
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
        snprintf(value, vlen, "%s", buf);//"010"
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

#if 0
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
#endif

static int cdb_add_record(char *text_record)
{
    int i=0;
    char prefix[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int enc, enc2;
    char *name = NULL;
    char *value = NULL;
    int length = strlen(text_record);

    if(NULL==text_record)
        return CDB_RET_FAILURE;

    while(text_record[i]!='_')
    {
        prefix[i] = text_record[i];
        i++;
        if(i>=length)
            return CDB_RET_FAILURE;
    }
    prefix[i] = '\0';

    while(text_record[i]!='=')
    {
        i++;
        if(i>=length)
            return CDB_RET_FAILURE;
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

    cdb_tmpfs_cleanup();

    return ret;
}

static int cdb_dump_lock(void)
{
    int cdb_lock = 0;
    int try_count = 0;
    wg_int lock_id;

    do
    {
        lock_id = wg_start_write(cdb);
        if(!lock_id)
            return CDB_RET_FAILURE;
    
        if(0 > rdb_read_int(CDB_DUMP_LOCK_STR, &cdb_lock))
        {
            wg_end_write(cdb, lock_id);
            return CDB_RET_FAILURE;
        }
    
        wg_end_write(cdb, lock_id);

        try_count++;
        if(try_count > CDB_DUMP_LOCK_MAX_TRY_COUNT)
            return CDB_RET_FAILURE;

        usleep(100 * 1000);
    } while(cdb_lock);

    return CDB_RET_SUCCESS;
}

static int cdb_dump_unlock(void)
{
    wg_int lock_id;

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_RET_FAILURE;

    rdb_write_int(CDB_DUMP_LOCK_STR, 0);

    wg_end_write(cdb, lock_id);

    return CDB_RET_SUCCESS;
}

static int cdb_export_lzo(void)
{
    int ret;

    if(0 > cdb_dump_lock())
        return CDB_RET_FAILURE;

    cdb_tmpfs_cleanup();

    ret = wg_dump(cdb, CDB_BINARY_DB_TEMP);
    if(0!=ret)
        goto ERR;

    ret = system("/bin/lzop -C " CDB_BINARY_DB_TEMP REDIRECT_STDOUT_STDERR);
    if(0!=ret)
        goto ERR;

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
    cdb_tmpfs_cleanup();

    cdb_dump_unlock();
    return CDB_RET_FAILURE;
}

int cdb_reset(void)
{
    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    if(0 > cdb_dump_lock())
        return CDB_RET_FAILURE;

    cdb_mark_clean();

    unlink(CDB_BINARY_DB_BAK);
    unlink(CDB_BINARY_DB);
    system("/bin/dd if=/dev/zero of=" CDB_MTD_BLOCK_DEV REDIRECT_STDOUT_STDERR);
    sync();

    cdb_dump_unlock();

    return CDB_RET_SUCCESS;
}

int cdb_save(void)
{
    int cdb_dirty = 1;
    int ret;

    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    if(0>rdb_read_int(CDB_DIRTY_MARK_STR, &cdb_dirty))
        return CDB_RET_FAILURE;

    if(cdb_dirty)
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

    return CDB_RET_FAILURE;
}

static void buf_pre_process(char *buf)
{
    int i;
    int length;

    length = strlen(buf);

    for(i=length-1;i>=0;i--)
    {
        if((buf[i]==0x0a)||(buf[i]==0x0d))
            buf[i] = '\0';
        else
            break;
    }
}

static int cdb_load_text_db(char *filename)
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
            if((!strcmp(buf, "quit")) || (!strcmp(buf, "q")) || (!strcmp(buf, "exit")))
                break;

            if(buf[0]=='#' || buf[0]=='\n' || buf[0]=='\0')
                continue;
#endif

            buf_pre_process(buf);
            cdb_add_record(buf);
        }

        fclose(fp);
        return CDB_RET_SUCCESS;
    }
    else
    {
        return CDB_RET_FAILURE;
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
    char var_name[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int enc;
    wg_int lock_id;
    char *buf = NULL;
    int offset = 0;

    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    buf = malloc(CDB_TXT_DB_MAX_SIZE);
    if(buf==NULL)
        return CDB_RET_FAILURE;

    lock_id = wg_start_read(cdb);
    if(lock_id)
    {
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
        return CDB_RET_FAILURE;
    }

    lock_id = wg_start_read(rdb);
    if(lock_id)
    {
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
        return CDB_RET_FAILURE;
    }

    if(filename==NULL)
        printf("%s", buf);

    free(buf);
    return CDB_RET_SUCCESS;
}

int cdb_merge(char *filename)
{
    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    return cdb_load_text_db(filename);
}

int cdb_commit_change(char *change_str)
{
    char name[CDB_NAME_MAX_LENGTH];
    void *rec;
    wg_int enc;
    wg_int lock_id;

    if(0>cdb_attach_db())
        return CDB_RET_FAILURE;

    if(change_str)
        change_str[0] = '\0';

    lock_id = wg_start_write(cdb);
    if(!lock_id)
        return CDB_RET_FAILURE;

    rec = wg_find_record_int(cdb, 2, WG_COND_EQUAL, 1, NULL);
    while(rec)
    {
        enc = wg_get_field(cdb, rec, 0);
        strcpy(name, wg_decode_str(cdb, enc));
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

int cdb_init(void)
{
    int ret;

    if(CDB_RET_FAILURE==cdb_create_db())
        return CDB_RET_FAILURE;

    ret = cdb_load_binary_db();
    if(0>ret)
    {
       cdb_create_index();

       /* re-init with default CDB */
       printf("Loading default CDB\n");
       ret = cdb_load_text_db(CDB_DEFAULT_CONFIG);
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
        return CDB_RET_FAILURE;
    }
    else
    {
        cdb_commit_change(NULL);
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

#if 1

int main(int argc, char *argv[])
{
    char name[CDB_NAME_MAX_LENGTH];
    char _line_buf[CDB_LINE_BUF_SIZE+1];
    char *line_buf = &_line_buf[1];
    char buf[CDB_DATA_MAX_LENGTH];
    int ret;
    char *p;

    if(0>cdb_attach_db())
    {
        if(CDB_RET_FAILURE==cdb_init())
            return 0;
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

        }
        else if(!strcmp(argv[1], "save"))
        {
            cdb_save();
        }
        else if(!strcmp(argv[1], "reset"))
        {
            if (!cdb_reset())
                printf("CDB Reset.\n");
        }
        else if(!strcmp(argv[1], "merge"))
        {

        }
        else if(!strcmp(argv[1], "load"))
        {

        }
        else
        {
            /* print usage here */
        }

        return 0;
    }

    while(1)
    {
        printf("CLI>");

        fgets(line_buf, CDB_LINE_BUF_SIZE, stdin);

        line_buf[strlen(line_buf)-1] = '\0';

        if(strlen(line_buf)==0)
            continue;

        if((!strcmp(line_buf, "quit")) || (!strcmp(line_buf, "q")) || (!strcmp(line_buf, "exit")))
            break;

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

    return 0;
}

#else

int main(int argc, char **argv)
{
    int i;
    char buf[512];
    
    if(CDB_RET_FAILURE==cdb_init())
        return 0;

    //cdb_debug_dump();

    cdb_save();

    #if 1
    cdb_delete_db();
    
    cdb_init();
    #endif

    #if 1
    cdb_set("$wl_ringbuf_sec", "123");
    cdb_save();
    #endif

    #if 1
    for(i=0;i<300;i++)
    {
        sprintf(buf, "this is %d", i);
        cdb_set("$spotify_ringbuf_sec", buf);
        cdb_set("$wl_ringbuf_sec", buf);
        cdb_get("$wl_noexist", buf);
        cdb_set("$rdb_test", "234234");
        cdb_get("$rdb_test", buf);
    }
    #endif

    //cdb_debug_dump();

    #if 1
    cdb_commit_change(buf);
    printf("(%s)\n", buf);

    cdb_commit_change(buf);
    printf("(%s)\n", buf);
    #endif

    cdb_dump_text_db(NULL);

    cdb_save();

    cdb_delete_db();

    return 0;
}

#endif

/* 
 Build hint
   mips-openwrt-linux-gcc -Wall -I/workplace3/panther_le.asic/openwrt/staging_dir/target-mips-openwrt-linux/usr/include/ -I. cdbtest.c -o cdbtest -L/workplace3/panther_le.asic/openwrt/staging_dir/target-mips-openwrt-linux/usr/lib -lwgdb
*/

