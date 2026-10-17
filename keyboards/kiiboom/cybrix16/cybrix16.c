/* Copyright 2022 Jacky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "quantum.h"
#include <hal.h>
#ifdef RAW_ENABLE
#    include "raw_hid.h"
#endif

#define STATUS_LED_INDEX 16
#define STAT_PWM_MAX     1000
#define STAT_DIM_LEVELS  6

// Helligkeitsstufen (aus 1000 bei 1 kHz Hardware-PWM auf TIM4 / Pins B8, B6, B7):
// Stufe 0 (Standard): 50% (500) | Stufe 1: 25% (250) | Stufe 2: 10% (100)
// Stufe 3: 3% (30) | Stufe 4: 0% (Komplett Aus) | Stufe 5: 100% (1000)
static const uint16_t stat_duty_table[STAT_DIM_LEVELS] = { 500, 250, 100, 30, 0, 1000 };
static const uint8_t  stat_val_table[STAT_DIM_LEVELS]  = { 128, 64, 26, 8, 0, 255 };
static uint8_t        stat_dim_step                    = 0;
static bool           keys_backlight_enabled           = false;

// Standard-Zuordnung der aktiven Low-Kathoden der Lightbar (Status-LED):
// Kanal 0 = PB6 (TIM4_CH1), Kanal 1 = PB7 (TIM4_CH2), Kanal 2 = PB8 (TIM4_CH3)
// In KiiBOOMs Original-Init: B8 (R), B6 (G), B7 (B)
static uint8_t pin_map_r = 2; // PB8 (TIM4_CH3)
static uint8_t pin_map_g = 0; // PB6 (TIM4_CH1)
static uint8_t pin_map_b = 1; // PB7 (TIM4_CH2)

static void send_host_action(uint8_t action_id) {
#ifdef RAW_ENABLE
    uint8_t pkt[32] = {0};
    pkt[0]          = 0xFE;
    pkt[1]          = 0xC1;
    pkt[2]          = action_id;
    raw_hid_send(pkt, sizeof(pkt));
#else
    (void)action_id;
#endif
}

static void get_layer_rgb_mask(uint8_t layer, bool *r_on, bool *g_on, bool *b_on) {
    switch (layer) {
        case 0: // Ebene 0: Weiß (R + G + B)
            *r_on = true;
            *g_on = true;
            *b_on = true;
            break;
        case 1: // Ebene 1: Blau (B)
            *r_on = false;
            *g_on = false;
            *b_on = true;
            break;
        case 2: // Ebene 2: Grün (G)
            *r_on = false;
            *g_on = true;
            *b_on = false;
            break;
        case 3: // Ebene 3: Gelb (R + G)
            *r_on = true;
            *g_on = true;
            *b_on = false;
            break;
        case 4: // Ebene 4: Pink/Magenta (R + B)
            *r_on = true;
            *g_on = false;
            *b_on = true;
            break;
        default: // Ebene 5: Cyan/Hellblau (G + B)
            *r_on = false;
            *g_on = true;
            *b_on = true;
            break;
    }
}

static void set_tim4_channel_duty(uint8_t ch, uint16_t duty) {
    switch (ch) {
        case 0:
            TIM4->CCR1 = duty; // PB6 (TIM4_CH1)
            break;
        case 1:
            TIM4->CCR2 = duty; // PB7 (TIM4_CH2)
            break;
        case 2:
            TIM4->CCR3 = duty; // PB8 (TIM4_CH3)
            break;
    }
}

static void status_led_update(uint8_t layer) {
    uint16_t duty = stat_duty_table[stat_dim_step];
    bool     r_on = false, g_on = false, b_on = false;
    get_layer_rgb_mask(layer, &r_on, &g_on, &b_on);

    set_tim4_channel_duty(pin_map_r, r_on ? duty : 0);
    set_tim4_channel_duty(pin_map_g, g_on ? duty : 0);
    set_tim4_channel_duty(pin_map_b, b_on ? duty : 0);
}

static void status_led_pwm_init(void) {
    // Hardware-PWM über TIM4 (CH1=PB6, CH2=PB7, CH3=PB8) bei 1 kHz, Active-Low
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->MAPR &= ~AFIO_MAPR_TIM4_REMAP;

    TIM4->CR1   = 0;
    TIM4->PSC   = 71;               // 72 MHz / 72 = 1 MHz (1 us pro Tick)
    TIM4->ARR   = STAT_PWM_MAX - 1; // 1000 us = 1 kHz PWM
    TIM4->CCR1  = 0;
    TIM4->CCR2  = 0;
    TIM4->CCR3  = 0;

    // PWM Mode 1 (6) mit Preload auf CH1, CH2, CH3
    TIM4->CCMR1 = (6U << 4) | TIM_CCMR1_OC1PE | (6U << 12) | TIM_CCMR1_OC2PE;
    TIM4->CCMR2 = (6U << 4) | TIM_CCMR2_OC3PE;

    // Active-Low Polarität (CCxP = 1): CNT < CCRx -> Pin LOW (LED AN), CNT >= CCRx -> Pin HIGH (LED AUS)
    TIM4->CCER  = TIM_CCER_CC1E | TIM_CCER_CC1P |
                  TIM_CCER_CC2E | TIM_CCER_CC2P |
                  TIM_CCER_CC3E | TIM_CCER_CC3P;

    TIM4->EGR   = TIM_EGR_UG;
    TIM4->CR1   = TIM_CR1_CEN | TIM_CR1_ARPE;

    palSetLineMode(B6, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
    palSetLineMode(B7, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
    palSetLineMode(B8, PAL_MODE_STM32_ALTERNATE_PUSHPULL);

    status_led_update(get_highest_layer(layer_state | default_layer_state));
}

void keyboard_post_init_kb(void) {
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk);
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;

    gpio_set_pin_output(B12);
    gpio_write_pin_low(B12);
    gpio_set_pin_output(B13);
    gpio_write_pin_low(B13);
    gpio_set_pin_output(B14);
    gpio_write_pin_high(B14);

    stat_dim_step          = 0;     // Standard beim Start: 50% gedimmt
    keys_backlight_enabled = false; // Tasten-Backlight beim Start standardmäßig AUS

    status_led_pwm_init();

#ifdef RGB_MATRIX_ENABLE
    if (!rgb_matrix_is_enabled()) {
        rgb_matrix_enable_noeeprom();
    }
#endif

    keyboard_post_init_user();
}

layer_state_t layer_state_set_kb(layer_state_t state) {
    state = layer_state_set_user(state);
    status_led_update(get_highest_layer(state | default_layer_state));
    return state;
}

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_kb(void) {
    if (!rgb_matrix_indicators_user()) {
        return false;
    }

    uint8_t current_layer = get_highest_layer(layer_state | default_layer_state);
    status_led_update(current_layer);

    // Tasten-Beleuchtung (LEDs 0..15) ausschalten, solange keys_backlight_enabled == false (Standard)
    if (!keys_backlight_enabled) {
        for (uint8_t i = 0; i < STATUS_LED_INDEX; i++) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }

    // Falls Index 16 bestückt ist, ebenfalls entsprechend Ebene & Dimmung setzen
    uint8_t stat_val = stat_val_table[stat_dim_step];
    bool    r_on = false, g_on = false, b_on = false;
    get_layer_rgb_mask(current_layer, &r_on, &g_on, &b_on);
    rgb_matrix_set_color(STATUS_LED_INDEX, r_on ? stat_val : 0, g_on ? stat_val : 0, b_on ? stat_val : 0);

    return true;
}
#endif

#ifdef VIA_ENABLE
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (length >= 5 && data[0] == 0xFE && data[1] == 0xC2) {
        // Live-Konfiguration der RGB-Kanalzuordnung über Raw HID (falls benötigt)
        pin_map_r = data[2] % 3;
        pin_map_g = data[3] % 3;
        pin_map_b = data[4] % 3;
        status_led_update(get_highest_layer(layer_state | default_layer_state));
        raw_hid_send(data, length);
        return true;
    }
    return false;
}
#endif

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    if (record->event.pressed) {
        switch (keycode) {
#ifdef RGB_MATRIX_ENABLE
            case QK_RGB_MATRIX_TOGGLE: {
                // Schaltet die 16 Tasten-LEDs an/aus, lässt die Status-LED aktiv
                keys_backlight_enabled = !keys_backlight_enabled;
                if (!rgb_matrix_is_enabled()) {
                    rgb_matrix_enable_noeeprom();
                }
                return false;
            }
#endif
            case KC_LYR_NEXT: {
                uint8_t current = get_highest_layer(layer_state | default_layer_state);
                uint8_t next    = (current + 1) % DYNAMIC_KEYMAP_LAYER_COUNT;
                layer_move(next);
                status_led_update(next);
                return false;
            }
            case KC_LYR_PREV: {
                uint8_t current = get_highest_layer(layer_state | default_layer_state);
                uint8_t prev    = (current + DYNAMIC_KEYMAP_LAYER_COUNT - 1) % DYNAMIC_KEYMAP_LAYER_COUNT;
                layer_move(prev);
                status_led_update(prev);
                return false;
            }
            case KC_STAT_DIM: {
                stat_dim_step = (stat_dim_step + 1) % STAT_DIM_LEVELS;
                status_led_update(get_highest_layer(layer_state | default_layer_state));
                return false;
            }
            case KC_LYR_TOG: {
                uint8_t current = get_highest_layer(layer_state | default_layer_state);
                uint8_t target  = (current == 0) ? 1 : 0;
                layer_move(target);
                status_led_update(target);
                return false;
            }
            case KC_AUD_SPK:
                send_host_action(1);
                return false;
            case KC_AUD_HDS:
                send_host_action(2);
                return false;
            case KC_SB_01 ... KC_SB_14:
                send_host_action(11 + (keycode - KC_SB_01));
                return false;
            case KC_SB_STOP:
                send_host_action(30);
                return false;
            case KC_SB_VOLD:
                send_host_action(31);
                return false;
            case KC_SB_VOLU:
                send_host_action(32);
                return false;
        }
    }
    return true;
}
