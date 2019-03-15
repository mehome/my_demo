#ifndef __SOUNDFIEUTILS_H__
#define __SOUNDFIEUTILS_H__

#include <stdio.h>

#ifdef _WIN32
#include <malloc.h>

#define strcasecmp		_stricmp
#define stat			_stat
#define strdup			_strdup
#define unlink			_unlink
//#define fopen			fopen_s
#endif	// _WIN32

#if (defined(_WIN32) || defined(_WIN64))&& (defined( _WINDOWS) ||defined(WINFORMS))
__pragma(warning(disable:4127))
__pragma(warning(disable:4200))
#endif

#include "spf-postapi.h"

#define SOUNDFILE_WAV	1
#define SOUNDFILE_PCM	0


#define RIFF_MAGIC  'FFIR'
#define RIFF_TYPE   'EVAW'
#define RIFF_FORMAT ' tmf'
#define RIFF_DATA   'atad'




typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;
typedef short	int16_t;

typedef struct riff_header_s {
    uint32_t magic;              /* always 'RIFF'                   */
    uint32_t length;             /* length of file                  */
    uint32_t chunk_type;         /* always 'WAVE'                   */
    uint32_t chunk_format;       /* always 'fmt'                    */
    uint32_t chunk_length;       /* length of data chunk (bits = 16)*/
    uint16_t format;             /* always 1 (PCM)                  */
    uint16_t channels;           /* 1 (monoral) or 2 (stereo)       */
    uint32_t sample_rate;        /* sampling rate (Hz)              */
    uint32_t speed;              /* byte / sec                      */
    uint16_t sample_size;        /* bytes / sample                  */
    /* mono:   1 (8bit) or 2 (16 bit)  */
    /* stereo: 2 (8 bit) or 4 (16 bit) */
    uint16_t precision;          /* bits / sample 8, 12 or 16       */
    uint32_t chunk_data;         /* always 'data'                   */
    uint32_t data_length;        /* length of data chunk            */
} riff_header_t;

typedef enum sftype_e {
    mono_wav = 0,
    stereo_wav,
    mono_pcm,
    stereo_pcm
} sftype_t;

typedef struct wav_info_s {
    FILE			*fd;
    uint32_t		length;	// filelength in samples per channel
    char			*name;
    sftype_t		type;
    riff_header_t	hdr;
} wav_info_t;


#define ISSTEREO(t)	((t) == stereo_wav || (t) == stereo_pcm)
#define ISMONO(t)	(!ISSTEREO(t))



struct sfinput_s
{
    struct wav_info_s	*txrx;		// stereo in
    struct wav_info_s	*tx[16];	// mono tx in (microphone)
    struct wav_info_s	*rx[16];	// mono rx in (speaker)
    int					ntx, nrx;   // number of tx and rx channels
    uint32_t			what;		// defines input pattern(s) as:
#define INPUT_TXRX			(1<<0)			// 0x01
#define INPUT_TX			(1<<1)			// 0x02
#define INPUT_RX			(1<<2)			// 0x04
#define CX_PATTERN_0		(INPUT_TXRX)					// ref AND prm
#define CX_PATTERN_1		(INPUT_TX | INPUT_RX)			// ref + prm
    uint32_t			length;		// max length
    uint32_t			fs;			// sampling rate
    uint32_t			show_progress;
    uint32_t			read_so_far;
};

#define OUTPUT_TX			0x01
#define OUTPUT_RX			0x02
#define OUTPUT_TXTX			0x04

struct sfoutput_s {
    struct wav_info_s	*txtx;
    char				*txpattern;
    char				*rxpattern;
    struct wav_info_s	*tx[16];
    struct wav_info_s	*rx[16];
    int					ntx, nrx;
    int					what;
};




extern PROFILE_TYPE(t) *base_profile();
extern void init_profile(void);
extern void *read_profile(char *name);
extern void assign_profile(void *b);


extern void extra_profile_check(void);
extern void extra_profile_afl_check(void);
extern void error_exit(int);
extern void report_warning(const char *f, ...);
extern void report_error(const char *f, ...);

extern void usage_multimic(void);
extern void *cmdl_malloc(const char *, char *, int, int);
#define CMDL_MALLOC(x) cmdl_malloc(__FUNCTION__, __FILE__, __LINE__,  x)

extern int get_skip_output_frames();

extern void add_input_files(char *name, int what);
extern void add_output_file(char *name, int ch);

extern int organize_inputs(char *__adjlen);
extern void organize_outputs(int force_wav, int force_pcm);

extern void organize_outputs_bli(int force_wav, int force_pcm);
extern void organize_outputs_bli1(int force_wav, int force_pcm);
extern void organize_outputs_blip(int force_wav, int force_pcm);
extern void organize_outputs_dlt(int force_wav, int force_pcm);
extern void organize_outputs_mux(int force_wav, int force_pcm, int ntx);
extern void organize_outputs_chn(int force_wav, int force_pcm, int ntx);
extern void organize_outputs_chz(int force_wav, int force_pcm, int ntx);


extern int read_wave_in(short *, short *);
extern void close_wave_in(void);
extern void write_wave_out(short *, short *, short *, short *);
extern int	get_sample_rate_global(void);
extern void set_sample_rate_global(int s);

#endif


