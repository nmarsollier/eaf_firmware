#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/adc_types.h"

/*
 * EAF hardware configuration.
 */

/* Focuser model identifier — reported in the serial protocol `capabilities`. */
#define DEVICE_NAME "NRM-EAF"

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

/* ADXL345 I2C address (SDO tied to GND). */
#define ADXL345_ADDR    0x53

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
#define MICROSTEPS      16

/* Primary-mirror fan — Noctua NF-A8 PWM + 12 V relay.
 * PWM drives the fan speed input (25 kHz) through an open-drain N-MOSFET;
 * the relay switches the fan's 12 V supply. */
#define FAN_PWM_GPIO           GPIO_NUM_14
#define FAN_RELAY_GPIO         GPIO_NUM_13
#define FAN_LED_GPIO           GPIO_NUM_35
#define FAN_RELAY_ACTIVE_LEVEL 1
#define FAN_SPEED_MAX         100    /* user speed at full duty (2200 RPM) */
#define FAN_SPEED_MIN         1      /* user speed at FAN_MIN_PWM_PCT (≈450 RPM) */
#define FAN_MIN_PWM_PCT       20     /* Noctua minimum duty (≈450 RPM) */
#define FAN_MAX_PWM_PCT       100    /* full duty (2200 RPM) */
#define FAN_RPM_MIN           450    /* NF-A8 PWM at FAN_MIN_PWM_PCT */
#define FAN_RPM_MAX           2200   /* NF-A8 PWM at FAN_MAX_PWM_PCT */
#define FAN_AUTO_THRESHOLD_C   1.0f  /* delta that switches the fan to max speed */
#define FAN_AUTO_HYSTERESIS_C  0.1f  /* half-band: max at T+h, min at T−h */

/* =========================================================================
 * Local buttons — push-to-GND (active-low, internal pull-ups).
 * ========================================================================= */

/* Focuser enable/disable toggle. */
#define BUTTON_FOCUSER_ENABLE_GPIO  GPIO_NUM_1

/* Fan mode cycle (off → auto → manual → off). */
#define BUTTON_FAN_MODE_GPIO        GPIO_NUM_2

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
