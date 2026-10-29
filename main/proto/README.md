# Focuser protocol (USB CDC-ACM)

Serial contract between the PC (Go gateway) and the focuser's ESP32-S3. It uses the
unified transport and identifies itself through the `capabilities` handshake
(`device_type="accessories"`, `device_id="eaf"`, `proto=2`).

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
- `target`: `device` | `focuser` | `buttons` | `operation`.
- `action` (for `type=action`) or `op` (for control/config/state) names the operation.
- `params` is an object; `id` is a non-zero uint32 (0 is reserved for events, not yet
  implemented); `session`/`boot_id` are strings.

`capabilities` is the exception: it uses the unified identification header below
instead of the session/data envelope.

## Unified capabilities

```json
{"type":"capabilities","id":1,"ok":true,
 "boot_id":"0e86a327","proto":2,
 "device_type":"accessories","device_id":"eaf",
 "name":"EAF","firmware_version":"0.1.0",
 "capabilities":{
   "focuser":{"relative_move":true,"signed_counter":true,"stop":true,
              "local_buttons":false,"tmc_uart":true,"encoder":false,
              "home_sensor":false,"temp_comp":false,"enable":true}}}
```

- `device_type`: `mount` | `accessories` — the discriminator.
- `device_id`, `name`, `firmware_version`, `proto`, `boot_id`: shared identity.

## Implemented catalog

| Operation | Status |
|---|---|
| `capabilities` | Unified handshake header + focuser capabilities (real). |
| `state / device` | `power_external_present`, `health`, `focuser_busy`, `faults[]` (real). |
| `state / focuser` | `counter_steps`, `enabled`, `is_moving`, `op_id`, counters, `driver.uart_ok` (real). |
| `action / focuser / move` | `params.delta_steps` + `expected_counter` → `motors_move_by` (real). |
| `action / focuser / stop` | `motors_stop` (real). |
| `action / device / restart` | Deferred reboot (real). |
| `state / buttons` | Focuser button press state (real, debounced). |
| `state / operation` | Last operation's status. |
| `config / <group> / get` | Read-only mock; `set` → `NOT_SUPPORTED`. |
| `control / focuser / enabled` | `params.value` (`true`/`false`) → `motors_set_enabled`. |
| `control / device / session_open`, `heartbeat` | Minimal, no real lease enforcement. |

## Error codes (strings)

`INVALID_ARGUMENT`, `BUSY`, `COUNTER_CHANGED`, `POWER_ABSENT`, `DRIVER_FAULT`,
`DISABLED`, `INTERNAL`, `NOT_SUPPORTED`.

## State error model

Every `state/<target>` response carries `ok:true` and a `data.error` field that
reports the subsystem's own condition:

- `null` — healthy.
- `"DRIVER_FAULT"` — the TMC2209 is not initialised (e.g. no 12 V); the focuser
  cannot move.

A `state` request **always answers**: a driver fault is returned as an error state,
never as a hang or a fabricated reading.

## Pending (out of this iteration)

Events/subscription and push; real lease/heartbeat; op_id dedup by content
(`OUTCOME_UNKNOWN`/`ID_CONFLICT`); `jog`, `guard`, `identify`, `clear_fault`,
`driver_status`; `config/set` with persistence.
