/* Motors - motors_error.c
 *
 * Purpose: error-state handling. The ERROR state is unrecoverable and always
 * aborts any in-flight motion.
 */
#include "motors_internal.h"

/* Latched hardware fault — set by a boot-time init failure. Only a reboot
 * clears it. */
static bool s_hardware_fault = false;

void motors_enter_error_state(void) {
    /* Abort any in-flight or queued motion. */
    motors_stop_generation++;
    motors_queue_clear();

    motors_transition_error();
}

void motors_enter_hardware_fault(void) {
    s_hardware_fault = true;
    motors_enter_error_state();
}

bool motors_has_hardware_fault(void) {
    return s_hardware_fault;
}
