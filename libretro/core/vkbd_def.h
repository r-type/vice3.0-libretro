#ifndef VKBD_DEF_H
#define VKBD_DEF_H 1

typedef struct {
	char norml[NLETT];
	char shift[NLETT];
	int val;	
} Mvk;


Mvk MVk[NPLGN*NLIGN*2]={

	{ " 1" ," !" , RETROK_1 },//0
	{ " 2" ," \"" ,RETROK_2 },
	{ " 3" ," 3"  ,RETROK_3 },
	{ " 4" ," $"  ,RETROK_4 },
	{ " 5" ," %"  ,RETROK_5 },
	{ " 6" ," ^"  ,RETROK_6 },
	{ " 7" ," &"  ,RETROK_7 },
	{ " 8" ," *"  ,RETROK_8 },
	{ " 9" ," ("  ,RETROK_9 },
	{ " 0" ," )"  ,RETROK_0 },

	{ " q" ," Q"  ,RETROK_q}, //10
	{ " w" ," W"  ,RETROK_w},
	{ " e" ," E"  ,RETROK_e},
	{ " r" ," R"  ,RETROK_r},
	{ " t" ," T"  ,RETROK_t},
	{ " y" ," Y"  ,RETROK_y},
	{ " u" ," U"  ,RETROK_u},
	{ " i" ," I"  ,RETROK_i},
	{ " o" ," O"  ,RETROK_o},
	{ " p" ," P"  ,RETROK_p},

	{ " a" ," A"  ,RETROK_a}, //20
	{ " s" ," S"  ,RETROK_s},
	{ " d" ," D"  ,RETROK_d},
	{ " f" ," F"  ,RETROK_f},
	{ " g" ," G"  ,RETROK_g},
	{ " h" ," H"  ,RETROK_h},
	{ " j" ," J"  ,RETROK_j},
	{ " k" ," K"  ,RETROK_k},	
	{ " l" ," L"  ,RETROK_l},
	{ " ;" ," :"  ,RETROK_SEMICOLON},

	{ " z" ," Z"  ,RETROK_z},//30
	{ " x" ," X"  ,RETROK_x},
	{ " c" ," C"  ,RETROK_c},
	{ " v" ," V"  ,RETROK_v},
	{ " b" ," B"  ,RETROK_b},
	{ " n" ," N"  ,RETROK_n},
	{ " m"," M"   ,RETROK_m},
	{ " ,"," <"   ,RETROK_COMMA},
	{ " ."," >"   ,RETROK_COLON},
	{ " /" ," /"  ,RETROK_SLASH},

	{ "PG2","PG2" ,-2}, //40
	{ "Esc","Esc" ,RETROK_ESCAPE},
	{ "F1" ,"F1"  ,RETROK_F1},
	{ "F3" ,"F3"  ,RETROK_F3},
	{ "F5" ,"F5"  ,RETROK_F5},
	{ "F7" ,"F7"  ,RETROK_F7},
	{ "C=" ,"c=" , RETROK_RCTRL},
	{ "Spc" ,"Spc",RETROK_SPACE},
	{ "<-" ,"<-"  ,RETROK_BACKSPACE},
	{ "Ent" ,"Ent",RETROK_RETURN},


	{ "DEL" ,"DEL" ,RETROK_DELETE}, //50
	{ "UP" ,"F3"   ,RETROK_UP},
	{ "CLR" ,"CLR" ,RETROK_CLEAR},
	{ "F5" ,"F5"  ,RETROK_F5},
	{ "SHL" ,"F6"  ,RETROK_LSHIFT},
	{ " @ " ," @ " , RETROK_AT},
	{ " ^ " ," ^ "  ,RETROK_CARET},	
	{ " : " ," : "  ,RETROK_COLON},
	{ " = "," = " ,RETROK_EQUALS},
	{ "SHR","SHR" ,RETROK_RSHIFT},


	{ "LF" ,"LF" , RETROK_LEFT },//60
	{ "Ent" ,"Ent" ,RETROK_RETURN},
	{ "RG" ,"RG"  ,RETROK_RIGHT},
	{ " 4" ," $"  ,RETROK_4 },
	{ " 5" ," %"  ,RETROK_5 },
	{ " 6" ," ^"  ,RETROK_6 },
	{ " 7" ," &"  ,RETROK_7 },
	{ " 8" ," *"  ,RETROK_8 },
	{ " 9" ," ("  ,RETROK_9 },
	{ " 0" ," )"  ,RETROK_0},

	{ "HOM","HOM" ,RETROK_HOME}, //70
	{ "DW" ,"DW" ,RETROK_DOWN}, 
	{ " - " ," - " , RETROK_MINUS},
	{ "CTL" ,"CTL" , RETROK_LCTRL},
	{ " * "," * " ,RETROK_RIGHTBRACKET},
	{ " + "," + " ,RETROK_PLUS},
	{ "AMP","AMP" ,RETROK_AMPERSAND},
	{ "Ins","Ins" ,RETROK_INSERT},
	{ "Hme","Hme" ,RETROK_HOME},
	{ "PgU","PgU" ,RETROK_PAGEUP},

	{ "Tab" ,"Tab",RETROK_TAB}, //80	
	{ " [" ,"  {" ,RETROK_LEFTBRACKET},
	{ " ]" ,"  }" ,RETROK_RIGHTBRACKET},
	{ "Ent" ,"Ent",RETROK_RETURN},
	{ "Del" ,"Del",RETROK_DELETE},
	{ "R/S" ,"End",RETROK_END},
	{ "c= " ,"c= ",RETROK_RCTRL},
	{ "SHL" ,"SHL",RETROK_LSHIFT},
	{ "SHR" ,"SHR" ,RETROK_RSHIFT},	
	{ "SPC" ,"SPC",RETROK_SPACE},	
	
	{ "PG1","PG1" ,-2},//90
	{ "JOY","JOY"   ,-14},
	{ "GUI","GUI"  ,-13},
	{ "CTR" ,"CTR" ,-12},
	{ "R/S" ,"R/S" ,-11},
	{ "SHI" ,"SHI" ,-10},
	{ "VFL","VFL",-5},
	{ "COL" ,"COL",-3},
	{ "Ent" ,"Ent",RETROK_RETURN},
	{ "KBD" ,"KBD",-4},

} ;

#endif
