#pragma once
#include<Windows.h>

HRESULT ProgressDialog(HWND hwndParent, LPCSTR strCancel);
BOOL SetProgressDialogTitle(LPCSTR title);
//0～100，-1为滚动样式
LONG SetProgressDialogBarValue(int percentage);
BOOL EndProgressDialog(int retCode);
void ProgressSetOnClickCancel(void(*fCallback)());
void ProgressSetOnShow(void(*fCallback)());
BOOL ProgressSetEnableClose(BOOL);
