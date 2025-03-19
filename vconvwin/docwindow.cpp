#include <Windows.h>
#include <Richedit.h>

#include "docwindow.h"
#include "common.h"

#define ID_CONTROL_DOC 2000

char *pDoc;
int docLength;
int docPos;

DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie,
	LPBYTE lpBuff,
	LONG cb,
	PLONG pcb)
{
	char*p=(char*)dwCookie;

	int ncpy = min(docLength - docPos, cb);
	if (ncpy <= 0)
		return -1;
	memcpy(lpBuff, p + docPos, ncpy);

	*pcb = ncpy;
	docPos += ncpy;

	return 0;
}

void CreateDocWindow(HWND hwnd)
{
	LoadLibrary(TEXT("Msftedit.dll"));
	HWND hDoc = CreateWindow(MSFTEDIT_CLASS, TEXT("Document"), ES_MULTILINE | ES_READONLY | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
		0, 0, CW_USEDEFAULT, 0, hwnd, (HMENU)ID_CONTROL_DOC, NULL, NULL);
	SetFocus(hDoc);

	EDITSTREAM es = { 0 };
	es.pfnCallback = EditStreamCallback;
	es.dwCookie = (DWORD_PTR)pDoc;
	docPos = 0;
	SendMessage(hDoc, EM_STREAMIN, SF_RTF, (LPARAM)&es);
}

void DestroyDocWindow(HWND hwnd)
{
	DestroyWindow(GetDlgItem(hwnd, ID_CONTROL_DOC));
}

void MoveDocWindow(HWND hwnd)
{
	RECT r;
	GetClientRect(hwnd, &r);
	MoveWindow(GetDlgItem(hwnd, ID_CONTROL_DOC), r.left, r.top, r.right - r.left, r.bottom - r.top, FALSE);
}

LRESULT CALLBACK DocWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		CreateDocWindow(hwnd);
		break;
	case WM_DESTROY:
		DestroyDocWindow(hwnd);
		break;
	case WM_SIZE:
		MoveDocWindow(hwnd);
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void DocWindowA(HWND hWndParent, char * doc, int length)
{
	pDoc = doc;
	docLength = length;
	WNDCLASSA wc{};
	wc.lpszClassName = "DocWindow";
	wc.lpfnWndProc = DocWindowProc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClassA(&wc);
	char title[32];
	GetWindowTextA(hWndParent, title, ARRAYSIZE(title));
	HWND hwnd = CreateWindowA(wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, PhyPixelsX(800), PhyPixelsY(600), NULL, NULL, wc.hInstance, NULL);
	ShowWindow(hwnd, SW_SHOW);
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
