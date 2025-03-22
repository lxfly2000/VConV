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
#include <atlstr.h>

#include "resource.h"
#include "vconv.h"
#include "docwindow.h"
#include "downloaddriver.h"
#include "common.h"

#pragma comment(lib,"ComCtl32.lib")

// 全局变量:
HINSTANCE hInst;                                // 当前实例
CStringA szTitle;                  // 标题栏文本
CStringA szWindowClass;            // 主窗口类名
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
	LoadLibraryA("Riched20.dll");

    // 初始化全局字符串
    szTitle.LoadString(hInstance, IDS_APP_TITLE);
    szWindowClass.LoadString(hInstance, IDC_VCONVWIN);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAcceleratorsA(hInstance, MAKEINTRESOURCEA(IDC_VCONVWIN));

    MSG msg;

    // 主消息循环:
    while (GetMessageA(&msg, nullptr, 0, 0))
    {
        if (!TranslateAcceleratorA(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
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
    WNDCLASSEXA wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_VCONVWIN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEA(IDC_VCONVWIN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = wcex.hIcon;

    return RegisterClassExA(&wcex);
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
	CStringA key, szPort;
	key.Format("Port%d", n);
	szPort.Format("%d", port);
	return WritePrivateProfileStringA(INI_APPNAME, key, szPort, INI_FILE);
}

WORD ReadPortConfig(int n)
{
	CStringA key;
	key.Format("Port%d", n);
	return GetPrivateProfileIntA(INI_APPNAME, key, PORT_DEFAULT + n, INI_FILE);
}

void UISetControllerStatus(int n, int status)
{
	PostMessageA(hWndMain, VCONV_WM_SETCONTROLLERSTATUS, n, status);
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
	SendDlgItemMessageA(hDlg, idsIcon[n], STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadBitmapA(hInst, MAKEINTRESOURCEA(idsStat[status])));
	SendDlgItemMessageA(hDlg, idsCheck[n], BM_SETCHECK, statButton[status], 0);
	int idsStr[] = { IDS_STRING_CONTROLLER_DISCONNECT,IDS_STRING_CONTROLLER_LISTEN,IDS_STRING_CONTROLLER_DISCONNECT,IDS_STRING_CONTROLLER_CONNECT };
	CStringA buf;
	buf.Format(idsStr[status], n + 1, VConVGetListeningPort(n));
	SetWindowTextA(GetDlgItem(hWndMain, ID_STATUS_BAR), buf);
	SendDlgItemMessageA(hDlg, idsEdit[n], EM_SETREADONLY, status > 0, NULL);
	HWND hSpin = GetDlgItem(hDlg, idsSpin[n]);
	LONG_PTR style = GetWindowLongPtrA(hSpin, GWL_STYLE);
	if (status == 0)
		style = (~WS_DISABLED)&style;
	else
		style = WS_DISABLED | style;
	SetWindowLongPtrA(hSpin, GWL_STYLE, style);
}

void UIReportErrorController(int n, int error)
{
	PostMessageA(hWndMain, VCONV_WM_ERRORCONTROLLER, n, error);
}

void UIErrorBox(int error)
{
	PostMessageA(hWndMain, VCONV_WM_ERRORBOX, 0, error);
}

int WINAPI MBPrintfA(int iconType, LPCSTR title, LPCSTR fmt, ...)
{
	va_list va;
	CStringA text;
	va_start(va, fmt);
	text.FormatV(fmt, va);
	va_end(va);
	return MessageBoxA(hWndMain, text, title, iconType);
}

int OnUIErrorBox(int error)
{
	CStringA buf;
	switch (error)
	{
	case VCONV_ERROR_SOCKET_INIT_ERROR:
		buf.LoadString(hInst, IDS_STRING_SOCKET_INIT_ERROR);
		break;
	}
	return MBPrintfA(MB_ICONERROR, NULL, buf);
}

void DlgDisableController(int n)
{
	VConVDisconnectControllerAndClosePort(n);
	UISetControllerStatus(n, 0);
}

void OnUIReportErrorController(int n, int error)
{
	CStringA buf;
	switch (error)
	{
	case VCONV_ERROR_PORT:
		buf.LoadString(hInst, IDS_STRING_PORT_CONFLICT);
		break;
	case VCONV_ERROR_SOCKET_CREATE_ERROR:
		buf.LoadString(hInst, IDS_STRING_SOCKET_CREATE_ERROR);
		break;
	}
	MBPrintfA(MB_ICONERROR, NULL, buf, n + 1, VConVGetListeningPort(n));
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
		SendDlgItemMessageA(hWnd, IDC_SPIN_PORT1, UDM_SETRANGE32, 0, 65535);
		SendDlgItemMessageA(hWnd, IDC_SPIN_PORT2, UDM_SETRANGE32, 0, 65535);
		SendDlgItemMessageA(hWnd, IDC_SPIN_PORT3, UDM_SETRANGE32, 0, 65535);
		SendDlgItemMessageA(hWnd, IDC_SPIN_PORT4, UDM_SETRANGE32, 0, 65535);
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
			if (SendDlgItemMessageA(hWnd, IDC_CHECK_ENABLE1, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(0);
			else
				DlgEnableController(0, GetDlgItemInt(hWnd, IDC_EDIT_PORT1, NULL, FALSE));
			break;
		case IDC_CHECK_ENABLE2:
			if (SendDlgItemMessageA(hWnd, IDC_CHECK_ENABLE2, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(1);
			else
				DlgEnableController(1, GetDlgItemInt(hWnd, IDC_EDIT_PORT2, NULL, FALSE));
			break;
		case IDC_CHECK_ENABLE3:
			if (SendDlgItemMessageA(hWnd, IDC_CHECK_ENABLE3, BM_GETCHECK, 0, 0) == BST_CHECKED)
				DlgDisableController(2);
			else
				DlgEnableController(2, GetDlgItemInt(hWnd, IDC_EDIT_PORT3, NULL, FALSE));
			break;
		case IDC_CHECK_ENABLE4:
			if (SendDlgItemMessageA(hWnd, IDC_CHECK_ENABLE4, BM_GETCHECK, 0, 0) == BST_CHECKED)
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

   hWndMain = CreateWindowA(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, 0, PhyPixelsX(440), PhyPixelsY(400), nullptr, nullptr, hInstance, nullptr);

   if (!hWndMain)
   {
      return FALSE;
   }

   HWND hStatusBar = CreateWindowExA(
	   WS_EX_TOPMOST,                       // no extended styles
	   STATUSCLASSNAMEA,         // name of status bar class
	   "",           // no text when first created
	   SBARS_SIZEGRIP |         // includes a sizing grip
	   WS_CHILD | WS_VISIBLE,   // creates a visible child window
	   0, 0, 0, 0,              // ignores size and position
	   hWndMain,              // handle to parent window
	   (HMENU)ID_STATUS_BAR,       // child window identifier
	   hInst,                   // handle to application instance
	   NULL);

   HWND hDlg = CreateDialogA(hInst, MAKEINTRESOURCEA(IDD_CONTROLLER_STAT), hWndMain, ControllerDlgProc);
   SetWindowLongPtrA(hDlg, GWLP_ID, ID_CONTROLLER_DLG);
   ShowWindow(hDlg, nCmdShow);

   ShowWindow(hWndMain, nCmdShow);
   UpdateWindow(hWndMain);

   return TRUE;
}

void Init(bool downloadDriver)
{
	int r = VConVInit();
	CStringA buf;
	if (r)
	{
		buf.Format(IDS_STRING_INIT_FAIL, r);
		SetWindowTextA(GetDlgItem(hWndMain, ID_STATUS_BAR), buf);
		if (r == 0xE0000001 && downloadDriver)
		{
			buf.LoadString(hInst, IDS_STRING_ALERT_DRIVER);
			if (MBPrintfA(MB_ICONEXCLAMATION | MB_YESNO, szTitle, buf) == IDYES && DownloadDriver(hWndMain) == 0)
				Init(false);
		}
		return;
	}
	buf.LoadString(hInst, IDS_STRING_INIT_OK);
	SetWindowTextA(GetDlgItem(hWndMain, ID_STATUS_BAR), buf);
	//获取本机IP
	char iplist[256];
	addrinfo*pAinfo = nullptr;
	gethostname(iplist, ARRAYSIZE(iplist));
	addrinfo hints = { NULL };
	hints.ai_family = AF_INET; /* Allow IPv4 */
	hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
	hints.ai_protocol = 0; /* Any protocol */
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(iplist, NULL, &hints, &pAinfo);
	iplist[0] = '\0';
	for (addrinfo *p = pAinfo; p != nullptr; p = p->ai_next)
	{
		char szIp[16];
		sockaddr_in*addr = (sockaddr_in*)p->ai_addr;
		inet_ntop(AF_INET, &addr->sin_addr, szIp, ARRAYSIZE(szIp));
		strcat_s(iplist, szIp);
		strcat_s(iplist, "\r\n");
	}
	freeaddrinfo(pAinfo);
	SetDlgItemTextA(GetDlgItem(hWndMain, ID_CONTROLLER_DLG), IDC_EDIT_IPLIST, iplist);
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
                DialogBoxA(hInst, MAKEINTRESOURCEA(IDD_ABOUTBOX), hWnd, About);
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
                return DefWindowProcA(hWnd, message, wParam, lParam);
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
		Init(true);
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
        return DefWindowProcA(hWnd, message, wParam, lParam);
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

	case WM_SHOWWINDOW:
	{
		CStringA buf;
		buf.LoadString(IDS_STRING_ABOUT);
		SetRichEditTextA(GetDlgItem(hDlg, IDC_RICHEDIT2_ABOUT), buf);
		break;
	}

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return OnRichEditClickMsg(hDlg, message, wParam, lParam);
}
