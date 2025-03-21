#include "vconv.h"

#include <stdio.h>
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
    if(USED_BUTTON_BITS(keysdown)){
        //TODO
    }
}