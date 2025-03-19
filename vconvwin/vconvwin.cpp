// VConV - Virtual Controller for ViGEm Windows

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <WS2tcpip.h>

#include "resource.h"
#include "vconv.h"
#include "docwindow.h"
#include "common.h"

#pragma comment(lib,"ComCtl32.lib")

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND hWndMain;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VCONVWIN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VCONVWIN));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	for (int i = 0; i < VCONV_MAX_CONTROLLER; i++)
		VConVDisconnectControllerAndClosePort(i);
	VConVRelease();

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VCONVWIN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VCONVWIN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = wcex.hIcon;

    return RegisterClassExW(&wcex);
}

#define ID_STATUS_BAR 2000
#define ID_CONTROLLER_DLG 2001
#define PORT_DEFAULT 32000
#define INI_FILE ".\\config.ini"
#define INI_APPNAME "vconvwin"
#define VCONV_WM_SETCONTROLLERSTATUS WM_USER+2000
#define VCONV_WM_ERRORCONTROLLER WM_USER+2001
#define VCONV_WM_ERRORBOX WM_USER+2002

BOOL SavePortConfig(int n, int port)
{
	char key[8], szPort[8];
	wsprintfA(key, "Port%d", n);
	wsprintfA(szPort, "%d", port);
	return WritePrivateProfileStringA(INI_APPNAME, key, szPort, INI_FILE);
}

WORD ReadPortConfig(int n)
{
	char key[8];
	wsprintfA(key, "Port%d", n);
	return GetPrivateProfileIntA(INI_APPNAME, key, PORT_DEFAULT + n, INI_FILE);
}

void UISetControllerStatus(int n, int status)
{
	PostMessage(hWndMain, VCONV_WM_SETCONTROLLERSTATUS, n, status);
}

void OnUISetControllerStatus(int n, int status)
{
	int idsIcon[] = { IDC_STATIC_STATUS1,IDC_STATIC_STATUS2, IDC_STATIC_STATUS3, IDC_STATIC_STATUS4 };
	int idsCheck[] = { IDC_CHECK_ENABLE1,IDC_CHECK_ENABLE2, IDC_CHECK_ENABLE3, IDC_CHECK_ENABLE4 };
	int idsStat[] = { IDB_BITMAP_OFFLINE,IDB_BITMAP_LISTEN,IDB_BITMAP_LISTEN,IDB_BITMAP_ONLINE };
	int statButton[] = { BST_UNCHECKED,BST_CHECKED,BST_CHECKED,BST_CHECKED };
	int idsEdit[] = { IDC_EDIT_PORT1,IDC_EDIT_PORT2,IDC_EDIT_PORT3,IDC_EDIT_PORT4 };
	int idsSpin[] = { IDC_SPIN_PORT1,IDC_SPIN_PORT2,IDC_SPIN_PORT3,IDC_SPIN_PORT4 };
	HWND hDlg = GetDlgItem(hWndMain, ID_CONTROLLER_DLG);
	SendDlgItemMessage(hDlg, idsIcon[n], STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadBitmap(hInst, MAKEINTRESOURCE(idsStat[status])));
	SendDlgItemMessage(hDlg, idsCheck[n], BM_SETCHECK, statButton[status], 0);
	int idsStr[] = { IDS_STRING_CONTROLLER_DISCONNECT,IDS_STRING_CONTROLLER_LISTEN,IDS_STRING_CONTROLLER_DISCONNECT,IDS_STRING_CONTROLLER_CONNECT };
	TCHAR buf[128], pbuf[128];
	LoadString(hInst, idsStr[status], pbuf, ARRAYSIZE(pbuf));
	wsprintf(buf, pbuf, n + 1, VConVGetListeningPort(n));
	SetWindowText(GetDlgItem(hWndMain, ID_STATUS_BAR), buf);
	SendDlgItemMessage(hDlg, idsEdit[n], EM_SETREADONLY, status > 0, NULL);
	HWND hSpin = GetDlgItem(hDlg, idsSpin[n]);
	LONG_PTR style = GetWindowLongPtr(hSpin, GWL_STYLE);
	if (status == 0)
		style = (~WS_DISABLED)&style;
	else
		style = WS_DISABLED | style;
	SetWindowLongPtr(hSpin, GWL_STYLE, style);
}

void UIReportErrorController(int n, int error)
{
	PostMessage(hWndMain, VCONV_WM_ERRORCONTROLLER, n, error);
}

void UIErrorBox(int error)
{
	PostMessage(hWndMain, VCONV_WM_ERRORBOX, 0, error);
}

