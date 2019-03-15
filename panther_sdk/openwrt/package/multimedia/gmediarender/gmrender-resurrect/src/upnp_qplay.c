/* upnp_qplay.c
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "upnp_qplay.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include <upnp/upnp.h>
#include <upnp/ithread.h>

#include "output.h"
#include "upnp.h"
#include "upnp_device.h"
#include "variable-container.h"
#include "xmlescape.h"
#include "md5.h"
#include <json-c/json.h>
#include "logging.h"

#define QPLAY_TYPE "urn:schemas-tencent-com:service:QPlay:1"
#define QPLAY_SERVICE_ID "urn:tencent-com:serviceId:QPlay"

#define QPLAY_SCPD_URL "/upnp/QPlaySCPD.xml"
#define QPLAY_CONTROL_URL "/upnp/control/QPlay"
#define QPLAY_EVENT_URL "/upnp/event/QPlay"

char *allmetadata;
struct transport_plist g_playlist;
extern ithread_mutex_t transport_mutex;
extern char *g_qplayid;

typedef enum {
    QPLAY_VAR_AAT_NUMBEROFTRACKS,
    QPLAY_VAR_AAT_STARTINGINDEX,
    QPLAY_VAR_AAT_NEXTINDEX,
    QPLAY_VAR_AAT_TRACKSMETADATA,
    QPLAY_VAR_AAT_QUEUEID,
    QPLAY_VAR_AAT_SEED,
    QPLAY_VAR_AAT_CODE,
    QPLAY_VAR_AAT_MID,
    QPLAY_VAR_AAT_DID,
    QPLAY_VAR_AAT_SSID,
    QPLAY_VAR_AAT_KEY,
    QPLAY_VAR_AAT_AUTHALGO,
    QPLAY_VAR_AAT_CIPHERALGO,
    QPLAY_VAR_UNKNOWN,
    QPLAY_VAR_COUNT
} qplay_variable_t;

enum {
    QPLAY_CMD_GETMAXTRACKS,
    QPLAY_CMD_GETTRACKSCOUNT,
    QPLAY_CMD_GETTRACKSINFO,
    QPLAY_CMD_INSERTTRACKS,
    QPLAY_CMD_QPLAYAUTH,
    QPLAY_CMD_REMOVETRACKS,
    QPLAY_CMD_SETNETWORK,
    QPLAY_CMD_SETTRACKSINFO,
    QPLAY_CMD_UNKNOWN,
    QPLAY_CMD_COUNT
};

enum UPNPTransportError {
	UPNP_QPLAY_E_TRANSITION_NA	= 701,
	UPNP_QPLAY_E_NO_CONTENTS	= 702,
	UPNP_QPLAY_E_READ_ERROR	= 703,
	UPNP_QPLAY_E_PLAY_FORMAT_NS	= 704,
	UPNP_QPLAY_E_TRANSPORT_LOCKED	= 705,
	UPNP_QPLAY_E_WRITE_ERROR	= 706,
	UPNP_QPLAY_E_REC_MEDIA_WP	= 707,
	UPNP_QPLAY_E_REC_FORMAT_NS	= 708,
	UPNP_QPLAY_E_REC_MEDIA_FULL	= 709,
	UPNP_QPLAY_E_SEEKMODE_NS	= 710,
	UPNP_QPLAY_E_ILL_SEEKTARGET	= 711,
	UPNP_QPLAY_E_PLAYMODE_NS	= 712,
	UPNP_QPLAY_E_RECQUAL_NS	= 713,
	UPNP_QPLAY_E_ILLEGAL_MIME	= 714,
	UPNP_QPLAY_E_CONTENT_BUSY	= 715,
	UPNP_QPLAY_E_RES_NOT_FOUND	= 716,
	UPNP_QPLAY_E_PLAYSPEED_NS	= 717,
	UPNP_QPLAY_E_INVALID_IID	= 718,
};

static const char *qplay_variable_names[] = {
	[QPLAY_VAR_AAT_NUMBEROFTRACKS]  = "A_ARG_TYPE_NumberOfTracks",
	[QPLAY_VAR_AAT_STARTINGINDEX]   = "A_ARG_TYPE_StartingIndex",
	[QPLAY_VAR_AAT_NEXTINDEX]       = "A_ARG_TYPE_NextIndex",
	[QPLAY_VAR_AAT_TRACKSMETADATA]  = "A_ARG_TYPE_TracksMetaData",
	[QPLAY_VAR_AAT_QUEUEID]         = "A_ARG_TYPE_QueueID",
	[QPLAY_VAR_AAT_SEED]            = "A_ARG_TYPE_Seed",
	[QPLAY_VAR_AAT_CODE]            = "A_ARG_TYPE_Code",
	[QPLAY_VAR_AAT_MID]             = "A_ARG_TYPE_MID",
	[QPLAY_VAR_AAT_DID]             = "A_ARG_TYPE_DID",
	[QPLAY_VAR_AAT_SSID]            = "A_ARG_TYPE_SSID",
	[QPLAY_VAR_AAT_KEY]             = "A_ARG_TYPE_Key",
	[QPLAY_VAR_AAT_AUTHALGO]        = "A_ARG_TYPE_AuthAlgo",
	[QPLAY_VAR_AAT_CIPHERALGO]      = "A_ARG_TYPE_CipherAlgo",

    [QPLAY_VAR_UNKNOWN]             = NULL
};

static const char *qplay_default_values[] = {
	[QPLAY_VAR_AAT_NUMBEROFTRACKS]  = "",
	[QPLAY_VAR_AAT_STARTINGINDEX]   = "",
	[QPLAY_VAR_AAT_NEXTINDEX]       = "",
	[QPLAY_VAR_AAT_TRACKSMETADATA]  = "",
	[QPLAY_VAR_AAT_QUEUEID]         = "",
	[QPLAY_VAR_AAT_SEED]            = "",
	[QPLAY_VAR_AAT_CODE]            = "",
	[QPLAY_VAR_AAT_MID]             = "",
	[QPLAY_VAR_AAT_DID]             = "",
	[QPLAY_VAR_AAT_SSID]            = "",
	[QPLAY_VAR_AAT_KEY]             = "",
	[QPLAY_VAR_AAT_AUTHALGO]        = "",
	[QPLAY_VAR_AAT_CIPHERALGO]      = "",
    [QPLAY_VAR_UNKNOWN]             = NULL
};

static const char *auth_algo[] = {
	"open",
	"shared",
	"WPA",
	"WPAPSK",
	"WPA2",
	"WPA2PSK",
	NULL
};

static const char *cipher_algo[] = {
	"none",
	"WEP",
	"TKIP",
	"AES",
	NULL
};

static struct var_meta qplay_var_meta[] = {
	[QPLAY_VAR_AAT_NUMBEROFTRACKS]   =  { SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_STARTINGINDEX]    =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_TRACKSMETADATA]   =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_QUEUEID]          =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_SEED]             =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_CODE]             =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_MID]              =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_DID]              =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_SSID]             =  { SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_KEY]              =  { SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[QPLAY_VAR_AAT_AUTHALGO]         =  { SENDEVENT_NO, DATATYPE_STRING, auth_algo, NULL },
	[QPLAY_VAR_AAT_CIPHERALGO]       =  { SENDEVENT_NO, DATATYPE_STRING, cipher_algo, NULL },
	[QPLAY_VAR_UNKNOWN]              =  { SENDEVENT_NO, DATATYPE_UNKNOWN, NULL, NULL }
};

static struct argument *arguments_getmaxtracks[] = {
        & (struct argument) { "MaxTracks", PARAM_DIR_OUT, QPLAY_VAR_AAT_NUMBEROFTRACKS},
        NULL
};

static struct argument *arguments_gettrackcnt[] = {
        & (struct argument) { "NrTracks", PARAM_DIR_OUT, QPLAY_VAR_AAT_NUMBEROFTRACKS},
        NULL
};

static struct argument *arguments_gettrackinfo[] = {
        & (struct argument) { "StartingIndex",  PARAM_DIR_IN, QPLAY_VAR_AAT_STARTINGINDEX},
        & (struct argument) { "NumberOfTracks", PARAM_DIR_IN, QPLAY_VAR_AAT_NUMBEROFTRACKS},
        & (struct argument) { "TracksMetaData", PARAM_DIR_OUT, QPLAY_VAR_AAT_TRACKSMETADATA},
        NULL
};

static struct argument *arguments_inserttracks[] = {
        & (struct argument) { "QueueID",  PARAM_DIR_IN, QPLAY_VAR_AAT_QUEUEID},
        & (struct argument) { "StartingIndex",  PARAM_DIR_IN, QPLAY_VAR_AAT_STARTINGINDEX},
        & (struct argument) { "TracksMetaData", PARAM_DIR_OUT, QPLAY_VAR_AAT_TRACKSMETADATA},
        & (struct argument) { "NumberOfSuccess", PARAM_DIR_OUT, QPLAY_VAR_AAT_NUMBEROFTRACKS },
        NULL
};

static struct argument *arguments_settrackinfo[] = {
        & (struct argument) { "QueueID", PARAM_DIR_IN, QPLAY_VAR_AAT_QUEUEID },
        & (struct argument) { "StartingIndex", PARAM_DIR_IN, QPLAY_VAR_AAT_STARTINGINDEX },
        & (struct argument) { "NextIndex", PARAM_DIR_IN, QPLAY_VAR_AAT_NEXTINDEX },
        & (struct argument) { "TracksMetaData", PARAM_DIR_IN, QPLAY_VAR_AAT_TRACKSMETADATA },
        & (struct argument) { "NumberOfSuccess", PARAM_DIR_OUT, QPLAY_VAR_AAT_NUMBEROFTRACKS },
	NULL
};

static struct argument *arguments_qauth[] = {
        & (struct argument) { "Seed", PARAM_DIR_IN, QPLAY_VAR_AAT_SEED },
        & (struct argument) { "Code", PARAM_DIR_OUT, QPLAY_VAR_AAT_CODE },
        & (struct argument) { "MID", PARAM_DIR_OUT, QPLAY_VAR_AAT_MID },
        & (struct argument) { "DID", PARAM_DIR_OUT, QPLAY_VAR_AAT_DID },
	NULL
};

static struct argument *arguments_removetracks[] = {
        & (struct argument) { "QueueID", PARAM_DIR_IN, QPLAY_VAR_AAT_QUEUEID },
        & (struct argument) { "StartingIndex", PARAM_DIR_IN, QPLAY_VAR_AAT_STARTINGINDEX },
        & (struct argument) { "NumberOfTracks", PARAM_DIR_IN, QPLAY_VAR_AAT_NUMBEROFTRACKS },
        & (struct argument) { "NumberOfSuccess", PARAM_DIR_OUT, QPLAY_VAR_AAT_NUMBEROFTRACKS },
	NULL
};

static struct argument *arguments_setnetwork[] = {
        & (struct argument) { "SSID", PARAM_DIR_IN, QPLAY_VAR_AAT_SSID },
        & (struct argument) { "Key", PARAM_DIR_IN, QPLAY_VAR_AAT_KEY },
        & (struct argument) { "AuthAlgo", PARAM_DIR_IN, QPLAY_VAR_AAT_AUTHALGO },
        & (struct argument) { "CipherAlgo", PARAM_DIR_IN, QPLAY_VAR_AAT_CIPHERALGO },
	NULL
};

static struct argument **argument_list[] = {
	[QPLAY_CMD_GETMAXTRACKS]    = arguments_getmaxtracks,
	[QPLAY_CMD_GETTRACKSCOUNT]  = arguments_gettrackcnt,
	[QPLAY_CMD_GETTRACKSINFO]   = arguments_gettrackinfo,
	[QPLAY_CMD_INSERTTRACKS]    = arguments_inserttracks,
	[QPLAY_CMD_QPLAYAUTH]       = arguments_qauth,
	[QPLAY_CMD_REMOVETRACKS]    = arguments_removetracks,
	[QPLAY_CMD_SETNETWORK]      = arguments_setnetwork,
	[QPLAY_CMD_SETTRACKSINFO]   = arguments_settrackinfo,
	[QPLAY_CMD_UNKNOWN]         = NULL
};

// Our 'instance' variables.
extern struct service qplay_service_;   // Defined below.
static variable_container_t *state_variables_ = NULL;

static ithread_mutex_t qplay_mutex;
ithread_mutex_t g_plist_mutex;
char glock_cnt;

void g_plist_lock(void)
{
	assert(glock_cnt==0);
	glock_cnt++;
	ithread_mutex_lock(&g_plist_mutex);
}

void g_plist_unlock(void)
{
	assert(glock_cnt!=0);
	glock_cnt--;
	ithread_mutex_unlock(&g_plist_mutex);
}

static void service_lock(void)
{
	ithread_mutex_lock(&transport_mutex);
	g_plist_lock();
	g_qplayid = g_playlist.qplayid;
	g_plist_unlock();
}

static void service_unlock(void)
{
	ithread_mutex_unlock(&transport_mutex);
}

static int get_max_tracks(struct action_event *event)
{
    upnp_add_response(event, "MaxTracks", "200");
	return 0;
}

static int get_track_cnt(struct action_event *event)
{
	return 0;
}

static int get_track_info(struct action_event *event)
{
    if(allmetadata)
        upnp_add_response(event, "TracksMetaData", allmetadata);
    else
        upnp_add_response(event, "TracksMetaData", "");

    return 0;
}

static int insert_track_info(struct action_event *event)
{
	return 0;
}

static int qauth(struct action_event *event)
{
    int i;
    int len = 0;
    unsigned char buf[256]={0};
    const unsigned char psk[16] ={ 0xe6, 0xbe, 0x9c, 0xe8, 0xb5, 0xb7, 0xe7, 0xa7,
                                 0x91, 0xe6, 0x8a, 0x80, 0x3a, 0x57, 0x69, 0x2d};

    const unsigned char *seed = upnp_get_string(event, "Seed");
    unsigned char digest[16];
    char md5string[33]={0};
    MD5_CTX context;

    if((len=strlen(seed)) == 0)
        return -1;

    memcpy(buf, seed, len);
    memcpy(buf + len, psk, sizeof(psk));

    MD5Init(&context);
    MD5Update(&context, buf, strlen(buf));
    MD5Final(digest, &context);
    for(i=0; i < 16; i++)
        sprintf(&md5string[i*2], "%02x", digest[i]);

    upnp_add_response(event, "Code", md5string);
    upnp_add_response(event, "MID", "62900065");
    upnp_add_response(event, "DID", "TonlyDMR");

    /**/

    return 0;
}

