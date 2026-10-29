/* Buttons — buttons.c — local push button.
 *
 * One push-to-GND button (active-low): the focuser enable/disable toggle. It is
 * read on the runtime loop, debounced, and its press edge triggers the action.
 */
#include "buttons.h"

#include "config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "motors.h"

/* Samples a level change must persist before it is accepted. At the 200 ms
 * runtime-loop cadence contact bounce (a few ms) never spans a full sample, so
 * one sample is enough to reject glitches while keeping the press responsive. */
#define BUTTON_DEBOUNCE_SAMPLES 1

typedef struct {
    bool pressed;   /* current debounced level */
    uint8_t count;  /* consecutive samples differing from `pressed` */
} Button;

static Button s_focuser_button;

/* Button bridges the pin to GND, so pressed == LOW. */
static bool read_raw(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;
}

/* Advance the debouncer and report whether a press edge occurred. */
static bool debounce(Button *b, bool raw) {
    bool was_pressed = b->pressed;

    if (raw == b->pressed) {
        b->count = 0;
    } else if (++b->count >= BUTTON_DEBOUNCE_SAMPLES) {
        b->pressed = raw;
        b->count = 0;
    }

    return b->pressed && !was_pressed;
}

void buttons_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << BUTTON_FOCUSER_ENABLE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
}

void buttons_update(void) {
    bool focuser_press = debounce(&s_focuser_button,
                                  read_raw(BUTTON_FOCUSER_ENABLE_GPIO));

    if (focuser_press) {
        bool enabled = !motors_is_enabled();
        motors_set_enabled(enabled);
    }
}

bool buttons_focuser_enable_pressed(void) {
    return s_focuser_button.pressed;
}
