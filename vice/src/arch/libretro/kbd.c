#include <stdio.h>
#include <string.h>

#include "kbd.h"
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

signed long kbd_arch_keyname_to_keynum(char *keyname) {
   return (signed long)atoi(keyname);
}

const char *kbd_arch_keynum_to_keyname(signed long keynum) {
   static char keyname[20];

   memset(keyname, 0, 20);
   sprintf(keyname, "%li", keynum);
   return keyname;
}

void kbd_arch_init()
{
   keyboard_clear_keymatrix();
}

void kbd_initialize_numpad_joykeys(int* joykeys)
{
}
