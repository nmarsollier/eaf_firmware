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
 * Re-configure the driver registers after the 12 V rail returns. The UART
 * peripheral (on the ESP) stays powered, so only the TMC2209's registers —
 * lost while the driver was unpowered — need to be rewritten. Does not
 * re-install the UART.
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
