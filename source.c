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
#define LAYER_AUTO_OFF_TIMEOUT	10000
#endif /* LAYER_AUTO_OFF_TIMEOUT */

#ifndef LAYER_AUTO_OFF_LAYER
#define LAYER_AUTO_OFF_LAYER	2	/* lowest layer number to manage */
#endif /* LAYER_AUTO_OFF_LAYER */

#if LAYER_AUTO_OFF_TIMEOUT > 0

static uint32_t layer_auto_off_action_timer = 0;
static uint16_t layer_auto_off_nkeypressed = 0;

bool
oled_task_user(void)
{
	uint8_t top;
	bool invert;

	if (!is_keyboard_master()) return true;
	top = get_highest_layer(layer_state);
	invert = !(top < LAYER_AUTO_OFF_LAYER);
	oled_invert(invert);
	return true;
}

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

#endif /* LAYER_AUTO_OFF_TIMEOUT */

/*
 * Multi Tap or Hold Layer Selection
 * ===========================
 *
 * kc is pressed then released after the T1:
 *
 * 0-------------->T1 (waiting for release)
 * +---------------+-----------------
 * |== kc =========|============
 * |               |== MO(L1) == momentary layer L1
 * +---------------+-----------------
 *
 * kc is pressed then released with in the T1,
 * then no kc actions with in the T2:
 *
 * 0-------------->T1 (waiting for release)
 * |        0------|------->T2 (waiting for press again)
 * +--------+------+--------+--------
 * |== kc ==|      |        |
 * |        |      |        |== TG(L1) == toggle layer L1
 * +--------+------+--------+--------
 *
 * kc is pressed then released with in the T1,
 * then kc is pressed again with in the T2 then released after the T3:
 *
 * 0-------------->T1 (waiting for release)
 * |        0------|------->T2 (waiting for press again)
 * |        |      |   0----|--------->T3 (waiting for release)
 * +--------+------+---+----+----------+-----------------
 * |== kc ==|      |   |    |          |
 * |        |      |   |== kc =========|============
 * |        |      |   |    |          |== MO(L2) == momentary layer L2
 * +--------+------+---+----+----------+-----------------
 *
 * kc is pressed then released with in the T1,
 * then kc is pressed again with in the T2 then released with in the T3:
 *
 * 0-------------->T1 (waiting for release)
 * |        0------|------->T2 (waiting for press again)
 * |        |      |   0----|--------->T3 (waiting for release)
 * +--------+------+---+----+---+------+-----------------
 * |== kc ==|      |   |    |   |      |
 * |        |      |   |== kc ==|      |
 * |        |      |   |    |   |== TG(L2) == toggle layer L2
 * +--------+------+---+----+---+------+-----------------
 */

#define MTHL_TIMER	190

#define MTHLDEF(pkc, n1, n2)						\
	{								\
		.kc = (pkc),						\
		.layer1 = (n1),						\
		.layer2 = (n2),						\
		.pending = false,					\
		.state = WAITING_PRESS,					\
	}

enum multi_tap_or_hold_layer_state {
	WAITING_PRESS,
	WAITING_RELEASE_OR_T1,
	WAITING_PRESS_OR_T2,
	WAITING_RELEASE_OR_T3,
	WAITING_RELEASE_FOR_L1,
	WAITING_RELEASE_FOR_L2,
};

static struct multi_tap_or_hold_layer_def {
	uint16_t kc;			/* keycode to sense */
	uint8_t layer1;			/* first prefered layer number */
	uint8_t layer2;			/* second prefered layer number */
	uint16_t timer;			/* timer for measuring interval */
	bool pending;			/* timer pending action exists */
	enum multi_tap_or_hold_layer_state state;
} multi_tap_or_hold_layer[] = {
	MTHLDEF(KC_HELP, 2, 3),
};

#define NMTHLDEFS (sizeof(multi_tap_or_hold_layer) / sizeof(struct multi_tap_or_hold_layer_def))

static void
mthl_timer_action(struct multi_tap_or_hold_layer_def *p)
{
	if (p->state == WAITING_RELEASE_OR_T1) {
		/* momentary layer 1 */
		layer_on(p->layer1);
		p->state = WAITING_RELEASE_FOR_L1;
	} else if (p->state == WAITING_PRESS_OR_T2) {
		/* toggle layer 1 */
		layer_on(p->layer1);
	} else if (p->state == WAITING_RELEASE_OR_T3) {
		/* momentary layer 2 */
		layer_on(p->layer2);
		p->state = WAITING_RELEASE_FOR_L2;
	}
	p->pending = false;
}

static void
mthl_process_record(struct multi_tap_or_hold_layer_def *p, keyrecord_t *record)
{
	p->pending = false;
	if (record->event.pressed) {
		if (p->state == WAITING_PRESS) {
			p->pending = true;
			p->timer = timer_read();
			p->state = WAITING_RELEASE_OR_T1;
		} else if (p->state == WAITING_PRESS_OR_T2) {
			p->pending = true;
			p->timer = timer_read();
			p->state = WAITING_RELEASE_OR_T3;
		}
	} else {
		if (p->state == WAITING_RELEASE_OR_T1) {
			p->pending = true;
			p->timer = timer_read();
			p->state = WAITING_PRESS_OR_T2;
		} else if (p->state == WAITING_RELEASE_OR_T3) {
			/* toggle layer 2 */
			layer_on(p->layer2);
			p->state = WAITING_PRESS;
		} else if (p->state == WAITING_RELEASE_FOR_L1) {
			layer_off(p->layer1);
			p->state = WAITING_PRESS;
		} else if (p->state == WAITING_RELEASE_FOR_L2) {
			layer_off(p->layer2);
			p->state = WAITING_PRESS;
		}
	}
}

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
	int i;
	struct multi_tap_or_hold_layer_def *p;

#if LAYER_AUTO_OFF_TIMEOUT > 0
	layer_auto_off_check();
#endif /* LAYER_AUTO_OFF_TIMEOUT */

	for (i = 0; i < NMTHLDEFS; i++) {
		p = &multi_tap_or_hold_layer[i];
		if (!p->pending) continue;
		if (timer_elapsed(p->timer) < MTHL_TIMER) continue;
		mthl_timer_action(p);
	}
}

bool
process_record_user(uint16_t keycode, keyrecord_t *record)
{
	int i;
	struct multi_tap_or_hold_layer_def *p;

#ifdef CONSOLE_ENABLE
	if (debug_enable) {
		uprintf("%05u %04X %02u %02u %u\n",
			record->event.time, keycode,
			record->event.key.col, record->event.key.row,
			record->event.pressed);
	}
#endif /* CONSOLE_ENABLE */

#if LAYER_AUTO_OFF_TIMEOUT > 0
	layer_auto_off_record(record);
#endif /* LAYER_AUTO_OFF_TIMEOUT */

	for (i = 0; i < NMTHLDEFS; i++) {
		p = &multi_tap_or_hold_layer[i];
		if (keycode != p->kc) continue;
		mthl_process_record(p, record);
		return false;
	}
	return true;
}
