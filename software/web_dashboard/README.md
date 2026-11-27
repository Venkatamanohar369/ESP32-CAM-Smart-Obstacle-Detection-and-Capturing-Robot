# Web Dashboard

- **Embedded Firmware Copy:** [`firmware/esp32cam/include/web_dashboard.h`](../../firmware/esp32cam/include/web_dashboard.h) (stored in `PROGMEM`, served at `GET /` on port `80`).
- **Editable Reference Copy:** [`software/web_dashboard/index.html`](index.html).
- **Live MJPEG Stream:** Served separately on port `81` at `/stream` so video streaming never blocks REST motor commands.
- **Zero Cloud Dependencies:** Runs 100% locally on the ESP32-CAM's LAN IP.

## REST API Endpoints (Port 80)

| Method & Path | Parameters | Validation & Behavior |
| :--- | :--- | :--- |
| `GET /` | — | Serves mobile-friendly HTML dashboard |
| `GET /api/status` | — | Returns JSON telemetry (`camOk`, `psram`, `wifiRssi`, `nanoOnline`, `distMm`, `pir`, `obs`, `motorDir`, `pwm`, `tgEnabled`, `tgOk`, `tgStatus`, `captures[]`) |
| `POST /api/move` | `dir=F\|B\|L\|R\|S`, `speed=0..255` | Validates direction & speed; forwards `$CMD,M,<dir>,<speed>` to Nano |
| `POST /api/capture` | — | Acquires JPEG frame, saves with current ultrasonic distance, queues Telegram photo |
| `POST /api/buzzer` | `mode=0\|1\|2` | `0`=off, `1`=200 ms beep, `2`=alarm |
| `POST /api/flash` | `on=0\|1` | Toggles onboard `GPIO 4` white Flash LED |
| `GET /api/photo` | `id=<N>` | Serves stored JPEG from PSRAM ring buffer |
