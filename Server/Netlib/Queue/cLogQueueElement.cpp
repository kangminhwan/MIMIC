#include "../../Include/Netlib/Queue/cLogQueueElement.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TCHAR NetLib::cLogQueueElement::m_nextline[3] = _T("\r\n");

NetLib::cLogQueueElement::cLogQueueElement()
{
	Init();
}


NetLib::cLogQueueElement::~cLogQueueElement()
{
	Destroy();
}

void NetLib::cLogQueueElement::Init()
{
	memset(m_tzLog, 0, sizeof(TCHAR)*BUFFSIZE);
	bCheckNewFile = FALSE;
	m_uiStringSize = 0;
	bUserLog = FALSE;
	m_log_grade = LOG_GRADE::LOG_NONE;
}

void NetLib::cLogQueueElement::Destroy()
{
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

BOOL NetLib::cLogQueueElement::CopyLogString(const TCHAR* tzLog, const DWORD dwLength, const LOG_GRADE grade, BOOL isWriteTime /*= TRUE*/)
{
	if((!tzLog) || (dwLength > BUFFSIZE))
		return FALSE;

	bUserLog = FALSE;

	// grade별로 스트링 컬러를 정한다.
	switch (grade)
	{
	case LOG_INFO:
		m_stringcolor = BLUE;
		break;
	case LOG_NOR:
		m_stringcolor = BLACK;
		break;
	case LOG_CRI:
		m_stringcolor = RED;
		break;
	case LOG_SYSTEM:
		m_stringcolor = SYSTEM;
		break;
	case LOG_ALERT:
		m_stringcolor = RED;
		break;
	case LOG_TRACE:
		m_stringcolor = TRACING;
		break;
	case LOG_PACKET_TRACE:
		m_stringcolor = PACKET_TRACE;
		break;
	default:
		m_stringcolor = BLACK;
		break;
	}

	m_log_grade = grade;

	// 로그타임 체그
	if(isWriteTime && grade != LOG_ALERT)
	{
		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);
		//swprintf_s( m_tzLog, _T("{%d-%02d-%02d %02d:%02d:%02d.%03d} %s\x0d\x0a"), 
		swprintf_s(m_tzLog, _T("{%d-%02d-%02d %02d:%02d:%02d.%03d} %s"),
			systemTime.wYear,
			systemTime.wMonth,
			systemTime.wDay,
			systemTime.wHour,
			systemTime.wMinute,
			systemTime.wSecond,
			systemTime.wMilliseconds,
			tzLog
		);
	}
	else
	{
		_tcsncpy_s(m_tzLog, _countof(m_tzLog), tzLog, _countof(m_tzLog) - 1);
	}

	m_uiStringSize = (UINT)_tcslen(m_tzLog);

	return TRUE;
}

BOOL NetLib::cLogQueueElement::CopyUserLogString(const __int64 AID, const TCHAR* tzLog, const DWORD dwLength, const LOG_GRADE grade)
{
	if((!tzLog) || (dwLength > BUFFSIZE))
		return FALSE;

	bUserLog = TRUE;

	// grade별로 스트링 컬러를 정한다.
	switch (grade)
	{
	case LOG_INFO:
		m_stringcolor = BLUE;
		break;
	case LOG_NOR:
		m_stringcolor = BLACK;
		break;
	case LOG_CRI:
		m_stringcolor = RED;
		break;
	case LOG_SYSTEM:
		m_stringcolor = SYSTEM;
		break;
	case LOG_ALERT:
		m_stringcolor = RED;
		break;
	case LOG_TRACE:
		m_stringcolor = TRACING;
		break;
	case LOG_PACKET_TRACE:
		m_stringcolor = PACKET_TRACE;
		break;
	default:
		m_stringcolor = BLACK;
		break;
	}

	// 로그타임 체그
	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);

	//swprintf_s( m_tzLog, _T("{%d-%02d-%02d %02d:%02d:%02d.%03d} %s\x0d\x0a"), 
	swprintf_s(m_tzLog, _T("[%d-%02d-%02d %02d:%02d:%02d.%03d] [AID=%I64d] [%s]"),
		systemTime.wYear,
		systemTime.wMonth,
		systemTime.wDay,
		systemTime.wHour,
		systemTime.wMinute,
		systemTime.wSecond,
		systemTime.wMilliseconds,
		AID,
		tzLog
	);

	m_uiStringSize = (UINT)_tcslen(m_tzLog);

	return TRUE;
}

TCHAR* NetLib::cLogQueueElement::GetStringBuffer()
{
	return m_tzLog;
}

TCHAR*	NetLib::cLogQueueElement::GetStringBufferWithCarrigeReturn()
{
	memcpy(m_tzLog + m_uiStringSize, m_nextline, sizeof(m_nextline));
	return m_tzLog;
}