

#ifndef __ALERTS_FILE_DATA_STORE_H__
#define __ALERTS_FILE_DATA_STORE_H__

#include "debug.h"
#include "AlertHead.h"



/* wirte alert list to the file "/etc/conf/alarms.json"
 *
 * return 0 successd ,otherwise failed 
 */
extern int alertfiledatastore_wirte_to_disk(list_t *list);


/* read all alerts list from  file "/etc/conf/alarms.json"
 *
 * return 0 successd ,otherwise failed 
 */
extern int alertfiledatastore_load_from_disk(AlertManager *manager);


#endif




