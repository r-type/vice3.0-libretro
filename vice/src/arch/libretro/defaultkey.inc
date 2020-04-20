static void libretro_keyboard()
{
    if (keyconvmap != NULL) {
        keyboard_keyconvmap_free();
    }

    keyboard_keyconvmap_alloc();
    keyboard_keyword_clear();

#if defined(__VIC20__)
    kbd_lshiftrow = 1;
    kbd_lshiftcol = 3;
    kbd_rshiftrow = 6;
    kbd_rshiftcol = 4;
    vshift=KEY_RSHIFT;
    shiftl=KEY_LSHIFT;

    keyboard_parse_set_pos_row(27, 0, 3, 8);                /*          ESC -> Run/Stop     */
    keyboard_parse_set_pos_row(49, 0, 0, 8);                /*            1 -> 1            */
    keyboard_parse_set_pos_row(50, 0, 7, 8);                /*            2 -> 2            */
    keyboard_parse_set_pos_row(51, 1, 0, 8);                /*            3 -> 3            */
    keyboard_parse_set_pos_row(52, 1, 7, 8);                /*            4 -> 4            */
    keyboard_parse_set_pos_row(53, 2, 0, 8);                /*            5 -> 5            */
    keyboard_parse_set_pos_row(54, 2, 7, 8);                /*            6 -> 6            */
    keyboard_parse_set_pos_row(55, 3, 0, 8);                /*            7 -> 7            */
    keyboard_parse_set_pos_row(56, 3, 7, 8);                /*            8 -> 8            */
    keyboard_parse_set_pos_row(57, 4, 0, 8);                /*            9 -> 9            */
    keyboard_parse_set_pos_row(48, 4, 7, 8);                /*            0 -> 0            */
    keyboard_parse_set_pos_row(45, 5, 0, 8);                /*        Minus -> Plus         */
    keyboard_parse_set_pos_row(61, 5, 7, 8);                /*        Equal -> Minus        */
    keyboard_parse_set_pos_row(8, 7, 0, 8);                 /*    Backspace -> Del          */
    keyboard_parse_set_pos_row(9, 0, 2, 8);                 /*          TAB -> Ctrl         */
    keyboard_parse_set_pos_row(113, 0, 6, 8);               /*            Q -> Q            */
    keyboard_parse_set_pos_row(119, 1, 1, 8);               /*            W -> W            */
    keyboard_parse_set_pos_row(101, 1, 6, 8);               /*            E -> E            */
    keyboard_parse_set_pos_row(114, 2, 1, 8);               /*            R -> R            */
    keyboard_parse_set_pos_row(116, 2, 6, 8);               /*            T -> T            */
    keyboard_parse_set_pos_row(121, 3, 1, 8);               /*            Y -> Y            */
    keyboard_parse_set_pos_row(117, 3, 6, 8);               /*            U -> U            */
    keyboard_parse_set_pos_row(105, 4, 1, 8);               /*            I -> I            */
    keyboard_parse_set_pos_row(111, 4, 6, 8);               /*            O -> O            */
    keyboard_parse_set_pos_row(112, 5, 1, 8);               /*            P -> P            */
    keyboard_parse_set_pos_row(91, 5, 6, 8);                /*            [ -> @            */
    keyboard_parse_set_pos_row(93, 6, 1, 8);                /*            ] -> *            */
    keyboard_parse_set_pos_row(13, 7, 1, 8);                /*       Return -> Return       */
    keyboard_parse_set_pos_row(306, 0, 5, 8);               /*    Left Ctrl -> CBM          */
    keyboard_parse_set_pos_row(97, 1, 2, 8);                /*            A -> A            */
    keyboard_parse_set_pos_row(115, 1, 5, 8);               /*            S -> S            */
    keyboard_parse_set_pos_row(100, 2, 2, 8);               /*            D -> D            */
    keyboard_parse_set_pos_row(102, 2, 5, 8);               /*            F -> F            */
    keyboard_parse_set_pos_row(103, 3, 2, 8);               /*            G -> G            */
    keyboard_parse_set_pos_row(104, 3, 5, 8);               /*            H -> H            */
    keyboard_parse_set_pos_row(106, 4, 2, 8);               /*            J -> J            */
    keyboard_parse_set_pos_row(107, 4, 5, 8);               /*            K -> K            */
    keyboard_parse_set_pos_row(108, 5, 2, 8);               /*            L -> L            */
    keyboard_parse_set_pos_row(59, 5, 5, 8);                /*            ; -> :            */
    keyboard_parse_set_pos_row(39, 6, 2, 8);                /*            ' -> ;            */
    keyboard_parse_set_pos_row(96, 0, 1, 8);                /*            ` -> Left Arrow   */
    keyboard_parse_set_pos_row(92, 6, 5, 8);                /*            \ -> =            */
    keyboard_parse_set_pos_row(304, 1, 3, 2);               /*   Left Shift -> Left Shift   */
    keyboard_parse_set_pos_row(301, 1, 3, 64);              /*    Caps Lock -> Shift Lock   */
    keyboard_parse_set_pos_row(122, 1, 4, 8);               /*            Z -> Z            */
    keyboard_parse_set_pos_row(120, 2, 3, 8);               /*            X -> X            */
    keyboard_parse_set_pos_row(99, 2, 4, 8);                /*            C -> C            */
    keyboard_parse_set_pos_row(118, 3, 3, 8);               /*            V -> V            */
    keyboard_parse_set_pos_row(98, 3, 4, 8);                /*            B -> B            */
    keyboard_parse_set_pos_row(110, 4, 3, 8);               /*            N -> N            */
    keyboard_parse_set_pos_row(109, 4, 4, 8);               /*            M -> M            */
    keyboard_parse_set_pos_row(44, 5, 3, 8);                /*            , -> ,            */
    keyboard_parse_set_pos_row(46, 5, 4, 8);                /*            . -> .            */
    keyboard_parse_set_pos_row(47, 6, 3, 8);                /*            / -> /            */
    keyboard_parse_set_pos_row(303, 6, 4, 4);               /*  Right Shift -> Right Shift  */
    keyboard_parse_set_pos_row(32, 0, 4, 8);                /*        Space -> Space        */
    keyboard_parse_set_pos_row(282, 7, 4, 8);               /*           F1 -> F1           */
    keyboard_parse_set_pos_row(283, 7, 4, 1);               /*           F2 -> F2           */
    keyboard_parse_set_pos_row(284, 7, 5, 8);               /*           F3 -> F3           */
    keyboard_parse_set_pos_row(285, 7, 5, 1);               /*           F4 -> F4           */
    keyboard_parse_set_pos_row(286, 7, 6, 8);               /*           F5 -> F5           */
    keyboard_parse_set_pos_row(287, 7, 6, 1);               /*           F6 -> F6           */
    keyboard_parse_set_pos_row(288, 7, 7, 8);               /*           F7 -> F7           */
    keyboard_parse_set_pos_row(289, 7, 7, 1);               /*           F8 -> F8           */
    keyboard_parse_set_pos_row(278, 6, 7, 8);               /*         Home -> CLR/HOME     */
    keyboard_parse_set_pos_row(273, 7, 3, 1);               /*           Up -> CRSR UP      */
    keyboard_parse_set_pos_row(276, 7, 2, 1);               /*         Left -> CRSR LEFT    */
    keyboard_parse_set_pos_row(275, 7, 2, 8);               /*        Right -> CRSR RIGHT   */
    keyboard_parse_set_pos_row(274, 7, 3, 8);               /*         Down -> CRSR DOWN    */
    keyboard_parse_set_pos_row(277, 6, 0, 8);               /*          Ins -> Pound        */
    keyboard_parse_set_pos_row(127, 6, 6, 8);               /*          Del -> Up Arrow     */
#elif defined(__CBM2__)
    kbd_lshiftrow = 8;
    kbd_lshiftcol = 4;
    kbd_rshiftrow = 8;
    kbd_rshiftcol = 4;
    vshift=KEY_LSHIFT;

    keyboard_parse_set_pos_row(27, 8, 1, 8);         /*          ESC -> ESC          */
    keyboard_parse_set_pos_row(49, 9, 1, 8);         /*            1 -> 1            */
    keyboard_parse_set_pos_row(50, 10, 1, 8);        /*            2 -> 2            */
    keyboard_parse_set_pos_row(51, 11, 1, 8);        /*            3 -> 3            */
    keyboard_parse_set_pos_row(52, 12, 1, 8);        /*            4 -> 4            */
    keyboard_parse_set_pos_row(53, 13, 1, 8);        /*            5 -> 5            */
    keyboard_parse_set_pos_row(54, 13, 2, 8);        /*            6 -> 6            */
    keyboard_parse_set_pos_row(55, 14, 1, 8);        /*            7 -> 7            */
    keyboard_parse_set_pos_row(56, 15, 1, 8);        /*            8 -> 8            */
    keyboard_parse_set_pos_row(57, 0, 1, 8);         /*            9 -> 9            */
    keyboard_parse_set_pos_row(48, 1, 1, 8);         /*            0 -> 0            */
    keyboard_parse_set_pos_row(45, 1, 2, 8);         /*        Minus -> Minus        */
    keyboard_parse_set_pos_row(61, 2, 1, 8);         /*        Equal -> Equal        */
    keyboard_parse_set_pos_row(8, 3, 3, 8);          /*    Backspace -> Del          */
    keyboard_parse_set_pos_row(9, 8, 2, 8);          /*          TAB -> TAB          */
    keyboard_parse_set_pos_row(113, 9, 2, 8);        /*            Q -> Q            */
    keyboard_parse_set_pos_row(119, 10, 2, 8);       /*            W -> W            */
    keyboard_parse_set_pos_row(101, 11, 2, 8);       /*            E -> E            */
    keyboard_parse_set_pos_row(114, 12, 2, 8);       /*            R -> R            */
    keyboard_parse_set_pos_row(116, 12, 3, 8);       /*            T -> T            */
    keyboard_parse_set_pos_row(121, 13, 3, 8);       /*            Y -> Y            */
    keyboard_parse_set_pos_row(117, 14, 2, 8);       /*            U -> U            */
    keyboard_parse_set_pos_row(105, 15, 2, 8);       /*            I -> I            */
    keyboard_parse_set_pos_row(111, 0, 2, 8);        /*            O -> O            */
    keyboard_parse_set_pos_row(112, 1, 3, 8);        /*            P -> P            */
    keyboard_parse_set_pos_row(91, 1, 4, 8);         /*            [ -> [            */
    keyboard_parse_set_pos_row(93, 2, 3, 8);         /*            ] -> ]            */
    keyboard_parse_set_pos_row(13, 2, 4, 8);         /*       Return -> Return       */
    keyboard_parse_set_pos_row(306, 8, 5, 8);        /*    Left Ctrl -> CTRL         */
    keyboard_parse_set_pos_row(97, 9, 3, 8);         /*            A -> A            */
    keyboard_parse_set_pos_row(115, 10, 3, 8);       /*            S -> S            */
    keyboard_parse_set_pos_row(100, 11, 3, 8);       /*            D -> D            */
    keyboard_parse_set_pos_row(102, 11, 4, 8);       /*            F -> F            */
    keyboard_parse_set_pos_row(103, 12, 4, 8);       /*            G -> G            */
    keyboard_parse_set_pos_row(104, 13, 4, 8);       /*            H -> H            */
    keyboard_parse_set_pos_row(106, 14, 3, 8);       /*            J -> J            */
    keyboard_parse_set_pos_row(107, 15, 3, 8);       /*            K -> K            */
    keyboard_parse_set_pos_row(108, 0, 3, 8);        /*            L -> L            */
    keyboard_parse_set_pos_row(59, 0, 4, 8);         /*            ; -> ;            */
    keyboard_parse_set_pos_row(39, 1, 5, 8);         /*            ' -> '            */
    keyboard_parse_set_pos_row(304, 8, 4, 2);        /*   Left Shift -> Left Shift   */
    keyboard_parse_set_pos_row(122, 9, 4, 8);        /*            Z -> Z            */
    keyboard_parse_set_pos_row(120, 10, 4, 8);       /*            X -> X            */
    keyboard_parse_set_pos_row(99, 10, 5, 8);        /*            C -> C            */
    keyboard_parse_set_pos_row(118, 11, 5, 8);       /*            V -> V            */
    keyboard_parse_set_pos_row(98, 12, 5, 8);        /*            B -> B            */
    keyboard_parse_set_pos_row(110, 13, 5, 8);       /*            N -> N            */
    keyboard_parse_set_pos_row(109, 14, 4, 8);       /*            M -> M            */
    keyboard_parse_set_pos_row(44, 15, 4, 8);        /*            , -> ,            */
    keyboard_parse_set_pos_row(46, 15, 5, 8);        /*            . -> .            */
    keyboard_parse_set_pos_row(47, 0, 5, 8);         /*            / -> /            */
    keyboard_parse_set_pos_row(303, 8, 4, 2);        /*  Right Shift -> Right Shift  */
    keyboard_parse_set_pos_row(32, 14, 5, 8);        /*        Space -> Space        */
    keyboard_parse_set_pos_row(282, 8, 0, 8);        /*           F1 -> F1           */
    keyboard_parse_set_pos_row(283, 9, 0, 8);        /*           F2 -> F2           */
    keyboard_parse_set_pos_row(284, 10, 0, 8);       /*           F3 -> F3           */
    keyboard_parse_set_pos_row(285, 11, 0, 8);       /*           F4 -> F4           */
    keyboard_parse_set_pos_row(286, 12, 0, 8);       /*           F5 -> F5           */
    keyboard_parse_set_pos_row(287, 13, 0, 8);       /*           F6 -> F6           */
    keyboard_parse_set_pos_row(288, 14, 0, 8);       /*           F7 -> F7           */
    keyboard_parse_set_pos_row(289, 15, 0, 8);       /*           F8 -> F8           */
    keyboard_parse_set_pos_row(290, 0, 0, 8);        /*           F9 -> F9           */
    keyboard_parse_set_pos_row(291, 1, 0, 8);        /*          F10 -> F10          */
    keyboard_parse_set_pos_row(278, 4, 0, 8);        /*         Home -> CLR/HOME     */
    keyboard_parse_set_pos_row(273, 3, 0, 8);        /*           Up -> CRSR UP      */
    keyboard_parse_set_pos_row(276, 3, 1, 8);        /*         Left -> CRSR LEFT    */
    keyboard_parse_set_pos_row(275, 3, 2, 8);        /*        Right -> CRSR RIGHT   */
    keyboard_parse_set_pos_row(274, 2, 0, 8);        /*         Down -> CRSR DOWN    */
    keyboard_parse_set_pos_row(277, 2, 2, 8);        /*          Ins -> Pound        */
    keyboard_parse_set_pos_row(127, 4, 1, 8);        /*          Del -> ?            */
    keyboard_parse_set_pos_row(281, 6, 0, 8);        /*       PgDown -> Norm/Graph   */
    keyboard_parse_set_pos_row(19, 2, 5, 8);         /*  Pause/Break -> Pi           */
    keyboard_parse_set_pos_row(280, 5, 0, 8);        /*         PgUp -> Rev/Off      */
    keyboard_parse_set_pos_row(279, 5, 1, 8);        /*          End -> CE           */
    keyboard_parse_set_pos_row(305, 3, 4, 8);        /*   Right Ctrl -> C=           */
    keyboard_parse_set_pos_row(271, 7, 4, 8);        /* Numpad Enter -> Numpad Enter */
    keyboard_parse_set_pos_row(267, 7, 1, 8);        /*     Numpad / -> Numpad /     */
    keyboard_parse_set_pos_row(263, 4, 2, 8);        /*     Numpad 7 -> Numpad 7     */
    keyboard_parse_set_pos_row(264, 5, 2, 8);        /*     Numpad 8 -> Numpad 8     */
    keyboard_parse_set_pos_row(265, 6, 2, 8);        /*     Numpad 9 -> Numpad 9     */
    keyboard_parse_set_pos_row(269, 7, 2, 8);        /*     Numpad - -> Numpad -     */
    keyboard_parse_set_pos_row(260, 4, 3, 8);        /*     Numpad 4 -> Numpad 4     */
    keyboard_parse_set_pos_row(261, 5, 3, 8);        /*     Numpad 5 -> Numpad 5     */
    keyboard_parse_set_pos_row(262, 6, 3, 8);        /*     Numpad 6 -> Numpad 6     */
    keyboard_parse_set_pos_row(270, 7, 3, 8);        /*     Numpad + -> Numpad +     */
    keyboard_parse_set_pos_row(257, 4, 4, 8);        /*     Numpad 1 -> Numpad 1     */
    keyboard_parse_set_pos_row(258, 5, 4, 8);        /*     Numpad 2 -> Numpad 2     */
    keyboard_parse_set_pos_row(259, 6, 4, 8);        /*     Numpad 3 -> Numpad 3     */
    keyboard_parse_set_pos_row(256, 4, 5, 8);        /*     Numpad 0 -> Numpad 0     */
    keyboard_parse_set_pos_row(266, 5, 5, 8);        /*     Numpad . -> Numpad .     */
#elif defined(__PET__)
    switch(machine_keyboard_type)
    {
        case 0: // Business (US)
            kbd_lshiftrow = 6;
            kbd_lshiftcol = 0;
            kbd_rshiftrow = 6;
            kbd_rshiftcol = 6;
            vshift=KEY_RSHIFT;

            keyboard_parse_set_pos_row(27, 2, 0, 8);         /*          ESC -> ESC          */
            keyboard_parse_set_pos_row(49, 1, 0, 8);         /*            1 -> 1            */
            keyboard_parse_set_pos_row(50, 0, 0, 8);         /*            2 -> 2            */
            keyboard_parse_set_pos_row(51, 9, 1, 8);         /*            3 -> 3            */
            keyboard_parse_set_pos_row(52, 1, 1, 8);         /*            4 -> 4            */
            keyboard_parse_set_pos_row(53, 0, 1, 8);         /*            5 -> 5            */
            keyboard_parse_set_pos_row(54, 9, 2, 8);         /*            6 -> 6            */
            keyboard_parse_set_pos_row(55, 1, 2, 8);         /*            7 -> 7            */
            keyboard_parse_set_pos_row(56, 0, 2, 8);         /*            8 -> 8            */
            keyboard_parse_set_pos_row(57, 9, 3, 8);         /*            9 -> 9            */
            keyboard_parse_set_pos_row(48, 1, 3, 8);         /*            0 -> 0            */
            keyboard_parse_set_pos_row(45, 9, 5, 8);         /*        Minus -> :            */
            keyboard_parse_set_pos_row(61, 0, 3, 8);         /*        Equal -> Minus        */
            keyboard_parse_set_pos_row(8, 4, 7, 8);          /*    Backspace -> Del          */
            keyboard_parse_set_pos_row(9, 4, 0, 8);          /*          TAB -> TAB          */
            keyboard_parse_set_pos_row(113, 5, 0, 8);        /*            Q -> Q            */
            keyboard_parse_set_pos_row(119, 4, 1, 8);        /*            W -> W            */
            keyboard_parse_set_pos_row(101, 5, 1, 8);        /*            E -> E            */
            keyboard_parse_set_pos_row(114, 4, 2, 8);        /*            R -> R            */
            keyboard_parse_set_pos_row(116, 5, 2, 8);        /*            T -> T            */
            keyboard_parse_set_pos_row(121, 4, 3, 8);        /*            Y -> Y            */
            keyboard_parse_set_pos_row(117, 5, 3, 8);        /*            U -> U            */
            keyboard_parse_set_pos_row(105, 4, 5, 8);        /*            I -> I            */
            keyboard_parse_set_pos_row(111, 5, 5, 8);        /*            O -> O            */
            keyboard_parse_set_pos_row(112, 4, 6, 8);        /*            P -> P            */
            keyboard_parse_set_pos_row(91, 5, 6, 8);         /*            [ -> [            */
            keyboard_parse_set_pos_row(93, 2, 4, 8);         /*            ] -> ]            */
            keyboard_parse_set_pos_row(13, 3, 4, 8);         /*       Return -> Return       */
            keyboard_parse_set_pos_row(306, 8, 0, 8);        /*    Left Ctrl -> RVS          */
            keyboard_parse_set_pos_row(97, 3, 0, 8);         /*            A -> A            */
            keyboard_parse_set_pos_row(115, 2, 1, 8);        /*            S -> S            */
            keyboard_parse_set_pos_row(100, 3, 1, 8);        /*            D -> D            */
            keyboard_parse_set_pos_row(102, 2, 2, 8);        /*            F -> F            */
            keyboard_parse_set_pos_row(103, 3, 2, 8);        /*            G -> G            */
            keyboard_parse_set_pos_row(104, 2, 3, 8);        /*            H -> H            */
            keyboard_parse_set_pos_row(106, 3, 3, 8);        /*            J -> J            */
            keyboard_parse_set_pos_row(107, 2, 5, 8);        /*            K -> K            */
            keyboard_parse_set_pos_row(108, 3, 5, 8);        /*            L -> L            */
            keyboard_parse_set_pos_row(59, 2, 6, 8);         /*            ; -> ;            */
            keyboard_parse_set_pos_row(39, 3, 6, 8);         /*            ' -> @            */
            keyboard_parse_set_pos_row(96, 9, 0, 8);         /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(304, 6, 0, 2);        /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(92, 4, 4, 8);         /*            \ -> \            */
            keyboard_parse_set_pos_row(122, 7, 0, 8);        /*            Z -> Z            */
            keyboard_parse_set_pos_row(120, 8, 1, 8);        /*            X -> X            */
            keyboard_parse_set_pos_row(99, 6, 1, 8);         /*            C -> C            */
            keyboard_parse_set_pos_row(118, 7, 1, 8);        /*            V -> V            */
            keyboard_parse_set_pos_row(98, 6, 2, 8);         /*            B -> B            */
            keyboard_parse_set_pos_row(110, 7, 2, 8);        /*            N -> N            */
            keyboard_parse_set_pos_row(109, 8, 3, 8);        /*            M -> M            */
            keyboard_parse_set_pos_row(44, 7, 3, 8);         /*            , -> ,            */
            keyboard_parse_set_pos_row(46, 6, 3, 8);         /*            . -> .            */
            keyboard_parse_set_pos_row(47, 8, 6, 8);         /*            / -> /            */
            keyboard_parse_set_pos_row(303, 6, 6, 4);        /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(32, 8, 2, 8);         /*        Space -> Space        */
            keyboard_parse_set_pos_row(278, 8, 4, 8);        /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(273, 5, 4, 1);        /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(276, 0, 5, 1);        /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(275, 0, 5, 8);        /*        Right -> CRSR RIGHT   */
            keyboard_parse_set_pos_row(274, 5, 4, 8);        /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(277, 9, 4, 8);        /*          Ins -> STOP         */
            keyboard_parse_set_pos_row(127, 1, 5, 8);        /*          Del -> Up arrow     */
            keyboard_parse_set_pos_row(305, 7, 6, 8);        /*   Right Ctrl -> RPT          */
            keyboard_parse_set_pos_row(263, 1, 4, 8);        /*     Numpad 7 -> Numpad 7     */
            keyboard_parse_set_pos_row(264, 0, 4, 8);        /*     Numpad 8 -> Numpad 8     */
            keyboard_parse_set_pos_row(265, 1, 7, 8);        /*     Numpad 9 -> Numpad 9     */
            keyboard_parse_set_pos_row(260, 5, 7, 8);        /*     Numpad 4 -> Numpad 4     */
            keyboard_parse_set_pos_row(261, 2, 7, 8);        /*     Numpad 5 -> Numpad 5     */
            keyboard_parse_set_pos_row(262, 3, 7, 8);        /*     Numpad 6 -> Numpad 6     */
            keyboard_parse_set_pos_row(257, 8, 7, 8);        /*     Numpad 1 -> Numpad 1     */
            keyboard_parse_set_pos_row(258, 7, 7, 8);        /*     Numpad 2 -> Numpad 2     */
            keyboard_parse_set_pos_row(259, 6, 7, 8);        /*     Numpad 3 -> Numpad 3     */
            keyboard_parse_set_pos_row(256, 7, 4, 8);        /*     Numpad 0 -> Numpad 0     */
            keyboard_parse_set_pos_row(266, 6, 4, 8);        /*     Numpad . -> Numpad .     */
            break;
        case 4:
            kbd_lshiftrow = 8;
            kbd_lshiftcol = 0;
            kbd_rshiftrow = 8;
            kbd_rshiftcol = 5;
            vshift=KEY_RSHIFT;

            keyboard_parse_set_pos_row(27, 9, 4, 8);         /*          ESC -> Run/Stop     */
            keyboard_parse_set_pos_row(49, 0, 0, 8);         /*            1 -> !            */
            keyboard_parse_set_pos_row(50, 1, 0, 8);         /*            2 -> "            */
            keyboard_parse_set_pos_row(51, 0, 1, 8);         /*            3 -> #            */
            keyboard_parse_set_pos_row(52, 1, 1, 8);         /*            4 -> $            */
            keyboard_parse_set_pos_row(53, 0, 2, 8);         /*            5 -> %            */
            keyboard_parse_set_pos_row(54, 1, 2, 8);         /*            6 -> '            */
            keyboard_parse_set_pos_row(55, 0, 3, 8);         /*            7 -> &            */
            keyboard_parse_set_pos_row(56, 1, 3, 8);         /*            8 -> \            */
            keyboard_parse_set_pos_row(57, 0, 4, 8);         /*            9 -> (            */
            keyboard_parse_set_pos_row(48, 1, 4, 8);         /*            0 -> )            */
            keyboard_parse_set_pos_row(45, 0, 5, 8);         /*        Minus -> Left arrow   */
            keyboard_parse_set_pos_row(8, 1, 8, 8);          /*    Backspace -> Del          */
            keyboard_parse_set_pos_row(113, 2, 0, 8);        /*            Q -> Q            */
            keyboard_parse_set_pos_row(119, 3, 0, 8);        /*            W -> W            */
            keyboard_parse_set_pos_row(101, 2, 1, 8);        /*            E -> E            */
            keyboard_parse_set_pos_row(114, 3, 1, 8);        /*            R -> R            */
            keyboard_parse_set_pos_row(116, 2, 2, 8);        /*            T -> T            */
            keyboard_parse_set_pos_row(121, 3, 2, 8);        /*            Y -> Y            */
            keyboard_parse_set_pos_row(117, 2, 3, 8);        /*            U -> U            */
            keyboard_parse_set_pos_row(105, 3, 3, 8);        /*            I -> I            */
            keyboard_parse_set_pos_row(111, 2, 4, 8);        /*            O -> O            */
            keyboard_parse_set_pos_row(112, 3, 4, 8);        /*            P -> P            */
            keyboard_parse_set_pos_row(91, 2, 5, 8);         /*            [ -> Up arrow     */
            keyboard_parse_set_pos_row(13, 6, 5, 8);         /*       Return -> Return       */
            keyboard_parse_set_pos_row(306, 9, 0, 8);        /*    Left Ctrl -> RVS ON/OFF   */
            keyboard_parse_set_pos_row(97, 4, 0, 8);         /*            A -> A            */
            keyboard_parse_set_pos_row(115, 5, 0, 8);        /*            S -> S            */
            keyboard_parse_set_pos_row(100, 4, 1, 8);        /*            D -> D            */
            keyboard_parse_set_pos_row(102, 5, 1, 8);        /*            F -> F            */
            keyboard_parse_set_pos_row(103, 4, 2, 8);        /*            G -> G            */
            keyboard_parse_set_pos_row(104, 5, 2, 8);        /*            H -> H            */
            keyboard_parse_set_pos_row(106, 4, 3, 8);        /*            J -> J            */
            keyboard_parse_set_pos_row(107, 5, 3, 8);        /*            K -> K            */
            keyboard_parse_set_pos_row(108, 4, 4, 8);        /*            L -> L            */
            keyboard_parse_set_pos_row(59, 5, 4, 8);         /*            ; -> :            */
            keyboard_parse_set_pos_row(304, 8, 0, 2);        /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(122, 6, 0, 8);        /*            Z -> Z            */
            keyboard_parse_set_pos_row(120, 7, 0, 8);        /*            X -> X            */
            keyboard_parse_set_pos_row(99, 6, 1, 8);         /*            C -> C            */
            keyboard_parse_set_pos_row(118, 7, 1, 8);        /*            V -> V            */
            keyboard_parse_set_pos_row(98, 6, 2, 8);         /*            B -> B            */
            keyboard_parse_set_pos_row(110, 7, 2, 8);        /*            N -> N            */
            keyboard_parse_set_pos_row(109, 6, 3, 8);        /*            M -> M            */
            keyboard_parse_set_pos_row(44, 7, 3, 8);         /*            , -> ,            */
            keyboard_parse_set_pos_row(46, 6, 4, 8);         /*            . -> ;            */
            keyboard_parse_set_pos_row(47, 7, 4, 8);         /*            / -> ?            */
            keyboard_parse_set_pos_row(303, 8, 5, 4);        /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(32, 9, 2, 8);         /*        Space -> Space        */
            keyboard_parse_set_pos_row(278, 9, 1, 8);        /*         Home -> [            */
            keyboard_parse_set_pos_row(273, 1, 6, 1);        /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(276, 0, 7, 1);        /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(275, 0, 7, 8);        /*        Right -> CRSR RIGHT   */
            keyboard_parse_set_pos_row(274, 1, 6, 8);        /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(277, 8, 1, 8);        /*          Ins -> @            */
            keyboard_parse_set_pos_row(127, 0, 6, 8);        /*          Del -> CLR/HOME     */
            keyboard_parse_set_pos_row(280, 8, 2, 8);        /*         PgUp -> ]            */
            keyboard_parse_set_pos_row(279, 9, 3, 8);        /*          End -> <            */
            keyboard_parse_set_pos_row(281, 8, 4, 8);        /*       PgDown -> >            */
            keyboard_parse_set_pos_row(305, 9, 4, 8);        /*   Right Ctrl -> RUN/STOP     */
            keyboard_parse_set_pos_row(271, 9, 7, 8);        /* Numpad Enter -> =            */
            keyboard_parse_set_pos_row(267, 3, 7, 8);        /*     Numpad / -> /            */
            keyboard_parse_set_pos_row(263, 2, 6, 8);        /*     Numpad 7 -> 7            */
            keyboard_parse_set_pos_row(264, 3, 6, 8);        /*     Numpad 8 -> 8            */
            keyboard_parse_set_pos_row(265, 2, 7, 8);        /*     Numpad 9 -> 9            */
            keyboard_parse_set_pos_row(269, 8, 7, 8);        /*     Numpad - -> Minus        */
            keyboard_parse_set_pos_row(260, 4, 6, 8);        /*     Numpad 4 -> 4            */
            keyboard_parse_set_pos_row(261, 5, 6, 8);        /*     Numpad 5 -> 5            */
            keyboard_parse_set_pos_row(262, 4, 7, 8);        /*     Numpad 6 -> 6            */
            keyboard_parse_set_pos_row(270, 7, 7, 8);        /*     Numpad + -> +            */
            keyboard_parse_set_pos_row(257, 6, 6, 8);        /*     Numpad 1 -> 1            */
            keyboard_parse_set_pos_row(258, 7, 6, 8);        /*     Numpad 2 -> 2            */
            keyboard_parse_set_pos_row(259, 6, 7, 8);        /*     Numpad 3 -> 3            */
            keyboard_parse_set_pos_row(256, 8, 6, 8);        /*     Numpad 0 -> 0            */
            keyboard_parse_set_pos_row(266, 9, 6, 8);        /*     Numpad . -> .            */
            break;
    }
#else
    kbd_lshiftrow = 1;
    kbd_lshiftcol = 7;
    kbd_rshiftrow = 6;
    kbd_rshiftcol = 4;
    vshift=KEY_RSHIFT;
    shiftl=KEY_LSHIFT;

    keyboard_parse_set_pos_row(27, 7, 7, 8);                /*          ESC -> Run/Stop     */
    keyboard_parse_set_pos_row(49, 7, 0, 8);                /*            1 -> 1            */
    keyboard_parse_set_pos_row(50, 7, 3, 8);                /*            2 -> 2            */
    keyboard_parse_set_pos_row(51, 1, 0, 8);                /*            3 -> 3            */
    keyboard_parse_set_pos_row(52, 1, 3, 8);                /*            4 -> 4            */
    keyboard_parse_set_pos_row(53, 2, 0, 8);                /*            5 -> 5            */
    keyboard_parse_set_pos_row(54, 2, 3, 8);                /*            6 -> 6            */
    keyboard_parse_set_pos_row(55, 3, 0, 8);                /*            7 -> 7            */
    keyboard_parse_set_pos_row(56, 3, 3, 8);                /*            8 -> 8            */
    keyboard_parse_set_pos_row(57, 4, 0, 8);                /*            9 -> 9            */
    keyboard_parse_set_pos_row(48, 4, 3, 8);                /*            0 -> 0            */
    keyboard_parse_set_pos_row(45, 5, 0, 8);                /*        Minus -> Plus         */
    keyboard_parse_set_pos_row(61, 5, 3, 8);                /*        Equal -> Minus        */
    keyboard_parse_set_pos_row(8, 0, 0, 8);                 /*    Backspace -> Del          */
    keyboard_parse_set_pos_row(9, 7, 2, 8);                 /*          TAB -> Ctrl         */
    keyboard_parse_set_pos_row(113, 7, 6, 8);               /*            Q -> Q            */
    keyboard_parse_set_pos_row(119, 1, 1, 8);               /*            W -> W            */
    keyboard_parse_set_pos_row(101, 1, 6, 8);               /*            E -> E            */
    keyboard_parse_set_pos_row(114, 2, 1, 8);               /*            R -> R            */
    keyboard_parse_set_pos_row(116, 2, 6, 8);               /*            T -> T            */
    keyboard_parse_set_pos_row(121, 3, 1, 8);               /*            Y -> Y            */
    keyboard_parse_set_pos_row(117, 3, 6, 8);               /*            U -> U            */
    keyboard_parse_set_pos_row(105, 4, 1, 8);               /*            I -> I            */
    keyboard_parse_set_pos_row(111, 4, 6, 8);               /*            O -> O            */
    keyboard_parse_set_pos_row(112, 5, 1, 8);               /*            P -> P            */
    keyboard_parse_set_pos_row(91, 5, 6, 8);                /*            [ -> @            */
    keyboard_parse_set_pos_row(93, 6, 1, 8);                /*            ] -> *            */
    keyboard_parse_set_pos_row(13, 0, 1, 8);                /*       Return -> Return       */
    keyboard_parse_set_pos_row(306, 7, 5, 8);               /*    Left Ctrl -> CBM          */
    keyboard_parse_set_pos_row(97, 1, 2, 8);                /*            A -> A            */
    keyboard_parse_set_pos_row(115, 1, 5, 8);               /*            S -> S            */
    keyboard_parse_set_pos_row(100, 2, 2, 8);               /*            D -> D            */
    keyboard_parse_set_pos_row(102, 2, 5, 8);               /*            F -> F            */
    keyboard_parse_set_pos_row(103, 3, 2, 8);               /*            G -> G            */
    keyboard_parse_set_pos_row(104, 3, 5, 8);               /*            H -> H            */
    keyboard_parse_set_pos_row(106, 4, 2, 8);               /*            J -> J            */
    keyboard_parse_set_pos_row(107, 4, 5, 8);               /*            K -> K            */
    keyboard_parse_set_pos_row(108, 5, 2, 8);               /*            L -> L            */
    keyboard_parse_set_pos_row(59, 5, 5, 8);                /*            ; -> :            */
    keyboard_parse_set_pos_row(39, 6, 2, 8);                /*            ' -> ;            */
    keyboard_parse_set_pos_row(96, 7, 1, 8);                /*            ` -> Left Arrow   */
    keyboard_parse_set_pos_row(92, 6, 5, 8);                /*            \ -> =            */
    keyboard_parse_set_pos_row(304, 1, 7, 2);               /*   Left Shift -> Left Shift   */
    keyboard_parse_set_pos_row(301, 1, 7, 64);              /*    Caps Lock -> Shift Lock   */
    keyboard_parse_set_pos_row(122, 1, 4, 8);               /*            Z -> Z            */
    keyboard_parse_set_pos_row(120, 2, 7, 8);               /*            X -> X            */
    keyboard_parse_set_pos_row(99, 2, 4, 8);                /*            C -> C            */
    keyboard_parse_set_pos_row(118, 3, 7, 8);               /*            V -> V            */
    keyboard_parse_set_pos_row(98, 3, 4, 8);                /*            B -> B            */
    keyboard_parse_set_pos_row(110, 4, 7, 8);               /*            N -> N            */
    keyboard_parse_set_pos_row(109, 4, 4, 8);               /*            M -> M            */
    keyboard_parse_set_pos_row(44, 5, 7, 8);                /*            , -> ,            */
    keyboard_parse_set_pos_row(46, 5, 4, 8);                /*            . -> .            */
    keyboard_parse_set_pos_row(47, 6, 7, 8);                /*            / -> /            */
    keyboard_parse_set_pos_row(303, 6, 4, 4);               /*  Right Shift -> Right Shift  */
    keyboard_parse_set_pos_row(32, 7, 4, 8);                /*        Space -> Space        */
    keyboard_parse_set_pos_row(282, 0, 4, 8);               /*           F1 -> F1           */
    keyboard_parse_set_pos_row(283, 0, 4, 1);               /*           F2 -> F2           */
    keyboard_parse_set_pos_row(284, 0, 5, 8);               /*           F3 -> F3           */
    keyboard_parse_set_pos_row(285, 0, 5, 1);               /*           F4 -> F4           */
    keyboard_parse_set_pos_row(286, 0, 6, 8);               /*           F5 -> F5           */
    keyboard_parse_set_pos_row(287, 0, 6, 1);               /*           F6 -> F6           */
    keyboard_parse_set_pos_row(288, 0, 3, 8);               /*           F7 -> F7           */
    keyboard_parse_set_pos_row(289, 0, 3, 1);               /*           F8 -> F8           */
    keyboard_parse_set_pos_row(278, 6, 3, 8);               /*         Home -> CLR/HOME     */
    keyboard_parse_set_pos_row(273, 0, 7, 1);               /*           Up -> CRSR UP      */
    keyboard_parse_set_pos_row(276, 0, 2, 1);               /*         Left -> CRSR LEFT    */
    keyboard_parse_set_pos_row(275, 0, 2, 8);               /*        Right -> CRSR RIGHT   */
    keyboard_parse_set_pos_row(274, 0, 7, 8);               /*         Down -> CRSR DOWN    */
    keyboard_parse_set_pos_row(277, 6, 0, 8);               /*          Ins -> Pound        */
    keyboard_parse_set_pos_row(127, 6, 6, 8);               /*          Del -> Up Arrow     */
    keyboard_parse_set_neg_row(280, -3, 0);                 /*       PageUp -> Restore      */
#endif
}

