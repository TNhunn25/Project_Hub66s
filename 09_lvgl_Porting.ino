#include <Arduino.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include <time.h>

#include "protocol_handler.h"
#include "mesh_handler.h"
#include "serial.h"
#include "config.h"
#include "function.h"
#include "led_status.h"

// Biến toàn cục từ mesh_handler.h
painlessMesh mesh;
bool meshReady = false; // định nghĩa cờ từ mesh_handler.h

// Biến toàn cục
LedStatus led(LED_PIN); // LED nối chân 2
LicenseInfo globalLicense;
PayloadStruct message;
LicenseInfo licenseInfo; // Biến lưu trạng thái license
device_info Device;
char jsonBuffer[BUFFER_SIZE];
int bufferIndex;
char messger[100];
uint8_t button = 0;
// Biến lưu cấu hình
int config_lid = 123;
int config_id = 2005;   // ID của LIC66S
int id_des = MASTER_ID; // ID của MASTER (HUB66S)
bool config_received = false;
int next_page = 0;
int old_page = 0;
// Biến retry ACK
bool awaitingAck = false; // mục 7: theo dõi ack/retry
unsigned long lastSentTime = 0;
uint8_t lastOpcode;   // lưu opcode để retry
JsonVariant lastData; // lưu data để retry

// Biến LED
bool errorState = false;
unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = LOW;

// Biến thời gian license
time_t start_time = 0;
const int duration = 60; // 60 phút
time_t now = 0;

// Biến expired
bool expired_flag = false; // Biến logic kiểm soát trạng thái còn hạn
int expired = expired_flag ? 1 : 0; // 1 là hết hạn, 0 là còn hạn
// lastTargetNode = from;
uint32_t lastTargetNode = 0;

// bool = hasSend = false; // Biến kiểm soát đã gửi lệnh hay chưa

// Kiểm tra xem MAC đã tồn tại trong danh sách chưa
bool isMacExist(uint32_t nodeId)
{
    for (int i = 0; i < Device.deviceCount; i++)
    {
        if (Device.NodeID[i] == nodeId)
        {
            return true; // MAC đã tồn tại
        }
    }
    return false;
}

// Thêm MAC vào danh sách
void addNodeToList(int id, int lid, uint32_t nodeId, unsigned long time_)
{
    if (Device.deviceCount < MAX_DEVICES && !isMacExist(nodeId))
    {
        Device.NodeID[Device.deviceCount] = nodeId;
        Device.DeviceID[Device.deviceCount] = id;
        Device.LocalID[Device.deviceCount] = lid;
        Device.timeLIC[Device.deviceCount] = time_;

        Device.deviceCount++;
        char macStr[18];
        // snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        //          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        Serial.print("Thiết bị mới: ");
        Serial.println(macStr);
        // timer_out=millis();

        // lv_timer_reset(timer);
    }
}

// Hiển thị danh sách thiết bị đã lưu
void printDeviceList()
{
    Serial.println("Danh sách thiết bị đã tìm thấy:");
    for (int i = 0; i < Device.deviceCount; i++)
    {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 Device.NodeID[i] >> 24, (Device.NodeID[i] >> 16) & 0xFF,
                 (Device.NodeID[i] >> 8) & 0xFF, Device.NodeID[i] & 0xFF,
                 Device.DeviceID[i], Device.LocalID[i]);
        Serial.print("Thiết bị ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(macStr);
    }
    Serial.println("------------------");
}

// Xử lý khi nhận phản hồi
void handleScanResponse(uint32_t nodeId, int device_id, int local_id)
{
    if (!isMacExist(nodeId))
    {
        addNodeToList(device_id, local_id, nodeId, millis());
    }
    else
    {
        Serial.printf("Thiết bị 0x%08X đã tồn tại, bỏ qua phản hồi\n", nodeId);
    }
}

void setup()
{
    String title = "LVGL porting example";

    Serial.begin(115200);
    Serial.println("[SENDER] Starting...");

    // Khởi tạo Wi-Fi Mesh
    WiFi.mode(WIFI_AP_STA);
    // delay(100);                       // Đợi WiFi mode ổn định
    // WiFi.setTxPower(WIFI_POWER_2dBm); // Giảm công suất phát để tiết kiệm năng lượng

    // Đồng bộ thời gian qua NTP để timestamp đúng
    configTime(0, 0, "pool.ntp.org");

    initMesh();                     // Khởi tạo mạng Mesh
    mesh.onReceive(&meshReceiveCb); // Đăng ký callback nhận dữ liệu Mesh

    // LED trạng thái
    led.setState(CONNECTION_ERROR);

    Board *board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);
    // Khởi tạo ui.
    ui_init();
    /* Release the mutex */
    lvgl_port_unlock();
}
bool ledstt = 1;

