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
	uint8_t top;
	bool invert;

	if (!is_keyboard_master()) return true;
	top = get_highest_layer(layer_state);
	invert = !(top < LAYER_AUTO_OFF_LAYER);
	oled_invert(invert);
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
