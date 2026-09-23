#include "../../Include/Netlib/Queue/cWebQueue.h"
#include "../../Include/Netlib/Queue/cWebQueueElement.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/Common/cSingleton.h"

NetLib::cWebQueue::cWebQueue() :
	m_pMemPooler(nullptr),
	m_bPoolCreated(false)
{
}


NetLib::cWebQueue::~cWebQueue()
{
	Destroy();
}

void NetLib::cWebQueue::Destroy()
{
	SetExitThread();

	if(m_pMemPooler)
	{
		size_t nPoolCnt = m_pMemPooler->GetRemainPoolCnt();
		int nCurCnt = m_pMemPooler->GetCurrentPoolCnt();

		size_t nPool_32K_Cnt = m_pMemPooler->GetRemainPool_32K_Cnt();
		int nCur_32K_Cnt = m_pMemPooler->GetCurrentPool_32K_Cnt();

		bool bLoop = true;

		while (bLoop)
		{
			NetLib::cWebQueueElement* pElement = reinterpret_cast<NetLib::cWebQueueElement*>(PopQueue(10));
			if (pElement != nullptr)
			{
				Free(pElement);
			}

			nPoolCnt = m_pMemPooler->GetRemainPoolCnt();
			nCurCnt = m_pMemPooler->GetCurrentPoolCnt();

			nPool_32K_Cnt = m_pMemPooler->GetRemainPool_32K_Cnt();
			nCur_32K_Cnt = m_pMemPooler->GetCurrentPool_32K_Cnt();

			if ((int)nPoolCnt == nCurCnt && (int)nPool_32K_Cnt == nCur_32K_Cnt)
			{
				bLoop = false;
			}
		}


		delete m_pMemPooler;
		m_pMemPooler = nullptr;
	}
}

void NetLib::cWebQueue::Free(NetLib::cWebQueueElement* pElem)
{
	if(pElem)
	{
		pElem->MakeInitialize();

		if(pElem->GetBufferSize() > G_NET_BUFFER_SIZE_BASIC)
		{
			m_pMemPooler->Push_32K(pElem);
			return;
		}

		m_pMemPooler->Push(pElem);
	}
}

void NetLib::cWebQueue::CreateWebQueueElementPool(int nMaxCnt)
{
	if (nMaxCnt == 0)
		throw ("CreateWebQueueElementPool nMaxCnt size 0");

	m_bPoolCreated = true;

	m_pMemPooler = new NetLib::cMemPooler<NetLib::cWebQueueElement>(0, nMaxCnt, nMaxCnt, TRUE);

	//CreateWebQueueElementPool_64K();
}

//void NetLib::cWebQueue::CreateWebQueueElementPool_64K()
//{
//	for (int n = 0; n < 100; ++n)
//	{
//		//함수가 32K 이지만 실제로는 64K를 넣어 주고있습니다.
//		m_pMemPooler->Create_32K(G_NET_BUFFER_SIZE_64K);
//	}
//}

void NetLib::cWebQueue::ReportStatus(size_t& stRemainPoolCnt, int& nCurrentPoolCnt, int& nMaxPoolCnt)
{
	if (m_pMemPooler == nullptr)
		return;

	stRemainPoolCnt += m_pMemPooler->GetRemainPoolCnt();
	nCurrentPoolCnt += m_pMemPooler->GetCurrentPoolCnt();
	nMaxPoolCnt += m_pMemPooler->GetMaxPoolCnt();
}

bool NetLib::cWebQueue::PushCommand(const NetLib::cIocpContext* pContext,
									const UINT nCommand,
									const BYTE* lpBuffer,
									const UINT nLength)
{
	NetLib::cWebQueueElement* pElement = nullptr;
	if(nLength <= G_NET_BUFFER_SIZE_BASIC)
	{
		pElement = m_pMemPooler->Pop();
	}
	else if(nLength > G_NET_BUFFER_SIZE_BASIC && nLength < G_NET_BUFFER_SIZE_64K)
	{
		pElement = m_pMemPooler->Pop_32K();
	}
	else
	{
		assert(false && "cWebQueue::PushCommand() is Failed. failed data length 64K Size Over pContext");
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
									LOG_GRADE::LOG_CRI,
									_T("cWebQueue::PushCommand([%u]) is Failed. failed data size[%u] length 64K Size Over pContext"), 
									nCommand, 
									nLength);
		return false;
	}

	if(pElement)
	{
		if(pElement->CopyData(pContext, nCommand, lpBuffer, nLength))
		{
			if(!PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cWebQueueElement)))
			{
				Free(pElement);
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NetLib::cWebQueue::PushCommand - PostQueuedCompletionStatus failed. #1"));
				return false;
			}
			return true;
		}
		else
		{
			Free(pElement);
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NetLib::cWebQueue::PushCommand CopyData failed #1"));
			return false;
		}
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NetLib::cWebQueue::PushCommand enough command queue #1"));
	return false;
}

bool NetLib::cWebQueue::PushCommand(const UINT nID,
									const UINT nCommand,
									const BYTE* lpBuffer,
									const UINT nLength)
{

	NetLib::cWebQueueElement* pElement = nullptr;
	if(nLength <= G_NET_BUFFER_SIZE_BASIC)
	{
		pElement = m_pMemPooler->Pop();
	}
	else if(nLength > G_NET_BUFFER_SIZE_BASIC && nLength < G_NET_BUFFER_SIZE_64K)
	{
		pElement = m_pMemPooler->Pop_32K();
	}
	else
	{
		assert(false && "cWebQueue::PushCommand() is Failed. failed data length 64K Size Over nID");
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(	
									LOG_GRADE::LOG_CRI,
									_T("cWebQueue::PushCommand([%u]) is Failed. failed data size[%u] length 64K Size Over nID"),
									nCommand,
									nLength);
		return false;
	}

	if(pElement)
	{
		if(pElement->CopyData(nID, nCommand, lpBuffer, nLength))
		{
			if(!PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cWebQueue)))
			{
				Free(pElement);
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NetLib::cWebQueue::PushCommand - PostQueuedCompletionStatus failed. #2"));
				return false;
			}
			return true;
		}
		else
		{
			Free(pElement);
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NetLib::cWebQueue::PushCommand CopyData failed #2"));
			return false;
		}
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NetLib::cWebQueue::PushCommand enough command queue #2"));
	return false;
}