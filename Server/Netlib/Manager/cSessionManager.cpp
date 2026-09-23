#include "../../Include/Netlib/Manager/cSessionManager.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../../Include/Netlib/Session/cSession.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"

NetLib::cSessionManager::cSessionManager()
{
}


NetLib::cSessionManager::~cSessionManager()
{
	Destroy();
}

/*
	ServerManager 가 생성이되고
	ServerConfiguration의 값이 채워진다음에 콜을 하면 됩니다.
*/
void NetLib::cSessionManager::Init()
{
	//m_pSessionTable = new CAtlMap<UINT,cSession*>;
	//m_pSessionTable = new std::vector<cSession*>;

	SetRejectSession(FALSE);

	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(!pServerManager)
	{
		::OutputDebugString(_T("cSessionManager::ServerManager has no Instance"));
		::MessageBox(NULL, _T("cSessionManager::ServerManager has no Instance"), _T("No cSessionManager Instance"), MB_ICONEXCLAMATION);
		return;
	}

	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if(!pConfig)
	{
		::OutputDebugString(_T("cSessionManager::Init() no Server Config"));
		::MessageBox(NULL, _T("cSessionManager::Init() no Server Config"), _T("No Server Config"), MB_ICONEXCLAMATION);
		return;
	}

	m_uiMaxUserLimit = pConfig->wMaxUser;
	m_uiBackLogCnt = pConfig->wBackLog;
	m_uiSocketPoolSize = pConfig->nSocketPoolSize;

	// 멀리쓰레드용일경우는 FALSE를 TRUE로 바꿔줄것.
	//m_pSessionPooler = new cMemPooler<cSession>( pConfig->wMaxUser, pConfig->wMaxUser, FALSE );
	m_pSessionPooler = new NetLib::cMemPooler<NetLib::cSession>(0, 0, 0, TRUE);// APP에서 cSession타입으로 생성해서 넣어준다.

	// wMaxUser Size만큼 미리 NULL로 할당해 놓는다.
	m_SessionTable.InitHashTable(pConfig->nSocketPoolSize);

	// rehash 방지 코드 CAtlMap의 생성자에서 가지고 왔다.
	// 생성된 인자수가 fLoThreshold값 보다 작으면, freenode시에 rehash가 일어나게 되어
	// 원치 않는 상황이 일어날수 있따.
	/*
	CAtlMap(
	_In_ UINT nBins = 17,
	_In_ float fOptimalLoad = 0.75f,
	_In_ float fLoThreshold = 0.25f,
	_In_ float fHiThreshold = 2.25f,
	_In_ UINT nBlockSize = 10) throw();
	*/
	m_SessionTable.SetOptimalLoad(0.75f, 0.0f, 2.25f, false);
	// Entity는 1부터 시작하니깐.... 1개더 확보 해 놓자 ㅡㅡ;
	//for(int n=0; n<pConfig->nSocketPoolSize + 1; ++n)
	// wMaxUser + BackLog + 10개로 잡아 놓은 nSocketPoolSize  + 알파 만큼 확보해 놓자. 
	/*for(int n=0; n<m_uiSocketPoolSize + 1; ++n)
	m_SessionTable.push_back(NULL);*/
}

void NetLib::cSessionManager::Destroy()
{
	if(m_pSessionPooler)
	{
		delete m_pSessionPooler;
		m_pSessionPooler = NULL;
	}

	POSITION PendingPos = m_PendingSessionTable.GetStartPosition();
	while (PendingPos != nullptr)
	{
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPendingPair = m_PendingSessionTable.GetAt(PendingPos);
		if (pPendingPair != nullptr)
		{
			NetLib::cSession* pPendingSession = pPendingPair->m_value;
			if (pPendingSession != nullptr)
			{
				delete pPendingSession;
				pPendingPair->m_value = nullptr;
			}
		}

		m_PendingSessionTable.GetNext(PendingPos);
	}

	m_PendingSessionTable.RemoveAll();

	POSITION pos = m_SessionTable.GetStartPosition();

	while (pos != nullptr)
	{
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_SessionTable.GetAt(pos);
		if(pPair != nullptr)
		{
			NetLib::cSession* pSession = pPair->m_value;
			if(pSession != nullptr)
			{
				delete pSession;
				pPair->m_value = nullptr;
			}
		}

		m_SessionTable.GetNext(pos);
	}

	m_SessionTable.RemoveAll();
}

