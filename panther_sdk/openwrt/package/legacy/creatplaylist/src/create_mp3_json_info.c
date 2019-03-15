#include <stdio.h>  
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <json-c/json.h>
#include "getMP3Info.h"
#include "debugprintf.h"
#include "playlist_info_json.h"
#include "getMusicInfo.h"
#include "getM4AInfo.h"
#include "getAPEInfo.h"
#include "getFLACInfo.h"
#include <wdk/cdb.h>
#define FILE_MP3        1
#define FILE_M4A        2
#define FILE_FLAC       3
#define FILE_APE        4
#define FILE_OTHER      -1

int del_str_line_info(char *str)
{
	char ch;
    while('\n' != *str && *str)
    {
    		++str;
    }
    *str = '\0';
    return 0;
}

int get_file_line(char *file_path)
{
    FILE * fp = NULL; 
    int c, lc=0; 
    int line = 0; 
    fp = fopen(file_path, "r");  
    if(fp == NULL)
    {
        PRINTF("file = %s is not exit \n",file_path);
        return line;
    }	
    if(((fgetc(fp)) == EOF))
    {		
        return line;
    }
     while((c = fgetc(fp)) != EOF) 
    {
        if(c == '\n') 
		line ++; 
        lc = c;
    }
    fclose(fp);

    return line;
}

void get_extension(const char *file_name,char *extension)  
{  
    int i=0,length; 
    //char *extension; 
    length=strlen(file_name); 
    char file_path[length+1];
    
    strcpy(file_path,file_name); 
    for(i=0;i<length;i++) 
    {     	
        if(file_path[i]=='.') 
        { 
            break; 
        } 
    }  
    if(i<length)
    {
        strcpy(extension,file_path+i+1); 
    }
    else  
        strcpy(extension,"\0");  
} 

char *find_file_name(const char *name)
{
    char *name_start = NULL;
    char *name_end = NULL;
    int sep = '/';
   
    if (NULL == name) 
    {
        return NULL;
    }
    
    name_start = strrchr(name, sep);

    return (NULL == name_start)?name:(name_start + 1);
 }

char *remove_suffix(char *file) 
{
    char *last_dot = strrchr(file, '.');
    
    if (last_dot != NULL && strrchr(file, '/') < last_dot)
        *last_dot = '\0';
    return file;
  }
void show_music_info(WIGMI_pMusicInformation info)
{    
	if(NULL!=info->pAlbum)
    {
        PRINTF("info->pAlbum   = [%s]\n",info->pAlbum );
    }
	if(NULL!=info->pArtist)
    {
        PRINTF("info->pArtist   = [%s]\n",info->pArtist );
    }
    if(NULL!=info->pTitle)
    {
        PRINTF("info->pTitle	= [%s]\n",info->pTitle );
    }
}


