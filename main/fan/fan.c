/* Fan — fan.c — mock fan subsystem.
 *
 * Returns fixed, coherent values per mode. Replaced by the real Noctua PWM +
 * relay control when that functionality is ported.
 */
#include "fan.h"

#include <string.h>

static const char *s_mode = "normal";

void fan_init(void) {
    s_mode = "normal";
}

const char *fan_get_mode(void) {
    return s_mode;
}

bool fan_set_mode(const char *mode) {
    if (strcmp(mode, "normal") == 0 || strcmp(mode, "soft") == 0 || strcmp(mode, "off") == 0) {
        s_mode = mode;
        return true;
    }
    return false;
}

int fan_get_output_pwm_pct(void) {
    if (strcmp(s_mode, "normal") == 0) return 100;
    if (strcmp(s_mode, "soft") == 0) return 50;
    return 0;
}

bool fan_get_relay_command(void) {
    return strcmp(s_mode, "off") != 0;
}
