#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#include "../../Include/Netlib/Manager/cSessionManager.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Session/cSession.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"

NetLib::cCommandQueueManager::cCommandQueueManager() :
	m_nCommandQueueCnt(0),
	m_nServerCommandArray(0)
{
	m_pCommandQueue = nullptr;
}


NetLib::cCommandQueueManager::~cCommandQueueManager()
{
	for (int n = 0; n<m_nCommandQueueCnt; ++n)
	{
		if(m_pCommandQueue[n] != NULL)
			delete m_pCommandQueue[n];
	}

	delete[] m_pCommandQueue;
}

bool NetLib::cCommandQueueManager::Init(const int nCommandQueueCnt, const int nCommandQueuePoolSize)
{
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(pServerManager == nullptr)
	{
		assert(false && " cCommandQueueManager::Init() is Failed. ServerManager is nullptr");
		return false;
	}

	TServerConfiguration* pServerConfig = pServerManager->GetConfiguration();
	if(pServerConfig == nullptr)
	{
		assert(false && "cCommandQueueManager::Init() is Failed. ServerConfiguration is nullptr");
		return false;
	}

	if(nCommandQueueCnt < 0 || nCommandQueueCnt > pServerConfig->nCommandThreadCnt)
	{
		Assert(FALSE, _T("cCommandQueueManager nCommandQueueCnt Must Set 1 ~ nCommandThreadCnt"));
		return false;
	}

	m_pCommandQueue = new NetLib::cCommandQueue*[pServerConfig->nCommandThreadCnt];
	memset(m_pCommandQueue, 0x00, sizeof(int*) * pServerConfig->nCommandThreadCnt);

	if(nCommandQueueCnt == 1)
	{
		m_pCommandQueue[0] = NetLib::cSingleton<NetLib::cCommandQueue>::GetInstance();
		m_pCommandQueue[0]->CreateCommandQueueElementPool(nCommandQueuePoolSize);
		m_nServerCommandArray = 0;
	}
	else
	{
		for (int n = 0; n<nCommandQueueCnt; ++n)
		{
			if(n == nCommandQueueCnt - 1)// 마지막 큐를 공용큐로 사용한다.
				m_pCommandQueue[n] = NetLib::cSingleton<NetLib::cCommandQueue>::GetInstance();
			else
				m_pCommandQueue[n] = new NetLib::cCommandQueue;

			m_pCommandQueue[n]->CreateCommandQueueElementPool(nCommandQueuePoolSize);
		}

		m_nServerCommandArray = nCommandQueueCnt - 1;
	}

	m_nCommandQueueCnt = nCommandQueueCnt;

	return true;
}

void NetLib::cCommandQueueManager::PrintAliveStatus()
{
	ULONGLONG presentTick = GetTickCount64();
	int idleThreadCont = 0;

	for (int n = 0; n<m_nCommandQueueCnt; ++n)
	{
		if(m_pCommandQueue[n] == nullptr)
			continue;

		// AliveTick을 얻어와서, 정보를 출력한다.
		ULONGLONG aliveTick = m_pCommandQueue[n]->GetAliveTick();

		// 10 초 이상차이나면 워닝 띄우고, 유휴시간 표시
		ULONGLONG threadIdleTick = presentTick - aliveTick;
		if(threadIdleTick >= 10000)
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI
				, _T("Warning cCommandThread, array[%d], IDLE time[%I64d ms]"), n, threadIdleTick);

			++idleThreadCont;
		}
	}

	if(idleThreadCont)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T("=========== COMMAND THREAD INFO ==========="));

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI
			, _T("== [Warning] cCommandThread, Total[%d], IDLE ThreadCnt[%d]"), m_nCommandQueueCnt, idleThreadCont);

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T("==================================="));
	}
}

int NetLib::cCommandQueueManager::GetRoomThreadNumber(int nRoomNo)
{
	return nRoomNo % m_nCommandQueueCnt;
}

NetLib::cCommandQueue* NetLib::cCommandQueueManager::GetCommandQueuePtr(const int nThreadArray)
{
	if(nThreadArray < 0 || nThreadArray >= m_nCommandQueueCnt)
		return NULL;

	return m_pCommandQueue[nThreadArray];
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

bool NetLib::cCommandQueueManager::PushCommand(NetLib::cIocpContext* pContext,
												const UINT nCommand,
												const BYTE* lpBuffer,
												const WORD nLength)
{
#ifdef USING_MULTI_THREAD
	UINT nCommandQueueArray = pContext->GetCommandQueueIndex();
#else
	UINT nCommandQueueArray = 0;
#endif

	if(nCommandQueueArray < 0 || nCommandQueueArray >= (UINT)m_nCommandQueueCnt)
	{
#ifdef _DEBUG
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI
			, _T("cCommandQueueManager::PushCommand return!! (CmdQArr<0 || CmdQArr >= m_CmdQCnt)!! CMD(%d) Len(%u) CmdQueArr(%d) m_nCmdQueCnt(%d)")
			, nCommand, nLength, nCommandQueueArray, m_nCommandQueueCnt);
#endif
		return false;
	}

	if(nCommandQueueArray >= (UINT)m_nCommandQueueCnt
		|| !m_pCommandQueue[nCommandQueueArray])
	{
#ifdef _DEBUG
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI
			, _T("cCommandQueueManager::PushCommand  return!! (m_pCommandQueue[%d] is NULL)!! CMD(%d) Len(%u) CmdQueArr(%d) m_nCmdQueCnt(%d)")
			, nCommandQueueArray, nCommand, nLength, nCommandQueueArray, m_nCommandQueueCnt);
#endif
		return false;
	}

	m_pCommandQueue[nCommandQueueArray]->PushCommand(	pContext,
														nCommand,
														lpBuffer,
														nLength);
	return true;
}

