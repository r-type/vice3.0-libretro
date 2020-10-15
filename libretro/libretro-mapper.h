#ifndef LIBRETRO_MAPPER_H
#define LIBRETRO_MAPPER_H

#define RETRO_DEVICE_VICE_JOYSTICK      RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)
#define RETRO_DEVICE_VICE_KEYBOARD      RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)

#define RETRO_DEVICE_ID_JOYPAD_LR       16
#define RETRO_DEVICE_ID_JOYPAD_LL       17
#define RETRO_DEVICE_ID_JOYPAD_LD       18
#define RETRO_DEVICE_ID_JOYPAD_LU       19
#define RETRO_DEVICE_ID_JOYPAD_RR       20
#define RETRO_DEVICE_ID_JOYPAD_RL       21
#define RETRO_DEVICE_ID_JOYPAD_RD       22
#define RETRO_DEVICE_ID_JOYPAD_RU       23

#define RETRO_MAPPER_VKBD               24
#define RETRO_MAPPER_STATUSBAR          25
#define RETRO_MAPPER_JOYPORT            26
#define RETRO_MAPPER_RESET              27
#define RETRO_MAPPER_ASPECT_RATIO       28
#define RETRO_MAPPER_ZOOM_MODE          29
#define RETRO_MAPPER_WARP_MODE          30

#define RETRO_MAPPER_DATASETTE_HOTKEYS  31
#define RETRO_MAPPER_DATASETTE_STOP     32
#define RETRO_MAPPER_DATASETTE_START    33
#define RETRO_MAPPER_DATASETTE_FORWARD  34
#define RETRO_MAPPER_DATASETTE_REWIND   35
#define RETRO_MAPPER_DATASETTE_RESET    36

#define RETRO_MAPPER_LAST               37

#define TOGGLE_VKBD                     -11
#define TOGGLE_STATUSBAR                -12
#define SWITCH_JOYPORT                  -13
#define MOUSE_SLOWER                    -5
#define MOUSE_FASTER                    -6

typedef struct
{
   int id;
   char value[20];
   char label[25];
} retro_keymap;

