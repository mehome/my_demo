#ifndef __VCP_API_H__
#define __VCP_API_H__

/************************************************************************/
/*                                                                      */
/*                Alango Voice Communication Package (VCP)              */
/*                           generation 8                               */
/*                                                                      */
/************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) && (defined( _WINDOWS) || defined(WINFORMS))
__pragma(warning(disable : 4200))                /* Zero-sized array is really defined. For further purposes RVV */
#endif

#define VERSION_STRING      "Alango Voice Communication Package (VCP), Generation 8"
#define PROFILE_VERSION     52


#define NUM_MEM_REGIONS     1

/************************************************************************/
/*  profile item type. Normally 16 bits per item.                       */
/************************************************************************/
typedef short           pitem_t;            /* 16 bits */
typedef unsigned short  uitem_t;
typedef short           pcm_t;

/************************************************************************/
/*  VCP GENERAL PROFILE. This defines common VCP parameters.            */
/************************************************************************/
typedef struct profile_general_s {
    pitem_t         pver;                   /* VCP profile version */
    pitem_t         sr;                     /* Sampling frequency */
    pitem_t         lite;                   /* VCP build LITE or not */
    pitem_t         frlen;                  /* input frame length in _SAMPLES_:  computed as frame length in seconds times sampling frequency in Hertz */
    pitem_t         lpf;                    /* last processing frequency */
    /* input/output gains: */
    pitem_t         tx_ig;                  /* TX Input Gain */
    pitem_t         rx_ig;
    pitem_t         tx_og;
    pitem_t         rx_og;                  /* RX Output Gain */
    /* other */
    pitem_t         stg;                    /* Side tone gain. */ 
    pitem_t         rpd;                    /* AEC Reference to primary (mic) delay */
    /* virtual volume */
    pitem_t         refg_nom;               /* Rx reference signal nominal gain (external volume control), dB */
    pitem_t         refg_max;               /* Rx reference signal maximum gain, dB */
    pitem_t         refg_vvol_nom;          /* virtual volume coresponding to reference gain nominal, dB */
    pitem_t         refg_vvol;              /* current volume, dB */
    /* Global noise reduction settings */
    pitem_t         tx_gmnr;                /*Global Maximal Noise Reduction, dB*/
    pitem_t         rx_gmnr;                /*Global Maximal Noise Reduction, dB*/
    /* info */
    pitem_t         date;                   /* Days since 1st of Jan, 2000. Should be enough for about 180 Years (who wants to live forever?) */
    pitem_t         time;                   /* Minutes since 00:00 on the date above */        
    pitem_t         crc;                    /* CRC for profile consistency checking */ 
} vcp_profile_general_t;


/************************************************************************/
/* OUTPUT ARC/DRC1/DRC2                                                 */
/************************************************************************/
typedef struct profile_agc_s {
    pitem_t         bps;                    /* Bypass modes. bitfield. bit 0 - copy, bit 1 - disable */
    pitem_t         drc1_sat;               /* DRC1 signal saturation level or const gain  Q8 */
    pitem_t         drc1_sl;                /* DRC1 min signal level Q15 */
    pitem_t         drc1_nl;                /* DRC1 max noise level  Q15 */
    pitem_t         drc1_ng;                /* DRC1 noise gain  Q8 */
    pitem_t         drc2_sat;               /* DRC2 signal saturation level or const gain  Q8 */
    pitem_t         agc_mxg;                /* AGC max gain Q8 */
    pitem_t         agc_mxgl;               /* AGC max gain signal level  Q15 */
    pitem_t         agc_zgl;                /* AGC zero gain signal level Q15 */
    pitem_t         agc_rt;                 /* AGC release time (ms) for doubling the AGC gain on weak voice signal  Q0 */
    pitem_t         agc_lthr;               /* AGC signal level threshold Q15 */
    pitem_t         agc_vthr;               /* AGC vad level threshold Q15 */
    pitem_t         agc_ft;                 /* AGC fade time (ms) for halving the AGC gain on silent pause  Q0 */
} vcp_profile_agc_t;



