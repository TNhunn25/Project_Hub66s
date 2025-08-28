#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include <painlessMesh.h>
#include <ArduinoJson.h>
#include "config.h"
#include "protocol_handler.h" // prototype của sendResponse(...)
#include "led_status.h"

// // Thông số mạng mesh
// #define MESH_SSID "Hub66sMesh"
// #define MESH_PASSWORD "mesh_pass_456"
// #define MESH_PORT 5555
// #define MESH_CHANNEL 6

// Các tham số mạng mesh có thể cấu hình động
extern String mesh_ssid;
extern String mesh_password;
extern uint8_t mesh_channel; // Kênh WiFi cho mesh trùng với sender

extern painlessMesh mesh;
extern Scheduler userScheduler;

/**
 * Callback khi nhận message từ mesh
 * Chỉ xử lý message JSON hợp lệ và xác thực đúng.
 */
inline void meshReceivedCallback(uint32_t from, String &msg)
{
    msg.trim();
    if (msg.isEmpty())
        return;

    // Chỉ xử lý message JSON
    if (!msg.startsWith("{"))
    {
        Serial.printf("[mesh] Non-JSON message: %s\n", msg.c_str());
        return;
    }

    // Parse JSON
    StaticJsonDocument<384> doc; // từ 512 hạ xuống 384
    auto err = deserializeJson(doc, msg);
    if (err)
    {
        Serial.printf("[mesh] JSON parse error: %s\n", err.c_str());
        return;
    }

    // 5) Lấy các trường
    int id_src = doc["id_src"].as<int>();
    int id_des = doc["id_des"].as<int>();
    uint8_t opcode = doc["opcode"].as<uint8_t>();
    String mac_src_str = doc["mac_src"].as<String>();
    String mac_des_str = doc["mac_des"].as<String>();

    // ⚠️ Chuyển MAC từ chuỗi HEX → uint32_t
    uint32_t mac_src = strtoul(mac_src_str.c_str(), nullptr, 16);
    uint32_t mac_des = strtoul(mac_des_str.c_str(), nullptr, 16);

    // 6) Đưa phần data vào DynamicJsonDocument
    DynamicJsonDocument dataDoc(256);
    dataDoc.set(doc["data"]);

    // // 7) Tính node đích (dùng 'from' ở đây)
    // uint32_t destNode = from;

    // 8) So sánh chuỗi xác thực auth
    String receivedAuth = doc["auth"].as<String>();
    String dataStr;
    serializeJson(dataDoc, dataStr);

    unsigned long timestamp = doc["time"] | 0; // thời gian gửi
    String calculatedAuth = md5Hash(id_src, id_des, mac_src, mac_des, opcode, dataStr, timestamp);

    // Kiểm tra xác thực
    if (!receivedAuth.equalsIgnoreCase(calculatedAuth))
    {
        Serial.println("❌ AUTH MISMATCH! Packet rejected.");
        return;
    }

    // Nếu gói tin không dành cho node này thì chuyển tiếp
    if (id_des != config_id && id_des != 0)
    {
        bool ok = false;
        if (mac_des != 0 && mesh.isConnected(mac_des))
        {
            ok = mesh.sendSingle(mac_des, msg);
            Serial.printf("➡️ Forward to %u %s\n", mac_des, ok ? "OK" : "FAIL");
        }
        else
        {
            ok = mesh.sendBroadcast(msg);
            Serial.printf("➡️ Forward broadcast %s\n", ok ? "OK" : "FAIL");
        }
        return;
    }

    Serial.println("✅ AUTH OK – processing...");

    // Xử lý logic sâu hơn: gọi hàm xử lý gói tin mesh chuyên biệt nếu có
    if (onMeshReceive)
        onMeshReceive(from, msg);

    // Nếu muốn gửi phản hồi thì gửi về node gửi
    // Có thể tuỳ chỉnh điều kiện này (hoặc bỏ nếu không cần)
    // Nếu cần gửi phản hồi sử dụng sendResponse như bên dưới:
    // sendResponse(id_src, id_des, mac_src, mac_des, opcode, doc["data"], from);
}

/**
 * Khởi tạo mạng mesh
 */
inline void initMesh()
{
    Serial.println("Khởi tạo Mesh...");
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.setTxPower(WIFI_POWER_15dBm);
    // Tắt chế độ tiết kiệm điện để root không sleep
    WiFi.setSleep(false);
    mesh.setDebugMsgTypes(ERROR | STARTUP);
    mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_STA, MESH_CHANNEL);
    // mesh.setRoot(true);          // ✅ đánh dấu node này là ROOT
    // Cố định root để hạn chế việc node liên tục scan và nhảy mạng
    mesh.setContainsRoot(true);
    // WiFi.softAP(MESH_SSID, MESH_PASSWORD, MESH_CHANNEL, true);

    mesh.onReceive(&meshReceivedCallback);
    Serial.println("✅ mesh_handler: painlessMesh initialized");
}

/**
 * Hàm gọi trong loop() để cập nhật mesh
 */
inline void meshLoop()
{
    mesh.update();
}

/**
 * Gửi broadcast tới toàn mạng
 */
inline void broadcastMeshMessage(const String &json)
{
    Serial.println("📤 Gửi broadcast tới mesh: " + json);

    if (!mesh.sendBroadcast(json))
    {
        Serial.println("❌ Gửi broadcast thất bại");
        led.setState(CONNECTION_ERROR);
    }
    else
    {
        Serial.println("✅ Gửi broadcast thành công");
        led.setState(FLASH_TWICE);
    }
}

/**
 * Trả về tổng số node đang kết nối trong mesh (bao gồm thiết bị hiện tại)
 */
inline uint32_t getConnectedDeviceCount()
{
    auto nodes = mesh.getNodeList();
    return nodes.size() + 1; // +1 tính cả thiết bị hiện tại
}

#endif // MESH_HANDLER_H