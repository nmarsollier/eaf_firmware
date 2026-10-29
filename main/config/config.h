#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/adc_types.h"

/*
 * EAF hardware configuration.
 */

/* Focuser model identifier — reported in the serial protocol `capabilities`. */
#define DEVICE_NAME "EAF"

/* =========================================================================
 * Pin assignments
 * ========================================================================= */

/* Board status LED — direct GPIO, LEDC PWM (active-high: anode via 100 Ω). */
#define BOARD_LED_GPIO          GPIO_NUM_35

/* Focuser status LED — reflects the TMC/focuser state. */
#define FOCUSER_LED_GPIO        GPIO_NUM_37

/* External 12 V rail detection — GPIO 17 (ADC2_CH6). */
#define BOARD_POWER_ADC_UNIT    ADC_UNIT_2
#define BOARD_POWER_ADC_CHANNEL ADC_CHANNEL_6

/* TMC2209 driver — single focuser axis, single-wire UART. */
#define TMC_UART_NUM    UART_NUM_1
#define TMC_EN_GPIO     GPIO_NUM_9     /* EN — LOW = enabled */
#define TMC_TX_GPIO     GPIO_NUM_18    /* ESP TX → 1 kΩ → PDN_UART */
#define TMC_RX_GPIO     GPIO_NUM_8     /* ESP RX → PDN_UART (direct) */
#define TMC_STEP_GPIO   GPIO_NUM_16    /* STEP — focuser motion */
#define TMC_DIR_GPIO    GPIO_NUM_15    /* DIR — focuser motion */
#define MICROSTEPS      16

/* =========================================================================
 * Local buttons — push-to-GND (active-low, internal pull-ups).
 * ========================================================================= */

/* Focuser enable/disable toggle. */
#define BUTTON_FOCUSER_ENABLE_GPIO  GPIO_NUM_1

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
