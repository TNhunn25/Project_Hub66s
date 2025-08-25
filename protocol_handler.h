#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <WiFi.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include "config.h"
#include <Preferences.h>
#include <painlessMesh.h>

extern painlessMesh mesh; // Khởi tạo đối tượng mesh toàn cục

extern bool dang_gui;            // cờ đang chờ response
extern unsigned long lastSendTime;


// Cấu trúc cho tin nhắn phản hồi
extern PayloadStruct message;

// NVS để lưu trữ license
extern Preferences preferences;

extern unsigned long runtime;
extern uint32_t nod; // Số lượng thiết bị, mặc định là 10

// Chuyển đổi MAC thành String
String macToString(const uint8_t *mac)
{
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

// Tạo tin nhắn phản hồi
String createMessage(int id_src, int id_des, uint8_t opcode, const JsonVariant &data,
                     unsigned long timestamp = 0)
{
    if (timestamp == 0)
    {
        timestamp = millis() / 1000; // Mô phỏng thời gian Unix
    }
    String dataStr;
    serializeJson(data, dataStr);   // Chuyển dữ liệu thành chuỗi JSON

    // Tạo mã MD5 xác thực
    String auth = md5Hash(id_src, id_des, opcode, dataStr, timestamp); // Tạo mã MD5
    DynamicJsonDocument jsonDoc(512);
    jsonDoc["id_src"] = id_src;   // ID nguồn
    jsonDoc["id_des"] = id_des;   // ID đích
    jsonDoc["opcode"] = opcode;   // Opcode
    jsonDoc["data"] = data;       // Dữ liệu
    jsonDoc["time"] = timestamp;  // Thời gian
    jsonDoc["auth"] = auth;       // Mã xác thực

    String out;
    serializeJson(jsonDoc, out); // Chuyển thành chuỗi JSON
    return out;
}

// Gửi response qua mesh
void sendResponse(int id_src, int id_des, uint8_t opcode, const JsonVariant &data, uint32_t destNodeId)
{
// 1) build JSON
  String packet = createMessage(id_src, id_des, opcode, data);
  if (packet.length() > sizeof(message.payload)) {
    Serial.println("❌ Payload quá lớn, không gửi được");
    led.setState(CONNECTION_ERROR);
    return;
  }
  packet.toCharArray(message.payload, sizeof(message.payload));

  // 2) gửi qua mesh unicast
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
        led.setState(FLASH_TWICE); // Chớp LED 2 lần để xác nhận gửi thành công
    }
}

void xu_ly_data(uint32_t from,
                int id_src,
                int id_des,
                uint8_t opcode,
                const JsonVariant &data,
                unsigned long packetTime,
                const String &recvAuth);

//Callback khi nhận gói tin từ mesh
void onMeshReceive(uint32_t from, String &msg)
{
    Serial.printf("[mesh] Received from %u: %s\n", from, msg.c_str());

    // parse JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error)
    {
        Serial.println("❌ Failed to parse JSON");
        return;
    }

    // Lấy thông tin từ JSON
    int id_src = doc["id_src"];
    int id_des = doc["id_des"];
    uint8_t opcode = doc["opcode"];
    JsonVariant data = doc["data"];
    unsigned long timestamp = doc["time"];
    String auth = doc["auth"];

    // Kiểm tra mã xác thực
    String expectedAuth = md5Hash(id_src, id_des, opcode, data.as<String>(), timestamp);
    if (auth != expectedAuth)
    {
        Serial.println("❌ Invalid auth");
        return;
    }

    dang_gui = false; // Reset cờ đang gửi
    xu_ly_data(from, id_src, id_des, opcode, data, timestamp, auth);

    // Xử lý gói tin hợp lệ
    Serial.println("✅ Valid packet received");
}

// Lưu dữ liệu license vào NVS (Non-Volatile Storage)
// NVS: đảm bảo dữ liệu không bị mất khi thiết bị tắt nguồn.
void saveLicenseData()
{
    preferences.begin("license", false); // Mở namespace "license" ở chế độ read/write
    preferences.putInt("lid", globalLicense.lid);
    preferences.putULong("created", globalLicense.created);
    preferences.putInt("duration", globalLicense.duration);
    preferences.putInt("remain", globalLicense.remain);
    preferences.putBool("expired_flag", globalLicense.expired_flag);
    preferences.putULong("runtime", runtime);
    preferences.putUInt("nod", globalLicense.nod);      // Bổ sung: Lưu NOD
    preferences.putULong("last_save", millis() / 1000); // Lưu thời điểm lưu cuối cùng
    preferences.end();
    Serial.println("✅ Đã lưu dữ liệu license vào NVS");

    Serial.print("Expired: ");
    Serial.println(globalLicense.expired_flag ? 1 : 0);
    Serial.print("Remain: ");
    Serial.println(globalLicense.remain);
}

