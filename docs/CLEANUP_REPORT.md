# Repository Cleanup & Size Audit Report

> **Audit Date:** 2026-11-27  
> **Original Workspace Size:** `1,919,686,917 bytes` (`1,830.76 MB`)

---

## 1. Directory Size Breakdown (Before Cleanup)

| Directory | Size (MB) | Category | Action | Reason |
| :--- | :---: | :--- | :--- | :--- |
| `.pio-core/` | `1,773.94 MB` | **B — Generated** | **REMOVE** | Downloaded PlatformIO toolchains (`xtensa-esp32`, `atmelavr`), frameworks (`arduinoespressif32`, `arduino-avr`), and package `.cache/tmp` archives; reproducible from `platformio.ini` |
| `.pio/` | `40.41 MB` | **B — Generated** | **REMOVE** | Compiled object files, `.elf`, `.bin`, `.hex` build outputs; reproducible via `pio run` |
| `.pio-venv/` | `16.20 MB` | **B — Generated** | **REMOVE** | Local Python `3.12` virtual environment (`platformio==6.2.0`); reproducible via `uv venv .pio-venv --python 3.12` |
| `tests/**/__pycache__/` | `<0.02 MB` | **B — Generated** | **REMOVE** | Python bytecode caches |
| `docs/` | `0.08 MB` | **A — Essential** | **PRESERVE** | Hardware audit, GPIO feasibility, power design, architecture, build guide, test report, and `docs/images/esp32-cam-robot-project.jpg` |
| `firmware/` | `0.06 MB` | **A — Essential** | **PRESERVE** | `shared/`, `esp32cam/`, and `nano/` C++ source and headers |
| `software/` | `0.02 MB` | **A — Essential** | **PRESERVE** | Web dashboard reference (`index.html`) and Telegram setup guide |
| `tests/` | `0.02 MB` | **A — Essential** | **PRESERVE** | Unit tests, integration tests, test plan, and `latest_results.json` |
| `hardware/` | `0.01 MB` | **A — Essential** | **PRESERVE** | `BOM.md`, `GPIO_MAPPING.md`, wiring/block/power diagrams |
| `config/` | `<0.01 MB` | **A — Essential** | **PRESERVE** | `config.example.h` (placeholders only; no secrets) |
| `source_material/` | `<0.01 MB` | **A — Essential** | **PRESERVE** | Original `PROJECT_BRIEF.md` |
| Root files | `<0.02 MB` | **A — Essential** | **PRESERVE** | `README.md`, `LICENSE`, `.gitignore`, `platformio.ini`, `PROJECT_MASTER.md`, `AGENTS.md` |

## 2. Category C Investigation (Potentially Unnecessary Files)
- **Result:** No duplicate source files, abandoned experiments, unused libraries, unreferenced images, or backup files exist in the tracked project tree.
- `software/web_dashboard/index.html` is the human-editable reference copy corresponding to `firmware/esp32cam/include/web_dashboard.h` (`PROGMEM`) and is referenced by `tests/integration/test_dashboard_and_protocol.py`. **Preserved.**
- `docs/images/esp32-cam-robot-project.jpg` is the supplied prototype image referenced by `README.md` and `docs/HARDWARE_AUDIT.md`. **Preserved.**
