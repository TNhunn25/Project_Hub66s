// #ifndef HUB66S_LED_DISPLAY_H
// #define HUB66S_LED_DISPLAY_H

// #include "config.h" // để truy cập globalLicense.expired_flag
// #include <Arduino.h>
// // #include "serial.h"

// namespace Hub66s {

// class LedDisplay {
// public:
//     void begin() {
//         Serial.println(F("🖥️ Khởi tạo điều khiển màn hình LED..."));

//         // Thiết lập chân hoặc màn hình theo loại bạn dùng
//         pinMode(screenPin_, OUTPUT);
//         digitalWrite(screenPin_, LOW); // màn hình ban đầu tắt

//         lastUpdate_ = millis();
//     }

//     // Gọi định kỳ trong loop()
//     void update() {
//         if (!globalLicense.expired_flag) {
//             showNormalDisplay();
//         } else {
//             showExpiredEffect();
//         }
//     }

// private:
//     const uint8_t screenPin_ = 13; // có thể thay bằng chân điều khiển màn hình thật
//     unsigned long lastUpdate_ = 0;
//     uint8_t glitchState_ = 0;

//     // ─────────────── Còn hạn: hiển thị bình thường ───────────────
//     void showNormalDisplay() {
//         static bool isOn = false;
//         if (!isOn) {
//             Serial.println(F("🔓 License còn hạn → bật màn hình LED"));
//             digitalWrite(screenPin_, HIGH); // bật LED / màn hình
//             isOn = true;
//         }

//         // Bạn có thể thêm hiệu ứng chạy chữ, animation ở đây
//         // displayText("Welcome to Event!");
//     }

//     // ─────────────── Hết hạn: hiệu ứng glitch/nhấp nháy ───────────────
//     void showExpiredEffect() {
//         unsigned long now = millis();
//         if (now - lastUpdate_ >= 400) {
//             lastUpdate_ = now;

//             glitchState_ = (glitchState_ + 1) % 4;

//             // Giả lập hiệu ứng lỗi bằng cách nhấp nháy pin
//             switch (glitchState_) {
//                 case 0:
//                     Serial.println(F("🛑 Hết hạn: Sáng vùng trái"));
//                     digitalWrite(screenPin_, HIGH);
//                     break;
//                 case 1:
//                     Serial.println(F("🛑 Hết hạn: Sáng vùng phải"));
//                     digitalWrite(screenPin_, LOW);
//                     break;
//                 case 2:
//                     Serial.println(F("🛑 Hết hạn: Nháy nhanh vùng giữa"));
//                     digitalWrite(screenPin_, HIGH);
//                     break;
//                 case 3:
//                     Serial.println(F("🛑 Hết hạn: Tắt toàn bộ"));
//                     digitalWrite(screenPin_, LOW);
//                     break;
//             }

//             // Bạn có thể thay bằng hiệu ứng LED matrix thật ở đây
//         }
//     }
// };

// } // namespace Hub66s

// #endif // HUB66S_LED_DISPLAY_H

#ifndef HUB66S_LED_DISPLAY_H
#define HUB66S_LED_DISPLAY_H

#include "config.h" // để truy cập globalLicense.expired_flag
#include <Arduino.h>
// #include "serial.h"

namespace Hub66s
{

    class LedDisplay
    {
    public:
        /**
         * @brief Khởi tạo phần cứng điều khiển màn hình LED.
         *        Tất cả các chân từ 1 đến 12 được cấu hình OUTPUT và ở mức LOW.
         */
        void begin()
        {
            Serial.println(F("🖥️ Khởi tạo điều khiển màn hình LED..."));

            // Thiết lập các chân LED và tắt tất cả
            for (uint8_t i = 1; i < 13; ++i)
            {
                pinMode(i, OUTPUT);
                digitalWrite(i, LOW);
            }

            // pinMode(screenPin_, OUTPUT);   // chân điều khiển chính
            // digitalWrite(screenPin_, LOW); // màn hình ban đầu tắt

            lastFlash_ = millis();
            lastRandom_ = millis();
        }

        /**
         * @brief Gọi định kỳ trong loop().
         *        Nếu license còn hạn → nháy các chân flashPins_[].
         *        Nếu hết hạn          → hiển thị hiệu ứng randomRGB.
         */
        void update()
        {
            if (!globalLicense.expired_flag)
            {
                for (uint8_t i = 1; i < 13; ++i)
                {
                    pinMode(i, OUTPUT);
                    digitalWrite(i, LOW);
                }
            }
            else
            {
                randomRGB();
            }
        }

    private:
        // const uint8_t screenPin_ = 13;                           // Có thể thay bằng chân điều khiển màn hình thật
        const uint8_t flashPins_[4] = {8, 10, 1, 4};             // Các chân nhấp nháy khi còn hạn
        const uint8_t groupRGB_[8] = {7, 9, 12, 11, 3, 2, 6, 5}; // Nhóm RGB ngẫu nhiên khi hết hạn

        unsigned long lastFlash_ = 0;
        bool flashState_ = false;
        unsigned long lastRandom_ = 0;

        /**
         * @brief Hiệu ứng nhấp nháy các chân flashPins_ mỗi giây.
         */
        void flashPinData()
        {
            unsigned long now = millis();
            if (now - lastFlash_ >= 1000)
            {
                lastFlash_ = now;
                flashState_ = !flashState_;
                for (uint8_t pin : flashPins_)
                {
                    digitalWrite(pin, flashState_ ? HIGH : LOW);
                }
            }
        }

        /**
         * @brief Hiển thị hiệu ứng ngẫu nhiên trên groupRGB_ mỗi 500 ms.
         */
        void randomRGB()
        {
            unsigned long now = millis();
            if (now - lastRandom_ >= 500)
            {
                lastRandom_ = now;
                for (uint8_t pin : groupRGB_)
                {
                    digitalWrite(pin, random(0, 2));
                }
            }
            Serial.println(F("🛑 Hết hạn: nhấp nháy RGB"));
            delay(1000);
        }
    };

} // namespace Hub66s

#endif // HUB66S_LED_DISPLAY_H
