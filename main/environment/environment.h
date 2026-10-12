#pragma once

/*
 * Environment sensors (mock for now — SHT31 ambient temp/humidity and DS18B20
 * mirror temp are not yet ported). Exposes fixed, coherent measurements.
 */

void environment_init(void);

float environment_get_ambient_c(void);
float environment_get_mirror_c(void);
float environment_get_humidity_pct(void);
float environment_get_dew_point_c(void);