//////////////////////////////////////////////////////////////////////
// Operation
//////////////////////////////////////////////////////////////////////
void NetLib::cSessionManager::ClearSession()
{
	NetLib::cUnionLock lock(&m_Session_Lock, TRUE);
	NetLib::cSession* pSession = NULL;
	POSITION TablePos = m_SessionTable.GetStartPosition();
	while (TablePos)
	{
		POSITION nextPos = TablePos;
		m_SessionTable.GetNext(nextPos); // 미리 다음 포지션을 가져옴

		pSession = m_SessionTable.GetValueAt(TablePos);
		if (pSession)
		{
			/*if (E_SESSION_STATUS::E_SESSION_STATUS_NONE == pSession->GetSessionStatus())
			{
				m_pSessionPooler->PushFront(pSession);
				m_SessionTable.RemoveAtPos(TablePos);
			}*/
		}
		else
		{
			m_pSessionPooler->PushFront(pSession);
			m_SessionTable.RemoveAtPos(TablePos);
		}
		TablePos = nextPos; // 다음 포지션으로 이동
	}
}


NetLib::cSession* NetLib::cSessionManager::AllocateSession(int64 playerIdx, NetLib::cInterfaceIocpContext* pContext)
{
	NetLib::cUnionLock lock(&m_Session_Lock, FALSE);
	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_SessionTable.Lookup(playerIdx);
	if (pPair == nullptr)
	{
		NetLib::cSession* pSession = m_pSessionPooler->Pop();
		if (pSession)
		{
			pSession->Clear();
			pSession->SetContext(pContext);
			pSession->SetAllocatedSessionTableSlot(playerIdx);
			m_SessionTable.SetAt(playerIdx, pSession);
			//m_SessionTable[Entity] = pSession;
			pContext->SetCurrentPacketTick();
			// Context 에 세션 연결
			pContext->SetSession( pSession );

			return pSession;
		}
	}
	else
	{
		if (pPair->m_value != nullptr) {

			// TODO 이곳에 걸리는 경우는 찾아야함
			// Session 에서 컨텍스트를 가지고 있지 않아서 문제가 생김
			pPair->m_value->SetContext(pContext);
			pContext->SetSession(pPair->m_value);
			pContext->SetCurrentPacketTick();
			return pPair->m_value;
		}
		else {

			// 이곳에 들어온 경우에는 세션테이블에 등록된 세션이 nullptr 인 경우임..
			return nullptr;
		}
	}
}

void NetLib::cSessionManager::GetSessionCountBySessionType(UINT& serversessions, UINT& clientsessions, UINT& unknownsessions, UINT& agentsessions, UINT& toolsessions)
{
	NetLib::cUnionLock lock(&m_Session_Lock, TRUE);

	NetLib::cSession* pSession = NULL;
	POSITION TablePos = m_SessionTable.GetStartPosition();
	while (TablePos)
	{
		pSession = m_SessionTable.GetValueAt(TablePos);
		if(pSession)
		{
			switch (pSession->GetSessionType())
			{
			case Sessions::SESSION_CLIENT:
				++clientsessions;
				break;
			case Sessions::SESSION_SERVER:
				++serversessions;
				break;
			case Sessions::SESSION_AGENT:
				++agentsessions;
				break;
			case Sessions::SESSION_TOOL:
				++toolsessions;
				break;
			case Sessions::SESSION_NONE:
			default:
				++unknownsessions;
				break;

			}
		}

		m_SessionTable.GetNext(TablePos);
	}
}

