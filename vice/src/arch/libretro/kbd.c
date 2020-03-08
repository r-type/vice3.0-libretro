#include "kbd.h"
#include "libretro.h"
#include "joystick.h"
#include "keyboard.h"

void kbd_handle_keydown(int kcode)
{
   keyboard_key_pressed((signed long)kcode);
}

void kbd_handle_keyup(int kcode)
{
   keyboard_key_released((signed long)kcode);
}
