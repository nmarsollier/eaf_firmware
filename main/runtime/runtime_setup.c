/* Runtime - runtime_setup.c
 *
 * Purpose: initialize the subsystems needed before the runtime loop starts.
 */
#include "runtime.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "board.h"
#include "buttons.h"
#include "led.h"
#include "motors.h"
#include "tmc.h"
#include "usb_cdc.h"

static const char *TAG = "RUNTIME_SETUP";

/*
 * Business use case: prepare the focuser for operation.
 *
 * Objective: bring the core services, board peripherals, motor subsystem and
 * the USB transport online so the focuser starts in a usable state.
 *
 * LED state is managed exclusively by board_update() / motors_led_update()
 * in the runtime loop — no explicit set_state calls here.
 */
void setup_init(void) {
    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    board_init();
    led_init();

    /* Configure the TMC2209 (UART + EN). Logs its own failure. */
    tmc2209_hw_init();

    esp_err_t motors_err = motors_init();
    if (motors_err != ESP_OK) {
        ESP_LOGE(TAG, "motors_init failed: %s — focuser in ERROR state",
                 esp_err_to_name(motors_err));
    }

    buttons_init();

    /* USB CDC-ACM transport — the protocol is wired to it by proto_init(). */
    esp_err_t cdc_result = usb_cdc_init();
    if (cdc_result != ESP_OK) {
        ESP_LOGW(TAG, "USB CDC init failed: %s", esp_err_to_name(cdc_result));
    }

    ESP_LOGI(TAG, "Board ready");
}
