#include "libretro.h"
#include "libretro-core.h"
#include "libretro-mapper.h"
#include "libretro-vkbd.h"

#include "archdep.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "mouse.h"
#include "resources.h"
#include "autostart.h"
#include "datasette.h"
#include "kbd.h"
#include "mousedrv.h"

static retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

#ifdef POINTER_DEBUG
int pointer_x = 0;
int pointer_y = 0;
#endif
int last_pointer_x = 0;
int last_pointer_y = 0;

/* VKBD starting point: 10x3 == f7 */
int vkey_pos_x = 10;
int vkey_pos_y = 3;
int vkbd_x_min = 0;
int vkbd_x_max = 0;
int vkbd_y_min = 0;
int vkbd_y_max = 0;

/* Mouse speed flags */
#define MOUSE_SPEED_SLOWER 1
#define MOUSE_SPEED_FASTER 2
/* Mouse speed multipliers */
#define MOUSE_SPEED_SLOW 5
#define MOUSE_SPEED_FAST 2
/* Mouse D-Pad acceleration */
#define MOUSE_DPAD_ACCEL

extern unsigned int opt_mouse_speed;
static void retro_mouse_move(int x, int y)
{
   mouse_move(
      (x * (float)opt_mouse_speed / 100.0f),
      (y * (float)opt_mouse_speed / 100.0f)
   );
}

/* Core flags */
int mapper_keys[RETRO_MAPPER_LAST] = {0};
int vkflag[10] = {0};
int retro_capslock = false;
unsigned int cur_port = 2;
static int cur_port_prev = -1;
bool cur_port_locked = false;
unsigned int mouse_value[2 + 1] = {0};
unsigned int mouse_speed[2] = {0};

unsigned int retro_statusbar = 0;
extern unsigned char statusbar_text[64];
unsigned int retro_warpmode = 0;
extern bool retro_vkbd;
extern bool retro_vkbd_transparent;
extern short int retro_vkbd_ready;
extern unsigned int retro_devices[RETRO_DEVICES];

static unsigned retro_key_state[RETROK_LAST] = {0};
unsigned retro_key_event_state[RETROK_LAST] = {0};
static int16_t joypad_bits[RETRO_DEVICES];
extern bool libretro_supports_bitmasks;
extern dc_storage *dc;

/* Core options */
extern unsigned int opt_retropad_options;
extern unsigned int opt_joyport_type;
static int opt_joyport_type_prev = -1;
extern unsigned int opt_dpadmouse_speed;
extern unsigned int opt_analogmouse;
extern unsigned int opt_analogmouse_deadzone;
extern float opt_analogmouse_speed;
bool datasette_hotkeys = false;

extern void emu_reset(int type);
extern unsigned int zoom_mode_id;
extern int zoom_mode_id_prev;
extern unsigned int opt_zoom_mode_id;
extern bool opt_keyrah_keypad;
extern bool opt_keyboard_pass_through;
extern unsigned int opt_aspect_ratio;
extern bool opt_aspect_ratio_locked;

bool retro_turbo_fire = false;
bool turbo_fire_locked = false;
unsigned int turbo_fire_button = 0;
unsigned int turbo_pulse = 6;
unsigned int turbo_state[RETRO_DEVICES] = {0};
unsigned int turbo_toggle[RETRO_DEVICES] = {0};

enum EMU_FUNCTIONS
{
   EMU_VKBD = 0,
   EMU_STATUSBAR,
   EMU_JOYPORT,
   EMU_RESET,
   EMU_SAVE_DISK,
   EMU_ASPECT_RATIO,
   EMU_ZOOM_MODE,
   EMU_TURBO_FIRE,
   EMU_WARP_MODE,
   EMU_DATASETTE_HOTKEYS,
   EMU_DATASETTE_STOP,
   EMU_DATASETTE_START,
   EMU_DATASETTE_FORWARD,
   EMU_DATASETTE_REWIND,
   EMU_DATASETTE_RESET,
   EMU_FUNCTION_COUNT
};

/* VKBD_MIN_HOLDING_TIME: Hold a direction longer than this and automatic movement sets in */
/* VKBD_MOVE_DELAY: Delay between automatic movement from button to button */
#define VKBD_MIN_HOLDING_TIME 200
#define VKBD_MOVE_DELAY 50
bool let_go_of_direction = true;
long last_move_time = 0;
long last_press_time = 0;

/* VKBD_STICKY_HOLDING_TIME: Button press longer than this triggers sticky key */
#define VKBD_STICKY_HOLDING_TIME 1000
int let_go_of_button = 1;
long last_press_time_button = 0;
int vkey_pressed = -1;
int vkey_sticky = -1;
int vkey_sticky1 = -1;
int vkey_sticky2 = -1;

