#ifndef PACKET_STORAGE_H
#define PACKET_STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

// Lưu trữ dữ liệu gói tin vào flash để giải phóng RAM
// Sử dụng NVS thông qua thư viện Preferences
static Preferences packetPrefs;

inline void initPacketStorage()
{
    // Mở bộ nhớ flash với namespace "packets"
    packetPrefs.begin("packets", false);
}

inline void storePacketToFlash(const String &packet)
{
    // Lưu chuỗi gói tin vào flash
    packetPrefs.putString("last", packet);
}

inline String loadPacketFromFlash()
{
    // Đọc lại gói tin đã lưu trong flash
    return packetPrefs.getString("last", "");
}

#endif // PACKET_STORAGE_H