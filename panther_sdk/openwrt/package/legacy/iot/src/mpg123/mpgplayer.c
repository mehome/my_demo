#define ME "main"

#include "mpgplayer.h"



#include <mpg123.h>
#include <out123.h>

#include "mpg123app.h"

#include <signal.h>

#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#define FALSE 0
#define TRUE  1

#include "httpget.h"

#include "common.h"

#include "myutils/debug.h"



mpg123_handle *mh = NULL;
off_t framenum;
off_t frames_left;
out123_handle *ao = NULL;
static long output_propflags = 0;

/* ThOr: pointers are not TRUE or FALSE */
static int skip_tracks = 0;
struct httpdata htd;
int fresh = TRUE;
int intflag = FALSE;
static int filept = -1;

static int network_sockets_used = 0; /* Win32 socket open/close Support */
/* File-global storage of command line arguments.
   They may be needed for cleanup after charset conversion. */
static char **argv = NULL;
static int    argc = 0;

/* Cleanup marker to know that we intiialized libmpg123 already. */
static int cleanup_mpg123 = FALSE;

static long new_header = FALSE;
static char *prebuffer = NULL;
static size_t prebuffer_size = 0;
static size_t prebuffer_fill = 0;
static size_t minbytes = 0;

static int please_stop;
static pthread_t mpg123Thread;
static pthread_t player_running;
//static sem_t mpg123Sem;
static int mpg123Wait = 0;
static MpgPlayerState mpg123PlayerState = PLAYER_STATE_STOP;

static int mpg123State = MPG123_STATE_NONE;
static pthread_mutex_t mpg123Mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static LINK_LIST_HEAD(mpg123UrlList);

static int urlIndex = 0;

void safe_exit(int code);

static void play_prebuffer(void)
{
	/* Ensure that the prebuffer bit has been posted. */
	if(prebuffer_fill)
	{
		if(out123_play(ao, prebuffer, prebuffer_fill) < prebuffer_fill)
		{
			error("Deep trouble! Cannot flush to my output anymore!");
			safe_exit(133);
		}
		prebuffer_fill = 0;
	}
}

/* Drain output device/buffer, but still give the option to interrupt things. */
static void controlled_drain(void)
{
	int framesize;
	size_t drain_block;

	play_prebuffer();

	if(intflag || !out123_buffered(ao))
		return;
	if(out123_getformat(ao, NULL, NULL, NULL, &framesize))
		return;
	drain_block = 1152*framesize;
	do
	{
		out123_ndrain(ao, drain_block);

	}
	while(!intflag && out123_buffered(ao));

}

static void check_fatal_output(int code)
{
	if(code)
	{
		error( "out123 error %i: %s"
			,	out123_errcode(ao), out123_strerror(ao) );
		safe_exit(code);
	}
}


void safe_exit(int code)
{
	char *dummy, *dammy;

	if(prebuffer)
		free(prebuffer);
	if(!code)
		controlled_drain();
	if(intflag)
		out123_drop(ao);
	out123_del(ao);

	if(mh != NULL) mpg123_delete(mh);

	if(cleanup_mpg123) mpg123_exit();

	httpdata_free(&htd);
	
	//exit(code);
}


static int open_track_fd (void)
{
	/* Let reader handle invalid filept */
	if(mpg123_open_fd(mh, filept) != MPG123_OK)
	{
		error("Cannot open fd %i: %s", filept, mpg123_strerror(mh));
		return 0;
	}
	debug("Track successfully opened.");
	fresh = TRUE;
	return 1;
	/*1 for success, 0 for failure */
}