// Lưu cấu hình thiết bị
void saveDeviceConfig()
{
    preferences.begin("license", false);
    preferences.putUInt("config_lid", config_lid);
    preferences.putUInt("config_id", config_id);
    preferences.putUInt("nod", ::nod);
    preferences.end();
}

// Tải dữ liệu license từ NVS (gọi trong setup())
void loadLicenseData()
{
    preferences.begin("license", true); // Mở namespace "license" ở chế độ read-only
    globalLicense.lid = preferences.getInt("lid", 0);
    config_lid = preferences.getInt("config_lid", config_lid);
    config_id = preferences.getInt("config_id", config_id);
    globalLicense.created = preferences.getULong("created", 0);
    globalLicense.duration = preferences.getInt("duration", 0);
    globalLicense.remain = preferences.getInt("remain", 0);
    globalLicense.expired_flag = preferences.getBool("expired_flag", false);
    unsigned long last_save = preferences.getULong("last_save", 0);
    runtime = preferences.getULong("runtime", 0);
    globalLicense.nod = preferences.getUInt("nod", 10); // Bổ sung: Đọc NOD, mặc định 10
    ::nod = globalLicense.nod;
    preferences.end();

    Serial.println("✅ Đã đọc và cập nhật dữ liệu license từ NVS:");
    Serial.print("LID: ");
    Serial.println(globalLicense.lid);
    Serial.print("Expired: ");
    Serial.println(globalLicense.expired_flag ? 1 : 0);
    // Serial.print("Remain: "); Serial.println(globalLicense.remain);
    Serial.print("Runtime: ");
    Serial.println(runtime);
}

