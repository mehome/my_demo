#include <syslog.h>
#include "wdk/cdb.h"


int cdb_set(const char *name, void *value)
{
	syslog(LOG_INFO, "%s - %d - %s: name = %s, value = %s\n", __FILE__, __LINE__, __FUNCTION__, name, (char *)value);

	return CDB_RET_SUCCESS;
}

char *cdb_get_str(const char *name, char *value, int vlen, const char *default_value)
{
	syslog(LOG_INFO, "%s - %d - %s: name = %s, vlen = %d\n", __FILE__, __LINE__, __FUNCTION__, name, vlen);

	return CDB_RET_SUCCESS;
}

int cdb_set_int(const char *name, int value)
{
	syslog(LOG_INFO, "%s - %d - %s: name = %s, value = %d\n", __FILE__, __LINE__, __FUNCTION__, name, value);

	return CDB_RET_SUCCESS;
}

int cdb_get_int(const char *name, int default_value)
{
	syslog(LOG_INFO, "%s - %d - %s: name = %s\n", __FILE__, __LINE__, __FUNCTION__, name);

	return CDB_RET_SUCCESS;
}

