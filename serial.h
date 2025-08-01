#ifndef SERIAL_H
#define SERIAL_H

#include "config.h"
#include <ArduinoJson.h>
extern Preferences preferences;

void recPC()
{
  while (Serial.available())
  {
    char c = Serial.read();
    if (c == '{')
    {
      bufferIndex = 0;
      jsonBuffer[bufferIndex++] = c;
    }
    else if (bufferIndex > 0)
    {
      if (bufferIndex < BUFFER_SIZE - 1)
      {
        jsonBuffer[bufferIndex++] = c;
      }
      if (c == '}')
      {
        memset(jsonBuffer, 0, BUFFER_SIZE);
        bufferIndex = 0;
        jsonBuffer[bufferIndex++] = c;
        jsonBuffer[bufferIndex] = '\0'; // Kết thúc chuỗi
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer);
        if (error)
        {
          Serial.print("❌ deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        config_lid = doc["lid"];
        String config_id = doc["id_src"];
        String id_des = doc["id_des"];
        time_t created = doc["created"].as<long>();
        int duration = doc["duration"].as<int>();
        int expired = doc["expired"].as<int>();
        time_t now = doc["time"].as<long>();

        config_processed = true;

        StaticJsonDocument<256> respDoc;
        respDoc["lid"] = config_lid;
        respDoc["status"] = 0;
        String output;
        serializeJson(respDoc, output);
        Serial.println(output);
      }
    }
  }

  // Nếu đã xử lý cấu hình thì lưu vào preferences
  if (config_processed)
  {
    preferences.begin("license", false);
    preferences.putInt("config_lid", config_lid);
    preferences.putInt("id_des", id_des);
    preferences.end();
  }
}

void serialPC()
{
  if (config_processed)
  {
    StaticJsonDocument<256> respDoc;
    respDoc["lid"] = config_lid;
    respDoc["status"] = 0;
    String output;
    serializeJson(respDoc, output);
    Serial.println(output);
    config_processed = false;
  }
}

#endif
