#ifndef DEVICE_STORAGE_H
#define DEVICE_STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include "function.h" // for device_info structure

// Lưu trữ danh sách thiết bị vào flash để giảm sử dụng RAM
static Preferences devicePrefs;

inline void initDeviceStorage()
{
    // Mở NVS với namespace "devices"
    devicePrefs.begin("devices", false);
}

inline void saveDeviceList(const device_info &info)
{
    // Ghi toàn bộ cấu trúc device_info vào flash
    devicePrefs.putBytes("list", &info, sizeof(device_info));
}

inline bool loadDeviceList(device_info &info)
{
    if (!devicePrefs.isKey("list"))
    {
        return false; // Chưa có dữ liệu lưu
    }
    devicePrefs.getBytes("list", &info, sizeof(device_info));
    return true;
}

#endif // DEVICE_STORAGE_H