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

    /* Focuser LED: hardware (driver) fault → beacon, moving → bright,
     * idle or power-off → dim. A missing 12 V rail is a board-level error and
     * is shown only by the board LED. */
    MotorsState ms = motors_current_state();
    if (motors_status_is_error(ms.status) && motors_has_hardware_fault()) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_BEACON);
    } else if (ms.status == MOTORS_STATUS_MOVING) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_BRIGHT);
    } else {
        led_set_state(LED_ID_FOCUSER, LED_STATE_DIM);
    }

    /* Fan LED: brightness follows fan speed, but only when 12 V is present
     * (the fan cannot run without it). */
    uint32_t fan_duty = 0;
    if (board_has_external_power() && fan_get_power()) {
        fan_duty = (uint32_t)fan_get_speed() * LED_BRIGHT_DUTY / 100;
    }
    led_set_duty(LED_ID_FAN, fan_duty);
}
