#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <regex.h>
#include "montage.h"
#include "debug.h"
#include <wdk/cdb.h>
#define LED_ON 	1
#define LED_OFF 0

//#define LED_INDEX_0 6
//#define LED_INDEX_1 7
//#define LED_INDEX_2 9
//#define LED_INDEX_3 10
//#define LED_INDEX_4 11
//#define LED_INDEX_5 12
//#define LED_INDEX_6 8

#define GPIO6 		6	//45
#define GPIO8		8	//47
#define GPIO9		9	//46
#define GPIO12		12	//52
#define GPIO17 		17	//11
#define GPIO18 		18	//12
#define GPIO19 		19	//13
#define GPIO20 		20	//14
#define GPIO25 		25	//37
#define LAN_LED		40
#define WAN_LED		41
#define WIFI_LED	43

#define MAX_DEV_NAME 256

/* BEGIN: Added by Frog, 2017-4-12 14:50:32 */
#define USB_TF_AUTO_PLAY_AFTER_BOOTING    
//#undef  USB_TF_AUTO_PLAY_AFTER_BOOTING
/* END:   Added by Frog, 2017-4-12 14:50:37 */

static char gpio_name[MAX_DEV_NAME];

extern pthread_mutex_t pmMUT;
extern int iUartfd;


int match(const char *string, char *pattern)
{
    int    status;
    regex_t    re;


    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
        return(0);      /* Report error. */
    }
    status = regexec(&re, string, (size_t) 0, NULL, 0);
    regfree(&re);
    if (status != 0) {
        return(0);      /* Report error. */
    }
    return(1);
}
/* 
* get bootload's cdb arguments 
*/
int boot_cdb_get(char *name, char *val)
{
	FILE *fp = NULL;
	char *p = NULL;
	char *eq = NULL;
	char buf[256];
	 
	fp = fopen("/proc/bootvars", "r"); 
	if(fp)
	{
		memset(buf, 0, sizeof(buf));
		while(fgets(buf, sizeof(buf), fp) != NULL)
		{
			buf[strcspn(buf, "\n")] = '\0';
			if((p = strstr(buf, name)))
				break;
			memset(buf, 0, sizeof(buf));
		}
		fclose(fp);
	 	 
		if(p)
		{
			eq = strchr(buf, '=');
			*eq = '\0';
			strcpy(val, eq + 1);
			return 1;
		}
	} 
	return 0;
}



// GPIO functions
//
/* export the gpio to user mode space */
static int export_gpio(int gpio)
{
        int err = 0;
        int fd;
        int wr_len;
        
        /* check if gpio is already exist or not */ 
        sprintf(gpio_name, "/sys/class/gpio/gpio%d/value", gpio);
                
        fd = open(gpio_name,O_RDONLY);
        if ( fd >= 0 ) {
                /* gpio device has already exported */
                close(fd);
                return 0;
        }
        
        /* gpio device is no exported, export it */
        strcpy(gpio_name,"/sys/class/gpio/export");
                
        fd = open(gpio_name,O_WRONLY);

        if (fd < 0) {
                fprintf(stderr, "failed to open device: %s\n", gpio_name);
                return -ENOENT;
        }
        DEBUG_INFO("%d export ok", gpio);
        sprintf(gpio_name,"%d",gpio);
        wr_len = strlen(gpio_name);

        if (write(fd,gpio_name,wr_len)!=wr_len) {
                fprintf(stderr, "failed to export gpio: %d\n", gpio);
                return -EACCES;
        }
		DEBUG_INFO("%d export ok", gpio);

        return err;
}
static int free_gpio( int gpio)
{
        int err = 0;
        int fd;
        int wr_len;

        /* check if gpio exist */
        sprintf(gpio_name, "/sys/class/gpio/gpio%d/value", gpio);

        fd = open(gpio_name,O_RDONLY);
        if ( fd < 0 ) {
                /* gpio device doesn't exist */
                return -ENOENT;
        }
        close(fd);

        /* gpio device is no exported, export it */
        strcpy(gpio_name,"/sys/class/gpio/unexport");

        fd = open(gpio_name,O_WRONLY);

        if (fd < 0) {
                fprintf(stderr, "failed to open device: %s\n", gpio_name);
                return -ENOENT;
        }

        sprintf(gpio_name,"%d",gpio);
        wr_len = strlen(gpio_name);

        if (write(fd,gpio_name,wr_len)!=wr_len) {
                fprintf(stderr, "failed to free gpio: %d\n", gpio);
                return -EACCES;
        }
		DEBUG_INFO("%d unexport ok", gpio);
        return err;

}

