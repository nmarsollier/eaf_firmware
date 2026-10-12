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
#include "motors.h"

void led_update(void) {
    /* Board LED: dim only when 12 V is present and the accelerometer (I2C)
     * answers; otherwise beacon (error). */
    if (board_has_external_power() && accelerometer_is_present()) {
        led_set_state(LED_ID_BOARD, LED_STATE_DIM);
    } else {
        led_set_state(LED_ID_BOARD, LED_STATE_BEACON);
    }

    /* Focuser LED: error → beacon, moving → bright, idle → dim. */
    MotorsState ms = motors_current_state();
    if (motors_status_is_error(ms.status)) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_BEACON);
    } else if (ms.status == MOTORS_STATUS_MOVING) {
        led_set_state(LED_ID_FOCUSER, LED_STATE_BRIGHT);
    } else {
        led_set_state(LED_ID_FOCUSER, LED_STATE_DIM);
    }
}