bool NetLib::cCommandQueueManager::PushCommand(	const UINT nID,
												const UINT nCommand,
												const BYTE* lpBuffer,
												const WORD nLength)
{
	if(m_nServerCommandArray < 0 || m_nServerCommandArray >= m_nCommandQueueCnt)
		return false;

	if(!m_pCommandQueue[m_nServerCommandArray])
		return false;

	m_pCommandQueue[m_nServerCommandArray]->PushCommand(	nID,
															nCommand,
															lpBuffer,
															nLength);
	return true;
}

void NetLib::cCommandQueueManager::BroadCastCommand(	const UINT nID,
														const UINT nCommand,
														const BYTE* lpBuffer,
														const WORD nLength)
{
	for (int n = 0; n<m_nCommandQueueCnt; ++n)
	{
		if(m_pCommandQueue[n])
			m_pCommandQueue[n]->PushCommand(nID, nCommand, lpBuffer, nLength);
	}
}

void NetLib::cCommandQueueManager::BroadCastCommandScheduleJob(	const UINT nID,
																const UINT nCommand,
																stSchedule* pSchedule,
																const WORD nLength)
{
	for (int n = 0; n<m_nCommandQueueCnt; ++n)
	{
		if(m_pCommandQueue[n])
		{
			pSchedule->uTargetCommandThreadIndex = n;
			m_pCommandQueue[n]->PushCommand(nID, nCommand, reinterpret_cast<BYTE*>(pSchedule), nLength);
		}
	}
}

void NetLib::cCommandQueueManager::RandomCastCommandScheduleJob(const UINT nID,
	const UINT nCommand,
	stSchedule* pSchedule,
	const WORD nLength)
{
	int random_command = rand() % m_nCommandQueueCnt;

	if (m_pCommandQueue[random_command])
	{
		pSchedule->uTargetCommandThreadIndex = random_command;
		m_pCommandQueue[random_command]->PushCommand(nID, nCommand, reinterpret_cast<BYTE*>(pSchedule), nLength);
	}
}

bool NetLib::cCommandQueueManager::PushCommand(	const int nRoomNo,
												const UINT nID,
												const UINT nCommand,
												const BYTE* lpBuffer,
												const WORD nLength)
{
	int commandqueueindex = nRoomNo % m_nCommandQueueCnt;

	if(commandqueueindex < 0 || commandqueueindex >= m_nCommandQueueCnt)
		return false;

	if(m_pCommandQueue[commandqueueindex])
		m_pCommandQueue[commandqueueindex]->PushCommand(nID, nCommand, lpBuffer, nLength);

	return true;
}

bool NetLib::cCommandQueueManager::PushCommand(	const NetLib::cUDPDispatcher* pUDPDispatcher,
												const UINT nCommand,
												const UINT Entity,
												const UINT uiPacketSeq,
												const NetLib::cUDPIocpOv* pUdpIocpOv,
												const BYTE* lpBuffer,
												const WORD nLength)
{
	// Context를 찾아서, CommandQueue를 선택해준다.
	NetLib::cSessionManager* pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance();
	NetLib::cSession* pSession = pSessionManager->Get(Entity);
	if(!pSession)
		return false;

	NetLib::cInterfaceIocpContext* pContext = pSession->GetContext();
	if(!pContext)
		return false;

#ifdef USING_MULTI_THREAD
	UINT nCommandQueueArray = pContext->GetCommandQueueIndex();
#else
	UINT nCommandQueueArray = 0;
#endif

	if(nCommandQueueArray < 0 || nCommandQueueArray >= (UINT)m_nCommandQueueCnt)
		return false;

	if(!m_pCommandQueue[nCommandQueueArray])
		return false;

	m_pCommandQueue[nCommandQueueArray]->PushCommand(	pUDPDispatcher,
														nCommand,
														Entity,
														uiPacketSeq,
														pUdpIocpOv,
														lpBuffer,
														nLength);
	return true;
}