#include "function.h"

void update_RTC(char *Hour, char *Minute, char *Second)
{
    char RTC[10];
    sprintf(RTC, "%2s:%2s:%2s", Hour, Minute, Second); // Sử dụng %s thay vì %d vì Hour, Minute, Second là chuỗi
    // if (ui_RTC3 != NULL) {
    //     lv_label_set_text(ui_RTC3, RTC);
    // }
}

void update_config(char *localID, char *deviceID, char *nod)
{
    if (ui_TextArea9 != NULL)
    {
        lv_textarea_set_text(ui_TextArea9, localID);
    }
    if (ui_TextArea8 != NULL)
    {
        lv_textarea_set_text(ui_TextArea8, deviceID);
        lv_textarea_set_text(ui_TextArea10, nod); // Lưu ý: Ghi đè deviceID bằng nod, kiểm tra xem có đúng ý định không
    }
}

void get_lic_ui()
{
    if (ui_TextArea5 != NULL)
    {
        datalic.lid = atoi(lv_textarea_get_text(ui_TextArea5));
    }
    if (ui_TextArea4 != NULL)
    {
        Device_ID = atoi(lv_textarea_get_text(ui_TextArea4));
    }
    if (ui_TextArea6 != NULL && ui_TextArea7 != NULL)
    {
        uint32_t temp = (uint32_t)atol(lv_textarea_get_text(ui_TextArea6)) * 60 + (uint32_t)atol(lv_textarea_get_text(ui_TextArea7)); // thêm 2 biến uint32_t
        datalic.duration = temp;
        datalic.expired = true;
    }
}

void get_id_lid_ui()
{
    if (ui_TextArea3 != NULL)
    {
        datalic.lid = atoi(lv_textarea_get_text(ui_TextArea3));
    }
    if (ui_TextArea2 != NULL)
    {
        Device_ID = atoi(lv_textarea_get_text(ui_TextArea2));
    }
}

// void Notification(char* messager) {

//     // lv_label_set_text(ui_Label19, messager);
//     // _ui_flag_modify(ui_Panel21, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
//     add_Notification(ui_SCRSetLIC,messager);

// }

void timer_cb(lv_timer_t *timer_)
{
    if (timer_ == NULL)
    {
        return;
    }
    lv_obj_t *spinner = (lv_obj_t *)timer_->user_data;

    if (spinner != NULL && lv_obj_is_valid(spinner))
    {
        lv_obj_del(spinner);
        if (spinner == ui_spinner)
        {
            ui_spinner = NULL;
        }
        else if (spinner == ui_spinner1)
        {
            ui_spinner1 = NULL;
        }
    }

    // lv_obj_del(spinner);
    enable_print_ui = true;
    lv_timer_del(timer_);
    timer = NULL;
}