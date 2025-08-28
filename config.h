#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <MD5Builder.h>
#include "led_status.h"
#include <Preferences.h> // Thư viện để lưu trữ dữ liệu không mất khi tắt nguồn

// ==== Chân LED hiển thị trạng thái ====
#define LED_PIN 46   // Chân GPIO kết nối LED
 
// Định nghĩa các opcode
#define LIC_TIME_GET 0x01
#define LIC_SET_LICENSE 0x02
#define LIC_GET_LICENSE 0x03
#define LIC_LICENSE_DELETE 0x04
#define LIC_LICENSE_DELETE_ALL 0x05
#define LIC_INFO 0x06
#define CONFIG_DEVICE 0x07
#define CONFIG_RESET 0x08

#define LIC_INFO_RESPONSE 0x80

// Kích thước buffer cho JSON
#define BUFFER_SIZE 250

// Khóa bí mật
#define private_key "khoabi_mat_123"

// Thông số mặc định cho mạng mesh
#define MESH_SSID "Hub66sMesh"
#define MESH_PASSWORD "mesh_pass_456"
#define MESH_PORT 5555
#define MESH_CHANNEL 6 // Kênh WiFi cho mesh trùng với sender


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

// Cấu trúc dữ liệu
typedef struct
{
    int lid; // License ID
    int id;
    String license; // Nội dung license
    time_t created;
    time_t expired;
    uint32_t duration;
    uint32_t remain;
    bool expired_flag; // Đã hết hạn chưa
    uint32_t nod;      // tổng số thiết bị trong nhóm LID
    String deviceName; // Tên thiết bị
    String version;
} LicenseInfo;

typedef struct
{
    char payload[250];
} PayloadStruct;

// Lấy địa chỉ MAC của thiết bị
String getDeviceMacAddress()
{
    return WiFi.macAddress();
}

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
extern const uint32_t duration;
extern bool expired_flag;
extern uint8_t expired;
extern uint32_t now;
extern uint32_t lastSendTime;
extern String device_id;
extern uint32_t nod; // Số lượng thiết bị, mặc định là 10


// Thông số mạng mesh có thể cấu hình
extern String mesh_ssid;
extern String mesh_password;
extern uint8_t mesh_channel;

#endif // CONFIG_H
