
   #ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>

#include "stdio.h"
#include <esp_display_panel.hpp>

#include <MD5Builder.h>
#include "led_status.h"
#include <painlessMesh.h>
#include <Preferences.h> // Thư viện để lưu trữ dữ liệu không mất khi tắt nguồn

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "ui.h"
#include "function.h"
#include "protocol_handler.h"
using namespace esp_panel::drivers;
using namespace esp_panel::board;
Licence datalic;
int Device_ID;

uint32_t timer_out = 0;
bool enable_print_ui = false;
bool enable_print_ui_set = false;

lv_timer_t *timer = NULL;

uint32_t mac_nhan = 0; // MAC nhận, mặc định là 0 (broadcast)

// ==== Mesh configuration ====
#define MESH_SSID "Hub66sMesh"
#define MESH_PASSWORD "mesh_pass_456"
#define MESH_PORT 5555
#define MESH_CHANNEL 2

#define MASTER_ID 1001 // ID chính của sender

#define maxLinesPerPage 5

// ==== Opcodes ====
#define LIC_TIME_GET 0x01
#define LIC_SET_LICENSE 0x02
#define LIC_GET_LICENSE 0x03
#define LIC_LICENSE_DELETE 0x04
#define LIC_LICENSE_DELETE_ALL 0x05
#define LIC_INFO 0x06
#define LIC_INFO_RESPONSE 0x80

// ==== JSON buffer sizes ====
#define BUFFER_SIZE 512 // buffer để đọc JSON từ PC
// ==== MD5 key ====
#define private_key "khoabi_mat_123"

// ==== LED pin (nếu dùng) ====
#define LED_PIN 2

// Dữ liệu license (nếu cần lưu lại)
typedef struct
{
    int lid;     // License ID
    int id;      // ID của thiết bị
    String license; // Nội dung license
    time_t created;
    time_t expired;
    int duration;
    int remain;
    bool expired_flag; // Đã hết hạn hay chưa
    String deviceName; // Tên thiết bị
    String version;
} LicenseInfo;

typedef struct
{
    char payload[250];
} PayloadStruct;

// Hàm mã hóa Auth MD5
String md5Hash(int id_src, int id_des, uint32_t mac_src, uint32_t mac_des, uint8_t opcode, const String &data,
               unsigned long timestamp)
{

    MD5Builder md5;
    md5.begin();
    md5.add(String(id_src));
    md5.add(String(id_des));
    md5.add(String(mac_src, HEX));
    md5.add(String(mac_des, HEX));
    md5.add(String(opcode));
    md5.add(data);
    md5.add(String(timestamp));
    md5.add(private_key);
    md5.calculate();
    return md5.toString(); // Trả về chuỗi MD5 hex
}

// Lấy địa chỉ MAC của thiết bị
String getDeviceMacAddress()
{
    return WiFi.macAddress();
}

// JSON payload từ PC

extern bool config_received;

// Biến toàn cục
extern LedStatus led;
extern LicenseInfo globalLicense;
extern PayloadStruct message;
extern int config_lid;
extern int config_id; // id_src
extern int id_des;    // id_des
extern bool config_processed;
extern char jsonBuffer[BUFFER_SIZE];
extern int bufferIndex;
extern time_t start_time;
extern const int duration;
extern bool expired_flag;
extern int expired;
extern time_t now;
extern unsigned long lastSendTime;
extern String device_id;
extern uint32_t nod; // Số lượng thiết bị, mặc định là 10

#endif // CONFIG_H
