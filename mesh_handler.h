#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H
#include <WiFi.h>
#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

#include "config.h"
#include "protocol_handler.h"

// Đối tượng mesh
extern painlessMesh mesh;

// Cờ báo mesh đã init xong
extern bool meshReady;

void onMeshReceive(uint32_t from, String &msg);

// Callback nhận dữ liệu từ mesh
inline void meshReceiveCb(uint32_t from, String &msg)
{

    Serial.printf("[mesh RX] from %u: %s\n", from, msg.c_str());

    // Bỏ qua những frame không phải JSON
    if (!msg.startsWith("{"))
        return;

    // Xử lý dữ liệu nhận được
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error)
    {
        Serial.print(" Json parse failed: ");
        Serial.println(error.c_str());
        return;
    }
    // Gọi handler JSON chuyên biệt
    onMeshReceive(from, msg);
}

// Khởi tạo mesh network
inline void initMesh()
{
    Serial.println("Khởi tạo Mesh...");
    WiFi.mode(WIFI_AP_STA);
    delay(100); // Đợi WiFi mode ổn định

    // Chỉ log ERROR, STARTUP, CONNECTION ở ví dụ này
    mesh.setDebugMsgTypes(ERROR | STARTUP); // Nếu cần hiện kết nối thì thêm CONNECTION

    // init(meshID, password, port, WiFiMode, channel)
    mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);

    // Đăng ký callback
    mesh.onReceive(&meshReceiveCb);
    Serial.println("✅ [mesh_handler] Khởi tạo painlessMesh thành công");

    // Log khi có node mới kết nối vào mesh
    mesh.onNewConnection([](uint32_t nodeId)
                         { mac_nhan = nodeId; // Lưu MAC nhận
                            Serial.printf("➕ Node %u vừa tham gia mesh\n", nodeId); });

    // Log khi có thay đổi kết nối trong mesh (node vào/ra)
    mesh.onChangedConnections([]()
                              {
    Serial.printf("🔄 Danh sách node hiện tại: ");
    for (auto n : mesh.getNodeList()) Serial.printf("%u ", n);
    Serial.println(); });

    meshReady = true;
    Serial.println("✅ Mạng mesh đã sẵn sàng");
}

inline void sendToNode(uint32_t nodeId, const String &message)
{
    mesh.sendSingle(nodeId, message);
    Serial.printf("📤 Sent to node %u: %s\n", nodeId, message.c_str());
}

inline void meshLoop()
{
    mesh.update();
}
#endif // MESH_HANDLER_H