/* Led — led_update.c
 *
 * Purpose: periodic LED state decision — the single public entry point for
 * all LED state changes.
 *
 * The board LED reflects the external power rail; the focuser LED reflects
 * the motors state. No other module calls led_set_state() directly.
 */
#include "led_internal.h"

#include "accelerometer.h"
#include "board.h"
#include "fan.h"
#include "motors.h"

void led_update(void) {
    /* Board LED: dim only when 12 V is present and the accelerometer (I2C)
     * answers; otherwise beacon (error). */
    if (board_has_external_power() && accelerometer_is_present()) {
        led_set_state(LED_ID_BOARD, LED_STATE_DIM);
    } else {
        led_set_state(LED_ID_BOARD, LED_STATE_BEACON);
    }

    /* Focuser LED: off when 12 V is missing (a board-level error shown only by
     * the board LED), beacon on a TMC driver fault, bright while moving, dim
     * when idle. */
    MotorsState ms = motors_current_state();
    if (!board_has_external_power()) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_OFF);
    } else if (motors_status_is_error(ms.status) && motors_has_hardware_fault()) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_BEACON);
    } else if (ms.status == MOTORS_STATUS_MOVING) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_BRIGHT);
    } else {
        led_set_state(LED_ID_FOCUSER, LED_STATE_DIM);
    }

    /* Fan LED: brightness follows fan intensity, but only when 12 V is present
     * (the fan cannot run without it). */
    uint32_t fan_duty = 0;
    if (board_has_external_power() && fan_get_power()) {
        fan_duty = (uint32_t)fan_get_intensity_pct() * LED_BRIGHT_DUTY / 100;
    }
    led_set_duty(LED_ID_FAN, fan_duty);
}