int read_m3u_create_WARTI_pMusicList(char *file_path)
{
	
    FILE *fp;
    char buf[256], *line, *pjsontostring,*outplaylistinfo,*file_name;
    char suffix[10];
    int ret = 1,i=0,j=0;
    int flag_a,flag_b,flag_c,flag_d,flag_e,flag_f;
    struct json_object *creatobject;
    struct json_object *object;
    struct json_object *array;

    object = json_object_new_object();
    json_object_object_add(object,"index", json_object_new_int(1));	
    array = json_object_new_array();

    WARTI_pMusicList lp;
    WARTI_pMusic mp;
    char m3u_path[64];

    strcpy (m3u_path,file_path);
    strcat(m3u_path,".m3u");

    int Num = get_file_line(m3u_path);

    WIGMI_pMusicInformation   pmp3information = NULL;
    unsigned char *tmp_mp3 = (unsigned char *)malloc(128);
    if(NULL == tmp_mp3)
    {		
        PRINTF("malloc failed!\n");	
        exit(1);   
    }

    lp = (WARTI_pMusicList)calloc(1,sizeof(WARTI_MusicList)); //yes
    if(NULL == lp)
    {		
        PRINTF("malloc failed!\n");	
        exit(1);   
    }
    memset(lp,0,sizeof(WARTI_MusicList));
    lp->Num = Num;
    lp->pMusicList = (WARTI_pMusic)calloc(1,(Num)*sizeof(WARTI_pMusic));//yes
    if(NULL == lp->pMusicList)
    {		
        PRINTF("malloc failed!\n");	
        exit(1);   
    }
    memset(lp->pMusicList,0,(Num)*sizeof(WARTI_pMusic));
    json_object_object_add(object,"num",json_object_new_int(lp->Num));

    fp = fopen(m3u_path, "rb");
    if( fp ) 
    {
        for(i=0;i<Num;i++)
        {
            line = fgets(buf, sizeof(buf), fp);

            if(line!=NULL)
            {			
                flag_a=memcmp(line,"null",strlen("null"));
                flag_b=memcmp(line,"(null)",strlen("(null)"));
        	}
            else
            {
            	continue;
            }
             
            if(!flag_a||!flag_b)
            {
                continue;
            }

            if(strlen(line)!=1 && strlen(line)!=0 )
            {  
                del_str_line_info(line);
                get_extension(line,suffix);

                lp->pMusicList[i] =(WARTI_pMusic)calloc(1, sizeof(WARTI_Music));
                if(NULL == lp->pMusicList[i])
                {		
                    PRINTF("malloc failed!\n");	
                    exit(1);   
                }
                
                memset(lp->pMusicList[i],0,sizeof(WARTI_Music));
                memset(tmp_mp3,0,128);
                sprintf(tmp_mp3,"/media/%s",line);
                if(access(tmp_mp3,0))
                {
                    continue;
                }


                pmp3information = WIFIAudio_GetMP3Information_MP3PathTopMP3Information(tmp_mp3);
                if(pmp3information ==NULL)
                {   
                    PRINTF("Can not parse this mp3  file\n");
                    goto   FLAG;
                }
                else if((WIGMI_pMusicInformation)0x1000 != pmp3information)
                {   
                    show_music_info(pmp3information);
                    goto  show;
                }

            	     
                pmp3information  = WIFIAudio_GetM4AInformation_M4APathTopM4AInformation(tmp_mp3);
                if(pmp3information==NULL)
            	{
                    PRINTF("Can not parse this m4a file\n");
                    goto   FLAG;
            	}
                else if((WIGMI_pMusicInformation)0x1000 != pmp3information )
                {
                    show_music_info(pmp3information );
                    goto show;
                }
                
                pmp3information = WIFIAudio_GetFLACInformation_FLACPathTopFLACInformation(tmp_mp3);
                if(pmp3information ==NULL)
                {
                    PRINTF("Can not parse this flac  file\n");
                    goto   FLAG;
                }
                else if((WIGMI_pMusicInformation)0x1000 != pmp3information )
                {
                    show_music_info(pmp3information );
                    goto show;
                }
                
                pmp3information = WIFIAudio_GetAPEInformation_APEPathTopAPEInformation(tmp_mp3);
                if(pmp3information ==NULL)
                {
                    PRINTF("Can not parse this APE file\n");
                    goto   FLAG;
                }
                else if((WIGMI_pMusicInformation)0x1000 != pmp3information )
                {
                    show_music_info(pmp3information );
                    goto show;
                }
               goto  FLAG;
            	   
show:      
                if(!(pmp3information ->pTitle == NULL))
                {

                    flag_a=memcmp(pmp3information->pTitle,"unknown",strlen("unknown"));
                    flag_b=memcmp(pmp3information->pTitle,"null",strlen("null"));
                    flag_d=memcmp(pmp3information ->pTitle,"(null)",strlen("(null)"));
                    flag_c=strcmp(pmp3information ->pTitle,"Undefined");
                    flag_e=strcmp(pmp3information ->pTitle,"");
                    flag_f=strcmp(pmp3information ->pTitle," ");

                    if(!flag_a||!flag_b||!flag_c||!flag_d||!flag_e||!flag_f)
                    {
                        (lp->pMusicList)[i]->pContentURL = strdup(line);
                        file_name =find_file_name(line);
                        file_name = remove_suffix(file_name);
                        (lp->pMusicList)[i]->pTitle =strdup(file_name);
                        
                        PRINTF("(lp->pMusicList)[%d]->pTitle ==%s\n",i,(lp->pMusicList)[i]->pTitle);
                        PRINTF("(lp->pMusicList)[%d]->pContentURL ==%s\n",i,(lp->pMusicList)[i]->pContentURL);
                    }
                    else
                    {
                        (lp->pMusicList)[i]->pTitle = pmp3information ->pTitle;
                        
                        if(!(pmp3information ->pArtist == NULL))
                            (lp->pMusicList)[i]->pArtist = pmp3information ->pArtist;

                        if(!(pmp3information ->pAlbum == NULL))
                            (lp->pMusicList)[i]->pAlbum = pmp3information ->pAlbum;

                        (lp->pMusicList)[i]->pContentURL = strdup(line);

                        PRINTF("(lp->pMusicList)[%d]->pTitle == %s\n",i,(lp->pMusicList)[i]->pTitle);
                        PRINTF("(lp->pMusicList)[%d]->pContentURL ==%s\n",i,(lp->pMusicList)[i]->pContentURL);
                    }

                }
                else
                {
FLAG:				
                    (lp->pMusicList)[i]->pContentURL = strdup(line);
                    file_name =find_file_name(line);
                    file_name = remove_suffix(file_name);
                    (lp->pMusicList)[i]->pTitle =strdup(file_name);

                    PRINTF("(lp->pMusicList)[%d]->pTitle ==%s\n",i,(lp->pMusicList)[i]->pTitle);
                    PRINTF("(lp->pMusicList)[%d]->pContentURL ==%s\n",i,(lp->pMusicList)[i]->pContentURL);
                }
            }
            json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((lp->pMusicList)[i]));
        }

        fclose(fp);
    }
	 
	free(tmp_mp3);

    json_object_object_add(object,"musiclist",array);  
    if(object == NULL)
    {
        PRINTF("creatobject===NULL\n");
    }

    pjsontostring =  json_object_to_json_string(object);
    SHOWDEBUG("pjsontostring = %p\n",pjsontostring);
    char tmp_json[64];
    int len_str = 0;

    strcpy (tmp_json,file_path);
    strcat(tmp_json,"InfoJson.txt");

    len_str = strlen(pjsontostring);
    if(NULL != (fp = fopen(tmp_json, "w")))
    {
        ret = fwrite(pjsontostring, sizeof(char),  len_str+ 1, fp);
        fclose(fp);
        fp = NULL;
    }

    if(NULL != (fp = fopen("/usr/script/playlists/currentPlaylistInfoJson.txt", "w")))
    {
        ret = fwrite(pjsontostring, sizeof(char), len_str + 1, fp);
        fclose(fp);
        fp = NULL;
    }
    json_object_put(object);

    if(NULL != lp->pMusicList)
    {
        for(i = 0; i<(lp->Num); i++)
        {
            if(NULL != (lp->pMusicList)[i])
            {
                json_pMusicFree((lp->pMusicList)[i]);
            }
        }

        free(lp->pMusicList);
        lp->pMusicList = NULL;
    }

    free(lp);
    lp = NULL;

    return 0;
}

