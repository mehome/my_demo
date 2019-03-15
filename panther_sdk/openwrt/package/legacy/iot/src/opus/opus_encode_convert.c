#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <opus/opus.h>
//#include <opus/debug.h>
#include <opus/opus_types.h>
//#include <opus/opus_private.h>
#include <opus/opus_multistream.h>
#include "myutils/debug.h"
#include "opus_encode_convert.h"
#define MAX_PACKET 1500
#define MODE_CELT_ONLY 1002
#define OPUS_SET_FORCE_MODE_REQUEST    11002
#define OPUS_SET_FORCE_MODE(x) OPUS_SET_FORCE_MODE_REQUEST, __opus_check_int(x)


static OpusEncoder* mp_opuEncoder = NULL;
static int m_frameSamples = 320;
static int m_maxDataBytes = (320 * sizeof(short));
static int m_vbr = 1;
static int m_bitRate = 27800;
static int m_complexity = 8;
static int m_signal = OPUS_SIGNAL_VOICE;
static int m_sample_rate = 16000;
static int m_channels = 1;
int initialized =0;
void print_usage( char* argv[] )
{
    fprintf(stderr, "Usage: %s [-e] <application> <sampling rate (Hz)> <channels (1/2)> "
        "<bits per second>  [options] <input> <output>\n", argv[0]);
    fprintf(stderr, "       %s -d <sampling rate (Hz)> <channels (1/2)> "
        "[options] <input> <output>\n\n", argv[0]);
    fprintf(stderr, "application: voip | audio | restricted-lowdelay\n" );
    fprintf(stderr, "options:\n" );
    fprintf(stderr, "-e                   : only runs the encoder (output the bit-stream)\n" );
    fprintf(stderr, "-d                   : only runs the decoder (reads the bit-stream as input)\n" );
    fprintf(stderr, "-cbr                 : enable constant bitrate; default: variable bitrate\n" );
    fprintf(stderr, "-cvbr                : enable constrained variable bitrate; default: unconstrained\n" );
    fprintf(stderr, "-delayed-decision    : use look-ahead for speech/music detection (experts only); default: disabled\n" );
    fprintf(stderr, "-bandwidth <NB|MB|WB|SWB|FB> : audio bandwidth (from narrowband to fullband); default: sampling rate\n" );
    fprintf(stderr, "-framesize <2.5|5|10|20|40|60|80|100|120> : frame size in ms; default: 20 \n" );
    fprintf(stderr, "-max_payload <bytes> : maximum payload size in bytes, default: 1024\n" );
    fprintf(stderr, "-complexity <comp>   : complexity, 0 (lowest) ... 10 (highest); default: 10\n" );
    fprintf(stderr, "-inbandfec           : enable SILK inband FEC\n" );
    fprintf(stderr, "-forcemono           : force mono encoding, even for stereo input\n" );
    fprintf(stderr, "-dtx                 : enable SILK DTX\n" );
    fprintf(stderr, "-loss <perc>         : simulate packet loss, in percent (0-100); default: 0\n" );
}

static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

static void check_encoder_option(int decode_only, const char *opt)
{
   if (decode_only)
   {
      fprintf(stderr, "option %s is only for encoding\n", opt);
      exit(EXIT_FAILURE);
   }
}

    int err;
    OpusEncoder *enc=NULL;
    int len[2];
    int frame_size;
    int channels = 1;
    opus_int32 bitrate_bps=0;
    unsigned char *data[2];
    unsigned char *fbytes;
    opus_int32 sampling_rate = 16000;
    int use_vbr;
    int max_payload_bytes;
    int complexity;
    int use_inbandfec;
    int use_dtx;
    int forcechannels;
    int cvbr = 0;
    int packet_loss_perc;
    opus_int32 count=0, count_act=0;
    int k;
    opus_int32 skip=0;
    int stop=0;
    short *in, *out;
    int application= OPUS_APPLICATION_RESTRICTED_LOWDELAY;
    double bits=0.0, bits_max=0.0, bits_act=0.0, bits2=0.0, nrg;
    double tot_samples=0;
    opus_uint64 tot_in, tot_out;
    int bandwidth=OPUS_AUTO;
    const char *bandwidth_string;
    int lost = 0, lost_prev = 1;
    int toggle = 0;
    opus_uint32 enc_final_range[2];
    int max_frame_size = 48000*2;
    size_t num_read;
    int curr_read=0;
    int sweep_bps = 0;
    int random_framesize=0, newsize=0, delayed_celt=0;
    int sweep_max=0, sweep_min=0;
    int random_fec=0;
    const int (*mode_list)[4]=NULL;
    int nb_modes_in_list=0;
    int curr_mode=0;
    int curr_mode_count=0;
    int mode_switch_time = 48000;
    int nb_encoded=0;
    int remaining=0;
    int variable_duration=OPUS_FRAMESIZE_ARG;
    int delayed_decision=0;
   FILE *fin, *fout;
  char  *outFile ="/tmp/iot.opus";



