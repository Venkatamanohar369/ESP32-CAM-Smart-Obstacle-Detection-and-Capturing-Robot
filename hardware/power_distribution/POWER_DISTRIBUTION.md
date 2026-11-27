# Power Distribution Reference — Option A (`PROPOSED`)

See [`docs/POWER_DESIGN.md`](../../docs/POWER_DESIGN.md) for the complete power budget, L298N thermal analysis, capacitor calculations, and mandatory pre-power-up checklist.

- **12V Bus:** `12V, 3A` Adapter → **`3.0 A` Fast-Blow Fuse (PROVISIONAL)** → Main Switch → `470–1000 µF 25V` + `100 nF` → **L298N `+12V`** (`5V-EN` jumper **REMOVED**) & **Buck Converter `IN+` (`≥ 2.0 A` continuous, e.g., `LM2596S`/`MP1584EN`)**.
- **5.0V Logic Rail:** Buck `OUT+` (verified **`5.00V ± 0.10V`** unloaded) → `470 µF 16V` + `100 nF` → **ESP32-CAM `5V`** (+ local `470 µF` + `100 nF`), **Arduino Nano `5V`** (unplug during USB!), **L298N `+5V` (`VSS`)**, **`74AHCT125 VCC`**, **HC-SR04 `VCC`**, **PIR `VCC`**, **5V Buzzer `+`**.
- **Star Ground:** Single common `GND` node tying all subsystems.
