#pragma once

#include <stdbool.h>

/*
 * Local inputs (mock for now — the physical buttons are not yet wired).
 *
 * Two buttons will exist:
 *   1. focuser enable/disable toggle;
 *   2. fan mode toggle (normal ↔ off).
 */

void inputs_init(void);

bool inputs_focuser_enable_pressed(void);
bool inputs_fan_mode_pressed(void);