/* 1 on success, 0 on failure */
int open_track(char *fname)
{
	debug("open :%s",fname);
	filept=-1;
	httpdata_reset(&htd);
	if(MPG123_OK != mpg123_param(mh, MPG123_ICY_INTERVAL, 0, 0))
		error("Cannot (re)set ICY interval: %s", mpg123_strerror(mh));
	if(!strcmp(fname, "-"))
	{
		filept = STDIN_FILENO;

		return open_track_fd();
	}
	else if (!strncmp(fname, "http://", 7)) /* http stream */
	{

		filept = http_open(fname, &htd);
		network_sockets_used = 1;
		/* utf-8 encoded URLs might not work under Win32 */
		
		/* now check if we got sth. and if we got sth. good */
		if(    (filept >= 0) && (htd.content_type.p != NULL)
			   && !(debunk_mime(htd.content_type.p) & IS_FILE) )
		{
			error("Unknown mpeg MIME type %s - is it perhaps a playlist (use -@)?", htd.content_type.p == NULL ? "<nil>" : htd.content_type.p);
			return 0;
		}
		if(filept < 0)
		{
			error("Access to http resource %s failed.", fname);
			return 0;
		}
		if(MPG123_OK != mpg123_param(mh, MPG123_ICY_INTERVAL, htd.icy_interval, 0))
			error("Cannot set ICY interval: %s", mpg123_strerror(mh));
		
	}


	debug("OK... going to finally open.");
	/* Now hook up the decoder on the opened stream or the file. */
	if(network_sockets_used) 
	{
		return open_track_fd();
	}
	else if(mpg123_open(mh, fname) != MPG123_OK)
	{
		error("Cannot open %s: %s", fname, mpg123_strerror(mh));
		return 0;
	}
	info("Track successfully opened.");

	fresh = TRUE;

	return 1;
}

/* for symmetry */
void close_track(void)
{
	mpg123_close(mh);

	network_sockets_used = 0;
	if(filept > -1) close(filept);
	filept = -1;
}

/* return 1 on success, 0 on failure */
int play_frame(void)
{
	unsigned char *audio;
	int mc;
	size_t bytes = 0;
	
	mc = mpg123_decode_frame(mh, &framenum, &audio, &bytes);
	mpg123_getstate(mh, MPG123_FRESH_DECODER, &new_header, NULL);

	/* Play what is there to play (starting with second decode_frame call!) */
	if(bytes)
	{

	
		if(bytes < minbytes && !prebuffer_fill)
		{
			/* Postpone playback of little buffers until large buffers can
				follow them right away, preventing underruns. */
			if(prebuffer_size < minbytes)
			{
				if(prebuffer)
					free(prebuffer);
				if(!(prebuffer = malloc(minbytes)))
					safe_exit(11);
				prebuffer_size = minbytes;
			}
			memcpy(prebuffer, audio, bytes);
			prebuffer_fill = bytes;
			bytes = 0;
		}

	}
	/* The bytes could have been postponed to later. */
	if(bytes)
	{
		unsigned char *playbuf = audio;
		if(prebuffer_fill)
		{
			size_t missing;
			/* This is tricky: The small piece needs to be filled up. Playing 10
			   pcm frames will always trigger underruns with ALSA, dammit!
			   This grabs some data from current frame, unless it itself would
			   end up smaller than the prebuffer. Ending up empty is fine. */
			if(  prebuffer_fill < prebuffer_size
			  && (  bytes <= (missing=prebuffer_size-prebuffer_fill)
			     || bytes >= missing+prebuffer_size ) )
			{
				if(bytes < missing)
					missing=bytes;
				memcpy(prebuffer+prebuffer_fill, playbuf, missing);
				playbuf += missing;
				bytes -= missing;
				prebuffer_fill += missing;
			}
			if(   out123_play(ao, prebuffer, prebuffer_fill) < prebuffer_fill
			   && !intflag )
			{
				error("Deep trouble! Cannot flush to my output anymore!");
				safe_exit(133);
			}
			prebuffer_fill = 0;
		}
		/* Interrupt here doesn't necessarily interrupt out123_play().
		   I wonder if that makes us miss errors. Actual issues should
		   just be postponed. */
		if(bytes && !intflag) /* Previous piece could already be interrupted. */
		if(out123_play(ao, playbuf, bytes) < bytes && !intflag)
		{
			error("Deep trouble! Cannot flush to my output anymore!");
			safe_exit(133);
		}
	}
	/* Special actions and errors. */
	if(mc != MPG123_OK)
	{
		if(mc == MPG123_ERR || mc == MPG123_DONE)
		{
			if(mc == MPG123_ERR) error("...in decoding next frame: %s", mpg123_strerror(mh));
			return 0;
		}
		if(mc == MPG123_NO_SPACE)
		{
			error("I have not enough output space? I didn't plan for this.");
			return 0;
		}
		if(mc == MPG123_NEW_FORMAT)
		{
			long rate;
			int channels;
			int encoding;
			play_prebuffer(); /* Make sure we got rid of old data. */
			mpg123_getformat(mh, &rate, &channels, &encoding);
			/* A layer I frame duration at minimum for live outputs. */
			if(output_propflags & OUT123_PROP_LIVE)
				minbytes = out123_encsize(encoding)*channels*384;
			else
				minbytes = 0;

			
			new_header = TRUE;
			check_fatal_output(out123_start(ao, rate, channels, encoding));
			/* We may take some time feeding proper data, so pause by default. */
			out123_pause(ao);
		}
	}

	return 1;
}


