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
#include "DCSApp/PortAudioMicrophoneWrapper.h"
#include "DCSApp/DeviceIoWrapper.h"
#include "LoggerUtils/DcsSdkLogger.h"
extern "C"{
	#include <libcchip/function/user_timer/user_timer.h>
}
namespace duerOSDcsApp {
namespace application {
    
static const int NUM_INPUT_CHANNELS = 1;
static const int NUM_OUTPUT_CHANNELS = 0;
static const double SAMPLE_RATE = 16000;
//static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK = paFramesPerBufferUnspecified;

std::unique_ptr<PortAudioMicrophoneWrapper> PortAudioMicrophoneWrapper::create(
    std::shared_ptr<duerOSDcsSDK::sdkInterfaces::DcsSdk> dcsSdk) {
    if (!dcsSdk) {
        APP_ERROR("Invalid dcsSdk passed to PortAudioMicrophoneWrapper");
        return nullptr;
    }
    std::unique_ptr<PortAudioMicrophoneWrapper> portAudioMicrophoneWrapper(new PortAudioMicrophoneWrapper(dcsSdk));
    if (!portAudioMicrophoneWrapper->initialize()) {
        APP_ERROR("Failed to initialize PortAudioMicrophoneWrapper");
        return nullptr;
    }
	
    return portAudioMicrophoneWrapper;
}

PortAudioMicrophoneWrapper::PortAudioMicrophoneWrapper(std::shared_ptr<duerOSDcsSDK::sdkInterfaces::DcsSdk> dcsSdk) :
        m_dcsSdk{dcsSdk},

#ifdef USE_ALSA_RECORDER
//        _m_rec{nullptr},
#else
        m_paStream{nullptr},
#endif
        m_callBack{nullptr} {
}


PortAudioMicrophoneWrapper::~PortAudioMicrophoneWrapper() {
#ifdef USE_ALSA_RECORDER
//    _m_rec->recorderClose();
#else
    Pa_StopStream(m_paStream);
    Pa_CloseStream(m_paStream);
    Pa_Terminate();
#endif
}
#ifdef USE_ALSA_RECORDER

#ifdef BACKUP_THE_DATA_TO_BE_UPLOADED
std::ofstream PortAudioMicrophoneWrapper::my_out("/tmp/snd.raw");
#endif
void PortAudioMicrophoneWrapper::recordDataInputCallback(char* buffer, long unsigned int size, void *userdata) {
	using namespace duerOSDcsApp::framework; 
	wakeupStartArgs_t *pWakeUpArgs=(wakeupStartArgs_t*)userdata;
	
    PortAudioMicrophoneWrapper* wrapper = static_cast<PortAudioMicrophoneWrapper*>(pWakeUpArgs->PortAudioMicrophoneWrapper_ptr);

    if (wrapper->running == false) {
        APP_ERROR("Record not started");
        return;
    }	
	if(pWakeUpArgs->isWakeUpSucceed){
		DeviceIoWrapper::getInstance()->callback(DeviceInput::KEY_WAKE_UP,NULL,0);
		pWakeUpArgs->isWakeUpSucceed=0;
#ifdef BACKUP_THE_DATA_TO_BE_UPLOADED
		wrapper->my_out.clear();
		wrapper->my_out.close();
		wrapper->my_out.open("/tmp/snd.raw");
#endif
	}

    ssize_t returnCode = 1;
    if (!DeviceIoWrapper::getInstance()->isPhoneConnected()) {
//		inf("upload audio data....")
        returnCode = wrapper->m_dcsSdk->writeAudioData(buffer, size/2);
		if(getListenStatus()){
#ifdef BACKUP_THE_DATA_TO_BE_UPLOADED
			my_out.write(buffer,size);
#endif
		}
    }

    if (wrapper->m_callBack && buffer && size != 0) {
        wrapper->m_callBack((const char *)buffer, size);
    }
    if (returnCode <= 0) {
        APP_ERROR("Failed to write to stream.");
        return;
    }
    return;
}
#endif

static userTimer_t* stopListenUserTimerPtr=NULL;
void stopListenTimeOutCallback(int sig){
	inf_nl(__func__);
	setListenStatusNoLock(0);
	stopListenUserTimerPtr->stop(stopListenUserTimerPtr);
}

bool PortAudioMicrophoneWrapper::initialize() {
#ifdef USE_ALSA_RECORDER
	wakeupStartArgs.pRecordDataInputCallback=recordDataInputCallback;
	wakeupStartArgs.PortAudioMicrophoneWrapper_ptr=this;
	wakeupStartArgs.userTimerPtr=userTimerCreate(__SIGRTMIN+5,&stopListenTimeOutCallback);
	if(!wakeupStartArgs.userTimerPtr){
		err("userTimerCreate failure");
		return -false;
	}
	stopListenUserTimerPtr=wakeupStartArgs.userTimerPtr;
//	struct sched_param param;
	pthread_attr_t attr; 
	pthread_attr_init(&attr);
//	param.sched_priority = 60;
//	pthread_attr_setschedpolicy (&attr, SCHED_RR);
//	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setstacksize(&attr, 64*1024);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	wakeupStartArgs.pAttr=&attr;
	wakeupStartArgs.wakeup_mode=0;
  	int ret  = wakeupStart(&wakeupStartArgs);
	pthread_attr_destroy(&attr);
	if(ret<0){
        err("Failed to new AudioRecorder");
        return false;	
	}
	return true;
#else
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        APP_ERROR("Failed to initialize PortAudio");
        return false;
    }
    err = Pa_OpenDefaultStream(
        &m_paStream,
        NUM_INPUT_CHANNELS,
        NUM_OUTPUT_CHANNELS,
        paInt16,
        SAMPLE_RATE,
        PREFERRED_SAMPLES_PER_CALLBACK,
        PortAudioCallback,
        this);
    if (err != paNoError) {
        APP_ERROR("Failed to open PortAudio default stream");
        return false;
    }
    return true;
#endif
}

bool PortAudioMicrophoneWrapper::startStreamingMicrophoneData() {
    std::lock_guard<std::mutex> lock{m_mutex};
#ifdef USE_ALSA_RECORDER
    running = true;
    return true;
#else
    PaError err = Pa_StartStream(m_paStream);
    if (err != paNoError) {
        APP_ERROR("Failed to start PortAudio stream");
        return false;
    }
    return true;
#endif
}

void PortAudioMicrophoneWrapper::setRecordDataInputCallback(void (*callBack)(const char *buffer, unsigned int size)) {
    m_callBack = callBack;
}

bool PortAudioMicrophoneWrapper::stopStreamingMicrophoneData() {
    std::lock_guard<std::mutex> lock{m_mutex};
#ifdef USE_ALSA_RECORDER
    running = false;
    return true;
#else
    PaError err = Pa_StopStream(m_paStream);
    if (err != paNoError) {
        APP_ERROR("Failed to stop PortAudio stream");
        return false;
    }
    return true;
#endif
}

#ifndef USE_ALSA_RECORDER
int PortAudioMicrophoneWrapper::PortAudioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long numSamples,

    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    PortAudioMicrophoneWrapper* wrapper = static_cast<PortAudioMicrophoneWrapper*>(userData);

    ssize_t returnCode = 1;
    if (!DeviceIoWrapper::getInstance()->isPhoneConnected()) {
        returnCode = wrapper->m_dcsSdk->writeAudioData(inputBuffer, numSamples);
    }

    if (wrapper->m_callBack && inputBuffer && numSamples != 0) {
        wrapper->m_callBack((const char *)inputBuffer, 2 * numSamples);
    }
    if (returnCode <= 0) {
        APP_ERROR("Failed to write to stream.");
        return paAbort;
    }
    return paContinue;
}
#endif
}  // namespace application
}  // namespace duerOSDcsApp
