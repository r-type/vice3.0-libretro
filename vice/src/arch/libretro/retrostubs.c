#include "libretro.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "libretro-core.h"
#include "vkbd_def.h"

extern retro_input_state_t input_state_cb;

extern int vkx,vky;
extern int CTRLON;
extern int RSTOPON;
extern int NPAGE;
extern int BKGCOLOR;
extern int KCOL;
extern int SHIFTON;
extern int SHOWKEY;
extern int CROP_WIDTH;
extern int CROP_HEIGHT;

int RSTOPON=-1;
int CTRLON=-1;
int vkx=0,vky=0;
unsigned int cur_port = 2;
bool num_locked = false;

#define MATRIX(a,b) (((a) << 3) | (b))

void quick_load()
{
	DlgFloppy_Main();
	pauseg=0;
}

void quick_option()
{
	Dialog_OptionDlg();
	pauseg=0;
}

void Screen_SetFullUpdate(int scr)
{
   if(scr==0 ||scr>1)memset(Retro_Screen, 0, sizeof(Retro_Screen));
  //if(scr>0)if(Screen)memset(Screen->pixels,0,Screen->h*Screen->pitch);
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

	switch (symkey){

		case RETROK_F9:	// F9: Quick Drive file select
			pauseg=2;
			break;
		case RETROK_F10:	// F10: 
			pauseg=3;
			break;
/*
//FIXME detect retroarch hotkey
		case RETROK_F11:	// F11:
			break;
*/
		case RETROK_F12:	// F12: Reset
			machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
			break;

		case RETROK_NUMLOCK:
			num_locked = true;
			break;

		case RETROK_KP_PLUS:	// '+' on keypad: FLip List NEXT
			//flilist next
			fliplist_attach_head(8, 1);
			break;
		case RETROK_KP_MINUS:	// '-' on keypad: FLip List PREV
			//flilist prev
			fliplist_attach_head(8, 0);
			break;
		case RETROK_KP_MULTIPLY:	// '*' on keypad: toggle current joy port
		    cur_port++;
			if(cur_port>2)cur_port=1;
			break;
		case RETROK_KP_DIVIDE:	// '/' on keypad: Toggle GUI 
			pauseg=1;
			break;

		default:
			kbd_handle_keydown( symkey);
			break;
	}
}

void retro_poll_event(int joyon)
{
	Retro_PollEvent(NULL,NULL,NULL);

	if(joyon) // retro joypad take control over keyboard joy
	{

    	BYTE j = joystick_value[cur_port];

    	if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ){
        	j |= 0x01;
    	} else {
        	j &= ~0x01;
    	}
    	if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ){
        	j |= 0x02;
    	} else {
        	j &= ~0x02;
    	}
    	if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ){
        	j |= 0x04;
    	} else {
        	j &=~ 0x04;
    	}
    	if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ){
        	j |= 0x08;
    	} else {
    	    j &= ~0x08;
    	}
    	if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) ){
    	    j |= 0x10;
    	} else {
    	    j &= ~0x10;
    	}
	
    	joystick_value[cur_port] = j;
	}


}


void virtual_kdb(char *buffer,int vx,int vy)
{

   int x, y, page;
   unsigned coul;

#if defined PITCH && PITCH == 4
unsigned *pix=(unsigned*)buffer;
#else
unsigned short *pix=(unsigned short *)buffer;
#endif

   page = (NPAGE == -1) ? 0 : 50;
   coul = RGB565(28, 28, 31);
   BKGCOLOR = (KCOL>0?0xFF808080:0);


   for(x=0;x<NPLGN;x++)
   {
      for(y=0;y<NLIGN;y++)
      {
         DrawBoxBmp((char*)pix,XBASE3+x*XSIDE,YBASE3+y*YSIDE, XSIDE,YSIDE, RGB565(7, 2, 1));
         Draw_text((char*)pix,XBASE0-2+x*XSIDE ,YBASE0+YSIDE*y,coul, BKGCOLOR ,1, 1,20,
               SHIFTON==-1?MVk[(y*NPLGN)+x+page].norml:MVk[(y*NPLGN)+x+page].shift);	
      }
   }

   DrawBoxBmp((char*)pix,XBASE3+vx*XSIDE,YBASE3+vy*YSIDE, XSIDE,YSIDE, RGB565(31, 2, 1));
   Draw_text((char*)pix,XBASE0-2+vx*XSIDE ,YBASE0+YSIDE*vy,RGB565(2,31,1), BKGCOLOR ,1, 1,20,
         SHIFTON==-1?MVk[(vy*NPLGN)+vx+page].norml:MVk[(vy*NPLGN)+vx+page].shift);	

}

int check_vkey2(int x,int y)
{
   int page;
   //check which key is press
   page= (NPAGE==-1) ? 0 : 50;
   return MVk[y*NPLGN+x+page].val;
}

void retro_virtualkb(void)
{
   // VKBD
   int i;
   //   RETRO        B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
   //   INDEX        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
   static int oldi=-1;

   if(oldi!=-1)
   {
	  kbd_handle_keyup(oldi);
      oldi=-1;
   }

   if(SHOWKEY==1)
   {
      static int vkflag[5]={0,0,0,0,0};		

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && vkflag[0]==0 )
         vkflag[0]=1;
      else if (vkflag[0]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) )
      {
         vkflag[0]=0;
         vky -= 1; 
      }

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && vkflag[1]==0 )
         vkflag[1]=1;
      else if (vkflag[1]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) )
      {
         vkflag[1]=0;
         vky += 1; 
      }

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && vkflag[2]==0 )
         vkflag[2]=1;
      else if (vkflag[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) )
      {
         vkflag[2]=0;
         vkx -= 1;
      }

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && vkflag[3]==0 )
         vkflag[3]=1;
      else if (vkflag[3]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) )
      {
         vkflag[3]=0;
         vkx += 1;
      }

      if(vkx<0)vkx=9;
      if(vkx>9)vkx=0;
      if(vky<0)vky=4;
      if(vky>4)vky=0;

      virtual_kdb(( char *)Retro_Screen,vkx,vky);
 
      i=8;
      if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==0) 	
         vkflag[4]=1;
      else if( !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==1)
      {
         vkflag[4]=0;
         i=check_vkey2(vkx,vky);

         if(i==-1){
            oldi=-1;
		 }
         if(i==-2)
         {
            NPAGE=-NPAGE;oldi=-1;
         }
         else if(i==-3)
         {
            //KDB bgcolor
            KCOL=-KCOL;
            oldi=-1;
         }
         else if(i==-4)
         {
            //VKbd show/hide 			
            oldi=-1;
            Screen_SetFullUpdate(0);
            SHOWKEY=-SHOWKEY;
         }
         else if(i==-5)
         {
			//flilist next
			fliplist_attach_head(8, 1);
            oldi=-1;
         }
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
				SHOWKEY=-SHOWKEY;
				Screen_SetFullUpdate(0);
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

	}


}


