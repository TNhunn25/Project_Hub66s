#include <Arduino.h>
#include "stdio.h"
#include <esp_display_panel.hpp>
#include "WiFi.h"
#include "ESP32_NOW.h"
#include <ArduinoJson.h>
// #include "mbedtls/md5.h"
#include <MD5Builder.h>
#include <time.h>
#include "led_status.h"

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "ui.h"
#include "function.h"
using namespace esp_panel::drivers;
using namespace esp_panel::board;
Licence datalic;
int Device_ID;
uint8_t button = 0;
uint32_t timer_out = 0;
bool enable_print_ui = false;
bool enable_print_ui_set = false;

lv_timer_t *timer = NULL;

char messger[100];
#define LED_PIN 2 // Chân LED báo trạng thái

LedStatus led(LED_PIN); // LED nối chân 2

#define LIC_TIME_GET 0x01
#define LIC_SET_LICENSE 0x02        // tạo bản tin license
#define LIC_GET_LICENSE 0x03        // đọc trạng thái license của Hub66s
#define LIC_LICENSE_DELETE 0x04     // xóa 1 license
#define LIC_LICENSE_DELETE_ALL 0x05 // xóa tất cả license
#define LIC_INFO 0x06               // cập nhật thông tin Lic66s
#define LIC_INFO_RESPONSE 0x80      // 0x06 | 0x80

// biến dùng trong hàm nhận dữ liệu json từ pc
const int BUFFER_SIZE = 512; // Tăng kích thước buffer để chứa JSON lớn
char jsonBuffer[BUFFER_SIZE];
int bufferIndex = 0;
//-----------------------------------------------

uint8_t receiverMac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; // Broadcast
const char *private_key = "khoabi_mat_123";

String getDeviceMacAddress()
{
  // Lấy địa chỉ Mac của Wifi station (client)
  return WiFi.macAddress();
}

typedef struct
{
  String lid;     // License ID
  String license; // Nội dung license
  time_t created;
  time_t expired;
  int duration;
  int remain;
  bool exired_flag;  // Đã hết hạn chưa
  String deviceName; // Tên thiết bị
  String version;
} LicenseInfo;
LicenseInfo globalLicense;

typedef struct
{
  char payload[250];
} PayloadStruct;

PayloadStruct message;

// Biến lưu cấu hình
int config_lid = 123;
int config_id = 2025;
bool config_received = false;
#define maxLinesPerPage 5
int next_page = 0;
int old_page = 0;


// Biến LED
bool errorState = false;
unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = LOW;

// Biến thời gian license
time_t start_time = 0;
const int duration = 60; // 60 phút

// Biến expired
bool expired_flag = false; // Biến logic kiểm soát trạng thái
int expired = 1;           // 0 = chưa hết hạn, 1 = hết hạn
device_info Device;

// Hàm mã hóa Auth MD5
String md5Hash(int id_src, int id_des, String mac_src, String mac_des, uint8_t opcode, String data, unsigned long timestamp)
{
  MD5Builder md5;
  md5.begin();
  md5.add(String(id_src));
  md5.add(String(id_des));
  md5.add(mac_src);
  md5.add(mac_des);
  md5.add(String(opcode));
  md5.add(data);
  md5.add(String(timestamp));
  md5.add(private_key);
  md5.calculate();
  return md5.toString(); // Trả về chuỗi MD5 hex
}
// Kiểm tra xem MAC đã tồn tại trong danh sách chưa
bool isMacExist(const uint8_t *mac_addr)
{
  for (int i = 0; i < Device.deviceCount; i++)
  {
    if (memcmp(Device.MACList[i], mac_addr, 6) == 0)
    {
      return true; // MAC đã tồn tại
    }
  }
  return false;
}

// Thêm MAC vào danh sách
void addMacToList(int id, int lid, const uint8_t *mac_addr, unsigned long time_)
{
  if (Device.deviceCount < MAX_DEVICES && !isMacExist(mac_addr))
  {
    memcpy(Device.MACList[Device.deviceCount], mac_addr, 6);

    Device.DeviceID[Device.deviceCount] = id;
    Device.LocalID[Device.deviceCount] = lid;
    Device.timeLIC[Device.deviceCount] = time_;

    Device.deviceCount++;
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Thiết bị mới: ");
    Serial.println(macStr);
    // timer_out=millis();
    
    // lv_timer_reset(timer);
  }
}

