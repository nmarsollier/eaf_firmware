/* Motors - motors_init.c
 *
 * Purpose: initialize the motors module — state, hardware, queue, motion
 * task and the focuser status LED.
 */
#include "motors.h"
#include "motors_internal.h"
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

/* Motion-settled semaphore — created in motors_init(), waited on by motors_stop(). */
SemaphoreHandle_t motors_idle_sem = NULL;

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

    motors_idle_sem = xSemaphoreCreateBinary();
    if (motors_idle_sem == NULL) {
        ESP_LOGE(TAG, "xSemaphoreCreateBinary failed");
        motors_enter_hardware_fault();
        return ESP_ERR_NO_MEM;
    }

    err = motors_motion_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "motors_motion_task_init: %s", esp_err_to_name(err));
        motors_enter_hardware_fault();
        return err;
    }

    /* The TMC2209 must have been configured (tmc2209_hw_init) before this
     * point; a missing driver is an unrecoverable hardware fault. */
    if (!tmc2209_is_initialized()) {
        ESP_LOGE(TAG, "TMC2209 not initialised — hardware fault");
        motors_enter_hardware_fault();
    }

    return ESP_OK;
}
