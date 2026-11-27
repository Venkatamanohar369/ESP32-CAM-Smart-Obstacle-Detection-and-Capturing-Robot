# Test Plan — ESP32-CAM Autonomous Surveillance Robot

## 1. Test Categories & Execution Scope
1. **Static Analysis & Secret Scanning (`tests/unit/test_robot_logic.py`):** Executed locally via Python `unittest`.
2. **Firmware Compilation (`pio run`):** Executed locally via PlatformIO Core `6.2.0` for `esp32cam` and `nanoatmega328` / `nanoatmega328new`.
3. **Software & Protocol Unit/Integration Tests (`tests/unit/`, `tests/integration/`):** Executed locally via Python `unittest` (verifies protocol framing, motor/sensor safety state machine invariants, web dashboard HTML/JS, HTTP validation, and Telegram multipart request construction).
4. **Physical Hardware Tests:** Remain **`NOT RUN`** until wiring, Buck Converter `5.00V` output, and motor winding resistance are verified and hardware power-up is approved.

## 2. Reproducible Test Commands
```powershell
$env:PLATFORMIO_CORE_DIR = "$PWD\.pio-core"
.\.pio-venv\Scripts\pio.exe run
python -m unittest discover -s tests -p "test_*.py" -v
```
