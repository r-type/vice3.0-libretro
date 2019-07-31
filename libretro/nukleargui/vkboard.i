int x = 0,y = 0;

char *name;
ctx->style.window.padding = nk_vec2(10,2);
ctx->style.window.spacing = nk_vec2(2,2);

ctx->style.button.padding = nk_vec2(1,0);
ctx->style.button.border = 0;
ctx->style.button.rounding = 1.0f;

struct nk_style_item key_color_default = ctx->style.button.normal;
struct nk_style_item key_color_alt = nk_style_item_color(nk_rgba(123, 127, 130, 255));

nk_layout_row_dynamic(ctx, 26, NPLGN);
nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);

vkey_pressed = -1;

for(y=0;y<NLIGN;y++)
{
      for(x=0;x<NPLGN;x++)
      {
             // Reset default color
             ctx->style.button.normal = key_color_default;

             // Function key color
             name = MVk[(y*NPLGN)+x].norml;
             if (strcmp(name, "F1") == 0 || strcmp(name, "F3") == 0 || strcmp(name, "F5") == 0 || strcmp(name, "F7") == 0)
                    ctx->style.button.normal = key_color_alt;

             if (nk_button_text(ctx, SHIFTON == -1 ? MVk[(y*NPLGN)+x].norml : MVk[(y*NPLGN)+x].shift , \
             SHIFTON == -1 ? strlen(MVk[(y*NPLGN)+x].norml) : strlen(MVk[(y*NPLGN)+x].shift)))
             {
                    LOGI("(%s) pressed! (%d,%d)\n", SHIFTON == -1 ? MVk[(y*NPLGN)+x].norml : MVk[(y*NPLGN)+x].shift,x,y);
                    vkey_pressed=MVk[(y*NPLGN)+x].val;
             }
      }
}