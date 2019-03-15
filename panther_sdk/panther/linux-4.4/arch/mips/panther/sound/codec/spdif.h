#ifndef _CODEC_SPDIF_H
#define _CODEC_SPDIF_H

#define STUB_RATES  (SNDRV_PCM_RATE_16000  |    \
                     SNDRV_PCM_RATE_22050  |    \
                     SNDRV_PCM_RATE_32000  |    \
                     SNDRV_PCM_RATE_44100  |    \
                     SNDRV_PCM_RATE_48000  |    \
                     SNDRV_PCM_RATE_88200  |    \
                     SNDRV_PCM_RATE_96000  |    \
                     SNDRV_PCM_RATE_176400 |    \
                     SNDRV_PCM_RATE_192000)

#define STUB_FORMATS    (SNDRV_PCM_FMTBIT_S16_LE  | \
                         SNDRV_PCM_FMTBIT_S20_3LE | \
                         SNDRV_PCM_FMTBIT_S24_LE)

#endif
