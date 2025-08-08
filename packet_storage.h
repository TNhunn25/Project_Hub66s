#ifndef PACKET_STORAGE_H
#define PACKET_STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

struct LicenseRecord
{
    int deviceID;
    int localID;
    int numberDevice;
    uint32_t mac;
    unsigned long time;
};

static Preferences packetPrefs;

inline void initPacketStorage()
{
    packetPrefs.begin("packets", false);
}

inline void storeLicenseRecord(const LicenseRecord &rec)
{
    packetPrefs.putBytes("last", &rec, sizeof(rec));
    Serial.println("Đã lưu thông tin license vào flash");
}

inline bool loadLastLicenseRecord(LicenseRecord &rec)
{
    if (!packetPrefs.isKey("last"))
    {
        return false;
    }
    packetPrefs.getBytes("last", &rec, sizeof(rec));
    return true;
}

#endif // PACKET_STORAGE_H