static int remove_track(struct action_event *event)
{
	return 0;
}

static int set_network(struct action_event *event)
{
    char cmd[512] = {0};
	const unsigned char *ssid = upnp_get_string(event, "SSID");
	const unsigned char *pass = upnp_get_string(event, "Key");

    if(strlen(ssid) == 0)
        return -1;

    if(strlen(pass) == 0)
        sprintf(cmd, "/lib/wdk/omnicfg_apply 3 %s",ssid);
    else
        sprintf(cmd, "/lib/wdk/omnicfg_apply 3 %s %s",ssid, pass);

    system(cmd);
	return 0;
}

static void plist_alloc(int total)
{
    g_plist_lock();
    if(!g_playlist.plist)
    {
        int i;
        g_playlist.plist = (struct playback_t *)malloc(total*sizeof(struct playback_t));
        g_playlist.totalcnt = total;
    }
    g_plist_unlock();
}

static void plist_clear(char clear)
{
    int i = 0;

    if(g_playlist.totalcnt)
    {
        for(i=0 ; i<g_playlist.totalcnt; i++)
        {
            struct playback_t *track = (struct playback_t *)&(g_playlist.plist[i]);
            free(track->trackuri);
            free(track->metadata);
        }
        free(g_playlist.plist);
        if(clear)
        {
            free(g_playlist.qplayid);
            g_playlist.qplayid = NULL;
            g_playlist.curidx = 0;
        }
        g_playlist.plist = NULL;
        g_playlist.totalcnt = 0;
    }

    return;
}