/*
이 함수는 m_SessionTable의 Lock을 오래 붙잡지 않기 위해, 100개를 세고 나서는 스위칭을 시켜 줍니다.
*/
void NetLib::cSessionManager::GetSessionCountBySessionTypeUpgrade(UINT& serversessions, UINT& clientsessions, UINT& unknownsessions, UINT& agentsessions, UINT& toolsessions)
{
	NetLib::cSession* pSession;

	int nRetryCount = 0;
	BOOL bRetry = FALSE;

	while (nRetryCount < 3 && bRetry == FALSE)
	{
		try
		{
			pSession = NULL;

			POSITION TablePos = m_SessionTable.GetStartPosition();

			if (TablePos == nullptr)
				return;

			int64 key = m_SessionTable.GetKeyAt(TablePos);

			while (TablePos)
			{
				{
					NetLib::cUnionLock Lock(&m_Session_Lock, TRUE);

					TablePos = SessionCountLoop(key, serversessions, clientsessions, unknownsessions, agentsessions, toolsessions);

					if (TablePos)
						key = m_SessionTable.GetKeyAt(TablePos);
					else
						key = 0;
				}

				// 스위칭을 강제로 발생시켜 줍니다.
				// Ready to run 되어 있는 Thread 가 지금 선점한 Thread 와 우선순위가 같아야 선점을 푼다. 
				// 이 얘기는 이펑션을 처리하는 commandthread간에 스위칭을 발생시키겠다는 뜻입니다.
				Sleep(0);
			}

			bRetry = TRUE;
		}
		catch (CAtlException &e)
		{
			++nRetryCount;

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, 
				"NetLib::cSessionManager::GetSessionCountBySessionTypeUpgrade failed. CAtlException nRetryCount[ %d ]", nRetryCount);
		}
	}

	// 3번 시도 해도 실패면 예전 방식으로 돌립니다.
	if (bRetry == FALSE)
	{
		serversessions = 0;
		clientsessions = 0;
		unknownsessions = 0;
		agentsessions = 0;
		toolsessions = 0;

		GetSessionCountBySessionType(serversessions, clientsessions, unknownsessions, agentsessions, toolsessions);
	}
}

POSITION NetLib::cSessionManager::SessionCountLoop(int64 key, UINT& serversessions, UINT& clientsessions, UINT& unknownsessions, UINT& agentsessions, UINT& toolsessions)
{
	UINT countingsessions = 0;

	// key로 시작할 포인트를 찾습니다.
	POSITION startpos = m_SessionTable.Lookup(key);

	if (key && startpos == nullptr)
	{
		// 이곳에 들어 왔다는 것은 해당 key에 있던 데이터가 없어 졌다는 뜻입니다.
		// 다른 쓰레드에서 지워 버렸을 가능성이 있습니다.
		// 처음부터 다시 셀지....를 결정해야 하는데...
		throw CAtlException(E_FAIL);
	}

	while (startpos && countingsessions < 100)
	{
		NetLib::cSession* pSession = m_SessionTable.GetValueAt(startpos);
		if (pSession == nullptr)
		{
			// 이곳에 들어 왔다는 것은 해당 POSITION에 있던 데이터가 없어 졌다는 뜻입니다.
			// 다른 쓰레드에서 지워 버렸을 가능성이 있습니다.
			// 처음부터 다시 셀지....를 결정해야 하는데...
			++countingsessions;
			startpos = nullptr;// 루프를 강제로 종료 시키기 위해..
			continue;
		}

		if (pSession)
		{
			switch (pSession->GetSessionType())
			{
			case Sessions::SESSION_CLIENT:
				++clientsessions;
				break;
			case Sessions::SESSION_SERVER:
				++serversessions;
				break;
			case Sessions::SESSION_AGENT:
				++agentsessions;
				break;
			case Sessions::SESSION_TOOL:
				++toolsessions;
				break;
			case Sessions::SESSION_NONE:
			default:
				++unknownsessions;
				break;

			}

			++countingsessions;
		}

		m_SessionTable.GetNext(startpos);
	}

	return startpos;
}

