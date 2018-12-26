
//NtDriver.cpp
#include "NtDriver.h"

CNtDriver::CNtDriver(LPTSTR lpszSysFilePath) {
	szSysFilePath = lpszSysFilePath;
	hSCM = NULL;
	hService = NULL;
	hDeviceN = 0;
}

CNtDriver::~CNtDriver() {
	//��������Ƿ���ֹͣ
	SERVICE_STATUS ss;

	if (hDeviceN) {
		__try {
			for (int i = 0; i<hDeviceN; i++)
				CloseHandle(hDevices[i]);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION) {
		}
		hDeviceN = 0;
	}

	if (hService) {
		if (!QueryServiceStatus(hService, &ss))
			MessageBox(GetThisHwnd(), _T("�޷���ȡ����״̬��"), _T("����"), MB_OK | MB_ICONSTOP);
		if (ss.dwCurrentState != SERVICE_STOPPED) {
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss)) {
				MessageBox(GetThisHwnd(), _T("�޷�ֹͣ������"), _T("����"), MB_OK | MB_ICONSTOP);
			}
		}
		if (!DeleteService(hService))
			MessageBox(GetThisHwnd(), _T("�޷�ɾ��������"), _T("����"), MB_OK | MB_ICONSTOP);
		CloseServiceHandle(hService);
		hService = 0;
	}
	if (hSCM) {
		CloseServiceHandle(hSCM);
	}
}

//���������ļ�·��
DWORD CNtDriver::SetFilePath(LPTSTR lpszSysFilePath) {
	//�������״̬�������ǹر�״̬��
	if (hService)
		return NDER_SERVICE_ALREADY_OPENED;

	szSysFilePath = lpszSysFilePath;
	return ExistFile(szSysFilePath) ? NDER_SUCCESS : NDER_FILE_NO_FOUND;
}

//���������豸
DWORD CNtDriver::Create(LPTSTR szDrvSvcName, LPTSTR szDrvDisplayName, LPTSTR filePath) {
	//(_T("Create service: %s\n"), szDrvSvcName);
	//�������״̬�������ǹر�״̬��
	if (hService)
		return NDER_SERVICE_ALREADY_OPENED;

	//����ļ��Ƿ����
	if (!ExistFile(filePath))
		return NDER_FILE_NO_FOUND;

	//���������ƹ�����
	if (!hSCM)
		hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM)
		return NDER_GETLASTERROR;

	//������������Ӧ������
	hService = CreateService(
		hSCM,
		szDrvSvcName,//��������
		szDrvDisplayName,//������ʾ����
		SERVICE_ALL_ACCESS,//Ȩ�ޣ�����Ȩ�ޣ�
		SERVICE_KERNEL_DRIVER,//�������ͣ��ں�������
		SERVICE_DEMAND_START,//�������ͣ��ֶ���
		SERVICE_ERROR_IGNORE,//���Դ���
		filePath,//�����ļ�·��
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);
	if (!hService) {
		hService = OpenService(hSCM, szDrvSvcName, SERVICE_ALL_ACCESS);
		if (!hService)
			return NDER_GETLASTERROR;
	}

	//(_T("Create service successful.\n"));
	return NDER_SUCCESS;
}

//�������豸
DWORD CNtDriver::Open(LPTSTR szDrvSvcName) {
	//(_T("Open service: %s\n"), szDrvSvcName);
	//�������״̬�������ǹر�״̬��
	if (hService)
		return NDER_SERVICE_ALREADY_OPENED;

	//���������ƹ�����
	if (!hSCM)
		hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM)
		return NDER_GETLASTERROR;

	//������
	hService = OpenService(hSCM, szDrvSvcName, SERVICE_ALL_ACCESS);
	if (!hService)
		return NDER_GETLASTERROR;

	//��ȡ��������
	LPQUERY_SERVICE_CONFIG pqsc;
	DWORD dwNeedSize = 0;
	DWORD dwServiceType=NULL;
	QueryServiceConfig(hService, NULL, 0, &dwNeedSize);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		pqsc = (LPQUERY_SERVICE_CONFIG)malloc(dwNeedSize);
		if (QueryServiceConfig(hService, pqsc, dwNeedSize, &dwNeedSize)) {
			dwServiceType = pqsc->dwServiceType;
			free(pqsc);
		}
		else
			return NDER_GETLASTERROR;
	}
	else
		return NDER_GETLASTERROR;

	//�����������
	if (dwServiceType != SERVICE_KERNEL_DRIVER) {
		CloseServiceHandle(hService);
		hService = 0;
		SetLastError(ERROR_BAD_DRIVER);
		return NDER_GETLASTERROR;
	}

	//(_T("Open service successful.\n"));
	return NDER_SUCCESS;
}

