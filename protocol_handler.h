#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>
// #include <deque>

#include "config.h"
#include "mesh_handler.h"
#include "led_status.h"
#include "serial.h"
#include "function.h"
#include "packet_storage.h"
extern painlessMesh mesh; // Khởi tạo đối tượng mesh toàn cục
extern uint32_t lastTargetNode;

// extern std::deque<LicenseRecord> packetPersistQueue;

// Tạo tin nhắn phản hồi
String createMessage(int id_src, int id_des, uint32_t mac_src, uint32_t mac_des,
                     uint8_t opcode, const JsonVariant &data, unsigned long timestamp = 0)
{
    if (timestamp == 0)
    {
        timestamp = millis() / 1000; // Mô phỏng thời gian Unix
    }
    String dataStr;
    serializeJson(data, dataStr); // Chuyển dữ liệu thành chuỗi JSON

    // Tạo mã MD5 xác thực
    String auth = md5Hash(id_src, id_des, mac_src, mac_des, opcode, dataStr, timestamp); // Tạo mã MD5
    DynamicJsonDocument jsonDoc(512);
    jsonDoc["id_src"] = id_src; // ID nguồn
    jsonDoc["id_des"] = id_des; // ID đích
    jsonDoc["mac_src"] = String(mac_src, HEX);
    jsonDoc["mac_des"] = String(mac_des, HEX); // Địa chỉ MAC nguồn và đích
    jsonDoc["opcode"] = opcode;                // Opcode
    jsonDoc["data"] = data;                    // Dữ liệu
    jsonDoc["time"] = timestamp;               // Thời gian
    jsonDoc["auth"] = auth;                    // Mã xác thực

    String output;
    serializeJson(jsonDoc, output); // Chuyển thành chuỗi JSON
    return output;
}

// Gửi payload JSON qua Mesh broadcast
// void meshBroadcastJson(const String &payload)
// {
//     String output;
//     bool ok = mesh.sendBroadcast(payload);
//     Serial.printf("[mesh] broadcast %s: %s\n", ok ? "OK" : "FAIL", payload.c_str());
// }

void meshBroadcastJson(const String &payload)
{
    sendToAllNodes(payload);
}

// Xử lý phản hồi từ Mesh
void processReceivedData(StaticJsonDocument<512> message, uint8_t opcode, const JsonVariant &dataObj, uint32_t nodeId)
{
    int config_id = message["id_src"];
    int id_des = message["id_des"];
    uint32_t mac_src = strtoul(message["mac_src"], nullptr, 16);
    uint32_t mac_des = strtoul(message["mac_des"], nullptr, 16);
    String dataStr;
    serializeJson(message["data"], dataStr);
    unsigned long timestamp = message["time"].as<unsigned long>();
    String receivedAuth = message["auth"].as<String>();

    String calculatedAuth = md5Hash(config_id, id_des, mac_src, mac_des, opcode, dataStr, timestamp);

    if (!receivedAuth.equalsIgnoreCase(calculatedAuth))
    {
        Serial.println("❌ Lỗi xác thực: Mã MD5 không khớp!");
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

        sprintf(messger, "Status: %d \nLocal ID: %d\n", Status, lid); // đổi %s sang %d
        if (error_msg != NULL)
        {
            strncat(messger, "Lỗi: ", sizeof(messger) - strlen(messger) - 1);
            strncat(messger, error_msg, sizeof(messger) - strlen(messger) - 1);
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
        uint32_t time_temp = data["remain"];
        
        addNodeToList(config_id, lid, nodeId, time_temp); // millis()
        
        // addNodeToList(id_src, lid, nodeId, time_temp);
        enable_print_ui = true;
        // printDeviceList();
        // Serial.println(data["license"].as<String>());
        break;
    }

    default:
        if (opcode != 0x83)
        { // Bỏ qua opcode 0x83
            Serial.printf("Unknown opcode: 0x%02X\n", opcode);
        }
        break;
    }
}

