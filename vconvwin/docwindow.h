#pragma once
#include <Windows.h>

void DocWindowA(HWND hWndParent, char*doc, int length);
void SetRichEditTextA(HWND h, LPCSTR text);
INT_PTR CALLBACK OnRichEditClickMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