void emu_function(int function)
{
   switch (function)
   {
      case EMU_VKBD:
         retro_vkbd = !retro_vkbd;
         /* Reset VKBD input readiness */
         retro_vkbd_ready = -2;
         /* Release VKBD controllable joypads */
         memset(joypad_bits, 0, 2*sizeof(joypad_bits[0]));
         break;
      case EMU_STATUSBAR:
         retro_statusbar = (retro_statusbar) ? 0 : 1;
         resources_set_int("SDLStatusbar", retro_statusbar);
         break;
      case EMU_JOYPORT:
#if defined(__XPET__) || defined(__XCBM2__) || defined(__XVIC__)
         break;
#endif
         cur_port++;
         if (cur_port > 2) cur_port = 1;
         /* Lock current port */
         cur_port_locked = true;
         /* Statusbar notification */
         snprintf(statusbar_text, 56, "%c Port %-48d",
               (' ' | 0x80), cur_port);
         imagename_timer = 50;
         break;
      case EMU_RESET:
         emu_reset(-1);
         break;
      case EMU_ASPECT_RATIO:
         if (opt_aspect_ratio == 0)
            opt_aspect_ratio = (retro_region == RETRO_REGION_NTSC) ? 1 : 2;
         opt_aspect_ratio++;
         if (opt_aspect_ratio > 3) opt_aspect_ratio = 1;
         /* Reset zoom */
         zoom_mode_id_prev = -1;
         /* Lock aspect ratio */
         opt_aspect_ratio_locked = true;
         /* Statusbar notification */
         snprintf(statusbar_text, 56, "%c Pixel Aspect %-40s",
               (' ' | 0x80), (opt_aspect_ratio == 1) ? "PAL" : (opt_aspect_ratio == 2) ? "NTSC" : "1:1");
         imagename_timer = 50;
         break;
      case EMU_ZOOM_MODE:
         if (zoom_mode_id == 0 && opt_zoom_mode_id == 0)
            break;
         if (zoom_mode_id > 0)
            zoom_mode_id = 0;
         else if (zoom_mode_id == 0)
            zoom_mode_id = opt_zoom_mode_id;
         /* Statusbar notification */
         snprintf(statusbar_text, 56, "%c Zoom Mode %-43s",
               (' ' | 0x80), (zoom_mode_id) ? "ON" : "OFF");
         imagename_timer = 50;
         break;
      case EMU_TURBO_FIRE:
         retro_turbo_fire = !retro_turbo_fire;
         /* Lock turbo fire */
         turbo_fire_locked = true;
         /* Statusbar notification */
         snprintf(statusbar_text, 56, "%c Turbo Fire %-42s",
               (' ' | 0x80), (retro_turbo_fire) ? "ON" : "OFF");
         imagename_timer = 50;
         break;
      case EMU_SAVE_DISK:
         dc_save_disk_toggle(dc, false, true);
         break;
      case EMU_WARP_MODE:
         retro_warpmode = (retro_warpmode) ? 0 : 1;
         resources_set_int("WarpMode", retro_warpmode);
         break;
      case EMU_DATASETTE_HOTKEYS:
         datasette_hotkeys = !datasette_hotkeys;
         /* Statusbar notification */
         snprintf(statusbar_text, 56, "%c Datasette Hotkeys %-35s",
               (' ' | 0x80), (datasette_hotkeys) ? "ON" : "OFF");
         imagename_timer = 50;
         break;

      case EMU_DATASETTE_STOP:
         datasette_control(DATASETTE_CONTROL_STOP);
         break;
      case EMU_DATASETTE_START:
         datasette_control(DATASETTE_CONTROL_START);
         break;
      case EMU_DATASETTE_FORWARD:
         datasette_control(DATASETTE_CONTROL_FORWARD);
         break;
      case EMU_DATASETTE_REWIND:
         datasette_control(DATASETTE_CONTROL_REWIND);
         break;
      case EMU_DATASETTE_RESET:
         datasette_control(DATASETTE_CONTROL_RESET);
         break;
   } 
}

void retro_key_up(int symkey)
{
   /* Prevent LShift keyup if ShiftLock is on */
   if (symkey == RETROK_LSHIFT)
   {
      if (!retro_capslock)
         kbd_handle_keyup(symkey);
   }
   else 
      kbd_handle_keyup(symkey);
}

void retro_key_down(int symkey)
{
   /* CapsLock / ShiftLock */
   if (symkey == RETROK_CAPSLOCK)
   {
      if (retro_capslock)
         kbd_handle_keyup(RETROK_LSHIFT);
      else
         kbd_handle_keydown(RETROK_LSHIFT);
      retro_capslock = !retro_capslock;
   }
   else
      kbd_handle_keydown(symkey);
}

void process_key(unsigned disable_keys)
{
   unsigned i = 0;

   for (i = RETROK_BACKSPACE; i < RETROK_LAST; i++)
   {
      if (disable_keys == 1 && (i == RETROK_UP || i == RETROK_DOWN || i == RETROK_LEFT || i == RETROK_RIGHT)
       || disable_keys == 2)
         retro_key_event_state[i] = 0;

      if (retro_key_event_state[i] && !retro_key_state[i])
      {
         /* Skip keydown if VKBD is active */
         if (retro_vkbd && i != RETROK_CAPSLOCK)
            continue;

         retro_key_state[i] = 1;
         retro_key_down(i);
      }
      else if (!retro_key_event_state[i] && retro_key_state[i])
      {
         retro_key_state[i] = 0;
         retro_key_up(i);
      }
   }
}

void retro_keyboard_event(bool down, unsigned code,
      uint32_t character, uint16_t mod)
{
   switch (code)
   {
      case RETROK_UNKNOWN:
      case RETROK_PAUSE:
         return;
   }

   retro_key_event_state[code] = down;
}