static int set_gpio_dir(int gpio)
{
        int err = 0;
        int fd;
        int wr_len;
        char *str_dir = "out";

       // err = export_gpio(gpio);

      //  if (err)
        //        return err;

        sprintf(gpio_name, "/sys/class/gpio/gpio%d/direction", gpio);

        fd = open(gpio_name,O_WRONLY);

        if (fd < 0) {
                fprintf(stderr, "failed to open device: %s\n", gpio_name);
                err = -ENOENT;
                goto exit;
        }

//        strcpy(str_dir,"out");
        wr_len = strlen(str_dir);
        if (write(fd, str_dir,wr_len) != wr_len) {
                fprintf(stderr, "Failed to set gpio %s direction to %s \n", gpio_name, str_dir);
                err =  -EACCES;
                goto exit;
        }

		DEBUG_INFO("%d out ok", gpio);

exit:
        close(fd);
	return err;
}

static int set_gpio_value(int gpio, int value)
{ 
        int err = 0; 
        int fd; 
 
	   //	err = export_gpio(gpio); 
 
       // if (err)  
          //      return err; 
 
        sprintf(gpio_name, "/sys/class/gpio/gpio%d/value", gpio); 
                 
        fd = open(gpio_name,O_WRONLY); 
 
        if (fd < 0) { 
                fprintf(stderr, "failed to open device: %s\n", gpio_name); 
                err = -ENOENT; 
                goto exit; 
        } 
 
        if (write(fd, value? "1":"0", 1) != 1) {  
                fprintf(stderr, "Failed to set gpio %s to %d\n", gpio_name, value); 
                err =  -EACCES; 
                goto exit; 
        } 
 	
		DEBUG_INFO("%d write %d ok", gpio, value);
exit: 
        close(fd); 
         
        return err; 
}

static int control_gpio1(int gpio, int value)
{
	int err = 0;

	free_gpio(gpio);
	err = export_gpio(gpio);
	if(err)
		return err;
	err =  set_gpio_dir(gpio);
	
	err =  set_gpio_value(gpio, value);

	return 0;
	
}

static int control_gpio(int gpio, int value)
{		
	int fd;	
	int err;
	char buf[64];	


	memset(buf,0, sizeof(buf));		
	fd = open("/sys/class/gpio/unexport", O_WRONLY);		
	if(fd < 0) {
		DEBUG_ERR("Can not open GPIO unexport");	
		return -ENOENT;	
	}
	
	sprintf(buf, "%d", gpio);		
	if (write(fd, buf, strlen(buf)) != strlen(buf)){
		DEBUG_ERR("%d unexport write failed ",gpio);				
		goto err;				
	}		
	DEBUG_INFO("%d unexport success", gpio);
	close(fd);		
	
	memset(buf,0, sizeof(buf));		
	fd = open("/sys/class/gpio/export", O_WRONLY);		
	if(fd < 0) {
		DEBUG_ERR("Can not open GPIO export");	
		return -ENOENT;			
	}		
	sprintf(buf, "%d", gpio);		
	if (write(fd, buf, strlen(buf)) != strlen(buf)){
		DEBUG_ERR("%d export  failed ",gpio);	
		err =  -EACCES; 
		goto err;				
	}
	 DEBUG_INFO("%d export success", gpio);
	close(fd);	

	
	memset(buf,0, sizeof(buf));		
	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);		
	fd = open(buf, O_WRONLY);		
	if (fd < 0)	{	
		DEBUG_ERR("Can not open GPIO%d direction", gpio);	
		err = -ENOENT; 
		goto err;				
	}
	
	if ( write(fd, "out", 4) != 4) {
		DEBUG_ERR("%d direction write failed ",gpio);		
		 err =  -EACCES; 
		goto err;				
	}	

	DEBUG_INFO("%d out success", gpio);
	close(fd);	

	
	memset(buf,0, sizeof(buf));		
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);		
	fd = open(buf, O_WRONLY);		
	if (fd < 0)	{	
		DEBUG_ERR("Can not open gpio%d/value",gpio);	
		return -ENOENT;			
	}		
	// Set GPIO high status		
	//memset(buf,0, sizeof(buf));
	//sprintf(buf, "%d", value);		
	if ( write(fd, value? "1":"0", 1) != 1){
		DEBUG_ERR("%d value write failed ", gpio);
		err =  -EACCES; 
		goto err;				
	}	
	DEBUG_INFO("%d write %d success", gpio, value);
	close(fd);		
	return 0;	
