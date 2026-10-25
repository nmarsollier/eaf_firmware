# eaf_firmware — Electronic Focuser

Electronic focuser for a Newtonian telescope with dedicated sensors.

## Functionality

- EAF (electronic autofocuser)
- Primary-mirror fan control
- Primary-mirror temperature control
- Ambient temperature and humidity control
- Telescope vibration sensor

## Implementation status (October 2026)

### Implemented

| Module | Role | Pins |
|---|---|---|
| `board` | 12 V power sense (ADC). 12 V missing → board LED in *beacon* (error). | ADC `BOARD_POWER_ADC_CHANNEL` |
| `led` | All status LEDs (board + focuser + fan). | board `BOARD_LED_GPIO`, focuser `FOCUSER_LED_GPIO`, fan `FAN_LED_GPIO` |
| `tmc` | TMC2209 over single-wire UART, init with verification and **re-init on 12 V return**. | EN `TMC_EN_GPIO`, TX `TMC_TX_GPIO` (1 kΩ), RX `TMC_RX_GPIO`, STEP `TMC_STEP_GPIO`, DIR `TMC_DIR_GPIO` |
| `motors` | Focuser motion: STEP/DIR over RMT with trapezoidal ramp, `move_to`/`move_by`/`stop`, power gating. | — |
| `i2c_bus` | Shared I2C master bus (ADXL345 + SHT31). | SDA `I2C_SDA_GPIO`, SCL `I2C_SCL_GPIO` |
| `accelerometer` | ADXL345 over I2C (driver: init, probe, read). | SDA `I2C_SDA_GPIO`, SCL `I2C_SCL_GPIO` |
| `sht31` | SHT31-D over I2C (driver: init, probe, read). | SDA `I2C_SDA_GPIO`, SCL `I2C_SCL_GPIO`, addr `SHT31_ADDR` |
| `ds18b20` | DS18B20 mirror temperature over 1-wire (driver: init, probe, non-blocking read). | `DS18B20_GPIO` |
| `vibration` | Per-axis movement figures (demeaned RMS in m/s²) from the ADXL345. | — |
| `usb_cdc` | USB CDC-ACM transport (TinyUSB). | — |
| `proto` | `proto=2`, `role="accessories"`, `schema="eaf-accessories"`; handlers `capabilities`/`state`/`action`/`config`/`control`. | — |
| `fan` | Noctua NF-A8 PWM (25 kHz) + relay; modes `off`/`auto`/`manual`. | PWM `FAN_PWM_GPIO`, relay `FAN_RELAY_GPIO` |
| `buttons` | Two local push buttons (active-low, debounced). | focuser `BUTTON_FOCUSER_ENABLE_GPIO`, fan `BUTTON_FAN_MODE_GPIO` |
| `environment` | Ambient temp/humidity from the SHT31 + dew point; mirror from the DS18B20. | — |

The protocol contract is documented in [`main/proto/README.md`](main/proto/README.md).

Two local push buttons (push-to-GND, active-low):

1. focuser enable/disable toggle (GPIO 1) — enables/disables the TMC2209 (EN),
   releasing the motor's holding torque so the focuser can be turned by hand;
2. fan mode cycle (GPIO 2) — off → auto → manual → off.

### Pending

- Real fan RPM readback (tach).
- Events/subscription, extended session (op_id dedup by content), `config/set` with persistence.
- Go gateway (Alpaca Focuser V4 / ObservingConditions V2 / Switch V3) — separate repo: <https://github.com/nmarsollier/mount_panel>.

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-S3 <https://www.amazon.com/dp/B0DG8L7MQ9> |
| Motor | NEMA 11 <https://www.amazon.com/dp/B0FHHSVXQ7> |
| Driver | TMC2209 <https://www.amazon.com/gp/aw/d/B08WZFK9KT> |
| Fan | Noctua NF-A8 PWM 12 V <https://www.amazon.com/dp/B00VG9PJBO> |
| Mirror temp sensor | DS18B20 <https://www.amazon.com/dp/B0FKG3HT9Q> |
| Ambient temp/humidity | SHT31-D <https://www.amazon.com/dp/B0GZZ9W7CK> |
| Step-down | LM2596 <https://www.amazon.com/gp/aw/d/B0GZ524QTV> |
| Relay | 12 V relay module <https://www.amazon.com/gp/aw/d/B0F3D1X27Y> |

## Electrical architecture

- The controller is fed with 12 V for the TMC2209, the ESP and the fan.
- The fan has a relay that cuts only the fan's 12 V supply.
- The ESP32-S3 is powered two ways: USB from a PC, or 5 V from the 12 V rail via a step-down.
- 12 V monitor: used to determine whether the whole circuit is correctly fed with 12 V.

### ADC circuit

When there is no voltage, the whole circuit enters an error state: an LED pulses
marking the lack of 12 V supply.

### LEDs

