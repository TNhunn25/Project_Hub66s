#include <WiFi.h>
#include <ArduinoJson.h>

#include "config.h"
#include "led_status.h"
#include "espnow_handler.h"
#include "protocol_handler.h"
#include "serial.h"

// Define Preferences object
Preferences preferences;

LedStatus led(LED_PIN);
LicenseInfo globalLicense;
PayloadStruct message;
int config_lid = 789;
int config_id = 2006; // ID của HUB66S
int id_des = 1001;   // ID của LIC66S
bool config_processed = false;
char jsonBuffer[BUFFER_SIZE];
int bufferIndex = 0;
time_t start_time = 0;
const int duration = 60;
bool expired_flag = false;
int expired = 0;
time_t now;
unsigned long lastSendTime = 0;
String device_id = "HUB66S_001";
bool networkConnected = false;
unsigned long runtime = 0;
uint32_t nod= 10; // số lượng thiết bị giả định 10 

void setup() {
  Serial.begin(115200);
  Serial.println("\n🌟 HUB66S Receiver Started");
  
  WiFi.mode(WIFI_STA); // Enable Wi-Fi in Station mode for ESP-NOW
  initEspNow(); // Initialize ESP-NOW
  configTime(0, 0, "pool.ntp.org"); // Configure NTP for time synchronization
  
  esp_now_register_recv_cb(onReceive); // Register receive callback
  
  loadLicenseData();
  globalLicense.lid = config_lid ;

  led.setState(CONNECTION_ERROR);
}

void loop() {
  recPC();
  serialPC();
  led.update();

  // Đồng bộ thời gian định kỳ để bảo đảm kiểm tra thời gian gói tin 
  unsigned long nowMillis = millis();
  if (nowMillis - lastSendTime > 5000) { //5 giây
    lastSendTime = nowMillis;
    now = time(nullptr);

     if (globalLicense.lid != 0 && globalLicense.duration > 0) { // Changed from !globalLicense.lid.isEmpty()

      int temp =runtime + (millis() / 60000);
      preferences.begin("license", false);
      preferences.putULong("runtime", temp);
      preferences.end();
       globalLicense.remain = globalLicense.duration - temp;

      // Kiểm tra license
       if (globalLicense.remain <= 0) {
         globalLicense.expired_flag = true;
         globalLicense.remain = 0;
         expired = 1;
     }
    }
    led.setState(networkConnected ? NORMAL_STATUS : CONNECTION_ERROR);
  }
}
