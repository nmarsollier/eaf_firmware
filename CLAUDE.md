# NRM-BFT-EAF Development Rules

Keep this file in a simple format so it can be read and edited quickly.

## Reference projects

- **`~/Dev/nrm_ha`** — ESP32-S3 equatorial mount firmware. Reference for the
  LED/motor logic, the TMC2209 driver (`HAv2_TMC` branch), 12 V detection and
  the USB CDC-ACM serial protocol.
- **`~/Dev/nrm_ha_go`** — Go gateway (Alpaca/UI). Will expose the Alpaca Focuser
  for this project. Out of scope for now.

## Project definition

- Electronic focuser (EAF) for a Newtonian telescope, on ESP32-S3.
- Functions: EAF (NEMA 11 motor + TMC2209 driver), primary-mirror fan control
  (Noctua NF-A8 PWM 12 V with relay), mirror temperature (DS18B20, 1-wire),
  ambient temperature/humidity (SHT31, I2C), vibration (ADXL345, I2C).
- 12 V supply (TMC2209, ESP and fan); the ESP is also powered over USB.
- 3 status LEDs (board, focuser, fan).
- USB CDC-ACM communication with JSON messages.
- Hardware detail in `README.md`.

## Architecture

Layers, outside-in:

```
Desktop app (Go) (Alpaca/UI) → USB CDC-ACM → Proto → Focuser → TMC2209/GPIO (hardware)
```

- **config** (`main/config/`) — single source of truth for pins and build constants.
- **board** (`main/board/`) — 12 V power sense.
- **led** (`main/led/`) — every status LED (board, focuser, fan).
- **tmc** (`main/tmc/`) — TMC2209 (single-wire UART + EN), re-init on 12 V return.
- **motors** (`main/motors/`) — focuser motion (STEP/DIR, RMT, ramp).
- **accelerometer** (`main/accelerometer/`) — ADXL345 over I2C.
- **vibration** (`main/vibration/`) — per-axis movement figures from the ADXL345.
- **usb_cdc** (`main/usb_cdc/`) — USB CDC-ACM transport.
- **proto** (`main/proto/`) — `proto=2` protocol (framing, dispatch, handlers).
- **fan** (`main/fan/`) — Noctua PWM + relay + modes (off/auto/manual).
- **environment / inputs** (`main/*/`) — mocks (fixed values).
- **runtime** (`main/runtime/`) — init and main loop.

## Current state

Implemented: `board` (12 V sense), `led` (board + focuser + fan LEDs), `tmc`
(TMC2209 with re-init on 12 V return), `motors` (motion), `accelerometer`
(ADXL345) + `vibration` (per-axis movement figures), `usb_cdc` (transport),
`proto` (`proto=2` protocol with capabilities/state/action/config/control
handlers), `fan` (Noctua PWM + relay, modes off/auto/manual) and `inputs`
(local buttons: focuser enable/disable + fan mode cycle). Mock of
`environment`.

LED behaviour: the board LED flags a missing 12 V rail or an I2C/accelerometer
fault; the focuser LED flags a TMC driver fault (off when 12 V is missing or the
focuser is disabled); the fan LED brightness follows the fan intensity. A
power-loss error is shown only by the board LED.

Two physical buttons are wired (push-to-GND on GPIO 1 and 2): focuser
enable/disable toggle and fan mode cycle (off/auto/manual).

Pending: real sensors
(DS18B20/SHT31), events/extended session, `config/set` with persistence, and
the Go gateway. Contract in `main/proto/README.md`.

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
