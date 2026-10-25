/* Environment — environment.c — ambient/mirror temperature and humidity.
 *
 * Ambient temperature and humidity come from the SHT31 over I2C (driver: sht31);
 * the mirror temperature comes from the DS18B20 over 1-wire (driver: ds18b20).
 * The dew point is computed from the ambient temperature and humidity (Magnus
 * formula).
 */
#include "environment.h"

#include <math.h>

#include "esp_log.h"
#include "ds18b20.h"
#include "sht31.h"

static const char *TAG = "ENVIRONMENT";

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
    }
    if (ds18b20_init() != ESP_OK) {
        ESP_LOGW(TAG, "DS18B20 unavailable — mirror reporting disabled");
    }

    environment_update();
}

void environment_update(void) {
    if (sht31_is_present()) {
        float t, h;
        /* Retry once — a single-shot can fail transiently. */
        esp_err_t err = sht31_read(&t, &h);
        if (err != ESP_OK) {
            err = sht31_read(&t, &h);
        }

        if (err == ESP_OK) {
            s_ambient_c = t;
            s_humidity_pct = h;
            s_dew_point_c = dew_point_c(t, h);
            s_ambient_valid = true;
        } else {
            s_ambient_valid = false;
        }
    } else {
        s_ambient_valid = false;
    }

    /* Mirror (DS18B20) — non-blocking conversion/read state machine. */
    ds18b20_update();
}

bool environment_ambient_valid(void) { return s_ambient_valid; }
bool environment_mirror_valid(void) { return ds18b20_reading_valid(); }
float environment_get_ambient_c(void)   { return s_ambient_c; }
float environment_get_humidity_pct(void){ return s_humidity_pct; }
float environment_get_dew_point_c(void) { return s_dew_point_c; }

/*
 * Fall back to the ambient temperature while no DS18B20 reading is available,
 * so the fan AUTO delta stays 0 until the mirror has been read.
 */
float environment_get_mirror_c(void) {
    return environment_mirror_valid() ? ds18b20_get_temperature_c() : s_ambient_c;
}
