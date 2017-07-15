#include "libretro.h"
#include "libretro-core.h"

//CORE VAR
#ifdef _WIN32
char slash = '\\';
#else
char slash = '/';
#endif

bool retro_load_ok = false;

char RETRO_DIR[512];

char DISKA_NAME[512]="\0";
char DISKB_NAME[512]="\0";
char TAPE_NAME[512]="\0";

//TIME
#ifdef __CELLOS_LV2__
#include "sys/sys_time.h"
#include "sys/timer.h"
#define usleep  sys_timer_usleep
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#endif

int cpuloop=1;

#ifdef FRONTEND_SUPPORTS_RGB565
	uint16_t *Retro_Screen;
	uint16_t bmp[WINDOW_SIZE];
	uint16_t save_Screen[WINDOW_SIZE];
#else
	unsigned int *Retro_Screen;
	unsigned int bmp[WINDOW_SIZE];
	unsigned int save_Screen[WINDOW_SIZE];
#endif 

int vice_statusbar=0;

//SOUND
short signed int SNDBUF[1024*2];
//FIXME: handle 50/60
int snd_sampler = 44100 / 50;

//PATH
char RPATH[512];

int pauseg=0; //emu status run/pause/end
int want_quit=0;

extern int MOUSE_EMULATED;
extern int SHOWKEY;

extern int app_init(void);
extern int app_free(void);
extern int app_render(int poll);

int CROP_WIDTH;
int CROP_HEIGHT;
int VIRTUAL_WIDTH;

#if defined(__CBM2__)
int	retrow=704;
int	retroh=266;
#elif defined(__CBM5X__) 
int	retrow=448;
int	retroh=284;
#elif defined(__VIC20__)
int	retrow=448;
int	retroh=284;
#else
int	retrow=384;
int	retroh=272;
#endif

#include "vkbd.i"

unsigned vice_devices[ 2 ];

extern int RETROJOY,RETROTDE,RETROSTATUS,RETRODRVTYPE,RETROSIDMODL;
extern int retrojoy_init,retro_ui_finalized;
extern void set_drive_type(int drive,int val);
extern void set_truedrive_emultion(int val);

//VICE DEF BEGIN
#include "resources.h"
#include "sid.h"
//VICE DEF END

extern void Emu_init(void);
extern void Emu_uninit(void);
extern void vice_main_exit(void);
extern void emu_reset(void);

const char *retro_save_directory;
const char *retro_system_directory;
const char *retro_content_directory;
char retro_system_data_directory[512];;

/*static*/ retro_input_state_t input_state_cb;
/*static*/ retro_input_poll_t input_poll_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

static char CMDFILE[512];

int loadcmdfile(char *argv)
{
   int res=0;

   FILE *fp = fopen(argv,"r");

   if( fp != NULL )
   {
      if ( fgets (CMDFILE , 512 , fp) != NULL )
         res=1;	
      fclose (fp);
   }

   return res;
}

int HandleExtension(char *path,char *ext)
{
   int len = strlen(path);

   if (len >= 4 &&
         path[len-4] == '.' &&
         path[len-3] == ext[0] &&
         path[len-2] == ext[1] &&
         path[len-1] == ext[2])
   {
      return 1;
   }

   return 0;
}

#include <ctype.h>

//Args for experimental_cmdline
static char ARGUV[64][1024];
static unsigned char ARGUC=0;

// Args for Core
static char XARGV[64][1024];
static const char* xargv_cmd[64];
int PARAMCOUNT=0;

extern int  skel_main(int argc, char *argv[]);
void parse_cmdline( const char *argv );

void Add_Option(const char* option)
{
   static int first=0;

   if(first==0)
   {
      PARAMCOUNT=0;	
      first++;
   }

   sprintf(XARGV[PARAMCOUNT++],"%s",option);
}

int pre_main(const char *argv)
{
   int i=0;
   bool Only1Arg;

   if (strlen(argv) > strlen("cmd"))
   {
      if( HandleExtension((char*)argv,"cmd") || HandleExtension((char*)argv,"CMD"))
         i=loadcmdfile((char*)argv);     
   }

   if(i==1)
   {
      parse_cmdline(CMDFILE);      
      LOGI("Starting game from command line :%s\n",CMDFILE);  
   }
   else
      parse_cmdline(argv); 

   Only1Arg = (strcmp(ARGUV[0],CORE_NAME) == 0) ? 0 : 1;

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;


   if(Only1Arg)
   {  Add_Option(CORE_NAME);
      /*
         if (strlen(RPATH) >= strlen("crt"))
         if(!strcasecmp(&RPATH[strlen(RPATH)-strlen("crt")], "crt"))
         Add_Option("-cartcrt");
         */
      Add_Option(RPATH/*ARGUV[0]*/);
   }
   else
   { // Pass all cmdline args
      for(i = 0; i < ARGUC; i++)
         Add_Option(ARGUV[i]);
   }

   for (i = 0; i < PARAMCOUNT; i++)
   {
      xargv_cmd[i] = (char*)(XARGV[i]);
      LOGI("%2d  %s\n",i,XARGV[i]);
   }

   skel_main(PARAMCOUNT,( char **)xargv_cmd); 

   xargv_cmd[PARAMCOUNT - 2] = NULL;

   return 0;
}

