/* Motors - motors_motion_task.c
 *
 * Purpose: FreeRTOS motion task — consumes MotionCommands from the queue
 * and drives the focuser position via RMT step pulses with a trapezoidal
 * acceleration ramp (accelerate → cruise → decelerate).
 *
 * The motion task is the sole writer of motors_state.position.
 * Status is written cooperatively by the motion task, motors_stop() and
 * the error/power paths, under motors_state_lock.
 */
#include "motors_internal.h"

#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "MOTORS_MOTION_TASK";

#define MOTION_TASK_STACK_WORDS 4096  /* 16 KB */
#define MOTION_TASK_PRIORITY    23

/* Steps per batch during the ramp — small batches keep the frequency ramp
 * smooth without hammering RMT. */
#define RAMP_BATCH 8U

/* ── Ramp helpers ──────────────────────────────────────────── */

/* Number of steps to accelerate from start to max speed. */
static uint32_t accel_step_count(void) {
    uint32_t num = MOTORS_MAX_SPEED_HZ * MOTORS_MAX_SPEED_HZ
                 - MOTORS_START_SPEED_HZ * MOTORS_START_SPEED_HZ;
    uint32_t den = 2U * MOTORS_ACCEL_HZ;
    return num / den;
}

/* Step period (RMT ticks) for a given step rate. */
static uint32_t speed_to_period_ticks(uint32_t speed_hz) {
    return (RMT_RESOLUTION_HZ + speed_hz / 2) / speed_hz;
}

/* Frequency at step `i` of an `n`-step accelerating ramp (linear in freq). */
static uint32_t ramp_speed_up(uint32_t i, uint32_t n) {
    if (n <= 1) return MOTORS_START_SPEED_HZ;
    return MOTORS_START_SPEED_HZ
         + (MOTORS_MAX_SPEED_HZ - MOTORS_START_SPEED_HZ) * i / (n - 1);
}

/* Frequency at step `i` of an `n`-step decelerating ramp (linear in freq). */
static uint32_t ramp_speed_down(uint32_t i, uint32_t n) {
    if (n <= 1) return MOTORS_START_SPEED_HZ;
    return MOTORS_MAX_SPEED_HZ
         - (MOTORS_MAX_SPEED_HZ - MOTORS_START_SPEED_HZ) * i / (n - 1);
}

/* ── State helpers ─────────────────────────────────────────── */

static int64_t read_position(void) {
    int64_t pos;
    portENTER_CRITICAL(&motors_state_lock);
    pos = motors_state.position;
    portEXIT_CRITICAL(&motors_state_lock);
    return pos;
}

static void add_position(int64_t delta) {
    portENTER_CRITICAL(&motors_state_lock);
    motors_state.position += delta;
    portEXIT_CRITICAL(&motors_state_lock);
}

static void set_status(MotorsStatus status) {
    portENTER_CRITICAL(&motors_state_lock);
    motors_state.status = status;
    portEXIT_CRITICAL(&motors_state_lock);
}

/* ── Step emission ─────────────────────────────────────────── */

/*
 * Emit `count` steps at a uniform `period_ticks`, in RMT batches of up to
 * RMT_MEM_BLOCK_SYMBOLS. Returns false if a STOP landed mid-emission.
 */
static bool emit_steps(uint32_t period_ticks, uint32_t count,
                       MotorDirection dir, uint32_t generation)
{
    rmt_symbol_word_t symbols[RMT_MEM_BLOCK_SYMBOLS];
    int64_t sign = (dir == MOTOR_DIRECTION_POSITIVE) ? 1 : -1;

    while (count > 0) {
        if (generation != motors_stop_generation) {
            return false;  /* aborted by a STOP */
        }

        uint32_t n = count < RMT_MEM_BLOCK_SYMBOLS ? count : RMT_MEM_BLOCK_SYMBOLS;
        uint32_t encoded = motors_rmt_encode_steps(symbols, RMT_MEM_BLOCK_SYMBOLS,
                                                   period_ticks, n);
        if (motors_rmt_transmit(symbols, encoded) != ESP_OK) {
            return false;
        }

        add_position(sign * (int64_t) n);
        count -= n;
    }
    return true;
}

/* ── Move execution ────────────────────────────────────────── */

static void run_move(int64_t target, uint32_t generation)
{
    int64_t distance = target - read_position();
    if (distance == 0) {
        return;
    }

    MotorDirection dir = distance > 0 ? MOTOR_DIRECTION_POSITIVE : MOTOR_DIRECTION_NEGATIVE;
    uint32_t steps = (uint32_t)(distance > 0 ? distance : -distance);

    motors_hw_set_direction(dir);
    set_status(MOTORS_STATUS_MOVING);

    uint32_t n_accel = accel_step_count();
    if (steps < 2 * n_accel) {
        n_accel = steps / 2;  /* triangular profile — no cruise */
    }

    /* Accelerate. */
    for (uint32_t done = 0; done < n_accel; done += RAMP_BATCH) {
        uint32_t n = n_accel - done < RAMP_BATCH ? n_accel - done : RAMP_BATCH;
        uint32_t speed = ramp_speed_up(done + n / 2, n_accel);
        if (!emit_steps(speed_to_period_ticks(speed), n, dir, generation)) {
            return;
        }
    }

    /* Cruise. */
    uint32_t cruise = steps - 2 * n_accel;
    if (!emit_steps(speed_to_period_ticks(MOTORS_MAX_SPEED_HZ), cruise, dir, generation)) {
        return;
    }

    /* Decelerate. */
    for (uint32_t done = 0; done < n_accel; done += RAMP_BATCH) {
        uint32_t n = n_accel - done < RAMP_BATCH ? n_accel - done : RAMP_BATCH;
        uint32_t speed = ramp_speed_down(done + n / 2, n_accel);
        if (!emit_steps(speed_to_period_ticks(speed), n, dir, generation)) {
            return;
        }
    }

    set_status(MOTORS_STATUS_READY);
}

/* ── Task loop ─────────────────────────────────────────────── */

static void motion_task(void *arg) {
    (void) arg;

    while (true) {
        MotionCommand cmd;
        if (xQueueReceive(motion_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        /* Discard a command invalidated by a STOP that landed after enqueue. */
        if (cmd.generation != motors_stop_generation) {
            continue;
        }

        int64_t target = cmd.target_steps;
        if (cmd.relative) {
            target = read_position() + cmd.target_steps;
        }

        run_move(target, cmd.generation);
    }
}

esp_err_t motors_motion_task_init(void) {
    if (xTaskCreatePinnedToCore(motion_task, "motors_motion",
                                MOTION_TASK_STACK_WORDS, NULL,
                                MOTION_TASK_PRIORITY, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "failed to create motion task");
        return ESP_FAIL;
    }
    return ESP_OK;
}
