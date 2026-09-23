#include "../../Include/Netlib/FileLoader/cIniFileLoader.h"

ATL::CAtlMap<CString, NetLib::cIniFileLoader*> NetLib::cIniFileLoader::m_atlmapInstance;
static BOOL FileLoadStatus = TRUE;

NetLib::cIniFileLoader::cIniFileLoader(TCHAR* filePath)
{
	m_pIniFilePath = nullptr;
	Clear();
	Create(filePath);
}

NetLib::cIniFileLoader::~cIniFileLoader()
{
	Clear();
}

NetLib::cIniFileLoader* NetLib::cIniFileLoader::CreateIniInstance(const CString& strKey, TCHAR* filePath)
{
	ATL::CAtlMap<CString, NetLib::cIniFileLoader*>::CPair* pPair = m_atlmapInstance.Lookup(strKey);
	if(pPair != nullptr)
	{
		return pPair->m_value;
	}
	NetLib::cIniFileLoader* pIniFileLoader = new NetLib::cIniFileLoader(filePath);
	m_atlmapInstance.SetAt(strKey, pIniFileLoader);
	return pIniFileLoader;
}

NetLib::cIniFileLoader* NetLib::cIniFileLoader::FindIniFile(const CString& strKey)
{
	ATL::CAtlMap<CString, NetLib::cIniFileLoader*>::CPair* pPair = m_atlmapInstance.Lookup(strKey);
	if(pPair == nullptr)
	{
		return nullptr;
	}
	return pPair->m_value;
}

void NetLib::cIniFileLoader::Destroy()
{
	POSITION pos = m_atlmapInstance.GetStartPosition();

	while (pos != nullptr)
	{
		NetLib::cIniFileLoader* pIniFileLoader = m_atlmapInstance.GetValueAt(pos);
		if(pIniFileLoader == nullptr)
		{
			continue;
		}
		delete pIniFileLoader;
		pIniFileLoader = nullptr;
		m_atlmapInstance.GetNext(pos);
	}
	m_atlmapInstance.RemoveAll();
}

bool NetLib::cIniFileLoader::Create(TCHAR* filePath)
{
	m_pIniFilePath = new TCHAR[_tcslen(filePath) + sizeof(TCHAR)];
	_tcscpy_s(m_pIniFilePath, _tcslen(filePath) + sizeof(TCHAR), filePath);

	return true;
}

void NetLib::cIniFileLoader::Clear()
{
	if(m_pIniFilePath != NULL)
	{
		delete m_pIniFilePath;
		m_pIniFilePath = NULL;
	}
}

bool NetLib::cIniFileLoader::WriteProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString)
{
	BOOL retval = WritePrivateProfileString(lpAppName, lpKeyName, lpString, m_pIniFilePath);

	return retval ? true : false;
}

UINT NetLib::cIniFileLoader::GetProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault)
{
	return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, m_pIniFilePath);
}

DWORD NetLib::cIniFileLoader::GetIniProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize)
{
	DWORD dwReturnedString = GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, m_pIniFilePath);

	if(_tcscmp(lpDefault, lpReturnedString) == 0)
	{
		FileLoadStatus = FALSE;
		TCHAR error[512];
		_stprintf_s(error, _T("%s Read Error AppName[%s], KeyName[%s]\n"), m_pIniFilePath, lpAppName, lpKeyName);
		MSG_BOX(error);
		exit(0);
	}
	return dwReturnedString;
}

DWORD NetLib::cIniFileLoader::GetIniProfileString(LPCTSTR iniFilePath, LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize)
{
	DWORD dwReturnedString = GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, iniFilePath);

	if(_tcscmp(lpDefault, lpReturnedString) == 0)
	{
		FileLoadStatus = FALSE;
		TCHAR error[512];
		_stprintf_s(error, _T("%s Read Error AppName[%s], KeyName[%s]\n"), iniFilePath, lpAppName, lpKeyName);
		MSG_BOX(error);
		exit(0);
	}
	return dwReturnedString;
}

BOOL NetLib::cIniFileLoader::CheckProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize)
{
	GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, m_pIniFilePath);

	if(_tcscmp(lpDefault, lpReturnedString) == 0)
		return FALSE;

	return TRUE;
}

BOOL NetLib::cIniFileLoader::GetFileLoadStatus()
{
	return FileLoadStatus;
}