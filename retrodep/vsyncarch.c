/*
 * vsyncarch.c - End-of-frame handling for libretro
 *
 */

#include "vice.h"

#include "kbdbuf.h"
#include "ui.h"
#include "uistatusbar.h"
#include "vsyncapi.h"
#include "videoarch.h"
#include "video.h"
#include "resources.h"

#include "libretro-core.h"

#if defined(VITA)
#include <psp2/kernel/threadmgr.h>
#endif

extern struct video_canvas_s *retro_canvas;

#include <time.h>

/*#define TICKSPERSECOND  1000000L*/     /* Microseconds resolution. */
#ifdef HAVE_NANOSLEEP
#define TICKSPERSECOND  1000000000L  /* Nanoseconds resolution. */
#define TICKSPERMSEC    1000000L
#define TICKSPERUSEC    1000L
#define TICKSPERNSEC    1L
#else
#define TICKSPERSECOND  1000000L     /* Microseconds resolution. */
#define TICKSPERMSEC    1000L
#define TICKSPERUSEC    1L
#endif

/* ------------------------------------------------------------------------- */

/* Number of timer units per second. */
unsigned long vsyncarch_frequency(void)
{
    /* Microseconds resolution. */
    return TICKSPERSECOND;
}

/* Get time in timer units. */
unsigned long vsyncarch_gettime(void)
{
    /* Microseconds resolution. */
    return retro_ticks();
}

void vsyncarch_init(void)
{
}

void vsyncarch_presync(void)
{
    video_canvas_render(
        retro_canvas, (BYTE *)&retro_bmp,
        retrow, retroh,
        retroXS, retroYS,
        0, 0, /*xi, yi,*/
        retrow*pix_bytes, 8*pix_bytes
    );
                        
    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_draw();
    }

    retro_renderloop = 0;
}

void vsyncarch_postsync(void)
{
    /* Dispatch all the pending UI events.  */
    ui_dispatch_events();
}
