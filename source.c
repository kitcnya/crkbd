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

#ifdef LAO_ENABLE

/*
 * Higher Layer Auto Off Timer
 */

#ifndef LAYER_AUTO_OFF_TIMEOUT
#define LAYER_AUTO_OFF_TIMEOUT	10000
#endif /* LAYER_AUTO_OFF_TIMEOUT */

#ifndef LAYER_AUTO_OFF_LAYER_LO
#define LAYER_AUTO_OFF_LAYER_LO	MAX_LAYER	/* lowest layer number to manage */
#endif /* LAYER_AUTO_OFF_LAYER_LO */
#ifndef LAYER_AUTO_OFF_LAYER_HI
#define LAYER_AUTO_OFF_LAYER_HI	MAX_LAYER	/* highest layer number to manage */
#endif /* LAYER_AUTO_OFF_LAYER_HI */

static uint32_t layer_auto_off_action_timer = 0;
static uint16_t layer_auto_off_nkeypressed = 0;

static void
layer_auto_off_check(void)
{
	uint8_t layer;

	if (timer_elapsed32(layer_auto_off_action_timer) < LAYER_AUTO_OFF_TIMEOUT) return;
	layer_auto_off_action_timer = timer_read32();
	/* timer expired */
	if (layer_auto_off_nkeypressed > 0) {
		layer_auto_off_nkeypressed--;
		return;
	}
	for (layer = LAYER_AUTO_OFF_LAYER_LO; layer <= LAYER_AUTO_OFF_LAYER_HI; layer++) {
		layer_off(layer);
	}
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

void
housekeeping_task_lao(void)
{
	layer_auto_off_check();
}

bool
process_record_lao(uint16_t keycode, keyrecord_t *record)
{
	layer_auto_off_record(record);
	return true;
}

#endif /* LAO_ENABLE */

#ifdef MTH_ENABLE

/*
 * Multi Tap or Hold layer selection
 * =================================
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
 *
 * Key code extension:
 * momentary action (i.e. MO()) can be replaced by normal key press and release action.
 */

#define MTH_TIMER	190

#define MTHDEF(pkc, k1, k2, n1, n2, t1, t2)				\
	{								\
		.kc = (pkc),						\
		.kc1 = (k1),						\
		.kc2 = (k2),						\
		.layer1 = (n1),						\
		.layer2 = (n2),						\
		.toggle1 = (t1),					\
		.toggle2 = (t2),					\
		.pending = false,					\
		.state = WAITING_PRESS,					\
	}
#define NMTHDEFS (sizeof(multi_tap_or_hold) / sizeof(struct multi_tap_or_hold_def))

enum multi_tap_or_hold_state {
	WAITING_PRESS,
	WAITING_RELEASE_OR_T1,
	WAITING_PRESS_OR_T2,
	WAITING_RELEASE_OR_T3,
	WAITING_RELEASE_FOR_L1,
	WAITING_RELEASE_FOR_L2,
};

struct multi_tap_or_hold_def {
	uint16_t kc;			/* keycode to sense */
	uint8_t kc1;			/* first preferred keycode (instead of changing layer) */
	uint8_t kc2;			/* second preferred keycode (instead of changing layer) */
	uint8_t layer1;			/* first preferred layer number */
	uint8_t layer2;			/* second preferred layer number */
	uint8_t toggle1;		/* toggle layer by single tapping */
	uint8_t toggle2;		/* toggle layer by double tapping */
	uint16_t timer;			/* timer for measuring interval */
	bool pending;			/* timer pending action exists */
	enum multi_tap_or_hold_state state;
};

static struct multi_tap_or_hold_def multi_tap_or_hold[] = {
	MTHDEF(ML2X2X, KC_NO, KC_NO, 2, 2, 2, 2),
	MTHDEF(MLAA36, KC_LALT, KC_LALT, 0, 0, 3, 6),
};

static void
mth_timer_action(struct multi_tap_or_hold_def *p)
{
	p->pending = false;
	switch (p->state) {
	case WAITING_RELEASE_OR_T1:
		/* first hold action */
		if (p->kc1 == KC_NO) {
			/* momentary layer 1 */
			layer_on(p->layer1);
		} else {
			/* press keycode 1 */
			register_code(p->kc1);
		}
		p->state = WAITING_RELEASE_FOR_L1;
		return;
	case WAITING_PRESS_OR_T2:
		/* toggle layer 1 */
		layer_invert(p->toggle1);
		break;
	case WAITING_RELEASE_OR_T3:
		/* second hold action */
		if (p->kc2 == KC_NO) {
			/* momentary layer 2 */
			layer_on(p->layer2);
		} else {
			/* press keycode 2 */
			register_code(p->kc2);
		}
		p->state = WAITING_RELEASE_FOR_L2;
		return;
	default:
		break;
	}
	p->state = WAITING_PRESS;
}

static void
mth_process_record(struct multi_tap_or_hold_def *p, keyrecord_t *record)
{
	p->pending = false;
	switch (p->state) {
	case WAITING_PRESS:
		if (!record->event.pressed) break;
		p->pending = true;
		p->timer = timer_read();
		p->state = WAITING_RELEASE_OR_T1;
		return;
	case WAITING_RELEASE_OR_T1:
		if (record->event.pressed) break;
		p->pending = true;
		p->timer = timer_read();
		p->state = WAITING_PRESS_OR_T2;
		return;
	case WAITING_PRESS_OR_T2:
		if (!record->event.pressed) break;
		p->pending = true;
		p->timer = timer_read();
		p->state = WAITING_RELEASE_OR_T3;
		return;
	case WAITING_RELEASE_OR_T3:
		if (record->event.pressed) break;
		/* toggle layer 2 */
		layer_invert(p->toggle2);
		break;
	case WAITING_RELEASE_FOR_L1:
		if (p->kc1 == KC_NO) {
			layer_off(p->layer1);
		} else {
			unregister_code(p->kc1);
		}
		break;
	case WAITING_RELEASE_FOR_L2:
		if (p->kc2 == KC_NO) {
			layer_off(p->layer2);
		} else {
			unregister_code(p->kc2);
		}
		break;
	default:
		break;
	}
	p->state = WAITING_PRESS;
}

void
housekeeping_task_mth(void)
{
	int i;
	struct multi_tap_or_hold_def *p;

	for (i = 0; i < NMTHDEFS; i++) {
		p = &multi_tap_or_hold[i];
		if (!p->pending) continue;
		if (timer_elapsed(p->timer) < MTH_TIMER) continue;
		mth_timer_action(p);
	}
}

bool
process_record_mth(uint16_t keycode, keyrecord_t *record)
{
	int i;
	struct multi_tap_or_hold_def *p;

	for (i = 0; i < NMTHDEFS; i++) {
		p = &multi_tap_or_hold[i];
		if (keycode != p->kc) continue;
		mth_process_record(p, record);
		return false;
	}
	return true;
}

#endif /* MTH_ENABLE */

#ifdef TMA_ENABLE

/*
 * Tapping Multi-modifier Assistant
 * ================================
 *
 * 0----------....>T1 (waiting for release)
 * |          0----|----............>T2 (waiting for press)
 * |          |    |    0----------....>T1
 * |          |    |    |          0----------|..........>T2
 * |          |    |    |          |          0-------------->T1
 * +----------+----+----+----------+----------+---------------+---------
 * |== mkc1 ==|    |    |          |          |               |
 * |                    |== mkc2 ==|          |               |
 * |            assist :--- mkc1 ---:         |               |
 * |                                          |== mkc3 =======|=====|
 * |                                 assist :---- mkc1 --------------:
 * |                                  assist :--- mkc2 ---------------:
 */

/***
@startuml
hide empty description

[*] --> WAITING_MODIFIER_PRESS
WAITING_MODIFIER_PRESS_OR_T2 -> WAITING_MODIFIER_PRESS : T2

WAITING_MODIFIER_PRESS --> WAITING_MODIFIER_RELEASE_OR_T1 : press
WAITING_MODIFIER_PRESS_OR_T2 --> WAITING_MODIFIER_RELEASE_OR_T1 : press (assist)

WAITING_MODIFIER_RELEASE_OR_T1 -> WAITING_MODIFIER_RELEASE : T1

WAITING_MODIFIER_RELEASE --> CLEAN_ASSIST_MODIFIRES  : release
WAITING_MODIFIER_RELEASE_OR_T1 --> CLEAN_ASSIST_MODIFIERS_FOR_NEXT_PRESS : release

WAITING_MODIFIER_PRESS_OR_T2 <-- CLEAN_ASSIST_MODIFIERS_FOR_NEXT_PRESS : (assist release)
WAITING_MODIFIER_PRESS <-- CLEAN_ASSIST_MODIFIRES : (assist release)

@enduml
 ***/

#define TMA_TIMER1	120
#define TMA_TIMER2	700

#define TMADEF(mkc, passist)						\
	{								\
		.kc = (mkc),						\
		.press = false,						\
		.assist_press = false,					\
		.assist = (passist),					\
	}

enum tapping_multi_modifier_assitant_state {
	WAITING_MODIFIER_PRESS,
	WAITING_MODIFIER_RELEASE_OR_T1,
	CLEAN_ASSIST_MODIFIERS_FOR_NEXT_PRESS,
	WAITING_MODIFIER_PRESS_OR_T2,
	WAITING_MODIFIER_RELEASE,
	CLEAN_ASSIST_MODIFIRES,
};

static struct tapping_multi_modifier_assistant {
	uint16_t kc;			/* (modifier) key code */
	uint16_t timer;			/* timer for measuring interval */
	enum tapping_multi_modifier_assitant_state state;
	bool alt;			/* assist alt */
	bool ctrl;			/* assist ctrl */
	bool shift;			/* assist shift */
} tma = {
	.state = WAITING_MODIFIER_PRESS,
};

static struct tapping_multi_modifier_assistant_keys {
	uint16_t kc;			/* (modifier) key code */
	bool press;			/* key press detected */
	bool assist_press; 		/* key press emitted */
	bool *assist;			/* use for assist */
} tmakey[] = {
	TMADEF(KC_LALT, &tma.alt),
	TMADEF(KC_RALT, &tma.alt),
	TMADEF(KC_LCTL, &tma.ctrl),
	TMADEF(KC_RCTL, &tma.ctrl),
	TMADEF(KC_LSFT, &tma.shift),
	TMADEF(KC_RSFT, &tma.shift),
};

#define NTMAKEYS (sizeof(tmakey) / sizeof(struct tapping_multi_modifier_assistant_keys))

static void
tma_assist_press(uint16_t kc)
{
	int i;
	struct tapping_multi_modifier_assistant_keys *p;

	for (i = 0; i < NTMAKEYS; i++) {
		p = &tmakey[i];
		if (p->kc == kc) {
			if (p->assist_press) {
				unregister_code(p->kc);
				p->assist_press = false;
			}
			*p->assist = false;
			continue;
		}
		if (!*p->assist) continue;
		if (p->assist_press) continue;
		register_code(p->kc);
		p->assist_press = true;
	}
}

static void
tma_assist_release(bool keep_assist)
{
	int i;
	struct tapping_multi_modifier_assistant_keys *p;

	for (i = 0; i < NTMAKEYS; i++) {
		p = &tmakey[i];
		if (!keep_assist) *p->assist = false;
		if (!p->assist_press) continue;
		unregister_code(p->kc);
		p->assist_press = false;
	}
}

static void
tma_check(void)
{
	switch (tma.state) {
	case WAITING_MODIFIER_RELEASE_OR_T1:
		if (timer_elapsed(tma.timer) < TMA_TIMER1) return;
		tma.state = WAITING_MODIFIER_RELEASE;
		return;
	case CLEAN_ASSIST_MODIFIERS_FOR_NEXT_PRESS:
		tma_assist_release(true);
		tma.state = WAITING_MODIFIER_PRESS_OR_T2;
		return;
	case WAITING_MODIFIER_PRESS_OR_T2:
		if (timer_elapsed(tma.timer) < TMA_TIMER2) return;
		tma_assist_release(false);
		tma.state = WAITING_MODIFIER_PRESS;
		return;
	case CLEAN_ASSIST_MODIFIRES:
		tma_assist_release(false);
		tma.state = WAITING_MODIFIER_PRESS;
		return;
	default:
		break;
	}
}

static void
tma_process_record(uint16_t keycode, keyrecord_t *record)
{
	int i;
	struct tapping_multi_modifier_assistant_keys *p, *k;

	k = 0;
	for (i = 0; i < NTMAKEYS; i++) {
		p = &tmakey[i];
		if (p->kc != keycode) continue;
		p->press = record->event.pressed;
		k = p;
	}
	if (!k) return;

	switch (tma.state) {
	case WAITING_MODIFIER_PRESS:
		if (!record->event.pressed) return;
		tma.kc = keycode;
		tma.timer = timer_read();
		tma.state = WAITING_MODIFIER_RELEASE_OR_T1;
		return;
	case WAITING_MODIFIER_RELEASE_OR_T1:
		if (keycode != tma.kc) {
			tma.state = CLEAN_ASSIST_MODIFIRES;
			return;
		}
		*k->assist = true;
		tma.timer = timer_read();
		tma.state = CLEAN_ASSIST_MODIFIERS_FOR_NEXT_PRESS;
		return;
	case WAITING_MODIFIER_PRESS_OR_T2:
		if (!record->event.pressed) {
			tma.state = CLEAN_ASSIST_MODIFIRES;
			return;
		}
		tma_assist_press(keycode);
		tma.kc = keycode;
		tma.timer = timer_read();
		tma.state = WAITING_MODIFIER_RELEASE_OR_T1;
		return;
	case WAITING_MODIFIER_RELEASE:
		for (i = 0; i < NTMAKEYS; i++) {
			p = &tmakey[i];
			if (p->press) return;
		}
		tma.state = CLEAN_ASSIST_MODIFIRES;
		break;
	default:
		break;
	}
}

void
housekeeping_task_tma(void)
{
	tma_check();
}

bool
process_record_tma(uint16_t keycode, keyrecord_t *record)
{
	tma_process_record(keycode, record);
	return true;
}

#endif /* TMA_ENABLE */

#ifdef KCR_ENABLE

/*
 * Keycode Reporter
 */

bool
process_record_kcr(uint16_t keycode, keyrecord_t *record)
{
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

#endif /* KCR_ENABLE */

/*
 * System Interfaces
 */

void
housekeeping_task_user(void)
{
#if defined(LAO_ENABLE)
	housekeeping_task_lao();
#endif
#if defined(TMA_ENABLE)
	housekeeping_task_tma();
#endif
#if defined(MTH_ENABLE)
	housekeeping_task_mth();
#endif
}

bool
process_record_user(uint16_t keycode, keyrecord_t *record)
{
	if (!(
#if defined(KCR_ENABLE)
		    process_record_kcr(keycode, record) &&
#endif
#if defined(LAO_ENABLE)
		    process_record_lao(keycode, record) &&
#endif
#if defined(TMA_ENABLE)
		    process_record_tma(keycode, record) &&
#endif
#if defined(MTH_ENABLE)
		    process_record_mth(keycode, record) &&
#endif
		    true)) {
		return false;
	}
	return true;
}

/*
 * Customizations
 */

bool
oled_task_user(void)
{
	uint8_t layer;

	if (!is_keyboard_master()) return true;

	oled_write_P(PSTR("L:"), false);
	for (layer = 0; layer < 10; layer++) {
		if (IS_LAYER_ON(layer)) {
#ifdef LAO_ENABLE
			if (layer < LAYER_AUTO_OFF_LAYER_LO || layer > LAYER_AUTO_OFF_LAYER_HI) {
				oled_write_char(layer + '0', false);
			} else {
				oled_write_char(layer + '0', true);
			}
#else
			oled_write_char(layer + '0', false);
#endif /* LAO_ENABLE */
		} else {
			oled_write_char(' ', false);
		}
	}
#ifdef TMA_ENABLE
	oled_advance_char();
	if (tma.alt) {
		oled_write_char('A', false);
	} else {
		oled_write_char(' ', false);
	}
	if (tma.ctrl) {
		oled_write_char('C', false);
	} else {
		oled_write_char(' ', false);
	}
	if (tma.shift) {
		oled_write_char('S', false);
	} else {
		oled_write_char(' ', false);
	}
#endif /* TMA_ENABLE */
	oled_write_ln_P(PSTR(""), false);
	return true;
}
