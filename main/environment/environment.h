#pragma once

#include <stdbool.h>

/*
 * Environment sensors — ambient temperature/humidity from the SHT31 (I2C) and
 * the mirror temperature (DS18B20, still mocked). The dew point is computed
 * from the ambient temperature and humidity.
 *
 * Measurements are refreshed by environment_update() from the runtime loop and
 * cached; the getters return the last cached values.
 */

void environment_init(void);

/* Refresh the SHT31 (and, later, the DS18B20) reading into the cache. */
void environment_update(void);

/* True when the last SHT31 read succeeded (ambient/humidity/dew are valid). */
bool environment_ambient_valid(void);

float environment_get_ambient_c(void);
float environment_get_mirror_c(void);
float environment_get_humidity_pct(void);
float environment_get_dew_point_c(void);
