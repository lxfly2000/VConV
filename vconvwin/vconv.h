#pragma once
//XINPUT���֧��4���ֱ�
#define VCONV_MAX_CONTROLLER 4
#include <Windows.h>
BOOL VConVInit();
BOOL VConVRelease();
BOOL VConVGetControllerIsConnected(int n);
BOOL VConVConnectController(int n);
BOOL VConVDisconnectController(int n);
//0��ʾû�м��������򷵻ض˿ں�
WORD VConVGetListeningPort(int n);
//�����˿�
BOOL VConVListenPort(int n, WORD port);
BOOL VConVClosePort(int n);

//Status:0=δ���� 1=������ 3=������
void UISetControllerStatus(int n, int status);
