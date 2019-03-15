#ifndef __SPF_POSTAPI_H__
#define __SPF_POSTAPI_H__


#include "vepug-api.h"

#ifdef __cplusplus
extern "C" {      
#endif            


/******** ENUM NAMES DEFINITIONS *********************************/

/******** From venum FLAG *********/
#define FLAG_OFF                        0        // off
#define FLAG_ON                         1        // on

/******** From venum FSR *********/
#define FSR_8000                        8000     // 8000
#define FSR_16000                       16000    // 16000
#define FSR_24000                       24000    // 24000
#define FSR_32000                       32000    // 32000

/******** From venum BPMODES *********/
#define BYPASS_NORMAL                   0        // Normal
#define BYPASS_AND_PROC                 1        // Bypass & Proc
#define BYPASS_BYPASS                   2        // Bypass

/******** From venum UGBF_PPROC_TYPE *********/
#define UGBF_NONE                       0        // none
#define UGBF_SIMPLE                     1        // simple
#define UGBF_ULTIMATE                   2        // ultimate
#define UGBF_THIRD                      3        // third

/******** From venum CNG_MODES *********/
#define MODE0                           0        // mode0
#define MODE1                           1        // mode1
#define MODE2                           1        // mode2

/******** From venum ADM_BPMODES *********/
#define ADM_BYPASS_NORMAL               0        // adm normal
#define ADM_BYPASS_COPY_AND_PROC_MIC0   1        // adm bypass copy and proc mic0
#define ADM_BYPASS_COPY_MIC0            2        // adm bypass copy mic0
#define ADM_BYPASS_COPY_AND_PROC_MIC1   3        // adm bypass copy and proc mic1
#define ADM_BYPASS_COPY_MIC1            4        // adm bypass copy mic1


/******** GENERAL DEFINITIONS *********************************/
#define __PACKAGE_NAME vepug_
#define __PACKAGE_PASTER(x,y) x ## y
#define __PACKAGE_EVALUATOR(x,y)  __PACKAGE_PASTER(x,y)
#define PROFILE_TYPE(x)           __PACKAGE_EVALUATOR(__PACKAGE_NAME, __PACKAGE_EVALUATOR(profile_, x))

#ifndef offsetof                                                      
#ifdef  ARCH_64_BITS                                                  
#define offsetof(s,m)   ((size_t)( (ptrdiff_t)&(((s *)0)->m) ))       
#else                                                                 
#define offsetof(s,m)   ((size_t)&(((s *)0)->m))                      
#endif                                                                
#endif                                                                
#define offp(x)        (offsetof(PROFILE_TYPE(t), x) / sizeof(void *))
#define offm(x,n)      ((offsetof(PROFILE_TYPE(t), x) + n * sizeof(void *)) / sizeof(void *)) // make MVS happy


/*** PROFILE STRUCTURE MEMBERS */

/**********************************************************
* GENERAL_PROTO
**********************************************************/
#define GENERAL_DUMP_MIXER_BIT     1 // Bip position for dump mixer flag 
#define GENERAL_STEREO_BIT         0 // Bit position of stereo flag

typedef struct vepug_profile_general_s {
    pitem_t flags;            /* General Flags [-32767 .. 32767], 0 */

    /*  === General settings [PROFILE_TYPE(general_t) p_gen] ===  */
    pitem_t sr;               /* Sampling frequency, Hz */
    pitem_t frlen;            /* Processing frame length in samples (CAN BE 8ms ONLY!) [64 .. 256], 128 */
    pitem_t nmic;             /* Number of microphones [2 .. 16], 4 */
    pitem_t nbeams;           /* Number of beams [1 .. 16], 1 */
    pitem_t stereo;           /* Enable/disable stereo RX [0 .. 1], 0 */
    pitem_t psl;              /* AEC primary (mic) saturation level [-96 .. 0], 0, dB */

    /* -- Digital gains [PROFILE_TYPE(general_t) p_gen] -- */
    pitem_t tx_ig;            /* Mic digital input gain [-72 .. 24], 0, dB */
    pitem_t rx_ig;            /* AEC Reference digital input gain applied right at input [-72 .. 24], 0, dB */
    pitem_t tx_og;            /* VEP output digital gain applied right at the output [-72 .. 24], 0, dB */

    /* -- Other important [PROFILE_TYPE(general_t) p_gen] -- */
    pitem_t lpf;              /* Last processing frequency [0 .. 16000], 4000, Hz */
    pitem_t rpd;              /* Reference to primary delay [-1000 .. 1000], 0, ms */

    /* Virtual Volumes */
    pitem_t refg_nom;         /* Nominal gain for the AEC Reference signal [-96 .. 40], 0, dB */
    pitem_t refg_max;         /* Maximal gain for the AEC Reference signal [-96 .. 40], 40, dB */
    pitem_t refg_vvol_nom;    /* Virtual volume level coresponding to the nominal reference gain [-96 .. 40], 0, dB */
    pitem_t refg_vvol;        /*  [-96 .. 40], 0, dB */
} vepug_profile_general_t;

/**********************************************************
* BF_PROTO
**********************************************************/
#define MAX_MARDNS_GROUPS     120 // MAX number of D&S groups => combin(2,16) == 120
#define MAX_MARDNS_MICS       16 // MAX number of microphones
#define MAX_MARDNS_BEAMS      16 // Max number of beams

typedef struct vepug_profile_bf_s {
    pitem_t flags;                      /*  [-32767 .. 32767], 0 */
    pitem_t xyz[MAX_MARDNS_MICS][3];    /* Mic coordinates [-32767 .. 32767], 0, mm*100 */
    pitem_t tns;                        /* Total Number of streams [0 .. 32767], 0 */
} vepug_profile_bf_t;

/**********************************************************
* BEAMPROP_PROTO
**********************************************************/
#define BEAM_DLANE_BIT            4 // Double Lane Beam (ADM'ed) beam
#define BEAM_WITH_NLP             5 // Beam with NLP
#define BEAM_FRONT_INCL           6 // Front BEAM is inclined from vector's dir
#define BEAM_REAR_INCL            7 // Rear BEAM is inclined from vector's dir
#define BEAM_GRP_FRONT_MANUAL     8 // Manual groups for front set
#define BEAM_GRP_REAR_MANUAL      9 // Manual groups for rear set
#define BEAM_OMNI                 10 // Special beam - omnidirectional
#define BEAM_NIU                  11 // Beam Not In Use flag
#define BEAM_SLAVERY              12 // Rear set is a slave to front one

typedef struct vepug_profile_beamprop_s {
    pitem_t flags;        /* Verious flags [0 .. 32767], 0 */
    pitem_t xf;           /* Crossfade top/bot as bit-nubble + 1 (0 - no xf) [-32767 .. 32767], 0 */
    pitem_t front_set;    /* Front Set [-32767 .. 32767], 0 */
    pitem_t rear_set;     /* Rear Set [-32768 .. 32767], 0 */
    pitem_t aa[2];        /* Acceptance Angle (front/rear) [-180 .. 180], 0, deg */
    pitem_t ngrp;         /* Combined number of rear/front groups incl. alias ones. [-32768 .. 32767], 0 */
    pitem_t front[3];     /* Front plane definitions [-32768 .. 32767], 0, mm*100 */
    pitem_t rear[3];      /* Rear Plane [-32768 .. 32767], 0, mm*100 */
    pitem_t fstart;       /* Beam's start frequency [0 .. 32767], 0, Hz */
    pitem_t fstop;        /* Stop Frequency [0 .. 32767], 0, Hz */
    pitem_t fdir[3];      /* Front (if any) beam direction [-32768 .. 32767], 0, mm*100 */
    pitem_t rdir[3];      /* Rear (if any) beam direction [-32768 .. 32767], 0, mm*100 */
    pitem_t sstp[4];      /* Shear Shift Theta Phi [-32767 .. 32767], 0, mm*100 */
} vepug_profile_beamprop_t;

/**********************************************************
* DCR_PROTO
**********************************************************/

typedef struct vepug_profile_dcr_s {
    pitem_t bps;     /* Processing modes */
    pitem_t fcut;    /* TX DC removal filter cut-off freq. [10 .. 500], 50, Hz */
} vepug_profile_dcr_t;

/**********************************************************
* AF_PROTO
**********************************************************/
#define AF_AFST_BIT      0 // Stereo flag
#define AF_AFCCA_BIT     1 // Adaptive filter correlation-controlled adaptation
#define AF_FOST_BIT      2 // Filter output sample type 
#define AF_AFAH_BIT      3 // Adaptive filter activity hold 
#define AF_TSF1_BIT      4 // Two stage adaptive filtering of AFL1
#define AF_TSF2_BIT      5 // Two stage adaptive filtering of AFL2

typedef struct vepug_profile_af_s {
    pitem_t flags;     /* Various flags [0 .. 32767], 0 */

    /* Bypass modes */
    pitem_t bps;       /* Bypass modes */

    /* General AF settings */
    pitem_t bff;       /* Bottom filter frequency. ERROR: AF_$1_BFF must be less than AF_$1_AFUF. [0 .. 3000], 0, Hz */
    pitem_t udf;       /* Upper Duplex Frequency. ERROR: Wrong UDF value. It must be less than FSF/2. Also AFUF must be less than UDF. [1000 .. 16000], 8000, Hz */
    pitem_t afuf;      /* adaptive filter upper frequency. ERROR: AFUF must be between AF_$1_BFF and AF_$1_UDF. [1000 .. 16000], 3000, Hz */
    pitem_t afathr;    /* Adaptive filter adaptation threshold [-92 .. 0], -60, dB */
    pitem_t affthr;    /* adaptive filter filtering threshold [-96 .. 0], -60, dB */

    /* AF length and adaptation */
    pitem_t arf;       /* Adaptation Rate Factor [0 .. 1], 0.7 */
    pitem_t flcfv;     /* Filter limit correlation function value [0 .. 1], 0.5 */
    pitem_t afl1;      /* adaptive filter length [5 .. 1000], 150, ms */
    pitem_t afl2;      /* adaptive filter length [5 .. 1000], 100, ms */
} vepug_profile_af_t;

/**********************************************************
* NS2_PROTO
**********************************************************/

typedef struct vepug_profile_ns2_s {
    /* Bypass modes */
    pitem_t bps;              /* $1 NS bypass mode */

    /* NS2 General settings */
    pitem_t fpf;              /* First (bottom) processed frequency [0 .. 500], 0, Hz */
    pitem_t hrf;              /* High resolution upper frequency [2000 .. 16000], 3500, Hz */
    pitem_t delay;            /* NS look ahead time in frames [0 .. 8], 0, Frms */
    pitem_t lpf;              /* Last (top) processed frequency [2000 .. 16000], 8000, Hz */

    /* Main noise reduction gain settings */
    pitem_t snsg;             /* Stationary noise suppression gain [-96 .. 0], -9, dB */
    pitem_t wbngg;            /* Wideband noise gate gain [-10 .. 0], -3, dB */
    pitem_t lfnf_ming;        /* Low frequency noise flattering minimal gain [-12 .. 0], -6, dB */
    pitem_t ming;             /* Minimal allowed total noise suppression gain [-96 .. 0], -96, dB */
    pitem_t agcdg;            /* Minimal allowed AGC-depended NR gain auto-correction [-9 .. 0], -9, dB */

    /* Noise-level dependent settings */
    pitem_t bnl_hi_thr;       /* BNL high threshold (subband NR gain increases above it) [-190 .. 0], 0, dB */
    pitem_t bnl_hi_slope;     /* BNL high threshold slope [0 .. 1], 0.75 */
    pitem_t bnl_low_thr;      /* BNL low threshold (subband NR gain is 0 dB below it) [-190 .. 0], -96, dB */
    pitem_t bnl_low_slope;    /* BNL low threshold slope [0 .. 1], 0.5 */
    pitem_t bnl_ming;         /* Minimal allowed NR gain correction by BNL [-96 .. 0], -12, dB */
    pitem_t bnl_maxd;         /* BNL spectrum curvature factor (lower -> more flat) [0 .. 42], 6, dB */

    /* Adaptation and sensitivity */
    pitem_t snat;             /* Stationary noise adaptation time [50 .. 5000], 700, ms */
    pitem_t tnat;             /* Transient noise adaptation time [0 .. 5000], 100, ms */
    pitem_t spat;             /* Speech noise adaptation time [2000 .. 30000], 3000, ms */
    pitem_t ntf1;             /* Sensitivity to signal at low frequencies [-1 .. 1], 0 */
    pitem_t ntf2;             /* Sensitivity to signal at high frequencies [-1 .. 1], 0 */
    pitem_t smooth;           /* NS smoothness [0 .. 1], 0.5 */

    /* Wind noise reduction */
    pitem_t wnr_ena;          /* Wind Noise Reduction enable [0 .. 1], 0 */
    pitem_t wnr_mode;         /* Wind noise reduction work mode [0 .. 0], 0 */
    pitem_t wnr_adatt;        /* Additional gain [-96 .. 0], -6, dB */
    pitem_t wnr_rms;          /* absolute rms threshold [-96 .. 0], -6, dB */
    pitem_t wnr_tup;          /* Time up [0 .. 0], 0, ms */
    pitem_t wnr_tdn;          /* Time down [0 .. 0], 0, ms */

    /* Auxiliary */
    pitem_t cng_cutg;         /* Comfort noise generation gain cut level [0 .. 1], 0.9 */
    pitem_t cng_mode;         /* Comfort noise generation work mode */
    pitem_t tc_thr;           /* Tone control threshold (> 0 - suppression, < 0 - attenuation) [-80 .. 80], 0, dB */
} vepug_profile_ns2_t;

/**********************************************************
* ADM_PROTO
**********************************************************/
#define ADM_EMSF_BIT     0 // Emergency mic (mic failure) switch flag 
#define ADM_SPPE_BIT     1 // Separate polar patterns for echo/no-echo flag
#define ADM_FAEE_BIT     2 // Freeze amplitude estimations during echo flag

typedef struct vepug_profile_adm_s {
    pitem_t flags;      /* Various flags [0 .. 32767], 0 */

    /* ADM Bypass modes */
    pitem_t bps;        /* Bypass Modes */

    /* Main ADM Settings */
    pitem_t dist;       /* Distance between microphones [5 .. 200], 20, mm */
    pitem_t nomtol;     /* Nominal mic sensitivity tolerance [-12 .. 0], -3, dB */
    pitem_t maxtol;     /* Maximal mic sensitivity tolerance [0 .. 30], 30, dB */

    /* Adaptation settings */
    pitem_t aa;         /* Acceptance Angle  [0 .. 180], 80, deg */
    pitem_t ftf;        /* Lowest reliable frequency (legacy FTB)  [0 .. 1500], 150, Hz */
    pitem_t arfn;       /* Adaptation Rate for Noise. [0 .. 1], 0.6 */
    pitem_t arfe;       /* Adaptation Rate for Echo [0 .. 1], 0.6 */
    pitem_t lpf;        /* Last processing frequency [0 .. 16000], 8000, Hz */

    /* Frequency compensation setitngs */
    pitem_t fcm;        /* Frequency compensation mode. ERROR: This value cannot be set for FCM parameter! [10 .. 13], 13 */
    pitem_t fcw;        /* Frequency compensation weight [0 .. 1], 1 */

    /* Non-linear processing */
    pitem_t nlpg;       /* Minimal gain for NLP  [-96 .. 0], 0, dB */
    pitem_t ucnlpg;     /* Minimal gain for NLP ??? [-96 .. 0], 0, dB */
    pitem_t lim;        /* Limiter, Permitted exceeding for output  [0 .. 96], 0, dB */

    /* Minimal Adaptation level settings */
    pitem_t mal_lf;     /* Manimal adaptation level, low frequency [0 .. 16000], 500, Hz */
    pitem_t mal_hf;     /* Minimal adaptation level, high frequency [0 .. 16000], 1000, Hz */
    pitem_t mal_lfa;    /* Minimal adaptation level, low-frequency amplitude [-130 .. 0], -100, dB */
    pitem_t mal_hfa;    /* Minimal adaptation level, high-frequency amplitude [-130 .. 0], -110, dB */
    pitem_t mal_ml;     /* Minimal adaptation level, minimal level [-130 .. 0], -120, dB */

    /* Integral mode settings */
    pitem_t im_sw;      /* Integral Mode Switching (integer 0,1,2) [0 .. 2], 1 */
    pitem_t im_uf;      /* Integral mode (IM) upper frequency. [0 .. 16000], 750, Hz */
    pitem_t im_ot;      /* integral mode threshold offset from top. [-96 .. 96], -6, dB */
    pitem_t im_ob;      /* Integral Mode Threshold Offset from Bottom. [-96 .. 96], -12, dB */
    pitem_t im_ht;      /* Integral Mode Hold Time. [0 .. 5000], 2000, ms */
    pitem_t im_ct;      /* Noise confirmation time  [0 .. 1024], 400, ms */

    /* Wind noise settings */
    pitem_t wnr_agr;    /* Wind noise reduction aggressiveness [0 .. 1], 0 */
    pitem_t wnr_uf;     /* Wind noise reduction upper frequency  [0 .. 16000], 3000, Hz */

    /* Echo control */
} vepug_profile_adm_t;

/**********************************************************
* BLI_PROTO
**********************************************************/

typedef struct vepug_profile_bli_s {
    pitem_t bf;     /* Bottom Frequency for the amplitude estimation [0 .. 2000], 250, Hz */
    pitem_t tf;     /* Top Frequency for the amplitude estimation [1000 .. 8000], 6500, Hz */
    pitem_t att;    /* Amplitude Track Time [0 .. 5000], 1600, ms */
    pitem_t aat;    /* Instant Amplitude Averaging Time (usually, length of trigger keyword). [0 .. 5000], 600, ms */
    pitem_t bfw;    /* Bottom Frequency Weight [0 .. 1], 0.2 */
    pitem_t tfw;    /* Top Frequency Weight [0 .. 1], 1 */
    pitem_t af;     /* Amplitude Attack factor [2 .. 2000], 10, mB */
    pitem_t rf;     /* Amplitude Release factor [2 .. 2000], 10, mB */
    pitem_t bat;    /* Backgound amplitude avaraging time [8 .. 1500], 300, ms */
} vepug_profile_bli_t;

/**********************************************************
* MUX_PROTO
**********************************************************/

typedef struct vepug_profile_mux_s {
    pitem_t fade;           /* Channels mux crossfade time [16 .. 32767], 100, ms */
    pitem_t dl;             /* Channels mux delay line length [16 .. 32767], 250, ms */
    pitem_t hyster;         /* Channels switch hysteresis [0 .. 2000], 100, mB */
    pitem_t passthrough;    /* Channels mux pass through channel number. ERROR: Channel's number must be less than number of beams! Set value to Max beam number! [0 .. 16], 0 */
    pitem_t pd;             /* Channel mux peak detector delay [0 .. 10000], 800, ms */
} vepug_profile_mux_t;

/**********************************************************
* NLP_PROTO
**********************************************************/

typedef struct vepug_profile_bfnlp_s {
    pitem_t flags;     /* flags [-32767 .. 32767], 0 */
    pitem_t nlpg;      /* NLP gain [-96 .. 0], 0, dB */
    pitem_t nlphf;     /* High Frequency NLP factor [-96 .. 0], 0, dB */
    pitem_t nlplf;     /* Low Frequency NLP factor [-96 .. 0], 0, dB */
    pitem_t hff[3];    /* NLP's HF's frequencies [0 .. 32767], 0, Hz */
    pitem_t lff[3];    /* NLP's LF's frequencies. ERROR: Condition   0 <= f0 < f1 < f2 <= Fs/2   isn't met or block is not enabled!. [0 .. 32767], 0, Hz */
} vepug_profile_bfnlp_t;

/**********************************************************
* STORE_PROTO
**********************************************************/
#define STORE_SIZE     1 // Zero if compiler supports it, 1 othervize

typedef struct vepug_profile_store_s {
    pitem_t size;                /*  [0 .. 32767], 0 */
    pitem_t data[STORE_SIZE];    /* data [-32767 .. 32767], 0 */
} vepug_profile_store_t;

/**********************************************************
* EQ_PROTO
**********************************************************/
#define EQ_MAX_ENTR        32 // Zero if compiler supports it. Othervise one.
#define FREQUENCY_STEP     62.5 // Bandgap, Hz

typedef struct vepug_profile_eq_s {
    pitem_t num;                /* number EQ entries [0 .. 32767], 0 */
    pitem_t type;               /* Type [0 .. 32767], 0 */
    pitem_t eq[EQ_MAX_ENTR];    /*  [-72 .. 24], 0, dB */
} vepug_profile_eq_t;


/*** PROFILE STRUCTURE DESCRIPTION */
typedef struct vepug_profile_s {
    vepug_profile_general_t  *p_gen;            /* General parameters */
    vepug_profile_bf_t       *p_bf;             /* Beamformer definitions */
    vepug_profile_store_t    *p_beams;          /* Beam's properties */
    vepug_profile_store_t    *p_phases;         /* BF phases */
    vepug_profile_store_t    *p_pindices;       /* Zero-sized array holder */
    vepug_profile_store_t    *p_grps;           /* BF groups */
    vepug_profile_store_t    *p_lgbt;           /* BF last groups bins */
    vepug_profile_bfnlp_t    *p_bfnlp[16];      /* Beamformers NLP proto */
    vepug_profile_dcr_t      *p_tx_dcr;         /* TX DC removal definitions */
    vepug_profile_dcr_t      *p_rx_dcr;         /* RX DC removal definitions */
    vepug_profile_af_t       *p_tx_af;          /* Adaptive filter definitions as they seen */
    vepug_profile_adm_t      *p_tx_adm[16];     /* ADM definitions as they seen */
    vepug_profile_ns2_t      *p_tx_ns2[16];     /* NS2 definitions as they seen */
    vepug_profile_bli_t      *p_bli;            /* Beam Level Index Block */
    vepug_profile_mux_t      *p_mux;            /* Output Multiplexer definitions  */
    vepug_profile_eq_t       *p_eq[16];         /* Equalizer's definitions as they seen */
} vepug_profile_t;

typedef vepug_profile_t profile_t;


#define PROFILE_ID_GENERAL      offp(p_gen)
#define PROFILE_ID_BF           offp(p_bf)
#define PROFILE_ID_BEAMS        offp(p_beams)
#define PROFILE_ID_PHASES       offp(p_phases)
#define PROFILE_ID_PINDICES     offp(p_pindices)
#define PROFILE_ID_GRPS         offp(p_grps)
#define PROFILE_ID_LGBT         offp(p_lgbt)
#define PROFILE_ID_BFNLP        offp(p_bfnlp)
#define PROFILE_ID_BFNLP_0      offm(p_bfnlp,0)
#define PROFILE_ID_BFNLP_1      offm(p_bfnlp,1)
#define PROFILE_ID_BFNLP_2      offm(p_bfnlp,2)
#define PROFILE_ID_BFNLP_3      offm(p_bfnlp,3)
#define PROFILE_ID_BFNLP_4      offm(p_bfnlp,4)
#define PROFILE_ID_BFNLP_5      offm(p_bfnlp,5)
#define PROFILE_ID_BFNLP_6      offm(p_bfnlp,6)
#define PROFILE_ID_BFNLP_7      offm(p_bfnlp,7)
#define PROFILE_ID_BFNLP_8      offm(p_bfnlp,8)
#define PROFILE_ID_BFNLP_9      offm(p_bfnlp,9)
#define PROFILE_ID_BFNLP_10     offm(p_bfnlp,10)
#define PROFILE_ID_BFNLP_11     offm(p_bfnlp,11)
#define PROFILE_ID_BFNLP_12     offm(p_bfnlp,12)
#define PROFILE_ID_BFNLP_13     offm(p_bfnlp,13)
#define PROFILE_ID_BFNLP_14     offm(p_bfnlp,14)
#define PROFILE_ID_BFNLP_15     offm(p_bfnlp,15)
#define PROFILE_ID_TX_DCR       offp(p_tx_dcr)
#define PROFILE_ID_RX_DCR       offp(p_rx_dcr)
#define PROFILE_ID_TX_AF        offp(p_tx_af)
#define PROFILE_ID_TX_ADM       offp(p_tx_adm)
#define PROFILE_ID_TX_ADM_0     offm(p_tx_adm,0)
#define PROFILE_ID_TX_ADM_1     offm(p_tx_adm,1)
#define PROFILE_ID_TX_ADM_2     offm(p_tx_adm,2)
#define PROFILE_ID_TX_ADM_3     offm(p_tx_adm,3)
#define PROFILE_ID_TX_ADM_4     offm(p_tx_adm,4)
#define PROFILE_ID_TX_ADM_5     offm(p_tx_adm,5)
#define PROFILE_ID_TX_ADM_6     offm(p_tx_adm,6)
#define PROFILE_ID_TX_ADM_7     offm(p_tx_adm,7)
#define PROFILE_ID_TX_ADM_8     offm(p_tx_adm,8)
#define PROFILE_ID_TX_ADM_9     offm(p_tx_adm,9)
#define PROFILE_ID_TX_ADM_10    offm(p_tx_adm,10)
#define PROFILE_ID_TX_ADM_11    offm(p_tx_adm,11)
#define PROFILE_ID_TX_ADM_12    offm(p_tx_adm,12)
#define PROFILE_ID_TX_ADM_13    offm(p_tx_adm,13)
#define PROFILE_ID_TX_ADM_14    offm(p_tx_adm,14)
#define PROFILE_ID_TX_ADM_15    offm(p_tx_adm,15)
#define PROFILE_ID_TX_NS2       offp(p_tx_ns2)
#define PROFILE_ID_TX_NS2_0     offm(p_tx_ns2,0)
#define PROFILE_ID_TX_NS2_1     offm(p_tx_ns2,1)
#define PROFILE_ID_TX_NS2_2     offm(p_tx_ns2,2)
#define PROFILE_ID_TX_NS2_3     offm(p_tx_ns2,3)
#define PROFILE_ID_TX_NS2_4     offm(p_tx_ns2,4)
#define PROFILE_ID_TX_NS2_5     offm(p_tx_ns2,5)
#define PROFILE_ID_TX_NS2_6     offm(p_tx_ns2,6)
#define PROFILE_ID_TX_NS2_7     offm(p_tx_ns2,7)
#define PROFILE_ID_TX_NS2_8     offm(p_tx_ns2,8)
#define PROFILE_ID_TX_NS2_9     offm(p_tx_ns2,9)
#define PROFILE_ID_TX_NS2_10    offm(p_tx_ns2,10)
#define PROFILE_ID_TX_NS2_11    offm(p_tx_ns2,11)
#define PROFILE_ID_TX_NS2_12    offm(p_tx_ns2,12)
#define PROFILE_ID_TX_NS2_13    offm(p_tx_ns2,13)
#define PROFILE_ID_TX_NS2_14    offm(p_tx_ns2,14)
#define PROFILE_ID_TX_NS2_15    offm(p_tx_ns2,15)
#define PROFILE_ID_BLI          offp(p_bli)
#define PROFILE_ID_MUX          offp(p_mux)
#define PROFILE_ID_EQ           offp(p_eq)
#define PROFILE_ID_EQ_0         offm(p_eq,0)
#define PROFILE_ID_EQ_1         offm(p_eq,1)
#define PROFILE_ID_EQ_2         offm(p_eq,2)
#define PROFILE_ID_EQ_3         offm(p_eq,3)
#define PROFILE_ID_EQ_4         offm(p_eq,4)
#define PROFILE_ID_EQ_5         offm(p_eq,5)
#define PROFILE_ID_EQ_6         offm(p_eq,6)
#define PROFILE_ID_EQ_7         offm(p_eq,7)
#define PROFILE_ID_EQ_8         offm(p_eq,8)
#define PROFILE_ID_EQ_9         offm(p_eq,9)
#define PROFILE_ID_EQ_10        offm(p_eq,10)
#define PROFILE_ID_EQ_11        offm(p_eq,11)
#define PROFILE_ID_EQ_12        offm(p_eq,12)
#define PROFILE_ID_EQ_13        offm(p_eq,13)
#define PROFILE_ID_EQ_14        offm(p_eq,14)
#define PROFILE_ID_EQ_15        offm(p_eq,15)
#define PROFILE_ID_MAX    ((PROFILE_ID_EQ_15) + 1)


#if defined(READER_NEEDS_SIZES)

static const pitem_t __bmsizes[] = {
 (sizeof(vepug_profile_general_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bf_t) / sizeof(pitem_t)),
-1,
-1,
-1,
-1,
-1,
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bfnlp_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_dcr_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_dcr_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_af_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_adm_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_ns2_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_bli_t) / sizeof(pitem_t)),
 (sizeof(vepug_profile_mux_t) / sizeof(pitem_t)),
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
-1,
};

#endif


/******** THE END ********************************************/

#ifdef __cplusplus
}                
#endif            


#endif // __SPF_POSTAPI_H__
