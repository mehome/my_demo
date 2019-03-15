/*********************************************************************
* 版权所有(C)2011-2018, 深圳市松西科技有限公司
**********************************************************************/
/*
 *===================================================================
 *  SOSEA SQE SoftWare module
 *===================================================================
 */
 
 
#ifndef       ___MNPC_SQE_H___
#define       ___MNPC_SQE_H___

#ifdef __cplusplus
extern "C" {
#endif

/* SQE初始化函数 */
extern short MNPCSqeInit(int sample_rate, int aec_param, int ns_param, int agc_param);

/* SQE处理函数  */
extern short MNPCSqeProc(short *in_near, short *in_far, short *out);

#ifdef __cplusplus
}
#endif

#endif