static char parseqplaytracksinfo(const char *metadata, const char *start, const char *next)
{
    json_object *jobj=NULL, *jmeta=NULL, *jtrack=NULL, *juri=NULL, *jhttp=NULL, *jtitle=NULL;
    int arraylen,urilen,i,j;
    char action = 0;
    FILE *fd;
    int idx;
    char curidx = 0;

#define Q_CLEAR 0x1
#define Q_STOP 0x2
#define Q_CLEAR_STOP 0x3

    if(!strcmp(start, "-1"))
        action |= Q_CLEAR;
    if(!strcmp(next, "-1"))
        action |= Q_STOP;

    idx = atoi(next);

    /*
     * Currently not familiar with mplayer playlist operation, just skip start and next process
     * only parse json format and add in playlist
     */
    if(action == Q_CLEAR_STOP)
    {
        for(i=0; i<g_playlist.totalcnt; i++)
            output_set_uri(NULL, NULL);
    }
    else if (action == Q_CLEAR)
    {
        if(g_playlist.totalcnt != 0)
        {
            for(i=0; i<g_playlist.totalcnt; i++)
                output_set_uri(NULL, NULL);
            plist_clear(0);
        }

    }
    else if(action == Q_STOP) {
        output_stop();
    }
    /*END*/

    jobj = json_tokener_parse((char *)metadata);
    jmeta = json_object_object_get(jobj, "TracksMetaData");
    arraylen = json_object_array_length(jmeta);

    plist_alloc(arraylen);
    fd=fopen("/tmp/playlist", "w");
    for (i = 0; i < arraylen; i++)
    {
        jtrack = json_object_array_get_idx(jmeta, i);
        jtitle = json_object_object_get(jtrack, "title");
        juri = json_object_object_get(jtrack,"trackURIs");
        urilen = json_object_array_length(juri);
        for(j=0; j<urilen;j++)
        {
            jhttp = json_object_array_get_idx(juri, j);
        }

        if(fd)
        {
            char *http;
            http = json_object_get_string(jhttp);
            fprintf(fd, "%s\n", http);
        }
        g_plist_lock();
        struct playback_t *track = (struct playback_t *)&g_playlist.plist[i];
        g_plist_unlock();
        if(!track || i >= 200)
            break;
        track->trackuri = strdup(json_object_get_string(jhttp));
        track->metadata = strdup(json_object_get_string(jtrack));
        output_set_uri(track->trackuri, NULL);
    }

    if(fd)
        fclose(fd);
    if(arraylen)
    {
        json_object_put(jtrack);
        json_object_put(jtitle);
        json_object_put(juri);
        json_object_put(jhttp);
    }
    json_object_put(jobj);
    json_object_put(jmeta);

    return arraylen;
}

