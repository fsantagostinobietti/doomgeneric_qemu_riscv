//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//       Key definitions
//

#ifndef __DOOMKEYS__
#define __DOOMKEYS__

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
#define KEY_RIGHTARROW	0xae
#define KEY_LEFTARROW	0xac
#define KEY_UPARROW		0xad
#define KEY_DOWNARROW	0xaf
#define KEY_STRAFE_L	0xa0
#define KEY_STRAFE_R	0xa1
#define KEY_USE			0xa2
#define KEY_FIRE		0xa3
#define KEY_ESCAPE		27
#define KEY_ENTER		13
#define KEY_TAB			9
#define KEY_F1			(0x80+0x3b)
#define KEY_F2			(0x80+0x3c)
#define KEY_F3			(0x80+0x3d)
#define KEY_F4			(0x80+0x3e)
#define KEY_F5			(0x80+0x3f)
#define KEY_F6			(0x80+0x40)
#define KEY_F7			(0x80+0x41)
#define KEY_F8			(0x80+0x42)
#define KEY_F9			(0x80+0x43)
#define KEY_F10			(0x80+0x44)
#define KEY_F11			(0x80+0x57)
#define KEY_F12			(0x80+0x58)

#define KEY_BACKSPACE	0x7f
#define KEY_PAUSE	0xff

#define KEY_EQUALS	0x3d
#define KEY_MINUS	0x2d

#define KEY_RSHIFT	(0x80+0x36)
#define KEY_RCTRL	(0x80+0x1d)
#define KEY_RALT	(0x80+0x38)

#define KEY_LALT	KEY_RALT

// new keys:

#define KEY_CAPSLOCK    (0x80+0x3a)
#define KEY_NUMLOCK     (0x80+0x45)
#define KEY_SCRLCK      (0x80+0x46)
#define KEY_PRTSCR      (0x80+0x59)

#define KEY_HOME        (0x80+0x47)
#define KEY_END         (0x80+0x4f)
#define KEY_PGUP        (0x80+0x49)
#define KEY_PGDN        (0x80+0x51)
#define KEY_INS         (0x80+0x52)
#define KEY_DEL         (0x80+0x53)

#define KEYP_0          0
#define KEYP_1          KEY_END
#define KEYP_2          KEY_DOWNARROW
#define KEYP_3          KEY_PGDN
#define KEYP_4          KEY_LEFTARROW
#define KEYP_5          '5'
#define KEYP_6          KEY_RIGHTARROW
#define KEYP_7          KEY_HOME
#define KEYP_8          KEY_UPARROW
#define KEYP_9          KEY_PGUP

#define KEYP_DIVIDE     '/'
#define KEYP_PLUS       '+'
#define KEYP_MINUS      '-'
#define KEYP_MULTIPLY   '*'
#define KEYP_PERIOD     0
#define KEYP_EQUALS     KEY_EQUALS
#define KEYP_ENTER      KEY_ENTER