void xu_ly_data(uint32_t from, int id_src, int id_des, uint8_t opcode, const JsonVariant &data,
                unsigned long packetTime, const String &recvAuth)
{
    StaticJsonDocument<512> doc;
    Serial.println("\n📩 Xử lý data từ Mesh:");

    // Bỏ qua nếu không phải đích
    if (id_des != config_id && id_des != 0) {
      Serial.println("❌ Không dành cho node này");
      return;
    }

    // MD5 verify
    String dataStr;
    serializeJson(data, dataStr);
    String calcAuth = md5Hash(id_src, id_des, opcode, dataStr, packetTime);
    if (!recvAuth.equalsIgnoreCase(calcAuth)) {
      Serial.println("❌ MD5 auth failed");
      DynamicJsonDocument respDoc(128);
      respDoc["status"] = 1;
      sendResponse(config_id, id_src, opcode|0x80, respDoc, from);
      led.setState(CONNECTION_ERROR);
      return;
    }
    Serial.println("✅ MD5 auth success");
    // test
    Serial.println("Current config_id: " + String(config_id));
    Serial.println("Current globalLicense.lid: " + String(globalLicense.lid));
    Serial.println("Full received packet:");

    Serial.print("Opcode: 0x");
    Serial.println(opcode, HEX);
    serializeJsonPretty(doc, Serial);
    Serial.println();

    // Xử lý theo opcode
    switch (opcode)
    {
    case LIC_SET_LICENSE:
    {
        JsonObject data = doc["data"].as<JsonObject>();
        int lid = data["lid"].as<int>(); // Changed from String to int
        int id = data["id"].as<int>();   // Changed from String to int
        time_t created = data["created"].as<long>();
        int duration = data["duration"].as<int>();
        int expired = data["expired"].as<int>(); // kiểm tra biến nếu đúng license còn hiệu lực set_lic và phản hồi.
        int nod = data["nod"].as<int>();
        // Nếu sai thì phản hồi lại lic hết hiệu lực.

        DynamicJsonDocument respDoc(256);
        bool isValid = false;
        String error_msg = "";

        // Kiểm tra LID và ID theo yêu cầu
        if (lid != 0 && lid == config_lid)
        { // LID phải khác 0 và khớp với config_lid
            if (id == 0 || id == config_id)
            {                   // ID có thể là 0 hoặc khớp với config_id
                isValid = true; //  ID khớp với thiết bị hoặc ID = 0
            }
            else
            {
                error_msg = "ID không dành cho thiết bị này"; // ID sai
            }
        }
        else
        {
            error_msg = "LID không hợp lệ"; // LID sai
        }
        if (isValid)
        {
            if (expired)
            {
                globalLicense.created = created;
                globalLicense.duration = duration;
                globalLicense.nod = nod;      // cập nhật số lượng thiết bị
                start_time = millis() / 1000; // đánh dấu mốc thời gian mới
                runtime = 0;
                globalLicense.remain = duration;    // làm mới thời gian còn lại
                globalLicense.expired_flag = false; // chắc chắn đánh dấu
                saveLicenseData();

                // Phản hồi thành công
                respDoc["lid"] = lid;
                respDoc["nod"] = globalLicense.nod; // cập nhật số lượng thiết bị
                respDoc["status"] = 0;              // Thành công
                sendResponse(config_id, id_src, LIC_SET_LICENSE | 0x80, respDoc, from);
                Serial.println("✅ Cập nhật giấy phép thành công: LID = " + String(lid) + ", ID = " + String(id));
                led.setState(FLASH_TWICE); // chớp LED 3 lần để xác nhận
                while (led.isBusy())
                {
                    delay(10); // Chờ LED hoàn thành chớp
                }
                // chờ ngắt rồi reset
                //  delay(200);
                ESP.restart(); // Khởi động lại thiết bị sau khi cập nhật giấy phép
            }
            else
            {
                respDoc["status"] = 3; // license hết hạn
                respDoc["nod"] = globalLicense.nod;
                sendResponse(config_id, id_src, LIC_SET_LICENSE | 0x80, respDoc, from);
                Serial.println("❌ Giấy phép hết hiệu lực");
                led.setState(CONNECTION_ERROR);
            }
        }
        else
        {
            // sai thông tin LID hoặc ID
            respDoc["status"] = 1; // LID hoặc ID không hợp lệ
            respDoc["error_msg"] = error_msg;
            Serial.println("❌ Lỗi: " + error_msg + " (LID = " + String(lid) + ", ID = " + String(id) + ", config_id = " + String(config_id) + ")");
            sendResponse(config_id, id_src, LIC_SET_LICENSE | 0x80, respDoc, from);
            led.setState(CONNECTION_ERROR);
        }
        break;
    }

    case LIC_GET_LICENSE:
    {
        JsonObject data = doc["data"].as<JsonObject>();
        int lid = data["lid"].as<int>();
        DynamicJsonDocument respDoc(256);

        // Kiểm tra LID có hợp lệ không
        if (lid == config_lid || lid == 0)
        {
            respDoc["lid"] = globalLicense.lid;
            respDoc["created"] = globalLicense.created;
            respDoc["expired"] = globalLicense.expired_flag ? 1 : 0;
            respDoc["duration"] = globalLicense.duration;
            respDoc["remain"] = globalLicense.remain;
            respDoc["nod"] = globalLicense.nod;
            respDoc["status"] = 0; // Thành công
            Serial.println("✅ License info sent for LID = " + String(lid));
            sendResponse(config_id, id_src, LIC_GET_LICENSE | 0x80, respDoc, from);
            led.setState(FLASH_TWICE); // chớp xác nhận truy vấn OK

            // Đợi LED chớp xong rồi hiển  thị trạng thái License
            while (led.isBusy())
            {
                led.update(); // duy trì hiệu ứng nhấp nháy
                delay(10);    // Chờ LED hoàn thành chớp
            }
            // Sau khi chớp xong, hiển thị trạng thái License
            if (globalLicense.expired_flag || globalLicense.remain <= 0)
            {
                Serial.println(F("🔒 License đã HẾT HẠN!"));
                led.setState(LICENSE_EXPIRED); // LED tắt
            }
            else
            {
                Serial.printf("🔓 License còn %lu phút\n", globalLicense.remain);
                led.setState(NORMAL_STATUS); // LED sáng liên tục
            }
        }
        else
        {
            Serial.println("❌ LID không hợp lệ: " + String(lid));
            led.setState(CONNECTION_ERROR);
        }
        break;
    }

    case CONFIG_DEVICE:
    {
        JsonObject data = doc["data"].as<JsonObject>();
        int lid = data["new_lid"].as<int>();
        uint32_t nod = data["nod"].as<uint32_t>();
        int id = data["new_id"].as<int>(); // Mặc định lấy config_id

        DynamicJsonDocument respDoc(256);
        bool isValid = false;
        String error_msg; // thông báo lỗi

        // Kiểm tra LID và ID và số lượng thiết bị (NOD)
        if (lid == 0)
        {
            isValid = false;
            error_msg = "LID không hợp lệ";
        }
        else if (id == 0)
        {
            isValid = false;
            error_msg = "ID không hợp lệ";
        }
        else if (id != config_id)
        {
            isValid = false;
            error_msg = "ID không khớp với thiết bị này";
        }
        else
        {
            isValid = true;
        }
        if (isValid)
        {
            // Cập nhật cấu hình
            globalLicense.lid = lid;
            globalLicense.id = id;
            globalLicense.nod = nod;

            preferences.begin("license", false);
            preferences.putUInt("lid", globalLicense.lid);
            preferences.putUInt("id", globalLicense.id);
            preferences.putUInt("nod", globalLicense.nod);
            preferences.end();

            // chớp LED để xác nhận
            led.setState(FLASH_TWICE); // chớp 3 lần
            Serial.println("✅ Cấu hình thành công: LID = " + String(lid) + ", ID = " + String(id) + ", NOD = " + String(nod));

            respDoc["status"] = 0;
            respDoc["lid"] = globalLicense.lid;
            respDoc["id"] = globalLicense.id;
            respDoc["nod"] = globalLicense.nod;
        }
        else
        {
            respDoc["status"] = 1;
            respDoc["error_msg"] = error_msg;
            Serial.println("❌ Lỗi: " + error_msg + " (LID = " + String(lid) + ", ID = " + String(id) + ", config_id = " + String(config_id) + ")");
        }

        sendResponse(config_id, id_src, CONFIG_DEVICE | 0x80, respDoc, from);
        break;
    }

    case LIC_LICENSE_DELETE:
    {
        int lid = doc["data"]["lid"].as<int>();
        DynamicJsonDocument respDoc(256);
        respDoc["lid"] = lid;
        respDoc["status"] = (lid == globalLicense.lid) ? 0 : 3; // Thành công hoặc không tìm thấy
        if (lid == globalLicense.lid)
        {
            // Xóa license bằng cách đặt về giá trị mặc định
            globalLicense.lid = 0;
            globalLicense.created = 0;
            globalLicense.duration = 0;
            globalLicense.remain = 0;
            globalLicense.expired_flag = false;
            saveLicenseData(); // Lưu trạng thái mới
        }
        sendResponse(config_id, id_src, LIC_LICENSE_DELETE | 0x80, respDoc, from);
        Serial.println(lid == globalLicense.lid ? "✅ Đã xóa license cho LID = " + String(lid) : "❌ Không tìm thấy license cho LID = " + String(lid));
        led.setState(lid == globalLicense.lid ? NORMAL_STATUS : CONNECTION_ERROR);
        break;
    }

    case LIC_LICENSE_DELETE_ALL:
    {
        // Xóa tất cả license
        globalLicense.lid = 0;
        globalLicense.created = 0;
        globalLicense.duration = 0;
        globalLicense.remain = 0;
        globalLicense.expired_flag = false;
        saveLicenseData(); // Lưu trạng thái mới
        DynamicJsonDocument respDoc(256);
        respDoc["status"] = 0; // Thành công
        sendResponse(config_id, id_src, LIC_LICENSE_DELETE_ALL | 0x80, respDoc, from);
        Serial.println("✅ Đã xóa tất cả license.");
        led.setState(NORMAL_STATUS);
        break;
    }

    case LIC_TIME_GET:
    {
        DynamicJsonDocument respDoc(256);
        respDoc["time"] = millis() / 1000; // Trả về thời gian receiver
        respDoc["status"] = 0;
        sendResponse(config_id, id_src, LIC_TIME_GET | 0x80, respDoc, from);
        Serial.println("✅ Time info sent.");
        break;
    }

    case LIC_INFO:
    {
        DynamicJsonDocument respDoc(256);
        respDoc["deviceName"] = globalLicense.deviceName;
        respDoc["version"] = globalLicense.version;
        respDoc["status"] = 0;
        sendResponse(config_id, id_src, LIC_INFO_RESPONSE, respDoc, from);
        Serial.println("✅ Device info sent.");
        break;
    }

    default:
    {
        DynamicJsonDocument respDoc(256);
        respDoc["status"] = 255; // Opcode không xác định
        sendResponse(config_id, id_src, opcode | 0x80, respDoc, from);
        Serial.printf("❌ Unknown opcode: 0x%02X\n", opcode);
        led.setState(CONNECTION_ERROR);
        break;
    }
    }
}
#endif // PROTOCOL_HANDLER_H
