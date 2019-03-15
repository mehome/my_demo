/**
 * @file mtdm_misc.c system help function
 *
 * @author  Frank Wang
 * @date    2010-10-06
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <shutils.h>
#include <errno.h>
#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_types.h"


/**
 * @define LOG_IN_DEVICE
 * @brief config of log, default as disable
 */
#define LOG_IN_DEVICE   0

/**
 * @define BUFFER_SIZE
 * @brief size of buffer, default as 10240 bytes
 */
#define BUFFER_SIZE 10240

/**
 * @define DEBUGKILL
 * @brief config of debug, default as disable
 */
#define DEBUGKILL       0
/**
 * @define DEBUGSYSTEM
 * @brief config of debug, default as disable
 */
#define DEBUGSYSTEM     0

#define DEBUG_INFO(fmt, args...)   do { printf("[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
#define ROUNDUP(x, y) ((((x)+(y)-1)/(y))*(y))


/**
 * @define sysprintf
 * @brief print message
 * @param fmt message format
 * @param args... args
 */
#define sysprintf(fmt, args...) do { \
            FILE *cfp = fopen(CONSOLE, "a"); \
            if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)

/**
 *  system command with parameters
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return number of command length
 *
 */
int __system(const char *file, const int line, const char *func, const char *fmt, ...)
{
    va_list args;
    int ret;
    char buf[MAX_COMMAND_LEN];

    va_start(args, fmt);
    vsprintf(buf, fmt,args);
    va_end(args);

#if LOG_IN_DEVICE
    mtdmDebug("sys cmd:%s\n",buf);
#endif

#if DEBUGSYSTEM
    int pid = getpid();
    int ptid= my_gettid();
    sysprintf("\033[1m\033[%d;%dm[%-5d %-5d %-20s:%5d %16s()]:::\033[0m[S]%s\n",40, 30+(ptid%8)+1, pid, ptid, file,line,func,buf);
    ret = system(buf);
    sysprintf("\033[1m\033[%d;%dm[%-5d %-5d %-20s:%5d %16s()]:::\033[0m[E]%s\n",40, 30+(ptid%8)+1, pid, ptid, file,line,func,buf);
#else
    ret = system(buf);
#endif

    return ret;
}

/**
 *  kill command w/ debug message
 *
 *  @param pid
 *  @param sig
 *
 *  @return status code
 *
 */
int __kill(const char *file, const int line, const char *func, int pid, int sig)
{
    int ret;

    if ((pid == -1) || (pid ==0) || (pid ==1)) {
        stdprintf("kill %d\n", pid);
        ret = -1;
    }
    else {
#if DEBUGKILL
        unsigned int runpid = getpid();
        sysprintf("\033[1m\033[%d;%dm[%5d %-20s:%5d %16s()]:::\033[0m[S]kill %d %d\n",40, 30+(runpid%8)+1, runpid, file,line,func,pid,sig);
#endif
        ret = kill(pid,sig);
#if DEBUGKILL
        sysprintf("\033[1m\033[%d;%dm[%5d %-20s:%5d %16s()]:::\033[0m[E]kill %d %d[%d]\n",40, 30+(runpid%8)+1, runpid, file,line,func,pid,sig,ret);
#endif
    }

    return ret;
}

/**
 *  print infomation message to stdout or console
 *
 *  @param *cfp fp
 *  @param *file file name
 *  @param *line line number
 *  @param *function function name
 *
 *  @return none.
 *
 */
#if DEBUGMSG
#include <time.h>

void __stdprintf_info__(FILE *cfp, const char *file, const int line, const char *function)
{
#if (DEBUGSLEEP || DEBUGWITHTIME)
    time_t now = time(NULL);
#endif
    fprintf(cfp,"["
#if (DEBUGSLEEP || DEBUGWITHTIME)
        "%4u "
#endif
        "%-20s:%5d %16s()]:: ",
#if (DEBUGSLEEP || DEBUGWITHTIME)
        (u32t )(now & 0x0fff), 
#endif
        file , line, function);
}
#endif

static int initialized;

/**
 *  get a random number
 *
 *  @return random number
 *
 */
