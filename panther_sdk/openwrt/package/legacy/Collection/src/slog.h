#ifndef MY_LOG_H_
#define MY_LOG_H_
typedef enum LOG_TYPE_T{
	_ERR,
	_WAR,
	_IPT,
	_INF,
	MAX_LOG_TYPE_T
}log_type_t;
typedef struct LOG_CTRL_T{
	char *name;
	char *color;
}log_ctrl_t;
void set_xx(int xx);
extern void get_time_ms(char *ts,int size);
extern void get_local_time_str(char *ts,int size);
extern char log_ctrl_set[MAX_LOG_TYPE_T+1];
extern int log_init(char * fpath);
#define LOG_NOTE_PATH 	"/tmp/log_note.txt"
#define LOG_SET_PATH 	"/tmp/log_set"
extern void slog(log_type_t num_type,char *log_ctrl_set,const char *out_file,const char *ts,const char *file,const int line,const char *fmt,...);
#define tlog(type,x...) {char ts[16]="";\
						get_time_ms(ts,sizeof(ts));\
						slog(type,log_ctrl_set,NULL,ts,__FILE__,__LINE__,x);}	
	
#define dlog(type,x...) slog(type,log_ctrl_set,NULL,"",__FILE__,__LINE__,x);

#define err(x...) 	dlog(_ERR,x);
#define war(x...) 	dlog(_WAR,x);
#define ipt(x...) 	dlog(_IPT,x);
#define inf(x...) 	dlog(_INF,x);

#endif