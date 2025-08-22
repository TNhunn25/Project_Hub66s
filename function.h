#ifndef FUNCTION_H
#define FUNCTION_H
#include <Arduino.h>
#include "stdio.h"
#include "ui.h"

typedef struct {
    long lid;
    uint32_t created;
    uint32_t expired;
    uint32_t duration;
} Licence;
extern Licence datalic;

#define MAX_DEVICES 100
typedef struct {
    uint32_t NodeID[MAX_DEVICES]; // Lưu nodeId của từng thiết bị
    int DeviceID[MAX_DEVICES];
    int LocalID[MAX_DEVICES];
    uint32_t timeLIC[MAX_DEVICES];
    int deviceCount;
} device_info;
extern device_info Device;

void addNodeToList(int id_src, int lid, uint32_t nodeId, uint32_t timestamp);
void printDeviceList();
void handleScanResponse(uint32_t nodeId, int device_id, int local_id, uint32_t time_);

extern int Device_ID;
extern bool enable_print_ui;
extern uint8_t button;
extern lv_timer_t * timer;
extern char messger[128];
extern bool enable_print_ui_set;
extern int next_page;

// Khai báo các hàm
void update_RTC(char* Hour, char* Minute, char* Second);
void update_config(char* localID, char* deviceID, char* nod);
void get_lic_ui();
void get_id_lid_ui();
void Notification(char* messager);
void timer_cb(lv_timer_t * timer);

#endif // FUNCTION_H