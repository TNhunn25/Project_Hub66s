#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <Arduino.h>

enum LedState {
  NORMAL_STATUS,      // LED sáng liên tục 
  CONNECTION_ERROR,
  BLINK_CONFIRM       // Trạng thái chớp LED để xác nhận cấu hình 
};

class LedStatus {
private:
  int pin;
  LedState state;
  unsigned long lastBlinkTime;
  bool blinkState;
  int blinkCount;           // Đếm số lần chớp
  int maxBlinkCount;        // Số lần chớp tối đa
  const int BLINK_INTERVAL = 200; // Thời gian chớp (ms)

public:
  LedStatus(int ledPin, int maxBlinks = 3) 
    : pin(ledPin), state(CONNECTION_ERROR), lastBlinkTime(0), 
      blinkState(false), blinkCount(0), maxBlinkCount(maxBlinks) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // Tắt LED mặc định 
  }

  void setState(LedState newState, int blinks = 3) {
    state = newState;
    maxBlinkCount = blinks; // Cập nhật số lần chớp nếu chỉ định
    if (state != BLINK_CONFIRM) {
      digitalWrite(pin, state == NORMAL_STATUS ? HIGH : LOW);
      blinkState = false;
      blinkCount = 0;
    } else {
      blinkState = true; // Bắt đầu chớp
      blinkCount = 0;    // Reset số lần chớp
      lastBlinkTime = millis(); 
      digitalWrite(pin, HIGH); // Bật LED lần đầu
    }
  }

  void update() {
    if (state == BLINK_CONFIRM) {
      unsigned long currentTime = millis();
      if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        blinkState = !blinkState; // Đảo trạng thái LED
        digitalWrite(pin, blinkState ? HIGH : LOW);
        lastBlinkTime = currentTime;

        // Tăng số lần chớp khi chuyển từ HIGH sang LOW
        if (!blinkState) {
          blinkCount++;
          if (blinkCount >= maxBlinkCount * 2) { // *2 vì mỗi chu kỳ có HIGH và LOW
            setState(NORMAL_STATUS); // Chuyển về trạng thái bình thường
          }
        }
      }
    }
  }
};

#endif