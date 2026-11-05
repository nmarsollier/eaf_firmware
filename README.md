# eaf_firmware — Electronic Focuser

Electronic focuser (EAF) for a Newtonian telescope. This board drives the focuser
only: the fan and the ambient/vibration sensors live in their own boards
(`fan_firmware` and `amb_firmware`).

## Functionality

- EAF (electronic autofocuser): NEMA 11 stepper + TMC2209 driver, STEP/DIR over RMT
  with a trapezoidal acceleration ramp.
- External 12 V detection: the TMC2209 answers a UART probe only when its 12 V rail
  is present.
- An onboard RGB status LED (WS2812) that flags a missing rail or a TMC driver fault.
- A local focuser enable/disable button (releases the motor's holding torque).

## Implementation status (November 2026)

| Module | Role | Pins |
|---|---|---|
| `led` | Onboard RGB status LED (WS2812): green when healthy, green beacon on a missing rail or TMC fault. | `LED_GPIO` |
| `tmc` | TMC2209 over single-wire UART, init with verification; a periodic UART probe doubles as the 12 V sense, with re-init on power return. | EN `TMC_EN_GPIO`, TX `TMC_TX_GPIO` (1 kΩ), RX `TMC_RX_GPIO`, STEP `TMC_STEP_GPIO`, DIR `TMC_DIR_GPIO` |
| `motors` | Focuser motion: STEP/DIR over RMT with trapezoidal ramp, `move_to`/`move_by`/`stop`, power gating. | — |
| `buttons` | One local push button (active-low, debounced): focuser enable/disable. | focuser `BUTTON_FOCUSER_ENABLE_GPIO` |
| `usb_cdc` | USB CDC-ACM transport (TinyUSB). | — |
| `proto` | `proto=2`, `device_id="eaf"`; handlers `capabilities`/`state`/`action`/`config`/`control`. | — |

The protocol contract is documented in [`main/proto/README.md`](main/proto/README.md).

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-S3 Super Mini <https://www.amazon.com/dp/B0GY3M83NL> |
| Motor | NEMA 11 <https://www.amazon.com/dp/B0FHHSVXQ7> |
| Driver | TMC2209 <https://www.amazon.com/gp/aw/d/B08WZFK9KT> |
| Step-down | LM2596 <https://www.amazon.com/gp/aw/d/B0GZ524QTV> |

## Electrical architecture

- The controller is fed with 12 V for the TMC2209 and the ESP.
- The ESP32-S3 is powered two ways: USB from a PC, or 5 V from the 12 V rail via a
  step-down.
- 12 V sense: the TMC2209 is probed over UART every second; a missing response means
  the 12 V rail is down.

### Status LED

A single onboard RGB LED (WS2812 on GPIO 48), driven via RMT.

- Green (solid) — healthy: 12 V present and no TMC driver fault.
- Green beacon — fault: 12 V missing or a TMC driver fault.

## EAF

The autofocus is the main function of this circuit.

- A NEMA 11 is connected with pulleys to the focuser's 1:10 reduction wheel via a GT2
  belt.
- The exact reduction and movement per step are not yet calibrated.
- Motion uses STEP/DIR over RMT with a trapezoidal acceleration ramp
  (`move_to`, `move_by`, `stop`), driven by the TMC2209.
- EN, STEP and DIR go straight to GPIOs. EN is toggled from the local enable/disable
  button or from the Go UI (`control / focuser / enabled`). Focuser state is shown on
  an LED.

The TMC2209 uses single-wire UART (a 1 kΩ resistor between the TMC TX and the two ESP
GPIOs) to configure `MICROSTEPS` microsteps and 0.8 A for smooth, silent operation.

## Communication

USB-CDC with JSON messages. Provides the focuser state and processes the focuser
commands needed for Alpaca. The Go gateway (`mount_panel`) implements the Alpaca
Focuser V4 interface on top of this link.

### Button-less flashing

`make flash` builds first, sends `action/device/bootloader` over one open USB
CDC connection, and waits for an ESP32-S3 ROM handshake before flashing. The
port may change names or keep the same name. A missing command response is only
accepted if the ROM subsequently responds (including a board already in download
mode). An explicit protocol rejection aborts the operation.

Connect one ESP board and close serial monitors/gateways before flashing. Port
selection requires a single `/dev/cu.usbmodem*`, or an explicit
`make flash PORT=/dev/cu.usbmodem...`. USB location/serial-string matching is not
required. The workflow uses the pyserial and esptool installed with ESP-IDF.

`make enter-flash` requests and verifies download mode without building or writing
flash. `make flash-download` builds and flashes a board already in download mode,
without sending an application command. Both support `PORT=...` and use the same
ROM verification. If software entry fails, use BOOT/RESET to enter download mode
and run `make flash-download`. The wire format is documented in
[`main/proto/README.md`](main/proto/README.md).

## Alpaca

The Go interface (`mount_panel`) implements the Alpaca Focuser V4 REST interface so it
is 100 % compatible with NINA.
