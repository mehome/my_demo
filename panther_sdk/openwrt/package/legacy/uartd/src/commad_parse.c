#include <pthread.h>  
#include <unistd.h>
#include <string.h>
#include "debug.h"
#include <libcchip/function/fifobuffer/fifobuffer.h>
#include "myutils/msg_queue.h"

#define COMMAND_FIFO_LENGTH (1024 * 2) 

FT_FIFO * command_fifo = NULL;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int uartd_pthread_cond_wait = 0;

int cmd_fifo_init()
{
	if (NULL == command_fifo) 
	{
		command_fifo = ft_fifo_alloc(COMMAND_FIFO_LENGTH);
		if(NULL == command_fifo)
			return -1;
		ft_fifo_clear(command_fifo);
	}

	return 0;
	
}

void cmd_fifo_free()
{
	if(NULL != command_fifo) 
	{
		ft_fifo_free(command_fifo);
	}
}

char *strstr_binary(char *father, int f_len, char *child, int c_len)
{
	int i = 0, child_flag = 0;
	if(c_len > f_len){
		return NULL;
	}
	while(i < f_len){
		if(father[i] == child[child_flag]){
			child_flag++;
		}else if((child_flag != 0 ) && (father[i] == child[0])){
			child_flag = 1;
		}else{
			child_flag = 0;
		}
		i++;
		if(child_flag == c_len){
			i -= c_len;
			break;
		}
	}
	if(i == f_len){
		return NULL;
	}else{
		return &father[i];
	}
}


void * command_parse(void *arg)
{
	int len = 0, i = 0;
	int cmd_len = 0, get_len = 0, tmp_len = 0;
	char buf[1024]={0};
    
	while(1) 
	{
		bzero(buf, sizeof(buf));
		pthread_mutex_lock(&mtx);    
		
		len = ft_fifo_getlenth(command_fifo);
		if(len == 0)
		{
			// DEBUG_INFO("len:%d, pthread_cond_wait",len);
			 uartd_pthread_cond_wait = 1;
			 pthread_cond_wait(&cond, &mtx); 
			 uartd_pthread_cond_wait = 0;
			 len = ft_fifo_getlenth(command_fifo);
		}
		DEBUG_INFO("parse cmd len :%d", len);
//		cmd_len = ft_fifo_seek_command(command_fifo,buf,0);
		get_len = ft_fifo_get(command_fifo, buf, 0, 1024);
		pthread_mutex_unlock(&mtx);
		if(get_len > 0) 
		{
#if 1
			i = 0;
			char *cmd = NULL, *tmp = NULL;
			cmd = strstr_binary(&buf[i], get_len-i, "MCU", 3);
			if(cmd != NULL){
				tmp_len = cmd - buf - i;
				i += (tmp_len +3);
			
				while(i < get_len){
					tmp = strstr_binary(&buf[i], get_len-i, "MCU", 3);
					if(tmp != NULL){
						cmd_len = tmp - cmd;
						i += cmd_len;
						cmd_len -= 2;
						uartd_handler(cmd, cmd_len);
						cmd = tmp;
					}else{
						cmd_len = get_len - i + 3;
						uartd_handler(cmd, cmd_len);
						break;
					}
				}
			}else{
				uartd_handler(buf, get_len);
			}

#else
			DEBUG_INFO("buf:%s, cmd_len=%d",buf, cmd_len);
			char *cmd = NULL;
			char *tmp = NULL;
			char *tmp1 = NULL;
			tmp = buf;
reparse:
			cmd = strstr(tmp, "MCU");
			if(cmd != NULL) {
//				DEBUG_INFO("cmd:%s",cmd);
				tmp1 = strstr(cmd+3, "MCU");
				if(tmp1 != NULL)
				{
					//DEBUG_INFO("tmp1:%s",tmp1);
					char ins[64]={0};
					cmd_len = tmp1-tmp;
					memcpy(ins, tmp, cmd_len);
//					DEBUG_INFO("ins:%s",ins);
					uartd_handler(ins, cmd_len);
					tmp = tmp1;
					goto reparse;
				}else {
					cmd_len -= (cmd - tmp);
					printf("cmdlen: %d, offset: %d\n", cmd_len, (cmd - tmp));
					uartd_handler(cmd, cmd_len);
				}
			} else {
				uartd_handler(buf, cmd_len);
			}
			
		}else {
			 DEBUG_INFO("cmd_len:%d, pthread_cond_wait",cmd_len);
			 uartd_pthread_cond_wait = 1;
			 pthread_cond_wait(&cond, &mtx); 
			 uartd_pthread_cond_wait = 0;
#endif			
		}
//		pthread_mutex_unlock(&mtx);     
	}
}
