#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"

/*
 * EAF hardware configuration.
 */

/* Focuser model identifier — reported in the serial protocol `capabilities`. */
#define DEVICE_NAME "EAF"

/* =========================================================================
 * Pin assignments
 * ========================================================================= */

/* Status LED — onboard RGB (WS2812), driven via RMT. GPIO 48 on the common
 * Super Mini (clone variants may use GPIO 21). */
#define LED_GPIO        GPIO_NUM_48

/* TMC2209 driver — single focuser axis, single-wire UART. */
#define TMC_UART_NUM    UART_NUM_1
#define TMC_EN_GPIO     GPIO_NUM_7     /* EN — LOW = enabled */
#define TMC_TX_GPIO     GPIO_NUM_6     /* ESP TX → 1 kΩ → PDN_UART */
#define TMC_RX_GPIO     GPIO_NUM_5     /* ESP RX → PDN_UART (direct) */
#define TMC_STEP_GPIO   GPIO_NUM_2     /* STEP — focuser motion */
#define TMC_DIR_GPIO    GPIO_NUM_1     /* DIR — focuser motion */
#define MICROSTEPS      16

/* =========================================================================
 * Local buttons — push-to-GND (active-low, internal pull-ups).
 * ========================================================================= */

/* Focuser enable/disable toggle. */
#define BUTTON_FOCUSER_ENABLE_GPIO  GPIO_NUM_8

/* =========================================================================
 * Focuser motion profile (tunable).
 * ========================================================================= */

/* Cruise step rate (steps/s). Position is counted in STEP pulses
 * (8 per full step, TMC MRES=8). Exact steps/mm is TBD — calibrate later. */
#define MOTORS_MAX_SPEED_HZ     1000

/* Step rate at the start/end of the acceleration ramp (steps/s). */
#define MOTORS_START_SPEED_HZ   200

/* Acceleration (steps/s²). Ramp length = (max² − start²) / (2·accel). */
#define MOTORS_ACCEL_HZ         2000