int opus_encode_buffer_init()
{
	
	  /* defaults: */
    use_vbr = 1;
    max_payload_bytes = MAX_PACKET;
    complexity = 10;
    use_inbandfec = 0;
    forcechannels = OPUS_AUTO;
    use_dtx = 0;
    packet_loss_perc = 0;
    if (sweep_max)
       sweep_min = bitrate_bps;
      initialized =1; 
	 fprintf(stderr, "%s\n", opus_get_version_string());

    frame_size = sampling_rate/50;
    
    if (channels < 1 || channels > 2)
    {
        fprintf(stderr, "Opus_demo supports only 1 or 2 channels.\n");
        return EXIT_FAILURE;
    }
   
  

    if (max_payload_bytes < 0 || max_payload_bytes > MAX_PACKET)
    {
        fprintf (stderr, "max_payload_bytes must be between 0 and %d\n",
                          MAX_PACKET);
        return EXIT_FAILURE;
    }
/*
    inFile = set_inFile;
    fin = fopen(inFile, "rb");
    if (!fin)
    {
        fprintf (stderr, "Could not open input file %s\n", set_inFile);
        return EXIT_FAILURE;
    }
  
    outFile = set_outFile;
    fout = fopen(outFile, "wb+");
    if (!fout)
    {
        fprintf (stderr, "Could not open output file %s\n", set_outFile);
        fclose(fin);
        return EXIT_FAILURE;
    }
*/
       enc = opus_encoder_create(sampling_rate, channels, application, &err);
       if (err != OPUS_OK)
       {
          fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(err));
         // fclose(fin);
         // fclose(fout);
          return -1;
       }
       info("bitrate_bps =%d\n",bitrate_bps);
       opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
       opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(bandwidth));
       opus_encoder_ctl(enc, OPUS_SET_VBR(use_vbr));
       opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
       opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
       opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(use_inbandfec));
       opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(forcechannels));
       opus_encoder_ctl(enc, OPUS_SET_DTX(use_dtx));
       opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));

       opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));
       opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));
       opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(variable_duration));
    
       fprintf(stderr, "Encoding %ld Hz input at %.3f kb/s "
                       "in %s with %d-sample frames.\n",
                       (long)sampling_rate, bitrate_bps*0.001,
                       bandwidth_string, frame_size);

   // in = (short*)malloc(max_frame_size*channels*sizeof(short));
   // out = (short*)malloc(max_frame_size*channels*sizeof(short));
    /* We need to allocate for 16-bit PCM data, but we store it as unsigned char. */
   // fbytes = (unsigned char*)malloc(max_frame_size*channels*sizeof(short));
/*
   data[0] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));
    info("use_inbandfec = %d\n",use_inbandfec);
    if ( use_inbandfec ) {
        data[1] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));
    }
   */ 
   
	
	}

