/* Runtime - runtime_loop.c
 *
 * Purpose: run the periodic board cycle for the buttons and status LEDs.
 */
#include "runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buttons.h"
#include "led.h"
#include "motors.h"
#include "tmc.h"
#if CONFIG_TINYUSB_CDC_ENABLED
#include "proto.h"
#endif

#define MAIN_LOOP_PERIOD_MS     200
#define MAIN_TASK_STACK_SIZE    4096
#define MAIN_TASK_PRIORITY      5

/*
 * Business use case: orchestrate the board's periodic operational cycle.
 *
 * Objective: keep input polling and status LEDs running at a stable cadence.
 * Motor motion runs in its own task (motors_motion_task).
 */
static void main_loop_task(void *arg) {
    (void) arg;
    const TickType_t period = pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS);
    unsigned int tick = 0;

    while (true) {
        /* Every ~1 s, probe the TMC2209 over UART as the external-power sense
         * and feed the result to the motors power gating. */
        if (++tick >= 5) {
            tick = 0;
            motors_update_power(tmc2209_probe());
        }

        buttons_update();
#if CONFIG_TINYUSB_CDC_ENABLED
        proto_poll();
#endif
        led_update();
        vTaskDelay(period);
    }
}

/*
 * Business use case: start the main runtime in the background.
 *
 * Objective: delegate continuous operation to a dedicated FreeRTOS task.
 */
void setup_runtime_start(void) {
    xTaskCreatePinnedToCore(
        main_loop_task,
        "main_loop",
        MAIN_TASK_STACK_SIZE,
        NULL,
        MAIN_TASK_PRIORITY,
        NULL,
        1);  /* CPU 1 — keep sensor I/O (ADC/I2C) off CPU 0 so a stuck read
              * never blocks the RX task (CPU 0, prio 19). */
}
