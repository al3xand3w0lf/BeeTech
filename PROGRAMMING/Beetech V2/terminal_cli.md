# Beetech V2 - Terminal CLI Reference

Serial Monitor: **115200 baud**, Newline (LF or CR)

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `status` | System status (sensors, WiFi, DB, offsets, intervals) |
| `read` | Read current weight and temperature |
| `tare` | Tare the scale (set current load as zero) |
| `saveoffset` | Save current tare offset to SD card (`/tare_offset.txt`) |
| `cal XX` | Calibrate with known weight XX kg (e.g. `cal 5.0`) |
| `upload` | Force immediate data upload to database |
| `wifi` | Show WiFi connection status and signal strength |
| `db` | Show MySQL database connection status |
| `reset` | Restart the ESP32 |

## Typical Workflows

### Initial Setup (new scale)

1. Place empty beehive box on the scale
2. Boot with `deep_sleep_enabled=0` in `config.txt`
3. `tare` — zeros the scale with the empty box weight
4. `read` — verify weight shows ~0.0 kg
5. `saveoffset` — persists the tare offset to `/tare_offset.txt` on SD
6. Set `deep_sleep_enabled=1` in `config.txt`
7. `reset` — system now runs in deep sleep mode

### Calibration (known reference weight)

1. Boot with `deep_sleep_enabled=0`
2. `tare` — zero the scale
3. Place known weight (e.g. 5 kg) on the scale
4. `cal 5.0` — calculates new calibration factor
5. `read` — verify weight is correct
6. Note the calibration factor from Serial output and update `config.txt`

### Troubleshooting

1. `status` — check which sensors are connected, WiFi/DB state
2. `wifi` — check signal strength (RSSI)
3. `db` — check database connection details
4. `upload` — test a manual upload to verify DB connectivity

## Operating Modes

| Mode | `deep_sleep_enabled` | Behavior |
|------|---------------------|----------|
| **Setup** | `0` | Normal loop with terminal, LCD, periodic readings. Use for tare/calibration. |
| **Betrieb** | `1` | Boot → measure → upload → deep sleep for `upload_interval` seconds. No terminal. |

## Tare Offset Storage

The tare offset is stored as a raw HX711 value in `/tare_offset.txt` on the SD card (one line, e.g. `123456`).

- Created automatically by the `saveoffset` command
- Can also be written manually if the offset value is already known
- Overrides any `scale_offset` value in `config.txt`
- Survives deep sleep cycles and power loss
