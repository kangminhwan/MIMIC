#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Queue/cCommandQueueElement.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"

NetLib::cCommandQueue::cCommandQueue() :
	m_bPoolCreated(false),
	m_pMemPooler(nullptr)
{
}


NetLib::cCommandQueue::~cCommandQueue()
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
			NetLib::cCommandQueueElement* pElement = reinterpret_cast<NetLib::cCommandQueueElement*>(PopQueue(100));
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

void NetLib::cCommandQueue::CreateCommandQueueElementPool(int nMaxCnt)
{
	if (nMaxCnt == 0)
		throw ("CreateCommandQueueElementPool nMaxCnt size 0");

	m_bPoolCreated = true;

	m_pMemPooler = new NetLib::cMemPooler<NetLib::cCommandQueueElement>(0, nMaxCnt, nMaxCnt, TRUE);

	//CreateCommandQueueElementPool_64K(nMaxCnt);
}

//void NetLib::cCommandQueue::CreateCommandQueueElementPool_64K(int nMaxCnt)
//{
//	// 100개만 만들고, 쓰자 cMemPooler 생성자에서 BaseQueueElement의 버퍼 사이즈를 다르게 생성가능한 방법 연구
//	for (int n = 0; n<100; ++n)
//	{
//		m_pMemPooler->Create_32K(G_NET_BUFFER_SIZE_64K);// 32K 이지만 실지로는 64를 넣고 있습니다. ㅠㅠ
//	}
//}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

bool NetLib::cCommandQueue::PushCommand(const NetLib::cIocpContext* pContext,
										const UINT nCommand,
										const BYTE* lpBuffer,
										const WORD nLength)
{
	NetLib::cCommandQueueElement* pElement = NULL;
	if(nLength <= G_NET_BUFFER_SIZE_BASIC)
		pElement = m_pMemPooler->Pop();
	else if (nLength > G_NET_BUFFER_SIZE_BASIC && nLength < G_NET_BUFFER_SIZE_64K)
	{
		pElement = m_pMemPooler->Pop_32K();
	}
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand[%u] Size[%u] failed data length over 32K"), nCommand, nLength);
		return false;
	}

	if(pElement)
	{
		if(pElement->CopyData(pContext, nCommand, lpBuffer, nLength))
		{
			if(!PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cCommandQueueElement)))
			{
				Free(pElement);
				NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand - PostQueuedCompletionStatus failed #1"));
				return false;
			}
			return true;
		}
		else
		{
			Free(pElement);
			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand CopyData failed #1"));
			return false;
		}
	}

	NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand not enough command queue #1"));
	return false;
}

bool NetLib::cCommandQueue::PushCommand(const UINT nID,
										const UINT nCommand,
										const BYTE* lpBuffer,
										const WORD nLength)
{
	NetLib::cCommandQueueElement* pElement = NULL;
	if(nLength <= G_NET_BUFFER_SIZE_BASIC)
		pElement = m_pMemPooler->Pop();
	else if(nLength > G_NET_BUFFER_SIZE_BASIC && nLength < G_NET_BUFFER_SIZE_32K)
		pElement = m_pMemPooler->Pop_32K();
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand failed data length over 32K"));
		return false;
	}

	if(pElement)
	{
		if(pElement->CopyData(nID, nCommand, lpBuffer, nLength))
		{
			if(!PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cCommandQueueElement)))
			{
				Free(pElement);
				NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand - PostQueuedCompletionStatus failed #2"));
				return false;
			}

			return true;
		}
		else
		{
			Free(pElement);
			return false;
		}
	}

	NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand not enough command queue #2"));
	return false;
}

bool NetLib::cCommandQueue::PushCommand(const NetLib::cUDPDispatcher* pUDPDispatcher,
										const UINT nCommand,
										const UINT Entity,
										const UINT uiPacketSeq,
										const NetLib::cUDPIocpOv* pUdpIocpOv,
										const BYTE* lpBuffer,
										const WORD nLength)
{
	NetLib::cCommandQueueElement* pElement = NULL;
	if(nLength <= G_NET_BUFFER_SIZE_BASIC)
		pElement = m_pMemPooler->Pop();
	else if(nLength > G_NET_BUFFER_SIZE_BASIC && nLength < G_NET_BUFFER_SIZE_32K)
		pElement = m_pMemPooler->Pop_32K();
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand failed data length over 32K"));
		return false;
	}

	if(pElement)
	{
		if(pElement->CopyData(pUDPDispatcher, nCommand, Entity, uiPacketSeq, pUdpIocpOv, lpBuffer, nLength))
		{
			if(!PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cCommandQueueElement)))
			{
				Free(pElement);
				NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand - PostQueuedCompletionStatus failed #3"));
				return false;
			}
		}
		else
		{
			Free(pElement);
			return false;
		}
		return true;
	}

	NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib::cCommandQueue::PushCommand not enough command queue #3"));
	return false;
}

void NetLib::cCommandQueue::ReportStatus()
{
	if(!m_bPoolCreated) return;

	size_t nRemainCommandQueueCnt = 0;
	size_t nCommandQueueCnt = m_pMemPooler->GetRemainPoolCnt();

	nRemainCommandQueueCnt = m_pMemPooler->GetRemainPoolCnt();

	float Rate = (float)nRemainCommandQueueCnt / nCommandQueueCnt;

	if((Rate <= fWarningRate) || (nRemainCommandQueueCnt == 0))
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("Warning Remained CommandQueueCnt is too small [count %d]"), nRemainCommandQueueCnt);
	}
}

size_t NetLib::cCommandQueue::GetRemainQueueCnt()
{
	if(!m_bPoolCreated) return 0;

	return m_pMemPooler->GetRemainPoolCnt();
}

int NetLib::cCommandQueue::GetMaxPoolCnt()
{
	if(!m_bPoolCreated)
		return 0;

	return m_pMemPooler->GetMaxPoolCnt();
}

int NetLib::cCommandQueue::GetCurrentPoolCnt()
{
	if(!m_bPoolCreated)
		return 0;

	return m_pMemPooler->GetCurrentPoolCnt();
}

void NetLib::cCommandQueue::Free(NetLib::cCommandQueueElement* pElem)
{
	pElem->MakeInitialize();

	if(pElem->GetBufferSize() > G_NET_BUFFER_SIZE_BASIC)
		m_pMemPooler->Push_32K(pElem);
	else
		m_pMemPooler->Push(pElem);
}