/************************************************************************/
/* NOISE SUPPRESSOR -2 */
/************************************************************************/
typedef struct profile_ns2_s {
	pitem_t        ns2_bps;                  /* Bypass mode 0,1,2 */
	pitem_t        ns2_fpf;                  /* first processing frequency */
	pitem_t        ns2_lpf;                  /* last processing frequency */
	pitem_t        ns2_hrf;                  /* High resolution area end frequency */
	pitem_t        ns2_snsg;                 /* Stationary Noise Suppression wb gain base, dB < 0, (+ EQ) */
	pitem_t        ns2_ming;                 /* Stationary Noise Suppression wb min gain base, dB < 0, (+ EQ) */
	pitem_t        ns2_wbngg;                /* Wide band noise gate gain, dB */
	pitem_t        ns2_tc_thr;               /* Tone suppression minimal gain/Tone preservation threshold, dB */
	pitem_t        ns2_agcdg;                /* AGC depended min gain, dB */
	pitem_t        ns2_bnl_ming;             /* Minimal BNL gain, dB < 0 */
	pitem_t        ns2_bnl_maxd;             /* gain spectrum curvature factor, dB > 0 */
	pitem_t        ns2_bnl_hi_thr;           /* high threshold for suppression increase, dB < 0, (+ EQ) */
	pitem_t        ns2_bnl_hi_slope;         /* high threshold slope, lin Q15, 0.25-1.0 */
	pitem_t        ns2_bnl_low_thr;          /* low threshold for suppression decrease, dB < 0 */
	pitem_t        ns2_bnl_low_slope;        /* low threshold slope, lin Q15, 0.25-1.0 */
	pitem_t        ns2_snat;                 /* Stationary noise adaptation time */
	pitem_t        ns2_tnat;                 /* Transient noise adaptation time */
	pitem_t        ns2_spat;                 /* Speech adaptation time */
	pitem_t        ns2_ntf1;                 /* Low frequencies noise threshold of NS sensitivity */
	pitem_t        ns2_ntf2;                 /* High frequencies noise threshold of NS sensitivity */
	pitem_t        ns2_lfnf_ming;            /* Low frequency noise flattering minimal gain, dB */
	pitem_t        ns2_smooth;               /* Smoothness NS gain in time, lin Q15 */
	pitem_t        ns2_delay;                /* Additional algorithmic delay, frames */
        pitem_t        ns2_wnr_flag;             /* Wind Noise Reduction flag off = 0 / on = 1 (i.e.auto)*/
        pitem_t        ns2_wnr_str;              /* WNR strength 0.0...1.0 */
        pitem_t        ns2_wnr_matt;             /* WNR minimal attenuation [dB] */
        pitem_t        ns2_wnr_thr;              /* WNR threshold [dB] */
        pitem_t        ns2_wnr_it;               /* WNR increase time, ms */
        pitem_t        ns2_wnr_dt;               /* WNR decrease time, ms */
	pitem_t        ns2_cng_mode;             /* comfort noise generation work mode 0,1,2 */ 
	pitem_t        ns2_cng_cutg;             /* comfort noise generation gain cut level, lin Q15 */
} vcp_profile_ns2_t;


/************************************************************************/
/* SUBBAND ECHO CANCELLER & DUAL FILTER & ECHO SUPPRESSOR */
/************************************************************************/
typedef struct profile_af_s {
    pitem_t         af_bps;                 /* Bypass Adaptive filter block */
    pitem_t         af_udf;                 /* Upper duplex frequency */
    pitem_t         af_afuf;                /* Adaptive filter upper frequency for length alf1 */
    pitem_t         af_afathr;              /* signal level threshold for dynamic management of AF */
    pitem_t         af_affthr;              /* signal level threshold for AF filtering */ 
    pitem_t         af_bff;                 /* Bottom filter frequency */
    pitem_t         af_afcca;               /* Adaptive filter correlation controlled adaptation */
    pitem_t         af_flcfv;               /* fix or limited correlation function value */ 
    pitem_t         af_afst;                /* Adaptive filter stereo reference signal */
    pitem_t         af_afam;                /* AF adaptation mode, normal, sparse mode, simple adaptation */
    pitem_t         af_arf;                 /* Adaptive rate factor for adaptive filter */
    pitem_t         af_afah;                /* Adaptive filter activity hold flag */
    pitem_t         af_afl1;                /* Effective length of adaptive filter */
    pitem_t         af_tsf1;                /* Two stage adaptive filter for ALF1 */
    pitem_t         af_afl2;                /* Effective length of adaptive filter for high frequencies */
    pitem_t         af_tsf2;                /* Two stage adaptive filter for ALF2 */
    pitem_t         af_fost;                /* Output sample type (0 - min after two stage, 1 - last stage result) */
} vcp_profile_af_t;



