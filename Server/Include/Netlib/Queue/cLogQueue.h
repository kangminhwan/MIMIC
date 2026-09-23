#pragma once
#include "../Common/cInterfaceLog.h"
#include "cIocpQueue.h"

BEGIN_NETLIB

class cLogQueueElement;
class cLogQueue : public cInterfaceLog, public cIocpQueue
{
public:
	void	PushCommandString(const TCHAR* szLog, BOOL bCheckNewFile = FALSE, const LOG_GRADE grade = LOG_INFO);

	void	PushCommand(const LOG_GRADE grade, const TCHAR* szFormat, ...);

	void	PushCommand(const LOG_GRADE grade, const char* szLog, ...);

	// 이셉션이 발생되는 푸쉬커맨드임
	void	EraseExcetionPushCommand(const LOG_GRADE grade, const char* szLog, ...);

	// UserLog 추가
	void	PushUserCommand(const __int64 AID, const LOG_GRADE grade, const TCHAR* szFormat, ...);

	void PushCommandLongString(const LOG_GRADE grade, const char* szFormat, ...);

public:
	void	Free(cLogQueueElement* pElem);

	void	CreateLogQueueElementPool(int nMaxCnt);

	size_t	GetRemainQueueCnt();
	int		GetMaxPoolCnt();
	int		GetCurrentPoolCnt();

private:
	cMemPooler<cLogQueueElement>* m_pMemPooler;
	bool	m_bPoolCreated;

public:
	void	SetStopListBoxPrint() { bListBoxPrintOpertaion = false; }
	void	SetGoOnListBoxPrint() { bListBoxPrintOpertaion = true; }
	bool	GetListBoxPrint() { return bListBoxPrintOpertaion; }
	bool	bListBoxPrintOpertaion;

public:
	cLogQueue();
	virtual ~cLogQueue();
};

END_NETLIB