#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "protocol_handler.h" // cho createMessage()
#include "mesh_handler.h"     // cho meshBroadcastJson()

// ==== Nhận cấu hình id và lid từ Serial Monitor ====
// inline void rec_PC()
// {
//   if (!Serial.available())
//     return;
//   String input = Serial.readStringUntil('\n');
//   input.trim();
//   if (input.length() == 0)
//   {
//     Serial.println("Sử dụng id/lid mặc định");
//     config_received = true;
//     return;
//   }
//   int sep = input.indexOf(' ');
//   if (sep < 0)
//   {
//     Serial.println("Sai định dạng. Nhập: <id> <lid>");
//     return;
//   }
//   String idStr = input.substring(0, sep);
//   String lidStr = input.substring(sep + 1);
//   idStr.trim();
//   lidStr.trim();
//   if (idStr.length() == 0 || lidStr.length() == 0)
//   {
//     Serial.println("ID hoặc LID trống, vui lòng nhập lại.");
//     return;
//   }
//   config_id = idStr.toInt();
//   config_lid = lidStr.toInt();
//   Serial.printf("Đã cấu hình: id=%d, lid= %d\n", config_id, config_lid);
//   config_received = true;
// }

// // ==== Nhận JSON lệnh từ PC, parse và gọi processReceivedData() ====
// static const int PC_BUFFER_SIZE = 512;
// // static char pcBuffer[PC_BUFFER_SIZE];
// static int pcBufIndex = 0;

// nhận chuỗi json từ pc
void serial_pc()
{
  while (Serial.available() > 0)
  {
    char incomingChar = Serial.read();

    if (bufferIndex < BUFFER_SIZE - 1)
    {
      jsonBuffer[bufferIndex++] = incomingChar;
    }

    // Kiểm tra kết thúc chuỗi bằng '\n'
    if (incomingChar == '\n')
    {
      jsonBuffer[bufferIndex] = '\0'; // Kết thúc chuỗi

      Serial.println("Đã nhận JSON:");
      Serial.println(jsonBuffer);

      // Parse JSON
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, jsonBuffer);

      if (error)
      {
        Serial.print("Lỗi JSON: ");
        Serial.println(error.f_str());
      }
      else
      {
        // Lấy trường ngoài
        const char *id = doc["id"];
        const char *mac_des = doc["mac"];
        int opcode = doc["opcode"];
        long time = doc["time"];
        const char *auth = doc["auth"];

        // Lấy object data bên trong
        JsonObject data = doc["data"];
        long lid = data["lid"];
        long created = data["created"];
        long expired = data["expired"];
        long duration = data["duration"];

        // In dữ liệu nhận được
        Serial.print("ID: ");
        Serial.println(id);
        Serial.print("MAC: ");
        Serial.println(mac_des);
        Serial.print("Opcode: ");
        Serial.println(opcode);
        Serial.print("Time: ");
        Serial.println(time);
        Serial.print("Auth: ");
        Serial.println(auth);
        Serial.println("== Data Object ==");
        Serial.print("LID: ");
        Serial.println(lid);
        Serial.print("Created: ");
        Serial.println(created);
        Serial.print("Expired: ");
        Serial.println(expired);
        Serial.print("Duration: ");
        Serial.println(duration);

        // So sánh opcode
        Serial.print("Xử lý opcode: ");
        switch (opcode)
        {
        case 1: // LIC_TIME_GET
          Serial.println("LIC_TIME_GET (Yêu cầu PC gửi gói tin LIC_TIME)");
          break;
        case 2: // HUB_SET_LICENSE
          Serial.println("HUB_SET_LICENSE (Tạo bản tin LICENSE)");

          break;
        case 3: // HUB_GET_LICENSE
          // getlicense(config_lid,WiFi.macAddress(),now);
          Serial.println("HUB_GET_LICENSE (Đọc trạng thái license của HUB66S)");
          break;
        case 4: // LIC_LICENSE_DELETE
          Serial.println("LIC_LICENSE_DELETE (Xóa bản tin đã gửi)");
          break;
        case 5: // LIC_LICENSE_DELETE_ALL
          Serial.println("LIC_LICENSE_DELETE_ALL (Xóa toàn bộ các bản tin)");
          break;
        case 6: // LIC_INFO
          Serial.println("LIC_INFO (Cập nhật thông tin LIC66S)");
          break;
        default:
          Serial.println("Không xác định opcode!");
          break;
        }
      }

      // Reset buffer
      bufferIndex = 0;
    }
  }
}

#endif // SERIAL_H
