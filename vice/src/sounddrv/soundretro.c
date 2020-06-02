/*
 * soundretro.c - Simple forwarding sound device for libretro
 *
 */

#include "vice.h"

#include <stdio.h>

#include "sound.h"

#include "libretro-core.h"

static int retro_sound_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    *speed = core_opt.SoundSampleRate;
    //*fragsize = 32;
    //*fragnr = 0;
    //*channels = 1;
    //printf("speed:%d fragsize:%d fragnr:%d channels:%d\n", *speed, *fragsize, *fragnr, *channels);
    return 0;
}

static int retro_write(SWORD *pbuf, size_t nr)
{
    //printf("pbuf:%d nr:%d\n", *pbuf, nr);
    retro_audio_render(pbuf, nr);
    return 0;
}

static int retro_flush(char *state)
{
    //printf("flush\n");
    return 0;
}

static sound_device_t retro_device =
{
    "retro",
    retro_sound_init,
    retro_write,
    NULL,
    retro_flush,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    1
};

int sound_init_retro_device(void)
{
    return sound_register_device(&retro_device);
}