err:
	close(fd);
	return err;
}

/*
GPIO6	
GPIO8	
GPIO9	
GPIO12	
GPIO17 	
GPIO18 	
GPIO19 	
GPIO20 	
GPIO25 	
LAN_LED	
WAN_LED	
WIFI_LED
*/



void gpio_test_xzx()
{
	#if 1
		control_gpio1(GPIO6,LED_OFF);
		control_gpio1(GPIO8,LED_OFF);
		control_gpio1(GPIO9,LED_OFF);
		control_gpio1(GPIO12,LED_OFF);
		control_gpio1(GPIO17,LED_OFF);
		control_gpio1(GPIO18,LED_OFF);
		control_gpio1(GPIO19,LED_OFF);
		control_gpio1(GPIO20,LED_OFF);
		control_gpio1(GPIO25,LED_ON);
//		control_gpio1(LAN_LED,LED_OFF);
//		control_gpio1(WAN_LED,LED_OFF);
//		control_gpio1(WIFI_LED,LED_OFF);
	#else
	
	int i = 0;
	for ( i = 0; i < 34; i++) {
		control_gpio1(i,1);
	}
	#endif
}

static void start_play(char *dir, char *name)
{
 	char cmd[128] = {0};
	char path[100] = {0};
	strcat(path, dir);
	strcat(path, "/");
	strcat(path, name);

	DEBUG_INFO("wavplayer %s ", path);

	snprintf(cmd, 128 ,"(amixer -c 0 set PCM 255 > /dev/null 2&>1 ) && aplay %s",path);
	//system(cmd);
	my_system(cmd);

}

int wait_until_boot_done()
{
    FILE *fp1 = NULL;
    char a = 0;
    int counter = 0;

     
    while(counter < 60)
    {
        fp1 = fopen("/tmp/boot_done","r");
        if(NULL == fp1)
        {       
            counter ++;
                sleep(1);
                continue;
        }
        fread(&a,1,1,fp1);
        fclose(fp1);
        if(a == 0x31)
        {
                printf("Boot done\n");
                break;
        }  
        counter ++;
        sleep(1);
        printf("Bootting now\n");
    }    
    fp1 = NULL;
    while(counter < 60)
    {
        fp1 = fopen("/tmp/mpd_done","r");
        if(NULL == fp1)
        {       
            counter ++;
                sleep(1);
                continue;
        }
        fread(&a,1,1,fp1);
        fclose(fp1);
        if(a == 0x31)
        {
                printf("Boot done\n");
                break;
        }  
        counter ++;
        sleep(1);
        printf("Bootting now\n");
    }    

    return 0;
}

int delete_file(char *filename)
{
    char cmd[64];
    
    memset(cmd,0,64);
    if(0 == access(filename,F_OK))
    {
        sprintf(cmd,"rm %s",filename);
        my_system(cmd);
        usleep(100*1000);
    }
}

