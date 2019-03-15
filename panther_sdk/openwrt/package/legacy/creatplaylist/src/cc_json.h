#ifndef MY_JSON_H_
#define MY_JSON_H_
#include <json/json.h>
//#include "slog.h"
#define my_jso_put(x) ({\
		int ret=0;\
		if(!is_error(x)){\
			if(!json_object_put(x)){\
				ret= -1;\
			}else{\
				ret=0;\
			};\
		}else{\
			ret=-1;\
		}\
		ret;\
	})
#define my_jso_object_get(y,obj,key) ({\
		int ret=0;\
		y=json_object_object_get(obj,key);\
		if(is_error(y)){\
			ret=-1;\
		}else{\
			ret=0;\
		}\
		ret;\
	})
#define my_jso_get_string(y,x) ({\
		int ret=0;\
		y=json_object_get_string(x);\
		if(NULL==y){\
			ret=-1;\
		}else{\
			ret=0;\
		}\
		ret;\
	})
#define my_jso_array_get_idx(y,x,i) ({\
		int ret=0;\
		y=json_object_array_get_idx(x,i);\
		if(is_error(y)){\
			ret=-1;\
		}else{\
			ret=0;\
		}\
		ret;\
	})
		
#define my_jso_tokener_parse(obj,x) ({\
		int ret=0;\
		struct json_object *obj=NULL;\
		obj=json_tokener_parse(x);\
		if(is_error(obj)){\
			ret=-1;\
		}else{\
			ret=0;\
		}\
		ret;\
	})
#define my_js_tokener_parse(y,x) ({\
		int ret=0;\
		y=json_tokener_parse(x);\
		if(is_error(y)){\
			ret=-1;\
		}else{\
			ret=0;\
		};\
		ret;\
	})
	
#define json_obj_get(x) ({\
		int ret=0;\
		ret;\
	})
#define JSON_INIT_STR 	"{ \"num\": 0, \"musiclist\": [] }"

typedef struct MATCH_GRUP_T{
	char *m_content_url	;
	char *m_col			;
	char *m_title		;
	char *m_artist		;
	char *m_album		;
	char *m_cover_url	;
	char *m_token		;
}match_grup_t;		

int cc_jso_get_str(char **p_str,struct json_object *obj,char *name);
int cc_jso_new(struct json_object **p_obj);
int cc_jso_invert(char *d_path,char *s_path);
int cc_jso_insert(char *d_path,char *s_path,int idx);
void convert_to_cc_jso_unit(struct json_object *d_obj,char *d_name,struct json_object *s_obj,char *s_name);
int build_cc_jso(struct json_object *d_obj,struct json_object *s_obj,match_grup_t *m_grup);
int cc_jso_get_m3u(char *m3u_path,char *json_path);

#endif
	
