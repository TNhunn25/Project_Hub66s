#ifndef PACKET_STORAGE_H
#define PACKET_STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

// Lưu trữ dữ liệu gói tin vào flash để giải phóng RAM
// Sử dụng NVS thông qua thư viện Preferences
static Preferences packetPrefs;
static uint32_t packetIndex = 0; // Chỉ số gói tin hiện tại trong NVS

inline void initPacketStorage()
{
    // Mở bộ nhớ flash với namespace "packets"
    packetPrefs.begin("packets", false);
    // Lấy chỉ số gói tin cuối cùng đã lưu
    packetIndex = packetPrefs.getUInt("idx", 0);
}

inline void storePacketToFlash(const String &packet)
{
    // Lưu chuỗi gói tin vào flash với key riêng
    String key = "pkt" + String(packetIndex++);
    packetPrefs.putString(key.c_str(), packet);
    packetPrefs.putUInt("idx", packetIndex);
}

inline String loadPacketFromFlash(uint32_t index)
{
    // Đọc gói tin theo chỉ số từ flash
    String key = "pkt" + String(index);
    return packetPrefs.getString(key.c_str(), "");
}

// Đọc gói tin mới nhất
inline String loadLastPacketFromFlash()
{
    if (packetIndex == 0)
        return "";
    return loadPacketFromFlash(packetIndex - 1);
}

#endif // PACKET_STORAGE_H