int WINAPI MBPrintfW(int iconType, LPCWSTR title, LPCWSTR fmt, ...)
{
	va_list va;
	WCHAR text[4096];
	va_start(va, fmt);
	wvsprintfW(text, fmt, va);
	va_end(va);
	return MessageBoxW(hWndMain, text, title, iconType);
}

int OnUIErrorBox(int error)
{
	TCHAR buf[32];
	switch (error)
	{
	case VCONV_ERROR_SOCKET_INIT_ERROR:
		LoadString(hInst, IDS_STRING_SOCKET_INIT_ERROR, buf, ARRAYSIZE(buf));
		break;
	}
	return MBPrintfW(MB_ICONERROR, NULL, buf);
}

void DlgDisableController(int n)
{
	VConVDisconnectControllerAndClosePort(n);
	UISetControllerStatus(n, 0);
}

void OnUIReportErrorController(int n, int error)
{
	TCHAR buf[32];
	switch (error)
	{
	case VCONV_ERROR_PORT:
		LoadString(hInst, IDS_STRING_PORT_CONFLICT, buf, ARRAYSIZE(buf));
		break;
	case VCONV_ERROR_SOCKET_CREATE_ERROR:
		LoadString(hInst, IDS_STRING_SOCKET_CREATE_ERROR, buf, ARRAYSIZE(buf));
		break;
	}
	MBPrintfW(MB_ICONERROR, NULL, buf, n, VConVGetListeningPort(n));
	DlgDisableController(n);
}

void DlgEnableController(int n, int port)
{
	if (VConVConnectControllerAndListenPort(n, port))
	{
		UIReportErrorController(n, VCONV_ERROR_PORT);
		return;
	}
	UISetControllerStatus(n, 3);
}

