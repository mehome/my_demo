//#include <sndfile.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <fcntl.h> 
#include <math.h>
#include <limits.h>
#include "resample_441_to_16.h"
#include "../debug.h"

#define	BUFFER_LEN		4096	/*-(1<<16)-*/
#define BUFFER_SIZE 441*12
#define INPUT_INTERVAL 12
#define OUTPUT_INTERVAL 33
#define POINTS_TO_COMPUTE 348 
#define BUFFER_HEADER ((POINTS_TO_COMPUTE/INPUT_INTERVAL))

#define MUL_FACTOR	 0x1000
static const short FIRCoef[] =
{
	1,	1,	1,	2,	2,	3,	3,	3,	3,	4,	3,	3,	3,	2,	1,	0,
	-1, -2, -3, -5, -6, -7, -8, -8, -8, -8, -7, -6, -4, -2, 0,	3,
	5,	8,	10, 12, 13, 13, 13, 12, 9,	7,	3,	-1, -5, -10,-14,
	-18 ,-21	,-23	,-24	,-23	,-22	,-18	,-14	,-8 ,-2 ,
	6	,13 ,20 ,27 ,32 ,35 ,37 ,37 ,34 ,29 ,22 ,14 ,3	,-8 ,-19	,-31	,
	-41 ,-49	,-55	,-58	,-58	,-54	,-47	,-36	,-23	,-7 ,
	9	,27 ,44 ,59 ,72 ,81 ,86 ,85 ,80 ,70 ,54 ,35 ,12 ,-13	,-38	,-63	,
	-86 ,-104	,-118	,-125	,-125	,-118	,-103	,-80	,-52	,-19	,
	17	,54 ,90 ,123	,150	,170	,181	,181	,171	,149	,117	,
	76	,28 ,-24	,-78	,-131	,-180	,-220	,-249	,-266	,-267	,-252	,
	-221	,-174	,-114	,-43	,36 ,118	,198	,273	,335	,383	,410	,
	414 ,394	,347	,276	,181	,67 ,-61	,-197	,-334	,-464	,-578	,
	-669	,-729	,-750	,-727	,-655	,-532	,-358	,-135	,134	,443	,
	784 ,1147	,1522	,1896	,2257	,2595	,2896	,3151	,3350	,3488	,3557	,
	3557	,3488	,3350	,3151	,2896	,2595	,2257	,1896	,1522	,1147	,784	,
	443 ,134	,-135	,-358	,-532	,-655	,-727	,-750	,-729	,-669	,-578	,
	-464	,-334	,-197	,-61	,67 ,181	,276	,347	,394	,414	,410	,
	383 ,335	,273	,198	,118	,36 ,-43	,-114	,-174	,-221	,-252	,
	-267	,-266	,-249	,-220	,-180	,-131	,-78	,-24	,28 ,76 ,117	,149	,
	171 ,181	,181	,170	,150	,123	,90 ,54 ,17 ,-19	,-52	,-80	,-103	,
	-118	,-125	,-125	,-118	,-104	,-86	,-63	,-38	,-13	,12 ,35 ,54 ,70 ,
	80	,85 ,86 ,81 ,72 ,59 ,44 ,27 ,9	,-7 ,-23	,-36	,-47	,-54	,-58	,-58	,
	-55 ,-49	,-41	,-31	,-19	,-8 ,3	,14 ,22 ,29 ,34 ,37 ,37 ,35 ,32 ,27 ,20 ,13 ,
	6	,-2 ,-8 ,-14	,-18	,-22	,-23	,-24	,-23	,-21	,-18	,-14	,-10	,
	-5	,-1 ,3	,7	,9	,12 ,13 ,13 ,13 ,12 ,10 ,8	,5	,3	,0	,-2 ,-4 ,-6 ,-7 ,-8 ,-8 ,
	-8	,-8 ,-7 ,-6 ,-5 ,-3 ,-2 ,-1 ,0	,1	,2	,3	,3	,3	,4	,3	,3	,3	,3	,2	,2	,
	1	,1	,1,
};

#if 1//def MUSIC_LITTLE_ENDIAN
// 小端
void ConvertFreq(short *source, long srcIdx, short *target, long tgtIdx,long coefIdx)
{
    //long j;
 		// long double temp;
  	int j;
  	int temp;
  	int temp1;
	
	  for (temp = 0,j=0; (coefIdx+ INPUT_INTERVAL*j)<POINTS_TO_COMPUTE; j++) {
	  	temp += source[srcIdx+j]*FIRCoef[coefIdx + INPUT_INTERVAL*j];
	  	if(tgtIdx==958 && srcIdx==2641) {
	  	}
	  }
	  temp /= MUL_FACTOR;
	  if (temp > 32767) temp = 32767;
	  else if (temp < -32767) temp = -32767;
	  target[tgtIdx] = temp;
		temp1 = (int) temp;
		if((int)temp1)
		  	temp1 = 0;
	  if(tgtIdx==958){
	  	
		}	  	
}

