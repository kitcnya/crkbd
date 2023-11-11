/*
 * Corne keyboard customizations
 * Copyright and License Terms are pending, 2023, kitcnya@outlook.com
 */

#include QMK_KEYBOARD_H
#ifdef CONSOLE_ENABLE
#include "print.h"
#endif

/*
 * hacks of sysmtem interfaces
 *
 * see:
 * - https://docs.qmk.fm/#/custom_quantum_functions
 * - https://docs.qmk.fm/#/feature_macros
 */

#define MAXTIMER	60000		/* msec */
static uint16_t basetime = 0;

void
housekeeping_task_user(void)
{
#ifdef CONSOLE_ENABLE
	if (debug_enable) {
		uint16_t time = timer_elapsed(basetime);
		if (time < MAXTIMER) return;
		basetime += time;
		print("LAPPED\n");
	}
#endif
}

bool
process_record_user(uint16_t keycode, keyrecord_t *record)
{
#ifdef CONSOLE_ENABLE
	if (debug_enable) {
		uint16_t time = timer_elapsed(basetime);
		uprintf("%05u %04X %s\n", time, keycode, (record->event.pressed ? "p" : "r"));
	}
#endif
	return true;
}
