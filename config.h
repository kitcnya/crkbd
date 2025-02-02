/*
 * Copyright (C) 2023 kitcnya@outlook.com
 * https://opensource.org/license/mit/
 * SPDX-License-Identifier: MIT
 */

#pragma once

/* Mouse Key Configuration */
#define MK_KINETIC_SPEED	1	// Kinetic Mode
#define MOUSEKEY_DELAY		5	// 5, Delay between pressing a movement key and cursor movement
#define MOUSEKEY_INTERVAL	10	// 10, Time between cursor movements in milliseconds
#define MOUSEKEY_MOVE_DELTA	10	// 16, Step size for accelerating from initial to base speed
#define MOUSEKEY_INITIAL_SPEED	8	// 100, Initial speed of the cursor in pixel per second
#define MOUSEKEY_BASE_SPEED	2000	// 5000, Maximum cursor speed at which acceleration stops

/* Key Action Configuration */
#define MAX_KA_REG	8
//#define KA_DEBUG	1

/* Special Keys */
#define MO2X	QK_USER_0
#define LALTX	QK_USER_1
#define L9687	QK_USER_2
#define LALTR	QK_USER_3
#define KCXV	QK_USER_4
