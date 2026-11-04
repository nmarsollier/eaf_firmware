# eaf_firmware Development Rules

Keep this file in a simple format so it can be read and edited quickly.

## Reference projects

- **`~/Dev/bft_firmware`** — ESP32-S3 fork equatorial mount firmware (the mount
  this focuser pairs with). Reference for the motor logic and the USB CDC-ACM serial
  protocol.
- **`~/Dev/mount_panel`** — Go gateway (Alpaca/UI). Exposes the Alpaca Focuser for
  this project.

## Project definition

- Electronic focuser (EAF) for a Newtonian telescope, on ESP32-S3.
- Functions: EAF (NEMA 11 motor + TMC2209 driver) and an onboard RGB status LED.
- 12 V supply (TMC2209 and ESP); the ESP is also powered over USB.
- USB CDC-ACM communication with JSON messages.
- Hardware detail in `README.md`.

## Architecture

Layers, outside-in:

```
Desktop app (Go) (Alpaca/UI) → USB CDC-ACM → Proto → Focuser → TMC2209/GPIO (hardware)
```

- **config** (`main/config/`) — single source of truth for pins and build constants.
- **led** (`main/led/`) — the onboard RGB status LED.
- **tmc** (`main/tmc/`) — TMC2209 (single-wire UART + EN).
- **motors** (`main/motors/`) — focuser motion (STEP/DIR, RMT, ramp).
- **usb_cdc** (`main/usb_cdc/`) — USB CDC-ACM transport.
- **proto** (`main/proto/`) — `proto=2` protocol (framing, dispatch, handlers).
- **buttons** (`main/buttons/`) — local button (focuser enable/disable).
- **runtime** (`main/runtime/`) — init and main loop.

## Current state

Implemented: `led` (onboard RGB status LED), `tmc` (TMC2209), `motors` (motion),
`usb_cdc` (transport), `proto` (`proto=2` protocol, `device_id="eaf"`, with
capabilities/state/action/config/control handlers) and `buttons` (local focuser
enable/disable toggle).

LED behaviour: the onboard RGB LED is green when healthy and green-beacons on a TMC
driver fault.

One physical button is wired (push-to-GND on GPIO 8): focuser enable/disable toggle.

Pending: events/extended session, `config/set` with persistence, and the Go gateway.
Contract in `main/proto/README.md`.

## General rules

- Never commit or push to GitHub without explicit authorization.
- Commit only when the whole feature is verified and compiling.
- Always base on code on disk as the single source of truth. Source code is the
  only valid reference for implementation details.
- Before any change, read the current state of the file to modify.
- Project written in C, following a functional style when possible.
- Modules separated by domain, each domain in a folder under `main/`.
- Each domain exposes:
    - `domain.h` — public API (functions other modules may call)
    - `domain_internal.h` — internal API (functions shared only among the module's files)
- One `.c` file per use case within each module.
- Public functions start with the module name followed by `_`.
- A module's state variables are never accessed directly from outside, only
  through the module's public functions.
- Respect: DRY - YAGNI - KISS.
- Header documentation describes the business problem or use case the function
  solves, not implementation details.
- Only comment code when it is hard for a human to understand.
- Code, comments and documentation (`.c`, `.h` and `.md`) are written in English.

## Code conventions

- **Language**: C (not C++)
- **snake_case** for functions and variables
- **UPPER_CASE** for macros and defines
- **Headers**: `#pragma once`, includes organized: the module's own first, then framework libraries
- **Logging**: `ESP_LOGI()`, `ESP_LOGW()`, `ESP_LOGE()` with a static tag per file
- **Results between layers**: use per-module `Result` types with the `{ ok, message }` shape