#else
// 大端
typedef signed short int16;
typedef unsigned short uint16;
#define le2he(a) ((int16)((uint16)(((((uint16)a)&0xff)<<8)|((((uint16)a)>>8)&0xff))))
void ConvertFreq(short *source, long srcIdx, short *target, long tgtIdx,long coefIdx)
{
    //long j;
 		// long double temp;
  	int j;
  	int temp;
  	int temp1;
	  for (temp = 0,j=0; (coefIdx+ INPUT_INTERVAL*j)<POINTS_TO_COMPUTE; j++) {
	  	temp += le2he(source[srcIdx+j])*FIRCoef[coefIdx + INPUT_INTERVAL*j];
	  	if(tgtIdx==958 && srcIdx==2641) {
	  	}
	  }
	  temp /= MUL_FACTOR;
	  if (temp > 32767) temp = 32767;
	  else if (temp < -32767) temp = -32767;
	  target[tgtIdx] = le2he(temp);
		temp1 = (int) temp;
		if((int)temp1)
		  	temp1 = 0;
	  if(tgtIdx== 958){

		}
}
#endif

//main (int argc, char *argv [])
void resample_441_to_16(char * infile,char * outfile)
{	
  FILE *inFile, *outFile;
  WAVE_FILE_HEADER fileHeader;
  unsigned long readSize;
  short *inFileData,*outFileData,*inFileData_tmp;
  // long i,j;
  int i,j;
  int source_type;
	//if((inFile = fopen(argv [1], "rb"))== NULL) return;
 	//if((outFile = fopen(argv [2],"wb"))== NULL) return;
  if((inFile = fopen(infile, "rb"))== NULL) return;
  if((outFile = fopen(outfile,"wb"))== NULL) return;
 	fread(&fileHeader,sizeof(WAVE_FILE_HEADER),1,inFile);
 	source_type = fileHeader.numChannels;
  if(fileHeader.numChannels ==2)
  {
     fileHeader.sampleRate = fileHeader.sampleRate*160/441;
     fileHeader.byteRate = fileHeader.byteRate*160/441/2;
     fileHeader.chunkSize -= fileHeader.subChunk2Size*((441-160)/441+1/2);
     fileHeader.subChunk2Size = fileHeader.subChunk2Size*160/441/2;
     fileHeader.numChannels = 1 ;
     fileHeader.blockAlign /= 2;
  }
  else
  {
     fileHeader.sampleRate = fileHeader.sampleRate*160/441;
     fileHeader.byteRate = fileHeader.byteRate*160/441;
     fileHeader.chunkSize -= fileHeader.subChunk2Size*(441-160)/441;
     fileHeader.subChunk2Size = fileHeader.subChunk2Size*160/441;
  }
  // fwrite(&fileHeader,sizeof(WAVE_FILE_HEADER),1,outFile);      
   inFileData = malloc((BUFFER_HEADER + BUFFER_SIZE)*sizeof(short));
   outFileData = malloc((BUFFER_SIZE)*sizeof(short));
   inFileData_tmp = malloc(2*BUFFER_SIZE*sizeof(short));
	 unsigned long srcIdx,tgtIdx;
	 long coefIdx;
   for(i=0;i<BUFFER_HEADER ;i++) inFileData[i]=0;
   coefIdx = j = 0;
   do
   {
      if(source_type ==2) {
	  		readSize = fread(&inFileData_tmp[0],sizeof(short),BUFFER_SIZE*2,inFile);   //read voice data.
	  		if(readSize) {
		  		for(i=0;i<BUFFER_SIZE;i++) {
		  			inFileData[BUFFER_HEADER+i]=inFileData_tmp[2*i];
	  			}
	  		}
	  		readSize = readSize/2;
			}else {
	  		readSize = fread(&inFileData[BUFFER_HEADER],sizeof(short),BUFFER_SIZE,inFile);   //read voice data.
			}
	 		if(!readSize)
				break;
		 	for (srcIdx=0,tgtIdx= 0; ;tgtIdx ++,j++) {
	   		while(coefIdx <0) {
		   		coefIdx += INPUT_INTERVAL;
		   		srcIdx++;
		   		if(srcIdx>=readSize) j=0;
	   		}
	 			if(((srcIdx==readSize)&&(coefIdx<=0))||(srcIdx>readSize)){
		   		break;
		 		}
	   		ConvertFreq(inFileData,srcIdx,outFileData,tgtIdx,coefIdx);
	  		if(j>=160) j=0;
      	if(j>=148) coefIdx--;
	  		coefIdx -= (OUTPUT_INTERVAL);
			}
			for(i=0;i<BUFFER_HEADER ;i++) inFileData[i]= inFileData[readSize + i];
  		fwrite(outFileData,sizeof(short),((readSize)*160/441),outFile);
   }while (readSize == BUFFER_SIZE);
  
 	free  (inFileData);
  free  (outFileData);
  free (inFileData_tmp);
  fclose(inFile);
  fclose(outFile);
}



