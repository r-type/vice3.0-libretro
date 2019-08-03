/* nuklear - v1.00 - public domain */

/* VICE HEADER */
#include "ui.h"
#include "log.h"
#include "resources.h"

#define NUMB(a) (sizeof(a) / sizeof(*a))

#define GUIRECT nk_rect(32, 35, 319, 199)

typedef enum
{
    GUI_NONE = 0,
    GUI_VKBD
} GUI_ACTION;

int GUISTATE = GUI_NONE;

extern int SHIFTON;
extern int vkey_pressed;
extern int RETROBORDERS;
extern int RETROTHEME;
extern int retro_ui_finalized;

extern void emu_reset(void);
extern void retro_shutdown_core(void);
extern unsigned retro_get_borders(void);

static int gui(struct nk_context *ctx)
{
    GUISTATE = GUI_NONE;
    if(SHOWKEY==1)
        GUISTATE = GUI_VKBD;

    switch(GUISTATE)
    {
        case GUI_VKBD:
            if (nk_begin(ctx,"Vice Keyboard", GUIRECT, NK_WINDOW_NO_SCROLLBAR)) {

#ifndef __ANDROID__
                if(RETROTHEME==1)
                  set_style(ctx, THEME_C64C);
                else
                  set_style(ctx, THEME_C64);
                
                /* ensure vkbd is centered regardless of border setting */
                if (retro_get_borders()) {
                    offset.x = 0;
                    offset.y = 0;
                } else {
                    offset.x = GUIRECT.x;
                    offset.y = GUIRECT.y;
                    if(retro_get_region() == RETRO_REGION_NTSC) offset.y -= 12;
                }
                nk_window_set_position(ctx, offset);
#endif
                #include "vkboard.i"
                nk_end(ctx);
            }
            break;

        default:
            break;
    }
    return GUISTATE;
}

