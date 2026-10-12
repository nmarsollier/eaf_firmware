/* Motors - motors_command_queue.c
 *
 * Purpose: FreeRTOS command queue for the motion task. Owns the queue
 * handle, lifecycle, send, and clear helpers.
 */
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_CMD_QUEUE";

/* Queue handle — shared across the motors module via motors_internal.h. */
QueueHandle_t motion_cmd_queue = NULL;

/* Stop epoch — incremented on every stop to invalidate queued commands. */
volatile uint32_t motors_stop_generation = 0;

esp_err_t motors_queue_init(void) {
    motion_cmd_queue = xQueueCreate(4, sizeof(MotionCommand));
    if (motion_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create motion command queue");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool motors_queue_put(MotionCommand *cmd) {
    if (motion_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Motion command queue not initialised");
        return false;
    }

    /* Stamp the current stop epoch so a STOP that lands after this enqueue
     * invalidates the command at execution time. */
    cmd->generation = motors_stop_generation;

    if (xQueueSendToBack(motion_cmd_queue, cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, dropping move command");
        return false;
    }
    return true;
}

void motors_queue_clear(void) {
    if (motion_cmd_queue != NULL) {
        xQueueReset(motion_cmd_queue);
    }
}
