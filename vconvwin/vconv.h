#pragma once
//XINPUT最多支持4个手柄
#define VCONV_MAX_CONTROLLER 4
#include <Windows.h>
BOOL VConVInit();
BOOL VConVRelease();
BOOL VConVGetControllerIsConnected(int n);
BOOL VConVConnectController(int n);
BOOL VConVDisconnectController(int n);
//0表示没有监听，否则返回端口号
WORD VConVGetListeningPort(int n);
//监听端口
BOOL VConVListenPort(int n, WORD port);
BOOL VConVClosePort(int n);

//Status:0=未启用 1=监听中 3=已连接
void UISetControllerStatus(int n, int status);