void resample_buf_441_to_16(char * in,unsigned long len,char * out, unsigned long *length)
{	

  unsigned long readSize,count;
  short *inFileData,*outFileData,*inFileData_tmp;
  // long i,j;
  int i,j;
  int source_type;
	
  source_type = 2;
  
  //DEBUG_PRINTF("len=%lu",len);  

   //*out   = malloc(len*160/441/2);
   //DEBUG_PRINTF("*out len=%lu",len*160/441/2);  

   inFileData = malloc((BUFFER_HEADER + BUFFER_SIZE)*sizeof(short));

   outFileData = malloc((BUFFER_SIZE)*sizeof(short));
	
   inFileData_tmp = malloc(2*BUFFER_SIZE*sizeof(short));

	 unsigned long srcIdx,tgtIdx;
	 long coefIdx;
   for(i=0;i<BUFFER_HEADER ;i++) inFileData[i]=0;
   coefIdx = j = 0;
   do
   {
      if(source_type ==2) {
	  		//readSize = fread(,sizeof(short),BUFFER_SIZE*2,inFile);   //read voice data.
	  		// 读readSize * 2;
	  		//DEBUG_PRINTF("len=%lu",len);
	  		if(len > BUFFER_SIZE*2*2) {
	  				DEBUG_PRINTF("len > BUFFER_SIZE*2*2",len);
	  				count = BUFFER_SIZE*2;
	  				len -= count*2;
	  		} else {
	  				count = len/2;
	  		}
	  		//DEBUG_PRINTF("count=%lu",count);
	  		//DEBUG_PRINTF("len=%lu",len);
	  		memcpy(&inFileData_tmp[0],in, count*2);
	  		if(count) {
		  		for(i=0;i<count;i++) {
		  			inFileData[BUFFER_HEADER+i]=inFileData_tmp[2*i];
	  			}
	  		}
	  		count = count/2;
			}else {
				//DEBUG_PRINTF("len=%lu",len);
				if(len > BUFFER_SIZE*2) {
	  				count = BUFFER_SIZE*2;
	  				len -= count;
	  		} else {
	  				count = len;
	  		}
	  		//DEBUG_PRINTF("count=%lu",count);
	  		//DEBUG_PRINTF("len=%lu",len);
	  		memcpy(&inFileData[BUFFER_HEADER],in, count*2);
	  		//readSize = fread(&inFileData[BUFFER_HEADER],sizeof(short),BUFFER_SIZE,inFile);   //read voice data.
			}
	 		if(!count)
				break;
		 	for (srcIdx=0,tgtIdx= 0; ;tgtIdx ++,j++) {
	   		while(coefIdx <0) {
		   		coefIdx += INPUT_INTERVAL;
		   		srcIdx++;
		   		if(srcIdx>=count) j=0;
	   		}
	 			if(((srcIdx==count)&&(coefIdx<=0))||(srcIdx>count)){
		   		break;
		 	}
	  	//	DEBUG_PRINTF("ConvertFreq");
	   		ConvertFreq(inFileData,srcIdx,outFileData,tgtIdx,coefIdx);
	  		if(j>=160) j=0;
      	if(j>=148) coefIdx--;
	  		coefIdx -= (OUTPUT_INTERVAL);
			}
			for(i=0;i<BUFFER_HEADER ;i++) inFileData[i]= inFileData[count + i];
  		//fwrite(outFileData,sizeof(short),((readSize)*160/441),outFile);
  		memcpy(out,outFileData,((count)*160/441*2));
  		out += ((count)*160/441*2);
  		//DEBUG_PRINTF("length1=%lu",((count)*160/441*2));
  		*length += ((count)*160/441*2);
  		//DEBUG_PRINTF("length=%lu",*length);
   }while (count == BUFFER_SIZE);
  //DEBUG_PRINTF("length=%lu",*length);
  free  (inFileData);
  free  (outFileData);
  free (inFileData_tmp);

}