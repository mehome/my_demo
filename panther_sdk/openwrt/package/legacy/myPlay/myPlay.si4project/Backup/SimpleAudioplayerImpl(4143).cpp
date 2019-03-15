/*
 * Copyright (c) 2017 Baidu, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "SimpleAudioPlayerImpl.h"
#include "myPlay.h"

namespace duerOSDcsApp {
namespace mediaPlayer {

#define MAX_AUDIO_FRAME_SIZE 22050
#define OUTPUT_CHANNEL 2
#define OUTPUT_SAMPLE_RATE 44100
#define FRAME_BUFFER_SIZE 44100

#define SEND_PORT 8060

int sockfd = -1;
struct sockaddr_in dest_address;

int SimpleAudioPlayerImpl::create_broadcast_socket(char* ifname)
{
    int b_opt_val = 1;
    struct ifreq ifr;
    int ret = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        printf("fail to create socket");
        ret = -1;
        goto RETURN;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &b_opt_val, sizeof(int)) == -1)
    {
        printf("fail to setopt broadcast");
        ret = -1;
        goto CLOSE_FD;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_ifrn.ifrn_name, ifname, strlen(ifname));
    ifr.ifr_addr.sa_family = AF_INET;

    if ((ioctl(sockfd, SIOCGIFFLAGS, &ifr)>= 0)&&(ifr.ifr_flags&IFF_UP))
    {
        if (setsockopt(sockfd,SOL_SOCKET,SO_BINDTODEVICE,ifname,strlen(ifname)+1) == -1 )
        {
            printf("setsockopt %s SO_BINDTODEVICE: %m", ifname);
            ret = -1;
            goto CLOSE_FD;
        }
    }
    else
    {
        printf("bind interface %s fail", ifname);
        ret = -1;
        goto CLOSE_FD;
    } 

    memset(&dest_address, 0, sizeof(dest_address));
    dest_address.sin_family      = AF_INET;
    dest_address.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    dest_address.sin_port        = htons(SEND_PORT);

    return ret;

CLOSE_FD:
    close(sockfd);
    sockfd = -1;

RETURN:
    return ret;
}

int SimpleAudioPlayerImpl::send_broadcast_to_interface(char* buffer, char* ifname)
{
	int i;
    ssize_t cnt = 0;
    int ret = 0;

    if (-1 == sockfd)
    {
        if (0 != create_broadcast_socket(ifname))
        {
            printf("create_socket %s fail", ifname);
            ret = -1;
            goto RETURN;
        }
    }

	for (i = 0; i < 10; i++)
	{
    	cnt = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&dest_address, (int)sizeof(dest_address));
		usleep(1000 * 100);
	}

    if (0>cnt) 
    {
        printf("sendto fail");
        ret = -1;
    } 
    else 
    {
        printf("%s send broadcast %s\n", ifname, buffer); 
    }

    close(sockfd);
    sockfd=-1;

RETURN:

    return ret;
}


int SimpleAudioPlayerImpl::sendToTestTool(long int startTime, char byFlag) {
	int ret = -1;
    if(access("/tmp/test_mode",0)) {
        return -1;  //no test signal
    }

	char byData[20]={0};

	if (0 == byFlag) {
		long int endTime = 0;
		struct timeval tv; 
		gettimeofday(&tv,NULL);	
		endTime = tv.tv_sec;

		long int processTime = endTime - startTime;
		sprintf(byData,"myPlay_%02d", processTime);
	} else {
		sprintf(byData,"startPlay_%02d", byFlag);
	}

    if (0 == send_broadcast_to_interface(byData, "br0")) {
        ret = 0;
    }

    if (0 == send_broadcast_to_interface(byData, "br1")) {
        ret = 0;
    }

	return ret;
}



SimpleAudioPlayerImpl::SimpleAudioPlayerImpl(const std::string &audio_dev) :
                                                                             m_alsaController(NULL),
                                                                             m_audioDecoder(NULL),
                                                                             m_listener(NULL),
                                                                             m_pcmBuffer(NULL),
                                                                             m_frameBuffer(NULL),
                                                                             m_FadeinTimeMs(0),
                                                                             m_FadeinTimeProgressMs(0),
                                                                             m_threadId(0),
                                                                             m_stopFlag(false),
                                                                             m_playReadyFlag(false),
                                                                             m_exitFlag(false) {
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_playCond, NULL);

    m_alsaController = new AlsaController(audio_dev);
    m_alsaController->init(OUTPUT_SAMPLE_RATE, OUTPUT_CHANNEL);

    m_audioDecoder = new AudioDecoder(OUTPUT_CHANNEL, OUTPUT_SAMPLE_RATE);
    m_pcmBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);
    m_frameBuffer = (uint8_t *)av_malloc(FRAME_BUFFER_SIZE);
#if 1
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1536*1024);
    pthread_create(&m_threadId, &attr, playFunc, this);
	pthread_attr_destroy(&attr);
#else
	pthread_create(&m_threadId,nullptr, playFunc, this);
#endif
}

SimpleAudioPlayerImpl::~SimpleAudioPlayerImpl() {
    m_stopFlag = true;
    m_exitFlag = true;
    pthread_cond_signal(&m_playCond);

    void *thread_return = NULL;
    if (m_threadId != 0) {
        pthread_join(m_threadId, &thread_return);
    }

    m_alsaController->aslaAbort();
    m_alsaController->alsaClose();
    if (m_alsaController) {
        delete m_alsaController;
        m_alsaController = NULL;
    }

    if (m_pcmBuffer != NULL) {
        av_free(m_pcmBuffer);
        m_pcmBuffer = NULL;
    }
    if (m_frameBuffer != NULL) {
        av_free(m_frameBuffer);
        m_frameBuffer = NULL;
    }
    if (m_audioDecoder != NULL) {
        delete m_audioDecoder;
        m_audioDecoder = NULL;
    }

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_playCond);
}
#define READ_FRAME_FAILIURE_RETRY_COUNT 5
unsigned int SimpleAudioPlayerImpl::m_failRetryCount =READ_FRAME_FAILIURE_RETRY_COUNT;
void *SimpleAudioPlayerImpl::playFunc(void *arg) {
	showProcessThreadId("SimpleAudioPlayerImpl");

    SimpleAudioPlayerImpl *player = (SimpleAudioPlayerImpl *) arg;
    while (!player->m_exitFlag) {
        pthread_mutex_lock(&player->m_mutex);
        player->m_playReadyFlag = true;
        while (player->m_resPath.empty()) {
            pthread_cond_wait(&player->m_playCond, &player->m_mutex);
            if (player->m_exitFlag) {
                return NULL;
            }
        }
        player->executePlaybackStarted();

        myPlayInf("play begin:%s", player->m_resPath.c_str());
        player->m_alsaController->alsaPrepare();

        bool validUrl = player->m_audioDecoder->open(player->m_resPath);
        if (!validUrl) {
            myPlayErr("play cancel, invalid url");
        }
		m_failRetryCount = READ_FRAME_FAILIURE_RETRY_COUNT;
        int pos = 0;
        player->fadeinReset();
        while (!player->m_stopFlag && validUrl) {
            unsigned int len = 0;
            memset(player->m_frameBuffer, 0, FRAME_BUFFER_SIZE);
            StreamReadResult ret = player->m_audioDecoder->readFrame(&player->m_frameBuffer,
                                                                     FRAME_BUFFER_SIZE,
                                                                     &len);
            if (ret == READ_SUCCEED) {
                uint8_t *p = player->m_pcmBuffer + pos;
                if (pos + len > MAX_AUDIO_FRAME_SIZE) {
                    player->fadeinProcess( pos);
                    player->m_alsaController->writeStream(OUTPUT_CHANNEL,
                                                          player->m_pcmBuffer,
                                                          pos);
                    pos = 0;
                    p = player->m_pcmBuffer;
                    memcpy(p, player->m_frameBuffer, len);
                    pos += len;
                } else {
                    memcpy(p, player->m_frameBuffer, len);
                    pos += len;
                }
            } else {
				if(ret == READ_AGAIN) {
					continue;
                }
				if(ret == READ_FAILED) {
					m_failRetryCount --;
					if(m_failRetryCount){
						continue;
					}
					break;
                }
				break;
            }
        }

        if (pos > 0) {
            player->fadeinProcess( pos);
            player->m_alsaController->writeStream(OUTPUT_CHANNEL,
                                                  player->m_pcmBuffer,
                                                  pos);
        }
        player->m_audioDecoder->close();
        usleep(500000);
        player->executePlaybackFinished();
        myPlayInf("play end, wait for next");

        player->m_alsaController->alsaClear();
        player->m_resPath = "";
        player->m_playReadyFlag = false;
        pthread_mutex_unlock(&player->m_mutex);
	
    }
    return NULL;
}

void SimpleAudioPlayerImpl::registerListener(AudioPlayerListener *notify) {
    m_listener = notify;
}

void SimpleAudioPlayerImpl::setPlayMode(PlayMode mode, unsigned int val) {
    if (m_audioDecoder != NULL) {
        if (mode == PLAYMODE_LOOP) {
            m_audioDecoder->setDecodeMode(DECODE_MODE_LOOP, val);
        } else {
            m_audioDecoder->setDecodeMode(DECODE_MODE_NORMAL, 0);
        }
    }
}

void SimpleAudioPlayerImpl::audioPlay(const std::string &url,
                                      RES_FORMAT format,
                                      unsigned int offset,
                                      unsigned int report_interval) {
    myPlayInf("audio_play called enter");
    while (!m_playReadyFlag) {
        usleep(20000);
    }
    audioStop();
    pthread_mutex_lock(&m_mutex);
    m_resPath = url;
    m_stopFlag = false;
    pthread_cond_signal(&m_playCond);
    pthread_mutex_unlock(&m_mutex);
    myPlayInf("audio_play called exit");
}

void SimpleAudioPlayerImpl::audioPause() {
}

void SimpleAudioPlayerImpl::audioResume() {
}

void SimpleAudioPlayerImpl::audioStop() {
    myPlayInf("audio_stop called enter");
    m_stopFlag = true;
	m_alsaController->aslaAbort();
    m_alsaController->alsaClear();
    myPlayInf("audio_stop called exit");
}

void SimpleAudioPlayerImpl::setFadeIn(int timeSec) {
    myPlayInf("setFadeIn %d", timeSec);
    m_FadeinTimeMs = timeSec * 1000;
    return;
}

void SimpleAudioPlayerImpl::executePlaybackStarted() {
    myPlayInf("executePlaybackStarted called");
    // m_executor->submit([this]() {
        // if (NULL != m_listener) {
            // m_listener->playbackStarted(0);
        // }
    // });
}
void SimpleAudioPlayerImpl::executePlaybackFinished() {
    myPlayInf("executePlaybackFinished called");
	sendToTestTool(startTime, 0);

	exit(0);
    // m_executor->submit([this]() {
        // if (m_stopFlag) {
            // m_listener->playbackFinished(0, AudioPlayerFinishStatus::AUDIOPLAYER_FINISHSTATUS_STOP);
        // } else {
            // m_listener->playbackFinished(0, AudioPlayerFinishStatus::AUDIOPLAYER_FINISHSTATUS_END);
        // }
    // });
}

float SimpleAudioPlayerImpl::getFadeAttenuate() {
    float min_attenuate_in_db = 0;
    float max_attenuate_in_db = -80;
    float attenuate_in_db = max_attenuate_in_db + (min_attenuate_in_db - max_attenuate_in_db)  *
                                                          m_FadeinTimeProgressMs / m_FadeinTimeMs;
    if (attenuate_in_db > 0) {
        attenuate_in_db = 0;
    }
    float attenuate = exp(attenuate_in_db / 20);
    myPlayInf("atten_in_db %f atten %f", attenuate_in_db, attenuate);
    return attenuate;
}
void SimpleAudioPlayerImpl::fadeinReset() {
    m_FadeinTimeProgressMs = 0;
}
void SimpleAudioPlayerImpl::fadeinProcess(int  size) {
    if (m_FadeinTimeMs > m_FadeinTimeProgressMs) {
        //liner fade from 0-10 sec
        float attenuate = getFadeAttenuate();
        short val;
        for (int i = 0; i< size / 2; i++) {
            val = ((short *)m_pcmBuffer)[i] * attenuate;
            ((short *)m_pcmBuffer)[i] = val;
        }
        m_FadeinTimeProgressMs += size * 1000 / OUTPUT_SAMPLE_RATE / OUTPUT_CHANNEL /2 ;
    }
}

unsigned long SimpleAudioPlayerImpl::getProgress() {
    return 0;
}

unsigned long SimpleAudioPlayerImpl::getDuration() {
    return 0;
}

}  // mediaPlayer
}  // duerOSDcsApp