static void Mpg123Init()
{
	int result;
	char *fname;
	mpg123_pars *mp;

	result = mpg123_init();
	if(result != MPG123_OK)
	{
		error("Cannot initialize mpg123 library: %s", mpg123_plain_strerror(result));
		safe_exit(77);
	}
	cleanup_mpg123 = TRUE;

	mp = mpg123_new_pars(&result); /* This may get leaked on premature exit(), which is mainly a cosmetic issue... */
	if(mp == NULL)
	{
		error("Crap! Cannot get mpg123 parameters: %s", mpg123_plain_strerror(result));
		safe_exit(78);
	}

	httpdata_init(&htd);

	mh = mpg123_parnew(mp, NULL, &result);
	if(mh == NULL)
	{
		error("Crap! Cannot get a mpg123 handle: %s", mpg123_plain_strerror(result));
		safe_exit(77);
	}
	mpg123_delete_pars(mp); /* Don't need the parameters anymore ,they're in the handle now. */
	ao = out123_new();
	if(!ao)
	{
		safe_exit(97);
	}

	check_fatal_output(out123_open(ao,"alsa", NULL ));
	out123_getparam_int(ao, OUT123_PROPFLAGS, &output_propflags);

#ifdef HAVE_TERMIOS
	term_init();
#endif

}

static void Mpg123Deinit()
{
#ifdef HAVE_TERMIOS
	term_exit();
#endif
	safe_exit(0);
}


static void mpg123_play(char *fname)
{
	debug("play :%s",fname);
	open_track(fname);
	
	while (1) 
    {
        if(!play_frame()){
            info("play done...");
            break;
        }
    }
    close_track();

}



/* oh, what a mess... */
void next_track(void)
{
	intflag = TRUE;
	++skip_tracks;
}

void prev_track(void)
{
	//if(pl.pos > 2) pl.pos -= 2;
	//else pl.pos = skip_tracks = 0;

	next_track();
}
void next_dir(void)
{
	//size_t npos = pl.pos ? pl.pos-1 : 0;
	//do { ++npos; }
	//while(npos < pl.fill && !cmp_dir(pl.list[npos-1].url, pl.list[npos].url));
	//pl.pos = npos;
	next_track();
}

void prev_dir(void)
{
	//size_t npos = pl.pos ? pl.pos-1 : 0;
	/* 1. Find end of previous directory. */
	//if(npos && npos < pl.fill)
	//do { --npos; }
	//while(npos && !cmp_dir(pl.list[npos+1].url, pl.list[npos].url));
	/* npos == the last track of previous directory */
	/* 2. Find the first track of this directory */
	//if(npos < pl.fill)
	//while(npos && !cmp_dir(pl.list[npos-1].url, pl.list[npos].url))
	//{ --npos; }
	//pl.pos = npos;
	next_track();
}
void set_intflag(int val)
{
	debug("set_intflag TRUE");
	intflag = val;
	skip_tracks = 0;
}

void get_intflag()
{
	int ret = 0;
	ret = (intflag == 0);	//1:表示被打断
	return ret;
}
void StartMpgPlay()
{
	pthread_mutex_lock(&mtx);
	pthread_mutex_lock(&mpg123Mtx);
	pthread_cond_signal(&cond);  
	pthread_mutex_unlock(&mpg123Mtx); 
	pthread_mutex_unlock(&mtx);
}
void AddMpg123Url(char *url)
{
	Mpg123Url *tmp = NULL;
	int i = 0;
	tmp = calloc(1, sizeof(Mpg123Url));
    
	if(tmp == NULL)
	{
		error("calloc for Mpg123Url failed");
		return ;
	}
    tmp->url = calloc(256,1);
    strcpy(tmp->url,url);
	pthread_mutex_lock(&mtx);
	//tmp->url = url;
	list_add_tail(&(tmp->list), &mpg123UrlList);	//将要播放的语音数据存放链表中
    
	debug("add [%d] %s", ++urlIndex, url);
	pthread_mutex_unlock(&mtx);
	StartMpgPlay();									//通知mpg123线程播放
}


