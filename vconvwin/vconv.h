#pragma once
#include <Windows.h>
#include <Xinput.h>

//XINPUT最多支持4个手柄
#define VCONV_MAX_CONTROLLER XUSER_MAX_COUNT

int VConVInit();
int VConVRelease();
BOOL VConVGetControllerIsConnected(int n);
int VConVConnectController(int n);
int VConVDisconnectController(int n);
//0表示没有监听，否则返回端口号
WORD VConVGetListeningPort(int n);
//监听端口
int VConVListenPort(int n, WORD port);
int VConVClosePort(int n);
int VConVConnectControllerAndListenPort(int n, WORD port);
int VConVDisconnectControllerAndClosePort(int n);

//Status:0=未启用 1=监听中 3=已连接
void UISetControllerStatus(int n, int status);
//报错并停止控制器
void UIReportErrorController(int n, int error);

#define VCONV_ERROR_PORT 1
