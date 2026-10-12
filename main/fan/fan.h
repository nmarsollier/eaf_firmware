#pragma once

#include <stdbool.h>

/*
 * Fan subsystem (mock for now — the real Noctua PWM + relay control is not yet
 * ported). Exposes the mode and the derived outputs so the protocol layer can
 * report them.
 */

void fan_init(void);

/* "normal" | "soft" | "off". */
const char *fan_get_mode(void);

/* Set the mode; returns false on an unknown value. */
bool fan_set_mode(const char *mode);

/* Effective PWM duty fed to the fan input, in percent (mock). */
int fan_get_output_pwm_pct(void);

/* Relay command: true while an active mode keeps the fan powered (mock). */
bool fan_get_relay_command(void);
