#pragma once
//XINPUT���֧��4���ֱ�
#define VCONV_MAX_CONTROLLER 4
#include <Windows.h>
int VConVInit();
int VConVRelease();
BOOL VConVGetControllerIsConnected(int n);
int VConVConnectController(int n);
int VConVDisconnectController(int n);
//0��ʾû�м��������򷵻ض˿ں�
WORD VConVGetListeningPort(int n);
//�����˿�
int VConVListenPort(int n, WORD port);
int VConVClosePort(int n);
int VConVConnectControllerAndListenPort(int n, WORD port);
int VConVDisconnectControllerAndClosePort(int n);

//Status:0=δ���� 1=������ 3=������
void UISetControllerStatus(int n, int status);
