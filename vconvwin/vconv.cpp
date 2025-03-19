#include <iostream>
#include <thread>
#include <future>
#include <atomic>
#include <Windows.h>

#include <ViGEm/Client.h>

#include "vconv.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "ws2_32.lib")

//用于接收手柄震动消息
VOID CALLBACK notificationCallback(
	PVIGEM_CLIENT Client,
	PVIGEM_TARGET Target,
	UCHAR LargeMotor,
	UCHAR SmallMotor,
	UCHAR LedNumber,
	LPVOID UserData
)
{
	//TODO
	static int count = 1;

	std::cout.width(3);
	std::cout << count++ << " ";
	std::cout.width(3);
	std::cout << (int)LargeMotor << " ";
	std::cout.width(3);
	std::cout << (int)SmallMotor << std::endl;
}

PVIGEM_CLIENT client = NULL;
PVIGEM_TARGET targets[VCONV_MAX_CONTROLLER] = { NULL };
XINPUT_STATE states[VCONV_MAX_CONTROLLER] = { NULL };
WORD ports[VCONV_MAX_CONTROLLER] = { 0 };
std::thread tds[VCONV_MAX_CONTROLLER];
std::promise<int>promiseReturns[VCONV_MAX_CONTROLLER];
std::atomic_int tRunnings[VCONV_MAX_CONTROLLER] = { 0 };
SOCKET controllerSocketsRecv[VCONV_MAX_CONTROLLER] = { NULL };
SOCKET controllerSocketsSend[VCONV_MAX_CONTROLLER] = { NULL };//TODO:向发送端发送震动消息

void OnReceiveData(int n, char*buf, int length)
{
	if (buf[0] == 0 || buf[0] > 22)
		return;
	else if (buf[0] <= 16)
		states[n].Gamepad.wButtons = (buf[1] ? ((~(1 << (buf[0] - 1)))&states[n].Gamepad.wButtons) : ((1 << (buf[0] - 1)) | states[n].Gamepad.wButtons));
	else if (buf[0] == 17)
		states[n].Gamepad.bLeftTrigger = buf[1];
	else if (buf[0] == 18)
		states[n].Gamepad.bRightTrigger = buf[1];
	else if (buf[0] == 19)
		states[n].Gamepad.sThumbLX = MAKEWORD(buf[1], buf[2]);
	else if (buf[0] == 20)
		states[n].Gamepad.sThumbLY = MAKEWORD(buf[1], buf[2]);
	else if (buf[0] == 21)
		states[n].Gamepad.sThumbRX = MAKEWORD(buf[1], buf[2]);
	else if (buf[0] == 22)
		states[n].Gamepad.sThumbRY = MAKEWORD(buf[1], buf[2]);
	vigem_target_x360_update(client, targets[n], *reinterpret_cast<XUSB_REPORT*>(&states[n].Gamepad));
}

void ControllerThread(int n)
{
	controllerSocketsRecv[n] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (controllerSocketsRecv[n] == INVALID_SOCKET)
	{
		controllerSocketsRecv[n] = NULL;
		UIReportErrorController(n, VCONV_ERROR_SOCKET_CREATE_ERROR);
		promiseReturns[n].set_value(-2);
		return;
	}
	sockaddr_in serverAddr, senderAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(ports[n]);
	int senderAddrLen = sizeof(senderAddr);

	if (bind(controllerSocketsRecv[n], (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		controllerSocketsRecv[n] = NULL;
		UIReportErrorController(n, VCONV_ERROR_PORT);
		promiseReturns[n].set_value(-3);
		return;
	}

	char buffer[512];
	while (tRunnings[n])
	{
		int bytesReceived = recvfrom(controllerSocketsRecv[n], buffer, sizeof(buffer) - 1, NULL, (sockaddr*)&senderAddr, &senderAddrLen);
		if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
			tRunnings[n] = 0;
		OnReceiveData(n, buffer, bytesReceived);
	}
	promiseReturns[n].set_value(0);
}

int VConVInit()
{
	client = vigem_alloc();
	if (client == nullptr)
		return -1;
	VIGEM_ERROR ret = vigem_connect(client);
	if (!VIGEM_SUCCESS(ret))
		return ret;

	//Socket Init
	WSADATA wsaData;
	int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (r)
		UIErrorBox(VCONV_ERROR_SOCKET_INIT_ERROR);
	return r;
}

int VConVRelease()
{
	WSACleanup();
	for (int i = 0; i < VCONV_MAX_CONTROLLER; i++)
		VConVClosePort(i);
	vigem_disconnect(client);
	vigem_free(client);
	return 0;
}

int VConVConnectControllerAndListenPort(int n, WORD port)
{
	int r = VConVConnectController(n);
	if (r)
		return r;

	return VConVListenPort(n, port);
}

int VConVDisconnectControllerAndClosePort(int n)
{
	int r = VConVDisconnectController(n);
	if (r)
		return r;

	return VConVClosePort(n);
}

WORD VConVGetListeningPort(int n)
{
	return ports[n];
}

BOOL VConVGetControllerIsConnected(int n)
{
	return targets[n] != NULL;
}

int VConVConnectController(int n)
{
	targets[n] = vigem_target_x360_alloc();
	VIGEM_ERROR err = vigem_target_add(client, targets[n]);
	if (!VIGEM_SUCCESS(err))
	{
		vigem_target_remove(client, targets[n]);
		vigem_target_free(targets[n]);
		targets[n] = NULL;
		return err;
	}
	//可选功能，失败不报错
	vigem_target_x360_register_notification(client, targets[n], notificationCallback, NULL);
	return 0;
}

int VConVDisconnectController(int n)
{
	if (targets[n])
		vigem_target_x360_unregister_notification(targets[n]);
	VIGEM_ERROR err = vigem_target_remove(client, targets[n]);
	if (!VIGEM_SUCCESS(err))
		return err;
	return 0;
}

int VConVListenPort(int n, WORD port)
{
	ports[n] = port;
	tRunnings[n] = 1;
	promiseReturns[n] = std::promise<int>();
	tds[n] = std::thread(ControllerThread, n);
	return 0;
}

int VConVClosePort(int n)
{
	tRunnings[n] = 0;
	if (tds[n].joinable())
	{
		closesocket(controllerSocketsRecv[n]);
		controllerSocketsRecv[n] = NULL;
		tds[n].join();
		std::future<int>f = promiseReturns[n].get_future();
		int r = f.get();
		if (r)
			return r;//发生错误
	}
	return 0;
}