// Lookup table for mapping AT keycodes to their doom keycode
static const char at_to_doom[] =
{
    /* 0x00 */ 0x00,
    /* 0x01 */ KEY_ESCAPE,
    /* 0x02 */ '1',
    /* 0x03 */ '2',
    /* 0x04 */ '3',
    /* 0x05 */ '4',
    /* 0x06 */ '5',
    /* 0x07 */ '6',
    /* 0x08 */ '7',
    /* 0x09 */ '8',
    /* 0x0a */ '9',
    /* 0x0b */ '0',
    /* 0x0c */ '-',
    /* 0x0d */ '=',
    /* 0x0e */ KEY_BACKSPACE,
    /* 0x0f */ KEY_TAB,
    /* 0x10 */ 'q',
    /* 0x11 */ 'w',
    /* 0x12 */ 'e',
    /* 0x13 */ 'r',
    /* 0x14 */ 't',
    /* 0x15 */ 'y',
    /* 0x16 */ 'u',
    /* 0x17 */ 'i',
    /* 0x18 */ 'o',
    /* 0x19 */ 'p',
    /* 0x1a */ '[',
    /* 0x1b */ ']',
    /* 0x1c */ KEY_ENTER,
    /* 0x1d */ KEY_FIRE, /* KEY_RCTRL, */
    /* 0x1e */ 'a',
    /* 0x1f */ 's',
    /* 0x20 */ 'd',
    /* 0x21 */ 'f',
    /* 0x22 */ 'g',
    /* 0x23 */ 'h',
    /* 0x24 */ 'j',
    /* 0x25 */ 'k',
    /* 0x26 */ 'l',
    /* 0x27 */ ';',
    /* 0x28 */ '\'',
    /* 0x29 */ '`',
    /* 0x2a */ KEY_RSHIFT,
    /* 0x2b */ '\\',
    /* 0x2c */ 'z',
    /* 0x2d */ 'x',
    /* 0x2e */ 'c',
    /* 0x2f */ 'v',
    /* 0x30 */ 'b',
    /* 0x31 */ 'n',
    /* 0x32 */ 'm',
    /* 0x33 */ ',',
    /* 0x34 */ '.',
    /* 0x35 */ '/',
    /* 0x36 */ KEY_RSHIFT,
    /* 0x37 */ KEYP_MULTIPLY,
    /* 0x38 */ KEY_LALT,
    /* 0x39 */ KEY_USE,
    /* 0x3a */ KEY_CAPSLOCK,
    /* 0x3b */ KEY_F1,
    /* 0x3c */ KEY_F2,
    /* 0x3d */ KEY_F3,
    /* 0x3e */ KEY_F4,
    /* 0x3f */ KEY_F5,
    /* 0x40 */ KEY_F6,
    /* 0x41 */ KEY_F7,
    /* 0x42 */ KEY_F8,
    /* 0x43 */ KEY_F9,
    /* 0x44 */ KEY_F10,
    /* 0x45 */ KEY_NUMLOCK,
    /* 0x46 */ 0x0,
    /* 0x47 */ 0x0, /* 47 (Keypad-7/Home) */
    /* 0x48 */ 0x0, /* 48 (Keypad-8/Up) */
    /* 0x49 */ 0x0, /* 49 (Keypad-9/PgUp) */
    /* 0x4a */ 0x0, /* 4a (Keypad--) */
    /* 0x4b */ 0x0, /* 4b (Keypad-4/Left) */
    /* 0x4c */ 0x0, /* 4c (Keypad-5) */
    /* 0x4d */ 0x0, /* 4d (Keypad-6/Right) */
    /* 0x4e */ 0x0, /* 4e (Keypad-+) */
    /* 0x4f */ 0x0, /* 4f (Keypad-1/End) */
    /* 0x50 */ 0x0, /* 50 (Keypad-2/Down) */
    /* 0x51 */ 0x0, /* 51 (Keypad-3/PgDn) */
    /* 0x52 */ 0x0, /* 52 (Keypad-0/Ins) */
    /* 0x53 */ 0x0, /* 53 (Keypad-./Del) */
    /* 0x54 */ 0x0, /* 54 (Alt-SysRq) on a 84+ key keyboard */
    /* 0x55 */ 0x0,
    /* 0x56 */ 0x0,
    /* 0x57 */ 0x0,
    /* 0x58 */ 0x0,
    /* 0x59 */ 0x0,
    /* 0x5a */ 0x0,
    /* 0x5b */ 0x0,
    /* 0x5c */ 0x0,
    /* 0x5d */ 0x0,
    /* 0x5e */ 0x0,
    /* 0x5f */ 0x0,
    /* 0x60 */ 0x0,
    /* 0x61 */ 0x0,
    /* 0x62 */ 0x0,
    /* 0x63 */ 0x0,
    /* 0x64 */ 0x0,
    /* 0x65 */ 0x0,
    /* 0x66 */ 0x0,
    /* 0x67 */ KEY_UPARROW,
    /* 0x68 */ 0x0,
    /* 0x69 */ KEY_LEFTARROW,
    /* 0x6a */ KEY_RIGHTARROW,
    /* 0x6b */ 0x0,
    /* 0x6c */ KEY_DOWNARROW,
    /* 0x6d */ 0x0,
    /* 0x6e */ 0x0,
    /* 0x6f */ 0x0,
    /* 0x70 */ 0x0,
    /* 0x71 */ 0x0,
    /* 0x72 */ 0x0,
    /* 0x73 */ 0x0,
    /* 0x74 */ 0x0,
    /* 0x75 */ 0x0,
    /* 0x76 */ 0x0,
    /* 0x77 */ 0x0,
    /* 0x78 */ 0x0,
    /* 0x79 */ 0x0,
    /* 0x7a */ 0x0,
    /* 0x7b */ 0x0,
    /* 0x7c */ 0x0,
    /* 0x7d */ 0x0,
    /* 0x7e */ 0x0,
    /* 0x7f */ KEY_FIRE, //KEY_RCTRL,
};

#endif          // __DOOMKEYS__