/*
 * Remove는 SessionTable에 있는 Session을 Pooler에 옮겨줍니다.
*/
bool NetLib::cSessionManager::Remove(int64 playerIdx)
{
	NetLib::cUnionLock lock(&m_Session_Lock, FALSE);

	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_SessionTable.Lookup(playerIdx);
	if(pPair == nullptr)
		return false;

	NetLib::cSession* pSession = pPair->m_value;

	// 세션테이블에서 삭제
	m_SessionTable.RemoveAtPos(pPair);

	if (pSession == nullptr)
		return false;

	UINT Entity = 0;
	auto pContext = pSession->GetContext();
	if (pContext != nullptr)
		Entity = pContext->GetEntity();

	pSession->SessionLogout(Entity);
	pSession->Init();

#ifdef USE_PUSH_FRONT_MEMPOOLER
	m_pSessionPooler->PushFront(pSession);
#else
	m_pSessionPooler->Push(pSession);
#endif
	return true;
}

void NetLib::cSessionManager::RemoveOnly(int64 playerIdx)
{
	NetLib::cUnionLock lock(&m_Session_Lock, FALSE);

	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_SessionTable.Lookup(playerIdx);
	if (pPair == nullptr)
		return;

	NetLib::cSession* pSession = pPair->m_value;

	m_SessionTable.RemoveKey(playerIdx);
	if (pSession == nullptr)
		return;

	pSession->Init();

	m_pSessionPooler->Push(pSession);
}
/*
* RemovePending는 m_PendingSessionTable에 있는 Session을 Pooler에 옮겨줍니다.
*/
bool NetLib::cSessionManager::RemovePending(int64 playerIdx)
{
	NetLib::cUnionLock lock(&m_Pending_Lock, FALSE);

	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
	if (pPair == nullptr)
		return false;

	NetLib::cSession* pSession = pPair->m_value;
	if (pSession == nullptr)
		return false;

	if (m_PendingSessionTable.RemoveKey(playerIdx) == FALSE)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cSessionManager::Remove failed. SessionTable.RemoveKey failed. AID [ %I64d ]", playerIdx);
	}
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "RemovePending Key :  [ %I64d ]", playerIdx);


	UINT Entity = 0;
	auto pContext = pSession->GetContext();
	if ( pContext != nullptr )
		Entity = pContext->GetEntity();

	pSession->SessionLogout(Entity);
	pSession->Init();

#ifdef USE_PUSH_FRONT_MEMPOOLER
	m_pSessionPooler->PushFront(pSession);
#else
	m_pSessionPooler->Push(pSession);
#endif
	return true;
}

//딴거안하고 그냥 바로 옮겨버리기
bool NetLib::cSessionManager::PendingToPooler(int64 playerIdx)
{
	NetLib::cUnionLock lock(&m_Pending_Lock, FALSE);

	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
	if (pPair == nullptr)
		return false;

	NetLib::cSession* pSession = pPair->m_value;
	if (pSession == nullptr)
		return false;

	if (m_PendingSessionTable.RemoveKey(playerIdx) == FALSE)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cSessionManager::Remove failed. SessionTable.RemoveKey failed. AID [ %I64d ]", playerIdx);
	}
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "RemovePending Key :  [ %I64d ]", playerIdx);


#ifdef USE_PUSH_FRONT_MEMPOOLER
	m_pSessionPooler->PushFront(pSession);
#else
	m_pSessionPooler->Push(pSession);
#endif
	return true;
}

NetLib::cSession* NetLib::cSessionManager::Get(int64 playerIdx)
{
	NetLib::cUnionLock Lock(&m_Session_Lock, TRUE);
	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_SessionTable.Lookup(playerIdx);
	if(pPair == nullptr)
		return NULL;

	return pPair->m_value;
}

NetLib::cSession* NetLib::cSessionManager::GetPending(int64 playerIdx)
{
	NetLib::cUnionLock Lock(&m_Pending_Lock, TRUE);
	ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
	if (pPair == nullptr)
		return NULL;

	return pPair->m_value;
}

