/* Environment — environment.c — ambient/mirror temperature and humidity.
 *
 * Ambient temperature and humidity come from the SHT31 over I2C (driver: sht31);
 * the mirror temperature (DS18B20) is still mocked. The dew point is computed
 * from the ambient temperature and humidity (Magnus formula).
 */
#include "environment.h"

#include <math.h>

#include "esp_log.h"
#include "sht31.h"

static const char *TAG = "ENVIRONMENT";

/* Mirror temperature is still the mock until the DS18B20 is ported. */
#define MIRROR_C_MOCK   24.5f

/* Seed values used until the first successful SHT31 read. */
#define AMBIENT_C_SEED  25.0f
#define HUMIDITY_SEED   50.0f

static float s_ambient_c;
static float s_humidity_pct;
static float s_dew_point_c;
static bool  s_ambient_valid;

/* Magnus-formula dew point. */
static float dew_point_c(float temp_c, float humidity_pct) {
    const float a = 17.62f;
    const float b = 243.12f;
    float gamma = logf(humidity_pct / 100.0f) + a * temp_c / (b + temp_c);
    return b * gamma / (a - gamma);
}

void environment_init(void) {
    s_ambient_c = AMBIENT_C_SEED;
    s_humidity_pct = HUMIDITY_SEED;
    s_dew_point_c = dew_point_c(s_ambient_c, s_humidity_pct);
    s_ambient_valid = false;

    if (sht31_init() != ESP_OK) {
        ESP_LOGW(TAG, "SHT31 unavailable — ambient/humidity reporting disabled");
        return;
    }

    environment_update();
}

void environment_update(void) {
    if (!sht31_is_present()) {
        s_ambient_valid = false;
        return;
    }

    float t, h;
    /* Retry once — a single-shot can fail transiently. */
    esp_err_t err = sht31_read(&t, &h);
    if (err != ESP_OK) {
        err = sht31_read(&t, &h);
    }

    if (err != ESP_OK) {
        s_ambient_valid = false;
        return;
    }

    s_ambient_c = t;
    s_humidity_pct = h;
    s_dew_point_c = dew_point_c(t, h);
    s_ambient_valid = true;
}

bool environment_ambient_valid(void) { return s_ambient_valid; }
float environment_get_ambient_c(void)   { return s_ambient_c; }
float environment_get_mirror_c(void)    { return MIRROR_C_MOCK; }
float environment_get_humidity_pct(void){ return s_humidity_pct; }
float environment_get_dew_point_c(void) { return s_dew_point_c; }
