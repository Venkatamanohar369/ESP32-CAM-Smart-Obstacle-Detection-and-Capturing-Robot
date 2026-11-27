import os
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


class TestDashboardAndHttpIntegration(unittest.TestCase):
    def test_dashboard_html_contains_required_controls_and_no_cloud_deps(self):
        path = os.path.join(REPO_ROOT, "software", "web_dashboard", "index.html")
        with open(path, "r", encoding="utf-8") as f:
            html = f.read()
        for required in [
            "LIVE STREAM", "CAPTURED IMAGE", "CAPTURED IMAGE GALLERY",
            'data-dir="F"', 'data-dir="L"', 'data-dir="S"', 'data-dir="R"', 'data-dir="B"',
            "Ultrasonic Distance", "PIR Motion", "OBSTACLE EMERGENCY STOP ACTIVE",
            "/api/status", "/api/move", "/api/capture", "/api/photo", ":81/stream",
        ]:
            self.assertIn(required, html)
        self.assertNotIn("https://cdn.", html)
        self.assertNotIn("http://cdn.", html)

    def test_webserver_validates_inputs(self):
        path = os.path.join(REPO_ROOT, "firmware", "esp32cam", "include", "web_server.h")
        with open(path, "r", encoding="utf-8") as f:
            srv = f.read()
        for ep in ['"/"', '"/api/status"', '"/api/move"', '"/api/capture"', '"/api/buzzer"', '"/api/flash"', '"/api/photo"', '"/stream"']:
            self.assertIn(ep, srv)
        self.assertIn("HTTPD_400_BAD_REQUEST", srv)
        self.assertIn("HTTPD_404_NOT_FOUND", srv)

    def test_telegram_uses_tls_and_placeholder_guard(self):
        path = os.path.join(REPO_ROOT, "firmware", "esp32cam", "include", "telegram_notifier.h")
        with open(path, "r", encoding="utf-8") as f:
            tg = f.read()
        self.assertIn("WiFiClientSecure", tg)
        self.assertIn("setCACert(TELEGRAM_ROOT_CA)", tg)
        self.assertIn("YOUR_TELEGRAM_BOT_TOKEN", tg)
        self.assertIn("multipart/form-data", tg)


if __name__ == "__main__":
    unittest.main()
