#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#define MAGIC_NUMBER   (2018) 


#define  MSG_UARTD_TO_IOT   (1)
#define  MSG_IOT_TO_UARTD   (2)    


typedef struct _msg 
{  
   long msgtype;  
   char msgtext[512];  
}msg_queue_t; 

//创建消息队列
int msg_queue_create_msg_queue(int magic_number);
//从消息队列查询指定的消息
char * msg_queue_recv(int msg_type);
//向消息队列中发送消息
int msg_queue_send(char *info,int msg_type);
//删除消息队列
int msg_queue_del();


#endif
