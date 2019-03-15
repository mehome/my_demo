#include "cc_json.h"
#include "utils.h"
#include <string.h>
//#include "flags.h"
//#include "search_and_play.h"

int cc_jso_new(struct json_object **p_obj){
	int fail=1;
	char *str=NULL;
	struct json_object *obj=NULL;
	if(!(str=strdup(JSON_INIT_STR))){
		goto Err0;
	}	
	obj=json_tokener_parse(str);
	if(is_error(obj)){
		goto Err1;
	}
	*p_obj=obj;//ipt(json_object_get_string(obj));
	fail=0;
Err1:
	FREE(str);
Err0:
	if(fail)return -1;
	return 0;
}
#if 1
int cc_jso_invert(char *d_path,char *s_path)
{
	int fail=1,count=0,i=0;
	char *s_str=NULL,*d_str=NULL;
	struct json_object *s_obj=NULL,*s_array_obj=NULL,*d_obj=NULL,*d_array_obj=NULL;
	if(read_str_from_file(&s_str,s_path,"r")<0){	
		goto Err0;
	}
	s_obj=json_tokener_parse(s_str);
	if(is_error(s_obj)){
		goto Err1;
	}
	s_array_obj=json_object_object_get(s_obj,"musiclist");
	if(is_error(s_array_obj)){
		goto Err1;
	}
	if((count=json_object_array_length(s_array_obj))<1){
		goto Err1;
	}	
	if(cc_jso_new(&d_obj)<0){
		goto Err1;
	}
	d_array_obj=json_object_object_get(d_obj,"musiclist");
	if(is_error(d_array_obj)){
		goto Err2;
	}	
	for(i=0;i<count;i++){
		struct json_object *s_idx_obj=NULL;	
		s_idx_obj=json_object_array_get_idx(s_array_obj,count-i-1);
		if(is_error(s_idx_obj)){
			goto Err2;
		}
		json_object_array_add(d_array_obj,json_object_get(s_idx_obj));		
	}
	d_str=json_object_get_string(d_obj);
	if(!d_str){
		goto Err2;
	}
	if(write_str_to_file(d_path,d_str,"w")<0){
		goto Err2;
	}
	fail=0;
Err2:
	my_jso_put(s_obj);
Err1:
	FREE(s_str);
Err0:
	if(fail)return -1;
	return 0;
}


int cc_jso_insert(char *d_path,char *s_path,int idx)
{
	int fail=1,s_count=0,d_count=NULL,i=0;
	char *s_str=NULL,*d_str=NULL,*dd_str=NULL;
	struct json_object *s_obj=NULL,*s_array_obj=NULL,*d_obj=NULL,*d_array_obj=NULL,*t_array_obj=NULL;
	if(read_str_from_file(&s_str,s_path,"r")<0){	
		goto Err0;
	}
	s_obj=json_tokener_parse(s_str);
	if(is_error(s_obj)){
		goto Err1;
	}
	s_array_obj=json_object_object_get(s_obj,"musiclist");
	if(is_error(s_array_obj)){
		my_popen("cat /usr/script/playlists/httpPlayListInfoJson.txt > /usr/script/playlists/currentPlaylistInfoJson.txt");
		return;
	}
	if((s_count=json_object_array_length(s_array_obj))<1){
		goto Err2;
	}	
	if(read_str_from_file(&d_str,d_path,"r")<0){
		my_popen("cp -f %s %s",s_path,d_path);
		goto Err2;
	}else{
		d_obj=json_tokener_parse(d_str);
		if(is_error(d_obj)){
			goto Err2;
		}
		d_array_obj=json_object_object_get(d_obj,"musiclist");
		if(is_error(d_array_obj)){
			goto Err3;	
		}
		if((d_count=json_object_array_length(d_array_obj))>0){
			t_array_obj=json_object_new_array();
			if(is_error(t_array_obj)){
				goto Err3;
			}
			if(idx==-1){
				idx=d_count;
			}
			for(i=0;i<idx;i++){
				struct json_object *d_idx_obj=NULL; 
				d_idx_obj=json_object_array_get_idx(d_array_obj,i);
				if(is_error(d_idx_obj)){
					goto Err4;
				}
				json_object_array_add(t_array_obj,json_object_get(d_idx_obj));		
			}
			for(i=0;i<s_count;i++){
				struct json_object *s_idx_obj=NULL;	
				s_idx_obj=json_object_array_get_idx(s_array_obj,i);
				if(is_error(s_idx_obj)){
					goto Err4;
				}
				json_object_array_add(t_array_obj,json_object_get(s_idx_obj));		
			}
			for(i=idx;i<d_count;i++){
				struct json_object *d_idx_obj=NULL; 
				d_idx_obj=json_object_array_get_idx(d_array_obj,i);
				if(is_error(d_idx_obj)){
					goto Err4;
				}
				json_object_array_add(t_array_obj,json_object_get(d_idx_obj));		
			}
			my_jso_put(d_array_obj);
			json_object_object_add(d_obj,"musiclist",t_array_obj);
			json_object_object_add(d_obj,"num", json_object_new_int(d_count + s_count));
			dd_str=json_object_get_string(d_obj);
			if(!dd_str){
				goto Err4;
			}
			if(write_str_to_file(d_path,dd_str,"w")<0){
				goto Err4;
			}
			fail=0;
		}	
	}
Err4:
	if(fail)my_jso_put(t_array_obj);		
Err3:
	my_jso_put(d_obj);
Err2:
	my_jso_put(s_obj);	
Err1:
	FREE(s_str);
Err0:
	if(fail)return -1;
	return 0;
}





