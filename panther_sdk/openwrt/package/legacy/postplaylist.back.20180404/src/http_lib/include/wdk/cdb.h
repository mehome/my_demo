#ifndef __CDB_H__
#define __CDB_H__

/* unmark following line to use IPC SEM based lock, instead of whitedb builtin database lock */
#define CDB_IPC_LOCK
//#define CDB_IPC_LOCK_DEBUG

#define CDB_IPC_LOCK_TIMEOUT	 (5000 * 1000)     // in micro-seconds
#define CDB_IPC_LOCK_RETRY_DELAY   (50 * 1000)

#if defined(CDB_IPC_LOCK_DEBUG)
#define CDB_LOCK_SHMSZ  1024
#else
#define CDB_LOCK_SHMSZ  32
#endif

#define DBNAME_CDB      "1001"
#define DBNAME_RDB      "1002"

#define DBSIZE_CDB      (256 * 1024)
#define DBSIZE_RDB      (256 * 1024)

#define CDB_TXT_DB_MAX_SIZE    (128 * 1024)

#define CDB_NAME_MAX_LENGTH 64
#define CDB_DATA_MAX_LENGTH 4096
#define CDB_LINE_BUF_SIZE   (CDB_DATA_MAX_LENGTH + CDB_NAME_MAX_LENGTH + 4)

#define CDB_RET_SUCCESS             0

#define CDB_ERR_FIND_REC           -1
#define CDB_ERR_CDB_RDB_CREATE     -2
#define CDB_ERR_CDB_CREATE         -3
#define CDB_ERR_RDB_CREATE         -4
#define CDB_ERR_CDB_RDB_ATTACH     -5
#define CDB_ERR_CDB_ATTACH         -6
#define CDB_ERR_RDB_ATTACH         -7
#define CDB_ERR_CDB_RLOCK          -8
#define CDB_ERR_CDB_WLOCK          -9
#define CDB_ERR_RDB_RLOCK          -10
#define CDB_ERR_RDB_WLOCK          -11
#define CDB_ERR_LZOP_COMPRESS      -12
#define CDB_ERR_LZOP_DECOMPRESS    -13
#define CDB_ERR_WG_DUMP            -14
#define CDB_ERR_NO_REC             -15
#define CDB_ERR_ARG                -16
#define CDB_ERR_RETRY_LIMIT        -17
#define CDB_ERR_NOMEM              -18

#define CDB_IPC_SEMGET             -19

#define CDB_ERR_GENERIC            -31

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

#define CDB_DELIM               "&"

enum
{
	CDB_NONE = 0,
	CDB_INT,
	CDB_STR,
	CDB_VER = CDB_STR,
	CDB_IP,
	CDB_MAC,
	CDB_IPV6,
	CDB_LEN_INT = 4,
	CDB_LEN_IP = 4,
	CDB_LEN_MAC = 6,
	CDB_LEN_IPV6 = 16,
};

int cdb_init(void);
int cdb_load_text_db(char *filename, int overwrite);
int cdb_attach_db(void);
int cdb_reset(void);
int cdb_save(void);

int cdb_service_commit_change(char *change_str);
int cdb_service_find_change(char *change_str);
int cdb_service_clear_change(char *service_name);
int cdb_service_set_change(char *service_name);

int cdb_unset(const char *name);
int cdb_set(const char *name, void *value);
int cdb_set_int(const char *name, int value);
int cdb_get(const char *name, void *value);
int cdb_get_array(const char *name, unsigned short array_num, void *value);
int cdb_get_int(const char *name, int default_value);
int cdb_get_array_int(const char *name, unsigned short array_num, int default_value);
char *cdb_get_str(const char *name, char *value, int vlen, const char *default_value);
char *cdb_get_array_str(const char *name, unsigned short array_num, char *value, int vlen, const char *default_value);

char *cdb_get_array_val(char *key, unsigned char type, char delimtype, char *value, unsigned int *len, char *val);
int boot_cdb_get(char *name, char *val);

#if defined(CDB_IPC_LOCK)
int cdb_ipc_connect(void);
void cdb_ipc_init(void);
#endif

int cdb_debug_get_lock_holder(void);
int cdb_debug_get_op_code(void);
char *cdb_debug_get_variable_name(void);
char *cdb_debug_get_process_name(void);
void cdb_debug_force_unlock(void);

#if 0 // obsoleted
char *rule2var(const char *str, const char *name);
char *updaterulevar(const char *str, const char *name, const char *value);
int get_all_attributes(char *str, char **att, char **val);
#endif

enum {
	CDB_GENR_DELIM,
	CDB_LAST_DELIM,
};

#endif // __CDB_H__
