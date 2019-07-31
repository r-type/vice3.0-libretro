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
extern int retro_ui_finalized;

extern void emu_reset(void);
extern void retro_shutdown_core(void);

static int
gui(struct nk_context *ctx)
{
    struct nk_rect total_space;

    /* window flags */
    static int border = nk_false;
    static int resize = nk_false;
    static int movable = nk_false;
    static int no_scrollbar = nk_false;
    static int minimizable = nk_false;
    static int title = nk_false;

    /* window flags */
    static nk_flags window_flags = 0;
    window_flags = 0;

    if (border) window_flags |= NK_WINDOW_BORDER;
    if (resize) window_flags |= NK_WINDOW_SCALABLE;
    if (movable) window_flags |= NK_WINDOW_MOVABLE;
    if (no_scrollbar || GUISTATE==GUI_VKBD) window_flags |= NK_WINDOW_NO_SCROLLBAR;
    if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;
    if (title) window_flags |= NK_WINDOW_TITLE;

    int tmpval;

    int border_disabled = 0;
    if(SHOWKEY==1)
    {
        GUISTATE = GUI_VKBD;

        /* this code is needed because changing borders on/off causes a reset */
        if(retro_ui_finalized)
#if defined(__VIC20__)
            resources_get_int("VICBorderMode", &border_disabled);
#elif defined(__PLUS4__)
            resources_get_int("TEDBorderMode", &border_disabled);
#else
            resources_get_int("VICIIBorderMode", &border_disabled);
#endif
        else
            border_disabled = RETROBORDERS;
    }

    switch(GUISTATE)
    {
        case GUI_VKBD:
            if (nk_begin(ctx,"Vice Keyboard", GUIRECT, window_flags)) {
                #include "vkboard.i"
                /* ensure vkbd is centered regardless of border setting */
                if (border_disabled) {
                    offset.x = 0;
                    offset.y = 0;
                } else {
                    offset.x = GUIRECT.x;
                    offset.y = GUIRECT.y;
                }
                nk_window_set_position(ctx, offset);
                nk_end(ctx);
            }
            break;

        default:
            break;
    }
    return GUISTATE;
}

