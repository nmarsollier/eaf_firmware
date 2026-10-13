#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Fan subsystem — Noctua NF-A8 PWM with a 12 V relay.
 *
 * Exposed as an Alpaca Switch: a boolean power channel (the 12 V relay) and a
 * numeric speed channel (PWM 0–100 %). RPM is read from the tachometer (not
 * wired yet). The speed input is driven with a 25 kHz PWM through an
 * open-drain N-MOSFET.
 */

void fan_init(void);

/* Power channel — switches the fan's 12 V relay. */
void fan_set_power(bool on);
bool fan_get_power(void);

/* Speed channel — PWM duty 0–100 % (clamped). */
void fan_set_speed(uint8_t pct);
uint8_t fan_get_speed(void);

/* Fan RPM from the tachometer; -1 while the tach is not wired. */
int fan_get_rpm(void);
