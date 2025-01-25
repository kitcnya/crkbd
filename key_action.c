/*
 * Copyright (C) 2025 kitcnya@outlook.com
 * https://opensource.org/license/mit/
 * SPDX-License-Identifier: MIT
 */

#include QMK_KEYBOARD_H
#include "key_action.h"
#ifdef CONSOLE_ENABLE
#include "print.h"
#endif

#if defined(CONSOLE_ENABLE) && defined(KA_DEBUG)
static void
_dbgmsg(struct key_action_reg_st *p, uint16_t event)
{
	uint16_t time = timer_read();
	uint16_t kc = p->keycode;
	union __attribute__((packed)) {
		struct key_action_status_st status;
		uint16_t u16;
	} t = {.status =  p->status};
	uint16_t timer = p->timer;
	uint16_t state = p->state;
	uprintf("%05u %04X %02X %05u %02u %02u\n", time, kc, t.u16, timer, state, event);
}
#define dbgmsg(p, event) _dbgmsg(p, event)
#else
#define dbgmsg(p, event) do {} while(0)
#endif

static struct key_action_reg_st key_action_reg[MAX_KA_REG] = {};

struct key_action_reg_st *
key_action_reg_init(uint8_t index, uint16_t keycode)
{
	if (index >= MAX_KA_REG) return 0;
	struct key_action_reg_st *p = &key_action_reg[index];
	p->control.enable = 1;
	p->control.ntap2 = 0;
	p->control.type = KA_TYPE_NONE;
	p->keycode = keycode;
	p->timeout = KEY_ACTION_TIMEOUT;
	p->layer1 = 0;
	p->layer2 = 0;
	p->layer3 = 0;
	p->layer4 = 0;
	p->kc1 = KC_NO;
	p->kc2 = KC_NO;
	p->status.pending = 0;
	p->status.watching = 0;
	p->state = KA_WAITING_PRESS1;
	return p;
}

void
key_action_config_timeout(struct key_action_reg_st *p, uint16_t timeout)
{
	if (!p) return;
	p->timeout = timeout;
}

void
key_action_config_layer2(struct key_action_reg_st *p, uint8_t layer1, uint8_t layer2)
{
	if (!p) return;
	p->control.ntap2 = 0;
	p->control.type = KA_TYPE_LAYER2;
	p->layer1 = layer1;
	p->layer2 = layer2;
}

void
key_action_config_layer4(struct key_action_reg_st *p, uint8_t layer1, uint8_t layer2, uint8_t layer3, uint8_t layer4)
{
	if (!p) return;
	p->control.ntap2 = 1;
	p->control.type = KA_TYPE_LAYER4;
	p->layer1 = layer1;
	p->layer2 = layer2;
	p->layer3 = layer3;
	p->layer4 = layer4;
}

void
key_action_config_kc_layer2(struct key_action_reg_st *p, uint16_t kc1, uint8_t layer1)
{
	if (!p) return;
	p->control.ntap2 = 0;
	p->control.type = KA_TYPE_KC_LAYER2;
	p->kc1 = kc1;
	p->layer1 = layer1;
}

void
key_action_config_kc_layer4(struct key_action_reg_st *p, uint16_t kc1, uint8_t layer1, uint16_t kc2, uint8_t layer2)
{
	if (!p) return;
	p->control.ntap2 = 1;
	p->control.type = KA_TYPE_KC_LAYER4;
	p->kc1 = kc1;
	p->layer1 = layer1;
	p->kc2 = kc2;
	p->layer2 = layer2;
}

void
key_action_config_kc2(struct key_action_reg_st *p, uint16_t kc1, uint16_t kc2)
{
	if (!p) return;
	p->control.ntap2 = 0;
	p->control.type = KA_TYPE_KC2;
	p->kc1 = kc1;
	p->kc2 = kc2;
}

void
key_action_config_fast_kc2(struct key_action_reg_st *p, uint16_t kc1, uint16_t kc2)
{
	if (!p) return;
	p->control.ntap2 = 0;
	p->control.type = KA_TYPE_FAST_KC2;
	p->kc1 = kc1;
	p->kc2 = kc2;
}