/************************************************************************/
/* ECHO SUPPRESSOR */
/************************************************************************/
typedef struct profile_es_s {
    pitem_t         es_bps;                 /* Bypass echo suppression block */
    pitem_t         es_hdtime;              /* half duplex time */
    pitem_t         es_esthr;               /* Signal level threshold of ES */
    pitem_t         es_refthr;              /* Reference signal activity level threshold */
    pitem_t         es_udfthr;              /* Reference signal activity level threshold for bands above udf */
    pitem_t         es_utff;                /* UDF Transfer Function Factor */
    pitem_t         es_cn_flag;             /* comfort noise flag */
    pitem_t         es_cng;                 /* comfort noise gain */
    pitem_t         es_bct;                 /* alternative band cancellation threshold */
    pitem_t         es_bct_lt;              /* band cancellation lookahead time. */
    pitem_t         es_gictv;               /* global initial convergence target value */
    pitem_t         es_ictv;                /* initial convergence target value */
    pitem_t         es_bct_eq_flag;         /* allows individual BCT settings for each band */
    pitem_t         es_gct;                 /* Global cancellation threshold */
    pitem_t         es_obt;                 /* minimal allowed percent of open sub-bands */
    pitem_t         es_esothr;              /*  ES output low level signal threshold */
    pitem_t         es_psl;                 /* input saturation level (to close ES bands during Far End activity). */
    pitem_t         es_rrt;                 /* room reverberation time */
    pitem_t         es_nld_flag;            /* non-linear Distortions (ND), flag (0-off,1-on) */
    pitem_t         es_nld_ndc;             /* non-linear distortions factor */
    pitem_t         es_nld_ndcs;            /* ndc decrease step in case of reference signal level decrease */
    pitem_t         es_nld_ndcr;            /* ndc release factor [ms] */
    pitem_t         es_nld_rsndc;           /* special ndc value for over-saturated Reference */
    pitem_t         es_nld_rsl;             /* reference signal saturation level, dB */
    pitem_t         es_nld_ndf1;            /* bottom frequency producing non-linear distortions [Hz] */
    pitem_t         es_nld_ndf2;            /* top frequency producing non-linear distortions [Hz] */
    pitem_t         es_nld_ndgf1;           /* bottom non-linear distortion generation frequency [Hz] */
    pitem_t         es_nld_ndgf2;           /* top non-linear distortion generation frequency [Hz] */
    pitem_t         es_inamp_sel;           /* ES should compare mic1 input vs. AFout or ADMout (before NLP) or ADMout (after NLP) */
    pitem_t         es_af_sel;              /* decide which AF should be taken into account while computing the AF attenuation */
    pitem_t         es_agc_ragib;           /* Ref. activity gain increase ban, When "on" - AGC gain cannot increase during ref. activity, When "off" - AGC gain can raise irrespective to the ref. activity */
} vcp_profile_es_t;

/************************************************************************/
/* FREQENCY SHIFTER */
/************************************************************************/
typedef struct profile_fs_s {
    pitem_t         fs_max;                 /* Max frequency shift for shift proportional to band freq, Hz */
    pitem_t         fs_const;               /* const frequency shift for all freq bands, Hz */
} vcp_profile_fs_t;

/************************************************************************/
/* DC REMOVAL */
/************************************************************************/
typedef struct profile_dcr_s {
    pitem_t         bps;                    /* processing flags */
    pitem_t         fcut;                   /* Cut-off frequency for LPF */
} vcp_profile_dcr_t;

