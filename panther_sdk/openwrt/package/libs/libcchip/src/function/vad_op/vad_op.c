#include <platform.h>
#include "vad_op.h"
static int chunkUnitCompare(vad_args_t *args,unsigned int idx){	
	short *sample=(char *)args->sample + idx * args->unitBytes;
	long long sum = 0, sum2 = 0;
	unsigned int  i=0;
	for(i = 0; i < args->unitBytes/2; i++) {
		sum += (sample[i] * sample[i]);
		sum2 += args->vadThreshold;
	}
	return sum > sum2 ?1:0;
}
static int isChunkSpeaking(vad_args_t *args){
	unsigned int overTimes=0;
	unsigned int unitCount=args->bytes/args->unitBytes;
	if(unitCount<1){
		err("unitBytes less then chunkBytes!");
	}
	unsigned int i=0;
	for(i=0;i<unitCount;i++){
		overTimes+=chunkUnitCompare(args,i);
	}
	//inf("3_overTimes=%d",overTimes);
	return overTimes >= (unitCount*0.1+1)?1:0;
}
vad_status_t get_vad_status(vad_args_t *args){
	if(VAD_CHECK_IDEL==args->savedStatus){
		if (isChunkSpeaking(args)){
			args->confirmCount++;
			if(args->confirmCount >= args->startedConfirmCount){
				args->confirmCount=0;
				return args->savedStatus=VAD_CHECK_STARTED;
			}		
		}else{
			args->confirmCount=0;
		}	
		return args->savedStatus=VAD_CHECK_IDEL;		
	}
	if(VAD_CHECK_STARTED==args->savedStatus){
		if (!isChunkSpeaking(args)){
			args->confirmCount++;
		}
		return args->savedStatus=VAD_CHECK_SPEAKING;
	}
	if(VAD_CHECK_SPEAKING==args->savedStatus){
		if (!isChunkSpeaking(args)){
			args->confirmCount++;
			if(args->confirmCount >= args->finishedConfirmCount){
				args->confirmCount=0;
				return args->savedStatus=VAD_CHECK_FINISHED;
			}		
		}else{
			args->confirmCount=0;
		}
		return args->savedStatus=VAD_CHECK_SPEAKING;	
	}
	return args->savedStatus;	
}