unsigned int get_random()
{
    if (!initialized) {
        int fd;
        unsigned int seed;

        fd = open("/dev/urandom", 0);

        if (fd < 0 || safe_read(fd, &seed, sizeof(seed)) < 0) {
            seed = (unsigned int)time(0);
        }

        if (fd >= 0) {
            close(fd);
        }

        srand(seed);
        initialized++;
    }
    return rand();
}

/**
 *  execute system command
 *  @param *fmt... format of command.
 *  @return the number of command characters.
 *
 */
int _execl(const char *fmt, ...)
{
    va_list args;
    int i;
    char buf[MAX_COMMAND_LEN];

    va_start(args, fmt);
    i = vsprintf(buf, fmt,args);
    va_end(args);

#if 1
    mtdmDebug("sys cmd:%s\n",buf);
#endif

    if (vfork()==0) {
        execl("/bin/sh", "sh", "-c", buf, (char *)0);
        exit(0);
    }

    wait(NULL);

    return i;
}

/**
 *  file exist or not
 *  @param *file_name file name
 *  @return 1 for exist, 0 for no.
 *
 */
int _isFileExist(char *file_name)
{
    struct stat status;

    if ( stat(file_name, &status) < 0)
        return 0;

    return 1;
}

#include <sys/syscall.h>

pid_t my_gettid(void)
{
    return syscall(SYS_gettid);
}

int my_atoi(char *str)
{
    if (str == NULL) return 0;

    char *pEnd;

    return (int )strtol(str, &pEnd, 10);
}

/*
 *  translate string mac to value array
 *  @val(dst)
 *  @mac(src)
 *  @return
 */
int my_mac2val(u8 *val, s8 *mac)
{
    return sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
        &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
}

/*
 *  translate value array to string mac
 *  @mac(dst)
 *  @val(src)
 *  @return
 */
int my_val2mac(s8 *mac, u8 *val)
{
    return sprintf(mac, "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", 
        val[0], val[1], val[2], val[3], val[4], val[5]);
}

int read_pid_file(char *file)
{
    FILE *pidfile;
    char pidnumber[16];
    int pid = UNKNOWN;

    if ((pidfile = fopen(file, "r")) != NULL) {
        if (fscanf(pidfile, "%s", pidnumber) == 1) {
            if (isdigit(pidnumber[0])) {
                pid = my_atoi(pidnumber); 
            }
        }
        fclose(pidfile);
    }

    return pid;
}

/*
 * free_mode: 0~3, 
 * free_time: 0~(LONG_MAX-1), 
 * threshold(KB): 0: always act, >0: act when lower than.
 */
int free_caches(const int free_mode, const int free_time, const unsigned int threshold)
{
    FILE *fp = NULL;
	char memdata[256] = {0}, mode[8] = {0};
    unsigned int memfree = 0;

	if( free_time < 0)
		return -1;

	if( free_mode < FREE_MEM_NONE || free_mode > FREE_MEM_ALL )
        return -1;

	if(threshold > 0) {
		if((fp = fopen("/proc/meminfo", "r")) != NULL) {
			while(fgets(memdata, sizeof(memdata), fp) != NULL) {
				if(strstr(memdata, "MemFree") != NULL) {
                    sscanf(memdata, "MemFree: %d kB", &memfree);
                    break;
                }
            }
            fclose(fp);
			if(memfree > threshold){
				printf("%s: memfree > threshold.\n", __FUNCTION__);
                return 0;
            }
        }
    }

	printf("%s: Start syncing...\n", __FUNCTION__);
    sync();

	snprintf(mode, sizeof(mode), "%d" , free_mode);
	f_write_string("/proc/sys/vm/drop_caches", mode, 0, 0);
	if(free_time > 0){
		sleep(free_time);
		printf("%s: Finish.\n", __FUNCTION__);
        f_write_string("/proc/sys/vm/drop_caches", "0", 0, 0);
    }

    return 0;
}

int is_directmode(void)
{
    int omicfg_config_enable = cdb_get_int("$omicfg_config_enable", 0);
    int omi_config_mode = cdb_get_int("$omi_config_mode", 0);
    if ((omicfg_config_enable == 0) && (omi_config_mode == OMNICFG_MODE_DIRECT)) {
        return 1;
    }
    else {
        return 0;
    }
}