/************************************************************************/
/* EQUALIZER. defined as an array on 'pitem'                            */
/************************************************************************/
#define EQ_MAX_ENTR         32
#define EQ_TYPE_AMPL        0   // lin Q11:-72dB (0x0),-24dB (0x0081), 0dB (0x0800), +24 dB (0x7eca)
#define EQ_TYPE_CMPLX       1
#define EQ_TYPE_MB          2   // mB (not implemented yet)
typedef struct profile_eq_s {
    pitem_t         num;                    /* number of entries */
    pitem_t         type;                   /* equalizer type (anyway padding is needed :)) */
    pitem_t         eq[EQ_MAX_ENTR];
} vcp_profile_eq_t;

/************************************************************************/
/* DTGC                                                                 */
/************************************************************************/
typedef struct profile_dtgc_s {
    pitem_t         dtgc_bps;               /* Bypass echo suppr block */
    pitem_t         dtgc_att;               /* Rx gain attenuation value, dB */
    pitem_t         dtgc_lim;               /* DRC_SAT attenuation value, dB */
    pitem_t         dtgc_dt;                /* Gain decrease time, ms */
    pitem_t         dtgc_it;                /* Gain increase time, ms */
    pitem_t         dtgc_ht;                /* Attenuated gain hold time, ms */
    pitem_t         dtgc_hp;                /* DTGC high pass gain, dB */
    pitem_t         dtgc_thr;               /* DTGC threshold upon ampl. on RX, dB */
    pitem_t         dtgc_vthr;              /* DTGC VAD threshold upon TX NS vad, dB */
} vcp_profile_dtgc_t;


/************************************************************************/
/* MIXER parameters                                                     */
/************************************************************************/
#define MIXER_MAX_ENTR    32
typedef struct profile_mixer_s {
    pitem_t         num;                        /* number of elements in mixer's array */
    pitem_t         inout;                      /* bits 15-8 num of ins, 7-0 num of outs */
    pitem_t         data[MIXER_MAX_ENTR];       /*  mixer's data matrix */
} vcp_profile_mixer_t;

/************************************************************************/
/* AUX data parameters                                                     */
/************************************************************************/
#define AUX_DATA_ENTR    32
typedef struct profile_auxdata_s {
    pitem_t         num;                        /* number of elements in aux's struct (not counting num itself) */
    pitem_t         pn;                         /* profile ID number */
    pitem_t         aux_int16[16];              /* really aux field */
    char            aux_char[16];               /* for comment */
    pitem_t         inout;                      /* bits 15-8 num of tx, 7-0 num of rx */
    pitem_t         data[AUX_DATA_ENTR];        /* gains data array */
} vcp_profile_auxdata_t;

/************************************************************************/
/* DEBUG parameters                                                     */
/************************************************************************/
typedef struct profile_debug_s {
    pitem_t         txin;                   /* input/output modes for all channels  0 - normal */
    pitem_t         txinfreq;
    pitem_t         txinampl;
    pitem_t         txindecay;
    pitem_t         txinlenp;
    pitem_t         txout;                  /* 1 - zero */
    pitem_t         txoutfreq;
    pitem_t         txoutampl;
    pitem_t         txoutdecay;
    pitem_t         txoutlenp;
    pitem_t         rxin;                   /* 2 - sine */
    pitem_t         rxinfreq;
    pitem_t         rxinampl;
    pitem_t         rxindecay;
    pitem_t         rxinlenp;
    pitem_t         rxout;                  /* 3 - noise, 4 - sweep, etc */ 
    pitem_t         rxoutfreq;
    pitem_t         rxoutampl;
    pitem_t         rxoutdecay;
    pitem_t         rxoutlenp;

    pitem_t         txisot;                 /* TX input overload threshold */
    pitem_t         txosl;                  /* On TX overload 'output signal length' */
    pitem_t         txosa;                  /*                'output signal amplitude' */
    pitem_t         txosf;                  /*                'output signal frequency' */

    pitem_t         rxisot;                 /* RX input overload threshold */
    pitem_t         rxosl;                  /* On RX overload 'output signal length' */
    pitem_t         rxosa;                  /*                'output signal amplitude' */
    pitem_t         rxosf;                  /*                'output signal frequency' */
} vcp_profile_debug_t;



/************************************************************************/
/*            ---- VCP logger parameters array ----                     */
/************************************************************************/
#define LOG_TAGS_SIZE            16              /* size in 16-bit words */
typedef struct profile_log_s {
    pitem_t                 log[LOG_TAGS_SIZE];
} vcp_profile_log_t;


