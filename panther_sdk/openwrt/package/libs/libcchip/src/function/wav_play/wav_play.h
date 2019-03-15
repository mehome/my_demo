#ifndef WAV_PLAY_H_
#define WAV_PLAY_H_ 1


/*
 * @file mtdm_types.h
 *
 * @Author  Frank Wang
 * @Date    2015-10-06
 */

#define BUF_LEN_16      16
#define BUF_LEN_32      32
#define BUF_LEN_64      64
#define BUF_LEN_128     128
#define BUF_LEN_256     256
#define BUF_LEN_512     512
#define BUF_LEN_1024    1024
#define BUF_LEN_2048    2048
#define BUF_LEN_4096    4096
#define BUF_LEN_8192    8192

#ifndef  NULL
#define  NULL   ((void*)0)
#endif

#define YES     1
#define NO      0
#define NotUsed (-1)

#define success 1
#define fail    0

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define BUF_LEN_4    4
#define BUF_LEN_8    8
#define BUF_LEN_16   16
#define BUF_LEN_32   32
#define BUF_LEN_64   64
#define BUF_LEN_128  128
#define BUF_LEN_256  256
#define BUF_LEN_512  512
#define BUF_LEN_1024 1024
#define BUF_LEN_2048 2048
#define BUF_LEN_4096 4096
#define BUF_LEN_8192 8192

#define IBUF_LEN     4   /* for cdb INT */
#define PBUF_LEN     16  /* for cdb IP */
#define MBUF_LEN     18  /* for cdb MAC */
#define SBUF_LEN     512 /* for cdb STR */
#define USBUF_LEN    BUF_LEN_32  /* for cdb small STR */
#define SSBUF_LEN    BUF_LEN_64  /* for cdb small STR */
#define MSBUF_LEN    BUF_LEN_128 /* for cdb middle STR */
#define LSBUF_LEN    BUF_LEN_512 /* for cdb large STR */


#define s8           char
#define u8           unsigned char
#define s16          short
#define u16          unsigned short
#define s32          int
#define u32          unsigned int


#include <byteswap.h>
#include <alsa/asoundlib.h>

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef long long 		 off64_t;

#define 	DEBUG 				0
#define	MICRO_BUF_SIZE			64
#define	MIN_BUF_SIZE			128
#define	MID_BUF_SIZE			512
#define	MAX_BUF_SIZE			1024

/* it's in chunks like .voc and amiga iff, but my source say there
   are in only in this combination, so i combined them in one header;
   it works on all wave-file i have
 */

typedef struct wavheader {
	uint32_t magic;						/* 'riff' */
	uint32_t length;					/* filelen */
	uint32_t type;						/* 'wave' */
} wavheader_t;

typedef struct wavfmt {
	uint32_t magic;  					/* 'fmt '*/
	uint32_t fmt_size; 				/* 16 or 18 */
	uint16_t format;					/* see wav_fmt_* */
	uint16_t channels;
	uint32_t sample_rate;			/* frequence of sample */
	uint32_t bytes_p_second;
	uint16_t blocks_align;		/* samplesize; 1 or 2 bytes */
	uint16_t sample_length;		/* 8, 12 or 16 bit */
} wavfmt_t;									/* 24 bytes */


typedef struct wavfmt_body {
	uint16_t format;		/* see WAV_FMT_* */
	uint16_t channels;
	uint32_t sample_fq;		/* frequence of sample */
	uint32_t byte_p_sec;
	uint16_t byte_p_spl;	/* samplesize; 1 or 2 bytes */
	uint16_t bit_p_spl;		/* 8, 12 or 16 bit */
} wavfmt_body_t;					/* 16 bytes */


typedef struct wavfmt_body_ext{
	wavfmt_body_t format;		/* 16 bytes */
	uint16_t ext_size;		
	uint16_t bit_p_spl;
	uint32_t channel_mask;
	uint16_t guid_format;	/* WAV_FMT_* */
	uint8_t  guid_tag[14];	/* WAV_GUID_TAG */
} wavfmt_body_ext_t;		/* 40 bytes */

typedef struct wavchunkheader {
	uint32_t type;						/* 'data' */
	uint32_t length;					/* sample count */
} wavchunkheader_t;					/* 8 bytes */

typedef struct wavcontainer {
	wavheader_t header;				/* 12 bytes */
	wavfmt_t format;					/* 24 bytes */
	wavchunkheader_t chunk;		/* 8 bytes */
} wavcontainer_t;


typedef struct sndpcmcontainer {
	snd_pcm_uframes_t chunk_size;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_format_t format;
	uint16_t channels;
	size_t chunk_bytes;
	size_t bits_per_sample;
	size_t bits_per_frame;

	uint8_t *data_buf;
} sndpcmcontainer_t;

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)		      (v)
#define LE_INT(v)		        (v)
#define BE_SHORT(v)		      bswap_16(v)
#define BE_INT(v)		        bswap_32(v)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)		      bswap_16(v)
#define LE_INT(v)		        bswap_32(v)
#define BE_SHORT(v)		      (v)
#define BE_INT(v)		        (v)
#else
#error "Wrong endian"
#endif

/* Note: the following macros evaluate the parameter v twice */
#define TO_CPU_SHORT(v, be) \
	((be) ? BE_SHORT(v) : LE_SHORT(v))
#define TO_CPU_INT(v, be) \
	((be) ? BE_INT(v) : LE_INT(v))


#define WAV_RIFF		COMPOSE_ID('R','I','F','F')
#define WAV_WAVE		COMPOSE_ID('W','A','V','E')
#define WAV_RIFX		COMPOSE_ID('R','I','F','X')

#define WAV_FMT			COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		COMPOSE_ID('d','a','t','a')
#define WAV_LIST		COMPOSE_ID('L','I','S','T')

/* WAVE fmt block constants from Microsoft mmreg.h header */
#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe
/* Used with WAV_FMT_EXTENSIBLE format */
#define WAV_GUID_TAG		"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71"

#define ROUNDUP(x, y) ((((x)+(y)-1)/(y))*(y))
int wav_play_preload(const char *filename);
int wav_play(const char *filename);


#define UNSIGNED_LONG 4294967295UL 

#define DEFAULT_FORMAT		SND_PCM_FORMAT_U8

#define TONE_VOL 60

int tone_wav_play(const char *path);
#endif

