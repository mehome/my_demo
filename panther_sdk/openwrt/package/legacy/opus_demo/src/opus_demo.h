#ifndef __OPUS_DEMO_H_
#define __OPUS_DEMO_H_

int opus_demo_file(char *set_cmd,char *set_app,int set_sample_rate,int set_channel,
           int set_bitrate_bps,/*char *set_bitrate,char *set_bandwidth,
           char *set_bandwidth_rate,char *set_framesize,int set_framesize_value,
           char *set_max_payload,int set_max_payload_value,char *set_complexity,int set_complexity_value,
           char *set_inbandfec,char *set_forcemono,char *set_cvbr,char *set_delayed_decision,
           char *set_loss,int set_loss_value,*/char *set_inFile,char *set_outFile);
           
           
int opus_demo_buff(char *set_cmd,char *set_app,int set_sample_rate,int set_channel,
           int set_bitrate_bps,/*char *set_bitrate,char *set_bandwidth,
           char *set_bandwidth_rate,char *set_framesize,int set_framesize_value,
           char *set_max_payload,int set_max_payload_value,char *set_complexity,int set_complexity_value,
           char *set_inbandfec,char *set_forcemono,char *set_cvbr,char *set_delayed_decision,
           char *set_loss,int set_loss_value,*/char *set_inFile,char *set_outFile);
           
#endif