void SetMpg123PlayerState(int state)
{
	pthread_mutex_lock(&mpg123Mtx);
	mpg123PlayerState = state;
	pthread_mutex_unlock(&mpg123Mtx);
}
int IsMpg123PlayerPlaying()
{
	int ret;
	pthread_mutex_lock(&mpg123Mtx);
	ret = (mpg123PlayerState == PLAYER_STATE_PLAY);
	pthread_mutex_unlock(&mpg123Mtx);
	return ret;
}
int GetMpg123PlayerState()
{
	int ret;
	pthread_mutex_lock(&mpg123Mtx);
	ret = mpg123PlayerState;
	pthread_mutex_unlock(&mpg123Mtx);
	return ret; 
}

int IsMpg123PlayerPause()
{
	int ret;
	pthread_mutex_lock(&mpg123Mtx);
	ret = (mpg123PlayerState == PLAYER_STATE_PAUSE);
	pthread_mutex_unlock(&mpg123Mtx);
	return ret;
}


int IsMpg123Cancled()
{
	int ret;
	pthread_mutex_lock(&mpg123Mtx);
	ret = (mpg123State == MPG123_STATE_CANCLE);
	pthread_mutex_unlock(&mpg123Mtx);
	return ret;
}

void SetMpg123State(int state)
{
	pthread_mutex_lock(&mpg123Mtx);
	mpg123State = state;
	pthread_mutex_unlock(&mpg123Mtx);
}
int IsMpg123Finshed()
{
	int ret;
	pthread_mutex_lock(&mpg123Mtx);
	ret = (mpg123State == MPG123_STATE_NONE);
	pthread_mutex_unlock(&mpg123Mtx);
	
	return ret;
}
int IsMpg123Done()
{
	int ret;
	pthread_mutex_lock(&mpg123Mtx);
	info("mpg123State == %d\n",mpg123State);
	ret = (mpg123State == MPG123_STATE_DONE);
	pthread_mutex_unlock(&mpg123Mtx);
	return ret;
}

void SetMpg123Cancle()
{
	pthread_mutex_lock(&mpg123Mtx);
	mpg123State = MPG123_STATE_CANCLE;
	pthread_mutex_unlock(&mpg123Mtx);
}


void StartMpg123Thread()
{
	pthread_mutex_lock(&mpg123Mtx);
	//if(wait) {
		debug("sem_post(&Http_sem);");
		//sem_post(&mpg123Sem);
	//}
	pthread_mutex_lock(&mpg123Mtx);
}

static void SetMpg123Wait(int wait)
{
	pthread_mutex_lock(&mpg123Mtx);
	mpg123Wait = wait;
	pthread_mutex_unlock(&mpg123Mtx);
}

 int GetMpg123Wait()
{
	int ret = 0;
	pthread_mutex_lock(&mpg123Mtx);
	ret = (mpg123Wait == 0);
	pthread_mutex_unlock(&mpg123Mtx);
	return ret;
}


static void freeMpgUrlList(struct list_head *head)
{
	pthread_mutex_lock(&mpg123Mtx);
    if (list_empty(head)){
		pthread_mutex_unlock(&mpg123Mtx);
		return;
	}
        
	
    struct list_head *pos, *n;

    list_for_each_safe(pos, n, head) 
    {
		 Mpg123Url *item = (Mpg123Url *)list_entry(pos, Mpg123Url, list);
		 if(item) {
		 	debug("free [%d] %s",urlIndex, item->url);
			urlIndex--;
			free(item->url);
			free(item);
			item = NULL;
		 }
        list_del(pos);
    }
	pthread_mutex_unlock(&mpg123Mtx);
}


static MpgPlayerState lastState=PLAYER_STATE_STOP;

