#include "status.h"
#include <mpd/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
//#include <cdb.h>

#include "../globalParam.h"

char g_byDeleteAlertFlag = 0;


pthread_t getMpdStatus_Pthread;

extern GLOBALPRARM_STRU g;

#define V_QUIET 0
#define V_DEFAULT 1
#define V_VERBOSE 2

typedef struct {
	const char *host;
	const char *port_str;
	int port;
	const char *password;
	const char *format;
	int verbosity; // 0 for quiet, 1 for default, 2 for verbose
	bool wait;

	bool custom_format;
} options_t;

options_t options = {
	.verbosity = V_DEFAULT,
	.password = NULL,
	.port_str = NULL,
	.format = "[%name%: &[%artist% - ]%title%]|%name%|[%artist% - ]%title%|%file%",
};

int 
printErrorAndExit(struct mpd_connection *conn)
{
	assert(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS);

	const char *message = mpd_connection_get_error_message(conn);
	if (mpd_connection_get_error(conn) == MPD_ERROR_SERVER)
		/* messages received from the server are UTF-8; the
		   rest is either US-ASCII or locale */
		//message = charset_from_utf8(message);

	fprintf(stderr, "alexa-> mpd error: %s\n", message);
	mpd_connection_free(conn);

	return -1;
	//exit(EXIT_FAILURE);
}

void
send_password(const char *password, struct mpd_connection *conn)
{
	if (!mpd_run_password(conn, password))
		printErrorAndExit(conn);
}

bool mpd_connection_update(struct mpd_connection **p_conn)
{
	struct mpd_connection *new_conn = NULL,*old_conn= *p_conn;
	struct mpd_status  *mpds = NULL;

	if(old_conn) mpds = mpd_run_status(old_conn);

	if(old_conn && mpds) return true;

	if(old_conn){
		mpd_connection_free(old_conn);
		*p_conn=old_conn = NULL;
	}
	
	new_conn = mpd_connection_new(options.host, options.port, 0);
	if (new_conn == NULL) {
		fputs("Out of memory\n", stderr);
		return false;
	}
	
	if (mpd_connection_get_error(new_conn) != MPD_ERROR_SUCCESS){
		if (0 != printErrorAndExit(new_conn)){
			return false;
		}
	}

	if(options.password) send_password(options.password, new_conn);

	*p_conn=new_conn;
	
	return true;
}

struct mpd_connection *
setup_connection(void)
{
	struct mpd_connection *conn = mpd_connection_new(options.host, options.port, 0);
	if (conn == NULL) {
		fputs("Out of memory\n", stderr);
		//exit(EXIT_FAILURE);
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
		if (0 != printErrorAndExit(conn))
			return NULL;

	if(options.password)
		send_password(options.password, conn);

	return conn;
}

int GetMpcVolume(void)
{
	int iVolume = 0;
	/*char *pTempBuffer = NULL;
	char byVolume[60] = {0};
	char *pTempVolume = NULL;

	pTempBuffer = GetScriptReturnVlue("mpc volume", "r");

	if (0 == strncmp(pTempBuffer, "volume:100%", sizeof("volume:100%") - 1))
		return 100;

	memcpy(byVolume, &pTempBuffer[8], 2);

	iVolume = atoi(byVolume);*/

	iVolume = cdb_get_int("$ra_vol", 0);

	return iVolume;
}

int SetMpcVolume(int iVolume)
{
	//int iVolume = 0;
	/*char *pTempBuffer = NULL;
	char byVolume[60] = {0};
	char *pTempVolume = NULL;

	pTempBuffer = GetScriptReturnVlue("mpc volume", "r");

	if (0 == strncmp(pTempBuffer, "volume:100%", sizeof("volume:100%") - 1))
		return 100;

	memcpy(byVolume, &pTempBuffer[8], 2);

	iVolume = atoi(byVolume);*/

	cdb_set_int("$ra_vol", iVolume);

	return iVolume;
}




// 0 查询成功，其它失败
int QueryMpdStatus(void)
{
	int iRet = -1;
	//int mode ;
	static struct mpd_connection *conn = NULL;

	// AudioPlayer	
	//mode = cdb_get_int("$current_play_mode", 4);
	
	if (AUDIO_STATUS_PLAY == GetPlayStatus())
	{
		/*if (NULL == g.CurrentPlayDirective.m_PlayToken)
		{
			char *playToken = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.token");
			if ((playToken != NULL) && (strlen(playToken) > 3))
			{
				g.CurrentPlayDirective.m_PlayToken = strdup(playToken);
			}
			if (playToken)
				free(playToken);
		}*/

		conn = setup_connection();

		//if (mpd_connection_update(&conn) && conn != NULL) 
		if (conn != NULL)
		{
			if (print_status(conn) != -1)
			{
				//UpDateAlexaCommonJsonStr();
				mpd_connection_free(conn);
				iRet = 0;
			}
		}
		else
		{
			//cdb_set_int("$current_play_mode", 0);
			SetPlayStatus(AUDIO_STATUS_STOP);
			iRet = -1;
		}

		conn = NULL;
	}
	return iRet;
}

void *GetMpdStatus(void)
{
	while (1)
	{
		QueryMpdStatus();

		usleep(1000 * 200);
	}
}

void getMpdStatusthread_wait(void)
{
	if (getMpdStatus_Pthread != 0)
	{
		pthread_join(getMpdStatus_Pthread, NULL);
		DEBUG_PRINTF("getMpdStatus_Pthread end...\n");
	}
}

void CreateGetMpdStatusPthread(void)
{
	int iRet;

	DEBUG_PRINTF("CreateGetMpdStatusPthread..");

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);

	iRet = pthread_create(&getMpdStatus_Pthread, &a_thread_attr, GetMpdStatus, NULL);
	//iRet = pthread_create(&getMpdStatus_Pthread, NULL, GetMpdStatus, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}



