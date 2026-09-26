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

void keyboard_post_init_kb(void) {
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk);
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
    gpio_set_pin_output(B8);
    gpio_write_pin_low(B8);
    gpio_set_pin_output(B6);
    gpio_write_pin_low(B6);
    gpio_set_pin_output(B7);
    gpio_write_pin_low(B7);

    gpio_set_pin_output(B12);
    gpio_write_pin_low(B12);
    gpio_set_pin_output(B13);
    gpio_write_pin_low(B13);
    gpio_set_pin_output(B14);
    gpio_write_pin_high(B14);

    keyboard_post_init_user();
}