static void
key_action_notify(struct key_action_reg_st *p, enum key_action_event event)
{
	dbgmsg(p, event);
	switch(p->control.type) {
	case KA_TYPE_LAYER2:
		switch (event) {
		case KA_EVENT_FAST1:
		case KA_EVENT_HEND1:
			layer_invert(p->layer1);
			break;
		case KA_EVENT_TAP1:
			layer_invert(p->layer1);
			layer_invert(p->layer2);
			break;
		default:
			break;
		}
		break;
	case KA_TYPE_LAYER4:
		switch (event) {
		case KA_EVENT_HOLD1:
		case KA_EVENT_QHOLD1:
			layer_on(p->layer1);
			break;
		case KA_EVENT_HEND1:
			layer_off(p->layer1);
			break;
		case KA_EVENT_TEND1:
			layer_invert(p->layer2);
			break;
		case KA_EVENT_HOLD2:
		case KA_EVENT_QHOLD2:
			layer_on(p->layer3);
			break;
		case KA_EVENT_HEND2:
			layer_off(p->layer3);
			break;
		case KA_EVENT_TEND2:
			layer_invert(p->layer4);
			break;
		default:
			break;
		}
		break;
	case KA_TYPE_KC_LAYER2:
		switch (event) {
		case KA_EVENT_FAST1:
			register_code(p->kc1);
			break;
		case KA_EVENT_TAP1:
			unregister_code(p->kc1);
			layer_invert(p->layer1);
			break;
		case KA_EVENT_HEND1:
			unregister_code(p->kc1);
			break;
		default:
			break;
		}
		break;
	case KA_TYPE_KC_LAYER4:
		switch (event) {
		case KA_EVENT_FAST1:
			register_code(p->kc1);
			break;
		case KA_EVENT_TAP1:
			unregister_code(p->kc1);
			layer_invert(p->layer1);
			break;
		case KA_EVENT_HEND1:
			unregister_code(p->kc1);
			break;
		case KA_EVENT_FAST2:
			layer_invert(p->layer1);
			register_code(p->kc2);
			break;
		case KA_EVENT_TAP2:
			unregister_code(p->kc2);
			layer_invert(p->layer2);
			break;
		case KA_EVENT_HEND2:
			unregister_code(p->kc2);
			break;
		default:
			break;
		}
		break;
	case KA_TYPE_KC2:
		switch (event) {
		case KA_EVENT_TAP1:
			register_code(p->kc1);
			break;
		case KA_EVENT_TEND1:
			unregister_code(p->kc1);
			break;
		case KA_EVENT_QHOLD1:
		case KA_EVENT_HOLD1:
			register_code(p->kc2);
			break;
		case KA_EVENT_HEND1:
			unregister_code(p->kc2);
			break;
		default:
			break;
		}
		break;
	case KA_TYPE_FAST_KC2:
		switch (event) {
		case KA_EVENT_FAST1:
			register_code(p->kc1); /* fast register */
			break;
		case KA_EVENT_TAP1:
			unregister_code(p->kc1); /* cancel */
			register_code(p->kc2);
			break;
		case KA_EVENT_TEND1:
			unregister_code(p->kc2);
			break;
		case KA_EVENT_HEND1:
			unregister_code(p->kc1);
			break;
		default:
			break;
		}
		break;
	default:
		dbgmsg(p, 99);
		break;
	}
}

static void
key_action_process_key_press(struct key_action_reg_st *p)
{
	switch (p->state) {
	case KA_WAITING_PRESS1:
		key_action_notify(p, KA_EVENT_FAST1);
		p->state = KA_WAITING_RELEASE1_PRESS_OR_T;
		break;
	case KA_WAITING_PRESS2_OR_T:
		if (p->control.ntap2) {
			key_action_notify(p, KA_EVENT_FAST2);
			p->state = KA_WAITING_RELEASE2_PRESS_OR_T;
		} else {
			key_action_notify(p, KA_EVENT_TEND1);
			key_action_notify(p, KA_EVENT_FAST1);
			p->state = KA_WAITING_RELEASE1_PRESS_OR_T;
		}
		break;
	case KA_WAITING_PRESS3_OR_T:
		key_action_notify(p, KA_EVENT_TEND2);
		key_action_notify(p, KA_EVENT_FAST1);
		p->state = KA_WAITING_RELEASE1_PRESS_OR_T;
		break;
	default:
		dbgmsg(p, 90);
		break;
	}
	p->status.pending = 1;
	p->status.watching = 1;
	p->timer = timer_read();
}

