/* upnp_qplay.h
 */

#ifndef _UPNP_QPLAY_H
#define _UPNP_QPLAY_H

#include <upnp/ithread.h>
#define QPLAYPREFIX "qplay://"

extern ithread_mutex_t g_plist_mutex;
struct playback_t
{
    char *trackuri;
    char *metadata;
};

struct transport_plist
{
    char *qplayid;
    char *allmetadata; //
    struct playback_t *plist;
    int seek_song_idx;
    char curidx;
    char totalcnt;
    char start;
};

void plist_output_update(char idx);
struct service *upnp_qplay_get_service(void);
int qplay_init(const char *);
void plist_update(void);

void g_plist_lock(void);
void g_plist_unlock(void);

extern char *allmetadata;
extern struct transport_plist g_playlist;

#endif /* _UPNP_QPLAY_H */
