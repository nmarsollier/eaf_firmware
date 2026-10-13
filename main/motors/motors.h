#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/*
 * Focuser motor subsystem — the single stepper axis driven by the TMC2209.
 *
 * Position is an int64_t absolute STEP-pulse counter (8 per full step,
 * TMC MRES=8). The exact steps-per-mm mapping is unknown and calibrated
 * later; the module only reasons in steps.
 */

typedef enum {
    /* Ready to accept a move command. */
    MOTORS_STATUS_READY,
    /* Executing a move to a target position. */
    MOTORS_STATUS_MOVING,
    /* Unrecoverable hardware error (TMC init failed). Only reboot clears it. */
    MOTORS_STATUS_ERROR
} MotorsStatus;

typedef enum {
    MOTOR_OK = 0,
    MOTOR_ERR_HARDWARE_ERROR,
    MOTOR_ERR_BUSY
} MotorResultCode;

/*
 * Authoritative snapshot of the motors subsystem. Position is read under a
 * spinlock so a cross-core reader never sees a torn int64_t.
 */
typedef struct {
    int64_t position;      /* absolute STEP-pulse counter */
    MotorsStatus status;
} MotorsState;

/*
 * Initialize the motors subsystem — STEP/DIR GPIO, RMT, command queue,
 * motion task and the focuser status LED. Returns ESP_OK on success;
 * on failure the subsystem is left in ERROR state.
 */
esp_err_t motors_init(void);

/*
 * Put the motors subsystem into the unrecoverable ERROR state: aborts any
 * in-flight motion and sets MOTORS_STATUS_ERROR.
 */
void motors_enter_error_state(void);

/*
 * Update the motors' view of the external 12 V rail. When power is lost the
 * motors enter ERROR and reject all motion; when power returns they recover
 * to READY (only if the error was caused by the power loss).
 */
void motors_update_power(bool power_ok);

/* Return a snapshot copy of the current MotorsState. */
MotorsState motors_current_state(void);

/* Move to an absolute position. Returns MOTOR_ERR_HARDWARE_ERROR in ERROR. */
MotorResultCode motors_move_to(int64_t position);

/* Move by a relative number of steps. Returns MOTOR_ERR_HARDWARE_ERROR in ERROR. */
MotorResultCode motors_move_by(int64_t steps);

/* Stop the current motion and return to READY. */
MotorResultCode motors_stop(void);

const char *motors_status_to_string(MotorsStatus status);

/* True if `status` is the fatal error state (motor fault). */
bool motors_status_is_error(MotorsStatus status);

/* True if a latched hardware fault occurred (e.g. TMC init failed with power
 * present). A power-loss ERROR does not set this — use it to tell a real
 * driver fault apart from a missing 12 V rail. */
bool motors_has_hardware_fault(void);