int save_diskinfo(void) 
{
    int 	ret = -1;
    DIR     *dir = NULL;
    struct dirent   *next = NULL;

    unlink("/var/diskinfo.txt");
    if ((dir = opendir("/media")) == NULL) {
            perror("Cannot open /media");
            return -1;
     }

    while ((next = readdir(dir)) != NULL) {
						
		char buf[270];
					 
            if (!strcmp(next->d_name, ".") || !strcmp(next->d_name, "..") )
                    continue;
                    
            if ( (next->d_name[0] != 's' ) && (next->d_name[0] != 'm') )
                    continue;
                    
		memset(buf, 0 , sizeof(buf));
		snprintf(buf, sizeof(buf), "/media/%s\n", next->d_name );
            f_write("/var/diskinfo.txt", buf, strlen(buf), FW_APPEND , 0 );
    }

    closedir(dir);
    return ret;
}


int delete_file(char *filename)
{
    char cmd[64];
    
    memset(cmd,0,64);
    if(0 == access(filename,F_OK))
    {
        sprintf(cmd,"rm %s",filename);
        exec_cmd(cmd);
        usleep(100*1000);
    }

    return 0;
}

int hangshu(char file[])
{

 int ch=0;
 int n=0;
 FILE *fp;
 fp=fopen(file,"r");
 if(fp==NULL)
      return -1;
 while((ch = fgetc(fp)) != EOF) 
         {  
             if(ch == '\n')  
             {  
                 n++;  
             } 
         }  

 fclose(fp); 
 return n; 

}


//check whether the firmware filename is newer than current version
//return value:
// 1:is newer
// 0:same or older
static int check_filename(char *filename)
{
	int ret = 0;
	char *file_version = NULL;
	char *file_name_version = NULL;
	char current_version[256] ={0};
	char buf[15] = {0};
	
	cdb_get_str("$sw_build", current_version, sizeof(current_version), NULL); 
	if(current_version)
	{
		if(strlen(current_version) != 14)
		{
			printf("Get current version error\n");
			ret = 0;
			goto ERROR;
		}
	}
	sysprintf("check_filename current_version = [%s]\n",current_version);
	file_name_version = strrchr(filename,'_');
	if(file_name_version)
	{
		strncpy(buf,file_name_version+1,14);
		
		if(strlen(buf) != 14)
		{
			printf("file_name error\n");
			ret = 0;
			goto ERROR;

		}
	}
	sysprintf("check_filename file_name_version = [%s]\n",buf);

	if(strcmp(current_version,buf) < 0)
	{
		ret = 1;
	}

	
ERROR:	
	//free(current_version);
	
	return ret;
}

//check whether the firmware is newer than current version
//return value:
// 1:is newer
// 0:same or older

static int check_file(char *path)
{
	FILE *fp_update = NULL;
    FILE *fp_current = NULL;

    unsigned int ver_update = 0;
    unsigned int ver_current = 0;
    int ret = 0;

    sysprintf("check_file path = [%s]\n",path);
    if(NULL != (fp_update = fopen(path,"rb")))
    {
        fseek(fp_update, 4, SEEK_SET);
        fread(&ver_update, sizeof(unsigned int), 1, fp_update);
        fclose(fp_update);
	}

    if(NULL != (fp_current = fopen("/dev/mtdblock2", "rb")))
    {
        fseek(fp_current, 4, SEEK_SET);
        fread(&ver_current, sizeof(unsigned int), 1, fp_current);
        fclose(fp_current);
    }


	sysprintf("[%d]\n[%d]\n",ver_update,ver_current);
    

	if(ver_update > ver_current )
	{
		ret = 1;
	}


	return ret;
}

 
/*
@params:path:where the firmware exist
@reryen value:
@ 1: upgrade
@ 0: no need to upgrade
*/
 int search_and_check_firmware(char *path)
 {
 	int ret = 0;
	DIR *d = NULL;
	struct dirent *dir = NULL;
	char file_path[128] = {0};
	char cmd[64] = {0};

	if(0 == strlen(path))
	{
		printf("error......111111111\n");
		return 0;

	}
	cslog("path = [%s].......\n",path);
	
	d = opendir(path);
	if (!d) 
	{
		printf("error:%s\n",strerror(errno));
		return 0;
	}
	while((dir = readdir(d)) != NULL) 
	{
		if (dir->d_name[0] == '.')
			continue;
		if ((dir->d_type == DT_REG) && ( strncmp(dir->d_name, "FW_panther_",11) == 0)) 
		{
			cslog("Found :%s\n",dir->d_name);
			ret = check_filename(dir->d_name);
			if( ret )
			{
				sprintf(file_path,"%s/%s",path,dir->d_name);
				ret &= check_file(file_path);
			}
			if(ret)
			{
				exec_cmd("wavplayer /tmp/res/upgrading.wav");
				sprintf(cmd,"/sbin/sysupgrade %s",file_path);
				break;
			}
		}
	}
	closedir(d);
	if(ret)
	{
		exec_cmd(cmd);
	}

	return ret;
 }

