/* nuklear - v1.00 - public domain */

/* VICE HEADER */
#include "autostart.h"
#include "vice.h"
#include "lib.h"
#include "machine.h"
#include "ui.h"
#include "attach.h"
#include "util.h"
#include "sid.h"
#include "log.h"
#include "resources.h"
#include "tape.h"
#include "cartridge.h"
#include "imagecontents.h"
#include "tapecontents.h"
#include "diskcontents.h"
#include "videoarch.h"
#include "fliplist.h"
#include "c64model.h"

#define GET_DRIVE(code) ((code)&0x0F)
#define NUMB(a) (sizeof(a) / sizeof(*a))

extern int pauseg;
extern int NPAGE,SHIFTON;
extern int vkey_pressed;
extern int vice_statusbar;
extern unsigned int cur_port;
extern int retrojoy_init;

extern char DISKA_NAME[512];
extern char DISKB_NAME[512];
extern char TAPE_NAME[512];

extern void emu_reset(void);
extern int HandleExtension(char *path,char *ext);
extern void set_drive_type(int drive,int val);
extern void retro_shutdown_core(void);

static void
gui(struct file_browser *browser,struct nk_context *ctx)
{
    struct nk_rect total_space;

    /* window flags */
    static int border = nk_true;
    static int resize = nk_true;
    static int movable = nk_true;
    static int no_scrollbar = nk_false;
    static nk_flags window_flags = 0;
    static int minimizable = nk_true;

    /* window flags */
    window_flags = 0;

    if (border) window_flags |= NK_WINDOW_BORDER;
    if (resize) window_flags |= NK_WINDOW_SCALABLE;
    if (movable) window_flags |= NK_WINDOW_MOVABLE;
    if (no_scrollbar || (pauseg==1 && LOADCONTENT==1) ) window_flags |= NK_WINDOW_NO_SCROLLBAR;
    if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;

    if (nk_begin(ctx,"Vice GUI", nk_rect(10,30, 364, 212), window_flags|NK_WINDOW_TITLE))
    {
	int tmpval;

	if(pauseg==1 && SHOWKEY==1)SHOWKEY=-1;

	// VKB IN GAME
	if(pauseg==0 && SHOWKEY==1)
    	{

        	int x = 0,y = 0;
        	int page = (NPAGE == -1) ? 0 : NLIGN*NPLGN;

       		nk_layout_row_dynamic(ctx, 32, NPLGN);

		vkey_pressed=-1;

   		for(y=0;y<NLIGN;y++)
   		{
   	  		for(x=0;x<NPLGN;x++)
  	   		{               
		
				if (nk_button_text(ctx,SHIFTON==-1?MVk[(y*NPLGN)+x+page].norml:MVk[(y*NPLGN)+x+page].shift , \
				       SHIFTON==-1?strlen(MVk[(y*NPLGN)+x+page].norml):strlen(MVk[(y*NPLGN)+x+page].shift))) {

					LOGI("(%s) pressed! (%d,%d)\n",SHIFTON==-1?MVk[(y*NPLGN)+x+page].norml:MVk[(y*NPLGN)+x+page].shift,x,y);
					vkey_pressed=MVk[(y*NPLGN)+x+page].val;
				}

  	   		}
		}



    	}
	else
	// Filebrowser
	if(pauseg==1 && SHOWKEY==-1 && LOADCONTENT==1)
	{
        static float ratio[] = {0.20f, NK_UNDEFINED};
        float spacing_x = ctx->style.window.spacing.x;

        /* output path directory selector in the menubar */
        ctx->style.window.spacing.x = 0;
        nk_menubar_begin(ctx);
        {
            char *d = browser->directory;
            char *begin = d + 1;
            nk_layout_row_dynamic(ctx, 25, 6);

            while (*d++) {
                if (*d == '/') {
                    *d = '\0';
                    if (nk_button_label(ctx, begin)) {
                        *d++ = '/'; *d = '\0';
                        file_browser_reload_directory_content(browser, browser->directory);
                        break;
                    }
                    *d = '/';
                    begin = d + 1;
                }
            }
        }
        nk_menubar_end(ctx);
        ctx->style.window.spacing.x = spacing_x;

        /* window layout */
        total_space = nk_window_get_content_region(ctx);
        nk_layout_row(ctx, NK_DYNAMIC, total_space.h, 2, ratio);
        nk_group_begin(ctx,"Special", NK_WINDOW_NO_SCROLLBAR);
        {

            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_button_label(ctx,  "Home"))
                file_browser_reload_directory_content(browser, browser->home);
            if (nk_button_label(ctx,"Desktop"))
                file_browser_reload_directory_content(browser, browser->desktop);
            if (nk_button_label(ctx,"/"))
                file_browser_reload_directory_content(browser, "/");
            if (nk_button_label(ctx,"Cancel"))
		LOADCONTENT=-1;
            nk_group_end(ctx);
        }

        /* output directory content window */
        nk_group_begin(ctx, "Content", 0);
        {
            int index = -1;
            size_t i = 0, j = 0, k = 0;
            size_t rows = 0, cols = 0;
            size_t count = browser->dir_count + browser->file_count;

            cols = 1;
            rows = count / cols;
            for (i = 0; i <= rows; i += 1) {
#if 1
                {size_t n = j + cols;
                nk_layout_row_dynamic(ctx, 16, (int)cols);
                for (; j < count && j < n; ++j) {
                    /* draw one row of icons */
                    if (j < browser->dir_count) {
                        /* draw and execute directory buttons */
                        if (nk_button_label(ctx,browser->directories[j]))
                            index = (int)j;
                    } else {
                        /* draw and execute files buttons */
                       
                        size_t fileIndex = ((size_t)j - browser->dir_count);
                       
                        if (nk_button_label(ctx, browser->files[fileIndex])) {
                            strncpy(browser->file, browser->directory, MAX_PATH_LEN);
                            n = strlen(browser->file);
                            strncpy(browser->file + n, browser->files[fileIndex], MAX_PATH_LEN - n);
                            //ret = 1;
			    sprintf(LCONTENT,"%s",browser->file);
			    LOADCONTENT=2;
                        }
                    }
                }}
#else
                {size_t n = k + cols;
                nk_layout_row_dynamic(ctx, 20, (int)cols);
                for (; k < count && k < n; k++) {
                    /* draw one row of labels */
                    if (k < browser->dir_count) {
                        nk_label(ctx, browser->directories[k], NK_TEXT_CENTERED);
                    } else {
                        size_t t = k-browser->dir_count;
                        nk_label(ctx,browser->files[t],NK_TEXT_CENTERED);
                    }
                }}
#endif
            }

            if (index != -1) {
                size_t n = strlen(browser->directory);
                strncpy(browser->directory + n, browser->directories[index], MAX_PATH_LEN - n);
                n = strlen(browser->directory);
                if (n < MAX_PATH_LEN - 1) {
                    browser->directory[n] = '/';
                    browser->directory[n+1] = '\0';
                }
                file_browser_reload_directory_content(browser, browser->directory);

            }
            nk_group_end(ctx);
        }
    

	}
	else
	// GUI IN PAUSE
	if(pauseg==1 && SHOWKEY==-1 && LOADCONTENT!=1)
        {
	    #define DEFHSZ 16
	    #define DEFWSZ 64

	    //joystick options
	    static int joy1on = nk_false;
    	    static int joy2on = nk_false;
    	    static int retrojoyon = nk_false;

	    if(cur_port==1){
		joy1on = nk_true;
		joy2on = nk_false;
	    }
	    else if(cur_port==2){
		joy2on = nk_true;
		joy1on = nk_false;
	    }
	    if(retrojoy_init)resources_get_int("RetroJoy",&tmpval);
	    else tmpval=0;

	    if(tmpval)retrojoyon=nk_true;
	    else retrojoyon =nk_false;

	    //misc options
	    static int showled = nk_false;
		
	    if (vice_statusbar) { 
		showled = nk_true;
	    }
	    else showled = nk_false;

	    //c64 sid model
	    int sid_item=0;
            static int sidmod=0;
	    sid_engine_model_t **list = sid_get_engine_model_list();

	    for (sid_item = 0; list[sid_item]; ++sid_item) {}

	    //c64 model option
	    static int current_model = 0;
	    static int selected_model = 0;
	    static int list_model = 0;
	    static const char *c64mod[] =  {"C64 PAL","C64C PAL","C64 old PAL","C64 NTSC","C64C NTSC","C64 old NTSC","Drean","C64 SX PAL","C64 SX NTSC","Japanese","C64 GS","PET64 PAL","PET64 NTSC","MAX Machine"};
	    int c64modint[] ={
		C64MODEL_C64_PAL ,C64MODEL_C64C_PAL ,C64MODEL_C64_OLD_PAL ,C64MODEL_C64_NTSC ,
		C64MODEL_C64C_NTSC ,C64MODEL_C64_OLD_NTSC ,C64MODEL_C64_PAL_N ,C64MODEL_C64SX_PAL ,C64MODEL_C64SX_NTSC ,
		C64MODEL_C64_JAP ,C64MODEL_C64_GS ,C64MODEL_PET64_PAL,C64MODEL_PET64_NTSC ,C64MODEL_ULTIMAX 
	    };
	 	
	    current_model = c64model_get();
	    for(tmpval=0;tmpval< NUMB(c64modint);tmpval++)
		if(c64modint[tmpval]==current_model)break;
	    list_model = tmpval;

	    //floppy option
	    static int fliplst = nk_false;
	    static int DriveTrueEmu = nk_false;
	    static int current_drvtype = 2;
	    static int old_drvtype = 2;
	    static char DF8NAME[512]="Choose Content\0";
	    static char DF9NAME[512]="Choose Content\0";
	    static const char *drivename[] =  {"1540","1541","1542","1551","1570","1571","1573","1581","2000","4000","2031","2040","3040","4040","1001","8050","8250"};

	    resources_get_int("DriveTrueEmulation",&tmpval);

	    if(tmpval)DriveTrueEmu=nk_true;
	    else DriveTrueEmu=nk_false;

	    // button toggle GUI/EMU
            nk_layout_row_dynamic(ctx, DEFHSZ, 3);
            if (nk_button_label(ctx, "Resume")){
                fprintf(stdout, "quit GUI\n");
		pauseg=0;
	    }
            if (nk_button_label(ctx, "Reset")){
                fprintf(stdout, "quit GUI & reset\n");
		pauseg=0;
		emu_reset();
	    }
            if (nk_button_label(ctx, "Quit")){
                fprintf(stdout, "quit GUI & emu\n");
		pauseg=0;
		retro_shutdown_core();
	    }

	    //joystick options
            nk_layout_row_dynamic(ctx, DEFHSZ, 3);
            nk_checkbox_label(ctx, "Joy1 on", &joy1on);
            nk_checkbox_label(ctx, "Joy2 on", &joy2on);
            nk_checkbox_label(ctx, "RetroJoy on", &retrojoyon);

	    if(joy1on && cur_port!=1){
		cur_port=1;
		joy2on=false;
	    }
	    else if (joy2on && cur_port!=2){
	    	cur_port=2;
		joy1on=false;
   	    }

	    if(retrojoy_init)resources_get_int("RetroJoy",&tmpval);
	    else tmpval=0;
		
	    if(retrojoyon){
		if(!tmpval)
		    resources_set_int( "RetroJoy", 1);
	    }
	    else if(tmpval)
		resources_set_int("RetroJoy", 0);

	    //misc options
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_checkbox_label(ctx, "Statusbar", &showled);

	    if(showled){
		if(!vice_statusbar)
			vice_statusbar = 1;
	    }
	    else if(vice_statusbar)
		vice_statusbar = 0;

	    //C64 Modele

	    nk_layout_row_static(ctx, DEFHSZ, 100, 2);
            nk_label(ctx, "C64 model:", NK_TEXT_LEFT);
	    tmpval = nk_combo(ctx, c64mod, LEN(c64mod), list_model, DEFHSZ, nk_vec2(200,200));
	    selected_model=c64modint[tmpval];
            if (selected_model != current_model)
	        c64model_set(selected_model);

	    //c64 sid model

            int sengine, smodel,cursidmod;
            resources_get_int("SidEngine", &sengine);
            resources_get_int("SidModel", &smodel);
	    cursidmod=((sengine << 8) | smodel) ;

            sidmod=cursidmod;

            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_label(ctx, "C64 SID:", NK_TEXT_LEFT);
            nk_layout_row_static(ctx, DEFHSZ, 250, 1);
	    for (tmpval = 0; list[tmpval]; ++tmpval) {
		int sdata=list[tmpval]->value;
                sidmod = nk_option_label(ctx, (char*)list[tmpval]->name, sidmod == sdata) ? sdata : sidmod;
	    }

	    if(sidmod!=cursidmod){
        	sengine = sidmod >> 8;
        	smodel = sidmod & 0xff;
        	sid_set_engine_model(sengine, smodel);
		printf("sid change to %d\n",sidmod);
	    }

	    //floppy option

	    nk_layout_row_static(ctx, DEFHSZ, 100, 2);
            nk_label(ctx, "Drive8Type:", NK_TEXT_LEFT);
	    current_drvtype = nk_combo(ctx, drivename, LEN(drivename), current_drvtype, DEFHSZ, nk_vec2(200,200));

	    if (old_drvtype != current_drvtype){

		    char str[100];
		    int val;
	      	    snprintf(str, sizeof(str), "%s", drivename[current_drvtype]);
	      	    val = strtoul(str, NULL, 0);
		    set_drive_type(8, val);
		    old_drvtype=current_drvtype;
	    }    
	
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_checkbox_label(ctx, "Attach to Fliplist", &fliplst);

            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_checkbox_label(ctx, "DriveTrueEmulation", &DriveTrueEmu);

	    resources_get_int("DriveTrueEmulation",&tmpval);

	    if(DriveTrueEmu){
		if(!tmpval)
		    resources_set_int("DriveTrueEmulation", 1);
	    }	
	    else if(tmpval)
		resources_set_int("DriveTrueEmulation", 0);

	    int i;

	    for(i=0;i<2;i++)
		if(LOADCONTENT==2 && LDRIVE==(i+8));
		else if( (i==0? DISKA_NAME: DISKB_NAME)!=NULL){
		    sprintf((i==0?DF8NAME:DF9NAME),"%s",(i==0? DISKA_NAME: DISKB_NAME));
		}
		//else sprintf(LCONTENT,"Choose Content\0");
	     
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_label(ctx, "DF8:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);

            if (nk_button_label(ctx, DF8NAME)){
                fprintf(stdout, "LOAD DISKA\n");
		LOADCONTENT=1;
		LDRIVE=8;
	    }

            nk_layout_row_dynamic(ctx, DEFHSZ, 1);
            nk_label(ctx, "DF9:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, DEFHSZ, 1);

            if (nk_button_label(ctx, DF9NAME)){
                fprintf(stdout, "LOAD DISKB\n");
		LOADCONTENT=1;
		LDRIVE=9;
	    }
	    if(LOADCONTENT==2 && strlen(LCONTENT) > 0){

		fprintf(stdout, "LOAD CONTENT DF%d (%s)\n",LDRIVE,LCONTENT);

		sprintf((LDRIVE==8? DISKA_NAME: DISKB_NAME),"%s",LCONTENT);
		LOADCONTENT=-1;

		cartridge_detach_image(-1);
		tape_image_detach(1);

		if(HandleExtension(LCONTENT,"CRT") || HandleExtension(LCONTENT,"crt"))
			cartridge_attach_image(CARTRIDGE_CRT, LCONTENT);
		else {

			if(fliplst){
				file_system_detach_disk(GET_DRIVE(LDRIVE));
				printf("Attach to flip list\n");
				file_system_attach_disk(LDRIVE, LCONTENT);
				fliplist_add_image(LDRIVE);
			}
			else if (LDRIVE==9) {
				file_system_detach_disk(GET_DRIVE(LDRIVE));
				printf("Attach DF9 disk\n");
				file_system_attach_disk(LDRIVE, LCONTENT);
			}
			else {
				printf("autostart\n");
				autostart_autodetect(LCONTENT, NULL, 0, AUTOSTART_MODE_RUN);
			}

		}

	    }
	    else if(LOADCONTENT==2)LOADCONTENT=-1;


        }

   }

   nk_end(ctx);
}

