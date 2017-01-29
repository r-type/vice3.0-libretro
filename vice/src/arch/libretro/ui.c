/*
 * ui.c - PSP user interface.
 *
 * Written by
 *  Akop Karapetyan <dev@psp.akop.org>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "machine.h"
#include "cmdline.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

int RETROJOY=0,RETROTDE=0,RETROSTATUS=0,RETRODRVTYPE=1542;
int retro_ui_finalized = 0;
extern int vice_statusbar;

static const cmdline_option_t cmdline_options[] = {
     { NULL }
};

/* Initialization  */
int ui_resources_init(void)
{

    if (machine_class != VICE_MACHINE_VSID) {
        return uistatusbar_init_resources();
    }
    return 0;
}

void ui_resources_shutdown(void)
{

}

int ui_init(void)
{
    return 0;
}

void ui_shutdown(void)
{
}

void ui_check_mouse_cursor(void)
{
    /* needed */
}

void ui_error(const char *format, ...)
{

   	char text[512];	   	
   	va_list	ap;	

   	if (format == NULL)return;		
		
   	va_start(ap,format );		
      		vsprintf(text, format, ap);	
   	va_end(ap);	
    fprintf(stderr, "ui_error: %s\n", text);
}

/* Update all the menus according to the current settings.  */

void ui_update_menus(void)
{
}

int ui_extend_image_dialog()
{
  return 0;
}

void ui_dispatch_events()
{
}

int ui_cmdline_options_init(void)
{
  return cmdline_register_options(cmdline_options);
}

int ui_init_finish()
{
  return 0;
}

int ui_init_finalize()
{
  //FIXME

  resources_set_int( "Mouse", 0);
  resources_set_int( "Mousetype", 0);
  resources_set_int( "Mouseport", 1);

  resources_set_int( "CrtcFilter",0);
  resources_set_int( "CrtcStretchVertical",0);

  //RETRO CORE OPT
  resources_set_int( "SDLStatusbar", 1);
  if(RETROSTATUS==1)vice_statusbar=1;
  else if(RETROSTATUS==0)vice_statusbar=0;

  if(RETROJOY==1)resources_set_int( "RetroJoy", 1);
  else if(RETROJOY==0)resources_set_int( "RetroJoy", 0);

  if(RETROTDE==1)resources_set_int("DriveTrueEmulation", 1);
  else if(RETROTDE==0)resources_set_int("DriveTrueEmulation", 0);

  resources_set_int_sprintf("Drive%iType",RETRODRVTYPE , 8);

  retro_ui_finalized = 1;

  return 0;
}

char* ui_get_file(const char *format,...)
{
    return NULL;
}



