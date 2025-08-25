#ifndef SERIAL_H
#define SERIAL_H

#include "config.h"
#include <ArduinoJson.h>

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
        jsonBuffer[min(bufferIndex, BUFFER_SIZE - 1)] = '\0';
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer);
        if (error)
        {
          Serial.print("❌ deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        config_lid = doc["lid"];
        int id_src = doc["id_src"];
        id_des = doc["id_des"];
        String mac_src = doc["mac_src"].as<String>();
        String mac_des = doc["mac_des"].as<String>();
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
