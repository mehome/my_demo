#ifndef _FIFO_H
#define _FIFO_H
#include <pthread.h>


//#define DECODE_FIFO_BUFFLTH (44100 * 2 * 5)
//#define DECODE_FIFO_BUFFLTH (16 * 1024 * 15)

//#define PLAY_FIFO_BUFFLTH (1024 * 1024 * 2)
//#define PLAY_FIFO_BUFFLTH (1024 * 600)


typedef struct ft_fifo {
	unsigned char *buffer;	/* the buffer holding the data */
	unsigned int size;	/* the size of the allocated buffer */
	unsigned int wrsize;/* the size of the start to write to file */
	unsigned int in;	/* data is added at offset (in % size) */
	unsigned int out;	/* data is extracted from off. (out % size) */
	pthread_mutex_t lock;	/* protects concurrent modifications */
}FT_FIFO;

/*
#define FIFO_FULL   1
#define FIFO_EMPTY  2
#define FIFO_FULL   1
#define FIFO_FULL   1
#define FIFO_FULL   1*/


extern FT_FIFO *ft_fifo_init(unsigned char *buffer, unsigned int size);
extern FT_FIFO *ft_fifo_alloc(unsigned int size);
extern void ft_fifo_free(FT_FIFO *fifo);
extern void _ft_fifo_clear(FT_FIFO *fifo);
extern unsigned int _ft_fifo_put(FT_FIFO *fifo,
				  unsigned char *buffer, unsigned int len);
extern unsigned int _ft_fifo_get(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int offset, unsigned int len);
extern unsigned int _ft_fifo_seek(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int offset, unsigned int len);
extern unsigned int _ft_fifo_setoffset(FT_FIFO *fifo,unsigned int offset);
extern unsigned int _ft_fifo_getlenth(FT_FIFO *fifo);
unsigned int _ft_fifo_change_length(FT_FIFO *fifo, unsigned int len);


static inline void ft_fifo_clear(FT_FIFO *fifo)
{
	if (NULL == fifo)
	{
		return ;
	}

	pthread_mutex_lock(&fifo->lock);

    _ft_fifo_clear(fifo);

    pthread_mutex_unlock(&fifo->lock);
}

static inline unsigned int ft_fifo_put(FT_FIFO *fifo,
				       unsigned char *buffer, unsigned int len)
{
	unsigned int ret;

	if (NULL == fifo)
	{
		error("NULL == fifo");
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);
	
	ret = _ft_fifo_put(fifo, buffer, len);
	
	pthread_mutex_unlock(&fifo->lock);

	return ret;
}

static inline unsigned int ft_fifo_get(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int offset, unsigned int len)
{
	unsigned int ret;

	if (NULL == fifo)
	{
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);

	ret = _ft_fifo_get(fifo, buffer, offset,len);
	if (fifo->in == fifo->out)
		fifo->in = fifo->out = 0;

	pthread_mutex_unlock(&fifo->lock);

	return ret;
}

static inline unsigned int ft_fifo_getlenth(FT_FIFO *fifo)
{
	unsigned int ret;

	if (NULL == fifo)
	{
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);

	ret = _ft_fifo_getlenth(fifo);

	pthread_mutex_unlock(&fifo->lock);

	return ret;
}

static inline unsigned int ft_fifo_seek(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int offset, unsigned int len)

{
	unsigned int ret;

	if (NULL == fifo)
	{
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);

	ret = _ft_fifo_seek(fifo, buffer, offset, len);

	pthread_mutex_unlock(&fifo->lock);

	return ret;
}

static inline unsigned int ft_fifo_setoffset(FT_FIFO *fifo,unsigned int offset)

{
	unsigned int ret;

	if (NULL == fifo)
	{
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);

	ret = _ft_fifo_setoffset(fifo, offset);

	pthread_mutex_unlock(&fifo->lock);

	return ret;
}

static inline unsigned int ft_fifo_delete_data(FT_FIFO *fifo,unsigned int offset)

{
	unsigned int ret;

	if (NULL == fifo)
	{
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);

	ret = _ft_fifo_change_length(fifo, offset);

	pthread_mutex_unlock(&fifo->lock);

	return ret;
}

#endif