void parse_cmdline(const char *argv)
{
   char *p,*p2,*start_of_word;
   int c,c2;
   static char buffer[512*4];
   enum states { DULL, IN_WORD, IN_STRING } state = DULL;

   strcpy(buffer,argv);
   strcat(buffer," \0");

   for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }	
   }
}

long GetTicks(void)
{ // in MSec
#ifndef _ANDROID_

#ifdef __CELLOS_LV2__

   //#warning "GetTick PS3\n"

   unsigned long	ticks_micro;
   uint64_t secs;
   uint64_t nsecs;

   sys_time_get_current_time(&secs, &nsecs);
   ticks_micro =  secs * 1000000UL + (nsecs / 1000);

   return ticks_micro;///1000;
#else
   struct timeval tv;
   gettimeofday (&tv, NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec);///1000;

#endif

#else

   struct timespec now;
   clock_gettime(CLOCK_MONOTONIC, &now);
   return (now.tv_sec*1000000 + now.tv_nsec/1000);///1000;
#endif

} 

void save_bkg(void)
{
   memcpy(save_Screen,Retro_Screen,PITCH*WINDOW_SIZE);
}

void restore_bgk(void)
{
   memcpy(Retro_Screen,save_Screen,PITCH*WINDOW_SIZE);
}

void Screen_SetFullUpdate(int scr)
{
   if(scr==0 ||scr>1)
      memset(&Retro_Screen, 0, sizeof(Retro_Screen));
   if(scr>0)
      memset(&bmp,0,sizeof(bmp));
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   static const struct retro_controller_description p1_controllers[] = {
      { "Vice Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Vice Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
   };
   static const struct retro_controller_description p2_controllers[] = {
      { "Vice Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Vice Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
   };


   static const struct retro_controller_info ports[] = {
      { p1_controllers, 2  }, // port 1
      { p2_controllers, 2  }, // port 2
      { NULL, 0 }
   };

   cb( RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports );

   struct retro_variable variables[] = {
      {
         "vice_Statusbar",
         "Status Bar; disabled|enabled",
      },
      {
         "vice_Drive8Type",
         "Drive8Type; 1541|1540|1542|1551|1570|1571|1573|1581|2000|4000|2031|2040|3040|4040|1001|8050|8250",
      },
      {
         "vice_DriveTrueEmulation",
         "DriveTrueEmulation; enabled|disabled",
      },
      {
         "vice_SidModel",
         "SidModel; 6581F|8580F|6581R|8580R|8580RD",
      },
      {
         "vice_RetroJoy",
         "Retro joy0; enabled|disabled",
      },
      {
         "vice_Controller",
         "Controller0 type; keyboard|joystick",
      },
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

static void update_variables(void)
{

   struct retro_variable var;

   var.key = "vice_Statusbar";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if(retro_ui_finalized){
         if (strcmp(var.value, "enabled") == 0)
            vice_statusbar=1;
         if (strcmp(var.value, "disabled") == 0)
            vice_statusbar=0;
      }
      else {
         if (strcmp(var.value, "enabled") == 0)RETROSTATUS=1;
         if (strcmp(var.value, "disabled") == 0)RETROSTATUS=0;
      }
   }

   var.key = "vice_Drive8Type";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      char str[100];
      int val;
      snprintf(str, sizeof(str), "%s", var.value);
      val = strtoul(str, NULL, 0);

      if(retro_ui_finalized)
         set_drive_type(8, val);
      else RETRODRVTYPE=val;
   }

   var.key = "vice_DriveTrueEmulation";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if(retro_ui_finalized){
         if (strcmp(var.value, "enabled") == 0)
            set_truedrive_emultion(1);
         if (strcmp(var.value, "disabled") == 0)
            set_truedrive_emultion(0);
      }
      else  {
         if (strcmp(var.value, "enabled") == 0)RETROTDE=1;
         if (strcmp(var.value, "disabled") == 0)RETROTDE=0;
      }
   }

   var.key = "vice_SidModel";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int eng=0,modl=0,sidmdl=0;

      if (strcmp(var.value, "6581F") == 0){eng=0;modl=0;}
      else if (strcmp(var.value, "8580F") == 0){eng=0;modl=1;}
      else if (strcmp(var.value, "6581R") == 0){eng=1;modl=0;}
      else if (strcmp(var.value, "8580R") == 0){eng=1;modl=1;}
      else if (strcmp(var.value, "8580RD") == 0){eng=1;modl=2;}

      sidmdl=((eng << 8) | modl) ;

      if(retro_ui_finalized)
        sid_set_engine_model(eng, modl);
      else RETROSIDMODL=sidmdl;
   }

   var.key = "vice_RetroJoy";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if(retrojoy_init){
         if (strcmp(var.value, "enabled") == 0)
            resources_set_int( "RetroJoy", 1);
         if (strcmp(var.value, "disabled") == 0)
            resources_set_int( "RetroJoy", 0);
      }
      else {
         if (strcmp(var.value, "enabled") == 0)RETROJOY=1;
         if (strcmp(var.value, "disabled") == 0)RETROJOY=0;
      }
   }

   var.key = "vice_Controller";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "keyboard") == 0)
         vice_devices[ 0 ] = 259;
      if (strcmp(var.value, "joystick") == 0)
         vice_devices[ 0 ] = 513;	  
   }

}


