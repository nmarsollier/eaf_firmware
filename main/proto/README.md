# Focuser protocol (USB CDC-ACM)

Serial contract between the PC (Go gateway) and the focuser's ESP32-S3. It uses the
unified transport and identifies itself through the `capabilities` handshake
(`device_type="accessories"`, `proto=2`).

## Transport and framing

- USB CDC-ACM (TinyUSB), a single serial port.
- Frame: `[u32 little-endian length][JSON UTF-8]`, up to 2048 bytes per body, no
  application CRC (USB is reliable).
- The receiver accepts partial reads and several frames together; on error it drops a
  byte and resynchronizes.

## Message envelope

Request:

```json
{"proto":2,"type":"action","id":42,"session":"g-7f2a","boot_id":"a1b2c3d4",
 "target":"focuser","action":"move","params":{"delta_steps":320,"expected_counter":1200}}
```

Response (`ok:true` + `data`, or `ok:false` + `error`):

```json
{"proto":2,"type":"action","id":42,"session":"g-7f2a","boot_id":"a1b2c3d4",
 "ok":true,"data":{...}}
{"proto":2,"type":"action","id":42,"session":"g-7f2a","boot_id":"a1b2c3d4",
 "ok":false,"error":{"code":"POWER_ABSENT","message":"No external power","field":null}}
```

- `type`: `capabilities` | `state` | `config` | `control` | `action`.
- `target`: `device` | `focuser` | `fan` | `environment` | `vibration` | `inputs` | `operation`.
- `action` (for `type=action`) or `op` (for control/config/state) names the operation.
- `params` is an object; `id` is a non-zero uint32 (0 is reserved for events, not yet
  implemented); `session`/`boot_id` are strings.

`capabilities` is the exception: it uses the unified identification header below
instead of the session/data envelope.

## Unified capabilities

A shared header identifies the device, then the `capabilities` object holds the
device-specific capabilities:

```json
{"type":"capabilities","id":1,"ok":true,
 "boot_id":"0e86a327","proto":2,
 "device_type":"accessories","device_id":"nrm-eaf",
 "name":"NRM-EAF","firmware_version":"0.1.0",
 "capabilities":{
   "focuser":{"relative_move":true,"signed_counter":true,"halt":true,
              "local_buttons":false,"tmc_uart":true,"encoder":false,
              "home_sensor":false,"temp_comp":false},
   "fan":{"modes":["normal","soft","off"],"pwm":true,"relay":true,"tach":false},
   "sensors":{"ambient":"SHT31","mirror":"DS18B20","vibration":"ADXL345"}}}
```

- `device_type`: `mount` | `accessories` — the discriminator.
- `device_id`, `name`, `firmware_version`, `proto`, `boot_id`: shared identity.

## Implemented catalog

| Operation | Status |
|---|---|
| `capabilities` | Unified handshake header + device-specific capabilities (real). |
| `state / device` | `power_external_present`, `health`, `focuser_busy`, `fan_mode`, `faults[]` (real). |
| `state / focuser` | `counter_steps`, `is_moving`, `op_id`, counters, `driver.uart_ok` (real). |
| `action / focuser / move` | `params.delta_steps` + `expected_counter` → `motors_move_by` (real). |
| `action / focuser / halt` | `motors_stop` (real). |
| `action / device / restart` | Deferred reboot (real). |
| `state / fan`, `environment`, `vibration`, `inputs` | Mocks (fixed values). |
| `state / operation` | Last operation's status. |
| `config / <group> / get` | Read-only mock; `set` → `NOT_SUPPORTED`. |
| `control / fan / mode` | Changes the mock mode (`normal`/`soft`/`off`). |
| `control / device / session_open`, `heartbeat` | Minimal, no real lease enforcement. |

## Error codes (strings)

`INVALID_ARGUMENT`, `OUT_OF_RANGE`, `NOT_SUPPORTED`, `NOT_CONNECTED`, `BUSY`,
`COUNTER_CHANGED`, `POWER_ABSENT`, `DRIVER_FAULT`, `SENSOR_FAULT`, `STALE_DATA`,
`INTERNAL`, `ID_CONFLICT`.

## State error model

Every `state/<target>` response carries `ok:true` and a `data.error` field that
reports the subsystem's own condition:

- `null` — healthy (or mock data, which is valid by definition).
- `"SENSOR_ABSENT"` — the sensor was not detected at boot (e.g. ADXL345). The
  value fields are `null`.
- `"DRIVER_FAULT"` — the TMC2209 is not initialised (e.g. no 12 V); the focuser
  cannot move.

A `state` request **always answers**: a sensor fault is returned as an error
state, never as a hang or a fabricated reading (no 0 °C / 0 m/s² placeholders).
This keeps the serial connection alive and the rest of the device functional —
one faulty sensor only degrades its own `state` target.

## Pending (out of this iteration)

Events/subscription and push; real lease/heartbeat; op_id dedup by content
(`OUTCOME_UNKNOWN`/`ID_CONFLICT`); `jog`, `guard`, `identify`, `clear_fault`,
`driver_status`, `environment/refresh`, `vibration/enable|baseline|capture`;
`config/set` with persistence; real fan and sensors.
