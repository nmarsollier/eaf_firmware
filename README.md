# eaf_firmware — Electronic Focuser

Electronic focuser (EAF) for a Newtonian telescope. This board drives the focuser
only: the fan and the ambient/mirror/vibration sensors live in their own boards
(`fan_firmware` and `temp_firmware`).

## Functionality

- EAF (electronic autofocuser): NEMA 11 stepper + TMC2209 driver, STEP/DIR over RMT
  with a trapezoidal acceleration ramp.
- External 12 V detection (ADC) and a board status LED that flags a missing rail.
- A focuser status LED (TMC/focuser state).
- A local focuser enable/disable button (releases the motor's holding torque).

## Implementation status (October 2026)

| Module | Role | Pins |
|---|---|---|
| `board` | 12 V power sense (ADC). 12 V missing → board LED in *beacon* (error). | ADC `BOARD_POWER_ADC_CHANNEL` |
| `led` | Status LEDs (board + focuser). | board `BOARD_LED_GPIO`, focuser `FOCUSER_LED_GPIO` |
| `tmc` | TMC2209 over single-wire UART, init with verification and **re-init on 12 V return**. | EN `TMC_EN_GPIO`, TX `TMC_TX_GPIO` (1 kΩ), RX `TMC_RX_GPIO`, STEP `TMC_STEP_GPIO`, DIR `TMC_DIR_GPIO` |
| `motors` | Focuser motion: STEP/DIR over RMT with trapezoidal ramp, `move_to`/`move_by`/`stop`, power gating. | — |
| `buttons` | One local push button (active-low, debounced): focuser enable/disable. | focuser `BUTTON_FOCUSER_ENABLE_GPIO` |
| `usb_cdc` | USB CDC-ACM transport (TinyUSB). | — |
| `proto` | `proto=2`, `device_id="eaf"`; handlers `capabilities`/`state`/`action`/`config`/`control`. | — |

The protocol contract is documented in [`main/proto/README.md`](main/proto/README.md).

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-S3 <https://www.amazon.com/dp/B0DG8L7MQ9> |
| Motor | NEMA 11 <https://www.amazon.com/dp/B0FHHSVXQ7> |
| Driver | TMC2209 <https://www.amazon.com/gp/aw/d/B08WZFK9KT> |
| Step-down | LM2596 <https://www.amazon.com/gp/aw/d/B0GZ524QTV> |

## Electrical architecture

- The controller is fed with 12 V for the TMC2209 and the ESP.
- The ESP32-S3 is powered two ways: USB from a PC, or 5 V from the 12 V rail via a
  step-down.
- 12 V monitor: determines whether the whole circuit is correctly fed with 12 V.

### ADC circuit

When there is no voltage, the circuit enters an error state: the board LED pulses,
marking the lack of 12 V supply.

### LEDs

Two LEDs: board (general state) and focuser (driver state). The circuit is the same
for both: LED cathode to GND, anode to a GPIO through a 100 Ω resistor; brightness is
dimmed via PWM duty.

- Board LED — beacon (error) when 12 V is missing; dim otherwise.
- Focuser LED — beacon on a TMC driver fault, bright while moving, dim when idle,
  off when 12 V is missing or the focuser is disabled. A missing 12 V rail is a
  board-level error, shown only by the board LED.

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

## Alpaca

The Go interface (`mount_panel`) implements the Alpaca Focuser V4 REST interface so it
is 100 % compatible with NINA.