int compare_FileAndUrl(char *filepatch,char *Url)
{

   if((NULL!=Url)&&(NULL!=filepatch)){

      FILE *dp =NULL;
	  char *deline;
      char buf[512];
	  int ret=0;
	  int i;
	  int hangnum=hangshu(filepatch);
	  sysprintf("hangnum===[%d]\n",hangnum);
   	   dp= fopen(filepatch, "r+");
	    if(NULL==dp){
		   DEBUG_INFO("file no exist\n");
           return -1;
		}
            
	         for(i=0;i<hangnum;i++)
	         {
			 deline=fgets(buf,512, dp);
			 if(deline==NULL)
			 	{
			 	    fclose(dp);
			 	 	return 0;
			 	}
			 ret=strncmp(deline,Url,strlen(Url)-1);
			 if(0==ret){
			 	i++;
				fclose(dp);
			 	return i;
			 	}	      	 
			 }
			 
			 
			 fclose(dp);
			 dp= NULL;
		  
		return 0;
 }
}


/* hotplug block, called by hotplug_main */
/* hotplug block, called by hotplug_main */




/* hotplug block, called by hotplug_main */
/* hotplug block, called by hotplug_main */
/* （part-1）  为分区数，一个分区将触发hotplug_block 一次*/

int
hotplug_block(char *type, char *action, char *name )
{
	char mntdev[128] = {0};
	char mntpath[128] = {0};
	int ret = 0;
	int sys_auto_upgrade = 0;
	 int result=-1;
	 char cmd[512] = {0};
	 
	if( !action )
		return -1;

	sprintf(mntdev, "/dev/%s", name);
	sprintf(mntpath, "/media/%s", name);

    sysprintf("\033[31maction = [%s], path = [%s]\033[0m\n", action,mntpath);
	if (!strcmp(action, "add")) 
	{

	   // cslog("compile in ------------\n");
	    char tempbuff[72];
		char result_string[1024] = {0};
		char *p;
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));		
   	/*	
         //fat32 格式
        mkdir(mntpath, 0777); 
		if( mount(mntdev, mntpath , "vfat", MS_MGC_VAL, NULL ) < 0) {
			unlink(mntpath);
			rmdir(mntpath);	
		    cslog("--mount fail\n");
			return -1;
		}
   */ 
	    mkdir(mntpath, 0777);
        snprintf(cmd, sizeof(cmd), "mount %s %s >/dev/null 2>&1", mntdev, mntpath);
        ret = exec_cmd(cmd);
        if(ret != 0) {
            unlink(mntpath);
            rmdir(mntpath);
        }      
		goto exit;
	    //cslog("mount success------------\n");
    	if(name[0] == 's' )
    	{			
        	//cslog("add s ------------\n");
			sys_auto_upgrade = cdb_get_int("$sys_auto_upgrade", 0); 
			if(sys_auto_upgrade)
			{
			    cslog("upgrade in------------\n");
				ret = search_and_check_firmware(mntpath);
				if(1 == ret)
				    return ;
			}
        	
            cdb_set_int("$usb_scan_over",0);
            if(get_pid_by_name("mpdaemon") <= 0)
            {
                
                cdb_set_int("$usb_status", 1);
                cdb_set("$usb_mount_point", name);
                return 0;
            }
            eval("mpc", "clear");
            usleep(100*1000);

            delete_file("/usr/script/playlists/usbPlaylist.m3u");
            delete_file("/usr/script/playlists/usbPlaylistInfoJson.txt");

            eval("mpc", "update",name);
            usleep(200*1000);
            eval("uartdfifo.sh", "usbint");   
            usleep(200*1000);
            eval("uartdfifo.sh", "inusbmode");
            usleep(200*1000);
            ret = cdb_set_int("$usb_status", 1);
            ret = cdb_set("$usb_mount_point", name);
            sleep(1);

		    if(cdb_get_int("$vtf003_usb_tf",0)){
            cdb_set("$shutdown_before_playmode", "003");
            //char* usb_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.usb_last_contentURL");
            char usb_last_contentURL[512]={0};
			cdb_get_str("$usb_last_contentURL",usb_last_contentURL,sizeof(usb_last_contentURL),"");

			if(usb_last_contentURL!=NULL){    
			////// 因为需要识别不同U盘或sd卡，所以需要更新 m3u ////
			exec_cmd("mpc listall | grep sd > /usr/script/playlists/usbPlaylist.m3u");
			usleep(200*1000);
			result=compare_FileAndUrl("/usr/script/playlists/usbPlaylist.m3u",usb_last_contentURL);
	       // free(usb_last_contentURL);
			}
			if(result<=0){
				result=1; 
     	    }
			sprintf(tempbuff, "playlist.sh usbautoplay %d", result);
			exec_cmd(tempbuff);
		 	}else{
				 eval("playlist.sh", "usbautoplay");
			}
        }
        else if(name[0] == 'm' ) 
        {
		    sys_auto_upgrade = cdb_get_int("$sys_auto_upgrade", 0); 
			if(sys_auto_upgrade)
			{
			    cslog("upgrade in------------\n");
				ret = search_and_check_firmware(mntpath);
				if(1 == ret)
			         return ;
			}

            cdb_set_int("$tf_scan_over",0);
            if(get_pid_by_name("mpdaemon") <= 0)
            {
                cdb_set_int("$tf_status", 1);
                cdb_set("$tf_mount_point", name);
                return 0;
            }
            eval("mpc", "clear");
            usleep(100*1000);
            
            delete_file("/usr/script/playlists/tfPlaylist.m3u");
            delete_file("/usr/script/playlists/tfPlaylistInfoJson.txt");
            
            eval("mpc", "update",name);
            usleep(200*1000);
            eval("uartdfifo.sh", "tfint");
            usleep(200*1000);
            eval("uartdfifo.sh", "intfmode");
            usleep(200*1000);
            ret = cdb_set_int("$tf_status", 1);
            ret = cdb_set("$tf_mount_point", name);
            sleep(1);

			if(cdb_get_int("$vtf003_usb_tf",0)){
            cdb_set("$shutdown_before_playmode", "004");
       		//char* tf_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.sd_last_contentURL");
			char tf_last_contentURL[512]={0};
			cdb_get_str("$sd_last_contentURL",tf_last_contentURL,sizeof(tf_last_contentURL),"");
			if(tf_last_contentURL!=NULL){
			exec_cmd("mpc listall | grep mmc > /usr/script/playlists/tfPlaylist.m3u");	
			usleep(200*1000);
			result=compare_FileAndUrl("/usr/script/playlists/tfPlaylist.m3u",tf_last_contentURL);
			//free(tf_last_contentURL);
			}
			if(result<=0){ 
			 result=1; 
			} 
			sprintf(tempbuff, "playlist.sh tfautoplay %d", result);
			exec_cmd(tempbuff);
			 }
			else{
                eval("playlist.sh", "tfautoplay");
			}
            
        }
		save_diskinfo();
	} 
	else if (!strcmp(action, "remove"))
	{
		goto exit1;
	    //cslog("remove ------------\n");
        char value[10] ={0};
		int usb_status = cdb_get_int("$usb_status",0);
		int tf_status = cdb_get_int("$tf_status",0);
        //value = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
        cdb_get_str("$playmode",value,sizeof(value),"");
    	if (name[0] == 's')
    	{
    	    //cslog("remove s ------------\n");
       		cslog("usb_status = %d,tf_status = %d\n", usb_status,tf_status);
			//cdb_set_int("$usb_exist", 0);
    		if (usb_status != 0)
    		{
    			eval("uartdfifo.sh", "usbout");
    			ret = cdb_set_int("$usb_status", 0);
    			ret = cdb_set_int("$usb_scan_over",0);
    			if (strcmp(value, "003") == 0)
    			{
    				eval("mpc", "clear");
					if(!cdb_get_int("$vtf003_usb_tf",0)){    //注意：vtf003 要去掉
    				cdb_set("$playmode", "000");
					}
    			}
                ret = cdb_set("$usb_mount_point", "");
                eval("mpc", "update", name );

    			cdb_set("$usb_status", "0");
				
                delete_file("/usr/script/playlists/usbPlaylist.m3u");
                delete_file("/usr/script/playlists/usbPlaylistInfoJson.txt");
				delete_file("/usr/script/playlists/currentPlaylistInfoJson.txt");	
			}
			if(cdb_get_int("$vtf003_usb_tf",0)){
				if((tf_status!=0)&&(strcmp(value,"003")==0)){
					cdb_set("$shutdown_before_playmode", "004");
                 	//      如果tfplaylist.m3u   不存在， 则创建             //
					if(!f_exists("/usr/script/playlists/tfPlaylist.m3u")){
						char buf[32];
						char value[16];
			            cdb_set_int("$tf_scan_over",0);
						exec_cmd("mpc clear");
						memset(value,0,16);
						cdb_get_str("$tf_mount_point",value,16,"");
						sprintf(buf,"mpc update %s",value);
						exec_cmd(buf);
						
						usleep(200*1000);
						exec_cmd("mpc listall | grep mmc > /usr/script/playlists/tfPlaylist.m3u");
			            usleep(200*1000);
					}

	                char sd_last_contentURL[512]={0};
	                //sd_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.sd_last_contentURL");
					cdb_get_str("$sd_last_contentURL",sd_last_contentURL,sizeof(sd_last_contentURL),"");
					if(sd_last_contentURL!=NULL ){
					    result=compare_FileAndUrl("/usr/script/playlists/tfPlaylist.m3u",sd_last_contentURL);
					    // free(sd_last_contentURL);
		        	}
					if(result<=0){
		                result=1;				
					}
		            //cslog("result====play usb num================[%d]\n",result);
		            exec_cmd("mpc clear > /dev/null");
		            usleep(200*1000);
		            DEBUG_INFO(" system creatPlayList tfmode  before");
		        	exec_cmd("uartdfifo.sh tfint");
		            usleep(200*1000);
		             
					 if(f_exists("/usr/script/playlists/tfPlaylistInfoJson.txt")){			
							char tempbuff[72];
							memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
							sprintf(tempbuff, "playlist.sh swtich_to_tf %d", result);
							exec_cmd(tempbuff);
					 }else{
							char tempbuff[72];
							memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
							sprintf(tempbuff, "playlist.sh tfautoplay %d", result);
							exec_cmd(tempbuff);
					 }			
			 	}
			}
    	}
    	else if (name[0] == 'm')
    	{
    		if(tf_status != 0)
    		{
    			eval("uartdfifo.sh", "tfout");
    			ret = cdb_set_int("$tf_status", 0);
    			ret = cdb_set_int("$tf_scan_over",0);
    			if (strcmp(value, "004") == 0)
    			{
        			eval("mpc", "clear");
        			if(!cdb_get_int("$vtf003_usb_tf",0)){
        			   cdb_set("$playmode", "000");
        			}
                }
                ret = cdb_set("$tf_mount_point", "");
                eval("mpc", "update", name );
                
    			cdb_set("$tf_status", "0");
				
                delete_file("/usr/script/playlists/tfPlaylist.m3u");
                delete_file("/usr/script/playlists/tfPlaylistInfoJson.txt");
				delete_file("/usr/script/playlists/currentPlaylistInfoJson.txt");


			}
			if(cdb_get_int("$vtf003_usb_tf",0)){
				//value = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
				cdb_get_str("$playmode",value,sizeof(value),"");
				if(usb_status!=0&&(strcmp(value,"004")==0)){
					cdb_set("$shutdown_before_playmode", "003");
				    if(!f_exists("/usr/script/playlists/usbPlaylist.m3u")){
		                char buf[32];
					    char value[16];
						cdb_set_int("$usb_scan_over", 0);
						exec_cmd("mpc clear");
						usleep(100*1000);
						memset(value,0,16);
						cdb_get_str("$usb_mount_point",value,16,"");
						sprintf(buf,"mpc update %s",value);
						exec_cmd(buf);
						usleep(200*1000);
						exec_cmd("mpc listall | grep sd > /usr/script/playlists/usbPlaylist.m3u");
						usleep(200*1000);
				    }
					char usb_last_contentURL[512]={0};	
					//usb_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.usb_last_contentURL");
					cdb_get_str("$usb_last_contentURL",usb_last_contentURL,sizeof(usb_last_contentURL),"");
					if(usb_last_contentURL!=NULL){
						/* 模式转换，U盘或tf卡一直在线，所以没有更新m3u文件*/	
						result=compare_FileAndUrl("/usr/script/playlists/usbPlaylist.m3u",usb_last_contentURL);
			            //free(usb_last_contentURL);
					}
		            if(result<=0){ /*如果再次插其他的U盘，歌曲不同，返回值为0，需要生成m3u文件*/
		                result=1;				
					}
				    exec_cmd("mpc clear > /dev/null");
		            usleep(200*1000);
					DEBUG_INFO("system creatPlayList usbmode before");
				    exec_cmd("uartdfifo.sh usbint");
		            usleep(200*1000);  
					if(f_exists("/usr/script/playlists/usbPlaylistInfoJson.txt"))
					{   
						char tempbuff[72];
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "playlist.sh swtich_to_usb %d", result);
						exec_cmd(tempbuff);   
					}else{
						char tempbuff[72];
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "playlist.sh usbautoplay %d", result);
						exec_cmd(tempbuff);
					}               
				}
			}
    	}		
