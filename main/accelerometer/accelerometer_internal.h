/* accelerometer_internal.h — internal API shared among the accelerometer files. */

#pragma once

#include <stdint.h>

#include "accelerometer.h"
#include "config.h"
#include "driver/i2c_master.h"

/* ── I2C hardware configuration ───────────────────────────── */

#define ACCEL_I2C_PORT    I2C_NUM_0
#define ACCEL_CLK_HZ      100000     /* 100 kHz standard mode */
#define ACCEL_TIMEOUT_MS  100

/* ── ADXL345 registers ────────────────────────────────────── */

#define ADXL345_REG_DEVID         0x00
#define ADXL345_DEVID_EXPECTED    0xE5
#define ADXL345_REG_DATA_FORMAT   0x31
#define ADXL345_REG_POWER_CTL     0x2D
#define ADXL345_POWER_CTL_MEASURE 0x08
#define ADXL345_REG_DATAX0        0x32

/* ±2 g, 10-bit: 256 LSB per g. */
#define ADXL345_LSB_PER_G         256.0f

/* ── Sensor state ─────────────────────────────────────────── */

typedef struct {
    uint8_t address;
    i2c_master_dev_handle_t dev_handle;
    bool present;
} AccelSensor;

extern AccelSensor accel_sensor;

esp_err_t accelerometer_read_register(const AccelSensor *sensor, uint8_t reg, uint8_t *value);
esp_err_t accelerometer_write_register(const AccelSensor *sensor, uint8_t reg, uint8_t value);