/************************************************************************ /
/*            ---- Unstructured user data array ----                    * /
/************************************************************************ /
#define USERDATA_SIZE        16
#define USERDATA_TEXT        1
#define USERDATA_HEX        0
typedef struct profile_userdata_s {
    pitem_t                 len;
    pitem_t                 type;
    pitem_t                 data[USERDATA_SIZE];
}vcp_profile_userdata_t;   */

/************************************************************************/
/*            ---- ADM data array ----                    */
/************************************************************************/
typedef struct profile_adm_s {
	pitem_t         adm_bp;             /* bypass mode: 0-normal, 1-processing with output overwriting, 2-copy only */
	pitem_t         adm_fcm;            /* Frequency compensation type */
	pitem_t         adm_fcw;            /* Frequency compensation (FC) weight (strength) */
	pitem_t         adm_mal_lf;         /* minimal adaptation level, low-frequency */
	pitem_t         adm_mal_hf;         /* minimal adaptation level, high-frequency */
	pitem_t         adm_mal_lfa;        /* Parameter for Variable MAL: minimal adaptation level, low-frequency amplitude in freq = adm_mal_lf */
	pitem_t         adm_mal_hfa;        /* Parameter for Variable MAL: minimal adaptation level, high-frequency amplitude in freq = adm_mal_hf */
	pitem_t         adm_mal_ml;         /* Parameter for Variable MAL: minimal adaptation level, minimal level */
	pitem_t         adm_dist;           /* Distance between the microphones in mm.Min = 5, Max = 1000, No Def.value. */
	pitem_t         adm_nomtol;         /* nominal sensitivity tolerance in dB */
	pitem_t         adm_maxtol;         /* parameter for Check_Channels algorithm - maximal sensitivity tolerance i.e. Switching threshold in dB */
	pitem_t         adm_ftf;            /* lowest reliable frequency (legacy "FTB") parameter */
	pitem_t         adm_lpf;            /* last processing frequency. Upper border for  all processing in Hz */
	pitem_t         adm_emsf;           /* emergency mic switch flag for Check_Channels. 0-fast switching is OFF; 1-fast switching is ON */
	pitem_t         adm_im_uf;          /* First parameter for Differential-Integral scheme switching. Integral mode (IM) upper frequency */
	pitem_t         adm_im_sw;          /* Control flag: 0-Diff-Integr.scheme switching is ON; 1-Switching is OFF:Diff.scheme; 2-Switching is OFF:Integr.scheme */
	pitem_t         adm_im_ot;          /* integral mode threshold offset from top: 'adm_im_ot' must be great than 'adm_im_ob' */
	pitem_t         adm_im_ob;          /* integral mode threshold offset from bottom */
    pitem_t         adm_im_ht;          /* D&S hold time, range: [0,5000] ms */
    pitem_t         adm_im_ct;          /* Noise confirmation time, range: [0,1024] ms, set 1024 ms to switch off this mechanism */
	pitem_t         adm_arfn;           /* Adaptation rate for noise */
	pitem_t         adm_arfe;           /* Adaptation rate for echo */
	pitem_t         adm_aa;             /* Acceptance angle (in Degrees) */
	pitem_t         adm_nlpg;           /* Minimal gain for "Classical" NLP below Alias in dB ("suppression strength") */
	pitem_t         adm_ucnlpg;         /* Minimal gain for Zelinski NLP ("suppression strength") */
	pitem_t         adm_lim;            /* [0...96] */
	pitem_t         adm_wnr_agr;        /* noise reduction strength. Min = 0 (no reduction), Max = 1 (max reduction) */
	pitem_t         adm_wnr_uf;         /* upper border for wind noise suppression in Hz [1,4000] */
    pitem_t         adm_sppe;           /* separate Polar Pattern during Echo */
	pitem_t         adm_faee;           /* Freeze Amplitude Estimations on Echo (freeze of WNR and mic switching by amplitude during Ref Activity */
} vcp_profile_adm_t;
/************************************************************************/
/* VCP profile as a set of pointers to sub-profiles.                    */
/************************************************************************/
typedef struct profile_s {
   vcp_profile_general_t       *p_gen;                  /* general profile */
   vcp_profile_ns2_t           *p_tx_ns2;               /* TX (mic) noise suppressor */
   vcp_profile_eq_t            *p_tx_ns2_snsg_eq;       /* TX (mic) noise suppressor-2 SNS equalizer*/
   vcp_profile_eq_t            *p_tx_ns2_bnlh_eq;       /* TX (mic) noise suppressor-2 BNL equalizer*/
   vcp_profile_eq_t            *p_tx_ns2_ming_eq;       /* TX (mic) noise suppressor-2 MIN gain equalizer*/
   vcp_profile_fs_t            *p_tx_fs;                /* TX frequency shifter */
   vcp_profile_eq_t            *p_tx_eq;                /* TX equalizer */
   vcp_profile_agc_t           *p_tx_agc;               /* TX output agc / drc1 / drc2 */
   vcp_profile_dcr_t           *p_tx_dcr;               /* TX DC removal */
   vcp_profile_dcr_t           *p_rx_dcr;               /* RX DC removal */
   vcp_profile_ns2_t           *p_rx_ns2;               /* RX (spk) noise suppressor */
   vcp_profile_eq_t            *p_rx_ns2_snsg_eq;       /* RX (spk) noise suppressor-2 SNS equalizer*/
   vcp_profile_eq_t            *p_rx_ns2_bnlh_eq;       /* RX (spk) noise suppressor-2 BNL equalizer*/
   vcp_profile_eq_t            *p_rx_ns2_ming_eq;       /* RX (spk) noise suppressor-2 MIN gain equalizer*/
   vcp_profile_fs_t            *p_rx_fs;                /* RX frequency shifter */
   vcp_profile_af_t            *p_tx_af;                /* TX sub-band echo canceller */
   vcp_profile_es_t            *p_tx_es;                /* TX echo suppressor */
   vcp_profile_eq_t            *p_tx_es_bct_eq;         /* BCT equalizer */
   vcp_profile_eq_t            *p_tx_es_gct_eq;         /* GCT equalizer */
   vcp_profile_eq_t            *p_rx_eq;                /* RX equalizer */
   vcp_profile_agc_t           *p_rx_agc;               /* RX output agc / drc1 / drc2 */
   vcp_profile_dtgc_t          *p_rx_dtgc;              /* DTGC */
   vcp_profile_mixer_t         *p_tx_mixer;             /* TX mixer */
   vcp_profile_auxdata_t       *p_auxdata;              /* Aux data */ 
   vcp_profile_adm_t           *p_adm;                  /* ADM */
   vcp_profile_debug_t         *p_deb;                  /* debug modes */
   vcp_profile_log_t           *p_log;                  /* logger's capabilities */
}vcp_profile_t;


