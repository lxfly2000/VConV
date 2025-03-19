#include "ProgressDialog.h"
#include "ProgressDialogRes.h"
#include <CommCtrl.h>

static HWND hwndDialog = nullptr;

struct ProgressBarParam
{
	void(*pfOnCancel)();
	void(*pfOnShow)();
	int pgMin;
	int pgMax;
	LPCSTR strCancel;
	ProgressBarParam():pfOnCancel(nullptr),pfOnShow(nullptr),pgMin(0),pgMax(100),strCancel(NULL){}
};
static ProgressBarParam pp;

BOOL SetProgressDialogTitle(LPCSTR title)
{
	return SetWindowTextA(hwndDialog, title);
}

LONG SetProgressDialogBarValue(int percentage)
{
	HWND hpg = GetDlgItem(hwndDialog, IDC_PROGRESS_MAIN);
	if (percentage == -1)
	{
		LONG s = GetWindowLongPtr(hpg, GWL_STYLE);
		if (!(s & PBS_MARQUEE))
		{
			s |= PBS_MARQUEE;
			SetWindowLongPtr(hpg, GWL_STYLE, s);
			SendDlgItemMessage(hwndDialog, IDC_PROGRESS_MAIN, PBM_SETMARQUEE, TRUE, 0);
		}
	}
	else
	{
		LONG s = GetWindowLongPtr(hpg, GWL_STYLE);
		if (s & PBS_MARQUEE)
		{
			s &= ~PBS_MARQUEE;
			SetWindowLongPtr(hpg, GWL_STYLE, s);
			SendDlgItemMessage(hwndDialog, IDC_PROGRESS_MAIN, PBM_SETMARQUEE, FALSE, 0);
		}
	}
	return SendDlgItemMessage(hwndDialog, IDC_PROGRESS_MAIN, PBM_SETPOS, percentage, 0);
}

BOOL EndProgressDialog(int retCode)
{
	return EndDialog(hwndDialog, retCode);
}

INT_PTR WINAPI ProgressDialogCallback(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		hwndDialog = hDlg;
		if (pp.strCancel)
			SetDlgItemTextA(hDlg, IDCANCEL, pp.strCancel);
		SendDlgItemMessage(hDlg, IDC_PROGRESS_MAIN, PBM_SETRANGE, 0, MAKELPARAM(pp.pgMin, pp.pgMax));
		SetProgressDialogBarValue(0);
		break;
	case WM_SHOWWINDOW:
		if (pp.pfOnShow)
			pp.pfOnShow();
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if (pp.pfOnCancel)
				pp.pfOnCancel();
			break;
		}
		break;
	}
	return 0;
}

HRESULT ProgressDialog(HWND hwndParent, LPCSTR strCancel)
{
	pp.strCancel = strCancel;
	return DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_PROGRESS), hwndParent, ProgressDialogCallback);
}

void ProgressSetOnClickCancel(void(*fCallback)())
{
	pp.pfOnCancel = fCallback;
}

void ProgressSetOnShow(void(*fCallback)())
{
	pp.pfOnShow = fCallback;
}

BOOL ProgressSetEnableClose(BOOL b)
{
	EnableMenuItem(GetSystemMenu(hwndDialog, FALSE), SC_CLOSE, b ? MF_ENABLED : MF_DISABLED);
	return EnableWindow(GetDlgItem(hwndDialog, IDCANCEL), b);
}
