#include "Common.h"
//#pragma comment(lib,"win64udl_lib.lib")
//void DSE_OFF();
//void DSE_ON();
//�жϵ�ǰ�����Ƿ�ӵ�й���ԱȨ��
BOOL _IsRunasAdmin()
{
	/*DSE_OFF();*/
	BOOL bElevated = FALSE;
	HANDLE hToken = NULL;

	// Get current process token  
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		return FALSE;

	TOKEN_ELEVATION tokenEle;
	DWORD dwRetLen = 0;

	// Retrieve token elevation information  
	if (GetTokenInformation(hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen))
	{
		if (dwRetLen == sizeof(tokenEle))
		{
			bElevated = tokenEle.TokenIsElevated;
		}
	}

	CloseHandle(hToken);
	return bElevated;
}