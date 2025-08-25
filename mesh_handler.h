#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include <painlessMesh.h>
#include <ArduinoJson.h>
#include "config.h"
#include "protocol_handler.h" // prototype của sendResponse(...)

// Thông số mạng mesh
#define MESH_SSID "Hub66sMesh"
#define MESH_PASSWORD "mesh_pass_456"
#define MESH_PORT 5555
#define MESH_CHANNEL 2 // Kênh WiFi cho mesh trùng với sender


// Đối tượng mesh
extern painlessMesh mesh;

extern bool dang_gui; // cờ đang gửi

// Helper: chuyển MAC string "AA:BB:CC:DD:EE:FF" → mảng byte[6]
inline void parseMacString(const String &macStr, uint8_t *mac)
{
    int macBytes[6];
    if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
               &macBytes[0], &macBytes[1], &macBytes[2],
               &macBytes[3], &macBytes[4], &macBytes[5]) == 6)
    {
        for (int i = 0; i < 6; ++i)
        {
            mac[i] = static_cast<uint8_t>(macBytes[i]);
        }
    }
}

// Callback khi nhận message từ mesh
inline void meshReceivedCallback(uint32_t from, String &msg)
{
    // 1) In thô để debug
    Serial.printf("[mesh] RAW from %u: |%s|\n", from, msg.c_str());

    // 2) Loại bỏ khoảng trắng, newline đầu/cuối
    msg.trim();
    if (msg.isEmpty()) {
        Serial.println("[mesh] Empty after trim, skip");
        return;
    }

    // 3) Nếu không phải JSON (không bắt đầu bằng '{'), chỉ in text
    if (!msg.startsWith("{")) {
        Serial.printf("[mesh] Text message: %s\n", msg.c_str());
        return;
    }

    // 4) Bây giờ msg chắc chắn JSON → parse
    StaticJsonDocument<512> doc;
    auto err = deserializeJson(doc, msg);
    if (err) {
        Serial.print("[mesh] JSON parse error: ");
        Serial.println(err.c_str());
        return;
    }

    // 5) Lấy các trường
    int     id_src = doc["id_src"].as<int>();
    int     id_des = doc["id_des"].as<int>();
    uint8_t opcode = doc["opcode"].as<uint8_t>();
    String  mac_src = doc["mac_src"].as<String>();
    String  mac_des = doc["mac_des"].as<String>();

    // 6) Đưa phần data vào DynamicJsonDocument
    DynamicJsonDocument dataDoc(256);
    dataDoc.set(doc["data"]);

    // 7) Tính node đích (dùng 'from' ở đây)
    uint32_t destNode = from;

    // 8) Gửi response nếu cần
    sendResponse(id_src, id_des, opcode, dataDoc, destNode);

    // 9) Gọi hàm xử lý JSON chuyên sâu
    onMeshReceive(from, msg);

    // 10) Reset flag gửi
    dang_gui = false;
}

// Khởi tạo mesh network
inline void initMesh()
{
    Serial.println("Khởi tạo Mesh...");
    // ESP-NOW ở receiver.ino đã gọi WiFi.mode(WIFI_STA)
    WiFi.mode(WIFI_AP_STA);
    delay(100); // Đợi WiFi mode ổn định

    WiFi.setTxPower(WIFI_POWER_2dBm);

    // Chỉ log ERROR, STARTUP, CONNECTION ở ví dụ này
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    // init(meshID, password, port, WiFiMode, channel)
    mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);

    // Đăng ký callback
    mesh.onReceive(&meshReceivedCallback);
    Serial.println("✅ mesh_handler: painlessMesh initialized");
}

// -----------------------------------------------------------------------------
// Hàm gọi trong loop() để cập nhật mesh
// -----------------------------------------------------------------------------
inline void meshLoop()
{
    mesh.update();
}

// Gửi broadcast tới toàn mạng

inline void broadcastMeshMessage(const String &json)
{
    Serial.println("📤 Gửi broadcast tới mesh: " + json);

    // Nếu đang gửi thì không gửi nữa
    if (dang_gui)
    {
        Serial.println("❌ Đang gửi, không thể gửi broadcast");
        return;
    }

    // Reset cờ gửi
    dang_gui = false;

    // Tạo message struct
    message.payload[0] = '\0'; // Đảm bảo payload rỗng trước khi copy
    json.toCharArray(message.payload, sizeof(message.payload));
    Serial.printf("📤 [mesh] broadcast: %s\n", message.payload);

    // Gửi broadcast
    bool ok = mesh.sendBroadcast(message.payload);
    if (!ok)
    {
        Serial.println("❌ Gửi broadcast thất bại");
        led.setState(CONNECTION_ERROR);
    }
    else
    {
        Serial.println("✅ Gửi broadcast thành công");
        led.setState(FLASH_TWICE); // Chớp LED 2 lần để xác nhận gửi thành công
    }
}
#endif // MESH_HANDLER_H