//��������
DWORD CNtDriver::Load() {
	//(_T("Start service..\n"));
	//��������Ƿ��Ѵ�
	if (!hService)
		return NDER_SERVICE_NOT_OPEN;

	SERVICE_STATUS ss;
	if (!QueryServiceStatus(hService, &ss))
		return NDER_GETLASTERROR;
	if (ss.dwCurrentState != SERVICE_STOPPED)
		return NDER_SERVICE_ALREADY_STARTED;

	//��������
	if (!StartService(hService, NULL, NULL)) {
		DWORD dwErr = GetLastError();
		if (dwErr == ERROR_IO_PENDING)
			return NDER_SERVICE_IO_PENDING;
		else if (dwErr == ERROR_SERVICE_ALREADY_RUNNING)
			return NDER_SERVICE_ALREADY_STARTED;
		else
			return NDER_GETLASTERROR;
	}

	//(_T("Start service successful.\n"));
	return NDER_SUCCESS;
}

//��ȡ����״̬
DWORD CNtDriver::GetStatus(LPTSTR driverServiceName) {
	//(_T("GetStatus service..\n"));
	//��������Ƿ��Ѵ�
	//������
	/*if (!hSCM)
		hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM)
		return NDER_GETLASTERROR;

	hService = OpenService(hSCM, driverServiceName, SERVICE_ALL_ACCESS);
	if (!hService)
		return DRIVER_CLOSED;*/

	if (!hService)
		return DRIVER_CLOSED;

	SERVICE_STATUS ss;
	if (!QueryServiceStatus(hService, &ss))
		return NDER_GETLASTERROR;

	switch (ss.dwCurrentState) {
	case SERVICE_RUNNING:
		return DRIVER_STARTED;
	case SERVICE_STOPPED:
		return DRIVER_OPENED;
	case SERVICE_PAUSED:
		return DRIVER_PAUSED;
	default:
		return DRIVER_BUSY;
	}
}

//ж������
DWORD CNtDriver::Unload(DWORD dwWaitMilliseconds = 1) {
	//(_T("Unload service..\n"));
	//��������Ƿ��Ѵ�
	if (!hService)
		return NDER_SERVICE_NOT_OPEN;

	//��������Ƿ���ֹͣ
	SERVICE_STATUS ss;
	if (!QueryServiceStatus(hService, &ss))
		return NDER_GETLASTERROR;
	if (ss.dwCurrentState == SERVICE_STOPPED)
		return NDER_SERVICE_NOT_STARTED;

	if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss))
		return NDER_GETLASTERROR;

	DWORD dwStopTick = GetTickCount();
	while (GetTickCount()<dwStopTick + dwWaitMilliseconds) {
		Sleep(1);
		if (!QueryServiceStatus(hService, &ss))
			return NDER_GETLASTERROR;
		switch (ss.dwCurrentState) {
		case SERVICE_STOPPED:
			return NDER_SUCCESS;
		case SERVICE_STOP_PENDING:
			break;
		default:
			return NDER_SERVICE_IO_PENDING;
		}
	}

	//(_T("Unload service successful.\n"));
	return NDER_SUCCESS;
}

DWORD CNtDriver::Close() {
	//(_T("Close service..\n"));
	//��������Ƿ��Ѵ�
	if (!hService)
		return NDER_SERVICE_NOT_OPEN;

	//�ر��������
	if (!CloseServiceHandle(hService))
		return NDER_GETLASTERROR;
	hService = 0;

	//(_T("Close service successful.\n"));
	return NDER_SUCCESS;
}

DWORD CNtDriver::Delete() {
	//(_T("Delete service..\n"));
	//��������Ƿ��Ѵ�
	if (!hService)
		return NDER_SERVICE_NOT_OPEN;

	//��������Ƿ���ֹͣ
	SERVICE_STATUS ss;
	if (!QueryServiceStatus(hService, &ss))
		return NDER_GETLASTERROR;
	if (ss.dwCurrentState != SERVICE_STOPPED)
		return NDER_SERVICE_ALREADY_STARTED;

	//ɾ������
	if (!DeleteService(hService))
		return NDER_GETLASTERROR;

	//�ر��������
	if (!CloseServiceHandle(hService))
		return NDER_GETLASTERROR;
	hService = 0;

	//�ر�SCM���
	if (hSCM) {
		if (!CloseServiceHandle(hSCM))
			return NDER_GETLASTERROR;
		hSCM = 0;
	}

	//(_T("Delete service successful.\n"));
	return NDER_SUCCESS;
}

