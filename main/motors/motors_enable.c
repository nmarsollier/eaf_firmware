/* Motors - motors_enable.c
 *
 * Purpose: focuser enable/disable. When disabled the TMC2209 EN pin is driven
 * HIGH so the motor holds no torque (the focuser can be turned by hand), and
 * motion is rejected until the focuser is re-enabled.
 */
#include "motors_internal.h"

static bool s_enabled = true;

void motors_set_enabled(bool enabled) {
    if (enabled == s_enabled) {
        return;
    }
    s_enabled = enabled;

    if (enabled) {
        motors_hw_set_enabled(true);
    } else {
        /* Settle any in-flight motion before releasing the driver. */
        motors_stop();
        motors_hw_set_enabled(false);
    }
}

bool motors_is_enabled(void) {
    return s_enabled;
}
