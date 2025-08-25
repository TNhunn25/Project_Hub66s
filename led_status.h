#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <Arduino.h>

/* ───── Trạng thái LED ────────────────────────────────────────── */
enum LedState : uint8_t
{
  NORMAL_STATUS,    // 50 ms ON / 950 ms OFF – nhá nhoáng 1 Hz
  CONNECTION_ERROR, // 500 ms ON / 500 ms OFF – lỗi mạng
  BLINK_CONFIRM,    // 100 ms ON / 100 ms OFF – nháy N lần rồi về NORMAL
  FLASH_TWICE,      // 50 ms ON / 200 ms OFF ×2
  LICENSE_EXPIRED   // LED tắt hẳn
};

class LedStatus
{
public: /* <<< dùng được ngoài file */
  explicit LedStatus(uint8_t pin, bool activeHigh = true)
      : pin_(pin), activeHigh_(activeHigh)
  {
    pinMode(pin_, OUTPUT);
    writeLed(false);
    setState(NORMAL_STATUS);
  }

  /** Đặt trạng thái; BLINK_CONFIRM truyền số nháy (default 3) */
  void setState(LedState s, uint8_t blinks = 3)
  {

    // Nếu yêu cầu trùng trạng thái hiện tại thì không làm gì
    if (s == state_ && s != BLINK_CONFIRM)
      return;

    if (state_ == FLASH_TWICE && busy_)
      return; // không cắt FLASH_TWICE

    if (s == BLINK_CONFIRM)
    {
      blinkEdges_ = 0;
      maxBlinkEdges_ = constrain(blinks, 1, 10) * 2; // mỗi nháy 2 cạnh
    }
    if (s == FLASH_TWICE)
    {
      prevState_ = state_;
      flashEdges_ = 0;
      busy_ = true;
    }

    state_ = s;
    // ledOn_ = false;
    lastTick_ = millis();
    // writeLed(true); // bật ngay
  }

  /** Gọi liên tục trong loop() */
  void update()
  {
    unsigned long now = millis();
    uint16_t onT, offT;
    getTiming(state_, onT, offT);

    /* Trạng thái giữ nguyên (ON hoặc OFF) */
    if (onT == 0 && offT == 0)
    {
      writeLed(true);
      return;
    }
    if (offT == 0)
    {
      writeLed(false);
      return;
    }

    uint16_t interval = ledOn_ ? onT : offT;
    if (now - lastTick_ < interval)
      return;

    /* Toggle LED */
    ledOn_ = !ledOn_;
    writeLed(ledOn_);
    lastTick_ = now;

    /* Đếm cạnh cho các chế độ đặc biệt */
    if (state_ == BLINK_CONFIRM && ++blinkEdges_ >= maxBlinkEdges_)
    {
      state_ = NORMAL_STATUS; // quay về bình thường
      return;
    }
    if (state_ == FLASH_TWICE && ++flashEdges_ >= 4)
    { // 2 lần bật
      busy_ = false;
      state_ = prevState_;
    }
  }

  bool isBusy() const { return busy_; } // TRUE khi đang FLASH_TWICE

private:
  const uint8_t pin_;
  const bool activeHigh_;

  LedState state_ = NORMAL_STATUS;
  LedState prevState_ = NORMAL_STATUS;

  unsigned long lastTick_ = 0;
  bool ledOn_ = false;
  bool busy_ = false;

  /* Đếm cạnh */
  uint8_t blinkEdges_ = 0;
  uint8_t maxBlinkEdges_ = 6; // default 3 nháy
  uint8_t flashEdges_ = 0;

  /* Bảng thời gian ON/OFF cho từng trạng thái */
  static void getTiming(LedState st, uint16_t &onT, uint16_t &offT)
  {
    switch (st)
    {
    case NORMAL_STATUS:
    {
      onT = 50;
      offT = 950;
      break;
    }
    case CONNECTION_ERROR:
    {
      onT = 200;
      offT = 50;
      break;
    }
    case BLINK_CONFIRM:
    {
      onT = 100;
      offT = 100;
      break;
    }
    case FLASH_TWICE:
    {
      onT = 50;
      offT = 200;
      break;
    }
    case LICENSE_EXPIRED:
    {
      onT = 500;
      offT = 500;
      break;
    }
    } // ← đóng switch
  }