/************************************************************************/
/*  MEMORY regions                                                      */
/************************************************************************/
typedef struct mem_reg_s
{
    void        *mem;                           /* memory provided */
    int         size;                           /* overall size of the region in bytes. */
} mem_reg_t;

/************************************************************************/
/*  ERRORS and codes                                                    */
/************************************************************************/
typedef struct err_s {
    short err;                                    /* error's id */
    short pid;                                    /* error profile's ID */
    short memb;                                   /* member number */
} err_t;





#undef ERR_NO_ERROR            
#undef ERR_OUT_OF_RANGE        
#undef ERR_BLK_NOT_IMPLEMENTED 
#undef ERR_NULL_POINTER        
#undef ERR_OBJ_CORRUPTED       
#undef ERR_DEMO_EXPIRED        
#undef ERR_INVALID_VALUE       
#undef ERR_INVALID_COMB        
#undef ERR_INVALID_CRC         
#undef ERR_NOT_ENOUGH_MEMORY   
#undef ERR_BUILD_NOT_CORRECT   
#undef ERR_STEREO_EC_DISABLED  
#undef ERR_ES_NLD_DISABLED    
#undef ERR_AF_NORMAL_ADAPTATION
#undef ERR_UNKNOWN             

#define ERR_NO_ERROR                0        /* no errors at all */
#define ERR_OUT_OF_RANGE            1        /* parameter is out of range */
#define ERR_BLK_NOT_IMPLEMENTED     2        /* block is not implemented */
#define ERR_NULL_POINTER            3        /* wrong parameter */
#define ERR_OBJ_CORRUPTED           4        /* object override happened in vcp_process */
#define ERR_DEMO_EXPIRED            5        /* Demo interval ended */
#define ERR_INVALID_VALUE           6        /* just invalid */
#define ERR_INVALID_COMB            7        /* invalid value combination */
#define ERR_INVALID_CRC             8        /* invalid CRC */
#define ERR_NOT_ENOUGH_MEMORY       9        /* not enough memory */
#define ERR_BUILD_NOT_CORRECT       10       /* Build and profile are not matched Lite and not Lite */
#define ERR_STEREO_EC_DISABLED      11       /* Stereo EC is not allowed in current build */
#define ERR_ES_NLD_DISABLED         12       /* NLD is not allowed in current build */
#define ERR_AF_NORMAL_ADAPTATION    13       /* only sparse or simplefied adaptation is allowed in current build */
#define ERR_AF_ES_ENABLE            14       /* ES can't work without AF */
#define ERR_AF_DTGC_ENABLE          15       /* DTGC can't work without AF */

