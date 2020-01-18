int x = 0,y = 0;

#if defined(__VIC20__)
ctx->style.window.padding   = nk_vec2(0,1);
#else
ctx->style.window.padding   = nk_vec2(3,2);
#endif
ctx->style.window.spacing   = nk_vec2(2,2);

ctx->style.button.padding   = nk_vec2(0,0);
ctx->style.button.border    = 0;
ctx->style.button.rounding  = 0;

struct nk_style_item key_color_default      = ctx->style.button.normal;
struct nk_style_item key_color_active       = ctx->style.button.active;
struct nk_style_item key_color_function     = ctx->style.window.header.normal;
struct nk_style_item key_color_hotkey       = ctx->style.property.normal;
struct nk_style_item key_color_sticky       = ctx->style.slider.cursor_normal;
struct nk_style_item key_color_datasette    = ctx->style.slider.cursor_hover;
struct nk_style_item key_color_reset        = ctx->style.slider.cursor_active;

#if defined(__VIC20__)
nk_layout_row_dynamic(ctx, 23, NPLGN);
#else
nk_layout_row_dynamic(ctx, 25, NPLGN);
#endif
nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);

/* Key label shifted */
bool shifted = false;
/* Key label shifted */
shifted = false;
if (SHIFTON == 1 || vkey_sticky1 == RETROK_LSHIFT || vkey_sticky2 == RETROK_LSHIFT || vkey_sticky1 == RETROK_RSHIFT || vkey_sticky2 == RETROK_RSHIFT)
    shifted = true;
if (vkey_pressed != -1 && (vkey_pressed == RETROK_LSHIFT || vkey_pressed == RETROK_RSHIFT))
    shifted = true;

vkey_pressed = -1;

for (y=0; y<NLIGN; y++)
{
    for (x=0; x<NPLGN; x++)
    {
        /* Reset default color */
        ctx->style.button.normal = key_color_default;
        ctx->style.button.active = key_color_active;

        /* Function key color */
        if (MVk[(y*NPLGN)+x].val == RETROK_F1
         || MVk[(y*NPLGN)+x].val == RETROK_F3
         || MVk[(y*NPLGN)+x].val == RETROK_F5
         || MVk[(y*NPLGN)+x].val == RETROK_F7
        )
            ctx->style.button.normal = key_color_function;

        /* Hotkey color */
        if (MVk[(y*NPLGN)+x].val == -3
         || MVk[(y*NPLGN)+x].val == -4
         || MVk[(y*NPLGN)+x].val == -20
        )
            ctx->style.button.normal = key_color_hotkey;

        /* Datasette color */
        if (MVk[(y*NPLGN)+x].val <= -11 && MVk[(y*NPLGN)+x].val >= -15)
            ctx->style.button.normal = key_color_datasette;

        /* Reset color */
        if (MVk[(y*NPLGN)+x].val == -2)
            ctx->style.button.normal = key_color_reset;

        /* Sticky color */
        if (MVk[(y*NPLGN)+x].val == vkey_sticky1
         || MVk[(y*NPLGN)+x].val == vkey_sticky2
         || (MVk[(y*NPLGN)+x].val == -5 && SHIFTON==1)
        )
        {
            ctx->style.button.normal = key_color_sticky;
            ctx->style.button.active = key_color_sticky;
        }

        if (nk_button_text(ctx, !shifted ? MVk[(y*NPLGN)+x].norml : MVk[(y*NPLGN)+x].shift, \
            !shifted ? strlen(MVk[(y*NPLGN)+x].norml) : strlen(MVk[(y*NPLGN)+x].shift)))
        {
            vkey_pressed=MVk[(y*NPLGN)+x].val;
        }
    }
}

