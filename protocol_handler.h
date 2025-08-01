#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <WiFi.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include "config.h"
#include <Preferences.h>
#include <painlessMesh.h>
#include "serial.h"

extern painlessMesh mesh; // Mesh object toàn cục
extern bool dang_gui;
extern unsigned long lastSendTime;
extern volatile bool hasNewPacket;
extern int lastPacketLen;
extern uint32_t lastPacketNodeId;
extern uint8_t lastPacketData[250];
extern uint32_t lastPacketNodeId; // Lưu nodeId của gói vừa nhận (thay cho MAC)
extern PayloadStruct message;
extern Preferences preferences;
extern unsigned long runtime;
extern uint32_t nod;

// ===== GỬI GÓI TIN QUA MESH =====
String createMessage(int id_src, int id_des, uint32_t mac_src, uint32_t mac_des, uint8_t opcode,
                     const JsonVariant &data, unsigned long timestamp = 0) {
    if (timestamp == 0)
        timestamp = millis() / 1000; // mô phỏng thời gian Unix

    String dataStr;
    serializeJson(data, dataStr);
    String auth = md5Hash(id_src, id_des, mac_src, mac_des, opcode, dataStr, timestamp);

    StaticJsonDocument <512> jsonDoc;
    jsonDoc["id_src"] = id_src;
    jsonDoc["id_des"] = id_des;
    jsonDoc["mac_src"] = String(mac_src, HEX);
    jsonDoc["mac_des"] = String(mac_des, HEX);
    jsonDoc["opcode"] = opcode;
    jsonDoc["data"] = data;
    jsonDoc["time"] = timestamp;
    jsonDoc["auth"] = auth;

    String messageStr;
    serializeJson(jsonDoc, messageStr);
    return messageStr;
}

void sendResponse(int id_src, int id_des, uint32_t mac_src, uint32_t mac_des, uint8_t opcode,
                  const JsonVariant &data, uint32_t destNodeId) {
    String packet = createMessage(id_src, id_des, mac_src, mac_des, opcode, data);
    if (packet.length() > sizeof(message.payload)) {
        Serial.println("❌ Payload quá lớn, không gửi được");
        led.setState(CONNECTION_ERROR);
        return;
    }
    packet.toCharArray(message.payload, sizeof(message.payload));
    dang_gui = true;
    bool ok = mesh.sendSingle(destNodeId, packet);
    Serial.printf("📤 [mesh] sendSingle to %u %s\n", destNodeId, ok ? "OK" : "FAIL");
    Serial.println(packet);
    lastSendTime = millis();
    if (!ok) {
        Serial.println("❌ Gửi gói tin thất bại");
        led.setState(CONNECTION_ERROR);
    } else {
        Serial.println("✅ Gửi gói tin thành công");
        led.setState(FLASH_TWICE);
    }
}

// ===== LƯU/LOAD LICENSE, DEVICE CONFIG =====
void saveLicenseData() {
    preferences.begin("license", false);
    preferences.putInt("lid", globalLicense.lid);
    preferences.putULong("created", globalLicense.created);
    preferences.putInt("duration", globalLicense.duration);
    preferences.putInt("remain", globalLicense.remain);
    preferences.putBool("expired_flag", globalLicense.expired_flag);
    preferences.putULong("runtime", runtime);
    preferences.putUInt("nod", globalLicense.nod);
    preferences.putULong("last_save", millis() / 1000);
    preferences.end();
    Serial.println("✅ Đã lưu dữ liệu license vào NVS");
    Serial.print("Expired: "); Serial.println(globalLicense.expired_flag ? 1 : 0);
    Serial.print("Remain: "); Serial.println(globalLicense.remain);
}

void saveDeviceConfig() {
    preferences.begin("license", false);
    preferences.putUInt("config_lid", config_lid);
    preferences.putUInt("config_id", config_id);
    preferences.putUInt("nod", ::nod);
    preferences.end();
}

