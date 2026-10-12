/* Motors - motors_hw.c
 *
 * Purpose: DIR GPIO control for the TMC2209 stepper driver.
 *
 * STEP pulse generation is handled by the RMT peripheral (motors_rmt.c).
 * EN is driven LOW by the TMC module (tmc_init.c). DIR and STEP connect
 * directly to the TMC2209 (VCC_IO = 3.3 V, no level shifting), so GPIO
 * level maps straight to the driver pin with no inversion.
 */
#include "motors_internal.h"

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t motors_hw_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << TMC_DIR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&config);
}

void motors_hw_set_direction(MotorDirection direction) {
    /* Direct connection: HIGH = positive, LOW = negative. The physical
     * sign is arbitrary — swap motor wires to flip it. */
    gpio_set_level(TMC_DIR_GPIO, direction == MOTOR_DIRECTION_POSITIVE ? 1 : 0);
}
