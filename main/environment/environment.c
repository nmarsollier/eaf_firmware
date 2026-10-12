/* Environment — environment.c — mock sensor measurements.
 *
 * Fixed values; replaced by SHT31 + DS18B20 reads when ported.
 */
#include "environment.h"

void environment_init(void) {
}

float environment_get_ambient_c(void)   { return 25.0f; }
float environment_get_mirror_c(void)    { return 24.5f; }
float environment_get_humidity_pct(void){ return 50.0f; }
float environment_get_dew_point_c(void) { return 13.0f; }
