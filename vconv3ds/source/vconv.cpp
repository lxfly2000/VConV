#include "vconv.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>

#include <3ds.h>

#include "utilities.h"
#include "keycodes.h"

#define SETTINGS_FILE "vconv.txt"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000


bool running=true;
bool screen_light=true;
int button_mapping_3ds_xbox[18]={11,12,6,5,4,3,1,2,10,9,13,14,15,16,17,18,19,20};
int button_mapping_onscreen_buttons_xbox[4]={15,16,7,8};
unsigned char onscreen_buttons_key_status=0;
unsigned char last_onscreen_buttons_key_status=0;
short xbox_controller_key_status[23]={0};
std::string serverIp=SERVER_IP_DEFAULT;
unsigned short serverPort=32000;
std::string error_msg="";
sockaddr_in addrSend={0};

u32 *SOC_buffer = NULL;
int socketfd;

bool vconv_init()
{
    read_settings();//读取设置出错不中断
    update_sockets_config();
    // allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		error_msg="memalign: failed to allocate";
        return false;
	}

	// Now intialise soc:u service
    Result ret;
	if ((ret=socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    	error_msg="socInit: "+(unsigned int)ret;
        return false;
	}
    socketfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(socketfd==-1){
        error_msg="socket error: "+std::to_string(errno);
        return false;
    }
    
    error_msg="Init OK. Check for update: https://github.com/lxfly2000/VConV";
    return true;
}

bool vconv_release()
{
    if(closesocket(socketfd)){
        return false;
    }
    socExit();
    save_settings();
    return true;
}

void update_sockets_config()
{
    addrSend.sin_family=AF_INET;
    addrSend.sin_port=htons(serverPort);
    addrSend.sin_addr.s_addr=inet_addr(serverIp.c_str());
}


bool read_settings()
{
	std::ifstream f(SETTINGS_FILE,std::ios::in);
    if(!f){
        return false;
    }
    f>>serverIp>>serverPort;
    for(int i=0;i<ARRAYSIZE(button_mapping_3ds_xbox);i++){
        f>>button_mapping_3ds_xbox[i];
    }
    for(int i=0;i<ARRAYSIZE(button_mapping_onscreen_buttons_xbox);i++){
        f>>button_mapping_onscreen_buttons_xbox[i];
    }
    return true;
}

bool save_settings()
{
	std::ofstream f(SETTINGS_FILE,std::ios::out);
    if(!f){
        return false;
    }
    f<<serverIp<<" "<<serverPort<<std::endl;
    for(int i=0;i<ARRAYSIZE(button_mapping_3ds_xbox);i++){
        if(i>0)f<<" ";
        f<<button_mapping_3ds_xbox[i];
    }
    f<<std::endl;
    for(int i=0;i<ARRAYSIZE(button_mapping_onscreen_buttons_xbox);i++){
        if(i>0)f<<" ";
        f<<button_mapping_onscreen_buttons_xbox[i];
    }
    return true;
}

circlePosition lastPadPos={0,0};
circlePosition lastCsPos={0,0};
u32 make_send_data(int keycode,int value)
{
    return (keycode&0xFF)|(value<<8);
}

void printSendData(void*p,int length)
{
    error_msg="Send: Length="+std::to_string(length)+", ["+std::to_string(((u8*)p)[0])+", ";
    if(length==2){
        error_msg+=std::to_string(*(u8*)(p+1));
    }else if(length==3){
        error_msg+=std::to_string(*(short*)(p+1));
    }
    error_msg+="]";
}

#define check_send_to(p,l) if(!check_send_to_rt(p,l))return false
bool check_send_to_rt(void*p,int length)
{
    if(-1==sendto(socketfd,p,length,NULL,(sockaddr*)&addrSend,sizeof(addrSend))){
        error_msg="sendto error: "+std::to_string(errno);
        return false;
    }else{
        printSendData(p,length);
        return true;
    }
}

s16 axis_min_max(s16 min_value,s16 value,s16 max_value)
{
    if(value<min_value)value=min_value;
    else if(value>max_value)value=max_value;
    return value;
}

