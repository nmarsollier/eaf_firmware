/* tmc_internal.h — internal API shared among TMC module files
 *
 * Functions and variables exposed here are for use ONLY within the
 * main/tmc/ module. No file outside tmc/ should include this header.
 */

#pragma once

#include <stdint.h>

#include "tmc.h"

/*
 * Mark the module initialised after the driver was configured and verified.
 * Called by tmc_init.c: tmc_init_driver() on success, tmc2209_hw_init() on failure.
 */
void tmc2209_set_initialized(bool initialized);

/*
 * Init diagnostics — written by tmc_init.c as the driver is configured.
 * Read by tmc_status.c for the public getters.
 */
extern TmcStatus tmc_status;
extern TmcError  tmc_error;
