#ifndef HUB66S_LED_DISPLAY_H
#define HUB66S_LED_DISPLAY_H

#include <Arduino.h>
#include "config.h" // để truy cập globalLicense.expired_flag

namespace Hub66s {

class LedDisplay {
public:
    void begin() {
        Serial.println(F("🖥️ Khởi tạo điều khiển màn hình LED..."));

        // Thiết lập chân hoặc màn hình theo loại bạn dùng
        pinMode(screenPin_, OUTPUT);
        digitalWrite(screenPin_, LOW); // màn hình ban đầu tắt

        lastUpdate_ = millis();
    }

    // Gọi định kỳ trong loop()
    void update() {
        if (!globalLicense.expired_flag) {
            showNormalDisplay();
        } else {
            showExpiredEffect();
        }
    }

private:
    const uint8_t screenPin_ = 13; // có thể thay bằng chân điều khiển màn hình thật
    unsigned long lastUpdate_ = 0;
    uint8_t glitchState_ = 0;

    // ─────────────── Còn hạn: hiển thị bình thường ───────────────
    void showNormalDisplay() {
        static bool isOn = false;
        if (!isOn) {
            Serial.println(F("🔓 License còn hạn → bật màn hình LED"));
            digitalWrite(screenPin_, HIGH); // bật LED / màn hình
            isOn = true;
        }

        // Bạn có thể thêm hiệu ứng chạy chữ, animation ở đây
        // displayText("Welcome to Event!");
    }

    // ─────────────── Hết hạn: hiệu ứng glitch/nhấp nháy ───────────────
    void showExpiredEffect() {
        unsigned long now = millis();
        if (now - lastUpdate_ >= 400) {
            lastUpdate_ = now;

            glitchState_ = (glitchState_ + 1) % 4;

            // Giả lập hiệu ứng lỗi bằng cách nhấp nháy pin
            switch (glitchState_) {
                case 0:
                    Serial.println(F("🛑 Hết hạn: Sáng vùng trái"));
                    digitalWrite(screenPin_, HIGH);
                    break;
                case 1:
                    Serial.println(F("🛑 Hết hạn: Sáng vùng phải"));
                    digitalWrite(screenPin_, LOW);
                    break;
                case 2:
                    Serial.println(F("🛑 Hết hạn: Nháy nhanh vùng giữa"));
                    digitalWrite(screenPin_, HIGH);
                    break;
                case 3:
                    Serial.println(F("🛑 Hết hạn: Tắt toàn bộ"));
                    digitalWrite(screenPin_, LOW);
                    break;
            }

            // Bạn có thể thay bằng hiệu ứng LED matrix thật ở đây
        }
    }
};

} // namespace Hub66s

#endif // HUB66S_LED_DISPLAY_H
