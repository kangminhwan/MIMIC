#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cLogQueueElement
{
private:
	TCHAR	m_tzLog[BUFFSIZE];
	static TCHAR m_nextline[3];
	UINT	m_uiStringSize;
	EStringColor m_stringcolor;
	LOG_GRADE m_log_grade;
public:
	BOOL	bUserLog;
	BOOL	bCheckNewFile; // 로그를 쓸때, bCheckData가 TRUE일 경우는 날짜가 변경되었는지 체크한다.

private:
	void	Init();
	void	Destroy();

public:
	//BOOL	CopyLogString(const TCHAR* szLog, const DWORD dwLength, const LOG_GRADE grade = LOG_INFO, BOOL isWriteTime = TRUE);
	BOOL	CopyLogString( const TCHAR* szLog , const DWORD dwLength , const LOG_GRADE grade , BOOL isWriteTime );
	BOOL	CopyUserLogString(const __int64 AID, const TCHAR* szLog, const DWORD dwLength, const LOG_GRADE grade = LOG_INFO);
	TCHAR*	GetStringBuffer();
	TCHAR*	GetStringBufferWithCarrigeReturn();
	const EStringColor GetStringColor() { return m_stringcolor; }
	const LOG_GRADE GetLogGrade() { return m_log_grade; }

public:
	cLogQueueElement();
	virtual ~cLogQueueElement();
};

END_NETLIB