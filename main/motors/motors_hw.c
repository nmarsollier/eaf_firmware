/* Motors - motors_hw.c
 *
 * Purpose: DIR GPIO control for the TMC2209 stepper driver.
 *
 * STEP pulse generation is handled by the RMT peripheral (motors_rmt.c).
 * EN (enable) is driven by the TMC module at boot (tmc_init.c) and, from then
 * on, by motors_hw_set_enabled() when the focuser is enabled/disabled.
 * DIR and STEP connect directly to the TMC2209 (VCC_IO = 3.3 V, no level
 * shifting), so GPIO level maps straight to the driver pin with no inversion.
 */
#include "motors_internal.h"

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t motors_hw_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << TMC_DIR_GPIO) | (1ULL << TMC_EN_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    motors_hw_set_enabled(true);
    return ESP_OK;
}

void motors_hw_set_direction(MotorDirection direction) {
    /* Direct connection: HIGH = positive, LOW = negative. The physical
     * sign is arbitrary — swap motor wires to flip it. */
    gpio_set_level(TMC_DIR_GPIO, direction == MOTOR_DIRECTION_POSITIVE ? 1 : 0);
}

void motors_hw_set_enabled(bool enabled) {
    /* EN is active-low: LOW = enabled, HIGH = disabled. */
    gpio_set_level(TMC_EN_GPIO, enabled ? 0 : 1);
}
