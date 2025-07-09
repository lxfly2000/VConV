#pragma once

#include <string>

#define SERVER_IP_DEFAULT "192.168.1.2"

//存储used_keycodes_3ds到used_keycodes_xbox的索引
extern int button_mapping_3ds_xbox[18];
extern std::string serverIp;
extern unsigned short serverPort;
extern std::string error_msg;
extern bool running;
extern bool screen_light;
extern short xbox_controller_key_status[23];//使用中的Xbox手柄按键状态，下标是KeyCode
extern int button_mapping_onscreen_buttons_xbox[4];//触屏按钮对应的xbox按键索引

bool vconv_init();
bool vconv_release();

bool vconv_send();

void update_sockets_config();

bool read_settings();
bool save_settings();