  inline void writeLed(bool on)
  {
    digitalWrite(pin_, on);
    // ledOn_ = on;
  }
};

#endif /* LED_STATUS_H */

// #ifndef LED_STATUS_H
// #define LED_STATUS_H

// #include <Arduino.h>

// /**
//  * @brief LED status helper with optional active‑LOW wiring.
//  *
//  *  • NORMAL_STATUS     – LED sáng liên tục (thiết bị OK / license còn hạn)
//  *  • CONNECTION_ERROR  – LED tắt liên tục (lỗi mạng hoặc thiết bị)
//  *  • BLINK_CONFIRM     – LED nhấp nháy N lần để xác nhận cấu hình
//  *  • LICENSE_EXPIRED   – LED tắt liên tục khi license hết hạn
//  */

// enum LedState {
//   NORMAL_STATUS,
//   CONNECTION_ERROR,
//   BLINK_CONFIRM,
//   LICENSE_EXPIRED
// };

// class LedStatus {
// private:
//   int   pin;                       // GPIO chân LED
//   bool  activeHigh;                // true  = HIGH -> LED ON
//                                     // false = LOW  -> LED ON (active‑LOW)
//   LedState state;                  // trạng thái hiện tại
//   unsigned long lastBlinkTime;     // mốc thời gian lần nhấp nháy cuối
//   bool  ledLevel;                  // mức logic hiện tại trên chân LED (true = ON)
//   int   blinkCount;                // số lần đã nhấp nháy (LOW edge)
//   int   maxBlinkCount;             // tổng số lần cần nhấp nháy (LOW edge)
//   static constexpr uint16_t BLINK_INTERVAL = 200; // ms giữa các lần toggle

//   /**
//    * @brief Ghi mức logic ra chân LED, tự động đảo theo activeHigh
//    */
//   inline void write(bool on) {
//     digitalWrite(pin, (on ^ !activeHigh) ? HIGH : LOW);
//     ledLevel = on;
//   }

// public:
//   /**
//    * @param ledPin      GPIO dùng làm LED
//    * @param activeHigh  true  → HIGH = LED ON (mặc định)
//    *                    false → LOW  = LED ON (nối LED kiểu đảo cực)
//    * @param maxBlinks   số lần nhấp nháy mặc định cho BLINK_CONFIRM
//    */
//   LedStatus(int ledPin, bool activeHigh = true, int maxBlinks = 3)
//       : pin(ledPin), activeHigh(activeHigh), state(CONNECTION_ERROR),
//         lastBlinkTime(0), ledLevel(false), blinkCount(0),
//         maxBlinkCount(maxBlinks) {
//     pinMode(pin, OUTPUT);
//     write(false);                   // LED OFF mặc định
//   }

//   /**
//    * @brief Đặt trạng thái LED
//    * @param newState  Trạng thái mới
//    * @param blinks    Số lần nhấp nháy (chỉ áp dụng cho BLINK_CONFIRM)
//    */
//   void setState(LedState newState, int blinks = 3) {
//     state = newState;
//     blinkCount = 0;
//     lastBlinkTime = millis();

//     switch (state) {
//       case NORMAL_STATUS:
//         write(true);
//         break;

//       case CONNECTION_ERROR:
//       case LICENSE_EXPIRED:
//         write(false);
//         break;

//       case BLINK_CONFIRM:
//         maxBlinkCount = blinks > 0 ? blinks : 3;
//         write(true);                // bật LED lần đầu
//         break;
//     }
//   }

//   /**
//    * @brief Gọi thường xuyên trong loop() để điều khiển nhấp nháy
//    */
//   void update() {
//     if (state == BLINK_CONFIRM) {
//       unsigned long now = millis();
//       if (now - lastBlinkTime >= BLINK_INTERVAL) {
//         write(!ledLevel);           // đảo trạng thái LED
//         lastBlinkTime = now;

//         if (!ledLevel) {            // chỉ đếm cạnh rơi (LED OFF)
//           ++blinkCount;
//           if (blinkCount >= maxBlinkCount) {
//             setState(NORMAL_STATUS); // trở về sáng liên tục
//           }
//         }
//       }
//     }
//   }

//   /**
//    * @return true nếu LED đang trong chuỗi nhấp nháy BLINK_CONFIRM
//    */
//   bool isBusy() const { return state == BLINK_CONFIRM; }
// };

// #endif // LED_STATUS_H