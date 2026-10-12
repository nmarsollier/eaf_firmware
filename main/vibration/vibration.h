#pragma once

#include <stdbool.h>

/*
 * Vibration subsystem — turns ADXL345 acceleration into a per-axis movement
 * figure (demeaned RMS in m/s²). Samples are averaged per second, and queries
 * return the last completed second's measurement.
 *
 * A missing sensor or I2C fault leaves the module disabled and, through the
 * led module, flags the board LED as error.
 */

void vibration_init(void);
void vibration_update(void);

bool vibration_get_enabled(void);
bool vibration_get_valid(void);
float vibration_get_rms_x(void);
float vibration_get_rms_y(void);
float vibration_get_rms_z(void);
float vibration_get_rms_total(void);
