#include "function.h"

void update_RTC(char* Hour, char* Minute, char* Second) {
    char RTC[10];
    sprintf(RTC, "%2s:%2s:%2s", Hour, Minute, Second); // Sử dụng %s thay vì %d vì Hour, Minute, Second là chuỗi
    // if (ui_RTC3 != NULL) {
    //     lv_label_set_text(ui_RTC3, RTC);
    // }
}

void update_config(char* localID, char* deviceID, char* nod) {
    if (ui_TextArea9 != NULL) {
        lv_textarea_set_text(ui_TextArea9, localID);
    }
    if (ui_TextArea8 != NULL) {
        lv_textarea_set_text(ui_TextArea8, deviceID);
        lv_textarea_set_text(ui_TextArea8, nod); // Lưu ý: Ghi đè deviceID bằng nod, kiểm tra xem có đúng ý định không
    }
}

void get_lic_ui() {
    if (ui_TextArea5 != NULL) {
        datalic.lid = atoi(lv_textarea_get_text(ui_TextArea5));
    }
    if (ui_TextArea4 != NULL) {
        Device_ID = atoi(lv_textarea_get_text(ui_TextArea4));
    }
    if (ui_TextArea6 != NULL && ui_TextArea7 != NULL) {
        long temp = atol(lv_textarea_get_text(ui_TextArea6)) * 60 + atol(lv_textarea_get_text(ui_TextArea7));
        datalic.duration = temp;
        datalic.expired = true;
    }
}

void get_id_lid_ui() {
    if (ui_TextArea3 != NULL) {
        datalic.lid = atoi(lv_textarea_get_text(ui_TextArea3));
    }
    if (ui_TextArea2 != NULL) {
        Device_ID = atoi(lv_textarea_get_text(ui_TextArea2));
    }
}

// void Notification(char* messager) {

//     // lv_label_set_text(ui_Label19, messager);
//     // _ui_flag_modify(ui_Panel21, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
//     add_Notification(ui_SCRSetLIC,messager);

// }




void timer_cb(lv_timer_t * timer_) {
    if (timer_ == NULL || timer_->user_data == NULL) {
        return;
    }
    lv_obj_t * spinner = (lv_obj_t *)timer_->user_data;
    lv_obj_del(spinner);
    enable_print_ui = true;
    lv_timer_del(timer_);
    timer = NULL;
    
}