//是本地文件还是APP推送的链接
int local_file_or_http(char *path)
{
    if('s' == path[0] || 'm' == path[0])
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

//根据文件名，获取后缀以粗略的判断是什么类型的文件
int get_file_type(char *path)
{
    int type = -1;
    char * dot = NULL;

    dot = strrchr(path,'.');
    
    if(dot != NULL)
    {
        if(0 == strncasecmp(dot,".mp3",4)) 
            return FILE_MP3;
        if(0 == strncasecmp(dot,".m4a",4)) 
            return FILE_M4A;
        if(0 == strncasecmp(dot,".flac",5)) 
            return FILE_FLAC;
        if(0 == strncasecmp(dot,".ape",4)) 
            return FILE_APE;    
    }
    
    return type;
}
//着重关注title的信息，若为空，则用歌曲名称替换掉
void check_tags(WIGMI_pMusicInformation pmp3information,char *path)
{
    if(pmp3information ->pTitle == NULL)
    {
        pmp3information ->pTitle = strdup(find_file_name(path));
    }
    else
    {
        if(0 == strcmp(pmp3information ->pTitle,"unknown") || 0 == strcmp(pmp3information ->pTitle,"null")
         || 0 == strcmp(pmp3information ->pTitle," ") || 0 == strcmp(pmp3information ->pTitle,"(null)"))
        {
            free(pmp3information ->pTitle);
            pmp3information ->pTitle = strdup(find_file_name(path));
        }
    }
   
    if(pmp3information ->pArtist == NULL)
    {
        pmp3information ->pArtist = strdup(" ");
    }
    
    if(pmp3information ->pAlbum == NULL)
    {
        pmp3information ->pAlbum = strdup(" ");
    }
}

void set_to_default_info(char *path)
{
/*
	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_artist", " ");
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title", find_file_name(path));
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_album"," ");
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", path);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_coverURL", " ");
*/
    cdb_set("$current_artist","");
    cdb_set("$current_title",find_file_name(path));
	cdb_set("$current_album","");
	cdb_set("$current_contentURL",path);
	cdb_set("$current_coverURL","");
}

int parse_local_file(char *path)
{
    WIGMI_pMusicInformation   pmp3information = NULL;
    int type = -1;
    char *filename = NULL;
    int len = strlen(path);

    filename = (char *)calloc(1,len+8);
    strcpy(filename,"/media/");
    strcat(filename,path);

    type = get_file_type(filename);
    
    if(FILE_MP3 == type)
    {
        pmp3information = WIFIAudio_GetMP3Information_MP3PathTopMP3Information(filename);
        if(pmp3information == NULL)
        {   
            SHOWERROR("Can not parse this mp3  file\n");
            goto   FOUND_NOTHING;
        }
    }
    else if(FILE_M4A == type)
    {
        pmp3information  = WIFIAudio_GetM4AInformation_M4APathTopM4AInformation(filename);
        if(pmp3information == NULL)
        {
            PRINTF("Can not parse this m4a file\n");
            goto   FOUND_NOTHING;
        }
    }
    else if(FILE_FLAC == type)
    {
        pmp3information = WIFIAudio_GetFLACInformation_FLACPathTopFLACInformation(filename);
        if(pmp3information ==NULL)
        {
            PRINTF("Can not parse this flac  file\n");
            goto   FOUND_NOTHING;
        }
    }
    else if(FILE_APE == type)
    {
        pmp3information = WIFIAudio_GetAPEInformation_APEPathTopAPEInformation(filename);
        if(pmp3information ==NULL)
        {
            PRINTF("Can not parse this APE file\n");
            goto   FOUND_NOTHING;
        }
    }
    else if(type < 0)   //other types , ogg,wav,wma etc.
    {
        goto   FOUND_NOTHING;
    }

    check_tags(pmp3information,path);
	/*
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_artist", pmp3information->pArtist);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title", pmp3information->pTitle);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_album", pmp3information->pAlbum);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", path);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_coverURL", " ");
    */
    cdb_set("$current_artist",pmp3information->pArtist);
    cdb_set("$current_title",pmp3information->pTitle);
	cdb_set("$current_album",pmp3information->pAlbum);
	cdb_set("$current_contentURL",path);
	cdb_set("$current_coverURL","");
    WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
    return 0;
        
FOUND_NOTHING:
    set_to_default_info(path);

}

void get_every_item_from_json(struct json_object *obj)
{
    WIGMI_pMusicInformation   pmp3information = NULL;
    struct json_object *tmpobject = NULL;

    pmp3information = (WIGMI_pMusicInformation)calloc(1, sizeof(WIGMI_MusicInformation));
    if(NULL == pmp3information)
    {
        fprintf(stderr,"Can not calloc\n");
        return ;
    }
    tmpobject = json_object_object_get(obj,"artist");
    if(NULL != tmpobject)
    {
        pmp3information->pArtist = strdup(json_object_get_string(tmpobject));
    }    
    tmpobject = json_object_object_get(obj,"title");
    if(NULL != tmpobject)
    {
        pmp3information->pTitle = strdup(json_object_get_string(tmpobject));
    }    
    tmpobject = json_object_object_get(obj,"album");
    if(NULL != tmpobject)
    {
        pmp3information->pAlbum = strdup(json_object_get_string(tmpobject));
    }
/*
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_artist", pmp3information->pArtist);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title", pmp3information->pTitle);
    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_album", pmp3information->pAlbum);
 */
    cdb_set("$current_artist",pmp3information->pArtist);
    cdb_set("$current_title",pmp3information->pTitle);
	cdb_set("$current_album",pmp3information->pAlbum);
    tmpobject = json_object_object_get(obj,"content_url");
    //WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", json_object_get_string(tmpobject));
     cdb_set("$current_contentURL",json_object_get_string(tmpobject));
    tmpobject = json_object_object_get(obj,"cover_url");
    //WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_coverURL", json_object_get_string(tmpobject));
    cdb_set("$current_coverURL",json_object_get_string(tmpobject));
    WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
   
}

void parse_json_from_string(char *str,char *path)
{
    struct json_object *getobject;
    struct json_object *object;
    struct json_object *tmpobject;
    struct json_object *obj; 
    int num = 0,i = 0;

    object = json_tokener_parse(str);
    getobject = json_object_object_get(object,"musiclist");
    if(NULL == getobject)
    {
        fprintf(stderr,"Not a standard json from APP\n");
        goto JSON_ERROR;
    }
    num = json_object_array_length(getobject);
    for(i = 0; i<num; i++)
    {
        obj = json_object_array_get_idx(getobject,i);
        tmpobject = json_object_object_get(obj,"content_url");
        if(0 == strcmp(path,json_object_get_string(tmpobject)))
        {
            get_every_item_from_json(obj);
            break;
        }
        
    }
    
JSON_ERROR:    
    json_object_put(object);
}

int parse_http_file_from_json(char *path)
{
    struct stat filestat;
    char *pjsonstring = NULL;
    int i = 0,ret;
    FILE *fp = NULL;

    char *httpjson = "/usr/script/playlists/httpPlayListInfoJson.txt";
    fprintf(stderr,"aaaaaaaaaaaaaaaaaaa\n");

    fp = fopen(httpjson, "r");
    if(NULL == fp)
    {
        goto OPEN_ERROR;
    }
    stat(httpjson, &filestat);
    pjsonstring = (char *)calloc(1,filestat.st_size + 1); 
    if(NULL == pjsonstring)
    {
        set_to_default_info(path);
        return 0;
    }
    ret = fread(pjsonstring, 1, filestat.st_size, fp);
    if(ret != filestat.st_size)
    {
       goto  READ_ERROR;
    }
    fclose(fp);
    fprintf(stderr,"bbbbbbbbbbbbbbbbbbb\n");
    parse_json_from_string(pjsonstring,path);
    free(pjsonstring);

    return 0;

READ_ERROR:
    free(pjsonstring);
    
CALLOC_ERROR:   
    fclose(fp);
    
OPEN_ERROR:
    set_to_default_info(path);

    return 1;
}

//将文件中的tag信息解析出来写到配置文件xzxwifiaudio中
int parse_current_song_tag(char *path)
{
    int ret = 0;

    fprintf(stderr,"\033[32mpath = [%s]\033[0m\n",path);

    ret = local_file_or_http(path);
    if(ret < 0)
    {
        parse_http_file_from_json(path);
    }
    else
    {   
        parse_local_file(path);
    }
    
    return 0;
}


