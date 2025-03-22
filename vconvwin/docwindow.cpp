#include <Windows.h>
#include <Richedit.h>
#include <atlstr.h>
#include <regex>

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
	HWND hDoc = CreateWindowA(RICHEDIT_CLASSA, "Document", ES_MULTILINE | ES_READONLY | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
		0, 0, CW_USEDEFAULT, 0, hwnd, (HMENU)ID_CONTROL_DOC, NULL, NULL);
	SetFocus(hDoc);

	EDITSTREAM es = { 0 };
	es.pfnCallback = EditStreamCallback;
	es.dwCookie = (DWORD_PTR)pDoc;
	docPos = 0;
	SendMessageA(hDoc, EM_STREAMIN, SF_RTF, (LPARAM)&es);
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
	CStringA title;
	title.Preallocate(GetWindowTextLengthA(hWndParent));
	GetWindowTextA(hWndParent, title.GetBuffer(), title.GetAllocLength());
	HWND hwnd = CreateWindowA(wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, PhyPixelsX(800), PhyPixelsY(600), NULL, NULL, wc.hInstance, NULL);
	ShowWindow(hwnd, SW_SHOW);
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
	MSG msg;
	while (GetMessageA(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}



std::string to_unix_str(std::string str)
{
	for (size_t i = 0; i < str.size();)
	{
		if (str[i] == '\r')
			str.erase(str.begin() + i);
		else
			i++;
	}
	return str;
}

void SetRichEditTextA(HWND h, LPCSTR text)
{
	std::string str = to_unix_str(text);
	std::regex r("https?://[^\\n ]+");
	std::smatch m;
	SetWindowTextA(h, text);
	SendMessageA(h, EM_SETEVENTMASK, NULL, ENM_LINK);
	size_t prelen = 0;
	CHARRANGE cr;
	while (std::regex_search(str, m, r, std::regex_constants::match_any))
	{
		cr.cpMin = prelen + m.position(0);
		cr.cpMax = prelen + m.position(0) + m.length(0);
		SendMessageA(h, EM_EXSETSEL, NULL, (LPARAM)&cr);
		CHARFORMATA cf;
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_LINK;
		cf.dwEffects = CFE_LINK;
		cf.crTextColor = RGB(0, 0, 255);
		SendMessageA(h, EM_SETCHARFORMAT, SCF_SELECTION, (WPARAM)&cf);
		str = str.substr(m.position(0) + m.length(0));
		prelen += m.position(0) + m.length(0);
	}
	cr.cpMin = cr.cpMax = 0;
	SendMessageA(h, EM_EXSETSEL, NULL, (LPARAM)&cr);
}

INT_PTR CALLBACK OnRichEditClickMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NOTIFY && ((LPNMHDR)lParam)->code == EN_LINK)
	{
		ENLINK *el = (ENLINK*)lParam;
		if (el->msg == WM_LBUTTONUP)
		{
			HWND hRE = GetDlgItem(hwnd, ((LPNMHDR)lParam)->idFrom);
			CStringA text;
			text.Preallocate(GetWindowTextLengthA(hRE));
			GetWindowTextA(hRE, text.GetBuffer(), text.GetAllocLength());
			std::string ut = to_unix_str(text.GetBuffer());
			ut = ut.substr(el->chrg.cpMin, el->chrg.cpMax - el->chrg.cpMin);
			return (int)ShellExecuteA(hwnd, "open", ut.c_str(), NULL, NULL, SW_NORMAL);
		}
	}
	return 0;
}
