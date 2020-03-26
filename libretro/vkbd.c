#include "libretro-core.h"
#include "vkbd_def.h"
#include "graph.h"

extern int NPAGE;
extern int SHOWKEYTRANS;
extern int SHIFTON;
extern int vkflag[8];
extern unsigned int opt_vkbd_theme;
extern unsigned int opt_vkbd_alpha;
extern unsigned int zoom_mode_id;

int RGB(int r, int g, int b)
{
   if (pix_bytes == 4) 
      return ARGB888(255, r, g, b);
   else
      return RGB565(r, g, b);
}

int BKG_PADDING(int len)
{
   static int font_width = 6;
   switch(len)
   {
      case 2:
         return -font_width;
         break;
      case 3:
         return -((font_width / 2) * 3);
         break;
   }
}

void print_virtual_kbd(unsigned short int *pixels)
{
   int x, y;
   bool shifted;
   int page             = (NPAGE == -1) ? 0 : NPLGN * NLIGN;
   uint16_t *pix        = &pixels[0];

   int XKEY             = 0;
   int YKEY             = 0;
   int XTEXT            = 0;
   int YTEXT            = 0;
   int XOFFSET          = 0;
   int XPADDING         = 0;
   int YOFFSET          = 0;
   int YPADDING         = 0;
   int XKEYSPACING      = 1;
   int YKEYSPACING      = 1;
   int ALPHA            = opt_vkbd_alpha;
   int BKG_ALPHA        = ALPHA;
   int BKG_PADDING_X    = 0;
   int BKG_PADDING_Y    = 0;
   int BKG_COLOR        = 0;
   int BKG_COLOR_NORMAL = 0;
   int BKG_COLOR_ALT    = 0;
   int BKG_COLOR_EXTRA  = 0;
   int BKG_COLOR_EXTRA2 = 0;
   int BKG_COLOR_SEL    = 0;
   int BKG_COLOR_ACTIVE = 0;
   int FONT_MAX         = 3;
   int FONT_WIDTH       = 1;
   int FONT_HEIGHT      = 1;
   int FONT_COLOR       = 0;
   int FONT_COLOR_NORMAL= 0;
   int FONT_COLOR_SEL   = 0;

   switch (opt_vkbd_theme)
   {
      default:
      case 0: // C64
         BKG_COLOR_NORMAL  = RGB( 68,  59,  58);
         BKG_COLOR_ALT     = RGB(123, 127, 130);
         BKG_COLOR_EXTRA   = RGB(143, 140, 129);
         BKG_COLOR_EXTRA2  = RGB( 89,  79,  78);
         BKG_COLOR_SEL     = RGB(160, 160, 160);
         BKG_COLOR_ACTIVE  = RGB( 48,  44,  45);
         FONT_COLOR_NORMAL = RGB(250, 250, 250);
         FONT_COLOR_SEL    = RGB( 10,  10,  10);
         break;
      
      case 1: // C64C
         BKG_COLOR_NORMAL  = RGB(216, 209, 201);
         BKG_COLOR_ALT     = RGB(159, 154, 150);
         BKG_COLOR_EXTRA   = RGB(143, 140, 129);
         BKG_COLOR_EXTRA2  = RGB(109,  99,  98);
         BKG_COLOR_SEL     = RGB( 60,  60,  60);
         BKG_COLOR_ACTIVE  = RGB(250, 250, 250);
         FONT_COLOR_NORMAL = RGB( 10,  10,  10);
         FONT_COLOR_SEL    = RGB(250, 250, 250);
         break;
      
      case 2: // Dark
         BKG_COLOR_NORMAL  = RGB( 32,  32,  32);
         BKG_COLOR_ALT     = RGB( 70,  70,  70);
         BKG_COLOR_EXTRA   = RGB( 14,  14,  14);
         BKG_COLOR_EXTRA2  = RGB( 50,  50,  50);
         BKG_COLOR_SEL     = RGB(140, 140, 140);
         BKG_COLOR_ACTIVE  = RGB( 16,  16,  16);
         FONT_COLOR_NORMAL = RGB(250, 250, 250);
         FONT_COLOR_SEL    = RGB( 10,  10,  10);
         break;
      
      case 3: // Light
         BKG_COLOR_NORMAL  = RGB(220, 220, 220);
         BKG_COLOR_ALT     = RGB(180, 180, 180);
         BKG_COLOR_EXTRA   = RGB(160, 160, 160);
         BKG_COLOR_EXTRA2  = RGB(190, 190, 190);
         BKG_COLOR_SEL     = RGB( 60,  60,  60);
         BKG_COLOR_ACTIVE  = RGB(250, 250, 250);
         FONT_COLOR_NORMAL = RGB( 10,  10,  10);
         FONT_COLOR_SEL    = RGB(250, 250, 250);
         break;
   }

   int BKG_PADDING_X_DEFAULT = -3;
   int BKG_PADDING_Y_DEFAULT = -3;

#if defined(__VIC20__)
   if (retro_get_borders())
   {
      XOFFSET           = 4;
      YOFFSET           = -4;
      XPADDING          = 4;
      YPADDING          = 8;
      
      if (opt_statusbar & STATUSBAR_TOP)
         YOFFSET        = 4;
   }
   else
   {
      if (retro_get_region() == RETRO_REGION_NTSC)
      {
         XOFFSET           = 9;
         YOFFSET           = -3;
         XPADDING          = 44;
         YPADDING          = 58;

         if (zoom_mode_id == 3)
         {
            if (opt_statusbar & STATUSBAR_TOP)
               YOFFSET = -2;
            else
               YOFFSET = -7;
         }
      }
      else
      {
         XOFFSET           = 4;
         YOFFSET           = -3;
         XPADDING          = 98;
         YPADDING          = 108;

         if (zoom_mode_id == 3)
         {
            if (opt_statusbar & STATUSBAR_TOP)
               YOFFSET = 2;
            else
               YOFFSET = -6;
         }
      }
   }
#elif defined(__PLUS4__)
   if (retro_get_borders())
   {
      XOFFSET           = 4;
      YOFFSET           = -3;
      XPADDING          = 4;
      YPADDING          = 8;
      
      if (opt_statusbar & STATUSBAR_TOP)
         YOFFSET        = 5;
   }
   else
   {
      if (retro_get_region() == RETRO_REGION_NTSC)
      {
         XOFFSET           = 0;
         YOFFSET           = -3;
         XPADDING          = 64;
         YPADDING          = 52;

         if (zoom_mode_id == 3)
         {
            if (opt_statusbar & STATUSBAR_TOP)
               YOFFSET = 1;
            else
               YOFFSET = -7;
         }
      }
      else
      {
         XOFFSET           = 4;
         YOFFSET           = -3;
         XPADDING          = 66;
         YPADDING          = 96;

         if (zoom_mode_id == 3)
         {
            if (opt_statusbar & STATUSBAR_TOP)
               YOFFSET = 1;
            else
               YOFFSET = -7;
         }      
      }
   }
#else
   if (retro_get_borders())
   {
      XOFFSET           = 1;
      YOFFSET           = -4;
      XPADDING          = 8;
      YPADDING          = 10;
      
      if (opt_statusbar & STATUSBAR_TOP)
         YOFFSET        = 4;
   }
   else
   {
      XOFFSET           = 0;
      YOFFSET           = 1;
      XPADDING          = 74;
      YPADDING          = 78;

      if (retro_get_region() != RETRO_REGION_NTSC)
      {
         if (zoom_mode_id == 3)
            YOFFSET = -3;
      }

      if (opt_statusbar & STATUSBAR_TOP)
         YOFFSET = 5;
   }
#endif

   int XSIDE = (retroW - XPADDING) / NPLGN;
   int YSIDE = (retroH - YPADDING) / NLIGN;

   int XBASEKEY = (XPADDING > 0) ? (XPADDING / 2) : 0;
   int YBASEKEY = (YPADDING > 0) ? (YPADDING / 2) : 0;

   int XBASETEXT = ((XPADDING > 0) ? (XPADDING / 2) : 0) + (XSIDE / 2);
   int YBASETEXT = YBASEKEY + (YSIDE / 2);

   /* Coordinates */
   vkbd_x_min = XBASEKEY + XKEYSPACING;
   vkbd_x_max = -XBASEKEY + retroW - XKEYSPACING;
   vkbd_y_min = YOFFSET + YBASEKEY + YKEYSPACING;
   vkbd_y_max = YOFFSET + YBASEKEY + (YSIDE * NLIGN);

   /* Opacity */
   BKG_ALPHA = (SHOWKEYTRANS == -1) ? 255 : ALPHA;

   /* Alternate color keys */
   int alt_keys[] =
   {
      RETROK_F1, RETROK_F3, RETROK_F5, RETROK_F7,
   };
   int alt_keys_len = sizeof(alt_keys) / sizeof(alt_keys[0]);

   /* Extra color keys */
   int extra_keys[] =
   {
      -3 /* STB */ , -4 /* JOY */, -20 /* TTF */
   };
   int extra_keys_len = sizeof(extra_keys) / sizeof(extra_keys[0]);

   /* Datasette color keys */
   int datasette_keys[] =
   {
      -11, -12, -13, -14, -15
   };
   int datasette_keys_len = sizeof(datasette_keys) / sizeof(datasette_keys[0]);

   /* Key label shifted */
   shifted = false;
   if (SHIFTON == 1 || vkey_sticky1 == RETROK_LSHIFT || vkey_sticky2 == RETROK_LSHIFT || vkey_sticky1 == RETROK_RSHIFT || vkey_sticky2 == RETROK_RSHIFT)
      shifted = true;
   if (vkflag[4] == 1 && (vkey_pressed == RETROK_LSHIFT || vkey_pressed == RETROK_RSHIFT))
      shifted = true;

   /* Key layout */
   for (x = 0; x < NPLGN; x++)
   {
      for (y = 0; y < NLIGN; y++)
      {
         /* Default key color */
         BKG_COLOR = BKG_COLOR_NORMAL;

         /* Reset key color */
         if (MVk[(y * NPLGN) + x].val == -2)
            BKG_COLOR = RGB(128, 0, 0);
         else
         {
            /* Alternate key color */
            for (int alt_key = 0; alt_key < alt_keys_len; ++alt_key)
                if (alt_keys[alt_key] == MVk[(y * NPLGN) + x + page].val)
                    BKG_COLOR = BKG_COLOR_ALT;

            /* Extra key color */
            for (int extra_key = 0; extra_key < extra_keys_len; ++extra_key)
                if (extra_keys[extra_key] == MVk[(y * NPLGN) + x + page].val)
                    BKG_COLOR = BKG_COLOR_EXTRA;

            /* Datasette key color */
            for (int datasette_key = 0; datasette_key < datasette_keys_len; ++datasette_key)
                if (datasette_keys[datasette_key] == MVk[(y * NPLGN) + x + page].val)
                    BKG_COLOR = BKG_COLOR_EXTRA2;
         }

         /* Key centering */
         BKG_PADDING_X     = BKG_PADDING_X_DEFAULT;
         BKG_PADDING_Y     = BKG_PADDING_Y_DEFAULT;
         if (strlen(MVk[(y * NPLGN) + x + page].norml) > 1)
            BKG_PADDING_X = BKG_PADDING(strlen(MVk[(y * NPLGN) + x + page].norml));

         /* Key positions */
         XKEY  = XBASEKEY + (x * XSIDE) + XOFFSET;
         XTEXT = XBASETEXT + BKG_PADDING_X + (x * XSIDE) + XOFFSET;
         YKEY  = YOFFSET + YBASEKEY + (y * YSIDE);
         YTEXT = YOFFSET + YBASETEXT + BKG_PADDING_Y + (y * YSIDE);

         /* Default font color */
         FONT_COLOR = FONT_COLOR_NORMAL;

         /* Sticky + CapsLock + pressed colors */
         if ((vkey_sticky1 == MVk[(y * NPLGN) + x + page].val
          ||  vkey_sticky2 == MVk[(y * NPLGN) + x + page].val
          ||   (SHIFTON==1 && MVk[(y * NPLGN) + x + page].val==-5)
          || (vkflag[7]==1 && MVk[(y * NPLGN) + x + page].val==RETROK_RETURN))
          && BKG_COLOR != BKG_COLOR_EXTRA && MVk[(y * NPLGN) + x + page].val != -2)
         {
            FONT_COLOR = FONT_COLOR_NORMAL;
            BKG_COLOR = BKG_COLOR_ACTIVE;
         }

         /* Key background */
         if (pix_bytes == 4)
            DrawFBoxBmp32((uint32_t *)pix, XKEY+XKEYSPACING, YKEY+YKEYSPACING, XSIDE-XKEYSPACING, YSIDE-YKEYSPACING, BKG_COLOR, BKG_ALPHA);
         else
            DrawFBoxBmp(pix, XKEY+XKEYSPACING, YKEY+YKEYSPACING, XSIDE-XKEYSPACING, YSIDE-YKEYSPACING, BKG_COLOR, BKG_ALPHA);

         /* Key text shadow */
         if (pix_bytes == 4)
            Draw_text32((uint32_t *)pix, (FONT_COLOR_SEL == RGB(250, 250, 250) ? XTEXT+FONT_WIDTH : XTEXT-FONT_WIDTH), (FONT_COLOR_SEL == RGB(250, 250, 250) ? YTEXT+FONT_WIDTH : YTEXT-FONT_WIDTH), (FONT_COLOR_SEL == RGB(250, 250, 250) ? RGB(80, 80, 80) : RGB(50, 50, 50)), BKG_COLOR, 100, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
               (!shifted) ? MVk[(y * NPLGN) + x + page].norml : MVk[(y * NPLGN) + x + page].shift);
         else
            Draw_text(pix, (FONT_COLOR_SEL == RGB(250, 250, 250) ? XTEXT+FONT_WIDTH : XTEXT-FONT_WIDTH), (FONT_COLOR_SEL == RGB(250, 250, 250) ? YTEXT+FONT_WIDTH : YTEXT-FONT_WIDTH), (FONT_COLOR_SEL == RGB(250, 250, 250) ? RGB(80, 80, 80) : RGB(50, 50, 50)), BKG_COLOR, 100, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
               (!shifted) ? MVk[(y * NPLGN) + x + page].norml : MVk[(y * NPLGN) + x + page].shift);

         /* Key text */
         if (pix_bytes == 4)
         {
            Draw_text32((uint32_t *)pix, XTEXT, YTEXT, FONT_COLOR, BKG_COLOR, 220, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
               (!shifted) ? MVk[(y * NPLGN) + x + page].norml : MVk[(y * NPLGN) + x + page].shift);
         }
         else
         {
            Draw_text(pix, XTEXT, YTEXT, FONT_COLOR, BKG_COLOR, 220, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
               (!shifted) ? MVk[(y * NPLGN) + x + page].norml : MVk[(y * NPLGN) + x + page].shift);
         }
      }
   }

   /* Key centering */
   BKG_PADDING_X     = BKG_PADDING_X_DEFAULT;
   BKG_PADDING_Y     = BKG_PADDING_Y_DEFAULT;
   if (strlen(MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].norml) > 1)
      BKG_PADDING_X = BKG_PADDING(strlen(MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].norml));

   /* Selected key position */
   XKEY  = XBASEKEY + (vkey_pos_x * XSIDE) + XOFFSET;
   XTEXT = XBASETEXT + BKG_PADDING_X + (vkey_pos_x * XSIDE) + XOFFSET;
   YKEY  = YOFFSET + YBASEKEY + (vkey_pos_y * YSIDE);
   YTEXT = YOFFSET + YBASETEXT + BKG_PADDING_Y + (vkey_pos_y * YSIDE);

   /* Opacity */
   BKG_ALPHA = (SHOWKEYTRANS == -1) ? 250 : 220;

   /* Pressed key color */
   if (vkflag[4] == 1 && (MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].val == vkey_sticky1 || MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].val == vkey_sticky2))
      ; // no-op
   else if (vkflag[4] == 1)
      BKG_COLOR_SEL = BKG_COLOR_ACTIVE;
   else
      FONT_COLOR = FONT_COLOR_SEL;

   /* Selected key background */
   if (pix_bytes == 4)
      DrawFBoxBmp32((uint32_t *)pix, XKEY+XKEYSPACING, YKEY+YKEYSPACING, XSIDE-XKEYSPACING, YSIDE-YKEYSPACING, BKG_COLOR_SEL, BKG_ALPHA);
   else
      DrawFBoxBmp(pix, XKEY+XKEYSPACING, YKEY+YKEYSPACING, XSIDE-XKEYSPACING, YSIDE-YKEYSPACING, BKG_COLOR_SEL, BKG_ALPHA);

   /* Selected key text */
   if (pix_bytes == 4)
   {
      Draw_text32((uint32_t *)pix, XTEXT, YTEXT, FONT_COLOR, 0, BKG_ALPHA, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
         (!shifted) ? MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].norml : MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].shift);
   }
   else
   {
      Draw_text(pix, XTEXT, YTEXT, FONT_COLOR, 0, BKG_ALPHA, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
         (!shifted) ? MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].norml : MVk[(vkey_pos_y * NPLGN) + vkey_pos_x + page].shift);
   }

#ifdef POINTER_DEBUG
   if (pix_bytes == 4)
      DrawHlineBmp32((uint32_t *)retro_bmp, pointer_x, pointer_y, 1, 1, RGB888(30, 0, 30));
   else
      DrawHlineBmp(retro_bmp, pointer_x, pointer_y, 1, 1, RGB565(30, 0, 30));
#endif
}

int check_vkey(int x, int y)
{
   /* Check which key is pressed */
   int page = (NPAGE == -1) ? 0 : NPLGN * NLIGN;
   return MVk[(y * NPLGN) + x + page].val;
}