// Hiển thị danh sách thiết bị đã lưu
void printDeviceList()
{
  Serial.println("Danh sách thiết bị đã tìm thấy:");
  for (int i = 0; i < Device.deviceCount; i++)
  {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             Device.MACList[i][0], Device.MACList[i][1], Device.MACList[i][2],
             Device.MACList[i][3], Device.MACList[i][4], Device.MACList[i][5]);
    Serial.print("Thiết bị ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(macStr);
  }
  Serial.println("------------------");
}

//Xử lý khi nhận phản hồi 
void handleScanResponse(const uint8_t *mac, int node_id, int local_id)
{
    if (!isMacExist(mac))
    {
        addMacToList(node_id, local_id, mac, millis());
    }
    else
    {
        Serial.println("Thiết bị đã tồn tại, bỏ qua phản hồi");
    }
}

//Gửi gói Reset để tất cả thiết bị phản hồi lại 
// void sendResetScanState()
// {
//     send_broadcast_packet(RESET_SCAN_STATE);
//     Serial.println("Đã gửi RESET_SCAN_STATE tới tất cả thiết bị ");
// }

// bool reported_before = false; // Đánh dấu đã từng phản hồi

//Nhận gói Scan
// void onScanRequest()
// {
//     if (!reported_before)
//     {
//         sendScanResponse(); // Gửi phản hồi về Master
//         reported_before = true;
//         Serial.println("Đã phản hồi SCAN");
//     }
//     else
//     {
//         Serial.println("Đã từng phản hồi trước đó, bỏ qua");
//     }
// }

//Rescan
// void onResetScanState()
// {
//     reported_before = false; // Reset trạng thái để phản hồi lại lần scan tiếp theo
//     Serial.println("Reset trạng thái: sẽ phản hồi SCAN tiếp theo");
// }


String createMessage(int id_src, int id_des, String mac_src, String mac_des, uint8_t opcode, DynamicJsonDocument data, unsigned long timestamp = 0)
{
  if (timestamp == 0)
  {
    timestamp = millis() / 1000; // Giả lập Unix time (cần đồng bộ thực tế)
  }

  String dataStr;
  serializeJson(data, dataStr);
  String auth = md5Hash(id_src, id_des, mac_src, mac_des, opcode, dataStr, timestamp);

  DynamicJsonDocument message(512);
  message["id_src"] = id_src;
  message["id_des"] = id_des;
  message["mac_src"] = mac_src;
  message["mac_des"] = mac_des;
  message["opcode"] = opcode;
  message["data"] = data;
  message["time"] = timestamp;
  message["auth"] = auth;

  String messageStr;
  serializeJson(message, messageStr);
  return messageStr;
}

void processReceivedData(StaticJsonDocument<512> message, const uint8_t *mac_addr)
{
  int id_src = message["id_src"];
  int id_des = message["id_des"];
  String mac_src = message["mac_src"];
  String mac_des = message["mac_des"];
  uint8_t opcode = message["opcode"];
  String dataStr;
  serializeJson(message["data"], dataStr);
  unsigned long timestamp = message["time"];
  String receivedAuth = message["auth"];

  String calculatedAuth = md5Hash(id_src, id_des, mac_src, mac_des, opcode, dataStr, timestamp);

  if (!receivedAuth.equalsIgnoreCase(calculatedAuth))
  {
    Serial.println("Odd - failing MD5 on String");
    return;
  }
  else
  {
    Serial.println("OK!");
  }

  serializeJson(message, Serial);

  switch (opcode)
  {

  case LIC_SET_LICENSE | 0x80:
  {
    JsonObject data = message["data"];
    int lid = data["lid"];
    int Status = data["status"];
    const char *error_msg = data["error_msg"].as<const char *>();

    sprintf(messger, "Status: %d \nLocal ID: %d\n", Status, lid);
    if (error_msg != NULL)
    {
      strcat(messger, "Lỗi: ");
      strcat(messger, error_msg);
      /* code */
    }

    // enable_print_ui_set=true;
    // timer_out=millis();
    Serial.println("== Đã nhận phản hồi Data Object ==");
    Serial.print("LID: ");
    Serial.println(lid);
    Serial.print("Status: ");
    Serial.println(Status);
    break;
  }

  case LIC_GET_LICENSE | 0x80:
  {
    Serial.println("Đã nhận phản hồi HUB_GET_LICENSE:");

    JsonObject data = message["data"];
    int lid = data["lid"];
    unsigned long time_temp = data["remain"];

    addMacToList(id_src, lid, mac_addr, time_temp);
    // printDeviceList();
    // Serial.println(data["license"].as<String>());
    break;
  }

  default:
    // if (opcode != 0x83) {  // Bỏ qua opcode 0x83
    Serial.printf("Unknown opcode: 0x%02X\n", opcode);
    // }
    break;
  }
}

