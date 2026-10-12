/* Motors - motors_init.c
 *
 * Purpose: initialize the motors module — state, hardware, queue, motion
 * task and the focuser status LED.
 */
#include "motors.h"
#include "motors_internal.h"
#include "board.h"
#include "tmc.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "MOTORS_INIT";

/*
 * Default motors state — position zero (home), ready.
 */
MotorsState motors_state = {
    .position = 0,
    .status = MOTORS_STATUS_READY,
};

/* Spinlock guarding the int64_t position counter and status. */
portMUX_TYPE motors_state_lock = portMUX_INITIALIZER_UNLOCKED;

esp_err_t motors_init(void) {
    esp_err_t err = motors_hw_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "motors_hw_init: %s", esp_err_to_name(err));
        motors_enter_hardware_fault();
        return err;
    }

    err = motors_rmt_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "motors_rmt_init: %s", esp_err_to_name(err));
        motors_enter_hardware_fault();
        return err;
    }

    err = motors_queue_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "motors_queue_init: %s", esp_err_to_name(err));
        motors_enter_hardware_fault();
        return err;
    }

    err = motors_motion_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "motors_motion_task_init: %s", esp_err_to_name(err));
        motors_enter_hardware_fault();
        return err;
    }

    /* The TMC2209 must have been configured (tmc2209_hw_init) before this
     * point. Missing with power present is an unrecoverable hardware fault;
     * missing with power off is a recoverable power fault — re-initialise the
     * driver when power returns. */
    if (!tmc2209_is_initialized()) {
        if (board_has_external_power()) {
            ESP_LOGE(TAG, "TMC2209 not initialised with power present — hardware fault");
            motors_enter_hardware_fault();
        } else {
            ESP_LOGI(TAG, "TMC2209 not initialised (no power) — will re-init on power-on");
            motors_enter_error_state();
            motors_mark_power_fault();
        }
    }

    return ESP_OK;
}
