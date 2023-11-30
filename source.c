/*
 * Corne keyboard customizations
 * Copyright (C) 2023 kitcnya@outlook.com
 * https://opensource.org/license/mit/
 * SPDX-License-Identifier: MIT
 */

#include QMK_KEYBOARD_H
#ifdef CONSOLE_ENABLE
#include "print.h"
#endif

#ifndef LAYER_AUTO_OFF_TIMEOUT
#define LAYER_AUTO_OFF_TIMEOUT	10000	/* XXX: value for test */
#endif /* LAYER_AUTO_OFF_TIMEOUT */

#ifndef LAYER_AUTO_OFF_LAYER
#define LAYER_AUTO_OFF_LAYER	2	/* lowest layer number to manage */
#endif /* LAYER_AUTO_OFF_LAYER */

#if LAYER_AUTO_OFF_TIMEOUT > 0

static uint32_t layer_auto_off_action_timer = 0;
static uint16_t layer_auto_off_nkeypressed = 0;

static void
layer_auto_off_check(void)
{
	uint8_t top;

	if (timer_elapsed32(layer_auto_off_action_timer) < LAYER_AUTO_OFF_TIMEOUT) return;
	layer_auto_off_action_timer = timer_read32();
	/* timer expired */
	if (layer_auto_off_nkeypressed > 0) {
		layer_auto_off_nkeypressed--;
		return;
	}
	top = get_highest_layer(layer_state);
	if (top < LAYER_AUTO_OFF_LAYER) return;
	layer_off(top);
}

static void
layer_auto_off_record(keyrecord_t *record)
{
	layer_auto_off_action_timer = timer_read32();
	if (record->event.pressed) {
		layer_auto_off_nkeypressed++;
	} else if (layer_auto_off_nkeypressed > 0) {
		layer_auto_off_nkeypressed--;
	}
}

bool
oled_task_user(void)
{
	static const char PROGMEM crkbd_logo[] = {
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4,
		0,
	};
	uint8_t top;

	if (is_keyboard_master()) return true;
	top = get_highest_layer(layer_state);
	if (top < LAYER_AUTO_OFF_LAYER) return true;
	oled_write_P(crkbd_logo, true);
	return false;
}

#endif /* LAYER_AUTO_OFF_TIMEOUT */

/*
 * System Interfaces
 *
 * see:
 * - https://docs.qmk.fm/#/custom_quantum_functions
 * - https://docs.qmk.fm/#/feature_macros
 */

void
housekeeping_task_user(void)
{
#if LAYER_AUTO_OFF_TIMEOUT > 0
	layer_auto_off_check();
#endif /* LAYER_AUTO_OFF_TIMEOUT */
}

bool
process_record_user(uint16_t keycode, keyrecord_t *record)
{
#if LAYER_AUTO_OFF_TIMEOUT > 0
	layer_auto_off_record(record);
#endif /* LAYER_AUTO_OFF_TIMEOUT */
#ifdef CONSOLE_ENABLE
	if (debug_enable) {
		uprintf("%05u %04X %02u %02u %u\n",
			record->event.time, keycode,
			record->event.key.col, record->event.key.row,
			record->event.pressed);
	}
#endif /* CONSOLE_ENABLE */
	return true;
}