void print_mac(const uint8_t *mac)
{
  // Hàm in địa chỉ MAC từ một mảng 6 byte
  for (int i = 0; i < 6; i++)
  {
    if (mac[i] < 0x10)
    {
      Serial.print("0"); // Thêm số 0 nếu byte < 16 (để đảm bảo 2 chữ số)
    }
    Serial.print(mac[i], HEX); // In byte dưới dạng thập lục phân
    if (i < 5)
    {
      Serial.print(":"); // Thêm dấu : giữa các byte, trừ byte cuối
    }
  }
  Serial.println(); // Xuống dòng sau khi in
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " Success" : " Fail");
}
void onReceive(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len)
{

  const uint8_t *mac_addr = recv_info->src_addr;
  Serial.print("\n📩 Received response:");
  Serial.println(strlen((const char *)incomingData));
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, incomingData, len);

  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    // blinkErrorLED();
    return;
  }
  processReceivedData(doc, mac_addr);
}

void addPeer(uint8_t *macAddr)
{
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macAddr, 6); // FF:FF:FF:FF:FF:FF
  peerInfo.channel = 1;                   // Kênh cố định để đồng bộ với sender
  peerInfo.encrypt = false;               // tạm thời tắt mã hóa
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("❌ Failed to add peer!");
  }
  else
    Serial.println("add peer ok");
}

void setup()
{
  String title = "LVGL porting example";

  Serial.begin(115200);

  Serial.println("Initializing board");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("❌ ESP-NOW init failed!");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6); // FF:FF:FF:FF:FF:FF
  peerInfo.channel = 1;                       // Kênh cố định để đồng bộ với sender
  peerInfo.encrypt = false;                   // tạm thời tắt mã hóa
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("❌ Failed to add peer!");
  }
  else
    Serial.println("add peer ok");

  configTime(0, 0, "pool.ntp.org");

  led.setState(CONNECTION_ERROR);

  Board *board = new Board();
  board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
  auto lcd = board->getLCD();
  // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
  lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
  auto lcd_bus = lcd->getBus();
  /**
   * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
   * "bounce buffer" functionality to enhance the RGB data bandwidth.
   * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
   */
  if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
  {
    static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
  }