bool NetLib::cSessionManager::PushPendingSession(int64 playerIdx)
{
	NetLib::cSession* pSession = nullptr;
	{
		NetLib::cUnionLock SessionTableLock(&m_Session_Lock, FALSE);
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_SessionTable.Lookup(playerIdx);

		// 여기에서 리턴한다는 것은, 접속을 종료 할때 세션 테이블에 세션이 없다는 얘기 이며, 로직에 문제가 있을 확률이 높다.
		// Debug 상태일때는 assert 로 처리해 둔다.
		if ( pPair == nullptr ) {
#ifdef _DEBUG
			//Assert( FALSE , _T( "cSessionManager::PushPendingSession Finding Session from SessionTable Failed" ) );

			return false;
			// 로그 출력

#else
			return false;
#endif
		}

		pSession = pPair->m_value;

		m_SessionTable.RemoveKey(playerIdx);

	}

	if (pSession == nullptr)
	{
		return false;
	}

	{
		NetLib::cUnionLock SessionPendingTableLock(&m_Pending_Lock, FALSE);
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
		if (pPair != nullptr)
		{
			return false;
		}

		// 세션의 상태를 변경해줍니다.
		if(pSession->GetContext() != nullptr)
			pSession->GetContext()->Disconnect();
		pSession->SetSessionStatus(E_SESSION_STATUS::E_SESSION_STATUS_PENDING);
		pSession->SetContext(nullptr);
		pSession->SetPendingTime();

		POSITION pos = m_PendingSessionTable.SetAt(playerIdx, pSession);
		if (pos == nullptr)
		{
			return false;
		}
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cSessionManager::PushPendingSession Success. PlayerIdx[ %I64d ]", playerIdx);

	return true;
}

NetLib::cSession* NetLib::cSessionManager::GetPendingSessionAndSyncContext(int64 playerIdx, NetLib::cInterfaceIocpContext* pContext)
{
	if ( pContext == nullptr )
		throw new std::exception( "cSessionManager::GetPendingSession() pContext is nullptr. dont do this please" );

	NetLib::cSession* pSession = nullptr;
	{
		NetLib::cUnionLock SessionPendingTableLock(&m_Pending_Lock, FALSE);
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
		if (pPair == nullptr ||
			pPair->m_value->GetSessionStatus() == E_SESSION_STATUS::E_SESSION_STATUS_DISCONNECTING)
		{
			return nullptr;
		}

		pSession = pPair->m_value;

		m_PendingSessionTable.RemoveAtPos(pPair);
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "removedpending2");

	}

	if (pSession == nullptr)
		return nullptr;

	// 세션의 상태를 변경해 줍니다.
	// 펜딩 세션을 찾은거라서 Context의 CommandQueueIndex를 세션이 알고 있는 CommandQueueIndex로 변경합니다.
	pSession->SetContext( pContext );
	pSession->SetSessionStatus( E_SESSION_STATUS::E_SESSION_STATUS_CONNECTED );

	pContext->SetSession( pSession );
	//pContext->SetCommandQueueIndex(pSession->GetCommandQueueIndex());
	pContext->SetCurrentPacketTick(); // 세션을 찾자마자, TickCount설정을 바로 해주지 않아서 튕기는 현상이 있습니다.

	NetLib::cUnionLock SessionTableLock(&m_Session_Lock, FALSE);

	// 세션 테이블로 옮겨줍니다.
	m_SessionTable.SetAt(playerIdx, pSession);

	return pSession;
}

NetLib::cSession* NetLib::cSessionManager::GetPendingSession(int64 playerIdx)
{
	NetLib::cSession* pSession = nullptr;
	{
		NetLib::cUnionLock SessionPendingTableLock(&m_Pending_Lock, FALSE);
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
		if (pPair == nullptr)
			return nullptr;

		pSession = pPair->m_value;
	}

	return pSession;
}

