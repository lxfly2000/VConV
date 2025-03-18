// VConV - Virtual Controller for ViGEm Windows

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <CommCtrl.h>

#include "resource.h"

#pragma comment(lib,"ComCtl32.lib")

#define MAX_LOADSTRING 100

#define PhyPixelsX(logicPixelsX) MulDiv(logicPixelsX,GetDeviceCaps(GetDC(0),LOGPIXELSX),USER_DEFAULT_SCREEN_DPI)
#define PhyPixelsY(logicPixelsY) MulDiv(logicPixelsY,GetDeviceCaps(GetDC(0),LOGPIXELSY),USER_DEFAULT_SCREEN_DPI)

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

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

    // TODO: 在此处放置代码。

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

INT_PTR CALLBACK ControllerDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		break;
	case WM_SHOWWINDOW:
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, 0, PhyPixelsX(400), PhyPixelsY(300), nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
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
	   hWnd,              // handle to parent window
	   (HMENU)ID_STATUS_BAR,       // child window identifier
	   hInst,                   // handle to application instance
	   NULL);

   HWND hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CONTROLLER_STAT), hWnd, ControllerDlgProc);
   SetWindowLongPtr(hDlg, GWLP_ID, ID_CONTROLLER_DLG);
   ShowWindow(hDlg, nCmdShow);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

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
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
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
		TCHAR buf[30];
		wsprintf(buf, TEXT("%dx%d"), LOWORD(lParam), HIWORD(lParam));
		SetWindowText(hSub, buf);
		hSub = GetDlgItem(hWnd, ID_CONTROLLER_DLG);
		RECT r;
		GetClientRect(hWnd, &r);
		MoveWindow(hSub, r.left, r.top, r.right - r.left, r.bottom - r.top - rs.bottom + rs.top, FALSE);
	}
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