#endif
void convert_to_cc_jso_unit(struct json_object *d_obj,char *d_name,
	struct json_object *s_obj,char *s_name)
{
	struct json_object  *str_obj=NULL;;
	char *s_str=NULL;
	if(s_name&&s_name[0]){
		if(!cc_jso_get_str(&s_str,s_obj,s_name)){
			if(s_str&&s_str[0]){
				str_obj=json_object_new_string(s_str);
				if(!is_error(str_obj)){
					json_object_object_add(d_obj,d_name,str_obj);
					return;
				}			
			}
		}

	}
	json_object_object_add(d_obj,d_name, NULL);
}
int cc_jso_get_str(char **p_str,struct json_object *obj,char *name){
	char *attr=NULL;
	struct json_object *attr_obj=NULL;
	attr_obj=json_object_object_get(obj,name);
	if(is_error(attr_obj)){
		return -1;
	}
	if(my_jso_get_string(attr,attr_obj)<0){
		return -1;
	}
	*p_str=attr;
	return 0;
}
int build_cc_jso(struct json_object *d_obj,struct json_object *s_obj,match_grup_t *m_grup)
{
	if(is_error(s_obj)){
		return -1;
	}
	convert_to_cc_jso_unit(d_obj,"content_url",	s_obj,m_grup->m_content_url );
	convert_to_cc_jso_unit(d_obj,"collect",		s_obj,m_grup->m_col 		);
	convert_to_cc_jso_unit(d_obj,"title",		s_obj,m_grup->m_title		);
	convert_to_cc_jso_unit(d_obj,"artist", 		s_obj,m_grup->m_artist		);
	convert_to_cc_jso_unit(d_obj,"album",		s_obj,m_grup->m_album		);
	convert_to_cc_jso_unit(d_obj,"cover_url",	s_obj,m_grup->m_cover_url	);
	convert_to_cc_jso_unit(d_obj,"token",		s_obj,m_grup->m_token		);
	return 0;
}

/*
int cc_jso_get_m3u(char *m3u_path,char *json_path)
{
	int count=0,fail=1,i=0;
	char *json_str=NULL;
	struct json_object *obj=NULL,*array_obj=NULL,*idx_obj=NULL;
	if(check_new_msg_come_flag()){
		err("");
		goto Err0;
	}
	if(read_str_from_file(&json_str,json_path,"r")<0){
		err("");
		goto Err0;
	}
	obj=json_tokener_parse(json_str);
	if(is_error(obj)){
		err("");
		goto Err1;
	}
	array_obj=json_object_object_get(obj,"musiclist");
	if(is_error(array_obj)){
		err("");
		goto Err2;
	}
	if(!(count=json_object_array_length(array_obj))){
		my_popen("rm -f %s",m3u_path);
	}
	for(i=0;i<count;i++){
		char *content_url_str=NULL;
		idx_obj=json_object_array_get_idx(array_obj,i);
		if(is_error(idx_obj)){
			err("");
			goto Err2;
		}
		if(cc_jso_get_str(&content_url_str,idx_obj,"content_url")<0){
			err("");
			goto Err2;
		}
		if(!content_url_str){
			err("");
			goto Err2;
		}
		my_popen("echo %s >> %s",content_url_str,m3u_path);
	}
	fail=0;
Err2:
	my_jso_put(obj);
Err1:
	FREE(json_str);
Err0:
	if(fail)return -1;
	return 0;			
}


*/