void Emu_init(void)
{
#ifdef RETRO_AND
   MOUSE_EMULATED=1;
#endif 
   update_variables();
   pre_main(RPATH);
}

void Emu_uninit(void)
{
   vice_main_exit();
}

void retro_shutdown_core(void)
{
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

void retro_reset(void)
{
   emu_reset();
}

void retro_init(void)
{    	
   const char *system_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
   {
      // if defined, use the system directory			
      retro_system_directory=system_dir;		
   }		   

   const char *content_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
   {
      // if defined, use the system directory			
      retro_content_directory=content_dir;		
   }			

   const char *save_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
   {
      // If save directory is defined use it, otherwise use system directory
      retro_save_directory = *save_dir ? save_dir : retro_system_directory;      
   }
   else
   {
      // make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend
      retro_save_directory=retro_system_directory;
   }

   if(retro_system_directory==NULL)sprintf(RETRO_DIR, "%s",".");
   else sprintf(RETRO_DIR, "%s", retro_system_directory);

#if defined(__WIN32__) 
   sprintf(retro_system_data_directory, "%s\\data",RETRO_DIR);
#else
   sprintf(retro_system_data_directory, "%s/data",RETRO_DIR);
#endif

#ifdef FRONTEND_SUPPORTS_RGB565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#else
   enum retro_pixel_format fmt =RETRO_PIXEL_FORMAT_XRGB8888;
#endif

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      fprintf(stderr, "PIXEL FORMAT is not supported.\n");
      exit(0);
   }

   struct retro_input_descriptor inputDescriptors[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" }
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputDescriptors);
}

void retro_deinit(void)
{
   app_free(); 
   Emu_uninit(); 
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}


void retro_set_controller_port_device( unsigned port, unsigned device )
{
   if ( port < 2 )
   {
      vice_devices[ port ] = device;

      printf(" (%d)=%d \n",port,device);
   }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "VICE " CORE_NAME;
   info->library_version  = "3.0";
   info->valid_extensions = "d64|d71|d80|d81|d82|g64|g41|x64|t64|tap|prg|p00|crt|bin|zip|gz|d6z|d7z|d8z|g6z|g4z|x6z|cmd";
   info->need_fullpath    = true;
   info->block_extract    = false;

}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   /* FIXME handle PAL/NTSC */
   struct retro_game_geometry geom = { retrow, retroh, retrow, retroh,4.0 / 3.0 };
   struct retro_system_timing timing = { 50.0, 44100.0 };

   info->geometry = geom;
   info->timing   = timing;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_audio_cb( short l, short r)
{
   audio_cb(l,r);
}

void retro_audiocb(signed short int *sound_buffer,int sndbufsize){
   int x; 
   if(pauseg==0)for(x=0;x<sndbufsize;x++)audio_cb(sound_buffer[x],sound_buffer[x]);	
}

void retro_blit(void)
{
   memcpy(Retro_Screen,bmp,PITCH*WINDOW_SIZE);
}

void retro_run(void)
{
   static int mfirst=1;
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   if(mfirst==1)
   {
      mfirst++;
      printf("MAIN FIRST\n");
      retro_load_ok=true;

      Emu_init();
      return;
   }

   if(pauseg==0)
   {
      while(cpuloop==1)
         maincpu_mainloop_retro();
      cpuloop=1;

      retro_blit();
      if(SHOWKEY==1)app_render(0);
   }
   else if (pauseg==1)app_render(1);
   //app_render(pauseg);

   video_cb(Retro_Screen,retrow,retroh,retrow<<PIXEL_BYTES);

   if(want_quit)retro_shutdown_core();
}

/*
   unsigned int lastdown,lastup,lastchar;
   static void keyboard_cb(bool down, unsigned keycode,
   uint32_t character, uint16_t mod)
   {

   printf( "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
   down ? "yes" : "no", keycode, character, mod);


   if(down)lastdown=keycode;
   else lastup=keycode;
   lastchar=character;

   }
   */

bool retro_load_game(const struct retro_game_info *info)
{
   /*
      struct retro_keyboard_callback cb = { keyboard_cb };
      environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
      */
   const char *full_path = info->path;

   strcpy(RPATH,full_path);

   update_variables();

   app_init();

   memset(SNDBUF,0,1024*2*2);

   return true;
}

void retro_unload_game(void){

   pauseg=-1;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

