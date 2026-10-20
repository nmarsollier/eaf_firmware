/* Sht31 — sht31_init.c
 *
 * Purpose: bring up the SHT31 sensor on the shared I2C bus.
 *
 * Attaches a device handle for address 0x44 to the shared bus (i2c_bus),
 * probes it and runs a priming measurement. A missing sensor is not fatal: the
 * environment module simply reports ambient/humidity unavailable.
 */
#include "sht31_internal.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_bus.h"

static const char *TAG = "SHT31_INIT";

Sht31Sensor sht31_sensor;

bool sht31_is_present(void) {
    return sht31_sensor.present;
}

esp_err_t sht31_init(void) {
    sht31_sensor.address    = SHT31_ADDR;
    sht31_sensor.dev_handle = NULL;
    sht31_sensor.present    = false;

    esp_err_t err = i2c_bus_init();
    if (err != ESP_OK) {
        return err;
    }
    i2c_master_bus_handle_t bus = i2c_bus_get();

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = sht31_sensor.address,
        .scl_speed_hz    = SHT31_CLK_HZ,
    };

    err = i2c_master_bus_add_device(bus, &dev_config, &sht31_sensor.dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "addr 0x%02X: device handle failed: %s",
                 sht31_sensor.address, esp_err_to_name(err));
        return err;
    }

    /* Probe the sensor with retries — I2C can fail intermittently. */
    err = ESP_ERR_NOT_FOUND;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        err = i2c_master_probe(bus, sht31_sensor.address, SHT31_TIMEOUT_MS);
        if (err == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "addr 0x%02X: probe attempt %d/3 failed (%s)",
                 sht31_sensor.address, attempt + 1, esp_err_to_name(err));
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "addr 0x%02X: not present after 3 attempts (%s) — sensor disabled",
                 sht31_sensor.address, esp_err_to_name(err));
        return err;
    }

    /* Prime the sensor: the first single-shot after power-up can be stale. */
    float t, h;
    sht31_read(&t, &h);

    sht31_sensor.present = true;
    ESP_LOGI(TAG, "SHT31 at 0x%02X ready", sht31_sensor.address);
    return ESP_OK;
}
