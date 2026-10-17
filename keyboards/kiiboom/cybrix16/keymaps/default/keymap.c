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

#ifndef RGB_TOG
#    define RGB_TOG RM_TOGG
#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_LYR_TOG,     KC_7,       KC_8,    KC_STAT_DIM,
        KC_WWW_HOME,    KC_4,       KC_5,    KC_6,
        LCTL(KC_X),     KC_1,       KC_2,    KC_3,
        KC_AUD_SPK,     KC_AUD_HDS, KC_0,    KC_DOT,
        _______,        RGB_TOG,    KC_MPLY, KC_MUTE
    ),
    [1] = LAYOUT(
        KC_LYR_TOG, KC_SB_01, KC_SB_02, KC_SB_03,
        KC_SB_04,   KC_SB_05, KC_SB_06, KC_SB_07,
        KC_SB_08,   KC_SB_09, KC_SB_10, KC_SB_11,
        KC_SB_12,   KC_SB_13, KC_SB_14, KC_SB_STOP,
        _______,    RGB_TOG,  KC_SB_STOP, KC_MUTE
    )
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_MEDIA_PREV_TRACK, KC_MEDIA_NEXT_TRACK), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(KC_LYR_PREV, KC_LYR_NEXT), ENCODER_CCW_CW(KC_SB_VOLD, KC_SB_VOLU),                   ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
#endif