void update_input(unsigned disable_keys)
{
   /* RETRO  B  Y  SL ST UP DN LT RT A  X  L   R   L2  R2  L3  R3  LR  LL  LD  LU  RR  RL  RD  RU
    * INDEX  0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23
    */

   static long now = 0;
   static long last_vkey_pressed_time = 0;
   static int last_vkey_pressed = -1;
   static int vkey_sticky1_release = 0;
   static int vkey_sticky2_release = 0;

   static int i = 0, j = 0, mk = 0;
   static int LX = 0, LY = 0, RX = 0, RY = 0;
   const int threshold = 20000;

   static int jbt[2][24] = {0};
   static int kbt[EMU_FUNCTION_COUNT] = {0};
    
   now = retro_ticks() / 1000;

   if (vkey_sticky && last_vkey_pressed != -1 && last_vkey_pressed > 0)
   {
      if (vkey_sticky1 > -1 && vkey_sticky1 != last_vkey_pressed)
      {
         if (vkey_sticky2 > -1 && vkey_sticky2 != last_vkey_pressed)
            kbd_handle_keyup(vkey_sticky2);
         vkey_sticky2 = last_vkey_pressed;
      }
      else
         vkey_sticky1 = last_vkey_pressed;
   }

   /* Keyup only after button is up */
   if (last_vkey_pressed != -1 && !vkflag[RETRO_DEVICE_ID_JOYPAD_B])
   {
      if (vkey_pressed == -1 && last_vkey_pressed >= 0 && last_vkey_pressed != vkey_sticky1 && last_vkey_pressed != vkey_sticky2)
         kbd_handle_keyup(last_vkey_pressed);

      last_vkey_pressed = -1;
   }

   if (vkey_sticky1_release)
   {
      vkey_sticky1_release = 0;
      vkey_sticky1 = -1;
      kbd_handle_keyup(vkey_sticky1);
   }
   if (vkey_sticky2_release)
   {
      vkey_sticky2_release = 0;
      vkey_sticky2 = -1;
      kbd_handle_keyup(vkey_sticky2);
   }

   /* Iterate hotkeys, skip Datasette hotkeys if Datasette hotkeys are disabled or if VKBD is on */
   int i_last = (datasette_hotkeys && !retro_vkbd) ? RETRO_MAPPER_DATASETTE_RESET : RETRO_MAPPER_DATASETTE_HOTKEYS;
   i_last -= 24;

   for (i = 0; i <= i_last; i++)
   {
      mk = i + 24; /* Skip RetroPad mappings from mapper_keys */

      /* Key down */
      if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, mapper_keys[mk]) && !kbt[i] && mapper_keys[mk])
      {
         kbt[i] = 1;
         switch (mk)
         {
            case RETRO_MAPPER_VKBD:
               emu_function(EMU_VKBD);
               break;
            case RETRO_MAPPER_STATUSBAR:
               emu_function(EMU_STATUSBAR);
               break;
            case RETRO_MAPPER_JOYPORT:
               emu_function(EMU_JOYPORT);
               break;
            case RETRO_MAPPER_RESET:
               emu_function(EMU_RESET);
               break;
            case RETRO_MAPPER_ASPECT_RATIO:
               emu_function(EMU_ASPECT_RATIO);
               break;
            case RETRO_MAPPER_ZOOM_MODE:
               emu_function(EMU_ZOOM_MODE);
               break;
            case RETRO_MAPPER_WARP_MODE:
               emu_function(EMU_WARP_MODE);
               break;
            case RETRO_MAPPER_DATASETTE_HOTKEYS:
               emu_function(EMU_DATASETTE_HOTKEYS);
               break;

            case RETRO_MAPPER_DATASETTE_STOP:
               emu_function(EMU_DATASETTE_STOP);
               break;
            case RETRO_MAPPER_DATASETTE_START:
               emu_function(EMU_DATASETTE_START);
               break;
            case RETRO_MAPPER_DATASETTE_FORWARD:
               emu_function(EMU_DATASETTE_FORWARD);
               break;
            case RETRO_MAPPER_DATASETTE_REWIND:
               emu_function(EMU_DATASETTE_REWIND);
               break;
            case RETRO_MAPPER_DATASETTE_RESET:
               emu_function(EMU_DATASETTE_RESET);
               break;
         }
      }
      /* Key up */
      else if (!input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, mapper_keys[mk]) && kbt[i] && mapper_keys[mk])
      {
         kbt[i] = 0;
         switch (mk)
         {
            case RETRO_MAPPER_WARP_MODE:
               emu_function(EMU_WARP_MODE);
               break;
         }
      }
   }

   /* The check for kbt[i] here prevents the hotkey from generating C64 key events */
   /* retro_vkbd check is now in process_key() to allow certain keys while retro_vkbd */
   for (i = 0; i < (sizeof(kbt)/sizeof(kbt[0])); i++)
   {
      if (kbt[i])
      {
         disable_keys = 2;
         break;
      }
   }

   process_key(disable_keys);

   /* RetroPad hotkeys for ports 1 & 2 */
   for (j = 0; j < 2; j++)
   {
      if (retro_devices[j] == RETRO_DEVICE_JOYPAD)
      {
         LX = input_state_cb(j, RETRO_DEVICE_ANALOG, 0, 0);
         LY = input_state_cb(j, RETRO_DEVICE_ANALOG, 0, 1);
         RX = input_state_cb(j, RETRO_DEVICE_ANALOG, 1, 0);
         RY = input_state_cb(j, RETRO_DEVICE_ANALOG, 1, 1);

         /* No left analog remappings with non-joysticks */
         if (opt_joyport_type > 1)
            LX = LY = 0;

         for (i = 0; i < RETRO_DEVICE_ID_JOYPAD_LAST; i++)
         {
            int just_pressed  = 0;
            int just_released = 0;
            if ((i < 4 || i > 7) && i < 16) /* Remappable RetroPad buttons excluding D-Pad */
            {
               /* Skip the VKBD buttons if VKBD is visible and buttons are mapped to keyboard keys */
               if (retro_vkbd)
               {
                  switch (i)
                  {
                     case RETRO_DEVICE_ID_JOYPAD_B:
                     case RETRO_DEVICE_ID_JOYPAD_Y:
                     case RETRO_DEVICE_ID_JOYPAD_A:
                     case RETRO_DEVICE_ID_JOYPAD_X:
                     case RETRO_DEVICE_ID_JOYPAD_START:
                        if (mapper_keys[i] >= 0)
                           continue;
                        break;
                  }
               }

               /* No mappings if button = turbo fire in joystick mode */
               if (retro_turbo_fire && i == turbo_fire_button && opt_joyport_type == 1)
                  continue;

               if ((joypad_bits[j] & (1 << i)) && !jbt[j][i])
                  just_pressed = 1;
               else if (!(joypad_bits[j] & (1 << i)) && jbt[j][i])
                  just_released = 1;
            }
            else if (i >= 16) /* Remappable RetroPad analog stick directions */
            {
               switch (i)
               {
                  case RETRO_DEVICE_ID_JOYPAD_LR:
                     if (LX > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LX < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_LL:
                     if (LX < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LX > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_LD:
                     if (LY > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LY < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_LU:
                     if (LY < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LY > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RR:
                     if (RX > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RX < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RL:
                     if (RX < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RX > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RD:
                     if (RY > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RY < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RU:
                     if (RY < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RY > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  default:
                     break;
               }
            }

            if (just_pressed)
            {
               jbt[j][i] = 1;
               if (!mapper_keys[i]) /* Unmapped, e.g. set to "---" in core options */
                  continue;

               if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_VKBD])
                  emu_function(EMU_VKBD);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_STATUSBAR])
                  emu_function(EMU_STATUSBAR);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_JOYPORT])
                  emu_function(EMU_JOYPORT);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_RESET])
                  emu_function(EMU_RESET);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_ASPECT_RATIO])
                  emu_function(EMU_ASPECT_RATIO);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_ZOOM_MODE])
                  emu_function(EMU_ZOOM_MODE);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_WARP_MODE])
                  emu_function(EMU_WARP_MODE);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_HOTKEYS])
                  emu_function(EMU_DATASETTE_HOTKEYS);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_STOP])
                  emu_function(EMU_DATASETTE_STOP);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_START])
                  emu_function(EMU_DATASETTE_START);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_FORWARD])
                  emu_function(EMU_DATASETTE_FORWARD);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_REWIND])
                  emu_function(EMU_DATASETTE_REWIND);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_RESET])
                  emu_function(EMU_DATASETTE_RESET);
               else if (mapper_keys[i] == MOUSE_SLOWER)
                  mouse_speed[j] |= MOUSE_SPEED_SLOWER;
               else if (mapper_keys[i] == MOUSE_FASTER)
                  mouse_speed[j] |= MOUSE_SPEED_FASTER;
               else if (mapper_keys[i] == TOGGLE_VKBD)
                  emu_function(EMU_VKBD);
               else if (mapper_keys[i] == TOGGLE_STATUSBAR)
                  emu_function(EMU_STATUSBAR);
               else if (mapper_keys[i] == SWITCH_JOYPORT)
                  emu_function(EMU_JOYPORT);
               else
                  retro_key_down(mapper_keys[i]);
            }
            else if (just_released)
            {
               jbt[j][i] = 0;
               if (!mapper_keys[i]) /* Unmapped, e.g. set to "---" in core options */
                  continue;

               if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_VKBD])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_STATUSBAR])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_JOYPORT])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_RESET])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_ASPECT_RATIO])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_ZOOM_MODE])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_WARP_MODE])
                  emu_function(EMU_WARP_MODE);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_HOTKEYS])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_STOP])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_START])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_FORWARD])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_REWIND])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_RESET])
                  ; /* nop */
               else if (mapper_keys[i] == MOUSE_SLOWER)
                  mouse_speed[j] &= ~MOUSE_SPEED_SLOWER;
               else if (mapper_keys[i] == MOUSE_FASTER)
                  mouse_speed[j] &= ~MOUSE_SPEED_FASTER;
               else if (mapper_keys[i] == TOGGLE_VKBD)
                  ; /* nop */
               else if (mapper_keys[i] == TOGGLE_STATUSBAR)
                  ; /* nop */
               else if (mapper_keys[i] == SWITCH_JOYPORT)
                  ; /* nop */
               else
                  retro_key_up(mapper_keys[i]);
            }
         } /* for i */
      } /* if retro_devices[0]==joypad */
   } /* for j */

   /* Virtual keyboard for ports 1 & 2 */
   if (retro_vkbd)
   {
      /* Wait for all inputs to be released */
      if (retro_vkbd_ready < 1)
      {
         if ((retro_vkbd_ready == 0 && !joypad_bits[0] && !joypad_bits[1]) ||
              retro_vkbd_ready < 0)
            retro_vkbd_ready++;
         return;
      }

      if (!vkflag[RETRO_DEVICE_ID_JOYPAD_B]) /* Allow directions when key is not pressed */
      {
         if (!vkflag[RETRO_DEVICE_ID_JOYPAD_UP] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
                                                    (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
                                                    input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_UP] = 1;
         else
         if (vkflag[RETRO_DEVICE_ID_JOYPAD_UP] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) &&
                                                   !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) &&
                                                   !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_UP] = 0;

         if (!vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
                                                      (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
                                                      input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] = 1;
         else
         if (vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) &&
                                                     !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) &&
                                                     !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] = 0;

         if (!vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
                                                      (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
                                                      input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] = 1;
         else
         if (vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) &&
                                                     !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) &&
                                                     !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] = 0;

         if (!vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ||
                                                       (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ||
                                                       input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 1;
         else
         if (vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) &&
                                                      !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) &&
                                                      !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)))
            vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 0;
      }
      else /* Release all directions when key is pressed */
      {
         vkflag[RETRO_DEVICE_ID_JOYPAD_UP]    = 0;
         vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN]  = 0;
         vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT]  = 0;
         vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 0;
      }

      if (vkflag[RETRO_DEVICE_ID_JOYPAD_UP] ||
          vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] ||
          vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] ||
          vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT])
      {
         if (let_go_of_direction)
            /* just pressing down */
            last_press_time = now;

         if ((now - last_press_time > VKBD_MIN_HOLDING_TIME
           && now - last_move_time > VKBD_MOVE_DELAY)
           || let_go_of_direction)
         {
            last_move_time = now;

            if (vkflag[RETRO_DEVICE_ID_JOYPAD_UP])
               vkey_pos_y -= 1;
            else if (vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN])
               vkey_pos_y += 1;

            if (vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT])
               vkey_pos_x -= 1;
            else if (vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT])
               vkey_pos_x += 1;
         }
         let_go_of_direction = false;
      }
      else
         let_go_of_direction = true;

      if (vkey_pos_x < 0)
         vkey_pos_x = VKBDX - 1;
      else if (vkey_pos_x > VKBDX - 1)
         vkey_pos_x = 0;
      if (vkey_pos_y < 0)
         vkey_pos_y = VKBDY - 1;
      else if (vkey_pos_y > VKBDY - 1)
         vkey_pos_y = 0;

      /* Absolute pointer */
      int p_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
      int p_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

      if (p_x != 0 && p_y != 0 && (p_x != last_pointer_x || p_y != last_pointer_y))
      {
         int px = (int)((p_x + 0x7fff) * retrow / 0xffff);
         int py = (int)((p_y + 0x7fff) * retroh / 0xffff);
         last_pointer_x = p_x;
         last_pointer_y = p_y;
#ifdef POINTER_DEBUG
         pointer_x = px;
         pointer_y = py;
#endif
         if (px >= vkbd_x_min && px <= vkbd_x_max && py >= vkbd_y_min && py <= vkbd_y_max)
         {
            float vkey_width = (float)(vkbd_x_max - vkbd_x_min) / VKBDX;
            vkey_pos_x = ((px - vkbd_x_min) / vkey_width);

            float vkey_height = (float)(vkbd_y_max - vkbd_y_min) / VKBDY;
            vkey_pos_y = ((py - vkbd_y_min) / vkey_height);

            vkey_pos_x = (vkey_pos_x < 0) ? 0 : vkey_pos_x;
            vkey_pos_x = (vkey_pos_x > VKBDX - 1) ? VKBDX - 1 : vkey_pos_x;
            vkey_pos_y = (vkey_pos_y < 0) ? 0 : vkey_pos_y;
            vkey_pos_y = (vkey_pos_y > VKBDY - 1) ? VKBDY - 1 : vkey_pos_y;

#ifdef POINTER_DEBUG
            printf("px:%d py:%d (%d,%d) vkey:%dx%d\n", p_x, p_y, px, py, vkey_pos_x, vkey_pos_y);
#endif
         }
      }

      /* Press Return, RetroPad Start */
      i = RETRO_DEVICE_ID_JOYPAD_START;
      if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                                (joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 1;
         kbd_handle_keydown(RETROK_RETURN);
      }
      else
      if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                        !(joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 0;
         kbd_handle_keyup(RETROK_RETURN);
      }

      /* Toggle ShiftLock, RetroPad Y */
      i = RETRO_DEVICE_ID_JOYPAD_Y;
      if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                                (joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 1;
         retro_key_down(RETROK_CAPSLOCK);
         retro_key_up(RETROK_CAPSLOCK);
      }
      else
      if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                        !(joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 0;
      }

      /* Press Space, RetroPad X */
      i = RETRO_DEVICE_ID_JOYPAD_X;
      if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                                (joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 1;
         kbd_handle_keydown(RETROK_SPACE);
      }
      else
      if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                        !(joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 0;
         kbd_handle_keyup(RETROK_SPACE);
      }

      /* Toggle transparency, RetroPad A */
      i = RETRO_DEVICE_ID_JOYPAD_A;
      if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                                (joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 1;
         retro_vkbd_transparent = !retro_vkbd_transparent;
      }
      else
      if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                        !(joypad_bits[1] & (1 << i))))
      {
         vkflag[i] = 0;
      }

      /* Key press, RetroPad B joyports 1+2 / Keyboard Enter / Pointer */
      i = RETRO_DEVICE_ID_JOYPAD_B;
      if (!vkflag[i] && ((joypad_bits[0] & (1 << i)) ||
                         (joypad_bits[1] & (1 << i)) ||
                         input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN) ||
                         input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)))
      {
         vkey_pressed = check_vkey(vkey_pos_x, vkey_pos_y);
         vkflag[i] = 1;

         if (vkey_pressed != -1 && last_vkey_pressed == -1)
         {
            switch (vkey_pressed)
            {
               case -2:
                  emu_function(EMU_RESET);
                  break;
               case -3:
                  if (retro_capslock)
                     emu_function(EMU_SAVE_DISK);
                  else
                     emu_function(EMU_STATUSBAR);
                  break;
               case -4:
                  if (retro_capslock)
                     emu_function(EMU_ASPECT_RATIO);
                  else
                     emu_function(EMU_JOYPORT);
                  break;
               case -5:
                  if (retro_capslock)
                     emu_function(EMU_ZOOM_MODE);
                  else
                     emu_function(EMU_TURBO_FIRE);
                  break;
               case -10: /* ShiftLock */
                  retro_key_down(RETROK_CAPSLOCK);
                  retro_key_up(RETROK_CAPSLOCK);
                  break;

               case -11:
                  emu_function(EMU_DATASETTE_STOP);
                  break;
               case -12:
                  emu_function(EMU_DATASETTE_START);
                  break;
               case -13:
                  emu_function(EMU_DATASETTE_FORWARD);
                  break;
               case -14:
                  emu_function(EMU_DATASETTE_REWIND);
                  break;
               case -15:
                  emu_function(EMU_DATASETTE_RESET);
                  break;

               default:
                  if (vkey_pressed == vkey_sticky1)
                     vkey_sticky1_release = 1;
                  if (vkey_pressed == vkey_sticky2)
                     vkey_sticky2_release = 1;
                  kbd_handle_keydown(vkey_pressed);
                  break;
            }
         }
         last_vkey_pressed = vkey_pressed;
      }
      else
      if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                        !(joypad_bits[1] & (1 << i)) &&
                        !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN) &&
                        !input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)))
      {
         vkey_pressed = -1;
         vkflag[i] = 0;
      }

      if (vkflag[RETRO_DEVICE_ID_JOYPAD_B] && vkey_pressed > 0)
      {
         if (let_go_of_button)
            last_press_time_button = now;
         if (now - last_press_time_button > VKBD_STICKY_HOLDING_TIME)
            vkey_sticky = 1;
         let_go_of_button = 0;
      }
      else
      {
         let_go_of_button = 1;
         vkey_sticky = 0;
      }
   }
