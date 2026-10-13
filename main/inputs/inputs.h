#pragma once

#include <stdbool.h>

/*
 * Local inputs — two push buttons, push-to-GND (active-low, internal pull-ups):
 *
 *   1. focuser enable/disable toggle (BUTTON_FOCUSER_ENABLE_GPIO);
 *   2. fan mode cycle: off → auto → manual → off (BUTTON_FAN_MODE_GPIO).
 *
 * inputs_update() samples and debounces both buttons and, on a press edge,
 * applies the corresponding action. The *_pressed() getters report the current
 * debounced level, for the protocol `state/inputs` handler.
 */

void inputs_init(void);

/* Sample, debounce and act on the buttons. Called from the runtime loop. */
void inputs_update(void);

bool inputs_focuser_enable_pressed(void);
bool inputs_fan_mode_pressed(void);