int get_line_count(char *filename)
{

 int ch=0;
 int n=0;
 FILE *fp;
 fp=fopen(filename,"r");
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
int compare_FileAndUrl(char *filepatch,char *Url)
{

   if((NULL!=Url)&&(NULL!=filepatch)){

      FILE *dp =NULL;
	  char *deline;
      char buf[512];
	  int ret=0;
	  int i;
	  int hangnum=get_line_count(filepatch);

   	   dp= fopen(filepatch, "r+");
	    if(NULL==dp){
		   DEBUG_INFO("file no exist\n");
           return -1;
		}
            
	         for(i=0;i<hangnum;i++)
	         {
			 deline=fgets(buf,512, dp);	//ä¸€æ¬¡èŽ·å–ä¸€è¡Œæ•°æ®
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




int start_playtf(char* playmode)
{
    int result=0;
	char buf[32];
    char value[16];

	if(strcmp(playmode,"003")==0)  return 0;
		
	
	        usleep(200*1000);
			pthread_mutex_lock(&pmMUT);
			DEBUG_ERR("write AXX+PLM++TF");
			write(iUartfd, "AXX+PLM++TF&",sizeof("AXX+PLM++TF&"));
			pthread_mutex_unlock(&pmMUT); 
			
			usleep(100*1000);
			
			delete_file("/usr/script/playlists/tfPlaylistInfoJson.txt");

			
		//	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.shutdown_before_playmode", "004");
		//	char* sd_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.sd_last_contentURL");
		   // char* sd_last_contentURL = cdb_get_str("sd_last_contentURL");
		   cdb_set_int("$shutdown_before_playmode", 004);
		   char sd_last_contentURL[SSBUF_LEN_512] = {0};
		   cdb_get_str("$sd_last_contentURL",sd_last_contentURL,sizeof(sd_last_contentURL),"");
			if(!strcmp(sd_last_contentURL,""))
			{
				my_system("mpc listall | grep mmc > /usr/script/playlists/tfPlaylist.m3u");
				usleep(200*1000);
				result=compare_FileAndUrl("/usr/script/playlists/tfPlaylist.m3u",sd_last_contentURL);
				//free(sd_last_contentURL);
			}
			if(result<=0) {  /*Èç¹ûÔÙ´Î²åÆäËûµÄUÅÌ£¬¸èÇú²»Í¬£¬·µ»ØÖµÎª0£¬ÐèÒªÉú³Ém3uÎÄ¼þ*/
				result=1; 
			}  
			char tempbuff[72];
			memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			sprintf(tempbuff, "playlist.sh tfautoplay %d", result);
			my_system(tempbuff);
		
}
/*************
if usb_status ==1

shutdown_before_playmode=003  play usb
shutdown_before_playmode=null  no tf,play usb 




****************/
int start_playusb(char *playmode,int tf_status)
{
		int result=0;
		char buf[32];
        char value[16];

		int ret=strcmp(playmode,"004");
		int rat=strcmp(playmode,"003");
	    
		if(ret==0)
			return 0;
		if((tf_status==1)&&(rat!=0))
			return 0;
		
        cdb_set_int("$usb_scan_over", 0);
        my_system("mpc clear");
        usleep(100*1000);

        memset(value,0,16);
	  	cdb_get_str("$usb_mount_point",value,16,"");
	  	sprintf(buf,"mpc update %s",value);
        my_system(buf);

        usleep(200*1000);
        pthread_mutex_lock(&pmMUT);
        write(iUartfd, "AXX+PLM+USB",sizeof("AXX+PLM+USB&"));
        pthread_mutex_unlock(&pmMUT); 

        delete_file("/usr/script/playlists/usbPlaylistInfoJson.txt");
		cdb_set_int("$shutdown_before_playmode", 003);
		//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.shutdown_before_playmode", "003");
		//char* usb_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.usb_last_contentURL");
        //if(usb_last_contentURL!=NULL){
        char usb_last_contentURL[SSBUF_LEN_512] = {0};
		cdb_get_str("$usb_last_contentURL",usb_last_contentURL,sizeof(usb_last_contentURL),"");
		if(!strcmp(usb_last_contentURL,"")){
			my_system("mpc listall | grep sd > /usr/script/playlists/usbPlaylist.m3u");
			usleep(200*1000);
			result=compare_FileAndUrl("/usr/script/playlists/usbPlaylist.m3u",usb_last_contentURL);
        	//free(usb_last_contentURL);
		}
		if(result<=0){
			result=1; 
     	}

		char tempbuff[72];
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		sprintf(tempbuff, "playlist.sh usbautoplay %d", result);
	    my_system(tempbuff); 
  }


void  uartd_UsbTf_status(int tf_status,int usb_status){
	if(tf_status == 1 ) {
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+TF++ONL&",sizeof("AXX+TF++ONL&"));
			pthread_mutex_unlock(&pmMUT);
	
	
			char buf[32];
			char value[16];
			cdb_set_int("$tf_scan_over",0);
			my_system("mpc clear");
			memset(value,0,16);
			cdb_get_str("$tf_mount_point",value,16,"");
			sprintf(buf,"mpc update %s",value);
			my_system(buf);
			DEBUG_ERR("cmd = %s\n",buf);
			usleep(200*1000);
			my_system("mpc listall | grep mmc > /usr/script/playlists/tfPlaylist.m3u");
			usleep(200*1000);
		
		}else 
		{
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+TF++OUL&",sizeof("AXX+TF++OUL&"));
			pthread_mutex_unlock(&pmMUT);
		}
		
	  usleep(500*1000);

	if(usb_status == 1 ){
		usleep(200*1000);
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+USB+ONL&",sizeof("AXX+USB+ONL&"));
		pthread_mutex_unlock(&pmMUT);

		char buf[32];
	    char value[16];
		cdb_set_int("$usb_scan_over", 0);
		my_system("mpc clear");
		usleep(100*1000);
		memset(value,0,16);
		cdb_get_str("$usb_mount_point",value,16,"");
		sprintf(buf,"mpc update %s",value);
		my_system(buf);
		usleep(200*1000);
		my_system("mpc listall | grep sd > /usr/script/playlists/usbPlaylist.m3u");
		usleep(200*1000);
		
		}else 
		{
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+USB+OUL&",sizeof("AXX+USB+OUL&"));
		pthread_mutex_unlock(&pmMUT);
		}
} 
void* check_usb_tf()
 {
    int result=-1;
	int tf_status =0;
	int usb_status = 0;
	char buf[32];
	char value[16];
	FILE *fp_boot_done = NULL;
	struct stat st;
	int sys_auto_upgrade = 0;

	wait_until_boot_done();
	tf_status = cdb_get_int("$tf_status", 0);
	usb_status = cdb_get_int("$usb_status", 0);
	DEBUG_INFO("tf_status=%d, usb_status=%d", tf_status, usb_status);

	if(cdb_get_int("$vtf003_usb_tf",0)){

	//char *plm=NULL;
    //plm=WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.shutdown_before_playmode");
    char plm[SSBUF_LEN_16] = {0};
	cdb_get_str("$shutdown_before_playmode",plm,sizeof(plm),"");
	printf("plm===========[%s]\n",plm);
    usleep(200*1000);
	
    uartd_UsbTf_status(tf_status,usb_status);
    usleep(200*1000);

	//delete_file("/usr/script/playlists/usbPlaylist.m3u"); /* ¿ª»úµÄÊ±ºò²åÓÐUÅÌ¸úTF¿¨ ¿ª»ú²¥·ÅµÄÊÇTF¿¨ ÔÙÇÐ»»Ä£Ê½UÅÌ¾Í²»²¥      */
    //delete_file("/usr/script/playlists/tfPlaylist.m3u");
	
	if((strcmp(plm,"003")==0)||(strcmp(plm,"004")==0)){

	if(tf_status == 1 ) 
		{
#ifdef USB_TF_AUTO_PLAY_AFTER_BOOTING	
		start_playtf(plm);
#endif							
		}
	usleep(200*1000);
	if(usb_status == 1 ) 
		{
#ifdef  USB_TF_AUTO_PLAY_AFTER_BOOTING
		  start_playusb(plm,tf_status);
#endif       
			}
	}
		}else{
		if(tf_status == 1 ) 
		{
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+TF++ONL&",sizeof("AXX+TF++ONL&"));
			pthread_mutex_unlock(&pmMUT);
			
#ifdef USB_TF_AUTO_PLAY_AFTER_BOOTING	

	        usleep(200*1000);
	    	pthread_mutex_lock(&pmMUT);
	    	DEBUG_ERR("write AXX+PLM++TF");
	    	write(iUartfd, "AXX+PLM++TF&",sizeof("AXX+PLM++TF&"));
	    	pthread_mutex_unlock(&pmMUT); 
	    	
	        usleep(100*1000);
	        delete_file("/usr/script/playlists/tfPlaylist.m3u");
	        delete_file("/usr/script/playlists/tfPlaylistInfoJson.txt");
	        
	        cdb_set_int("$tf_scan_over",0);
	    	my_system("mpc clear");
	    	memset(value,0,16);
		  	cdb_get_str("$tf_mount_point",value,16,"");
		  	sprintf(buf,"mpc update %s",value);
		  	my_system(buf);
		  	DEBUG_ERR("cmd = %s\n",buf);
		  	usleep(200*1000);
	        my_system("playlist.sh tfautoplay");

#endif							
	}
	else 
	{
        pthread_mutex_lock(&pmMUT);
        write(iUartfd, "AXX+TF++OUL&",sizeof("AXX+TF++OUL&"));
        pthread_mutex_unlock(&pmMUT);
	}
	usleep(200*1000);
	if(usb_status == 1 ) 
	{
	    usleep(200*1000);
		pthread_mutex_lock(&pmMUT);
	    write(iUartfd, "AXX+USB+ONL&",sizeof("AXX+USB+ONL&"));
	    pthread_mutex_unlock(&pmMUT);

#ifdef  USB_TF_AUTO_PLAY_AFTER_BOOTING
        if(tf_status == 1)
        {
            return ;
        }
        
        cdb_set_int("$usb_scan_over", 0);
        my_system("mpc clear");
        usleep(100*1000);

        memset(value,0,16);
	  	cdb_get_str("$usb_mount_point",value,16,"");
	  	sprintf(buf,"mpc update %s",value);
        my_system(buf);

        usleep(200*1000);
        pthread_mutex_lock(&pmMUT);
        write(iUartfd, "AXX+PLM+USB",sizeof("AXX+PLM+USB&"));
        pthread_mutex_unlock(&pmMUT); 

        delete_file("/usr/script/playlists/usbPlaylist.m3u");
        delete_file("/usr/script/playlists/usbPlaylistInfoJson.txt");

        my_system("playlist.sh usbautoplay");
        usleep(1000*1000);
#endif       
 	} 
 	else 
 	{
        pthread_mutex_lock(&pmMUT);
        write(iUartfd, "AXX+USB+OUL&",sizeof("AXX+USB+OUL&"));
        pthread_mutex_unlock(&pmMUT);
	}
 			

		}
}




 void scan_usb_mmc()
{
	FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	int usb = 0;
	char *usb_dir =NULL;
	char usb_path[64] = {0};
	int sd = 0;
	char *sd_dir =NULL;
	char sd_path[64]  = {0};

	DEBUG_INFO("Read file content of /var/diskinfo.txt");
	stream = fopen("/var/diskinfo.txt", "r");
	if (stream == NULL) {
		DEBUG_ERR("Result shows no disk inserted");
		return;
	}

	while ((read = getline(&line, &len, stream)) != -1) {
		DEBUG_INFO("file: %s", line);
		line[strlen(line) -1] = 0x00; /* delete the \n, or opendir will fail */
		
		DIR *d = NULL;
		struct dirent *dir = NULL;
		d = opendir(line);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
	
				if (dir->d_name[0] == '.')
					continue;
				if ((dir->d_type == DT_REG) && ( strcmp(dir->d_name, "usb.wav") == 0)) {
					//DEBUG_ERR("Found :%s/%s", line, dir->d_name);
					start_play(line, dir->d_name);
					my_system("uartdfifo.sh usb ok");
					DEBUG_INFO("usb ok");
					usb = 1;
					//usb_dir = line;
				}
				if ((dir->d_type == DT_REG) && ( strcmp(dir->d_name, "sd.wav") == 0)) {
					//DEBUG_ERR("Found :%s/%s", line, dir->d_name);
					my_system("uartdfifo.sh sd ok");
					start_play(line, dir->d_name);
					DEBUG_INFO("sd ok");
					sd = 1;
					//sd_dir = line;
				}
			}
			closedir(d);
		} else {
			DEBUG_ERR("opendir %s  failed, err= %d", line, errno);
		}
	}

	if(sd == 0) {
		my_system("uartdfifo.sh sdfail");
	}
	if(usb == 0) {
		my_system("uartdfifo.sh usbfail");
	}
	DEBUG_INFO("Search End");
	free(line);
	fclose(stream);

}

static pthread_mutex_t popen_mtx = PTHREAD_MUTEX_INITIALIZER;

int exec_cmd(const char *cmd)
{
	int iRet = -1;
	char buf[1024];
	FILE *pfile;
	int status = -2;

	pthread_mutex_lock(&popen_mtx);
	if ((pfile = popen(cmd, "r"))) {
		fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
		while(!feof(pfile)) {
			fgets(buf, sizeof buf, pfile);
		}
		status = pclose(pfile);
		iRet = 0;
	}
	else
	{
		iRet = -1;
	}
	pthread_mutex_unlock(&popen_mtx);
	return iRet;
}

typedef void (*sighandler_t)(int);  
int pox_system(const char *cmd_line)  
{  
   int ret = 0;  
   sighandler_t old_handler;  
  
   old_handler = signal(SIGCHLD, SIG_DFL);  
  // ret =  change_system(cmd_line);
   ret = exec_cmd(cmd_line);
   signal(SIGCHLD, old_handler);  
  
   return ret;  
}  

int my_system(char *cmd)
{
  int ret;
  ret = exec_cmd(cmd);
  return ret;

}