static void
key_action_process_key_release(struct key_action_reg_st *p)
{
	p->status.pending = 0;
	p->status.watching = 0;
	switch (p->state) {
	case KA_WAITING_RELEASE1_PRESS_OR_T:
		key_action_notify(p, KA_EVENT_TAP1);
		p->state = KA_WAITING_PRESS2_OR_T;
		p->status.pending = 1;
		p->timer = timer_read();
		break;
	case KA_WAITING_RELEASE1:
		key_action_notify(p, KA_EVENT_HEND1);
		p->state = KA_WAITING_PRESS1;
		break;
	case KA_WAITING_RELEASE2_PRESS_OR_T:
		key_action_notify(p, KA_EVENT_TAP2);
		p->state = KA_WAITING_PRESS3_OR_T;
		p->status.pending = 1;
		p->timer = timer_read();
		break;
	case KA_WAITING_RELEASE2:
		key_action_notify(p, KA_EVENT_HEND2);
		p->state = KA_WAITING_PRESS1;
		break;
	default:
		dbgmsg(p, 91);
		break;
	}
}

static void
key_action_process_another_key(struct key_action_reg_st *p)
{
	p->status.pending = 0;
	p->status.watching = 0;
	switch (p->state) {
	case KA_WAITING_RELEASE1_PRESS_OR_T:
		key_action_notify(p, KA_EVENT_QHOLD1);
		p->state = KA_WAITING_RELEASE1;
		break;
	case KA_WAITING_RELEASE2_PRESS_OR_T:
		key_action_notify(p, KA_EVENT_QHOLD2);
		p->state = KA_WAITING_RELEASE2;
		break;
	default:
		dbgmsg(p, 92);
		break;
	}
}

static void
key_action_process_timeout(struct key_action_reg_st *p)
{
	p->status.pending = 0;
	p->status.watching = 0;
	switch (p->state) {
	case KA_WAITING_RELEASE1_PRESS_OR_T:
		key_action_notify(p, KA_EVENT_HOLD1);
		p->state = KA_WAITING_RELEASE1;
		break;
	case KA_WAITING_PRESS2_OR_T:
		key_action_notify(p, KA_EVENT_TEND1);
		p->state = KA_WAITING_PRESS1;
		break;
	case KA_WAITING_RELEASE2_PRESS_OR_T:
		key_action_notify(p, KA_EVENT_HOLD2);
		p->state = KA_WAITING_RELEASE2;
		break;
	case KA_WAITING_PRESS3_OR_T:
		key_action_notify(p, KA_EVENT_TEND2);
		p->state = KA_WAITING_PRESS1;
		break;
	default:
		dbgmsg(p, 93);
		break;
	}
}

void
housekeeping_task_ka(void)
{
	uint8_t i;

	for (i = 0; i < MAX_KA_REG; i++) {
		struct key_action_reg_st *p = &key_action_reg[i];
		if (!p->control.enable) continue;
		if (!p->status.pending) continue;
		if (timer_elapsed(p->timer) < p->timeout) continue;
		key_action_process_timeout(p);
	}
}

bool
process_record_ka(uint16_t keycode, keyrecord_t *record)
{
	uint8_t i;

	for (i = 0; i < MAX_KA_REG; i++) {
		struct key_action_reg_st *p = &key_action_reg[i];
		if (!p->control.enable) continue;
		if (keycode != p->keycode) continue;
		if (record->event.pressed) {
			key_action_process_key_press(p);
		} else {
			key_action_process_key_release(p);
		}
		return false;
	}
	/* another key event */
	for (i = 0; i < MAX_KA_REG; i++) {
		struct key_action_reg_st *p = &key_action_reg[i];
		if (!p->control.enable) continue;
		if (!p->status.watching) continue;
		key_action_process_another_key(p);
	}
	return true;
}