void loadLicenseData() {
    preferences.begin("license", true);
    globalLicense.lid = preferences.getInt("lid", 0);
    config_lid = preferences.getInt("config_lid", config_lid);
    config_id = preferences.getInt("config_id", config_id);
    globalLicense.created = preferences.getULong("created", 0);
    globalLicense.duration = preferences.getInt("duration", 0);
    globalLicense.remain = preferences.getInt("remain", 0);
    globalLicense.expired_flag = preferences.getBool("expired_flag", false);
    runtime = preferences.getULong("runtime", 0);
    globalLicense.nod = preferences.getUInt("nod", 10);
    ::nod = globalLicense.nod;
    preferences.end();
    Serial.println("✅ Đã đọc và cập nhật dữ liệu license từ NVS:");
    Serial.print("LID: "); Serial.println(globalLicense.lid);
    Serial.print("Expired: "); Serial.println(globalLicense.expired_flag ? 1 : 0);
    Serial.print("Runtime: "); Serial.println(runtime);
}

// ===== CALLBACK KHI NHẬN TỪ MESH =====
void onMeshReceive(uint32_t from, String &msg) {
    Serial.printf("[mesh] Received from %u: %s\n", from, msg.c_str());

    // Lưu nodeId và data để xử lý ở loop nếu muốn (hoặc gọi luôn xử lý)
    size_t len = msg.length();
    if (len > sizeof(lastPacketData))
        len = sizeof(lastPacketData);
    memcpy(lastPacketData, msg.c_str(), len);
    lastPacketLen = len;
    lastPacketNodeId = from;
    hasNewPacket = true;

    // Hoặc gọi luôn xử lý (nếu không cần defer trong loop):
    // xu_ly_data(from, (const uint8_t*)msg.c_str(), len);
}

// ===== HÀM PHÂN TÍCH, CHUYỂN TỪ BUFFER -> THAM SỐ =====
void xu_ly_data(uint32_t from, const uint8_t *data, int len);

