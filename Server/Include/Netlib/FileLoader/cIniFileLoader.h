#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cIniFileLoader
{
private:
	static ATL::CAtlMap<CString, cIniFileLoader*> m_atlmapInstance;

private:
	TCHAR* m_pIniFilePath;

public:
	static cIniFileLoader* CreateIniInstance(const CString& strKey, TCHAR* filePath);
	static cIniFileLoader* FindIniFile(const CString& strKey);
	static void Destroy();
	static DWORD GetIniProfileString(LPCTSTR iniFilePath, LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
public:
	//Ini파일 입출력함수
	bool WriteProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString);
	UINT GetProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault);
	DWORD GetIniProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
	BOOL CheckProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);

	BOOL GetFileLoadStatus();

private:
	bool Create(TCHAR* filePath);
	void Clear();

private:
	cIniFileLoader(TCHAR* filePath);
	virtual ~cIniFileLoader();
};

END_NETLIB