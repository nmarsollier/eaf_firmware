/* Fan — fan.c — Noctua NF-A8 PWM + 12 V relay, exposed as an Alpaca Switch.
 *
 * Drives the fan speed input on FAN_PWM_GPIO (25 kHz LEDC) and the relay on
 * FAN_RELAY_GPIO. The LEDC output is inverted in hardware because the MOSFET
 * that drives the fan's open-collector input inverts too: writing a duty of
 * N % makes the fan see N %.
 */
#include "fan.h"

#include <stdint.h>

#include "config.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "FAN";

#define FAN_LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define FAN_LEDC_TIMER      LEDC_TIMER_2
#define FAN_LEDC_CHANNEL    LEDC_CHANNEL_2
#define FAN_PWM_FREQ_HZ     25000
#define FAN_PWM_DUTY_RES    LEDC_TIMER_8_BIT
#define FAN_PWM_MAX_DUTY    255

static bool s_power = false;
static uint8_t s_speed = 0;

static void fan_apply_pwm(uint8_t pct) {
    uint32_t duty = (uint32_t)pct * FAN_PWM_MAX_DUTY / 100;
    ledc_set_duty(FAN_LEDC_SPEED_MODE, FAN_LEDC_CHANNEL, duty);
    ledc_update_duty(FAN_LEDC_SPEED_MODE, FAN_LEDC_CHANNEL);
}

static void fan_apply_relay(bool on) {
    gpio_set_level(FAN_RELAY_GPIO, on ? FAN_RELAY_ACTIVE_LEVEL : !FAN_RELAY_ACTIVE_LEVEL);
}

void fan_init(void) {
    gpio_config_t relay = {
        .pin_bit_mask = 1ULL << FAN_RELAY_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&relay));

    ledc_timer_config_t timer = {
        .speed_mode = FAN_LEDC_SPEED_MODE,
        .duty_resolution = FAN_PWM_DUTY_RES,
        .timer_num = FAN_LEDC_TIMER,
        .freq_hz = FAN_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t chan = {
        .gpio_num = FAN_PWM_GPIO,
        .speed_mode = FAN_LEDC_SPEED_MODE,
        .channel = FAN_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = FAN_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = { .output_invert = 1 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan));

    /* Power off, speed 0 — the fan stays stopped until commanded. */
    fan_apply_relay(false);
    fan_apply_pwm(0);

    ESP_LOGI(TAG, "PWM GPIO %d @ 25 kHz, relay GPIO %d (active-%s)",
             (int)FAN_PWM_GPIO, (int)FAN_RELAY_GPIO,
             FAN_RELAY_ACTIVE_LEVEL ? "high" : "low");
}

void fan_set_power(bool on) {
    s_power = on;
    fan_apply_relay(on);
}

bool fan_get_power(void) {
    return s_power;
}

void fan_set_speed(uint8_t pct) {
    if (pct > 100) {
        pct = 100;
    }
    s_speed = pct;
    fan_apply_pwm(pct);
}

uint8_t fan_get_speed(void) {
    return s_speed;
}

int fan_get_rpm(void) {
    return -1;  /* tachometer not wired yet */
}
