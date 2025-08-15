#include <WiFi.h>
#include <ArduinoJson.h>

#include "config.h"
#include "led_status.h"
#include "serial.h"
#include "mesh_handler.h"
#include "protocol_handler.h"
#include "watch_dog.h"
#include "led_display.h"

// Biến toàn cục
Preferences preferences;
painlessMesh mesh;
Scheduler userScheduler;
LedStatus led(LED_PIN, /*activeHigh=*/false); // nếu LED nối kiểu active-LOW
Hub66s::LedDisplay ledDisplay; // Khởi tạo đối tượng từ lớp LedDisplay

// Cấu trúc dữ liệu License
LicenseInfo globalLicense;
PayloadStruct message;

// Biến lưu cấu hình
int config_lid = 114;
int config_id = 2002; // ID của HUB66S
int id_des = 1001;    // ID của LIC66S
String device_id = "HUB66S_001";

bool config_processed = false;
char jsonBuffer[BUFFER_SIZE];
int bufferIndex = 0;
bool expired_flag = false;
int expired = 0; // Biến lưu trạng thái hết hạn
unsigned long now;
time_t start_time = 0;
const int duration = 60;
unsigned long lastSendTime = 0;
uint32_t lastPacketNodeId = 0; // Chỉ ở một chỗ duy nhất!

unsigned long runtime = 0;
uint32_t nod = 0; // số lượng thiết bị trong mesh, sẽ được cập nhật ở setup()

bool dang_gui = false; // cờ đang gửi

unsigned long lastTime = 0; // thời điểm gửi lần cuối
uint8_t retries = 0;        // số lần đã thử gửi

uint8_t lastPacketData[250];

// Lưu MAC của gói vừa nhận
uint8_t lastPacketMac[6];
volatile bool hasNewPacket = false; // Lưu độ dài payload
int lastPacketLen;

// Lưu payload (có thể điều chỉnh kích thước tuỳ theo nhu cầu, ở đây bằng tối đa của PayloadStruct)

void xu_ly_dang_gui()
{
  // Chỉ xử lý khi đang trong trạng thái gửi
  if (!dang_gui)
    return;

  unsigned long now = millis();
  // // Chưa đủ 1s kể từ lần gửi trước thì bỏ qua
  if (now - lastTime < 1000)
    return;

  // Đã đủ 1s, cập nhật thời điểm và thử gửi
  lastTime = now;
  Serial.println("📤 Đang gửi gói tin...");
  // gọi hàm truyền data
  // Gửi thất bại, tăng bộ đếm và thử lại
  retries++;
  if (retries >= 3)
  {
    // Đã thử 3 lần mà vẫn fail → dừng gửi
    dang_gui = false;
    retries = 0;
  }
}
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n🌟 HUB66S Receiver Started");
  ledDisplay.begin(); // Khởi tạo module LED hiển thị

  Hub66s::WatchDog::begin(10); // ⭐ khởi tạo WDT 10 s (toàn chip reset khi quá hạn)

  // // Thiết lập WiFi mode cho ESP-NOW
  // WiFi.mode(WIFI_STA); // Enable Wi-Fi in Station mode for ESP-NOW
  // delay(100);
  // WiFi.setTxPower(WIFI_POWER_7dBm);
  configTime(0, 0, "pool.ntp.org"); // Configure NTP for time synchronization

  initMesh(); // Initialize mesh network
  mesh.onReceive(&meshReceivedCallback);
  nod= getConnectedDeviceCount();
  loadLicenseData();
  globalLicense.lid = config_lid;
  globalLicense.nod = nod;

  // Khởi tạo trạng thái expired dựa trên globalLicense
  if (globalLicense.remain > 0 && !globalLicense.expired_flag)
  {
    expired = 0; // Giấy phép còn hạn
  }
  else
  {
    expired = 1; // Giấy phép hết hạn hoặc không hợp lệ
  }
  saveLicenseData();
  led.setState(CONNECTION_ERROR);
}

void loop()
{
  recPC();
  serialPC();
  led.update();
  ledDisplay.update(); // Cập nhật hiển thị màn hình LED
  delay(10); // Giảm tải CPU

  // Cập nhật license, remain, expired, NVS mỗi 1 phút
  unsigned long nowMillis = millis();
  if (nowMillis - lastSendTime > 60000) 
  {
    lastSendTime = nowMillis;
    now = time(nullptr);

    if (globalLicense.lid != 0 && globalLicense.duration > 0)
    {
      runtime++; // tăng thời gian chạy từng phút
      preferences.begin("license", false);
      preferences.putULong("runtime", runtime);
      preferences.end();
      globalLicense.remain = globalLicense.duration > runtime ? globalLicense.duration - runtime : 0; // Ngăn remain âm
      // Kiểm tra license hết hạn
      if (globalLicense.remain <= 0 && !globalLicense.expired_flag)
      {
        globalLicense.expired_flag = true;
        globalLicense.remain = 0;
        expired = 1; // Giấy phép hết hạn
        saveLicenseData(false);
      }
      else if (globalLicense.remain > 0 && globalLicense.expired_flag)
      {
        globalLicense.expired_flag = false;
        expired = 0;
        saveLicenseData(false);
      }
      else
      {
        saveLicenseData(false); // Lưu runtime và remain
      }
    }
    else
    {
      // Giấy phép không hợp lệ
      expired = 1; // Hết hạn
      globalLicense.expired_flag = true;
      globalLicense.remain = 0;
      saveLicenseData(false);
    }

    // Cập nhật LED trạng thái
    if (globalLicense.expired_flag || globalLicense.remain <= 0)
    {
      led.setState(LICENSE_EXPIRED);
    }
    else
    {
      led.setState(NORMAL_STATUS);
    }
  }

  if (hasNewPacket)
  {
    xu_ly_data(lastPacketNodeId, lastPacketData, lastPacketLen);
    hasNewPacket = false;
    dang_gui = true;
    retries = 0;
  }

  xu_ly_dang_gui();
  meshLoop();
  Hub66s::WatchDog::feed();
}