HANDLE CNtDriver::OpenDevice(LPTSTR lpszSymLinkName) {
	//��������Ƿ��Ѵ�
	if (!hService)
		return 0;

	//��������Ƿ���������
	SERVICE_STATUS ss;
	if (!QueryServiceStatus(hService, &ss))
		return 0;
	if (ss.dwCurrentState != SERVICE_RUNNING)
		return 0;

	HANDLE hDevice;
	hDevice = CreateFile(
		lpszSymLinkName,
		GENERIC_READ | GENERIC_WRITE,
		0,	//�ǹ���
		NULL,	//��ȫ������
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL	//û��ģ��
	);
	if (hDevice >= 0)
		hDevices[hDeviceN++] = hDevice;
	return hDevice;
}

BOOL CNtDriver::CloseDevice(HANDLE hDevice) {
	int i;
	if (CloseHandle(hDevice)) {
		for (i = 0; i<hDeviceN; i++) {
			if (hDevice == hDevices[i]) {
				hDevices[i] = hDevices[--hDeviceN];
			}
		}
		return TRUE;
	}
	else
		return FALSE;
}

void CNtDriver::OperateDriver() {
	//��������Ƿ��Ѵ�
	if (!hService) {
		MessageBox(GetActiveWindow(), _T("����û�а�װ��"), _T("����"), MB_OK | MB_ICONSTOP);
		return;
	}
	//��������Ƿ���������
	SERVICE_STATUS ss;
	if (!QueryServiceStatus(hService, &ss))
	{
		MessageBox(GetActiveWindow(), _T("��ȡ����״̬ʧ�ܣ�"), _T("����"), MB_OK | MB_ICONSTOP);
		return;
	}
	if (ss.dwCurrentState != SERVICE_RUNNING)
	{
		MessageBox(GetActiveWindow(), _T("����û�����У�"), _T("����"), MB_OK | MB_ICONSTOP);
		return;
	}

	HANDLE DeviceHandle;
	DeviceHandle = CreateFile(
		DEVICE_LINK_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,	//�ǹ���
		NULL,	//��ȫ������
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL	//û��ģ��
	);
	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		MessageBox(GetActiveWindow(), _T("�����������ʧ�ܣ�"), _T("����"), MB_OK | MB_ICONSTOP);
		return;
	}
	char BufferData = NULL;
	DWORD ReturnLength = 0;
	BOOL IsOk = DeviceIoControl(DeviceHandle, CTL_SYS,
		"Ring3->Ring0",//��������
		strlen("Ring3->Ring0") + 1,//�������ݳ���
		(LPVOID)BufferData,//�������
		0,//������ݳ���
		&ReturnLength,
		NULL);
	if (IsOk == FALSE)
	{
		int LastError = GetLastError();

		if (LastError == ERROR_NO_SYSTEM_RESOURCES)
		{
			char BufferData[MAX_PATH] = { 0 };
			IsOk = DeviceIoControl(DeviceHandle, CTL_SYS,
				"Ring3->Ring0",//��������
				strlen("Ring3->Ring0") + 1,//�������ݳ���
				(LPVOID)BufferData,//�������
				MAX_PATH,//������ݳ���
				&ReturnLength,
				NULL);

			if (IsOk == TRUE)
			{
				Sleep(2000);
				OutputDebugStringA((LPCSTR)BufferData);
				//MessageBox(GetActiveWindow(), (LPTSTR)BufferData, _T("����"), MB_OK | MB_ICONSTOP);
			}
		}
	}
	if (DeviceHandle != NULL)
	{
		CloseHandle(DeviceHandle);
		DeviceHandle = NULL;
	}
}

string CNtDriver::GetLastErrorToString(DWORD errorCode)
{
	//��ΪFORMAT_MESSAGE_ALLOCATE_BUFFER��־�����������������ڴ棬������ҪLocalFree���ͷ�
	char *text;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&text, 0, NULL);
	string result(text);    //���
	LocalFree(text);
	//return stringToLPCWSTR(result);
	return result;
}

LPCWSTR CNtDriver::stringToLPCWSTR(string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	return wcstring;
}

void CNtDriver::ShowError() {
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);
	MessageBox(GetActiveWindow(), (LPTSTR)lpMsgBuf, _T("����"), MB_OK | MB_ICONSTOP);
	LocalFree(lpMsgBuf);
}

HWND CNtDriver:: GetThisHwnd() {
	return GetActiveWindow();
}

