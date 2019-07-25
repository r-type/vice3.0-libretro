#include "libretro.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "fliplist.h"
#include "mouse.h"

#include "kbd.h"
#include "mousedrv.h"
#include "libretro-core.h"

extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;

extern void save_bkg();
extern void Screen_SetFullUpdate(int scr);

//EMU FLAGS
int SHOWKEY=-1;
int SHIFTON=-1;
int KBMOD=-1;
int RSTOPON=-1;
int CTRLON=-1;
int NPAGE=-1;
int KCOL=1;
int SND=1;
int vkey_pressed;
unsigned char MXjoy[2]; // joy
char core_key_state[512];
char core_old_key_state[512];
int MOUSE_EMULATED=-1,PAS=4;
int slowdown=0;
int pushi=0; //mouse button
int c64mouse_enable=0;
unsigned int cur_port = 2;
bool num_locked = false;

extern bool retro_load_ok;
extern int mapper_keys[16];

void emu_reset()
{
	machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
}

void Keymap_KeyUp(int symkey)
{
   if (symkey == RETROK_NUMLOCK)
      num_locked = false;
   else
      kbd_handle_keyup(symkey);
}

void Keymap_KeyDown(int symkey)
{
   //FIXME detect retroarch hotkey
   switch (symkey)
   {
      case RETROK_F9: /* F9: toggle vkbd */
         SHOWKEY=-SHOWKEY;
         break;
      case RETROK_F10: /* F10: */
         pauseg=1;
         save_bkg();
         printf("enter gui!\n");
         break;
      case RETROK_F11: /* F11: */
         break;
      case RETROK_F12: /* F12: Reset */
         machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
         break;
      case RETROK_NUMLOCK:
         num_locked = true;
         break;
      case RETROK_KP_PLUS: /* '+' on keypad: FLip List NEXT */
         /* flilist next */
         fliplist_attach_head(8, 1);
         break;
      case RETROK_KP_MINUS: /* '-' on keypad: FLip List PREV */
         /* flilist prev */
         fliplist_attach_head(8, 0);
         break;
      case RETROK_KP_MULTIPLY: /* '*' on keypad: toggle current joy port */
         cur_port++;
         if(cur_port>2)cur_port=1;
         break;
      case RETROK_KP_DIVIDE: /* '/' on keypad: Toggle GUI */
         save_bkg();
         printf("enter gui!\n");
         pauseg=1;
         break;
      default:
         kbd_handle_keydown(symkey);
         break;
   }
}

void app_vkb_handle(void)
{
   static int oldi=-1;
   int i;

   if(oldi!=-1)
   {  
      kbd_handle_keyup(oldi);
      oldi=-1;
   }

   if(vkey_pressed==-1)return;

   i=vkey_pressed;
   vkey_pressed=-1;

   if(i==-1){
      oldi=-1;
   }
   if(i==-2)
   {
      NPAGE=-NPAGE;oldi=-1;
   }
   else if(i==-3)
   {
      //flilist prev
      fliplist_attach_head(8, 0);
      oldi=-1;
   }
   else if(i==-4)
   {
      //VKbd show/hide
      oldi=-1;
      SHOWKEY=-SHOWKEY;
   }
   else if(i==-5)
   {
      //flilist next
      fliplist_attach_head(8, 1);
      oldi=-1;
   }
   /*
      else if(i==-6)
      {
   //Exit
   retro_deinit();
   oldi=-1;
   exit(0);
   }
   */
   else
   {
      if(i==-10) //SHIFT
      {
         if(SHIFTON == 1)kbd_handle_keyup(RETROK_RSHIFT);
         else kbd_handle_keydown(RETROK_LSHIFT);
         SHIFTON=-SHIFTON;
         oldi=-1;
      }
      else if(i==-11) //CTRL
      {     
         if(CTRLON == 1)kbd_handle_keyup(RETROK_LCTRL);
         else kbd_handle_keydown(RETROK_LCTRL);
         CTRLON=-CTRLON;
         oldi=-1;
      }
      else if(i==-12) //RSTOP
      {
         if(RSTOPON == 1)kbd_handle_keyup(RETROK_ESCAPE);
         else kbd_handle_keydown(RETROK_ESCAPE); 
         RSTOPON=-RSTOPON;
         oldi=-1;
      }
      else if(i==-13) //GUI
      {
         pauseg=1;
         save_bkg();
         //SHOWKEY=-SHOWKEY;
         //Screen_SetFullUpdate(0);
         oldi=-1;
      }
      else if(i==-14) //JOY PORT TOGGLE
      {    
         //cur joy toggle
         cur_port++;if(cur_port>2)cur_port=1;
         SHOWKEY=-SHOWKEY;
         oldi=-1;
      }
      else
      {
         oldi=i;
         kbd_handle_keydown(oldi);
      }
   }
}