void loop()
{
    // 1) Chạy meshLoop chỉ khi đã init xong
    if (meshReady)
    {
        meshLoop();

        // 7) Retry nếu chưa có ACK sau 5s
        if (awaitingAck && millis() - lastSentTime > 5000)
        {
            Serial.println("⏱ Retry last command");
            String output = createMessage(config_id, 0, mesh.getNodeId(), lastTargetNode,
                                          lastOpcode, lastData.as<JsonVariant>(), millis());

            mesh.sendSingle(lastTargetNode, output);
            Serial.println("📤 Re-sent:");
            Serial.println(output);
            lastSentTime = millis();
        }
    }

    // 2) Đợi config từ PC qua Serial
    // if (!config_received)
    // {
    //     rec_PC();
    //     return;
    // }

    // 3) Nhận lệnh JSON từ PC và gửi lên mesh
    serial_pc();

    // // 4) Ví dụ gửi GET_LICENSE định kỳ mỗi 10s
    // static unsigned long timer = 0;
    // if (millis() - timer > 10000)
    // {
    //     timer = millis();
    //     DynamicJsonDocument doc(128);
    //     doc["lid"] = config_lid;
    //     processReceivedData(doc.as<JsonVariant>(), LIC_GET_LICENSE);
    // }

    // 5) Cập nhật trạng thái LED
    led.update();

    if (button != 0)
    {
        // uint32_t mac_des = 0; // 0: broadcast hoặc bạn có thể chỉ định node cụ thể
        Serial.println(button);
        Serial.println("button pressed: ");
        switch (button)
        {
        case 1:
            Serial.println("Gửi lệnh LIC_SET_LICENSE");

            set_license(Device_ID, datalic.lid, mac_nhan, millis(), datalic.duration, expired, millis());
            break;
        case 4:

            Serial.println("Gửi lệnh LIC_GET_LICENSE");    

            getlicense(Device_ID, datalic.lid, mac_nhan, millis());
            // enable_print_ui=true;
            // timer_out=millis();
            break;
        case 5:
            memset(&Device, 0, sizeof(Device));
            getlicense(Device_ID, mac_nhan, datalic.lid, millis());
            // enable_print_ui=true;
            // timer_out=millis();
            break;
        default:
            break;
        }
        button = 0;
    }
    // delay(10);

    if (enable_print_ui_set)
    {

        // if(millis()-timer_out>1000){
        lv_obj_t *OBJ_Notification = add_Notification(ui_SCRSetLIC, messger);
        enable_print_ui_set = false;
        // }
    }

    if (enable_print_ui || (old_page != next_page))
    {

        // if(millis()-timer_out>5000){
        // if (ui_spinner1 != NULL) {
        // lv_anim_del(ui_spinner1, NULL);
        // lv_obj_del(ui_spinner1); // Xóa đối tượng spinner
        // ui_spinner1 = NULL; // Đặt con trỏ về NULL để tránh tham chiếu sai
        // }
        lv_obj_clean(ui_Groupdevice);
        lv_obj_invalidate(ui_Groupdevice);

        // lv_obj_invalidate(lv_scr_act());
        if ((next_page * maxLinesPerPage) >= Device.deviceCount)
        {
            next_page = 0; // Quay về trang đầu
        }
        old_page = next_page;
        char buf[64];
        snprintf(buf, sizeof(buf), "LIST DEVICE: %2d - Page %2d", Device.deviceCount + 1, next_page + 1);
        Serial.printf("BUF= %s\n", buf);
        lv_label_set_text(ui_Label7, buf);
        // int startIdx = next_page * maxLinesPerPage;
        // int endIdx = startIdx + maxLinesPerPage;

        // if (endIdx > Device.deviceCount)
        // {
        //     endIdx = Device.deviceCount;
        // }
        // Serial.printf("%d %d %d \n ", startIdx, endIdx, next_page);

        // for (int i = startIdx; i < endIdx; i++)
        // {
        //     char macStr[100];
        //     char idStr[100];
        //     char lidStr[100];
        //     char timeStr[100];

        //     Serial.printf("Node %d: 0x%08X\n", i + 1, Device.NodeID[i]);
        //     snprintf(macStr, sizeof(macStr), "0x%08X", Device.NodeID[i]);
        //     snprintf(idStr, sizeof(idStr), "%d", Device.DeviceID[i]);
        //     snprintf(lidStr, sizeof(lidStr), "%d", Device.LocalID[i]);
        //     snprintf(timeStr, sizeof(timeStr), "%d", Device.timeLIC[i]);

        //     lv_obj_t *ui_DeviceINFO = ui_DeviceINFO1_create(ui_Groupdevice, idStr, lidStr, "****", macStr, timeStr);
        // }
        // enable_print_ui = false;

        int startIdx = next_page * maxLinesPerPage;
        int endIdx = startIdx + maxLinesPerPage;
        if (endIdx > Device.deviceCount)
            endIdx = Device.deviceCount;
        if (startIdx >= endIdx || startIdx < 0 || endIdx > MAX_DEVICES)
        {
            enable_print_ui = false;
            return;
        }

        if (ui_Groupdevice)
        {
            lv_obj_clean(ui_Groupdevice);
            lv_obj_invalidate(ui_Groupdevice);
        }
        if (ui_Label7)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "LIST DEVICE: %2d - Page %2d", Device.deviceCount, next_page + 1);
            lv_label_set_text(ui_Label7, buf);
        }

        for (int i = startIdx; i < endIdx; i++)
        {
            if (i < 0 || i >= MAX_DEVICES)
                continue;
            char macStr[18], idStr[18], lidStr[18], timeStr[18];
            snprintf(macStr, sizeof(macStr), "0x%08X", Device.NodeID[i]);
            snprintf(idStr, sizeof(idStr), "%d", Device.DeviceID[i]);
            snprintf(lidStr, sizeof(lidStr), "%d", Device.LocalID[i]);
            snprintf(timeStr, sizeof(timeStr), "%d", Device.timeLIC[i]);
            lv_obj_t *ui_DeviceINFO = ui_DeviceINFO1_create(ui_Groupdevice, idStr, lidStr, "****", macStr, timeStr);
        }
        enable_print_ui = false;
    }
}