#define ERR_UNKNOWN                 -1

//#if defined(__TEXT_ERRORS_PRINT)
    /* check 'main()' for details */
    static char *__text_error[] = {
        "no error",
        "parameter is out of range",
        "block is not implemented",
        "parameter is a null pointer",
        "object has been corrupted",
        "demo license has expired",
        "invalid parameter's value",
        "invalid parameters combination",
        "invalid CRC",
        "not enough memory allocated",
        "build and profile do not match",
        "stereo EC is not allowed",
        "nld is not allowed",
        "normal EC adaptation is not allowed",
        "ES can't work without AF",
        "normal and sparse EC adaptation is not allowed"
    };
//#endif



/************************************************************************/
/* BASIC ROUTINES:                                                      */
/************************************************************************/

/************************************************************************
FUNCTION:
vcp_init() -- initializes VCP object with given profile and memory area(s).
ARGS:
p       - pointer to actual profile.
reg     - pointer to memory region structure(s).
RETURNS: integer values, corresponding to the following errors:
ERR_NO_ERROR,
ERR_INVALID_CRC,
ERR_INVALID_VALUE
For precise error detection use vcp_init_debug()
OTHER:
Memory region(s) must be allocated prior vcp_init() call,
size field of 'reg' must be filled with allocated size.
Once inited, neither 'reg' not its fields can be altered.
Profile 'p' can be destroyed after exit.
************************************************************************/
extern int vcp_init(vcp_profile_t *p, mem_reg_t *reg);



/************************************************************************
FUNCTION:
vcp_process() -- does main VCP processing
ARGS:
in/out  - pointers to in/out streams
reg     - pointer to memory region structure(s).
RETURNS:
ERR_OBJ_CORRUPTED,
ERR_DEMO_EXPIRED,
ERR_UNKNOWN,
ERR_NO_ERROR
OTHER:
input buffers must be filled with data with size defined in
general profile field 'frlen'. Output sizes will be the same.
Input buffers must not be modified during call.
************************************************************************/
extern err_t vcp_process(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *txout, pcm_t *rxout);
extern err_t vcp_process_rx(mem_reg_t *reg, pcm_t *rxin, pcm_t *rxout);
extern err_t vcp_process_tx(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *txout);


/************************************************************************/
/* SEVICE ROUTINES AND DEFINITIONS                                      */
/* VCP SEVICE LIBRARY REQUIRED                                          */
/************************************************************************/


/************************************************************************
FUNCTION:
vcp_process_debug() -- same as above with some extra debug
functionality and paranoid checks.
ARGS:
in/out  - pointers to in/out streams
reg     - pointer to memory region structure(s).
RETURNS:
all errors possible
************************************************************************/
extern err_t vcp_process_debug(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *txout, pcm_t *rxout);

extern err_t vcp_process_rx_debug(mem_reg_t *reg, pcm_t *rxin, pcm_t *rxout);
extern err_t vcp_process_tx_debug(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *txout);


/************************************************************************
FUNCTION:
vcp_get_mem_size() -- computes required memory.
ARGS:
p       - pointer to actual profile.
reg     - pointer to memory region structure(s).
mem     - memory allocated with size returned by vcp_get_hook_size()
RETURNS:
error code
OTHER:
upon return memory region(s) will have 'size'
field reflecting memory required.
************************************************************************/
extern err_t vcp_get_mem_size(vcp_profile_t *p, mem_reg_t *reg, void *mem);


