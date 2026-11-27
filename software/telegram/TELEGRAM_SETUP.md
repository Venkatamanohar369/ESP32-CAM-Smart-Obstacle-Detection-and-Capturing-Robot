# Telegram Bot API Setup Guide

## 1. Create a Bot with `@BotFather`
1. Open Telegram and message **`@BotFather`**.
2. Send `/newbot`, choose a display name and a username ending in `bot`.
3. Copy the **Bot Token** (format: `123456789:ABCdef...`).

## 2. Obtain Your Chat ID
1. Start a chat with your new bot and send `/start` (or add the bot to a group).
2. Message `@userinfobot` (or fetch `https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates`) to read your numeric `chat.id`.

## 3. Configure Locally (Never Commit Secrets)
1. Copy `config/config.example.h` to **`config/config.h`** (which is ignored by `.gitignore`).
2. Update:
   ```cpp
   #define TELEGRAM_ENABLED    true
   #define TELEGRAM_BOT_TOKEN  "YOUR_REAL_BOT_TOKEN"
   #define TELEGRAM_CHAT_ID    "YOUR_REAL_CHAT_ID"
   ```
3. **TLS Verification:** `firmware/esp32cam/include/telegram_notifier.h` uses `WiFiClientSecure` with `TELEGRAM_ROOT_CA` (`GoDaddy Root Certificate Authority - G2`). Keep `TELEGRAM_ALLOW_INSECURE_TLS` as `false` for normal use.
4. **Rate Limiting & Error Handling:** Uploads run asynchronously on Core 0 with `TELEGRAM_COOLDOWN_MS` (default `15000 ms`) and `TELEGRAM_TIMEOUT_MS` (`10000 ms`). Failed HTTPS connections log to Serial and update the dashboard `TG:` status badge without blocking motor control or streaming.
