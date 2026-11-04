/* Proto — proto_session.c — boot identity, config revision and idempotency.
 *
 * `boot_id` is a random value drawn once per boot so a client can detect a
 * restart.  `config_rev` is persisted in NVS and bumped on every successful
 * CONFIG write so clients can detect configuration changes.  The idempotency
 * ring remembers each ACTION request's id and its original result, so a
 * retried request replays that result instead of re-executing (a rejected
 * request stays rejected).  It is cleared when the client reconnects.
 */
#include "proto_internal.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "nvs.h"

#include "freertos/FreeRTOS.h"

static const char *TAG = "PROTO_SESSION";
static const char *NVS_NAMESPACE = "proto";
static const char *NVS_CFG_REV = "cfg_rev";

#define PROTO_SEEN_IDS 8

typedef struct {
    uint32_t id;
    char response[PROTO_MAX_FRAME];   /* full response body (JSON) */
} proto_seen_entry_t;

static uint32_t s_boot_id;
static uint32_t s_config_rev;

/* Circular ring of executed action ids and their responses.  `s_seen_next` is
 * the next slot to overwrite (the oldest once the ring is full). */
static proto_seen_entry_t s_seen[PROTO_SEEN_IDS];
static uint32_t s_seen_len;
static uint32_t s_seen_next;
static portMUX_TYPE s_seen_lock = portMUX_INITIALIZER_UNLOCKED;

void proto_session_init(void) {
    s_boot_id = esp_random();

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        uint32_t rev = 0;
        if (nvs_get_u32(handle, NVS_CFG_REV, &rev) == ESP_OK) {
            s_config_rev = rev;
        }
        nvs_close(handle);
    }

    ESP_LOGI(TAG, "boot_id=%08lx config_rev=%lu",
             (unsigned long) s_boot_id, (unsigned long) s_config_rev);
}

uint32_t proto_boot_id(void) {
    return s_boot_id;
}

void proto_boot_id_str(char *out, size_t cap) {
    snprintf(out, cap, "%08lx", (unsigned long) s_boot_id);
}

uint32_t proto_config_rev(void) {
    return s_config_rev;
}

void proto_config_rev_bump(void) {
    s_config_rev++;

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u32(handle, NVS_CFG_REV, s_config_rev);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

bool proto_action_lookup(uint32_t id, char *response, size_t cap) {
    if (id == 0 || response == NULL || cap == 0) {
        return false;   /* no id / no buffer — no idempotency */
    }

    portENTER_CRITICAL(&s_seen_lock);
    for (uint32_t i = 0; i < s_seen_len; i++) {
        if (s_seen[i].id == id) {
            snprintf(response, cap, "%s", s_seen[i].response);
            portEXIT_CRITICAL(&s_seen_lock);
            return true;
        }
    }
    portEXIT_CRITICAL(&s_seen_lock);
    return false;
}

void proto_action_record(uint32_t id, const char *response) {
    if (id == 0 || response == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_seen_lock);
    /* Update an existing entry rather than duplicating it. */
    for (uint32_t i = 0; i < s_seen_len; i++) {
        if (s_seen[i].id == id) {
            snprintf(s_seen[i].response, sizeof(s_seen[i].response), "%s", response);
            portEXIT_CRITICAL(&s_seen_lock);
            return;
        }
    }

    s_seen[s_seen_next].id = id;
    snprintf(s_seen[s_seen_next].response, sizeof(s_seen[s_seen_next].response),
             "%s", response);
    s_seen_next = (s_seen_next + 1) % PROTO_SEEN_IDS;
    if (s_seen_len < PROTO_SEEN_IDS) {
        s_seen_len++;
    }
    portEXIT_CRITICAL(&s_seen_lock);
}

void proto_session_reset(void) {
    portENTER_CRITICAL(&s_seen_lock);
    s_seen_len = 0;
    s_seen_next = 0;
    portEXIT_CRITICAL(&s_seen_lock);
}
