#pragma once
#include "cIocpQueue.h"

BEGIN_NETLIB

class cWebQueue : public cIocpQueue
{
private:
	cMemPooler<class cWebQueueElement>* m_pMemPooler;
	bool m_bPoolCreated;

private:
	void Destroy();

public:
	void Free(class cWebQueueElement* pElem);

	void CreateWebQueueElementPool(int nMaxCnt);
	//void CreateWebQueueElementPool_64K();

	void ReportStatus(size_t& stRemainPoolCnt, int& nCurrentPoolCnt, int& nMaxPoolCnt);

public:
	bool PushCommand(	const class cIocpContext* pContext,
						const UINT nCommand,
						const BYTE* lpBuffer = nullptr,
						const UINT nLength = 0);

	bool PushCommand(	const UINT nID,
						const UINT nCommand,
						const BYTE* lpBuffer = nullptr,
						const UINT nLength = 0);

public:
	cWebQueue();
	~cWebQueue();
};

END_NETLIB