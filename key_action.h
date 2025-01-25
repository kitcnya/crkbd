/*
 * Copyright (C) 2025 kitcnya@outlook.com
 * https://opensource.org/license/mit/
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifndef MAX_KA_REG
#define MAX_KA_REG	4
#endif /* MAX_KA_REG */

#ifndef KEY_ACTION_TIMEOUT
#define KEY_ACTION_TIMEOUT	190
#endif /* KEY_ACTION_TIMEOUT */

/*
 * State Transition Table
 *
 * | State  | Event           | Next   | Timer   | Action |
 * |--------+-----------------+--------+---------+--------|
 * | P1     | KEY press       | R1/P/T | start T | FAST1  |
 * |--------+-----------------+--------+---------+--------|
 * | R1/P/T | KEY release     | P2/T   | start T | TAP1   |
 * | R1/P/T | OKEY press      | R1     |         | QHOLD1 |
 * | R1/P/T | T expired       | R1     |         | HOLD1  |
 * | P2/T   | KEY press again | R2/P/T | start T | FAST2  | <- ntap2
 * | P2/T   | T expired       | P1     |         | TEND1  |
 * | R1     | KEY release     | P1     |         | HEND1  |
 * |--------+-----------------+--------+---------+--------|
 * | R2/P/T | KEY release     | P3/T   | start T | TAP2   |
 * | R2/P/T | OKEY press      | R2     |         | QHOLD2 |
 * | R2/P/T | T expired       | R2     |         | HOLD2  |
 * | P3/T   | KEY press again | R1/P/T | start T | FAST1  | <- limit
 * | P3/T   | T expired       | P1     |         | TEND2  |
 * | R2     | KEY release     | P1     |         | HEND2  |
 *
 * Pseudo Flow (python like)
 *
 * if key.pressed:
 *     FAST1
 *     if T.expired:
 *         HOLD1
 *         if key.released:
 *             HEND1 # FAST1-HOLD1-HEND1
 *     else if anotherkey:
 *         QHOLD1
 *         if key.released:
 *             HEND1 # FAST1-QHOLD1-HEND1
 *     else if key.released:
 *         TAP1
 *         if T.expired:
 *             TEND1 # FAST1-TAP1-TEND1
 *         else if key.pressed: # 2nd
 *             if not ntap2:
 *                 TEND1 # FAST1-TAP1-TEND1
 *                 FAST1 # recursive!
 *             else:
 *                 FAST2
 *                 if T.expired:
 *                     HOLD2
 *                     if key.released:
 *                         HEND2 # FAST1-TAP1-FAST2-HOLD2-HEND2
 *                 else if anotherkey:
 *                     QHOLD2
 *                     if key.released:
 *                         HEND2 # FAST1-TAP1-FAST2-QHOLD2-HEND2
 *                 else if key.released:
 *                     TAP2
 *                     if T.expired:
 *                         TEND2 # FAST1-TAP1-FAST2-TAP2-TEND2
 *                     else if key.pressed: # 3rd
 *                         TEND2 # FAST1-TAP1-FAST2-TAP2-TEND2
 *                         FAST1 # recursive!
 *
 * Sequences (tap1 == not tap2):
 *
 *   FAST1-HOLD1-HEND1
 *   FAST1-QHOLD1-HEND1
 *   FAST1-TAP1-TEND1
 * 
 * Sequences (tap2):
 *
 *   FAST1-HOLD1-HEND1
 *   FAST1-QHOLD1-HEND1
 *   FAST1-TAP1-TEND1
 *   FAST1-TAP1-FAST2-HOLD2-HEND2
 *   FAST1-TAP1-FAST2-QHOLD2-HEND2
 *   FAST1-TAP1-FAST2-TAP2-TEND2
 */

enum key_action_state {
	KA_WAITING_PRESS1,
	KA_WAITING_RELEASE1_PRESS_OR_T,
	KA_WAITING_PRESS2_OR_T,
	KA_WAITING_RELEASE1,
	KA_WAITING_RELEASE2_PRESS_OR_T,
	KA_WAITING_PRESS3_OR_T,
	KA_WAITING_RELEASE2,
};

enum key_action_event {
	/* 1st events */
	KA_EVENT_FAST1,
	KA_EVENT_TAP1,
	KA_EVENT_QHOLD1,		/* quick hold */
	KA_EVENT_HOLD1,
	KA_EVENT_TEND1,			/* tap end */
	KA_EVENT_HEND1,			/* hold end */
	/* 2nd events */
	KA_EVENT_FAST2,
	KA_EVENT_TAP2,
	KA_EVENT_QHOLD2,		/* quick hold */
	KA_EVENT_HOLD2,
	KA_EVENT_TEND2,			/* tap end */
	KA_EVENT_HEND2,			/* hold end */
};

enum key_action_type {
	KA_TYPE_NONE,			/* no action */
	KA_TYPE_LAYER2,			/* MO(), TG() */
	KA_TYPE_LAYER4,			/* MO(), TG(), MO(), TG() */
	KA_TYPE_KC_LAYER2,		/* KC(), TG() */
	KA_TYPE_KC_LAYER4,		/* KC(), TG(), KC(), TG() */
	KA_TYPE_KC2,			/* Tap for KC(), Hold for KC() */
	KA_TYPE_FAST_KC2,		/* Tap for KC(), Hold for KC() w/fast */
};

struct __attribute__((packed)) key_action_control_st {
	uint8_t enable:1;		/* master control */
	uint8_t ntap2:1;		/* number of taps */
	uint8_t type:4;			/* action type */
};

struct __attribute__((packed)) key_action_status_st {
	uint8_t pending:1;		/* timer is running */
	uint8_t watching:1;		/* watching another key */
};

struct key_action_reg_st {
	/* settings */
	struct key_action_control_st control;
	uint16_t keycode;		/* keycode to sense */
	uint16_t timeout;		/* timeout value */
	/* action parameters */
	struct {
		uint16_t layer1:4;
		uint16_t layer2:4;
		uint16_t layer3:4;
		uint16_t layer4:4;
	};
	uint16_t kc1;
	uint16_t kc2;
	/* status */
	struct key_action_status_st status;
	enum key_action_state state;	/* current machine state */
	uint16_t timer;			/* time stamp to measure time */
};

extern struct key_action_reg_st *key_action_reg_init(uint8_t index, uint16_t keycode);
extern void key_action_config_timeout(struct key_action_reg_st *p, uint16_t timeout);
extern void key_action_config_layer2(struct key_action_reg_st *p, uint8_t layer1, uint8_t layer2);
extern void key_action_config_layer4(struct key_action_reg_st *p, uint8_t layer1, uint8_t layer2, uint8_t layer3, uint8_t layer4);
extern void key_action_config_kc_layer2(struct key_action_reg_st *p, uint16_t kc1, uint8_t layer1);
extern void key_action_config_kc_layer4(struct key_action_reg_st *p, uint16_t kc1, uint8_t layer1, uint16_t kc2, uint8_t layer2);
extern void key_action_config_kc2(struct key_action_reg_st *p, uint16_t kc1, uint16_t kc2);
extern void key_action_config_fast_kc2(struct key_action_reg_st *p, uint16_t kc1, uint16_t kc2);

extern void housekeeping_task_ka(void);
extern bool process_record_ka(uint16_t keycode, keyrecord_t *record);
