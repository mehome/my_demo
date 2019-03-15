#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/ipc.h>   
#include <sys/msg.h> 

#include <errno.h>
#include "msg_queue.h"
#include "log.h"
#include "debug.h"


int msg_queue_id = 0;



//创建消息队列
//返回值：> 0成功; <0 失败
int msg_queue_create_msg_queue(int magic_number)
{
    int ret = 0;
    int key = ftok( "/", magic_number );
    
    if(key < 0)
    {
        key = 12306;
    }

    msg_queue_id = msgget(key,IPC_CREAT|0666); //没有，则创建；有，则直接使用。
    if(msg_queue_id < 0)
    {
        printf("Create message queue failed:%s\n",strerror(errno));
        ret = -1;
    }

    return ret;
}


//记得释放空间
//接收消息队列中指定类型的消息
char * msg_queue_recv(int msg_type)
{
    msg_queue_t msg = {0};
    int ret = 0;
    printf("Waiting now..................\n");
    ret = msgrcv(msg_queue_id,&msg,sizeof(msg_queue_t),msg_type,0); /*接收消息队列*/
    printf("text=[%s]\n",msg.msgtext);
    return strdup(msg.msgtext);
}


//将数据发送到消息队列中，数据长度限制为512字节
//msg_ype:MSG_UARTD_TO_IOT 或者 MSG_IOT_TO_UARTD
//返回值：成功返回0；失败返回-1
int msg_queue_send(char *info,int msg_type)
{
    int ret_value = 0;
    msg_queue_t msg = {0};

    msg.msgtype = msg_type;
    strncpy(msg.msgtext,info,512);
    if(strlen(info) > 512)
    {
        warning("Too many datas to send\n");
    }
    //能发送就发送，不能发送就丢掉数据立即返回
    ret_value = msgsnd(msg_queue_id,&msg,strlen(msg.msgtext),IPC_NOWAIT);
    if(ret_value < 0)
    {
        printf("Send message failed:%s\n",strerror(errno));
        
    }
    else
    {
        printf("Send over................\n");
    }
    return ret_value;
}



int msg_queue_del()
{
    msgctl(msg_queue_id,IPC_RMID,0); //删除消息队列
}

