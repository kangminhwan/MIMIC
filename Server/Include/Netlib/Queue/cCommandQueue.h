#pragma once
#include "cIocpQueue.h"

BEGIN_NETLIB

const float fWarningRate = 0.5;

class cUDPIocpOv;
class cUDPDispatcher;
class cIocpContext;
class cCommandQueueElement;
class cCommandQueue : public cIocpQueue
{
private:
	cMemPooler<cCommandQueueElement>	*m_pMemPooler;
	bool	m_bPoolCreated;

public:
	bool	PushCommand(	const cIocpContext* pContext,
							const UINT nCommand,
							const BYTE* lpBuffer = NULL,
							const WORD nLength = 0);

	bool	PushCommand(	const UINT nID,
							const UINT nCommand,
							const BYTE* lpBuffer = NULL,
							const WORD nLength = 0);

	bool	PushCommand(	const cUDPDispatcher* pUDPDispatcher,
							const UINT nCommand,
							const UINT Entity,
							const UINT uiPacketSeq,
							const cUDPIocpOv* pUdpIocpOv,
							const BYTE* lpBuffer = NULL,
							const WORD nLength = 0);

	void	Free(cCommandQueueElement* pElem);

	void	ReportStatus();
	size_t	GetRemainQueueCnt();
	int		GetMaxPoolCnt();
	int		GetCurrentPoolCnt();

	void	CreateCommandQueueElementPool(int nMaxCnt);
	//void    CreateCommandQueueElementPool_64K(int nMaxCnt);

public:
	cCommandQueue();
	virtual ~cCommandQueue();
};

END_NETLIB