#if 0
   printf("vkey:%d sticky:%d sticky1:%d sticky2:%d, now:%d last:%d\n", vkey_pressed, vkey_sticky, vkey_sticky1, vkey_sticky2, now, last_press_time_button);
#endif
}

int process_keyboard_pass_through()
{
   unsigned process = 0;

   /* Defaults */
   int fire_button = RETRO_DEVICE_ID_JOYPAD_B;
   int jump_button = -1;

   /* Fire button */
   switch (opt_retropad_options)
   {
      case 1:
      case 3:
         fire_button = RETRO_DEVICE_ID_JOYPAD_Y;
         break;
   }

   /* Jump button */
   switch (opt_retropad_options)
   {
      case 2:
         jump_button = RETRO_DEVICE_ID_JOYPAD_A;
         break;
      case 3:
         jump_button = RETRO_DEVICE_ID_JOYPAD_B;
         break;
   }
   /* Null only with RetroPad */
   if (retro_devices[0] == RETRO_DEVICE_JOYPAD)
   {
      if (mapper_keys[fire_button] || (retro_turbo_fire && fire_button == turbo_fire_button))
         fire_button = -1;

      if (mapper_keys[jump_button] || (retro_turbo_fire && jump_button == turbo_fire_button))
         jump_button = -1;
   }

   /* Prevent RetroPad from generating keyboard key presses when RetroPad is controlled with keyboard */
   switch (retro_devices[0])
   {
      case RETRO_DEVICE_JOYPAD:
         if ((fire_button > -1 && (joypad_bits[0] & (1 << fire_button))) ||
             (jump_button > -1 && (joypad_bits[0] & (1 << jump_button))) ||
             (retro_turbo_fire && (joypad_bits[0] & (1 << turbo_fire_button))) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_B]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_Y))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_Y]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_A]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_X))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_X]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L2]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R2]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L3]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R3]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT)) && mapper_keys[RETRO_DEVICE_ID_JOYPAD_SELECT]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_START))  && mapper_keys[RETRO_DEVICE_ID_JOYPAD_START]))
            process = 2; /* Skip all keyboard input when RetroPad buttons are pressed */
         else
         if ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 1; /* Skip cursor keys */
         break;

      case RETRO_DEVICE_VICE_JOYSTICK:
         if ((fire_button > -1 && (joypad_bits[0] & (1 << fire_button))) ||
             (jump_button > -1 && (joypad_bits[0] & (1 << jump_button))))
            process = 2; /* Skip all keyboard input when RetroPad buttons are pressed */
         else
         if ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 1; /* Skip cursor keys */
         break;
   }

   /* Fire button */
   fire_button = RETRO_DEVICE_ID_JOYPAD_B;
   switch (opt_retropad_options)
   {
      case 1:
      case 3:
         fire_button = RETRO_DEVICE_ID_JOYPAD_Y;
         break;
   }

   /* Jump button */
   jump_button = -1;
   switch (opt_retropad_options)
   {
      case 2:
         jump_button = RETRO_DEVICE_ID_JOYPAD_A;
         break;
      case 3:
         jump_button = RETRO_DEVICE_ID_JOYPAD_B;
         break;
   }
   /* Null only with RetroPad */
   if (retro_devices[1] == RETRO_DEVICE_JOYPAD)
   {
      if (mapper_keys[fire_button] || (retro_turbo_fire && fire_button == turbo_fire_button))
         fire_button = -1;

      if (mapper_keys[jump_button] || (retro_turbo_fire && jump_button == turbo_fire_button))
         jump_button = -1;
   }

   switch (retro_devices[1])
   {
      case RETRO_DEVICE_JOYPAD:
         if ((fire_button > -1 && (joypad_bits[1] & (1 << fire_button))) ||
             (jump_button > -1 && (joypad_bits[1] & (1 << jump_button))) ||
             (retro_turbo_fire && (joypad_bits[1] & (1 << turbo_fire_button))) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_B]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_Y))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_Y]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_A))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_A]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_X))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_X]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L2]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R2]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L3]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R3]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT)) && mapper_keys[RETRO_DEVICE_ID_JOYPAD_SELECT]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_START))  && mapper_keys[RETRO_DEVICE_ID_JOYPAD_START]) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 2; /* Skip all keyboard input from RetroPad 2 */
         break;

      case RETRO_DEVICE_VICE_JOYSTICK:
         if ((fire_button > -1 && (joypad_bits[1] & (1 << fire_button))) ||
             (jump_button > -1 && (joypad_bits[1] & (1 << jump_button))) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 2; /* Skip all keyboard input from RetroPad 2 */
         break;
   }

   return process;
}

