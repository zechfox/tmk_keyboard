/* 
 * Cherry G84-4125 Layout
 */
#include "keymap_common.h"


#ifdef KEYMAP_SECTION_ENABLE
const uint8_t keymaps[][MATRIX_ROWS][MATRIX_COLS] __attribute__ ((section (".keymap.keymaps"))) = {
#else
const uint8_t keymaps[][MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
#endif
    /* Layer 0: Default Layer
     * ,-------------------------------------------------------------------------.
     * |Esc|  F1|  F2|  F3|  F4|  F5|  F6|  F7|  F8|  F9| F10|NLCK|PSCR|SLCK|PAUS|
     * |-------------------------------------------------------------------------|
     * |    1|   2|   3|   4|   5|   6|   7|   8|   9|   O|   -|   =|  BSPC |HOME|
     * |-------------------------------------------------------------------------|
     * |Tab|   Q|   W|   E|   R|   T|   Y|   U|   I|   O|   P|   [|   ]|   \|PGUP|
     * |-------------------------------------------------------------------------|
     * |CAPS|   A|   S|   D|   F|   G|   H|   J|   K|   L|   ;|   '|   Enter|PGDN|
     * |-------------------------------------------------------------------------|
     * |  LSFT|   Z|   X|   C|   V|   B|   N|   M|   ,|   .|   /|  RSFT|  UP| END|
     * |-------------------------------------------------------------------------|
     * |LCTL| FN0|LALT|  `|          SPC           |RALT| INS| DEL|LEFT|DOWN|RGHT|
     */
    KEYMAP(ESC, F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,  NLCK,PSCR, SLCK,PAUS,   \
           1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,  EQL, BSPC,HOME,   \
           TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS, PGUP,      \
           CAPS,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,ENT,PGDN,             \
           LSFT,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,RSFT,UP, END,            \
           LCTL,FN0,LALT,GRV,          SPC,                RALT,INS,DEL,LEFT,DOWN,RGHT),
    /* Layer 1: FunctionKey Layer
     * ,-------------------------------------------------------------------------.
     * |Esc| F11| F12|  F3|  F4|  F5|  F6|  F7|  F8|  F9| F10|NLCK|PSCR|SLCK|PAUS|
     * |-------------------------------------------------------------------------|
     * |     |    |    |    |    |    |   7|   8|   9|   *|    |    |       |    |
     * |-------------------------------------------------------------------------|
     * |   |    |    |    |    |    |    |   4|   5|   6|   -|    |    |    |WH_U|
     * |-------------------------------------------------------------------------|
     * |    |    |    |    |    |    |    |   1|   2|   3|   +|   '|   Enter|WH_D|
     * |-------------------------------------------------------------------------|
     * |      |    |    |    |    |    |    |   0|    |   .|   /|  RSFT|MS_U| END|
     * |-------------------------------------------------------------------------|
     * |LCTL| FN0|LALT|   |         SPC           |    |    |    |MS_L|MS_D|MS_R|
     */

    KEYMAP(ESC, F11,   F12,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,  NLCK,PSCR, SLCK,PAUS,   \
           1,   2,   3,   4,   5,   6, KP_7, KP_8, KP_9, KP_ASTERISK,   MINS,  EQL, BSPC,HOME,   \
           TAB, Q,   W,   E,   R,   T,   Y, KP_4, KP_5, KP_6, KP_MINUS,   LBRC,RBRC,BSLS, WH_U,      \
           CAPS,A,   S,   D,   F,   G,   H,   KP_1, KP_2, KP_3, KP_PLUS,QUOT,KP_ENTER,WH_D,             \
           LSFT,Z,   X,   C,   V,   B,   N,   KP_0,   COMM,KP_DOT, KP_SLASH,RSFT,MS_U, END,            \
           LCTL,FN0,LALT, GRV,          SPC,                RALT,INS,DEL,MS_L,MS_D,MS_R),
};



/*
 * Fn action definition
 */
#ifdef KEYMAP_SECTION_ENABLE
const uint16_t fn_actions[] __attribute__ ((section (".keymap.fn_actions"))) = {
#else
const uint16_t fn_actions[] PROGMEM = {
#endif
    [0] = ACTION_LAYER_MOMENTARY(1),
};
