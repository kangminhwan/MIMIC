#pragma once
#include "Netlib.h"

BEGIN_NETLIB

static TCHAR szLogGradeStrings[LOG_GRADE_CNT][20] = { _T("LOG_INFO"), _T("LOG_NOR"), _T("LOG_CRI"), _T("LOG_SYSTEM"), _T("LOG_TRACE") , _T("LOG_PACKET_TRACE") };

class cInterfaceLog
{
public:
	virtual void PushCommandString(const TCHAR* szLog, BOOL bCheckDate = FALSE, const LOG_GRADE grade = LOG_INFO) = 0;

	virtual void PushCommand(const LOG_GRADE grade, const TCHAR* szLog, ...) = 0;

	virtual void PushCommand(const LOG_GRADE grade, const char* szLog, ...) = 0;
};

END_NETLIB