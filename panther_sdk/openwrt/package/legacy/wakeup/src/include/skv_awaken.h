/* Copyright (C) 2017 SKV
Written by Shuai Nie */
/**
*  @file wakeup.h
*  @brief .
*/

#ifndef SKV_WAKEUP_H
#define SKV_WAKEUP_H


#ifdef __cplusplus
extern "C" {
#endif

	#define INFO_RIGHT 1				// Succeed initialize SKV preprocessor
	#define INFO_PARAM_ERROR 2			// Cann't support the parameter of audio that you input
	#define INFO_EXCEEDDATE_ERROR 3		// Exceed the limited date
	#define INFO_UNKNOW_ERROR 4         // Unknown error
	#define FRAME_SIZE 256


	/** Initialize the SKV wakeupor. You MUST initialize the SKV wakeupor before using this speech frond-end processing.
	* @param sampling_rate Sampling rate used for the input, MUST be 16000.
	* @param bits_persample Bites per sample used for the input. MUST be 16
	* @return wakeupor state (INFO_RIGHT 1, INFO_PARAM_ERROR 2, INFO_EXCEEDDATE_ERROR 3, INFO_UNKNOW_ERROR 4)
	*/
	int skv_awaken_init(int sampling_rate,int bits_persample);
	/** Destroys a SKV wakeupor
	*/
	void skv_awaken_destroy();

	/** Preprocess a buffer of audio data 
	* @param in Audio data (Short) to be processed in PCM format, its size of MUST be 256 * (num_src_channel + num_ref_channel) * N samples, N = 1, 2, 3, ... n
	* @param in_size Lenght of the array  must be 256
	* @return Size of processed audio data.
	*/
	int skv_awaken_run(short * in, int in_size);

#ifdef __cplusplus
}
#endif

/** @}*/
#endif