bool vconv_send()
{
    auto keysdown=keysDown();
    auto keysup=keysUp();
    auto keysheld=keysHeld();
    circlePosition padPos={0,0};
    circlePosition csPos={0,0};
    hidCircleRead(&padPos);
    hidCstickRead(&csPos);
    padPos.dx=axis_min_max(keycodes_3ds[33].value_range_start,padPos.dx,keycodes_3ds[33].value_range_end);
    padPos.dy=axis_min_max(keycodes_3ds[34].value_range_start,padPos.dy,keycodes_3ds[34].value_range_end);
    csPos.dx=axis_min_max(keycodes_3ds[35].value_range_start,csPos.dx,keycodes_3ds[35].value_range_end);
    csPos.dy=axis_min_max(keycodes_3ds[36].value_range_start,csPos.dy,keycodes_3ds[36].value_range_end);
    if(!PAD_X_TRIGGERED(keysheld)){
        padPos.dx=0;
    }
    if(!PAD_Y_TRIGGERED(keysheld)){
        padPos.dy=0;
    }
    if(!CSTICK_X_TRIGGERED(keysheld)){
        csPos.dx=0;
    }
    if(!CSTICK_Y_TRIGGERED(keysheld)){
        csPos.dy=0;
    }
    for(int i=0;i<32;i++){
        if((1<<i)&keysdown&USED_BUTTON_BITS){
            int keycode3DS=i+1;
            int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[keycode3DS]]];
            int value3DS=1;
            int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)*
                (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)/
                (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)+
                keycodes_xbox[keycodeXbox].value_range_start;
            if(keycodeXbox!=0){
                auto sendData=make_send_data(keycodeXbox,valueXbox);
                check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
                xbox_controller_key_status[keycodeXbox]=valueXbox;
            }
        }
        if((1<<i)&keysup&USED_BUTTON_BITS){
            int keycode3DS=i+1;
            int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[keycode3DS]]];
            int value3DS=0;
            int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)*
                (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)/
                (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)+
                keycodes_xbox[keycodeXbox].value_range_start;
            if(keycodeXbox!=0){
                auto sendData=make_send_data(keycodeXbox,valueXbox);
                check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
                xbox_controller_key_status[keycodeXbox]=valueXbox;
            }
        }
    }
    if(padPos.dx!=lastPadPos.dx){
        lastPadPos.dx=padPos.dx;
        int keycode3DS=33;
        int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[keycode3DS]]];
        int value3DS=padPos.dx;
        int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)*
            (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)/
            (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)+
            keycodes_xbox[keycodeXbox].value_range_start;
        if(keycodeXbox!=0){
            auto sendData=make_send_data(keycodeXbox,valueXbox);
            check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
            xbox_controller_key_status[keycodeXbox]=valueXbox;
        }
    }
    if(padPos.dy!=lastPadPos.dy){
        lastPadPos.dy=padPos.dy;
        int keycode3DS=34;
        int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[keycode3DS]]];
        int value3DS=padPos.dy;
        int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)*
            (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)/
            (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)+
            keycodes_xbox[keycodeXbox].value_range_start;
        if(keycodeXbox!=0){
            auto sendData=make_send_data(keycodeXbox,valueXbox);
            check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
            xbox_controller_key_status[keycodeXbox]=valueXbox;
        }
    }
    if(csPos.dx!=lastCsPos.dx){
        lastCsPos.dx=csPos.dx;
        int keycode3DS=35;
        int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[keycode3DS]]];
        int value3DS=csPos.dx;
        int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)*
            (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)/
            (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)+
            keycodes_xbox[keycodeXbox].value_range_start;
        if(keycodeXbox!=0){
            auto sendData=make_send_data(keycodeXbox,valueXbox);
            check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
            xbox_controller_key_status[keycodeXbox]=valueXbox;
        }
    }
    if(csPos.dy!=lastCsPos.dy){
        lastCsPos.dy=csPos.dy;
        int keycode3DS=36;
        int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[keycode3DS]]];
        int value3DS=csPos.dy;
        int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)*
            (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)/
            (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)+
            keycodes_xbox[keycodeXbox].value_range_start;
        if(keycodeXbox!=0){
            auto sendData=make_send_data(keycodeXbox,valueXbox);
            check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
            xbox_controller_key_status[keycodeXbox]=valueXbox;
        }
    }
    for(int i=0;i<4;i++){
        if(((onscreen_buttons_key_status>>i)&1)==1&&((last_onscreen_buttons_key_status>>i)&1)==0){
            int keycodeXbox=index_used_keycodes_xbox[button_mapping_onscreen_buttons_xbox[i]];
            int valueXbox=keycodes_xbox[keycodeXbox].value_range_end;
            auto sendData=make_send_data(keycodeXbox,valueXbox);
            check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
            xbox_controller_key_status[keycodeXbox]=valueXbox;
        }
        if(((onscreen_buttons_key_status>>i)&1)==0&&((last_onscreen_buttons_key_status>>i)&1)==1){
            int keycodeXbox=index_used_keycodes_xbox[button_mapping_onscreen_buttons_xbox[i]];
            int valueXbox=keycodes_xbox[keycodeXbox].value_range_start;
            auto sendData=make_send_data(keycodeXbox,valueXbox);
            check_send_to(&sendData,1+keycodes_xbox[keycodeXbox].value_length);
            xbox_controller_key_status[keycodeXbox]=valueXbox;
        }
    }
    last_onscreen_buttons_key_status=onscreen_buttons_key_status;
    return true;
}
