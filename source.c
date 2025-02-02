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
#ifdef KA_ENABLE
#include "key_action.h"
#endif

#ifdef LAO_ENABLE

/*
 * Layer Auto Off Timer
 */

#ifndef LAYER_AUTO_OFF_TIMEOUT
#define LAYER_AUTO_OFF_TIMEOUT	10000
#endif /* LAYER_AUTO_OFF_TIMEOUT */

struct layer_auto_off_control_st {
	/* settings */
	uint8_t enable:1;		/* managed layer */
	uint8_t immediate:1;		/* period expired immediately or extensible on keying */
	/* status */
	uint8_t previous:1;		/* previous layer state (on/off) */
	uint8_t ongoing:1;		/* timer is on going */
	uint32_t timer;			/* timer count */
};

struct layer_auto_off_control_st layer_auto_off_reg[MAX_LAYER] = {};

static void
layer_auto_off_check(void)
{
	uint8_t layer;

	for (layer = 0; layer < MAX_LAYER; layer++) {
		if (!layer_auto_off_reg[layer].enable) continue;
		if (!IS_LAYER_ON(layer)) {
			if (!layer_auto_off_reg[layer].previous) continue;
			/* layer off by other feature */
			layer_auto_off_reg[layer].previous = 0;
			layer_auto_off_reg[layer].ongoing = 0;
			continue;
		}
		if (!layer_auto_off_reg[layer].previous) {
			/* start watching */
			layer_auto_off_reg[layer].previous = 1;
			layer_auto_off_reg[layer].ongoing = 1;
			layer_auto_off_reg[layer].timer = timer_read32();
			continue;
		}
		/* watching state */
		if (layer_auto_off_reg[layer].ongoing) {
			if (timer_elapsed32(layer_auto_off_reg[layer].timer) < LAYER_AUTO_OFF_TIMEOUT) continue;
			/* timer expired */
			layer_auto_off_reg[layer].ongoing = 0;
		}
		layer_off(layer);
	}
}

static void
layer_auto_off_record(keyrecord_t *record)
{
	uint8_t layer;

	if (record->event.pressed) {
		/* timer extension only */
		for (layer = 0; layer < MAX_LAYER; layer++) {
			if (!layer_auto_off_reg[layer].enable) continue;
			if (layer_auto_off_reg[layer].immediate) continue;
			if (!layer_auto_off_reg[layer].ongoing) continue;
			layer_auto_off_reg[layer].timer = timer_read32();
		}
	} else {
		for (layer = 0; layer < MAX_LAYER; layer++) {
			if (!layer_auto_off_reg[layer].enable) continue;
			if (layer_auto_off_reg[layer].immediate) {
				layer_auto_off_reg[layer].ongoing = 0;
			} else if (layer_auto_off_reg[layer].ongoing) {
				layer_auto_off_reg[layer].timer = timer_read32();
			}
		}
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
@startuml tapping-multi-modifier-assistant.png
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
#if defined(KA_ENABLE)
	housekeeping_task_ka();
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
#if defined(KA_ENABLE)
		    process_record_ka(keycode, record) &&
#endif
		    true)) {
		return false;
	}
	return true;
}

/*
 * Customizations
 */

void
keyboard_post_init_user(void)
{
#ifdef KCR_ENABLE
	debug_enable = 1;
#endif /* KCR_ENABLE */
#ifdef LAO_ENABLE
	layer_auto_off_reg[2].enable = 1;
	layer_auto_off_reg[6].enable = 1;
	layer_auto_off_reg[7].enable = 1;
	layer_auto_off_reg[6].immediate = 1;
	layer_auto_off_reg[7].immediate = 1;
#endif /* LAO_ENABLE */
#ifdef KA_ENABLE
	uint8_t index = 0;
	key_action_config_layer4   (key_action_reg_init(index++, MO2X), 2, 2, 2, 3);
	key_action_config_kc_layer2(key_action_reg_init(index++, LALTX), KC_LALT, 5);
	key_action_config_layer4   (key_action_reg_init(index++, L9687), 9, 6, 8, 7);
	key_action_config_fast_kc2 (key_action_reg_init(index++, LALTR), KC_LALT, KC_R);
	key_action_config_kc2      (key_action_reg_init(index++, KCXV), KC_X, KC_V);
#endif /* KA_ENABLE */
}

bool
oled_task_user(void)
{
	uint8_t layer;

	if (!is_keyboard_master()) return true;

	oled_write_P(PSTR("L:"), false);
	for (layer = 0; layer < 10; layer++) {
		if (IS_LAYER_ON(layer)) {
#ifdef LAO_ENABLE
			if (layer_auto_off_reg[layer].enable) {
				oled_write_char(layer + '0', true);
			} else {
				oled_write_char(layer + '0', false);
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
