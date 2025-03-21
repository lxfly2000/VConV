#include "vconv.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>

#include <3ds.h>

#include "utilities.h"
#include "keycodes.h"

#define SETTINGS_FILE "vconv.txt"

int button_mapping_3ds_xbox[18]={0};
std::string serverIp=SERVER_IP_DEFAULT;
unsigned short serverPort=32000;
std::string error_msg="";
sockaddr_in addrSend={0};

int socketfd;

bool vconv_init()
{
    if(!read_settings()){
        return false;
    }
    socketfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(socketfd==-1){
        error_msg="socket() error.";
        return false;
    }
    
    error_msg="Init OK.";
    return true;
}

bool vconv_release()
{
    if(closesocket(socketfd)){
        return false;
    }
    return save_settings();
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
    return true;
}

bool controller_enabled=false;
circlePosition padPos={0,0};
circlePosition csPos={0,0};

void vconv_send()
{
    auto keysdown=keysDown();
    auto keysup=keysUp();
    auto keysheld=keysHeld();
    circlePosition padPos={0,0};
    circlePosition csPos={0,0};
    for(int i=0;i<32;i++){
        if((1<<i)&keysdown&USED_BUTTON_BITS){
            int keycode3DS=i+1;
            int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[i+1]]];
            int value3DS=1;
            int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)/
                (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)*
                (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)+
                keycodes_xbox[keycodeXbox].value_range_start;
            unsigned char senddata[4]={keycodeXbox,0};
            memcpy(senddata+1,&valueXbox,keycodes_xbox[keycodeXbox].value_length);
        }
        if((1<<i)&keysup&USED_BUTTON_BITS){
            int keycode3DS=i+1;
            int keycodeXbox=index_used_keycodes_xbox[button_mapping_3ds_xbox[keycodes_3ds_to_index_used[i+1]]];
            int value3DS=0;
            int valueXbox=(value3DS-keycodes_3ds[keycode3DS].value_range_start)/
                (keycodes_3ds[keycode3DS].value_range_end-keycodes_3ds[keycode3DS].value_range_start)*
                (keycodes_xbox[keycodeXbox].value_range_end-keycodes_xbox[keycodeXbox].value_range_start)+
                keycodes_xbox[keycodeXbox].value_range_start;
            unsigned char senddata[4]={keycodeXbox,0};
            memcpy(senddata+1,&valueXbox,keycodes_xbox[keycodeXbox].value_length);
        }
    }
}
