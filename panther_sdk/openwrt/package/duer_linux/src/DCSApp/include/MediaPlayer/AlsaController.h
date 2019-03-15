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

#ifndef DUEROS_DCS_APP_MEDIAPLAYER_ALSACONTROLLER_H_
#define DUEROS_DCS_APP_MEDIAPLAYER_ALSACONTROLLER_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <alsa/asoundlib.h>
#include <mutex>

namespace duerOSDcsApp {
namespace mediaPlayer {

class AlsaController {
public:
    AlsaController(const std::string &audio_dev);

    ~AlsaController();

    bool init(unsigned int rate, unsigned int channels);

    bool isAccessable() const;

    bool alsaPrepare();

    bool alsaPause();

    bool alsaResume();

    bool aslaAbort();

    void writeStream(unsigned int channels, const void *buffer, unsigned long buff_size);

    bool alsaClear();

    bool alsaClose();
    int alsaChunkByte();
	char getStopWriteiFlag(){
		std::lock_guard<std::mutex> lock{writeiMutex};
		char flag=0;
		flag=StopWriteiFlag;
		return flag;
	}
	void setStopWriteiFlag(char flag){
		std::lock_guard<std::mutex> lock{writeiMutex};
		StopWriteiFlag=flag;
	}
private:
    AlsaController(const AlsaController &);

    AlsaController &operator=(const AlsaController &);

    bool openDevice();

    bool setParams(unsigned int rate, unsigned int channels);

    snd_pcm_t *m_pcmHandle;

    int m_alsaCanPause;

    bool m_abortFlag;

    size_t m_chunkBytes;
    size_t m_bits_per_frame;

    std::string m_pcmDevice;
	
	std::mutex writeiMutex;
	char StopWriteiFlag;
};

}  // mediaPlayer
}  // duerOSDcsApp

#endif  // DUEROS_DCS_APP_MEDIAPLAYER_ALSACONTROLLER_H_
