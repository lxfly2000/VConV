#include "downloaddriver.h"
#include "ProgressDialog.h"
#include <VersionHelpers.h>
#include<shellapi.h>
#include<urlmon.h>
#include<sstream>
#include<thread>
#include<string>

#pragma comment(lib,"urlmon.lib")

const char driverURL[] = "https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe";

static std::thread tSub;

class StatusCallback :public IBindStatusCallback
{
private:
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		*ppv = NULL;
		if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) {
			*ppv = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return m_cRef++;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		if (--m_cRef == 0) {
			delete this;
			return 0;
		}
		return m_cRef;
	}

	STDMETHODIMP GetBindInfo(DWORD* grfBINDF, BINDINFO* bindinfo)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetPriority(LONG* nPriority)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* formatetc, STGMEDIUM* stgmed)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OnLowResource(DWORD reserved)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OnObjectAvailable(REFIID iid, IUnknown* punk)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding* pib)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
	{
		if (downloadStatus == 2 || downloadStatus == 3)
			return E_ABORT;
		if (ulProgressMax)
			SetProgressDialogBarValue(ulProgress * 100 / ulProgressMax);
		return S_OK;
	}

	DWORD m_cRef;
public:
	int downloadStatus;//0=������� 1=������ 2=���س��� 3=�����ж�
	StatusCallback() :m_cRef(1), downloadStatus(1) {}
};

static StatusCallback dcb;
static char retFilePath[MAX_PATH]{};
static LPCSTR retFileName = NULL;
static int downloadRetryLeft = 3;

void Subthread_Download()
{
	LPCSTR tURL = driverURL;
	retFileName = strrchr(tURL, '/') + 1;
	std::string title = "Downloading: ";
	title.append(retFileName);
	SetProgressDialogTitle(title.c_str());
	dcb.downloadStatus = 1;
	ProgressSetEnableClose(TRUE);
	downloadRetryLeft--;
	if (URLDownloadToCacheFileA(NULL, tURL, retFilePath, ARRAYSIZE(retFilePath) - 1, 0, &dcb) == S_OK)
	{
		ProgressSetEnableClose(FALSE);
		title = "Installing: ";
		title.append(retFileName);
		SetProgressDialogTitle(title.c_str());
		SetProgressDialogBarValue(-1);
		SHELLEXECUTEINFOA se{};
		se.cbSize = sizeof se;
		se.lpVerb = "open";
		se.lpFile = retFilePath;
		//qf:��ʾ�������û�����
		//qr:ֻ��ʾ��װ���Ľ���������
		//qb:ֻ��ʾmsiͨ�õĽ��������֣����ܻ����ĳЩ��װ���Ĳ������°�װʧ�ܣ�
		//qn:��ȫ����ʾ���棨�޷�����UACȨ�ޣ����ܵ��°�װʧ�ܣ�
		se.lpParameters = "/qr";
		se.fMask = SEE_MASK_NOCLOSEPROCESS;
		se.nShow = SW_NORMAL;
		ShellExecuteExA(&se);
		WaitForSingleObject(se.hProcess, INFINITE);
		DWORD retcode;
		GetExitCodeProcess(se.hProcess, &retcode);
		CloseHandle(se.hProcess);
		//MSIEXEC�����룺https://learn.microsoft.com/en-us/windows/win32/msi/error-codes
		//Advanced Installer�����룺https://www.advancedinstaller.com/user-guide/exe-setup-file.html
		if (retcode == 0x570 && downloadRetryLeft)
		{
			DeleteFileA(retFilePath);
			Subthread_Download();
			return;
		}
		EndProgressDialog(retcode);
	}
}

int DownloadDriver(HWND hwnd)
{
	ProgressSetOnClickCancel([]()
	{
		dcb.downloadStatus = 3;
		EndProgressDialog(E_ABORT);//��֪��Ϊɶ���ŵ�Subthread_Download��ȥ���ڳ����˳�ʱ����
	});
	ProgressSetOnShow([]()
	{
		tSub = std::thread(Subthread_Download);
	});
	switch (ProgressDialog(hwnd, "&Cancel"))
	{
	case E_ABORT:
		MessageBoxA(hwnd, "Download was interrupted, please download this driver manually later to finish setup.", retFileName, MB_ICONEXCLAMATION);
		break;
	case 0x570:
		MessageBoxA(hwnd, "File corrupted or unable to read, please download this driver manually later to finish setup.", retFileName, MB_ICONEXCLAMATION);
		break;
	case 1641:
		//����Ҫ����������ϵͳ
		break;
	case S_OK:default:
		//�޶���
		break;
	}
	tSub.join();
	return 0;
}
