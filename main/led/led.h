#pragma once

/*
 * LED module — owns every status LED (board and focuser, plus the fan LED when
 * it is wired). Each LED has three states: dim (idle), bright (moving) and
 * beacon (error). led_update() is the single entry point that decides the
 * state of each LED from the board and motors state.
 */

/* Initialize every LED's LEDC PWM channel. */
void led_init(void);

/* Periodic update — call from the runtime loop. */
void led_update(void);
