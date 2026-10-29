#pragma once

#include <stdbool.h>

/*
 * Local buttons — one push button, push-to-GND (active-low, internal pull-up):
 *
 *   1. focuser enable/disable toggle (BUTTON_FOCUSER_ENABLE_GPIO).
 *
 * buttons_update() samples and debounces the button and, on a press edge,
 * applies the corresponding action. The *_pressed() getter reports the current
 * debounced level, for the protocol `state/buttons` handler.
 */

void buttons_init(void);

/* Sample, debounce and act on the button. Called from the runtime loop. */
void buttons_update(void);

bool buttons_focuser_enable_pressed(void);
