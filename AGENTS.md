# AGENTS.md — Permanent Repository Maintenance Rules

All AI coding agents and contributors modifying this repository **must** follow these rules:

## 1. Mandatory `PROJECT_MASTER.md` Synchronization Rule
**Whenever any project-related modification is made, [`PROJECT_MASTER.md`](PROJECT_MASTER.md) must be reviewed and updated in the same development task.**

This applies to any change involving:
- Firmware source code (`firmware/`)
- GPIO assignments or pin mappings
- Hardware components, wiring, or power design
- UART communication protocol (`uart_protocol.h`)
- Added, modified, or removed features
- Web dashboard (`software/web_dashboard/`, `web_dashboard.h`)
- Dependencies or build configuration (`platformio.ini`)
- Tests or validation results (`tests/`, `docs/TEST_REPORT.md`)
- Security or credential handling (`config/`, `.gitignore`)
- Repository structure

### Required Agent Workflow
1. Inspect the affected implementation and documentation.
2. Identify every section of `PROJECT_MASTER.md` impacted by the change.
3. Update those sections accurately in the same task.
4. Append a new chronological entry to **Section 18 (Change History)** of `PROJECT_MASTER.md` (Date, Files Changed, What Changed, Why, Build/Test Impact, Docs Updated).
5. Verify that `PROJECT_MASTER.md` matches actual code and test outputs—never document an unimplemented feature as completed or a physical test as passed unless actually executed.
6. If a change does not affect `PROJECT_MASTER.md`, explicitly state in the task summary that `PROJECT_MASTER.md` was reviewed and no update was necessary.
7. Do not create competing master documents; focused docs (`hardware/`, `docs/`) may exist, but `PROJECT_MASTER.md` remains the authoritative single source of truth.

## 2. Safety & Security Guardrails
- Never commit `config/config.h` or real Wi-Fi/Telegram credentials.
- Keep unverified physical wiring marked `PROPOSED` until bench-tested.
- Never instruct connecting `12V` to ESP32-CAM, Arduino Nano, `74AHCT125`, or 5V sensors.
- Do not commit `.pio/`, `.pio-venv/`, `.pio-core/`, or `__pycache__/`.
