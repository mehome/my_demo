#ifndef __VCP_POSTAPI_H__
#define __VCP_POSTAPI_H__

#include "vcp-api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef pitem_t     pitem_t;
typedef uitem_t     uitem_t;


/************************************************************************/
/* BYPASS MODES                                                         */
/************************************************************************/
#define BYPASS_NORMAL       0
#define BYPASS_COPYANDPROC  1
#define BYPASS_COPY         2

#define AF1_NORMAL_AF2_BYPASS_COPY        3 // allocate memory only for AF1, the arr_af[0] and arr_af[1] shared the same pointer
#define AF1_NORMAL_AF2_BYPASS_COPYANDPROC 4 // aloocate memory for both AF
#define AF2_NORMAL_AF1_BYPASS_COPY        5 // allocate memory only for AF2, the arr_af[0] and arr_af[1] shared the same pointer
#define AF2_NORMAL_AF1_BYPASS_COPYANDPROC 6 // aloocate memory for both AF


/************************************************************************/
/* CONNECTION MODES                                                     */
/************************************************************************/
#define CONNECT_MODE_NORMAL              0
#define CONNECT_MODE_ZERO                1
#define CONNECT_MODE_SINE                2
#define CONNECT_MODE_NOISE               3
#define CONNECT_MODE_SWEEP               4
#define CONNECT_MODE_SQUARE              5
#define CONNECT_MODE_SQUARE_SWEEP        6
#define CONNECT_MODE_BYPASS              7


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define __PACKAGE_NAME vcp_

#define __PACKAGE_PASTER(x,y) x ## y
#define __PACKAGE_EVALUATOR(x,y)  __PACKAGE_PASTER(x,y)

#define PROFILE_TYPE(x)     __PACKAGE_EVALUATOR(__PACKAGE_NAME, __PACKAGE_EVALUATOR(profile_, x))

#ifndef offsetof
#ifdef  ARCH_64_BITS
#define offsetof(s,m)   ((size_t)( (ptrdiff_t)&(((s *)0)->m) ))
#else
#define offsetof(s,m)   ((size_t)&(((s *)0)->m))
#endif
#endif

    // for service routines only.
    // you may define these two in order to get 
    // pretty error printing in calling routine.
    // VCP LIB does not use all of these defines!
#if defined(__EXTRA_CMDL_SERVICE)
#if !defined(__EXTRA_SRC_SERV_SRC)
#define _CSER(a,b,m,c)        {"vcp_profile_" #a ,#b,#m,c, sizeof(PROFILE_TYPE(a)) / sizeof(pitem_t)}
#define _XSER(a,b,m,c)        {"vcp_profile_" #a,#b,#m,c, 0}
//#define _USER(a,b,m,c)        {#a,#b,#m,c, -1}   not in use
#else
#define _CSER(a,b,c,d)        {sizeof(PROFILE_TYPE(a)) / sizeof(pitem_t)}
#define _XSER(a,b,c,d)        {0}
//#define _USER(a,b,c,d)        {-1}   //not in use
#endif
    static const struct cmds_s {
#if !defined(__EXTRA_SRC_SERV_SRC)
        const char *type;
        const char *memb;
        const char *mime;
        const char *com;
#endif
        const int wsize;
    } cmds[] = {
        _CSER(general_t, p_gen, general, "General profile"),
        _CSER(ns2_t, p_tx_ns2, ns2, "TX NS2"),
        /*Attn! adding blocks to the profile struct, don't forget to add them here! */
        _XSER(eq_t,p_tx_ns2_snsg_eq, eq, "TX SNS EQ" ),
        _XSER(eq_t,p_tx_ns2_bnlh_eq, eq, "TX BNLH EQ" ),
        _XSER(eq_t,p_tx_ns2_ming_eq, eq, "TX MING EQ" ),
        _CSER(fs_t, p_tx_fs, fs, "TX FS"),
        _XSER(eq_t, p_tx_eq, eq, "TX EQ"),               // UNSIZED!!!
        _CSER(agc_t, p_tx_agc, agc, "TX AGC/DRC"),
        _CSER(dcr_t, p_tx_dcr, dcr, "TX DC removal"),
        _CSER(dcr_t, p_rx_dcr, dcr, "RX DC removal"),
        _CSER(ns2_t, p_rx_ns2, ns2, "RX NS2"),
        _XSER(eq_t,p_rx_ns2_snsg_eq, eq, "RX SNS EQ" ),
        _XSER(eq_t,p_rx_ns2_bnlh_eq, eq, "RX BNLH EQ" ),
        _XSER(eq_t,p_rx_ns2_ming_eq, eq, "RX MING EQ" ),
        _CSER(fs_t, p_rx_fs, fs, "RX FS"),
        _CSER(af_t, p_tx_af, af, "TX AF"),
        _CSER(es_t, p_tx_es, es, "TX ES"),
        _XSER(eq_t, p_tx_es_bct_eq, eq, "BCT EQ"),              // UNSIZED!!!
        _XSER(eq_t, p_tx_es_gct_eq, eq, "GCT EQ"),              // UNSIZED!!!
        _XSER(eq_t, p_rx_eq, eq, "RX EQ"),               // UNSIZED!!!
        _CSER(agc_t, p_rx_agc, agc, "RX AGC/DRC"),
        _CSER(dtgc_t, p_rx_dtgc, agc, "RX DTGC"),
        _XSER(mixer_t, p_tx_mixer, tmix, "TX MIXER"),
        _XSER(auxdata_t, p_auxdata, aux, "AUX DATA"),
		_CSER(adm_t, p_adm, adm, "ADM"),

        _CSER(debug_t, p_deb, debug, "Debugs"),
        _CSER(log_t, p_log, logger, "Logger")
        //, _USER(userdata_t, p_udata, uudata, "Unstructured User Data")
    };
