#ifndef __DOSS_H__
#define     __DOSS_H__

typedef enum {
	SINGLE_SEARCH=0,
	LIST_SEARCH,
	SONG_SEARCH,
	SINGER_SEARCH,
	MAX_STYLE
}search_style_t;
	
typedef struct {
	char* cmd;
	char* name;
	char* key;
} scene_item_t;

typedef struct {
	char* music_name;
	char* singer_name;
} song_key_t;

typedef struct {
	char *sence_key;
	song_key_t song_key;
}search_key_t;

#define DEFINE_SCENE_CMD_TABLE static scene_item_t scene_cmd_table[]={ \
	{"MCU+PLY+REG","rege",			"热歌"	},\
	{"MCU+PLY+JDG","jingdian",		"经典"	},\
	{"MCU+PLY+DJG","dj",			"dj"	},\
	{"MCU+PLY+YWG","yingwen",		"英文"	},\
	{"MCU+PLY+SGG","shanggan",		"伤感"	},\
	{"MCU+PLY+EGG","erge",			"儿歌"	},\
	{"MCU+PLY+QYY","qingyinyue",	"轻音乐"},\ 	
    {"MCU+PLY+CAI","cai",			"猜"		}\
};
#define HEAP_FREE(x) do {free(x);x=NULL; }while(0)
#define FREE(x) if(x)do {free(x);x=NULL;}while(0)

#define EFECTTIVE_LENTH 32
void FreeSearchKey(search_key_t **key);


typedef enum {
	MUSIC_TYPE_PHONE = 0,
	MUSIC_TYPE_SERVER,
	MUSIC_TYPE_TTPOD,
	MUSIC_TYPE_MIGU,
	MUSIC_TYPE_LOCAL ,
}MusicType;


#endif
