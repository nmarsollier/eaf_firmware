/*
 * tmc.h — TMC2209 driver public API.
 *
 * The microstep configuration (MICROSTEPS) lives in config.h.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/*
 * Initialize UART, configure GCONF/IHOLD_IRUN/CHOPCONF on the focuser
 * driver, enable it (EN low) and verify that the hardware latched the
 * requested microsteps.
 *
 * Must be called before any step/direction motion is started.
 */
esp_err_t tmc2209_hw_init(void);

/*
 * Probe the driver over UART: return true when it answers a register read.
 * This doubles as the external-power sense — the driver is fed by VM (12 V),
 * so it is silent when the rail is absent and its logic supply collapses.
 */
bool tmc2209_probe(void);

/*
 * Re-configure the driver registers after it stops answering and then answers
 * again (power returned): its registers were lost while unpowered, so they are
 * rewritten. Does not re-install the UART.
 */
esp_err_t tmc2209_reinit(void);

/*
 * Query whether the TMC2209 UART and driver were initialised successfully.
 * Used for error-state LED signalling.
 */
bool tmc2209_is_initialized(void);

/* --------------------------------------------------------------------------
 * Init diagnostics — status and failure reason.
 * -------------------------------------------------------------------------- */

/* Init status. */
typedef enum {
    TMC_NOT_INIT = 0,   /* init not run / not reached */
    TMC_OK,             /* configured and verified */
    TMC_ERROR,          /* failed — see tmc2209_get_error() */
} TmcStatus;

/* Failure reason (valid when status == TMC_ERROR). */
typedef enum {
    TMC_ERROR_NONE = 0,
    TMC_ERROR_UART,            /* uart_driver_install / param_config / set_pin */
    TMC_ERROR_GCONF_WRITE,     /* GCONF write + readback */
    TMC_ERROR_IHOLD_WRITE,     /* IHOLD_IRUN write (IFCNT verify) */
    TMC_ERROR_CHOPCONF_VERIFY, /* CHOPCONF write + readback / MRES mismatch */
    TMC_ERROR_TPWMTHRS_WRITE,  /* TPWMTHRS write (IFCNT verify) */
    TMC_ERROR_PWMCONF_VERIFY   /* PWMCONF write + readback */
} TmcError;

TmcStatus tmc2209_get_status(void);
TmcError  tmc2209_get_error(void);

const char *tmc2209_status_to_string(TmcStatus status);
const char *tmc2209_error_to_string(TmcError error);
