# NRM-BFT-EAF — Electronic Focuser

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
| `board` | 12 V power sense (ADC). 12 V missing → board LED in *beacon* (error). | 12 V on 17 |
| `led` | All status LEDs (board + focuser; fan when wired). | board 21, focuser 47 |
| `tmc` | TMC2209 over single-wire UART, init with verification and **re-init on 12 V return**. | EN 9, TX 18 (1 kΩ), RX 8, STEP 16, DIR 15 |
| `motors` | Focuser motion: STEP/DIR over RMT with trapezoidal ramp, `move_to`/`move_by`/`halt`/`stop`, power gating. | — |
| `accelerometer` | ADXL345 over I2C (driver: init, probe, read). | SDA 5, SCL 4 |
| `vibration` | Per-axis movement figures (demeaned RMS in m/s²) from the ADXL345. | — |
| `usb_cdc` | USB CDC-ACM transport (copied from NRM-HA). | — |
| `proto` | `proto=2`, `role="accessories"`, `schema="nrm-accessories"`; handlers `capabilities`/`state`/`action`/`config`/`control`. | — |
| mocks | `fan`, `environment`, `inputs` (fixed values). | — |

The protocol contract is documented in [`main/proto/README.md`](main/proto/README.md).

Two physical buttons are planned (not yet wired):

1. focuser enable/disable toggle;
2. fan mode toggle (normal ↔ off).

### Pending

- Real fan (Noctua PWM + relay) and MODE button.
- Real sensors: DS18B20, SHT31.
- Events/subscription, extended session (op_id dedup by content), `config/set` with persistence.
- Go gateway (Alpaca Focuser V4 / ObservingConditions V2 / Switch V3) — separate repo.

## References

Reference projects for programming style and examples:

- **NRM-HA** — <https://github.com/nmarsollier/nrm_ha> — EQ mount firmware similar to
  this project; source of style and programming rules. It has the LED/motor logic and
  the whole serial protocol we largely copy. The `HAv2_TMC` branch implements TMC2209
  correctly and must be used as the reference for TMC init/usage. It also has the 12 V
  current-detection logic, and its commit history has the ADXL read logic
  (`ed255b515968b862b76a47308591af3ac1976114`). Cloned locally at `~/Dev`.
- **NRM-HA-GO** — <https://github.com/nmarsollier/nrm_ha_go> — Alpaca gateway UI for the
  mount; will also work for this project by extending its functions. Cloned locally at `~/Dev`.

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

There are 3 LEDs: board general state, fan state, focuser state. The circuit is the
same for all three: LED cathode straight to GND, anode to a GPIO through a 100 Ω
resistor. LEDs support brightness dimming via PWM frequency.

## EAF

The autofocus is the main function of this circuit.

- A NEMA 11 is connected with pulleys to the focuser's 1:10 reduction wheel via a GT2 belt.
- The real reduction and movement per step are unknown.
- There will be an acceleration ramp similar to NRM-HA but simplified, since motion is
  much simpler here (one motor, very simple functions). The motion task and RMT will be
  similar in essence.
- The NEMA is controlled with the TMC2209.

Control connections: EN, STEP and DIR go straight to GPIOs. EN can be changed from the
Go UI and with a push button on a GPIO (manual use). Focuser state is shown on an LED.

UART is configured single-wire with a 1 kΩ resistor between the TMC TX and two ESP
GPIOs. Over UART it configures 16 microsteps, 0.8 A and whatever is needed for smooth,
silent motor operation.

## Noctua NF-A8 PWM fan

- The fan is turned on/off with a relay on a dedicated GPIO.
- Normal state is controlled by the ESP, which reads ambient and mirror temperatures to
  set the speed: while a temperature difference exists the fan runs at 100%; when the
  temperatures match, flow drops to 30% to keep air moving.
- Speed can also be forced by software (UI, or during tracking to avoid vibrations).
- A push button toggles between normal and off.
- A dedicated LED shows the fan state, and it also shows in the Go UI.
- PWM is controlled with another GPIO to adjust speed.

## Mirror temperature sensor — DS18B20

- Placed on the back face of the primary mirror to measure the glass temperature.
- Read over 1-wire on a dedicated GPIO.

## Ambient temperature and humidity — SHT31

- I2C bus, usual address 0x44.
- SDA/SCL are shared with the ADXL.
- Must be placed in a ventilated area, away from the ESP32, regulator and TMC2209 heat.
- Provides ambient temperature (controls the main fan and informs NINA of temperature
  changes for autofocus) and humidity (for the dew point, reported to Go).

## ADXL345 accelerometer

- I2C address 0x53.
- Measures telescope vibrations.
- Encapsulated with the temperature/humidity sensor, mounted on the telescope tube.
- Initially statistics only, reported to Go. Could later analyze mount problems and
  tracking quality.

## Firmware

The internal firmware is responsible for reading all sensors and:

- handling the fan in normal mode;
- blocking actions in error mode;
- leaving error when the problem is solved;
- detecting and reporting sensor errors;
- controlling temperature and reporting changes to NINA efficiently for autofocus;
- handling the focus motor.

## Communication

USB-CDC with JSON messages similar to NRM-HA. Provides the information and processes the
commands needed for Alpaca and other devices controlled by the ESP. The Go gateway will
implement the full Alpaca Focuser and the other needed interfaces.

## Telemetry

The Go gateway saves full snapshots every 30 seconds for data analysis. Variables:
logical focuser position, focuser motor state, ambient temperature, mirror temperature,
humidity, fan speed, RPM, relay state, vibration statistics, supply voltage, errors.
Commands, movements and important errors are logged immediately by Go.

## Alpaca

The Go interface will implement the REST and UDP interfaces for the focuser so it is
100% compatible with NINA.
