/* Led — led_update.c
 *
 * Purpose: periodic LED state decision — the single public entry point for all
 * LED state changes.
 *
 * Called from the runtime loop every ~200 ms. Inspects the motors state to pick
 * the correct LED state:
 *
 *   1. Motor error (power loss or driver fault) -> beacon (blink-blink-pause)
 *   2. Otherwise                                 -> green (normal)
 *
 * No other module calls led_set_state() directly. This keeps all LED logic
 * cohesive in one place.
 */
#include "led_internal.h"

#include "motors.h"

void led_update(void) {
    MotorsState ms = motors_current_state();

    bool fault = motors_status_is_error(ms.status);

    led_set_state(fault ? LED_STATE_ERROR : LED_STATE_NORMAL);
}