/************************************************************************
FUNCTION:
vcp_get_hook_size()     -- returns the size of memory required
for low stack use in vcp_get_mem_size().
RETURNS:
size required
************************************************************************/
extern int vcp_get_hook_size(void);


/************************************************************************
FUNCTION:
vcp_check_profile() -- Check parameters.
ARGS:
profile_t *p - pointer to actual profile
RETURNS:
error code.
************************************************************************/
extern err_t vcp_check_profile(vcp_profile_t *p);


/************************************************************************
FUNCTION:
vcp_init_debug()        -- Same as vcp_init with extra
parameters checking.
RETURNS:
error code
************************************************************************/
extern err_t vcp_init_debug(vcp_profile_t *p, mem_reg_t *reg);


/************************************************************************
FUNCTION:
logger_get_buf_n_size()   -- Extracts pointer to logger data buffer and its size in bytes
bytes
ARGS:
void *hook              -- void pointer to vcp object
RETURNS:
pointer to log buffer
size of the logger data buffer
************************************************************************/
extern void* vcp_logger_get_buf_n_size(void *hook, int *out_size);


/************************************************************************
FUNCTION:
vcp_logger_enabled()    --  Informs about the logger availability
ARGS:
void *vobj              --  pointer to VCP object
RETURNS:
0 if disabled, non-0 if enabled
************************************************************************/
extern int vcp_logger_enabled(void *vobj);

/************************************************************************
FUNCTION:
vcp_set_param()    --  Sets a set of parameter in VCP run-time mode
ARGS:
void *vobj         --  pointer to VCP object
pcm_t param      --  the parameter. see definitions below
pcm_t value       --  the parameter value that will be set, the value must be in hexadecimal form (NOT the physical value).
RETURNS:
1 if ok, 0 if parameter cannot be set
If returned value is equal to -9 then corresponding block is absent in
profile 
If returned value is equal to -1 then the parameter value cannot be set
(out of range)
************************************************************************/
extern int vcp_set_param(mem_reg_t * reg, pitem_t param, pitem_t value);

/************************************************************************
FUNCTION:
vcp_get_param()    --  Gets parameter value in VCP run-time mode
ARGS:
void *vobj         --  pointer to VCP object
pcm_t param      --  the parameter. see definitions below
pcm_t *value     --  the parameter value that will be read
ATTN! pcm_t type is used!

RETURNS:
There are must be TWO calls to the procedure.
The first one points witch parameter must be read and returns 0,
The second one returns 1 and the parameter value.
If returned value is equal to -9 then corresponding block is absent in
profile 
************************************************************************/

extern int vcp_get_param(mem_reg_t *reg, pitem_t param, pitem_t *value);
/* below are listed parameters (pitem_t param) for functions vcp_set_param() and 
vcp_set_param  */
#define PARAM_TX_IG               1
#define PARAM_RX_IG               2
#define PARAM_TX_OG               3
#define PARAM_RX_OG               4
#define PARAM_TX_NS_SNS           14
#define PARAM_TX_NS_BNL           15
#define PARAM_TX_DRC1_SAT         26
#define PARAM_TX_DRC2_SAT         27
#define PARAM_TX_AGC_MXG          28 
#define PARAM_RX_NS_SNS           39
#define PARAM_RX_NS_BNL           310
#define PARAM_TX_AF_REFG_NOM      411
#define PARAM_TX_AF_REFG_MAX      412
#define PARAM_TX_AF_VVOL          413
#define PARAM_TX_AF_VVOL_NOM      414
#define PARAM_TX_ES_ESTHR         515
#define PARAM_TX_ES_BCT           516
#define PARAM_TX_ES_GCT           517
#define PARAM_TX_ES_ESOTHR        518
#define PARAM_TX_ES_PSL           519
#define PARAM_RX_DRC1_SAT         620
#define PARAM_RX_DRC2_SAT         621
#define PARAM_RX_AGC_MXG          622


#ifdef __cplusplus
};
#endif

#endif /*__VCP_API_H__ */
