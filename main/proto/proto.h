#pragma once

/*
 * Focuser protocol — public entry point.
 *
 * The protocol is a length-prefixed JSON message stream carried over the
 * USB CDC-ACM transport (proto=2, role="accessories"). It exposes the focuser
 * and its accessories (fan, environment, vibration, inputs) so a desktop app
 * translates them into ASCOM Alpaca and a UI. See README.md in this folder for
 * the wire contract.
 *
 * proto_init() must be called after usb_cdc_init(): it wires the CDC receive
 * stream to the frame parser.
 */
void proto_init(void);

/*
 * Periodic housekeeping — advance the operation status (running → completed)
 * once the motor stops. Called from the runtime loop, never from a request
 * handler, so handlers stay pure readers of the cached state.
 */
void proto_poll(void);
