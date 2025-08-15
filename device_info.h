#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <Arduino.h>

struct DeviceInfo {
    uint32_t nodeId = 0;        // Node ID trong mạng mesh
    String macAddress;          // Địa chỉ MAC dạng chuỗi
    String name;                // Tên thiết bị
    int lid = 0;                // License ID của node
    int id = 0;                 // ID cấu hình của node
    String version;             // Phiên bản phần mềm
    String firmware;            // Tên/phiên bản firmware
    bool connected = false;     // Trạng thái kết nối
    unsigned long lastSeen = 0; // Thời điểm nhận gói tin cuối cùng
};

class DeviceRegistry {
  public:
    static const size_t MAX_DEVICES = 20;

    DeviceInfo *find(uint32_t nodeId) {
        for (size_t i = 0; i < MAX_DEVICES; ++i) {
            if (devices_[i].nodeId == nodeId) {
                return &devices_[i];
            }
        }
        return nullptr;
    }

    DeviceInfo *upsert(uint32_t nodeId, const String &mac, const String &name,
                       int lid, int id, const String &version,
                       const String &firmware) {
        DeviceInfo *info = find(nodeId);
        if (!info) {
            for (size_t i = 0; i < MAX_DEVICES; ++i) {
                if (devices_[i].nodeId == 0) {
                    info = &devices_[i];
                    info->nodeId = nodeId;
                    break;
                }
            }
        }
        if (info) {
            info->macAddress = mac;
            info->name = name;
            info->lid = lid;
            info->id = id;
            info->version = version;
            info->firmware = firmware;
            info->connected = true;
            info->lastSeen = millis();
        }
        return info;
    }

    size_t size() const {
        size_t count = 0;
        for (size_t i = 0; i < MAX_DEVICES; ++i) {
            if (devices_[i].nodeId != 0) {
                ++count;
            }
        }
        return count;
    }

  private:
    DeviceInfo devices_[MAX_DEVICES];
};

extern DeviceRegistry deviceRegistry;

#endif // DEVICE_INFO_H