#endif
#endif
  assert(board->begin());

  Serial.println("Initializing LVGL");
  lvgl_port_init(board->getLCD(), board->getTouch());

  Serial.println("Creating UI");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  lvgl_port_lock(-1);
  // Khởi tạo ui.
  ui_init();
  /* Release the mutex */
  lvgl_port_unlock();
}
bool ledstt = 1;
time_t now;
unsigned long nowMillis = millis();
static unsigned long lastSendTime = 0;
void loop()
{

  // lv_timer_handler();
  // Serial.println("IDLE loop");
  // delay(1000);
  // if (datalic.expired)
  // {
  //     Serial.printf("Local ID: %d\n\r",datalic.lid);
  //     Serial.printf("Device ID: %d\n\r",Device_ID);
  //     Serial.printf("Duration: %d\n\r",datalic.duration);
  //     datalic.expired=false;
  // }
  // }
  led.update(); // Gọi liên tục trong loop()

  if (button != 0)
  {
        Serial.println("button pressed: ");
        Serial.println(button);
    switch (button)
    {
    case 1:
      set_license(Device_ID, datalic.lid, WiFi.macAddress(), millis(), datalic.duration, 1, 1, millis());
      break;
    case 4:
      getlicense(Device_ID, WiFi.macAddress(), datalic.lid, millis());
      // enable_print_ui=true;
      // timer_out=millis();
      break;
    case 5:
      memset(&Device, 0, sizeof(Device));
      getlicense(Device_ID, WiFi.macAddress(), datalic.lid, millis());
      // enable_print_ui=true;
      // timer_out=millis();
      break;
    default:
      break;
    }
    button = 0;
  }
  delay(10);

  if (enable_print_ui_set)
  {

    // if(millis()-timer_out>1000){
    lv_obj_t *OBJ_Notification = add_Notification(ui_SCRSetLIC, messger);
    enable_print_ui_set = false;
    // }
  }

  if (enable_print_ui || (old_page != next_page))
  {

    // if(millis()-timer_out>5000){
    // if (ui_spinner1 != NULL) {
    // lv_anim_del(ui_spinner1, NULL);
    // lv_obj_del(ui_spinner1); // Xóa đối tượng spinner
    // ui_spinner1 = NULL; // Đặt con trỏ về NULL để tránh tham chiếu sai
    // }
    lv_obj_clean(ui_Groupdevice);
    lv_obj_invalidate(ui_Groupdevice);
    
    // lv_obj_invalidate(lv_scr_act());
    if ((next_page * maxLinesPerPage) >= Device.deviceCount)
    {
      next_page = 0; // Quay về trang đầu
    }
    old_page = next_page;
    char buf[64];
    snprintf(buf, sizeof(buf), "LIST DEVICE: %2d - Page %2d", Device.deviceCount + 1, next_page +1 );
    Serial.printf("BUF= %s\n", buf);
    lv_label_set_text(ui_Label7, buf);
    int startIdx = next_page * maxLinesPerPage;
    int endIdx = startIdx + maxLinesPerPage;
    if (endIdx > Device.deviceCount)
    {
      endIdx = Device.deviceCount;
    }
    Serial.printf( "%d %d %d \n " , startIdx, endIdx, next_page);
    for (int i = startIdx ; i < endIdx; i++)
    {
      char macStr[18];
      char idStr[18];
      char lidStr[18];
      char timeStr[18];

      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               Device.MACList[i][0], Device.MACList[i][1], Device.MACList[i][2],
               Device.MACList[i][3], Device.MACList[i][4], Device.MACList[i][5]);

      snprintf(idStr, sizeof(idStr), "%d", Device.DeviceID[i]);
      snprintf(lidStr, sizeof(lidStr), "%d", Device.LocalID[i]);
      snprintf(timeStr, sizeof(timeStr), "%d", Device.timeLIC[i]);

      lv_obj_t *ui_DeviceINFO = ui_DeviceINFO1_create(ui_Groupdevice, idStr, lidStr, "****", macStr, timeStr);
    }
    enable_print_ui = false;
    // }
  }
}

// Gửi HUB_SET_LICENSE
/*
lid: local id
*/
void set_license(int id_des, int lid, String mac_des, time_t created, const int duration, int remain, int expired, unsigned long now)
{
  int opcode = LIC_SET_LICENSE;
  String mac = WiFi.macAddress();
  int id_src = config_id;
  DynamicJsonDocument dataDoc(256);

  dataDoc["lid"] = lid;
  dataDoc["created"] = created;
  dataDoc["duration"] = duration;
  dataDoc["remain"] = remain;
  dataDoc["expired"] = expired;

  String output = createMessage(id_src, id_des, mac, mac_des, opcode, dataDoc, now);

  if (output.length() > sizeof(message.payload))
  {
    Serial.println("❌ Payload quá lớn!");
    return;
  }

  output.toCharArray(message.payload, sizeof(message.payload));
  esp_now_send(receiverMac, (uint8_t *)&message, sizeof(message));

  Serial.println("\n📤 Gửi HUB_SET_LICENSE:");
  Serial.println(output);
}

// Gửi HUB_GET_LICENSE
void getlicense(int id_des, String mac_des, int lid, unsigned long now)
{
  int opcode = LIC_GET_LICENSE;
  String mac = WiFi.macAddress();
  int id_src = config_id;
  DynamicJsonDocument dataDoc(128);
  dataDoc["lid"] = lid;
  String output = createMessage(id_src, id_des, mac, mac_des, opcode, dataDoc, now);

  if (output.length() > sizeof(message.payload))
  {
    Serial.println("❌ Payload quá lớn!");
    return;
  }

  output.toCharArray(message.payload, sizeof(message.payload));
  esp_now_send(receiverMac, (uint8_t *)&message, sizeof(message));

  Serial.println("📤 Gửi HUB_GET_LICENSE:");
  Serial.println(output);
}
// nhận id và local id từ pc
// void rec_PC()
// {
//   if (Serial.available()) {
//       String input = Serial.readStringUntil('\n');
//       input.trim();