static retro_keymap retro_keys[RETROK_LAST] =
{
   {0,                  "---",                 "---"},
   {TOGGLE_VKBD,        "TOGGLE_VKBD",         "Toggle Virtual Keyboard"},
   {TOGGLE_STATUSBAR,   "TOGGLE_STATUSBAR",    "Toggle Statusbar"},
   {SWITCH_JOYPORT,     "SWITCH_JOYPORT",      "Switch Joyport"},
   {MOUSE_SLOWER,       "MOUSE_SLOWER",        "Mouse Slower"},
   {MOUSE_FASTER,       "MOUSE_FASTER",        "Mouse Faster"},
   {RETROK_BACKSPACE,   "RETROK_BACKSPACE",    "Backspace"},
   {RETROK_TAB,         "RETROK_TAB",          "Tab"},
/* {RETROK_CLEAR,       "RETROK_CLEAR",        "Clear"}, */
   {RETROK_RETURN,      "RETROK_RETURN",       "Return"},
/* {RETROK_PAUSE,       "RETROK_PAUSE",        "Pause"}, */
   {RETROK_ESCAPE,      "RETROK_ESCAPE",       "Escape"},
   {RETROK_SPACE,       "RETROK_SPACE",        "Space"},
/* {RETROK_EXCLAIM,     "RETROK_EXCLAIM",      "!"}, */
/* {RETROK_QUOTEDBL,    "RETROK_QUOTEDBL",     "\""}, */
/* {RETROK_HASH,        "RETROK_HASH",         "#"}, */
/* {RETROK_DOLLAR,      "RETROK_DOLLAR",       "$"}, */
/* {RETROK_AMPERSAND,   "RETROK_AMPERSAND",    "&"}, */
   {RETROK_QUOTE,       "RETROK_QUOTE",        "\'"},
   {RETROK_LEFTPAREN,   "RETROK_LEFTPAREN",    "("},
   {RETROK_RIGHTPAREN,  "RETROK_RIGHTPAREN",   ")"},
   {RETROK_ASTERISK,    "RETROK_ASTERISK",     "*"},
   {RETROK_PLUS,        "RETROK_PLUS",         "+"},
   {RETROK_COMMA,       "RETROK_COMMA",        ","},
   {RETROK_MINUS,       "RETROK_MINUS",        "-"},
   {RETROK_PERIOD,      "RETROK_PERIOD",       "."},
   {RETROK_SLASH,       "RETROK_SLASH",        "/"},
   {RETROK_0,           "RETROK_0",            "0"},
   {RETROK_1,           "RETROK_1",            "1"},
   {RETROK_2,           "RETROK_2",            "2"},
   {RETROK_3,           "RETROK_3",            "3"},
   {RETROK_4,           "RETROK_4",            "4"},
   {RETROK_5,           "RETROK_5",            "5"},
   {RETROK_6,           "RETROK_6",            "6"},
   {RETROK_7,           "RETROK_7",            "7"},
   {RETROK_8,           "RETROK_8",            "8"},
   {RETROK_9,           "RETROK_9",            "9"},
   {RETROK_COLON,       "RETROK_COLON",        ":"},
   {RETROK_SEMICOLON,   "RETROK_SEMICOLON",    ";"},
   {RETROK_LESS,        "RETROK_LESS",         "<"},
   {RETROK_EQUALS,      "RETROK_EQUALS",       "="},
   {RETROK_GREATER,     "RETROK_GREATER",      ">"},
/* {RETROK_QUESTION,    "RETROK_QUESTION",     "?"}, */
/* {RETROK_AT,          "RETROK_AT",           "@"}, */
   {RETROK_LEFTBRACKET, "RETROK_LEFTBRACKET",  "["},
   {RETROK_BACKSLASH,   "RETROK_BACKSLASH",    "\\"},
   {RETROK_RIGHTBRACKET,"RETROK_RIGHTBRACKET", "]"},
   {RETROK_CARET,       "RETROK_CARET",        "^"},
   {RETROK_UNDERSCORE,  "RETROK_UNDERSCORE",   "_"},
   {RETROK_BACKQUOTE,   "RETROK_BACKQUOTE",    "`"},
   {RETROK_a,           "RETROK_a",            "A"},
   {RETROK_b,           "RETROK_b",            "B"},
   {RETROK_c,           "RETROK_c",            "C"},
   {RETROK_d,           "RETROK_d",            "D"},
   {RETROK_e,           "RETROK_e",            "E"},
   {RETROK_f,           "RETROK_f",            "F"},
   {RETROK_g,           "RETROK_g",            "G"},
   {RETROK_h,           "RETROK_h",            "H"},
   {RETROK_i,           "RETROK_i",            "I"},
   {RETROK_j,           "RETROK_j",            "J"},
   {RETROK_k,           "RETROK_k",            "K"},
   {RETROK_l,           "RETROK_l",            "L"},
   {RETROK_m,           "RETROK_m",            "M"},
   {RETROK_n,           "RETROK_n",            "N"},
   {RETROK_o,           "RETROK_o",            "O"},
   {RETROK_p,           "RETROK_p",            "P"},
   {RETROK_q,           "RETROK_q",            "Q"},
   {RETROK_r,           "RETROK_r",            "R"},
   {RETROK_s,           "RETROK_s",            "S"},
   {RETROK_t,           "RETROK_t",            "T"},
   {RETROK_u,           "RETROK_u",            "U"},
   {RETROK_v,           "RETROK_v",            "V"},
   {RETROK_w,           "RETROK_w",            "W"},
   {RETROK_x,           "RETROK_x",            "X"},
   {RETROK_y,           "RETROK_y",            "Y"},
   {RETROK_z,           "RETROK_z",            "Z"},
   {RETROK_DELETE,      "RETROK_DELETE",       "Delete"},
   {RETROK_KP0,         "RETROK_KP0",          "Numpad 0"},
   {RETROK_KP1,         "RETROK_KP1",          "Numpad 1"},
   {RETROK_KP2,         "RETROK_KP2",          "Numpad 2"},
   {RETROK_KP3,         "RETROK_KP3",          "Numpad 3"},
   {RETROK_KP4,         "RETROK_KP4",          "Numpad 4"},
   {RETROK_KP5,         "RETROK_KP5",          "Numpad 5"},
   {RETROK_KP6,         "RETROK_KP6",          "Numpad 6"},
   {RETROK_KP7,         "RETROK_KP7",          "Numpad 7"},
   {RETROK_KP8,         "RETROK_KP8",          "Numpad 8"},
   {RETROK_KP9,         "RETROK_KP9",          "Numpad 9"},
   {RETROK_KP_PERIOD,   "RETROK_KP_PERIOD",    "Numpad ."},
   {RETROK_KP_DIVIDE,   "RETROK_KP_DIVIDE",    "Numpad /"},
   {RETROK_KP_MULTIPLY, "RETROK_KP_MULTIPLY",  "Numpad *"},
   {RETROK_KP_MINUS,    "RETROK_KP_MINUS",     "Numpad -"},
   {RETROK_KP_PLUS,     "RETROK_KP_PLUS",      "Numpad +"},
   {RETROK_KP_ENTER,    "RETROK_KP_ENTER",     "Numpad Enter"},
   {RETROK_KP_EQUALS,   "RETROK_KP_EQUALS",    "Numpad ="},
   {RETROK_UP,          "RETROK_UP",           "Up"},
   {RETROK_DOWN,        "RETROK_DOWN",         "Down"},
   {RETROK_RIGHT,       "RETROK_RIGHT",        "Right"},
   {RETROK_LEFT,        "RETROK_LEFT",         "Left"},
   {RETROK_INSERT,      "RETROK_INSERT",       "Insert"},
   {RETROK_HOME,        "RETROK_HOME",         "Home"},
   {RETROK_END,         "RETROK_END",          "End"},
   {RETROK_PAGEUP,      "RETROK_PAGEUP",       "Page Up"},
   {RETROK_PAGEDOWN,    "RETROK_PAGEDOWN",     "Page Down"},
   {RETROK_F1,          "RETROK_F1",           "F1"},
   {RETROK_F2,          "RETROK_F2",           "F2"},
   {RETROK_F3,          "RETROK_F3",           "F3"},
   {RETROK_F4,          "RETROK_F4",           "F4"},
   {RETROK_F5,          "RETROK_F5",           "F5"},
   {RETROK_F6,          "RETROK_F6",           "F6"},
   {RETROK_F7,          "RETROK_F7",           "F7"},
   {RETROK_F8,          "RETROK_F8",           "F8"},
   {RETROK_F9,          "RETROK_F9",           "F9"},
   {RETROK_F10,         "RETROK_F10",          "F10"},
   {RETROK_F11,         "RETROK_F11",          "F11"},
   {RETROK_F12,         "RETROK_F12",          "F12"},
   {RETROK_F13,         "RETROK_F13",          "F13"},
   {RETROK_F14,         "RETROK_F14",          "F14"},
   {RETROK_F15,         "RETROK_F15",          "F15"},
/* {RETROK_NUMLOCK,     "RETROK_NUMLOCK",      "Num Lock"}, */
/* {RETROK_CAPSLOCK,    "RETROK_CAPSLOCK",     "Caps Lock"}, */
/* {RETROK_SCROLLOCK,   "RETROK_SCROLLOCK",    "Scroll Lock"}, */
   {RETROK_RSHIFT,      "RETROK_RSHIFT",       "Right Shift"},
   {RETROK_LSHIFT,      "RETROK_LSHIFT",       "Left Shift"},
   {RETROK_RCTRL,       "RETROK_RCTRL",        "Right Control"},
   {RETROK_LCTRL,       "RETROK_LCTRL",        "Left Control"},
   {RETROK_RALT,        "RETROK_RALT",         "Right Alt"},
   {RETROK_LALT,        "RETROK_LALT",         "Left Alt"},
/* {RETROK_RMETA,       "RETROK_RMETA",        "Right Meta"}, */
/* {RETROK_LMETA,       "RETROK_LMETA",        "Left Meta"}, */
   {RETROK_RSUPER,      "RETROK_RSUPER",       "Right Super"},
   {RETROK_LSUPER,      "RETROK_LSUPER",       "Left Super"},
/* {RETROK_MODE,        "RETROK_MODE",         "Mode"}, */
/* {RETROK_COMPOSE,     "RETROK_COMPOSE",      "Compose"}, */
/* {RETROK_HELP,        "RETROK_HELP",         "Help"}, */
/* {RETROK_PRINT,       "RETROK_PRINT",        "Print"}, */
/* {RETROK_SYSREQ,      "RETROK_SYSREQ",       "Sys Reg"}, */
/* {RETROK_BREAK,       "RETROK_BREAK",        "Break"}, */
/* {RETROK_MENU,        "RETROK_MENU",         "Menu"}, */
/* {RETROK_POWER,       "RETROK_POWER",        "Power"}, */
/* {RETROK_EURO,        "RETROK_EURO",         "Euro"}, */
/* {RETROK_UNDO,        "RETROK_UNDO",         "Undo"}, */
/* {RETROK_OEM_102,     "RETROK_OEM_102",      "OEM-102"} */
   {RETROK_LAST, {0}, {0}}
};

#endif /* LIBRETRO_MAPPER_H */
