/* Motors - motors_power.c
 *
 * Purpose: gate motor motion on the external-power rail. When the 12 V
 * switch is off (USB-only) the motors enter ERROR and stay there until
 * power returns, at which point the TMC2209 is re-initialised (its registers
 * were lost while unpowered).
 */
#include "motors_internal.h"
#include "tmc.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_POWER";

/* True when the current ERROR was caused by a power loss (recoverable),
 * as opposed to a hardware fault (unrecoverable). */
static bool s_power_fault = false;

void motors_mark_power_fault(void) {
    s_power_fault = true;
}

void motors_update_power(bool power_ok) {
    if (!power_ok) {
        if (!s_power_fault) {
            ESP_LOGW(TAG, "external power lost — motors in ERROR");
        }
        motors_enter_error_state();
        s_power_fault = true;
        return;
    }

    if (!s_power_fault) {
        return;  /* power was already on — nothing to do */
    }

    /* Power returned — re-initialise the TMC2209, whose registers were lost
     * while the driver was unpowered. */
    s_power_fault = false;

    if (motors_has_hardware_fault()) {
        ESP_LOGI(TAG, "external power restored — hardware fault persists");
        return;
    }

    if (tmc2209_reinit() == ESP_OK) {
        ESP_LOGI(TAG, "external power restored — motors back to READY");
        portENTER_CRITICAL(&motors_state_lock);
        motors_state.status = MOTORS_STATUS_READY;
        portEXIT_CRITICAL(&motors_state_lock);
    } else {
        ESP_LOGE(TAG, "external power restored but TMC re-init failed — hardware fault");
        motors_enter_hardware_fault();
    }
}
