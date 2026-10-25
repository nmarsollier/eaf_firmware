#pragma once

#include <stdbool.h>

/*
 * Environment sensors — ambient temperature/humidity from the SHT31 (I2C) and
 * the mirror temperature from the DS18B20 (1-wire). The dew point is computed
 * from the ambient temperature and humidity.
 *
 * Measurements are refreshed by environment_update() from the runtime loop and
 * cached; the getters return the last cached values. The mirror temperature is
 * read through a non-blocking conversion/read state machine, so its first
 * valid value appears after the first ~750 ms conversion completes.
 */

void environment_init(void);

/* Refresh the SHT31 and advance the DS18B20 read into the cache. */
void environment_update(void);

/* True when the last SHT31 read succeeded (ambient/humidity/dew are valid). */
bool environment_ambient_valid(void);

/* True when a DS18B20 temperature has been read successfully. */
bool environment_mirror_valid(void);

float environment_get_ambient_c(void);
float environment_get_mirror_c(void);
float environment_get_humidity_pct(void);
float environment_get_dew_point_c(void);
