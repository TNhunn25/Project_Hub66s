#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <Arduino.h>

// Định nghĩa trạng thái LED
enum LedState {
    NORMAL_STATUS,
    CONNECTION_ERROR,
    CUSTOM_STATUS // có thể mở rộng thêm các trạng thái khác
};

class LedStatus {
private:
    uint8_t ledPin;              // Chân kết nối LED
    LedState currentState;       // Trạng thái hiện tại
    unsigned long previousMillis; // Thời gian lưu lần chớp trước
    bool ledOn;                  // Trạng thái hiện tại của LED

    // Hàm lấy thời gian sáng và tắt tương ứng với trạng thái
    void getBlinkTiming(LedState state, unsigned long &onTime, unsigned long &offTime) {
        switch (state) {
            case NORMAL_STATUS:
                onTime = 50;
                offTime = 950;
                break;
            case CONNECTION_ERROR:
                onTime = 500;
                offTime = 500;
                break;
            case CUSTOM_STATUS:
                onTime = 100;
                offTime = 100;
                break;
            default:
                onTime = 1000;
                offTime = 0;
                break;
        }
    }

public:
    LedStatus(uint8_t pin) {
        ledPin = pin;
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
        currentState = NORMAL_STATUS;
        previousMillis = 0;
        ledOn = false;
    }

    // Hàm đặt trạng thái hiện tại
    void setState(LedState state) {
        currentState = state;
    }

    // Hàm gọi trong loop() để xử lý chớp LED
    void update() {
        unsigned long currentMillis = millis();
        unsigned long onTime, offTime;
        getBlinkTiming(currentState, onTime, offTime);

        if (ledOn && (currentMillis - previousMillis >= onTime)) {
            ledOn = false;
            previousMillis = currentMillis;
            digitalWrite(ledPin, LOW);
        } else if (!ledOn && (currentMillis - previousMillis >= offTime)) {
            ledOn = true;
            previousMillis = currentMillis;
            digitalWrite(ledPin, HIGH);
        }
    }
};

#endif