static int set_track_info(struct action_event *event)
{
    char cnt;
    char songnum[8]={0};
    char *id, *queueid;

	const char *qid = upnp_get_string(event, "QueueID");
	const char *next = upnp_get_string(event, "NextIndex");
	const char *start = upnp_get_string(event, "StartingIndex");
	const char *metadata = upnp_get_string(event, "TracksMetaData");

    service_lock();
    id = g_qplayid+8;
    if(strcmp(id, qid))
    {
        upnp_set_error(event, 718, "Invalid QueueID");
        return -1;
    }
    if(!next || !start || !metadata)
        return -1;

    cnt = parseqplaytracksinfo(metadata,start,next);

    if(allmetadata)
    {
        free(allmetadata);
        allmetadata = NULL;
    }
    allmetadata = malloc(strlen(metadata));
    memcpy(allmetadata, metadata, strlen(metadata));

    sprintf(songnum, "%d", cnt);
    upnp_add_response(event, "NumberOfSuccess", songnum);

    service_unlock();
#if 0
    /*After we got SetTrackInfo we change 
     * CurrentURI and CurrentURIMetaData
     * */
    replace_var(TRANSPORT_VAR_AV_URI, queueid);
    replace_var(TRANSPORT_VAR_AV_URI_META, allmetadata);
#endif
    return 0;
}

