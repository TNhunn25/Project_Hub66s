#ifndef FUNCTION_H
#define FUNCTION_H
#include <Arduino.h>
#include "stdio.h"
#include "ui.h"

typedef struct {
    long lid;
    long created;
    long expired;
    long duration;
} Licence;
extern Licence datalic;

#define MAX_DEVICES 200
typedef struct {
    uint8_t MACList[MAX_DEVICES][6]; // Lưu địa chỉ MAC (6 bytes)
    int DeviceID[MAX_DEVICES];
    int LocalID[MAX_DEVICES];
    unsigned long timeLIC[MAX_DEVICES];
    int deviceCount;
} device_info;
extern device_info Device;

extern int Device_ID;
extern bool enable_print_ui;
extern uint8_t button;
extern lv_timer_t * timer;
extern char messger[100];
extern bool enable_print_ui_set;
// Khai báo các hàm
void update_RTC(char* Hour, char* Minute, char* Second);
void update_config(char* localID, char* deviceID, char* nod);
void get_lic_ui();
void get_id_lid_ui();
void Notification(char* messager);
void timer_cb(lv_timer_t * timer);

#endif // FUNCTION_H