//       if (input.length() == 0) {
//         Serial.println("Không nhập id hoặc lid, dùng mặc định.");
//         config_received = true;
//         return;
//       }

//       int sepIndex = input.indexOf(' ');
//       if (sepIndex == -1) sepIndex = input.indexOf(',');

//       if (sepIndex == -1) {
//         Serial.println("Sai định dạng nhập. Nhập dạng: id lid");
//         return;
//       }

//       String idStr = input.substring(0, sepIndex);
//       String lidStr = input.substring(sepIndex + 1);
//       idStr.trim();
//       lidStr.trim();

//       if (idStr.length() > 0 && lidStr.length() > 0) {
//         config_id = idStr;
//         config_lid = lidStr;
//         Serial.print("Đã cấu hình id = ");
//         Serial.println(config_id);
//         Serial.print("Đã cấu hình lid = ");
//         Serial.println(config_lid);
//         config_received = true;
//       } else {
//         Serial.println("Giá trị id hoặc lid không hợp lệ, vui lòng nhập lại.");
//       }
//     }
// }

// nhận chuỗi json từ pc
void serial_pc()
{
  while (Serial.available() > 0)
  {
    char incomingChar = Serial.read();

    if (bufferIndex < BUFFER_SIZE - 1)
    {
      jsonBuffer[bufferIndex++] = incomingChar;
    }

    // Kiểm tra kết thúc chuỗi bằng '\n'
    if (incomingChar == '\n')
    {
      jsonBuffer[bufferIndex] = '\0'; // Kết thúc chuỗi

      Serial.println("Đã nhận JSON:");
      Serial.println(jsonBuffer);

      // Parse JSON
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, jsonBuffer);

      if (error)
      {
        Serial.print("Lỗi JSON: ");
        Serial.println(error.f_str());
      }
      else
      {
        // Lấy trường ngoài
        const char *id = doc["id"];
        const char *mac = doc["mac"];
        int opcode = doc["opcode"];
        long time = doc["time"];
        const char *auth = doc["auth"];

        // Lấy object data bên trong
        JsonObject data = doc["data"];
        long lid = data["lid"];
        long created = data["created"];
        long expired = data["expired"];
        long duration = data["duration"];

        // In dữ liệu nhận được
        Serial.print("ID: ");
        Serial.println(id);
        Serial.print("MAC: ");
        Serial.println(mac);
        Serial.print("Opcode: ");
        Serial.println(opcode);
        Serial.print("Time: ");
        Serial.println(time);
        Serial.print("Auth: ");
        Serial.println(auth);
        Serial.println("== Data Object ==");
        Serial.print("LID: ");
        Serial.println(lid);
        Serial.print("Created: ");
        Serial.println(created);
        Serial.print("Expired: ");
        Serial.println(expired);
        Serial.print("Duration: ");
        Serial.println(duration);

        // So sánh opcode
        Serial.print("Xử lý opcode: ");
        switch (opcode)
        {
        case 1: // LIC_TIME_GET
          Serial.println("LIC_TIME_GET (Yêu cầu PC gửi gói tin LIC_TIME)");
          break;
        case 2: // HUB_SET_LICENSE
          Serial.println("HUB_SET_LICENSE (Tạo bản tin LICENSE)");

          break;
        case 3: // HUB_GET_LICENSE
          // getlicense(config_lid,WiFi.macAddress(),now);
          Serial.println("HUB_GET_LICENSE (Đọc trạng thái license của HUB66S)");
          break;
        case 4: // LIC_LICENSE_DELETE
          Serial.println("LIC_LICENSE_DELETE (Xóa bản tin đã gửi)");
          break;
        case 5: // LIC_LICENSE_DELETE_ALL
          Serial.println("LIC_LICENSE_DELETE_ALL (Xóa toàn bộ các bản tin)");
          break;
        case 6: // LIC_INFO
          Serial.println("LIC_INFO (Cập nhật thông tin LIC66S)");
          break;
        default:
          Serial.println("Không xác định opcode!");
          break;
        }
      }

      // Reset buffer
      bufferIndex = 0;
    }
  }
}