void retro_poll_event()
{
   unsigned i, j;

   input_poll_cb();

   for (j = 0; j < RETRO_DEVICES; j++)
   {
      if (libretro_supports_bitmasks)
         joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      else
      {
         joypad_bits[j] = 0;
         for (i = 0; i < RETRO_DEVICE_ID_JOYPAD_LR; i++)
            joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
      }
   }

   /* Keyboard pass-through */
   unsigned disable_keys = 0;
   if (!opt_keyboard_pass_through)
      disable_keys = process_keyboard_pass_through();
   update_input(disable_keys);

   /* retro joypad take control over keyboard joy */
   /* override keydown, but allow keyup, to prevent key sticking during keyboard use, if held down on opening keyboard */
   /* keyup allowing most likely not needed on actual keyboard presses even though they get stuck also */
   int retro_port;
   for (retro_port = 0; retro_port <= 4; retro_port++)
   {
      if (retro_devices[retro_port] == RETRO_DEVICE_VICE_JOYSTICK || retro_devices[retro_port] == RETRO_DEVICE_JOYPAD)
      {
         int vice_port = cur_port;
         uint8_t joy_value = 0;

         if (retro_port == 1) /* Second joypad controls other player */
            vice_port = (cur_port == 2) ? 1 : 2;
         else if (retro_port == 2)
            vice_port = 3;
         else if (retro_port == 3)
            vice_port = 4;
         else if (retro_port == 4)
            vice_port = 5;

         /* No same port joystick movements with non-joysticks */
         if (opt_joyport_type > 1 && vice_port == cur_port)
            continue;
         /* No joystick movements with paddles */
         if (opt_joyport_type == 2)
            continue;

         joy_value = get_joystick_value(vice_port);

         /* Up */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP9)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP3)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP8)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP2)
                  )
               )
            )
         )
            joy_value |= (!retro_vkbd) ? 0x01 : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)))
            joy_value &= ~0x01;

         /* Down */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP3)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP9)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP2)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP8)
                  )
               )
            )
         )
            joy_value |= (!retro_vkbd) ? 0x02 : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)))
            joy_value &= ~0x02;

         /* Left */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP7)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP1)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP4)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP6)
                  )
               )
            )
         )
            joy_value |= (!retro_vkbd) ? 0x04 : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)))
            joy_value &= ~0x04;

         /* Right */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP1)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP7)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP6)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP4)
                  )
               )
            )
         )
            joy_value |= (!retro_vkbd) ? 0x08 : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            joy_value &= ~0x08;

         /* Fire button */
         int fire_button = RETRO_DEVICE_ID_JOYPAD_B;
         switch (opt_retropad_options)
         {
            case 1:
            case 3:
               fire_button = RETRO_DEVICE_ID_JOYPAD_Y;
               break;
         }
         /* Null only with RetroPad */
         if (retro_devices[retro_port] == RETRO_DEVICE_JOYPAD)
         {
            if (mapper_keys[fire_button] || (retro_turbo_fire && fire_button == turbo_fire_button))
               fire_button = -1;
         }

         if ((fire_button > -1 && (joypad_bits[retro_port] & (1 << fire_button)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port && input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP0))
                  ||
                  (vice_port == cur_port && input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP5))
               )
            )
         )
            joy_value |= (!retro_vkbd) ? 0x10 : joy_value;
         else
            joy_value &= ~0x10;

         /* Jump button */
         int jump_button = -1;
         switch (opt_retropad_options)
         {
            case 2:
               jump_button = RETRO_DEVICE_ID_JOYPAD_A;
               break;
            case 3:
               jump_button = RETRO_DEVICE_ID_JOYPAD_B;
               break;
         }
         /* Null only with RetroPad */
         if (retro_devices[retro_port] == RETRO_DEVICE_JOYPAD)
         {
            if (mapper_keys[jump_button] || (retro_turbo_fire && jump_button == turbo_fire_button))
               jump_button = -1;
         }

         if (jump_button > -1 && (joypad_bits[retro_port] & (1 << jump_button)))
         {
            joy_value |= (!retro_vkbd) ? 0x01 : joy_value;
            joy_value &= ~0x02;
         }
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
         && (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP9)
                  )
                  &&
                  (vice_port == cur_port
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP8)
                  )
               )
            )
         )
            joy_value &= ~0x01;

         /* Turbo fire */
         if (retro_devices[retro_port] == RETRO_DEVICE_JOYPAD
          && retro_turbo_fire
          && !(joypad_bits[retro_port] & (1 << fire_button)))
         {
            if ((joypad_bits[retro_port] & (1 << turbo_fire_button)))
            {
               if (turbo_state[vice_port])
               {
                  if ((turbo_toggle[vice_port]) == (turbo_pulse))
                     turbo_toggle[vice_port] = 1;
                  else
                     turbo_toggle[vice_port]++;

                  if (turbo_toggle[vice_port] > (turbo_pulse / 2))
                     joy_value &= ~0x10;
                  else
                     joy_value |= (!retro_vkbd) ? 0x10 : joy_value;
               }
               else
               {
                  turbo_state[vice_port] = 1;
                  joy_value |= (!retro_vkbd) ? 0x10 : joy_value;
               }
            }
            else
            {
               turbo_state[vice_port] = 0;
               turbo_toggle[vice_port] = 0;
            }
         }

         joystick_set_value_absolute(vice_port, joy_value);