void NetLib::cSessionManager::CheckPendingSession()
{
	NetLib::cUnionLock SessionPendingTableLock(&m_Pending_Lock, TRUE);

	ULONGLONG _ullCheckTime = ::GetTickCount64();

	POSITION pos = m_PendingSessionTable.GetStartPosition();
	while (pos)
	{
		NetLib::cSession* pSession = m_PendingSessionTable.GetValueAt(pos);
		if (pSession != nullptr) {

			// 클라이언트만 처리 합니다.
			if (pSession->GetSessionType() == Sessions::SESSION_CLIENT) {

				// isRemoveReady 가 true 를 리턴하지 않으면 지우지 않습니다.
				if (pSession->isRemoveReady()) {

					if ((_ullCheckTime - pSession->GetPendingTime()) >= CSDef::E_TIMER_INTERVAL::E_IOS_PENDING_DELAY_TIME) {
						pSession->SessionLogout(0);
						// 연결 해제 중이라는 Flag를 답니다.
						pSession->SetSessionStatus(E_SESSION_STATUS::E_SESSION_STATUS_DISCONNECTING);
						Sys_Net_Session_Log_Out_Data data;
						data.llAllocatedSessionSlot = pSession->GetAllocatedSessionTableSlot();

						// 이 부분에서 세션 로그 아웃 해줍니다.
						NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr(pSession->GetCommandQueueIndex());
						if (pCommandQueue)
							pCommandQueue->PushCommand(static_cast<UINT>(0), CSNet::ProtocolCommand::SYS_NET_SESSION_LOG_OUT, reinterpret_cast<BYTE*>(&data), sizeof(Sys_Net_Session_Log_Out_Data));
						else
							NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->PushCommand(static_cast<UINT>(0), CSNet::ProtocolCommand::SYS_NET_SESSION_LOG_OUT, reinterpret_cast<BYTE*>(&data), sizeof(Sys_Net_Session_Log_Out_Data));
					}
				}
				/*else
				{
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cSessionManager::CheckPendingSession isRemoveReady false.");
				}*/
			}
			else {


				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cSessionManager::CheckPendingSession Connected Server Exists.");
			}
		}

		m_PendingSessionTable.GetNext(pos);
	}
}

std::vector<NetLib::cSession*> NetLib::cSessionManager::GetSessions()
{
	std::vector<NetLib::cSession*> sessions;
	POSITION pos = m_SessionTable.GetStartPosition();
	while (pos != nullptr) {
		int64_t key;
		NetLib::cSession* value;
		m_SessionTable.GetNextAssoc(pos, key, value);
		sessions.push_back(value);
	}
	return sessions;
}

NetLib::cSession* NetLib::cSessionManager::GetReLoginSessionAndSyncContext(int64 playerIdx, NetLib::cInterfaceIocpContext* pContext)
{
	if (pContext == nullptr)
		throw new std::exception("cSessionManager::GetPendingSession() pContext is nullptr. dont do this please");
	NetLib::cSession* pSession = nullptr;
	{
		NetLib::cUnionLock SessionPendingTableLock(&m_Pending_Lock, FALSE);
		ATL::CAtlMap<int64, NetLib::cSession*>::CPair* pPair = m_PendingSessionTable.Lookup(playerIdx);
		if (pPair == nullptr)
		{
			NetLib::cUnionLock lock(&m_Session_Lock, FALSE);
			pPair = m_SessionTable.Lookup(playerIdx);
			if (pPair == nullptr)
			{
				return nullptr;
			}
			else
			{
				if (pPair->m_value == nullptr)
				{
					return nullptr;
				}
				pSession = pPair->m_value;
				if (pSession->GetContext() != nullptr)
				{
					pSession->GetContext()->SetSession(nullptr);
					pSession->GetContext()->Disconnect();
				}
				m_SessionTable.RemoveKey(playerIdx);
			}
		}
		else
		{
			pSession = pPair->m_value;
			m_PendingSessionTable.RemoveKey(playerIdx);
		}
	}

	if (pSession == nullptr)
		return nullptr;

	// 세션의 상태를 변경해 줍니다.
	// 펜딩 세션을 찾은거라서 Context의 CommandQueueIndex를 세션이 알고 있는 CommandQueueIndex로 변경합니다.
	pSession->SetContext(pContext);
	pSession->SetSessionStatus(E_SESSION_STATUS::E_SESSION_STATUS_CONNECTED);

	pContext->SetSession(pSession);
	//pContext->SetCommandQueueIndex(pSession->GetCommandQueueIndex());
	pContext->SetCurrentPacketTick(); // 세션을 찾자마자, TickCount설정을 바로 해주지 않아서 튕기는 현상이 있습니다.

	NetLib::cUnionLock SessionTableLock(&m_Session_Lock, FALSE);

	// 세션 테이블로 옮겨줍니다.
	m_SessionTable.SetAt(playerIdx, pSession);

	return pSession;
}