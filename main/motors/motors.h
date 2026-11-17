#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/*
 * Focuser motor subsystem — the single stepper axis driven by the TMC2209.
 *
 * Position is an int64_t absolute STEP-pulse counter (8 per full step,
 * TMC MRES=8). It is always relative to a reference that is re-declared on
 * boot and on every disable→enable (MOTORS_REFERENCE_POSITION), and bounded by
 * soft limits MOTORS_MAX_TRAVEL_STEPS on either side of that reference.
 */

typedef enum {
    /* The driver is disabled (EN released) — no motion until re-enabled. */
    MOTORS_STATUS_DISABLED,
    /* Ready to accept a move command. */
    MOTORS_STATUS_READY,
    /* A move command is reserved and waiting for the motion task. */
    MOTORS_STATUS_QUEUED,
    /* Executing a move to a target position. */
    MOTORS_STATUS_MOVING,
    /* Unrecoverable hardware error (TMC init failed). Only reboot clears it. */
    MOTORS_STATUS_ERROR
} MotorsStatus;

typedef enum {
    MOTOR_OK = 0,
    MOTOR_ERR_HARDWARE_ERROR,
    MOTOR_ERR_BUSY,
    MOTOR_ERR_DISABLED,
    MOTOR_ERR_LIMIT
} MotorResultCode;

/*
 * Authoritative snapshot of the motors subsystem. Position is read under a
 * spinlock so a cross-core reader never sees a torn int64_t.
 */
typedef struct {
    int64_t position;      /* absolute STEP-pulse counter */
    bool counter_valid;    /* false when the reference has been lost */
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
 * Update the motors' view of the external power rail (from the TMC probe).
 * When power is lost the motors enter ERROR and reject all motion; when power
 * returns the driver is re-initialised and the motors recover to READY.
 */
void motors_update_power(bool power_ok);

/* True when the external power rail is present (cached from the last probe). */
bool motors_has_power(void);

/* Return a snapshot copy of the current MotorsState. */
MotorsState motors_current_state(void);

/* Move to an absolute position. Returns MOTOR_ERR_LIMIT if `position` is
 * outside the soft limits, MOTOR_ERR_HARDWARE_ERROR in ERROR. */
MotorResultCode motors_move_to(int64_t position);

/* Move by a relative number of steps. Returns MOTOR_ERR_LIMIT if the resulting
 * position would be outside the soft limits. */
MotorResultCode motors_move_by(int64_t steps);

/* Stop the current motion and return to READY. */
MotorResultCode motors_stop(void);

/* Soft-limit bounds: the positions the axis may not move past. */
int64_t motors_min_position(void);
int64_t motors_max_position(void);

/*
 * Enable/disable the focuser driver (TMC2209 EN pin). Disabling releases the
 * motor's holding torque so the focuser can be turned by hand, and rejects all
 * motion until re-enabled. Re-enabling re-declares the position reference, so
 * the counter restarts at MOTORS_REFERENCE_POSITION. Toggled by the local
 * enable/disable button.
 */
void motors_set_enabled(bool enabled);
bool motors_is_enabled(void);

const char *motors_status_to_string(MotorsStatus status);

/* True if `status` is the fatal error state (motor fault). */
bool motors_status_is_error(MotorsStatus status);

/* True while a move is pending or in progress (QUEUED or MOVING). */
bool motors_status_is_busy(MotorsStatus status);

/* True if a latched hardware fault occurred (e.g. TMC init failed). */
bool motors_has_hardware_fault(void);
