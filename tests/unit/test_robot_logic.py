import os
import re
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def xor_checksum(payload: str) -> int:
    cs = 0
    for ch in payload.encode("ascii"):
        cs ^= ch
    return cs


def format_frame(payload: str) -> str:
    return f"${payload}*{xor_checksum(payload):02X}\n"


def parse_frame(line: str):
    line = line.strip()
    if len(line) < 5 or not line.startswith("$") or line[-3] != "*":
        return None
    try:
        expected = int(line[-2:], 16)
    except ValueError:
        return None
    payload = line[1:-3]
    if xor_checksum(payload) != expected:
        return None
    return payload


def echo_us_to_mm(us: int) -> int:
    if us == 0:
        return -1
    mm = (us * 10) // 58
    if mm < 20 or mm > 4000:
        return -1
    return mm


class TestStaticAndSecurity(unittest.TestCase):
    def test_gitignore_excludes_secrets_and_build(self):
        with open(os.path.join(REPO_ROOT, ".gitignore"), "r", encoding="utf-8") as f:
            gi = f.read()
        for entry in ["config/config.h", ".pio/", ".pio-venv/", ".pio-core/"]:
            self.assertIn(entry, gi)

    def test_config_example_has_no_real_secrets(self):
        with open(os.path.join(REPO_ROOT, "config", "config.example.h"), "r", encoding="utf-8") as f:
            cfg = f.read()
        self.assertIn("YOUR_WIFI_SSID", cfg)
        self.assertIn("YOUR_TELEGRAM_BOT_TOKEN", cfg)
        self.assertIn("TELEGRAM_ENABLED        false", cfg)

    def test_pin_consistency_across_firmware_and_docs(self):
        with open(os.path.join(REPO_ROOT, "firmware", "esp32cam", "include", "app_config.h"), "r", encoding="utf-8") as f:
            esp = f.read()
        self.assertRegex(esp, r"#define\s+PIN_UART2_RX\s+13")
        self.assertRegex(esp, r"#define\s+PIN_UART2_TX\s+14")

        with open(os.path.join(REPO_ROOT, "firmware", "nano", "include", "nano_pins.h"), "r", encoding="utf-8") as f:
            nano = f.read()
        for name, val in [
            ("PIN_UART_RX", "8"), ("PIN_UART_TX", "9"),
            ("PIN_L298N_ENA", "5"), ("PIN_L298N_ENB", "6"),
            ("PIN_L298N_IN1", "4"), ("PIN_L298N_IN2", "7"),
            ("PIN_L298N_IN3", "2"), ("PIN_L298N_IN4", "3"),
            ("PIN_US_TRIG", "10"), ("PIN_US_ECHO", "11"),
            ("PIN_PIR_OUT", "12"), ("PIN_BUZZER", "A0"),
        ]:
            self.assertRegex(nano, rf"{name}\s*=\s*{val}\s*;")

    def test_no_bss138_in_wiring_diagram(self):
        with open(os.path.join(REPO_ROOT, "hardware", "wiring_diagram", "WIRING_DIAGRAM.md"), "r", encoding="utf-8") as f:
            wd = f.read()
        self.assertIn("74AHCT125", wd)
        self.assertNotIn("BSS138", wd)


class TestProtocolAndLogic(unittest.TestCase):
    def test_uart_frame_roundtrip_and_rejection(self):
        frame = format_frame("TEL,450,1,0,F,180")
        self.assertEqual(parse_frame(frame), "TEL,450,1,0,F,180")
        # Corrupt checksum
        bad = frame[:-2] + ("0" if frame[-2] != "0" else "1") + "\n"
        self.assertIsNone(parse_frame(bad))
        self.assertIsNone(parse_frame("NO_DOLLAR*00"))

    def test_ultrasonic_conversion(self):
        self.assertEqual(echo_us_to_mm(0), -1)
        self.assertEqual(echo_us_to_mm(50), -1)      # < 20 mm
        self.assertEqual(echo_us_to_mm(30000), -1)   # > 4000 mm
        self.assertEqual(echo_us_to_mm(1160), 200)   # 20.0 cm

    def test_motor_and_obstacle_cpp_invariants_present(self):
        with open(os.path.join(REPO_ROOT, "firmware", "shared", "include", "robot_logic.h"), "r", encoding="utf-8") as f:
            src = f.read()
        self.assertIn("DEFAULT_DEADTIME_MS = 100", src)
        self.assertIn("obstacleActive && (activeDir_ == MotorDir::FORWARD || targetDir_ == MotorDir::FORWARD)", src)
        self.assertIn("nowMs - lastCmdMs_ >= cmdTimeoutMs_", src)


if __name__ == "__main__":
    unittest.main()