Three LEDs: board (general state), focuser (driver state) and fan (speed). The
circuit is the same for all three: LED cathode to GND, anode to a GPIO through a
100 Ω resistor; brightness is dimmed via PWM duty.

- Board LED — beacon (error) when 12 V is missing or an I2C sensor (ADXL345/SHT31) faults; dim otherwise.
- Focuser LED — beacon on a TMC driver fault, bright while moving, dim when idle,
  off when 12 V is missing or the focuser is disabled. A missing 12 V rail is a
  board-level error, shown only by the board LED.
- Fan LED — brightness proportional to fan intensity (≈`FAN_MIN_PWM_PCT`–`FAN_MAX_PWM_PCT` % when running); off when unpowered or 12 V absent.

## EAF

The autofocus is the main function of this circuit.

- A NEMA 11 is connected with pulleys to the focuser's 1:10 reduction wheel via a GT2 belt.
- The exact reduction and movement per step are not yet calibrated.
- Motion uses STEP/DIR over RMT with a trapezoidal acceleration ramp
  (`move_to`, `move_by`, `stop`), driven by the TMC2209.
- EN, STEP and DIR go straight to GPIOs. EN is toggled from the local enable/disable
  button or from the Go UI (`control / focuser / enabled`). Focuser state is shown
  on an LED.

The TMC2209 uses single-wire UART (a 1 kΩ resistor between the TMC TX and the two ESP
GPIOs) to configure `MICROSTEPS` microsteps and 0.8 A for smooth, silent operation.

## Noctua NF-A8 PWM fan

Three modes: `off`, `auto` and `manual`. The button cycles them (off → auto →
manual → off); the mode is also settable from the protocol.

- The fan's 12 V supply is switched by a relay on a dedicated GPIO (off in `off` mode).
- Speed is set with PWM on a GPIO at 25 kHz. The fan's PWM input has an internal
  5 V pull-up, so the 3.3 V GPIO drives it through an open-drain N-MOSFET (2N7002).
- Speed range: `FAN_SPEED_MIN`–`FAN_SPEED_MAX`, mapped linearly to `FAN_MIN_PWM_PCT`–`FAN_MAX_PWM_PCT` % PWM (≈450–2200 RPM); 0 % stops it.
- The protocol reports a **theoretical RPM** (`state/fan` → `rpm`) derived from the
  PWM duty and Noctua's published 450–2200 RPM curve (0 when off). It is an
  estimate until the tachometer is wired.
- `manual` mode uses the speed set by the client; `auto` computes it from the
  mirror/ambient delta: `FAN_SPEED_MAX` above the threshold + hysteresis, `FAN_SPEED_MIN` below it.
  Threshold (`FAN_AUTO_THRESHOLD_C`) and hysteresis (±`FAN_AUTO_HYSTERESIS_C`) are tunable in `config.h`.
- A status LED mirrors the fan intensity (≈`FAN_MIN_PWM_PCT`–`FAN_MAX_PWM_PCT` % when running).

## Mirror temperature sensor — DS18B20

- Placed on the back face of the primary mirror to measure the glass temperature.
- Read over 1-wire on GPIO 6 (RMT-backed; internal pull-up enabled, a 4.7 kΩ
  pull-up to 3.3 V is recommended). The read is non-blocking: a 12-bit
  conversion (~750 ms) runs between runtime-loop ticks.

## Ambient temperature and humidity — SHT31

- I2C bus, usual address 0x44.
- SDA/SCL are shared with the ADXL.
- Must be placed in a ventilated area, away from the ESP32, regulator and TMC2209 heat.
- Provides ambient temperature (controls the main fan and informs NINA of temperature
  changes for autofocus) and humidity (for the dew point, reported to Go).

## ADXL345 accelerometer

- I2C address `ADXL345_ADDR`.
- Measures telescope vibrations.
- Encapsulated with the temperature/humidity sensor, mounted on the telescope tube.
- Initially statistics only, reported to Go. Could later analyze mount problems and
  tracking quality.

## Firmware

The internal firmware is responsible for reading all sensors and:

- handling the fan (power + speed);
- blocking actions in error mode;
- leaving error when the problem is solved;
- detecting and reporting sensor errors;
- controlling temperature and reporting changes to NINA efficiently for autofocus;
- handling the focus motor.

## Communication

USB-CDC with JSON messages. Provides the information and processes the commands needed
for Alpaca and other devices controlled by the ESP. The Go gateway will implement the
full Alpaca Focuser and the other needed interfaces.

## Telemetry

The Go gateway saves full snapshots every 30 seconds for data analysis. Variables:
logical focuser position, focuser motor state, ambient temperature, mirror temperature,
humidity, fan speed, RPM, relay state, vibration statistics, supply voltage, errors.
Commands, movements and important errors are logged immediately by Go.

## Alpaca

The Go interface will implement the REST and UDP interfaces for the focuser so it is
100% compatible with NINA.
