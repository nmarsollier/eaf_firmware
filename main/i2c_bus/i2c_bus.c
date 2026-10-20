/* i2c_bus — i2c_bus.c — shared I2C master bus.
 *
 * Owns the single I2C master bus shared by the ADXL345 and the SHT31. Creation
 * is idempotent so each driver can call i2c_bus_init() in any order.
 */
#include "i2c_bus.h"

#include "esp_log.h"

#include "config.h"

static const char *TAG = "I2C_BUS";

#define I2C_PORT    I2C_NUM_0
#define I2C_CLK_HZ  100000    /* 100 kHz standard mode */

static i2c_master_bus_handle_t s_bus_handle = NULL;

esp_err_t i2c_bus_init(void) {
    if (s_bus_handle != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port               = I2C_PORT,
        .sda_io_num             = I2C_SDA_GPIO,
        .scl_io_num             = I2C_SCL_GPIO,
        .clk_source             = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt      = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &s_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(err));
        s_bus_handle = NULL;
        return err;
    }

    ESP_LOGI(TAG, "I2C master bus ready (SDA %d, SCL %d, %d kHz)",
             (int) I2C_SDA_GPIO, (int) I2C_SCL_GPIO, I2C_CLK_HZ / 1000);
    return ESP_OK;
}

i2c_master_bus_handle_t i2c_bus_get(void) {
    return s_bus_handle;
}