static struct action qplay_actions[] = {
	[QPLAY_CMD_GETMAXTRACKS]    = {"GetMaxTracks", get_max_tracks},     //ok
	[QPLAY_CMD_GETTRACKSCOUNT]  = {"GetTracksCount", get_track_cnt},
	[QPLAY_CMD_GETTRACKSINFO]   = {"GetTracksInfo", get_track_info},    //ok
	[QPLAY_CMD_INSERTTRACKS]    = {"InsertTracks", insert_track_info},
	[QPLAY_CMD_QPLAYAUTH]       = {"QPlayAuth", qauth},                 //ok
	[QPLAY_CMD_REMOVETRACKS]    = {"RemoveTracks", remove_track},
	[QPLAY_CMD_SETNETWORK]      = {"SetNetwork", set_network},          //option
	[QPLAY_CMD_SETTRACKSINFO]   = {"SetTracksInfo", set_track_info},    //Need to sync player 
	[QPLAY_CMD_UNKNOWN]         = {NULL, NULL}
};

struct service *upnp_qplay_get_service(void) {
	if (qplay_service_.variable_container == NULL) {
		state_variables_ =
			VariableContainer_new(QPLAY_VAR_COUNT,
					      qplay_variable_names,
					      qplay_default_values);
		qplay_service_.variable_container = state_variables_;
	}
	return &qplay_service_;
}