#endif

#define offp(x)        (offsetof(PROFILE_TYPE(t),x) / sizeof(void *))

#if defined(__EXTRA_CMDL_SERVICE)
#define PROFILE_ID_GENERAL          offp(p_gen)
#define PROFILE_ID_TX_NS2           offp(p_tx_ns2)    
#define PROFILE_ID_TX_NS2_SNS_EQ   offp(p_tx_ns2_snsg_eq)    
#define PROFILE_ID_TX_NS2_BNLH_EQ   offp(p_tx_ns2_bnlh_eq)    
#define PROFILE_ID_TX_NS2_MING_EQ   offp(p_tx_ns2_ming_eq)    
#define PROFILE_ID_TX_FS            offp(p_tx_fs)
#define PROFILE_ID_TX_EQ1           offp(p_tx_eq)
#define PROFILE_ID_TX_AGC           offp(p_tx_agc)
#define PROFILE_ID_TX_DCR           offp(p_tx_dcr)
#define PROFILE_ID_RX_DCR           offp(p_rx_dcr)
#define PROFILE_ID_RX_NS2           offp(p_rx_ns2)
#define PROFILE_ID_RX_NS2_SNS_EQ   offp(p_rx_ns2_snsg_eq)    
#define PROFILE_ID_RX_NS2_BNLH_EQ   offp(p_rx_ns2_bnlh_eq)    
#define PROFILE_ID_RX_NS2_MING_EQ   offp(p_rx_ns2_ming_eq)    
#define PROFILE_ID_RX_FS            offp(p_rx_fs)
#define PROFILE_ID_TX_AF            offp(p_tx_af)
#define PROFILE_ID_TX_ES            offp(p_tx_es)
#define PROFILE_ID_TX_BCT_EQ        offp(p_tx_es_bct_eq)
#define PROFILE_ID_TX_GCT_EQ        offp(p_tx_es_gct_eq)
#define PROFILE_ID_RX_EQ1           offp(p_rx_eq)
#define PROFILE_ID_RX_AGC           offp(p_rx_agc)
#define PROFILE_ID_RX_DTGC          offp(p_rx_dtgc)
#define PROFILE_ID_TX_MIXER         offp(p_tx_mixer)
#define PROFILE_ID_AUXDATA          offp(p_auxdata)
#define PROFILE_ID_ADM              offp(p_adm)
#define PROFILE_ID_DEBUG            offp(p_deb)
#define PROFILE_ID_LOGGER           offp(p_log)
//#define PROFILE_ID_UDATA            offp(p_udata)
//#define PROFILE_ID_MAX              ((PROFILE_ID_UDATA +1))
#define PROFILE_ID_MAX              ((PROFILE_ID_LOGGER +1))
#endif



#ifdef __cplusplus
};
#endif


#endif //__VCP_POSTAPI_H__

