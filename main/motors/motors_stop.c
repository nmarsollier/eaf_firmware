/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement. Clears the command queue, invalidates
 * in-flight motion via the stop generation, waits for the motion task to
 * settle, and returns to READY with the final position counter.
 */
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_STOP";

MotorResultCode motors_stop(void) {
    /* ERROR is unrecoverable — never transition out of it. */
    if (motors_status_is_error(motors_state.status)) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    bool was_moving;
    portENTER_CRITICAL(&motors_state_lock);
    was_moving = (motors_state.status == MOTORS_STATUS_MOVING);
    portEXIT_CRITICAL(&motors_state_lock);

    motors_stop_generation++;
    motors_queue_clear();

    /* Wait for the motion task to finish the in-flight RMT batch and settle,
     * so the counter is final before the caller reads it. */
    if (was_moving) {
        if (xSemaphoreTake(motors_idle_sem, pdMS_TO_TICKS(500)) != pdTRUE) {
            ESP_LOGW(TAG, "motion task did not settle within 500 ms");
        }
    }

    portENTER_CRITICAL(&motors_state_lock);
    motors_state.status = MOTORS_STATUS_READY;
    portEXIT_CRITICAL(&motors_state_lock);

    return MOTOR_OK;
}