// Core input Key(not GUI) 
void Core_Processkey(void)
{
   int i;

   for(i=0; i<320; i++)
      core_key_state[i]=input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,i) ? 0x80: 0;

   if (memcmp(core_key_state, core_old_key_state, sizeof(core_key_state)))
   {
      for(i=0; i<320; i++)
      {
         if(core_key_state[i] && core_key_state[i]!=core_old_key_state[i])
         {	
            if(i==RETROK_LALT)
            {
               //KBMOD=-KBMOD;
               //printf("Modifier alt pressed %d \n",KBMOD);
               continue;
            }
            //printf("press: %d \n",i);
            Keymap_KeyDown(i);

         }
         else if (!core_key_state[i] && core_key_state[i] != core_old_key_state[i])
         {
            if(i==RETROK_LALT)
            {
               //KBMOD=-KBMOD;
               //printf("Modifier alt released %d \n",KBMOD);
               continue;
            }
            //printf("release: %d \n",i);
            Keymap_KeyUp(i);

         }
      }
   }
   memcpy(core_old_key_state, core_key_state, sizeof(core_key_state));
}

// Core input (not GUI) 
int Core_PollEvent(void)
{
    //   RETRO        B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
    //   INDEX        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
    //   C64          BOOT VKB  M/J  R/S  UP   DWN  LEFT RGT  B1   GUI  F7   F1   F5   F3   SPC  1 

   int i;
   static int jbt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   static int vbt[16]={0x1C,0x39,0x01,0x3B,0x01,0x02,0x04,0x08,0x80,0x40,0x15,0x31,0x24,0x1F,0x6E,0x6F};
   static int kbt[4]={0,0,0,0};

   // MXjoy[0]=0;
   if(!retro_load_ok)return 1;
   input_poll_cb();

   int mouse_l;
   int mouse_r;
   int16_t mouse_x,mouse_y;
   mouse_x=mouse_y=0;

   if (SHOWKEY==-1 && pauseg==0)
      Core_Processkey();

/*
   // F9 toggle vkbd
   i=0;
   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F9) && kbt[i]==0)
   {
      kbt[i]=1;
   }
   else if (kbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F9))
   {
      kbt[i]=0;
      SHOWKEY=-SHOWKEY;
   }

   // KEYPAD_DIVIDE toggle gui
   i=1;
   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP_DIVIDE) && kbt[i]==0)
   {
      kbt[i]=1;
   }   
   else if (kbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP_DIVIDE))
   {
      kbt[i]=0;
      pauseg = 1;
      save_bkg();
      printf("enter gui!\n");
   }
*/
   /*
   Defaults defined in libretro-core.c
   --------------------------------
   Default action       keys
   --------------------------------
   Y       showvkey      F9
   START   gui  	      F10 or KP/
   L       flip next     KP+
   R       flip prev     KP-
   L2      swap joyport KP*
   F12     reset         F12
   --------------------------------
   */
   if (pauseg == 0)
   {
      for(i=0; i<16; i++)
      {
         if( (i<4 || i>8) && i!=2)
         {
            if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0)
            {
               jbt[i]=1;
               Keymap_KeyDown(mapper_keys[i]);
            }
            else if (jbt[i]==1 && !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
            {
               jbt[i]=0;
               Keymap_KeyUp(mapper_keys[i]);
            }
         }
      }
   }

   i=2; // mouse/joy toggle
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && jbt[i]==0)
      jbt[i]=1;
   else if (jbt[i]==1 && !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
   {
      jbt[i]=0;
      MOUSE_EMULATED=-MOUSE_EMULATED;	  
   }

   if(slowdown>0)
      return 0;

   if(MOUSE_EMULATED==1)
   {
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))mouse_x += PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))mouse_x -= PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))mouse_y += PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))mouse_y -= PAS;
      mouse_l=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
      mouse_r=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
   }
   else
   {
      mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
   }

   slowdown=1;

   static int mmbL=0,mmbR=0;

   if (mmbL==0 && mouse_l)
   {
      mmbL=1;
      pushi=1;
   }
   else if (mmbL==1 && !mouse_l)
   {
      mmbL=0;
      pushi=0;
   }

   if (mmbR==0 && mouse_r)
   {
      mmbR=1;
   }
   else if (mmbR==1 && !mouse_r)
   {
      mmbR=0;
   }

   if (pauseg==0 && c64mouse_enable)
   {
      mouse_move((int)mouse_x, (int)mouse_y);
      mouse_button(0,mmbL);
      mouse_button(1,mmbR);
  }

   return 1;
}

void retro_poll_event(int joyon)
{
   Core_PollEvent();

   if (joyon && (SHOWKEY == -1) && pauseg != 1) // Disable emulated joysticks while vkbd is displayed (interferes with key presses)
   {
      int retro_port;
      for (retro_port = 0; retro_port <= 4; retro_port++)
      {
         int vice_port = cur_port;

         if (retro_port == 1)  // second joypad controls other player
         {
            if (cur_port == 2)
               vice_port = 1;
            else
               vice_port = 2;
         } else if (retro_port == 2)
            vice_port = 3;
         else if (retro_port == 3)
            vice_port = 4;
         else if (retro_port == 4)
            vice_port = 5;

         BYTE j = joystick_value[vice_port];

         if (input_state_cb(retro_port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
            j |= 0x01;
         else
            j &= ~0x01;

         if (input_state_cb(retro_port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
            j |= 0x02;
         else
            j &= ~0x02;

         if (input_state_cb(retro_port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
            j |= 0x04;
         else
            j &=~ 0x04;

         if (input_state_cb(retro_port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
            j |= 0x08;
         else
            j &= ~0x08;

         if (input_state_cb(retro_port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
            j |= 0x10;
         else
            j &= ~0x10;

         joystick_value[vice_port] = j;
      }
   }
}