void upnp_qplay_register_variable_listener(variable_change_listener_t cb,
					       void *userdata) {
	VariableContainer_register_callback(state_variables_, cb, userdata);
}

int qplay_init(const char *uri)
{
    int ret = 0;
    char *queueid = NULL;

    plist_clear(1);
    if(strstr(uri, QPLAYPREFIX))
    {
        queueid = malloc(strlen(uri)+1);
        if(!queueid) {
            ret = -1;
            goto done;
        }
        memcpy(queueid, uri, strlen(uri));
        queueid[strlen(uri)] = '\0';
        g_playlist.qplayid = queueid;
    }
    else
        ret = 1;
done:
    return ret;
}

struct service qplay_service_ = {
	.service_id =           QPLAY_SERVICE_ID,
	.service_type =         QPLAY_TYPE,
	.scpd_url =		        QPLAY_SCPD_URL,
	.control_url =		    QPLAY_CONTROL_URL,
	.event_url =		    QPLAY_EVENT_URL,
	.event_xml_ns =         NULL,
	.actions =              qplay_actions,
	.action_arguments =     argument_list,
	.variable_names =       qplay_variable_names,
	.variable_container =   NULL,
	.last_change =          NULL,
	.variable_meta =        qplay_var_meta,
	.variable_count =       QPLAY_VAR_UNKNOWN,
	.command_count =        QPLAY_CMD_UNKNOWN,
	.service_mutex =        &qplay_mutex
};
