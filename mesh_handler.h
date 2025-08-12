#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H
#include <WiFi.h>
#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <set>

#include "config.h"
// #include "protocol_handler.h"

// Đối tượng mesh
extern painlessMesh mesh;

// Cờ báo mesh đã init xong
extern bool meshReady;

// Danh sách node đã kết nối để tránh kết nối lại không cần thiết
inline std::set<uint32_t> connectedNodes;

void onMeshReceive(uint32_t from, String &msg);

// Callback nhận dữ liệu từ mesh
inline void meshReceiveCb(uint32_t from, String &msg)
{
    // Bỏ qua gói tin do chính node phát ra
    if (from == mesh.getNodeId())
        return;

    // Bỏ qua những frame không phải JSON
    if (!msg.startsWith("{"))
        return;

    // Phân tích JSON để kiểm tra id nguồn
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error)
    {
        Serial.print(" Json parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Nếu id nguồn trùng với id của thiết bị thì bỏ qua
    int id_src = doc["id_src"] | 0;
    if (id_src == config_id)
        return;

    Serial.printf("[mesh RX] from %u: %s\n", from, msg.c_str());

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
    // mesh.onNewConnection([](uint32_t nodeId)
    //                      { mac_nhan = nodeId; // Lưu MAC nhận
    //                         Serial.printf("➕ Node %u vừa tham gia mesh\n", nodeId); });

    // // Log khi có thay đổi kết nối trong mesh (node vào/ra)
    // mesh.onChangedConnections([]()
    //                           {
    // Serial.printf("🔄 Danh sách node hiện tại: ");
    // for (auto n : mesh.getNodeList()) Serial.printf("%u ", n);
    // Serial.println(); });

    // Log khi có node mới kết nối vào mesh, tránh lặp lại nếu đã kết nối
    mesh.onNewConnection([&](uint32_t nodeId)
                         {
                             if (connectedNodes.count(nodeId) == 0)
                             {
                                 connectedNodes.insert(nodeId);
                                 mac_nhan = nodeId; // Lưu MAC nhận
                                 Serial.printf("➕ Node %u vừa tham gia mesh\n", nodeId);
                             } });

    // Khi có node rời khỏi mesh, loại bỏ khỏi danh sách
    mesh.onDroppedConnection([&](uint32_t nodeId)
                             {
                                  connectedNodes.erase(nodeId);
                                  Serial.printf("➖ Node %u đã rời mesh\n", nodeId); });

    // Log khi có thay đổi kết nối trong mesh (node vào/ra)
    mesh.onChangedConnections([&]()
                              {
                                  auto nodeList = mesh.getNodeList();
                                  Serial.printf("🔄 Danh sách node hiện tại có tổng %u node: ",
                                                (unsigned int)nodeList.size());
                                  connectedNodes.clear();
                                  for (auto n : nodeList)
                                  {
                                      Serial.printf("%u ", n);
                                      connectedNodes.insert(n);
                                  }
                                  Serial.println(); });

    meshReady = true;
    Serial.println("✅ Mạng mesh đã sẵn sàng");
}

// inline void sendToNode(uint32_t nodeId, const String &message)
// {
//     mesh.sendSingle(nodeId, message);
//     Serial.printf("📤 Sent to node %u: %s\n", nodeId, message.c_str());
// }

// inline void meshLoop()
// {
//     mesh.update();
// }

inline void sendToNode(uint32_t nodeId, const String &message)
{
    mesh.sendSingle(nodeId, message);
    Serial.printf("📤 Sent to node %u: %s\n", nodeId, message.c_str());
}

inline void sendToAllNodes(const String &message)
{
    bool ok = mesh.sendBroadcast(message);
    Serial.printf("📤 Broadcast %s: %s\n", ok ? "OK" : "FAIL", message.c_str());
}

inline void meshLoop()
{
    mesh.update();
}

#endif // MESH_HANDLER_H