exit1:	
		eval("umount", "-f", mntpath );
		unlink(mntpath);
		rmdir(mntpath);
		//cslog("umount m ------------\n");
	}
	
    if(ret != 0)
    {
        cslog("CDB set error....................\n");
    }
exit:
	save_diskinfo();
	return 0;
}



#define RST_BTN     "BTN_0"
#define WPS_BTN     "BTN_1"
#define RST_TO_DFT  (5)
#define ACT_PRESS   "pressed"
#define ACT_RELEASE "released"
void rst_ctrl(char *action, long seen)
{
	if (!strcmp(action, ACT_RELEASE))
	{
		if (RST_TO_DFT <= seen)
		{
			exec_cmd("/lib/wdk/cdbreset");
			killall("upmpdcli", SIGUSR1);
			reboot(RB_AUTOBOOT);
		}
		else
		{
			killall("upmpdcli", SIGUSR1);
			reboot(RB_AUTOBOOT);
		}
	}
}

void wps_ctrl(char *action)
{
#if 0
	if (!strcmp(action, ACT_PRESS))
	{
		set_time_press_btn(WPS_BTN);
	}
	else if (!strcmp(action, ACT_RELEASE))
	{
		if ((1000 * OMNI_TRI) <= get_time_btn_pressed(WPS_BTN))
		{
			exec_cmd("/lib/wdk/omnicfg_ctrl trigger");
		}
		else
		{
			exec_cmd("hostapd_cli wps_pbc");
			exec_cmd("wpa_cli wps_pbc");
		}
	}
#else
	if (!strcmp(action, ACT_RELEASE))
	{
		exec_cmd("/lib/wdk/omnicfg_ctrl trigger");
	}
#endif
}

void hotplug_button(char *button, char *action, char *seen)
{
	if (!strcmp(button, RST_BTN))
	{
		rst_ctrl(action, atol(seen));
	} else if (!strcmp(button, WPS_BTN)) {
		wps_ctrl(action);
	}
}
