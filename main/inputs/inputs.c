/* Inputs — inputs.c — mock local button state.
 *
 * No buttons are wired yet, so everything reports released. Replaced by GPIO
 * reads with debounce when the buttons are ported.
 */
#include "inputs.h"

void inputs_init(void) {
}

bool inputs_focuser_enable_pressed(void) { return false; }
bool inputs_fan_mode_pressed(void)       { return false; }
