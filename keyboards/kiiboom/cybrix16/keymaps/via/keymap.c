/*
Copyright 2021 owlab
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#define ENCODER_DIRECTION_FLIP

#ifndef RGB_TOG
#    define RGB_TOG RM_TOGG
#    define RGB_MOD RM_NEXT
#    define RGB_RMOD RM_PREV
#    define RGB_HUI RM_HUEU
#    define RGB_HUD RM_HUED
#    define RGB_SAI RM_SATU
#    define RGB_SAD RM_SATD
#    define RGB_VAI RM_VALU
#    define RGB_VAD RM_VALD
#    define RGB_SPI RM_SPDU
#    define RGB_SPD RM_SPDD
#    define RGB_MODE_PLAIN RM_NEXT
#    define RGB_MODE_BREATHE RM_NEXT
#    define RGB_MODE_RAINBOW RM_NEXT
#endif

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_MEDIA_PREV_TRACK, KC_MEDIA_NEXT_TRACK), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_SB_VOLD, KC_SB_VOLU),                   ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [2] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_NO, KC_NO),                             ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [3] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_NO, KC_NO),                             ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [4] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_NO, KC_NO),                             ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [5] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(RGB_VAD, RGB_VAI),                         ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // Ebene 0: Standard + Audio-Umschaltung + Status-LED-Dimmer
    // [0,0] = Wechsel Ebene 0 <-> 1 | [0,3] = Status-LED Dimmung durchschalten
    // [3,0] = Lautsprecher          | [3,1] = Headset
    [0] = LAYOUT(
        KC_LYR_TOG,     KC_7,       KC_8,    KC_STAT_DIM,
        KC_WWW_HOME,    KC_4,       KC_5,    KC_6,
        LCTL(KC_X),     KC_1,       KC_2,    KC_3,
        KC_AUD_SPK,     KC_AUD_HDS, KC_0,    KC_DOT,
        _______,        RGB_TOG,    KC_MPLY, KC_MUTE
    ),

    // Ebene 1: Soundboard (14 Sound-Slots + Stopp-Taste unten rechts)
    // [0,0] = Zurück zu Ebene 0
    [1] = LAYOUT(
        KC_LYR_TOG, KC_SB_01, KC_SB_02, KC_SB_03,
        KC_SB_04,   KC_SB_05, KC_SB_06, KC_SB_07,
        KC_SB_08,   KC_SB_09, KC_SB_10, KC_SB_11,
        KC_SB_12,   KC_SB_13, KC_SB_14, KC_SB_STOP,
        _______,    RGB_TOG,  KC_SB_STOP, KC_MUTE
    ),

    [2] = LAYOUT(
        KC_LYR_TOG, KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, TO(3)
    ),

    [3] = LAYOUT(
        KC_LYR_TOG, KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, TO(4)
    ),

    [4] = LAYOUT(
        KC_LYR_TOG, KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, KC_NO,
        KC_NO,      KC_NO, KC_NO, TO(5)
    ),

    [5] = LAYOUT(
        RGB_TOG,          RGB_MOD,           RGB_RMOD, RGB_HUD,
        RGB_HUI,          RGB_SAD,           RGB_SAI,  RGB_VAD,
        RGB_VAI,          RGB_SPD,           RGB_SPI,  RGB_MODE_PLAIN,
        RGB_MODE_BREATHE, RGB_MODE_RAINBOW,  KC_NO,    KC_STAT_DIM,
        KC_NO,            KC_NO,             KC_NO,    TO(0)
    )
};

