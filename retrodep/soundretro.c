/*
 * soundretro.c - Simple forwarding sound device for libretro
 *
 */

#include "vice.h"
#include "sound.h"

#include "libretro-core.h"
extern void retro_audio_queue(const int16_t *data, int32_t samples);

static int retro_sound_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    *speed = vice_opt.SoundSampleRate;
#if 0
    printf("speed:%d fragsize:%d fragnr:%d channels:%d\n", *speed, *fragsize, *fragnr, *channels);
#endif
    return 0;
}

static int retro_sound_write(SWORD *pbuf, size_t nr)
{
#if 0
    printf("pbuf:%d nr:%d\n", *pbuf, nr);
#endif
    retro_audio_queue(pbuf, nr);
    return 0;
}

static sound_device_t retro_device =
{
    "retro",            /* name */
    retro_sound_init,   /* init */
    retro_sound_write,  /* write */
    NULL,               /* dump */
    NULL,               /* flush */
    NULL,               /* bufferspace */
    NULL,               /* close */
    NULL,               /* suspend */
    NULL,               /* resume */
    0,                  /* need_attenuation */
    2,                  /* max_channels */
    true                /* is_timing_source */
};

int sound_init_retro_device(void)
{
    return sound_register_device(&retro_device);
}

