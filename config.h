#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <MD5Builder.h>
#include "led_status.h"
#include <debug.h>

// Định nghĩa chân LED
#define LED_PIN 2

// Định nghĩa các opcode
#define LIC_TIME_GET 0x01
#define LIC_SET_LICENSE 0x02
#define LIC_GET_LICENSE 0x03
#define LIC_LICENSE_DELETE 0x04
#define LIC_LICENSE_DELETE_ALL 0x05
#define LIC_INFO 0x06
#define CONFIG_DEVICE 0x07
#define LIC_INFO_RESPONSE 0x80

// Kích thước buffer cho JSON
#define BUFFER_SIZE 512

// Địa chỉ MAC broadcast và khóa bí mật
static uint8_t senderMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // MAC của LIC66S
static uint8_t receiverMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // MAC broadcast
#define private_key "khoabi_mat_123"

// Hàm mã hóa Auth MD5
String md5Hash(int id_src, int id_des, const String &mac_src, const String &mac_des, uint8_t opcode, const String &data,
    unsigned long timestamp) {

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
    return md5.toString();  // Trả về chuỗi MD5 hex
}

// Cấu trúc dữ liệu
typedef struct {
    int lid; // License ID
    int id;
    String license; // Nội dung license
    time_t created;
    time_t expired;
    int duration;
    int remain;
    bool expired_flag; // Đã hết hạn chưa
    uint32_t nod; //number of device   
    String deviceName; // Tên thiết bị
    String version;
} LicenseInfo;

typedef struct {
    char payload[512];
} PayloadStruct;

// Lấy địa chỉ MAC của thiết bị
String getDeviceMacAddress() {
     return WiFi.macAddress(); 
    }

// Biến toàn cục
extern LedStatus led;
extern LicenseInfo globalLicense;
extern PayloadStruct message;
extern int config_lid;
extern int config_id; // id_src
extern int id_des;   // id_des
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
extern bool networkConnected;
extern unsigned long runtime;
extern unsigned long lastRuntimeSave;
extern uint32_t nod; // Số lượng thiết bị, mặc định là 10

#endif // CONFIG_H