// ===== HÀM XỬ LÝ CHÍNH SAU KHI ĐÃ PARSE JSON =====
void xu_ly_data(uint32_t from, int id_src, int id_des, uint32_t mac_src, uint32_t mac_des,
                uint8_t opcode, const JsonVariant &data, unsigned long packetTime, const String &recvAuth) {
    Serial.println("\n📩 Xử lý data từ Mesh:");
    // Bỏ qua nếu không phải đích
    if (id_des != config_id && id_des != 0) {
        Serial.println("❌ Không dành cho node này");
        return;
    }

    // MD5 verify
    String dataStr;
    serializeJson(data, dataStr);
    String calcAuth = md5Hash(id_src, id_des, mac_src, mac_des, opcode, dataStr, packetTime);
    if (!recvAuth.equalsIgnoreCase(calcAuth)) {
        Serial.println("❌ MD5 auth failed");
        StaticJsonDocument <128> respDoc;
        respDoc["status"] = 1;
        sendResponse(config_id, id_src, mac_src, mac_des, opcode | 0x80, respDoc, from);
        led.setState(CONNECTION_ERROR);
        return;
    }
    Serial.println("✅ MD5 auth success");
    Serial.print("Opcode: 0x"); Serial.println(opcode, HEX);

    // Xử lý từng opcode như trước
    switch (opcode) {
    case LIC_SET_LICENSE: {
        JsonObject licData = data.as<JsonObject>();
        int lid = licData["lid"].as<int>();
        int id = licData["id"].as<int>();
        time_t created = licData["created"].as<long>();
        int duration = licData["duration"].as<int>();
        int expired = licData["expired"].as<int>();
        int nod = licData["nod"].as<int>();

        StaticJsonDocument <256> respDoc;
        bool isValid = false;
        String error_msg = "";

        if (lid != 0 && lid == config_lid) {
            if (id == 0 || id == config_id) isValid = true;
            else error_msg = "ID không dành cho thiết bị này";
        } else {
            error_msg = "LID không hợp lệ";
        }

        if (isValid) {
            if (expired == 0) {
                globalLicense.created = created;
                globalLicense.duration = duration;
                globalLicense.nod = nod;
                start_time = millis() / 1000;
                runtime = 0;
                globalLicense.remain = duration;
                globalLicense.expired_flag = false;
                saveLicenseData();

                respDoc["lid"] = lid;
                respDoc["nod"] = globalLicense.nod;
                respDoc["status"] = 0;
                sendResponse(config_id, id_src, mac_des, mac_src, LIC_SET_LICENSE | 0x80, respDoc, from);
                Serial.println("✅ Cập nhật giấy phép thành công: LID = " + String(lid) + ", ID = " + String(id));
                led.setState(FLASH_TWICE);
                while (led.isBusy()) delay(10);
                saveLicenseData();
                delay(100);
                // ESP.restart();
            } else {
                respDoc["status"] = 3;
                respDoc["nod"] = globalLicense.nod;
                sendResponse(config_id, id_src, mac_des, mac_src, LIC_SET_LICENSE | 0x80, respDoc, from);
                Serial.println("❌ Giấy phép hết hiệu lực");
                led.setState(CONNECTION_ERROR);
            }
        } else {
            respDoc["status"] = 1;
            respDoc["error_msg"] = error_msg;
            Serial.println("❌ Lỗi: " + error_msg + " (LID = " + String(lid) + ", ID = " + String(id) + ", config_id = " + String(config_id) + ")");
            sendResponse(config_id, id_src, mac_src, mac_des, LIC_SET_LICENSE | 0x80, respDoc, from);
            led.setState(CONNECTION_ERROR);
        }
        break;
    }
    case LIC_GET_LICENSE: {
        JsonObject licData = data.as<JsonObject>();
        uint32_t lid = licData["lid"].as<int>();
        StaticJsonDocument <256> respDoc;
        if (lid == config_lid || lid == 0) {
            respDoc["lid"] = globalLicense.lid;
            respDoc["created"] = globalLicense.created;
            respDoc["expired"] = globalLicense.expired_flag ? 1 : 0;
            respDoc["duration"] = globalLicense.duration;
            respDoc["remain"] = globalLicense.remain;
            respDoc["nod"] = globalLicense.nod;
            respDoc["status"] = 0;
            Serial.println("✅ License info sent for LID = " + String(lid));
            sendResponse(config_id, id_src, mac_src, mac_des, LIC_GET_LICENSE | 0x80, respDoc, from);
            led.setState(FLASH_TWICE);
            while (led.isBusy()) { led.update(); delay(10); }
            if (globalLicense.expired_flag || globalLicense.remain <= 0) {
                Serial.println(F("🔒 License đã HẾT HẠN!"));
                led.setState(LICENSE_EXPIRED);
            } else {
                Serial.printf("🔓 License còn %lu phút\n", globalLicense.remain);
                led.setState(NORMAL_STATUS);
            }
        } else {
            Serial.println("❌ LID không hợp lệ: " + String(lid));
            led.setState(CONNECTION_ERROR);
        }
        break;
    }
    case CONFIG_DEVICE: {
        JsonObject licData = data.as<JsonObject>();
        int lid = licData["new_lid"].as<int>();
        uint32_t nod = licData["nod"].as<uint32_t>();
        int id = licData["new_id"].as<int>();
        StaticJsonDocument <256> respDoc;
        bool isValid = false;
        String error_msg;
        if (lid == 0) { isValid = false; error_msg = "LID không hợp lệ"; }
        else if (id == 0) { isValid = false; error_msg = "ID không hợp lệ"; }
        else if (id != config_id) { isValid = false; error_msg = "ID không khớp với thiết bị này"; }
        else isValid = true;
        if (isValid) {
            globalLicense.lid = lid;
            globalLicense.id = id;
            globalLicense.nod = nod;
            preferences.begin("license", false);
            preferences.putUInt("lid", globalLicense.lid);
            preferences.putUInt("id", globalLicense.id);
            preferences.putUInt("nod", globalLicense.nod);
            preferences.end();
            led.setState(FLASH_TWICE);
            Serial.println("✅ Cấu hình thành công: LID = " + String(lid) + ", ID = " + String(id) + ", NOD = " + String(nod));
            respDoc["status"] = 0;
            respDoc["lid"] = globalLicense.lid;
            respDoc["id"] = globalLicense.id;
            respDoc["nod"] = globalLicense.nod;
        } else {
            respDoc["status"] = 1;
            respDoc["error_msg"] = error_msg;
            Serial.println("❌ Lỗi: " + error_msg + " (LID = " + String(lid) + ", ID = " + String(id) + ", config_id = " + String(config_id) + ")");
        }
        // sendResponse(config_id, id_src, mac_src, mac_des, CONFIG_DEVICE | 0x80, respDoc, from);
        break;
    }
    case LIC_LICENSE_DELETE: {
        int lid = data["lid"].as<int>();
        StaticJsonDocument <256> respDoc;
        respDoc["lid"] = lid;
        respDoc["status"] = (lid == globalLicense.lid) ? 0 : 3;
        if (lid == globalLicense.lid) {
            globalLicense.lid = 0;
            globalLicense.created = 0;
            globalLicense.duration = 0;
            globalLicense.remain = 0;
            globalLicense.expired_flag = false;
            saveLicenseData();
        }
        // sendResponse(config_id, id_src, mac_src, mac_des, LIC_LICENSE_DELETE | 0x80, respDoc, from);
        Serial.println(lid == globalLicense.lid ? "✅ Đã xóa license cho LID = " + String(lid) : "❌ Không tìm thấy license cho LID = " + String(lid));
        led.setState(lid == globalLicense.lid ? NORMAL_STATUS : CONNECTION_ERROR);
        break;
    }
    case LIC_LICENSE_DELETE_ALL: {
        globalLicense.lid = 0;
        globalLicense.created = 0;
        globalLicense.duration = 0;
        globalLicense.remain = 0;
        globalLicense.expired_flag = false;
        saveLicenseData();
        StaticJsonDocument <256> respDoc;
        respDoc["status"] = 0;
        // sendResponse(config_id, id_src, mac_src, mac_des, LIC_LICENSE_DELETE_ALL | 0x80, respDoc, from);
        Serial.println("✅ Đã xóa tất cả license.");
        led.setState(NORMAL_STATUS);
        break;
    }
    case LIC_TIME_GET: {
        StaticJsonDocument <256> respDoc;
        respDoc["time"] = millis() / 1000;
        respDoc["status"] = 0;
        // sendResponse(config_id, id_src, mac_src, mac_des, LIC_TIME_GET | 0x80, respDoc, from);
        Serial.println("✅ Time info sent.");
        break;
    }
    case LIC_INFO: {
        StaticJsonDocument <256> respDoc;
        respDoc["deviceName"] = globalLicense.deviceName;
        respDoc["version"] = globalLicense.version;
        respDoc["status"] = 0;
        // sendResponse(config_id, id_src, mac_src, mac_des, LIC_INFO_RESPONSE, respDoc, from);
        Serial.println("✅ Device info sent.");
        break;
    }
    default: {
        StaticJsonDocument <256> respDoc;
        respDoc["status"] = 255;
        // sendResponse(config_id, id_src, mac_src, mac_des, opcode | 0x80, respDoc, from);
        Serial.printf("❌ Unknown opcode: 0x%02X\n", opcode);
        led.setState(CONNECTION_ERROR);
        break;
    }
    }
}

// ===== HÀM PARSE JSON BUFFER VÀ GỌI XỬ LÝ CHÍNH =====
void xu_ly_data(uint32_t from, const uint8_t *data, int len) {
    String msg((const char*)data, len);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.print("❌ Lỗi parse JSON trong xu_ly_data: ");
        Serial.println(error.c_str());
        return;
    }
    int id_src = doc["id_src"] | 0;
    int id_des = doc["id_des"] | 0;
    String mac_src_str = doc["mac_src"] | "";
    String mac_des_str = doc["mac_des"] | "";
    uint8_t opcode = doc["opcode"] | 0;
    JsonVariant data_field = doc["data"];
    unsigned long packetTime = doc["time"] | 0;
    String auth = doc["auth"] | "";

    uint32_t mac_src = strtoul(mac_src_str.c_str(), nullptr, 16);
    uint32_t mac_des = strtoul(mac_des_str.c_str(), nullptr, 16);

    xu_ly_data(from, id_src, id_des, mac_src, mac_des, opcode, data_field, packetTime, auth);
}

#endif // PROTOCOL_HANDLER_H