#if 0
         if (vice_port == 2)
            printf("Joy %d: Button %d, %2d %d %d\n", vice_port, turbo_fire_button, joystick_value, turbo_state[vice_port], turbo_toggle[vice_port]);
#endif
      }
   }

   /* Default to joysticks, set both ports */
   if (opt_joyport_type == 1)
   {
      if (opt_joyport_type_prev != opt_joyport_type)
      {
         opt_joyport_type_prev = opt_joyport_type;
         resources_set_int("JoyPort1Device", opt_joyport_type);
         resources_set_int("JoyPort2Device", opt_joyport_type);
      }
   }
   /* Other than a joystick, set only cur_port */
   else if (opt_joyport_type > 1 && !retro_vkbd)
   {
      if (opt_joyport_type_prev != opt_joyport_type || cur_port_prev != cur_port)
      {
         opt_joyport_type_prev = opt_joyport_type;
         cur_port_prev = cur_port;

         if (cur_port == 2)
         {
            resources_set_int("JoyPort1Device", 1);
            resources_set_int("JoyPort2Device", opt_joyport_type);
         }
         else
         {
            resources_set_int("JoyPort2Device", 1);
            resources_set_int("JoyPort1Device", opt_joyport_type);
         }
      }

      int j = cur_port - 1;
      int retro_j = 0;
      static float mouse_multiplier[2] = {1};
      static int dpadmouse_speed[2] = {0};
      static int dpadmouse_pressed[2] = {0};
#ifdef MOUSE_DPAD_ACCEL
      static long now = 0;
      now = retro_ticks() / 1000;
#endif

      int retro_mouse_x[2] = {0}, retro_mouse_y[2] = {0};
      unsigned int retro_mouse_l[2] = {0}, retro_mouse_r[2] = {0}, retro_mouse_m[2] = {0};
      static unsigned int vice_mouse_l[2] = {0}, vice_mouse_r[2] = {0}, vice_mouse_m[2] = {0};

      int analog_left[2] = {0};
      int analog_right[2] = {0};
      double analog_left_magnitude = 0;
      double analog_right_magnitude = 0;
      int analog_deadzone = 0;
      analog_deadzone = (opt_analogmouse_deadzone * 32768 / 100);

      /* Paddles (opt_joyport_type = 2) share the same joyport, but are meant for 2 players.
         Therefore treat retroport0 vertical axis as retroport1 horizontal axis, and second fire as retroport1 fire. */

      /* Joypad buttons */
      if (!retro_vkbd)
      {
         if (retro_devices[0] == RETRO_DEVICE_JOYPAD && (opt_retropad_options == 1 || opt_retropad_options == 3))
         {
            retro_mouse_l[j] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_Y));
            /* Paddles-split */
            if (opt_joyport_type == 2)
               retro_mouse_r[j] = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_Y));
            else
               retro_mouse_r[j] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B));
         }
         else
         {
            retro_mouse_l[j] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B));
            /* Paddles-split */
            if (opt_joyport_type == 2)
               retro_mouse_r[j] = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B));
            else
               retro_mouse_r[j] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A));
         }
      }

      /* Real mouse buttons */
      if (!retro_mouse_l[j] && !retro_mouse_r[j] && !retro_mouse_m[j])
      {
         retro_mouse_l[j] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
         retro_mouse_r[j] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
         retro_mouse_m[j] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
      }

      /* Joypad movement */
      if (!retro_vkbd)
      {
         for (retro_j = 0; retro_j < 2; retro_j++)
         {
            /* Digital mouse speed modifiers */
            if (!dpadmouse_pressed[retro_j])
#ifdef MOUSE_DPAD_ACCEL
               dpadmouse_speed[retro_j] = opt_dpadmouse_speed - 2;
#else
               dpadmouse_speed[retro_j] = opt_dpadmouse_speed;
#endif

            if (mouse_speed[retro_j] & MOUSE_SPEED_FASTER)
               dpadmouse_speed[retro_j] = dpadmouse_speed[retro_j] + 3;
            if (mouse_speed[retro_j] & MOUSE_SPEED_SLOWER)
               dpadmouse_speed[retro_j] = dpadmouse_speed[retro_j] - 4;

#ifdef MOUSE_DPAD_ACCEL
            /* Digital mouse acceleration */
            if (dpadmouse_pressed[retro_j] && (now - dpadmouse_pressed[retro_j] > 300))
            {
               dpadmouse_speed[retro_j]++;
               dpadmouse_pressed[retro_j] = now;
            }
#endif

            /* Digital mouse speed limits */
            if (dpadmouse_speed[retro_j] < 2) dpadmouse_speed[retro_j] = 2;
            if (dpadmouse_speed[retro_j] > 14) dpadmouse_speed[retro_j] = 14;
         }

         if (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
            retro_mouse_x[j] += dpadmouse_speed[0];
         else if (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
            retro_mouse_x[j] -= dpadmouse_speed[0];

         /* Paddles-split */
         if (opt_joyport_type == 2)
         {
            if (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
               retro_mouse_y[j] += dpadmouse_speed[1];
            else if (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
               retro_mouse_y[j] -= dpadmouse_speed[1];
         }
         else
         {
            if (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
               retro_mouse_y[j] += dpadmouse_speed[0];
            else if (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
               retro_mouse_y[j] -= dpadmouse_speed[0];
         }

#ifdef MOUSE_DPAD_ACCEL
         for (retro_j = 0; retro_j < 2; retro_j++)
         {
            /* Acceleration timestamps */
            if ((retro_mouse_x[j] != 0 || retro_mouse_y[j] != 0) && !dpadmouse_pressed[retro_j])
               dpadmouse_pressed[retro_j] = now;
            else if ((retro_mouse_x[j] == 0 && retro_mouse_y[j] == 0) && dpadmouse_pressed[retro_j])
               dpadmouse_pressed[retro_j] = 0;
         }
#endif
      }

      /* Left analog movement */
      analog_left[0] = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      /* Paddles split */
      if (opt_joyport_type == 2)
         analog_left[1] = -input_state_cb(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      else
         analog_left[1] = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
      analog_left_magnitude = sqrt((analog_left[0]*analog_left[0]) + (analog_left[1]*analog_left[1]));
      if (analog_left_magnitude <= analog_deadzone)
      {
         analog_left[0] = 0;
         analog_left[1] = 0;
      }

      for (retro_j = 0; retro_j < 2; retro_j++)
      {
         /* Analog stick speed modifiers */
         mouse_multiplier[retro_j] = 1;
         if (mouse_speed[retro_j] & MOUSE_SPEED_FASTER)
            mouse_multiplier[retro_j] = mouse_multiplier[retro_j] * MOUSE_SPEED_FAST;
         if (mouse_speed[retro_j] & MOUSE_SPEED_SLOWER)
            mouse_multiplier[retro_j] = mouse_multiplier[retro_j] / MOUSE_SPEED_SLOW;
      }

      if (abs(analog_left[0]) > 0)
      {
         retro_mouse_x[j] = analog_left[0] * 10 * (opt_analogmouse_speed * opt_analogmouse_speed * 0.7) / (32768 / mouse_multiplier[0]);
         if (retro_mouse_x[j] == 0 && abs(analog_left[0]) > analog_deadzone)
            retro_mouse_x[j] = (analog_left[0] > 0) ? 1 : -1;
      }

      if (abs(analog_left[1]) > 0)
      {
         retro_mouse_y[j] = analog_left[1] * 10 * (opt_analogmouse_speed * opt_analogmouse_speed * 0.7) / (32768 / mouse_multiplier[(opt_joyport_type == 2) ? 1 : 0]);
         if (retro_mouse_y[j] == 0 && abs(analog_left[1]) > analog_deadzone)
            retro_mouse_y[j] = (analog_left[1] > 0) ? 1 : -1;
      }

      /* Real mouse movement */
      if (!retro_mouse_x[j] && !retro_mouse_y[j])
      {
         retro_mouse_x[j] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
         retro_mouse_y[j] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      }

      /* Ports 1 & 2 to VICE */
      for (j = 0; j < 2; j++)
      {
         retro_j = j + 1;
         if (!retro_mouse_l[j] && !retro_mouse_r[j] && !retro_mouse_m[j] &&
             !retro_mouse_x[j] && !retro_mouse_y[j])
               mouse_value[retro_j] = 0;

         /* Buttons */
         if (retro_mouse_l[j] && !vice_mouse_l[j])
         {
            mouse_button(0, 1);
            mouse_value[retro_j] |= 0x10;
            vice_mouse_l[j] = 1;
         }
         else if (!retro_mouse_l[j] && vice_mouse_l[j])
         {
            mouse_button(0, 0);
            mouse_value[retro_j] &= ~0x10;
            vice_mouse_l[j] = 0;
         }

         if (retro_mouse_r[j] && !vice_mouse_r[j])
         {
            mouse_button(1, 1);
            mouse_value[retro_j] |= 0x20;
            vice_mouse_r[j] = 1;            
         }
         else if (!retro_mouse_r[j] && vice_mouse_r[j])
         {
            mouse_button(1, 0);
            mouse_value[retro_j] &= ~0x20;
            vice_mouse_r[j] = 0;            
         }

         if (retro_mouse_m[j] && !vice_mouse_m[j])
         {
            mouse_button(2, 1);
            mouse_value[retro_j] |= 0x40;
            vice_mouse_m[j] = 1;
         }
         else if (!retro_mouse_m[j] && vice_mouse_m[j])
         {
            mouse_button(2, 0);
            mouse_value[retro_j] &= ~0x40;
            vice_mouse_m[j] = 0;
         }

         /* Movement */
         if (retro_mouse_x[j] || retro_mouse_y[j])
         {
            if (retro_mouse_y[j] < 0 && !(mouse_value[retro_j] & 0x01))
               mouse_value[retro_j] |= 0x01;
            if (retro_mouse_y[j] > -1 && mouse_value[retro_j] & 0x01)
               mouse_value[retro_j] &= ~0x01;

            if (retro_mouse_y[j] > 0 && !(mouse_value[retro_j] & 0x02))
               mouse_value[retro_j] |= 0x02;
            if (retro_mouse_y[j] < -1 && mouse_value[retro_j] & 0x02)
               mouse_value[retro_j] &= ~0x02;

            if (retro_mouse_x[j] < 0 && !(mouse_value[retro_j] & 0x04))
               mouse_value[retro_j] |= 0x04;
            if (retro_mouse_x[j] > -1 && mouse_value[retro_j] & 0x04)
               mouse_value[retro_j] &= ~0x04;

            if (retro_mouse_x[j] > 0 && !(mouse_value[retro_j] & 0x08))
               mouse_value[retro_j] |= 0x08;
            if (retro_mouse_x[j] < -1 && mouse_value[retro_j] & 0x08)
               mouse_value[retro_j] &= ~0x08;

            retro_mouse_move(retro_mouse_x[j], retro_mouse_y[j]);
         }
      }
   }
}