void SetLastState()
{
	lastState = GetMpg123PlayerState();
}
int IsMpg123PlayerStateChange()
{
	int ret;
	int state ;
	state = GetMpg123PlayerState();
	ret = (lastState != state);
	return ret;
}
static void *Mpg123Thread(void *arg)
{
	char *url = NULL;
	Mpg123Url *item = NULL;
	//struct list_head mpg123UrlList;
	int i = 0;
    Mpg123Init();
    while(1) 
    {
    	int j = 0;
		debug("MPG123_STATE_NONE ...");
		SetMpg123State(MPG123_STATE_NONE);

		SetMpg123Wait(1);
		pthread_mutex_lock(&mtx);
		while(list_empty(&mpg123UrlList)) 
        {
			 //MpdResume();

			 //阻塞等待，条件变量：等待唤醒该线程起来工作
			 debug("waiting.........................");
			 pthread_cond_wait(&cond, &mtx);
		}
		pthread_mutex_unlock(&mtx);
		SetMpg123Wait(0);
		SetMpg123State(MPG123_STATE_STARTED);
		struct list_head *pos = NULL;
		long endTime = getCurrentTime();//获取当前的时间，单位为:ms
		//ShowSpendTime("start play", endTime);
		debug("start play");
		//pthread_mutex_lock(&mtx);
		list_for_each(pos, &mpg123UrlList) 	//循环遍历整个链表
        {
			//pthread_mutex_lock(&mtx);
			++j;
			Mpg123Url *item = (Mpg123Url *)list_entry(pos, Mpg123Url, list);//查找取出链表中的成员，从head开始
			//pthread_mutex_unlock(&mtx);
			url = item->url;
			if(IsMpg123Cancled()) {
				goto exit;
			}
			long start_time = getCurrentTime();
			open_track(url);		//将语音链接进行播放
			long end_time = getCurrentTime();
			debug("j = %d ,open %s 耗时：%d.%d",j,url, (end_time-start_time)/1000, (end_time-start_time)%1000);
			while (!intflag) 
            {
				if(IsMpg123Cancled()) 
                {
					close_track();
					error("Mpg123 Cancle ...");
					goto exit;
				}	
				SetMpg123State(MPG123_STATE_ONGING);
				SetMpg123PlayerState(PLAYER_STATE_PLAY);
				if(!play_frame()){
					debug("play %s done...",url);
					break;
				}

#ifdef HAVE_TERMIOS
				term_control(mh, ao);
#endif		
			}
			close_track();
			//pthread_mutex_lock(&mtx);
		}
		//pthread_mutex_unlock(&mtx);
        
#ifdef TURN_ON_SLEEP_MODE
		if(!IsMpg123Cancled()) {
			if(ShouldEnterSleepMode()) {
				error(" ShouldEnterSleepMode ....");
				eval("uartdfifo.sh", "tlkoff");
			} else {
				error(" start ai mode ....");
				eval("uartdfifo.sh", "tlkon");
			}
		}
#endif
		
		
exit:	
		SetMpg123PlayerState(PLAYER_STATE_STOP);
		if(IsMpg123PlayerStateChange()) {				
			debug("IsMpg123PlayerStateChange");
			IotDeviceStatusReport();
		}
		SetLastState();
        set_intflag(0);
//		info("mpg123UrlList befor");
		pthread_mutex_lock(&mtx);
		//将链表的数据释放掉，这里的处理即每次获取一条TTS语音的链接"url"时放入链表中，
		//通过mpg123播放完成后，即释放掉链表的资源，保证每次链表中只有一条TTS语音链接存在
		freeMpgUrlList(&mpg123UrlList);		
		pthread_mutex_unlock(&mtx);
//		info("mpg123UrlList after");
		//SetMpg123State(MPG123_STATE_DONE);
		
	}
    Mpg123Deinit();
}


void CreateMpg123Thread(void *arg)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);
	iRet = pthread_create(&mpg123Thread, &a_thread_attr, Mpg123Thread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
	return 0;
}
void DestoryMpg123Thread()
{
	if (mpg123Thread != 0)
	{
		pthread_join(mpg123Thread, NULL);
		debug("mpg123Thread destroy end...\n");
	}	
}


void player_stop(void)
{
	please_stop = 1;
	debug("player_stop...");
	//pthread_kill(mpg123Thread, SIGUSR1);
	debug("pthread_kill player_thread");
	pthread_join(mpg123Thread, NULL);
	player_running = 0;
}




