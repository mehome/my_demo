/* Copyright (C) 2017 SKV
Written by Shuai Nie */
/**
*  @file skv_preprocess.h
*  @brief SKV preprocessor.
*/

#ifndef SKV_PREPROCESS_H
#define SKV_PREPROCESS_H

typedef unsigned char       BYTE;

#ifdef __cplusplus
extern "C" {
#endif

	#define INFO_RIGHT 1				// Succeed initialize SKV preprocessor
	#define INFO_PARAM_ERROR 2			// Cann't support the parameter of audio that you input
	#define INFO_EXCEEDDATE_ERROR 3		// Exceed the limited date
	#define INFO_UNKNOW_ERROR 4         // Unknown error
	#define FRAME_SIZE 256
    #define FRAME_SIZE_PBNLMS 64

	/** Initialize the SKV preprocessor. You MUST initialize the SKV preprocessor before using this speech frond-end processing.
	* @param sampling_rate Sampling rate used for the input, MUST be 16000.
	* @param bits_persample Bites per sample used for the input. MUST be 16
	* @param num_src_channel Number of channel of input audio. MUSR >= 1
	* @param num_ref_channel Number of channel of refference signals. MUSR >= 1
	* @return preprocessor state (INFO_RIGHT 1, INFO_PARAM_ERROR 2, INFO_EXCEEDDATE_ERROR 3, INFO_UNKNOW_ERROR 4)
	*/
	int skv_preprocess_state_init(int sampling_rate, int bits_persample, int num_src_channel, int num_ref_channel);

	/** Destroys a SKV preprocessor
	*/
	void skv_preprocess_state_destroy();

	/** Preprocess a buffer of audio data 
	* @param in Audio data (Short) to be processed in PCM format, its size of MUST be 256 * (num_src_channel + num_ref_channel) * N samples, N = 1, 2, 3, ... n
	* @param in_size Lenght of the array 'in' that you want to process
	* @param left_ref_channel Channel Index of left_ref_channel in array 'in'
	* @param right_ref_channel Channel Index of right_ref_channel in array 'in'
	* @return Size of processed audio data.
	*/
	int skv_preprocess_short(short * in, int in_size, int left_ref_channel, int right_ref_channel);

	/** Preprocess a buffer of audio data 
	* @param in Audio data (BYTE) to be processed in PCM format, its size of MUST be 256 * 2 * (num_src_channel + num_ref_channel) * N BYTE
	* @param in_size Lenght of the array 'in' that you want to process
	* @param left_ref_channel Channel Index of left_ref_channel in array 'in'
	* @param right_ref_channel Channel Index of right_ref_channel in array 'in'
	* @return Size of processed audio data.
	*/
	int skv_preprocess_byte(BYTE * in,short*out, int in_size, int left_ref_channel, int right_ref_channel, int bigOrlittle);
#ifdef __cplusplus
}
#endif

/** @}*/
#endif
