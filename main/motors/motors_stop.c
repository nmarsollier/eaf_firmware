/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement. Clears the command queue, invalidates
 * in-flight motion via the stop generation, then waits for the motion task to
 * settle — which brings the focuser back to READY with the final counter.
 */
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_STOP";

MotorResultCode motors_stop(void) {
    /* ERROR is unrecoverable — never transition out of it. */
    if (motors_status_is_error(motors_state.status)) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    motors_stop_generation++;
    motors_queue_clear();

    /* Wait for the motion task to settle. The idle token is held while the
     * task is mid-move and given back (with a READY transition) when it
     * settles, so taking it — unconditionally — blocks exactly until the task
     * is idle, even if the STOP landed just as a move was starting. The RMT
     * transmit timeout is 5 s, so wait a little longer to guarantee the settle. */
    if (xSemaphoreTake(motors_idle_sem, pdMS_TO_TICKS(5500)) != pdTRUE) {
        ESP_LOGE(TAG, "motion task did not settle within 5.5 s");
        motors_enter_hardware_fault();
        return MOTOR_ERR_HARDWARE_ERROR;
    }
    xSemaphoreGive(motors_idle_sem);

    /* A queued command was cleared above and the in-flight move (if any) has
     * settled, so the axis returns to READY — unless a fault latched during
     * settle. A DISABLED axis stays DISABLED (stop never re-enables it). */
    if (motors_status_is_error(motors_state.status)) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }
    motors_transition_settle();

    return MOTOR_OK;
}
