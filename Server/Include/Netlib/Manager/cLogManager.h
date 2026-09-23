#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cLogManager
{
public:
	static BOOL	outputConsole;

private:
	HANDLE	m_hFile;
	HANDLE	m_hUserLogFile;
	BOOL	m_bMoveLogFolder;

	cCriticalSection m_Lock;

	TCHAR	szLogFileName[MAX_PATH];
	TCHAR   szUserLogFileName[MAX_PATH];

	SYSTEMTIME	m_TSystemTime;

	WORD wPresentDay;
	WORD wPresentHour;
	WORD wPresentMin;

public:
	bool	Create(TCHAR* pFileName, TCHAR* pUserLogFileName);
	void	Close();
	//TCHAR*	Open( TCHAR* pFileName );
	TCHAR*	Open();

	//void	WriteLog( TCHAR* format, ...);
	void	WriteLog(TCHAR* format, BOOL bUserLog);
	BOOL	Writes(TCHAR* pWriteBuffer, DWORD dwLen, BOOL bUserLog);

	BOOL	CheckDayChange();
	BOOL	CheckHourChange();
	BOOL	CheckMinChange();

public:
	cLogManager();
	~cLogManager();
};

END_NETLIB