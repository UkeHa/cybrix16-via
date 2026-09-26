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

    // Simple numpad-style layout as a starting point.
    // The 3 knobs default to page up/down, prev/next track and volume
    // (see the encoder map further down in this file).
    [0] = LAYOUT(
        KC_MY_COMPUTER, KC_7,       KC_8, KC_9,
        KC_WWW_HOME,    KC_4,       KC_5, KC_6,
        LCTL(KC_X),     KC_1,       KC_2, KC_3,
        LCTL(KC_C),     LCTL(KC_V), KC_0, KC_DOT,
        _______,        RGB_TOG,    KC_MPLY, KC_NO
    )
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_PAGE_UP, KC_PAGE_DOWN), ENCODER_CCW_CW(KC_MEDIA_PREV_TRACK, KC_MEDIA_NEXT_TRACK), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
#endif
