/* Motors - motors_rmt.c
 *
 * Purpose: RMT step pulse generation for the single focuser axis.
 *
 * One FIFO RMT TX channel drives the STEP pin. Each symbol is a HIGH pulse
 * (rising edge = one step) followed by an idle LOW spanning the rest of the
 * step period. The motion task varies the period between batches to build
 * the acceleration ramp; transmission is synchronous (block until done).
 *
 * RMT resolution: 2 MHz (0.5 µs/tick). Much simpler than the mount: no DMA,
 * no double-buffering, no channel recreation.
 */
#include "motors_internal.h"

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/semphr.h"

/* RMT symbol max duration per half — 15-bit field. */
#define RMT_DURATION_MAX  32767U

static const char *TAG = "MOTORS_RMT";

static rmt_channel_handle_t s_channel;
static rmt_encoder_handle_t s_encoder;
static SemaphoreHandle_t s_done_sem;

static bool IRAM_ATTR on_tx_done(rmt_channel_handle_t channel,
                                 const rmt_tx_done_event_data_t *edata,
                                 void *user_data)
{
    (void) channel;
    (void) edata;
    (void) user_data;
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(s_done_sem, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}

esp_err_t motors_rmt_init(void) {
    rmt_tx_channel_config_t tx_config = {
        .gpio_num = TMC_STEP_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = RMT_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = 1,
        .intr_priority = 3,
    };

    esp_err_t err = rmt_new_tx_channel(&tx_config, &s_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_tx_channel: %s", esp_err_to_name(err));
        return err;
    }

    rmt_copy_encoder_config_t enc_config = {};
    err = rmt_new_copy_encoder(&enc_config, &s_encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_copy_encoder: %s", esp_err_to_name(err));
        rmt_del_channel(s_channel);
        s_channel = NULL;
        return err;
    }

    rmt_tx_event_callbacks_t cbs = {
        .on_trans_done = on_tx_done,
    };
    err = rmt_tx_register_event_callbacks(s_channel, &cbs, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_tx_register_event_callbacks: %s", esp_err_to_name(err));
        rmt_del_encoder(s_encoder);
        s_encoder = NULL;
        rmt_del_channel(s_channel);
        s_channel = NULL;
        return err;
    }

    s_done_sem = xSemaphoreCreateBinary();
    if (s_done_sem == NULL) {
        ESP_LOGE(TAG, "xSemaphoreCreateBinary failed");
        rmt_del_encoder(s_encoder);
        s_encoder = NULL;
        rmt_del_channel(s_channel);
        s_channel = NULL;
        return ESP_ERR_NO_MEM;
    }

    err = rmt_enable(s_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_enable: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/*
 * Encode `count` uniform-period steps, one symbol per step. Each symbol is
 * STEP_PULSE_TICKS HIGH followed by (period − pulse) LOW.
 *
 * The idle portion is clamped to RMT_DURATION_MAX for safety; at focuser
 * speeds (period ≤ ~10 000 ticks) it always fits in a single symbol.
 */
uint32_t motors_rmt_encode_steps(rmt_symbol_word_t *symbols,
                                 uint32_t max_symbols,
                                 uint32_t period_ticks,
                                 uint32_t count)
{
    if (period_ticks < STEP_MIN_PERIOD_TICKS) {
        period_ticks = STEP_MIN_PERIOD_TICKS;
    }

    uint32_t idle = period_ticks - STEP_PULSE_TICKS;
    if (idle > RMT_DURATION_MAX) {
        idle = RMT_DURATION_MAX;
    }

    uint32_t total = 0;
    for (uint32_t i = 0; i < count && total < max_symbols; i++) {
        symbols[total].level0 = 1;
        symbols[total].duration0 = STEP_PULSE_TICKS;
        symbols[total].level1 = 0;
        symbols[total].duration1 = idle;
        total++;
    }
    return total;
}

/*
 * Transmit `count` symbols and block until the RMT peripheral signals
 * completion. Drains the done semaphore first to clear any stale count.
 */
esp_err_t motors_rmt_transmit(const rmt_symbol_word_t *symbols, uint32_t count)
{
    while (xSemaphoreTake(s_done_sem, 0) == pdTRUE) { }

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };

    size_t payload_bytes = count * sizeof(rmt_symbol_word_t);
    esp_err_t err = rmt_transmit(s_channel, s_encoder, symbols, payload_bytes, &tx_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_transmit: %s", esp_err_to_name(err));
        return err;
    }

    if (xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGW(TAG, "transmit done timeout");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}