void onMeshReceive(uint32_t from, String &msg)
{
    Serial.println("\n📩 Received response:");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Trích thông tin cần thiết từ gói tin
    uint8_t opcode = doc["opcode"];
    JsonObject payload = doc["data"];
    int id_src = doc["id_src"];
    int id_des = doc["id_des"];
    String mac_src = doc["mac_src"];
    String mac_des = doc["mac_des"];
    unsigned long timestamp = doc["time"];
    String auth = doc["auth"];

    int status = payload.containsKey("status") ? payload["status"] : 0;

    Serial.printf("\n[mesh RX] From nodeId = 0x%08X (Node ID = %d)  timestamp = %lu\n", from, id_src, timestamp);
    Serial.printf("Opcode: 0x%02X   Status: %d\n", opcode, status);
    // serializeJsonPretty(doc, Serial);
    // Serial.println();

    // Gọi xử lý phản hồi (sẽ tự ghi flash nếu cần)
    processReceivedData(doc, opcode, payload, from);
}

// // --- Gửi HUB_SET_LICENSE qua Mesh ---
void set_license(int id_src, int id_des, int lid,  uint32_t mac_des, time_t created, uint32_t duration, uint8_t expired, uint32_t now)
{
    uint32_t mac_src = mesh.getNodeId(); // MAC nguồn
    int opcode = LIC_SET_LICENSE;
    // int id_des = config_id; // ID của LIC66S

    DynamicJsonDocument dataDoc(256);
    dataDoc["lid"] = lid;
    dataDoc["created"] = created;
    dataDoc["duration"] = duration;
    dataDoc["expired"] = expired;

    String output = createMessage(id_src, id_des, mac_src, mac_des, opcode, dataDoc, now);
    if (output.length() > sizeof(message.payload))
    {
        Serial.println("❌ Payload quá lớn!");
        return;
    }
    // Chờ đến khi node đích có trong mạng mesh trước khi gửi
    const unsigned long timeout = 5000; // thời gian chờ tối đa 5 giây
    unsigned long start = millis();
    while (!mesh.isConnected(mac_des) && (millis() - start < timeout))
    {
        mesh.update(); // duy trì mesh và tìm node
        delay(50);
    }

    if (!mesh.isConnected(mac_des))
    {
        Serial.printf("❌ Node 0x%08X chưa kết nối, hủy gửi HUB_SET_LICENSE\n", mac_des);
        Serial.println("Không set license cho node");
        return;
    }
    sendToNode(mac_des, output);

    // Serial.println("\n📤 Gửi HUB_SET_LICENSE:");
    // Serial.println(output);
}

void getlicense(int id_des, int lid, uint32_t mac_des, unsigned long now)
{
    int opcode = LIC_GET_LICENSE;
    uint32_t mac_src = mesh.getNodeId();
    int id_src = config_id; // ID của LIC66S

    DynamicJsonDocument dataDoc(128);
    dataDoc["lid"] = lid;

    // Nếu broadcast theo MAC, cũng broadcast theo ID
    if (mac_des == 0)
    {
        id_des = 0;
    }

    String output = createMessage(id_src, id_des, mac_src, mac_des, opcode, dataDoc, now);
    if (output.length() > sizeof(message.payload))
    {
        Serial.println("❌ Payload quá lớn!");
        return;
    }

    // Gửi gói tin: broadcast nếu mac_des == 0, ngược lại gửi tới node cụ thể
    if (mac_des == 0)
    {
        sendToAllNodes(output);
    }
    else
    {
        sendToNode(mac_des, output);
    }

    Serial.println("📤 Gửi HUB_GET_LICENSE:");
    // Serial.println(output);
}

// --- Gửi HUB_GET_LICENSE qua Mesh ---
// void getlicense(int id_des, int lid, uint32_t mac_des, unsigned long now)
// {
//     int opcode = LIC_GET_LICENSE;
//     uint32_t mac_src = mesh.getNodeId();
//     int id_src = config_id; // ID của LIC66S

//     DynamicJsonDocument dataDoc(128);
//     dataDoc["lid"] = lid;
//     String output = createMessage(id_src, id_des, mac_src, mac_des, opcode, dataDoc, now);
//     if (output.length() > sizeof(message.payload))
//     {
//         Serial.println("❌ Payload quá lớn!");
//         return;
//     }

//     // Gửi gói tin đến mac_des (có thể là broadcast nếu mac_des == 0)
//     // meshReceiveCb(mesh.getNodeId(), output);
//     mesh.sendSingle(mac_des, output);
//     Serial.println("nhay vao thu vien protocol_handler.h cho getlicense");
//     Serial.println("📤 Gửi HUB_GET_LICENSE:");
//     Serial.println(output);
// }

#endif // PROTOCOL_HANDLER_H
