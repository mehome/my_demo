/*********************************************************************
* ��Ȩ����(C)2011-2018, �����������Ƽ����޹�˾
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

/* SQE��ʼ������ */
extern short MNPCSqeInit(int sample_rate, int aec_param, int ns_param, int agc_param);

/* SQE������  */
extern short MNPCSqeProc(short *in_near, short *in_far, short *out);

#ifdef __cplusplus
}
#endif

#endif




