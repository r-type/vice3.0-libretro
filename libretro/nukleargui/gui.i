/* nuklear - v1.00 - public domain */
typedef enum
{
    GUI_NONE = 0,
    GUI_VKBD
} GUI_ACTION;

int GUISTATE = GUI_NONE;

static int gui(struct nk_context *ctx)
{
    GUISTATE = GUI_NONE;
    if (SHOWKEY==1)
        GUISTATE = GUI_VKBD;

    struct nk_color key_color_text_normal       = ctx->style.button.text_normal;
    struct nk_color key_color_text_hover        = ctx->style.button.text_hover;
    struct nk_color key_color_text_active       = ctx->style.button.text_active;
    struct nk_style_item key_color_default      = ctx->style.button.normal;
    struct nk_style_item key_color_hover        = ctx->style.button.hover;
    struct nk_style_item key_color_active       = ctx->style.button.active;
    struct nk_style_item key_color_function     = ctx->style.window.header.normal;
    struct nk_style_item key_color_hotkey       = ctx->style.property.normal;
    struct nk_style_item key_color_sticky       = ctx->style.slider.cursor_normal;
    struct nk_style_item key_color_datasette    = ctx->style.slider.cursor_hover;
    struct nk_style_item key_color_reset        = ctx->style.slider.cursor_active;

    key_color_reset = nk_style_item_color(nk_rgba(128,   0,   0, vkbd_alpha));

    switch (GUISTATE)
    {
        case GUI_VKBD:
            switch (opt_vkbd_theme)
            {
                default:
                case 0: // C64
                    key_color_text_normal   = nk_rgba(250, 250, 250, 255);
                    key_color_text_hover    = nk_rgba(  4,   4,   4, 255);
                    key_color_text_active   = nk_rgba(255, 255, 255, 255);
                    key_color_default       = nk_style_item_color(nk_rgba( 69,  59,  58, vkbd_alpha));
                    key_color_hover         = nk_style_item_color(nk_rgba(160, 160, 160, vkbd_alpha));
                    key_color_active        = nk_style_item_color(nk_rgba( 48,  44,  45, vkbd_alpha));
                    key_color_function      = nk_style_item_color(nk_rgba(123, 127, 130, vkbd_alpha));
                    key_color_hotkey        = nk_style_item_color(nk_rgba(144, 141, 129, vkbd_alpha));
                    key_color_sticky        = nk_style_item_color(nk_rgba( 40,  36,  37, 240));
                    key_color_datasette     = nk_style_item_color(nk_rgba( 89,  79,  78, vkbd_alpha));
                    break;

                case 1: // C64C
                    key_color_text_normal   = nk_rgba(  4,   4,   4, 255);
                    key_color_text_hover    = nk_rgba(250, 250, 250, 255);
                    key_color_text_active   = nk_rgba(  8,   8,   8, 255);
                    key_color_default       = nk_style_item_color(nk_rgba(216, 209, 201, vkbd_alpha));
                    key_color_hover         = nk_style_item_color(nk_rgba( 60,  60,  60, vkbd_alpha));
                    key_color_active        = nk_style_item_color(nk_rgba(250, 250, 250, vkbd_alpha));
                    key_color_function      = nk_style_item_color(nk_rgba(157, 152, 149, vkbd_alpha));
                    key_color_hotkey        = nk_style_item_color(nk_rgba(144, 141, 129, vkbd_alpha));
                    key_color_sticky        = nk_style_item_color(nk_rgba(255, 255, 255, 240));
                    key_color_datasette     = nk_style_item_color(nk_rgba(109,  99,  98, vkbd_alpha));
                    break;

                case 2: // Dark
                    key_color_text_normal   = nk_rgba(250, 250, 250, 255);
                    key_color_text_hover    = nk_rgba(  4,   4,   4, 255);
                    key_color_text_active   = nk_rgba(255, 255, 255, 255);
                    key_color_default       = nk_style_item_color(nk_rgba( 32,  32,  32, vkbd_alpha));
                    key_color_hover         = nk_style_item_color(nk_rgba(140, 140, 140, vkbd_alpha));
                    key_color_active        = nk_style_item_color(nk_rgba( 16,  16,  16, 224));
                    key_color_function      = nk_style_item_color(nk_rgba( 80,  80,  80, vkbd_alpha));
                    key_color_hotkey        = nk_style_item_color(nk_rgba( 16,  16,  16, vkbd_alpha));
                    key_color_sticky        = nk_style_item_color(nk_rgba(  8,   8,   8, 240));
                    key_color_datasette     = nk_style_item_color(nk_rgba( 50,  50,  50, vkbd_alpha));
                    break;

                case 3: // Light
                    key_color_text_normal   = nk_rgba(  4,   4,   4, 255);
                    key_color_text_hover    = nk_rgba(250, 250, 250, 255);
                    key_color_text_active   = nk_rgba(  8,   8,   8, 255);
                    key_color_default       = nk_style_item_color(nk_rgba(220, 220, 220, vkbd_alpha));
                    key_color_hover         = nk_style_item_color(nk_rgba(140, 140, 140, vkbd_alpha));
                    key_color_active        = nk_style_item_color(nk_rgba(255, 255, 255, 224));
                    key_color_function      = nk_style_item_color(nk_rgba(180, 180, 180, vkbd_alpha));
                    key_color_hotkey        = nk_style_item_color(nk_rgba(160, 160, 160, vkbd_alpha));
                    key_color_sticky        = nk_style_item_color(nk_rgba(255, 255, 255, 240));
                    key_color_datasette     = nk_style_item_color(nk_rgba(190, 190, 190, vkbd_alpha));
                    break;
            }

            if (nk_begin(ctx, "VKBD", GUIRECT, NK_WINDOW_NO_SCROLLBAR))
            {
                /* Ensure VKBD is centered regardless of border setting */
                if (retro_get_borders())
                {
                    offset.x = 0;
                    offset.y = 0;
                }
                else
                {
                    offset.x = GUIRECT.x;
                    offset.y = GUIRECT.y;
#if defined(__VIC20__)
                    if (retro_get_region() == RETRO_REGION_NTSC)
                    {
                        offset.x -= 16;
                        offset.y -= 26;
                    }
#elif defined(__PLUS4__)
                    offset.y += 5;
                    if (retro_get_region() == RETRO_REGION_NTSC)
                        offset.y -= 22;
#else
                    if (retro_get_region() == RETRO_REGION_NTSC)
                        offset.y -= 12;
#endif
                    if (zoom_mode_id < 3)
                        offset.y += 4;
                }

                if (opt_statusbar & STATUSBAR_TOP && (zoom_mode_id == 3 || retro_get_borders()))
                    offset.y += 8;

                nk_window_set_position(ctx, offset);
                #include "vkboard.i"
                nk_end(ctx);
            }
            break;

        default:
            break;
    }
    return GUISTATE;
}