INT_PTR CALLBACK ControllerDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		//设置编辑框范围
		SendDlgItemMessage(hWnd, IDC_SPIN_PORT1, UDM_SETRANGE32, 0, 65535);
		SendDlgItemMessage(hWnd, IDC_SPIN_PORT2, UDM_SETRANGE32, 0, 65535);
		SendDlgItemMessage(hWnd, IDC_SPIN_PORT3, UDM_SETRANGE32, 0, 65535);
		SendDlgItemMessage(hWnd, IDC_SPIN_PORT4, UDM_SETRANGE32, 0, 65535);
		//读取存储值
		SetDlgItemInt(hWnd, IDC_EDIT_PORT1, ReadPortConfig(0), FALSE);
		SetDlgItemInt(hWnd, IDC_EDIT_PORT2, ReadPortConfig(1), FALSE);
		SetDlgItemInt(hWnd, IDC_EDIT_PORT3, ReadPortConfig(2), FALSE);
		SetDlgItemInt(hWnd, IDC_EDIT_PORT4, ReadPortConfig(3), FALSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK_ENABLE1:
			if (SendDlgItemMessage(hWnd, IDC_CHECK_ENABLE1, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(0);
			else
				DlgEnableController(0, GetDlgItemInt(hWnd, IDC_EDIT_PORT1, NULL, FALSE));
			break;
		case IDC_CHECK_ENABLE2:
			if (SendDlgItemMessage(hWnd, IDC_CHECK_ENABLE2, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(1);
			else
				DlgEnableController(1, GetDlgItemInt(hWnd, IDC_EDIT_PORT2, NULL, FALSE));
			break;
		case IDC_CHECK_ENABLE3:
			if (SendDlgItemMessage(hWnd, IDC_CHECK_ENABLE3, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(2);
			else
				DlgEnableController(2, GetDlgItemInt(hWnd, IDC_EDIT_PORT3, NULL, FALSE));
			break;
		case IDC_CHECK_ENABLE4:
			if (SendDlgItemMessage(hWnd, IDC_CHECK_ENABLE4, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(3);
			else
				DlgEnableController(3, GetDlgItemInt(hWnd, IDC_EDIT_PORT4, NULL, FALSE));
			break;
		case IDC_EDIT_PORT1:
			if (HIWORD(wParam) == EN_CHANGE && IsWindowVisible(hWnd))
				SavePortConfig(0, GetDlgItemInt(hWnd, IDC_EDIT_PORT1, NULL, FALSE));
			break;
		case IDC_EDIT_PORT2:
			if (HIWORD(wParam) == EN_CHANGE && IsWindowVisible(hWnd))
				SavePortConfig(1, GetDlgItemInt(hWnd, IDC_EDIT_PORT2, NULL, FALSE));
			break;
		case IDC_EDIT_PORT3:
			if (HIWORD(wParam) == EN_CHANGE && IsWindowVisible(hWnd))
				SavePortConfig(2, GetDlgItemInt(hWnd, IDC_EDIT_PORT3, NULL, FALSE));
			break;
		case IDC_EDIT_PORT4:
			if (HIWORD(wParam) == EN_CHANGE && IsWindowVisible(hWnd))
				SavePortConfig(3, GetDlgItemInt(hWnd, IDC_EDIT_PORT4, NULL, FALSE));
			break;
		}
		break;
	}
	return 0;
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   InitCommonControls();

   hWndMain = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, 0, PhyPixelsX(400), PhyPixelsY(360), nullptr, nullptr, hInstance, nullptr);

   if (!hWndMain)
   {
      return FALSE;
   }

   HWND hStatusBar = CreateWindowEx(
	   WS_EX_TOPMOST,                       // no extended styles
	   STATUSCLASSNAME,         // name of status bar class
	   (PCTSTR)NULL,           // no text when first created
	   SBARS_SIZEGRIP |         // includes a sizing grip
	   WS_CHILD | WS_VISIBLE,   // creates a visible child window
	   0, 0, 0, 0,              // ignores size and position
	   hWndMain,              // handle to parent window
	   (HMENU)ID_STATUS_BAR,       // child window identifier
	   hInst,                   // handle to application instance
	   NULL);

   HWND hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CONTROLLER_STAT), hWndMain, ControllerDlgProc);
   SetWindowLongPtr(hDlg, GWLP_ID, ID_CONTROLLER_DLG);
   ShowWindow(hDlg, nCmdShow);

   ShowWindow(hWndMain, nCmdShow);
   UpdateWindow(hWndMain);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case ID_MENU_JOY:
				ShellExecuteA(hWnd, "open", "rundll32", "shell32,Control_RunDLL joy.cpl", NULL, SW_SHOW);
				break;
			case ID_MENU_INSTRUCTIONS:
			{
				HRSRC hRsrc = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_RTF_INSTRUCTIONS), "RTF");
				DocWindowA(hWnd, (char*)LockResource(LoadResource(NULL, hRsrc)), SizeofResource(NULL, hRsrc));
			}
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_SIZE:
	{
		HWND hSub = GetDlgItem(hWnd, ID_STATUS_BAR);
		RECT rs;
		GetClientRect(hSub, &rs);
		MoveWindow(hSub, 0, 0, 0, 0, FALSE);//状态栏的矩形是系统自己管理的
		hSub = GetDlgItem(hWnd, ID_CONTROLLER_DLG);
		RECT r;
		GetClientRect(hWnd, &r);
		MoveWindow(hSub, r.left, r.top, r.right - r.left, r.bottom - r.top - rs.bottom + rs.top, TRUE);
	}
		break;
	case WM_SHOWWINDOW:
	{
		int r = VConVInit();
		TCHAR buf[32];
		if (r == 0)
		{
			LoadString(hInst, IDS_STRING_INIT_OK, buf, ARRAYSIZE(buf));
		}
		else
		{
			TCHAR pbuf[32];
			LoadString(hInst, IDS_STRING_INIT_FAIL, pbuf, ARRAYSIZE(pbuf));
			wsprintf(buf, pbuf, r);
		}
		SetWindowText(GetDlgItem(hWnd, ID_STATUS_BAR), buf);
		//获取本机IP
		char iplist[512];
		addrinfo*pAinfo = nullptr;
		gethostname(iplist, ARRAYSIZE(iplist));
		addrinfo hints = { NULL };
		hints.ai_family = AF_INET; /* Allow IPv4 */
		hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
		hints.ai_protocol = 0; /* Any protocol */
		hints.ai_socktype = SOCK_STREAM;
		getaddrinfo(iplist, NULL, &hints, &pAinfo);
		iplist[0] = '\0';
		for (addrinfo *p=pAinfo;p!=nullptr;p=p->ai_next)
		{
			char szIp[16];
			sockaddr_in*addr = (sockaddr_in*)p->ai_addr;
			inet_ntop(AF_INET, &addr->sin_addr, szIp, ARRAYSIZE(szIp));
			strcat_s(iplist, szIp);
			strcat_s(iplist, "\r\n");
		}
		freeaddrinfo(pAinfo);
		SetDlgItemTextA(GetDlgItem(hWnd, ID_CONTROLLER_DLG), IDC_EDIT_IPLIST, iplist);
	}
		break;
	case VCONV_WM_SETCONTROLLERSTATUS:
		OnUISetControllerStatus(wParam, lParam);
		break;
	case VCONV_WM_ERRORCONTROLLER:
		OnUIReportErrorController(wParam, lParam);
		break;
	case VCONV_WM_ERRORBOX:
		OnUIErrorBox(lParam);
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
