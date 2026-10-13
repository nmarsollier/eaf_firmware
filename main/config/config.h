#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/adc_types.h"

/*
 * Focuser hardware configuration.
 *
 * Single source of truth for the build-time constants that vary between
 * devices: pin assignments, motor resolution and mechanical reduction.
 * To adapt the firmware to a different focuser, edit this file only —
 * no other module may hardcode these values.
 */

/* =========================================================================
 * Focuser identity.
 * ========================================================================= */

/* Focuser model identifier — reported in the serial protocol `capabilities`. */
#define DEVICE_NAME "NRM-EAF" /* TODO: confirm final name */

/* =========================================================================
 * Pin assignments
 * ========================================================================= */

/* Board status LED — direct GPIO, LEDC PWM (active-high: anode via 100 Ω). */
#define BOARD_LED_GPIO          GPIO_NUM_21

/* Focuser status LED — reflects the TMC/focuser state. */
#define FOCUSER_LED_GPIO        GPIO_NUM_47

/* External 12 V rail detection — GPIO 17 (ADC2_CH6). */
#define BOARD_POWER_ADC_UNIT    ADC_UNIT_2
#define BOARD_POWER_ADC_CHANNEL ADC_CHANNEL_6

/* I2C bus — shared by the ADXL345 (vibration) and the SHT31 (environment). */
#define I2C_SDA_GPIO    GPIO_NUM_5
#define I2C_SCL_GPIO    GPIO_NUM_4

/* TMC2209 driver — single focuser axis, single-wire UART. */
#define TMC_UART_NUM    UART_NUM_1
#define TMC_EN_GPIO     GPIO_NUM_9     /* EN — LOW = enabled */
#define TMC_TX_GPIO     GPIO_NUM_18    /* ESP TX → 1 kΩ → PDN_UART */
#define TMC_RX_GPIO     GPIO_NUM_8     /* ESP RX → PDN_UART (direct) */
#define TMC_STEP_GPIO   GPIO_NUM_16    /* STEP — focuser motion */
#define TMC_DIR_GPIO    GPIO_NUM_15    /* DIR — focuser motion */

/* Primary-mirror fan — Noctua NF-A8 PWM + 12 V relay.
 * PWM drives the fan speed input (25 kHz) through an open-drain N-MOSFET;
 * the relay switches the fan's 12 V supply. */
#define FAN_PWM_GPIO           GPIO_NUM_14
#define FAN_RELAY_GPIO         GPIO_NUM_13
/* Fan status LED — brightness follows fan speed (off when unpowered).
 * GPIO 42 (JTAG MTMS): free on this board — JTAG debug goes over USB-Serial/JTAG.
 * Avoid 48 = RGB LED, 38 = builtin LED, 19/20 = USB, 0/45/46 = strap. */
#define FAN_LED_GPIO           GPIO_NUM_35
/* Relay coil polarity: 1 = active-high module, 0 = active-low module. */
#define FAN_RELAY_ACTIVE_LEVEL 1

/* =========================================================================
 * Focuser motion profile (tunable).
 * ========================================================================= */

/* Cruise step rate (steps/s). Position is counted in STEP pulses
 * (16 per full step, TMC MRES=16). Exact steps/mm is TBD — calibrate later. */
#define MOTORS_MAX_SPEED_HZ     1000

/* Step rate at the start/end of the acceleration ramp (steps/s). */
#define MOTORS_START_SPEED_HZ   200

/* Acceleration (steps/s²). Ramp length = (max² − start²) / (2·accel). */
#define MOTORS_ACCEL_HZ         2000

/* =========================================================================
 * Driver / sensor configuration.
 * ========================================================================= */

/* TMC2209 microstep resolution (STEP pulses per full step). */
#define MICROSTEPS      16

/* ADXL345 I2C address (SDO tied to GND). */
#define ADXL345_ADDR    0x53
