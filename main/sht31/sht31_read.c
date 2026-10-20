/* Sht31 — sht31_read.c
 *
 * Purpose: run one single-shot measurement and convert the raw values.
 *
 * The command 0x2C 0x06 starts a high-repeatability measurement with clock
 * stretching; the sensor holds SCL low while measuring, so the receive below
 * blocks until the 6 result bytes are ready. Each 16-bit value is followed by
 * a CRC-8 (poly 0x31) over its two bytes.
 */
#include "sht31_internal.h"

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

esp_err_t sht31_read(float *temp_c, float *humidity_pct) {
    if (temp_c == NULL || humidity_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t cmd[2] = { SHT31_CMD_MEASURE_HIGH_CS_MSB, SHT31_CMD_MEASURE_HIGH_CS_LSB };
    uint8_t data[6] = { 0 };

    esp_err_t err = i2c_master_transmit_receive(sht31_sensor.dev_handle, cmd, 2,
                                                data, 6, SHT31_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    /* Verify the CRC that follows each 16-bit value. */
    if (crc8(data, 2) != data[2] || crc8(&data[3], 2) != data[5]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t raw_t = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_h = (uint16_t)((data[3] << 8) | data[4]);

    *temp_c       = -45.0f + 175.0f * raw_t / 65535.0f;
    *humidity_pct = 100.0f * raw_h / 65535.0f;

    return ESP_OK;
}
