#include <string.h>
#include <stdio.h>
#include <myutils/msg_queue.h>


void show_help(char *pro)
{
    char *cmd_list[]={
    "MCU+MUT+MIC&",   
    "MCU+UNM+MIC&",    
    "MCU+TLK+WECHAT&",    
    "MCU+TLK+CHATEND&",   
    };
    int i = 0;
    printf("usage:%s cmd\n",pro);
    for(i=0; i< (sizeof cmd_list)/sizeof(cmd_list[0]);i++)
    {
        printf("%s\n",cmd_list[i]);
    }
    
}

int main(int argc,char **argv)
{
    char msg[32] = {0};
    
    if(argc < 2)
    {
        show_help(argv[0]);
        return 0;
    }
    else
    {
#if 0	
        if(strncmp(argv[1],"MCU+",4)){
            printf("Bad cmd\n");
            return 0;
        }
#endif		
        strcpy(msg,argv[1]);
        
    }
    
    msg_queue_create_msg_queue(MAGIC_NUMBER);
    
    msg_queue_send(msg,MSG_UARTD_TO_IOT);

    return 0;
}


