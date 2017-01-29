#include "kbd.h"
#include "libretro.h"
#include "joystick.h"
#include "keyboard.h"

extern unsigned int cur_port;

int joystick_handle_key(int kcode, int pressed)
{
    BYTE value = 0;

	BYTE j = joystick_value[cur_port];

    switch (kcode) {
            case RETROK_KP7:               /* North-West */
                value = 0x5;
                break;
            case RETROK_KP8:               /* North */
                value = 0x1;
                break;
            case RETROK_KP9:               /* North-East */
                value = 0x9;
                break;
            case RETROK_KP6:               /* East */
                value = 0x8;
                break;
            case RETROK_KP3:               /* South-East */
                value = 0xa;
                break;
            case RETROK_KP2:               /* South */
            case RETROK_KP5:
                value = 0x2;
                break;
            case RETROK_KP1:               /* South-West */
                value = 0x6;
                break;
            case RETROK_KP4:                  /* West */
                value = 0x4;
                break;
            case RETROK_KP0:
            case RETROK_RCTRL:
                value = 0x10;
                break;
            default:
                /* (make compiler happy) */
                break;
    }

	if(value){
		if (pressed) j |= value;		
		else j &=~value;		

   	 	joystick_value[cur_port] = j;

		//printf("JOY) k:%d v:%d %s\n",kcode,value,pressed?"press":"release");
	}

    return value;
}

int kbd_handle_keydown(int kcode)
{
    if (!joystick_handle_key(kcode, 1))
	{
		//printf("press:%d\n",kcode);
        keyboard_key_pressed((signed long)kcode);
    }

    return 0;
}

int kbd_handle_keyup(int kcode)
{
    if (!joystick_handle_key(kcode, 0))
	{
		//printf("release:%d\n",kcode);
        keyboard_key_released((signed long)kcode);
    }

    return 0;
}