int opus_encode_buffer_convert(char *i_pData, int DataLen, char *o_pData,int *encode_length,opus_uint32 *enc_final_range_length)
{    
    //while (1)
    //{ 
      fout = fopen(outFile, "a+");
    if (!fout)
    {
        fprintf (stderr, "Could not open output file %s\n", outFile);
        
        return EXIT_FAILURE;
    }
            int i;
            //num_read = fread(fbytes, sizeof(short)*channels, frame_size-remaining, fin);
             fbytes = (unsigned char*)malloc(max_frame_size*channels*sizeof(short));
             //data[0] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));
             in = (short*)malloc(max_frame_size*channels*sizeof(short));
           //  info("convert use_inbandfec = %d\n",use_inbandfec);
             if ( use_inbandfec ) {
                data[1] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));
              }
            memcpy(fbytes, i_pData, DataLen);      
            curr_read = (int)DataLen;
            tot_in += curr_read;
            for(i=0;i<curr_read*channels;i++)
            {
                opus_int32 s;
                s=fbytes[2*i+1]<<8|fbytes[2*i];
                s=((s&0xFFFF)^0x8000)-0x8000;
                in[i+remaining*channels]=s;
            }
            if (curr_read+remaining < frame_size)
            {
                for (i=(curr_read+remaining)*channels;i<frame_size*channels;i++)
                   in[i] = 0;
                  // stop = 1;
            }
            len[toggle] = opus_encode(enc, in, frame_size, o_pData, max_payload_bytes);
            nb_encoded = opus_packet_get_samples_per_frame(o_pData, sampling_rate)*opus_packet_get_nb_frames(o_pData, len[toggle]);
            remaining = frame_size-nb_encoded;
            for(i=0;i<remaining*channels;i++)
               in[i] = in[nb_encoded*channels+i];
            if (sweep_bps!=0)
            {
            	printf("sweep_bps =%d\n",sweep_bps);
               bitrate_bps += sweep_bps;
               if (sweep_max)
               {
                  if (bitrate_bps > sweep_max)
                     sweep_bps = -sweep_bps;
                  else if (bitrate_bps < sweep_min)
                     sweep_bps = -sweep_bps;
               }
               /* safety */
               if (bitrate_bps<1000)
                  bitrate_bps = 1000;
               opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
            }
            opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range[toggle]));
            if (len[toggle] < 0)
            {
                fprintf (stderr, "opus_encode() returned %d\n", len[toggle]);
               // fclose(fin);
                //fclose(fout);
                return EXIT_FAILURE;
            }
            curr_mode_count += frame_size;
            if (curr_mode_count > mode_switch_time && curr_mode < nb_modes_in_list-1)
            {
            	//printf("curr_mode_count =%d,mode_switch_time =%d,curr_mode =%d,nb_modes_in_list =%d\n",curr_mode_count,mode_switch_time,curr_mode,nb_modes_in_list);
               curr_mode++;
               curr_mode_count = 0;
            }
        

       // if (encode_only)
        //{
         
            unsigned char int_field[4];
            int_to_char(len[toggle], int_field);
            if (fwrite(int_field, 1, 4, fout) != 4) {
               fprintf(stderr, "Error writing.\n");
               return EXIT_FAILURE;
            }
            int_to_char(enc_final_range[toggle], int_field);
            if (fwrite(int_field, 1, 4, fout) != 4) {
               fprintf(stderr, "Error writing.\n");
               return EXIT_FAILURE;
            }
            if (fwrite(o_pData, 1, len[toggle], fout) != (unsigned)len[toggle]) {
               fprintf(stderr, "Error writing.\n");
               return EXIT_FAILURE;
            }
          
            tot_samples += nb_encoded;

        //}

     //   o_pData=data[toggle];
    //  info("demo len[toggle] =%d,enc_final_range[toggle] =%d\n",len[toggle],enc_final_range[toggle]);
        *encode_length = len[toggle];
        *enc_final_range_length = enc_final_range[toggle];
        lost_prev = lost;
 // info("demo encode_length =%d,enc_final_range_length =%d\n",*encode_length,*enc_final_range_length);
  //  }   
    count -= use_inbandfec;
   
 /*
       fprintf (stderr, "active bitrate:              %7.3f kb/s\n",
               1e-3*bits_act*sampling_rate/(1e-15+frame_size*(double)count_act));
    fprintf (stderr, "bitrate standard deviation:  %7.3f kb/s\n",
            1e-3*sqrt(bits2/count - bits*bits/(count*(double)count))*sampling_rate/frame_size);
     */       
  //  silk_TimerSave("opus_timing.txt");
 /*
    opus_encoder_destroy(enc);
    free(data[0]);
    if (use_inbandfec)
        free(data[1]);
    fclose(fin);
    fclose(fout);
    free(in);
    free(out);
    free(fbytes);
    return EXIT_SUCCESS;
 */

 if(fbytes)
 free(fbytes);
 if(in)
 free(in);
  fclose(fout);
return EXIT_SUCCESS;
}

int opus_codec_destroy() {
    //printf("opus_codec_destroy++\n");
    if (!initialized) {
        printf("Opus codec is not initialized\n");
        return 0;
    }
    if (enc) {
        opus_encoder_destroy(enc);
    }
    enc = NULL;
    initialized = 0;
    //printf("opus destroy--\n");
    return 0;
}

int End_Opus_Encdoe(void)
{
	int iRet;
	
	iRet = opus_codec_destroy();
	if(iRet < 0)
	{
		printf("[VOICE_DEBUG]opus_codec_encode() fialed!\n");
		return -1;
	}
	
	return 0;
}

#if 0
int main(int argc,char *argv[]){
	/*
	opus_demo(char *set_cmd,char *set_app,int set_sample_rate,int set_channel,
           int set_bitrate_bps,char *set_bitrate,char *set_bandwidth,
           char *set_bandwidth_rate,char *set_framesize,int set_framesize_value,
           char *set_max_payload,int set_max_payload_value,char *set_complexity,int set_complexity_value,
           char *set_inbandfec,char * set_forcemono,char *cvbr,char *set_delayed-decision,
           char *set_loss,int set_loss_value,char *set_inFile,char *set_outFile)
           */
   opus_demo_file("-e","restricted-lowdelay",16000,1,0,"/tmp/recoder.pcm","/tmp/opus_demo_encoder.opus");     
//   opus_demo("-d","",16000,1,0,"./opus_demo_encoder.opus","opus_demo_decoder.pcm");    
	return 0;
	}
#endif