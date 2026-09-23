#include "../../Include/Netlib/UdpModule/cUDPSessionManager.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"

NetLib::cUDPSessionManager::cUDPSessionManager() :
	m_iClientIndex(0)
{
}


NetLib::cUDPSessionManager::~cUDPSessionManager()
{
	Destroy();
}

/*
	ServerManager 가 생성이 완료가되고 
	ServerConfiguration에 값이 채워지고 난다음에 
	Init() 함수를 호출하면됩니다.
*/
void NetLib::cUDPSessionManager::Init()
{
	m_pos = m_pUDPSessionTable.GetStartPosition();

	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(!pServerManager)
	{
		::MessageBox(NULL, _T("cUDPSessionManager::cUDPSessionManager() No instance ServerManager"), _T("No instance ServerManager"), MB_ICONEXCLAMATION);
		return;
	}

	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if(!pConfig)
	{
		::OutputDebugString(_T("cUDPSessionManager::cUDPSessionManager() no Server Config"));
		::MessageBox(NULL, _T("cUDPSessionManager::cUDPSessionManager() no Server Config"), _T("No Server Config"), MB_ICONEXCLAMATION);
		return;
	}

	// Initialize the Hash Table
	m_pUDPSessionTable.InitHashTable(pConfig->wMaxUser);

	// Hash Table 데이터 초기화
	/*for (int n = 0; n<pConfig->wMaxUser; ++n)
	{
		m_pUDPSessionTable[n] = nullptr;
	}*/

	// Momory Pooler 생성
	m_pUDPSessionPooler = new NetLib::cMemPooler<NetLib::cUDPSession>;
}

void NetLib::cUDPSessionManager::Destroy()
{
	POSITION pos = m_pUDPSessionTable.GetStartPosition();
	NetLib::cUDPSession* pSession = nullptr;
	while (pos != nullptr)
	{
		pSession = m_pUDPSessionTable.GetValueAt(pos);
		if(pSession)
			delete pSession;

		m_pUDPSessionTable.GetNext(pos);
	}

	m_pUDPSessionTable.RemoveAll();

	if (m_pUDPSessionPooler != nullptr)
	{
		m_pUDPSessionPooler->DestroyPool();
		delete m_pUDPSessionPooler;
	}
}

//NetLib::cUDPSession* NetLib::cUDPSessionManager::AddSessionIPv4(SOCKADDR_IN* sockAddr, UINT entity)
NetLib::cUDPSession* NetLib::cUDPSessionManager::AddSessionIPv4(SOCKADDR_IN* sockAddr, const int64 user_idx)
{
	if (m_pUDPSessionPooler == nullptr)
	{
		assert(false && "cUDPSessionManager::AddSessionIPv4 Failed. UDPSessionPooler nullptr");
		return nullptr;
	}

	// 기존 Entity로 등록된 UDPSession 검색
	NetLib::cUDPSession* pUDPSession = nullptr;

	// 기존에 UDP세션이 존재 한다면 반환해주자.
	m_udpSessionTableLock.AcquireLockShared();

	ATL::CAtlMap<int64, NetLib::cUDPSession*>::CPair* pPair = m_pUDPSessionTable.Lookup(user_idx);
	if(pPair != nullptr)
		pUDPSession = pPair->m_value;

	m_udpSessionTableLock.ReleaseLockShared();

	// 세션이 존재 하지 않음으로, 신규로 등록해준다.
	if(pUDPSession == nullptr)
		pUDPSession = m_pUDPSessionPooler->Pop();

	if(!pUDPSession)
	{
		// UDPSession 고갈
		return nullptr;
	}

	pUDPSession->Clear();
	pUDPSession->SetAddrIPv4(sockAddr);

	m_udpSessionTableLock.AcquireLockExclusive();

	m_pUDPSessionTable.SetAt(user_idx, pUDPSession);

	m_udpSessionTableLock.ReleaseLockExclusive();
	return pUDPSession;
}

//NetLib::cUDPSession* NetLib::cUDPSessionManager::AddSessionIPv6(SOCKADDR_IN6* sockAddr, UINT entity)
//{
//	if (m_pUDPSessionPooler == nullptr)
//	{
//		assert(false && "cUDPSessionManager::AddSessionIPv6 Failed. UDPSessionPooler nullptr");
//		return nullptr;
//	}
//
//	// 기존 Entity로 등록된 UDPSession 검색
//	NetLib::cUDPSession* pUDPSession = NULL;
//
//	// 기존에 UDP세션이 존재 한다면 반환해주자.
//	ATL::CAtlMap<UINT, NetLib::cUDPSession*>::CPair* pPair = m_pUDPSessionTable.Lookup(entity);
//	if(pPair != NULL)
//		pUDPSession = pPair->m_value;
//
//	// 세션이 존재 하지 않음으로, 신규로 등록해준다.
//	if(pUDPSession == nullptr)
//		pUDPSession = m_pUDPSessionPooler->Pop();
//
//	if(!pUDPSession)
//	{
//		// UDPSession 고갈
//		return NULL;
//	}
//
//	pUDPSession->Clear();
//	pUDPSession->SetAddrIPv6(sockAddr);
//	m_pUDPSessionTable.SetAt(entity, pUDPSession);
//	return pUDPSession;
//}

BOOL NetLib::cUDPSessionManager::RemoveAndPushSessionByKey(const int64 user_idx)
{
	if (m_pUDPSessionPooler == nullptr)
	{
		assert(false && "cUDPSessionManager::RemoveAndPushSessionByKey Failed. UDPSessionPooler nullptr");
		return FALSE;
	}

	m_udpSessionTableLock.AcquireLockShared();

	NetLib::cUDPSession* pUDPSession = nullptr;
	ATL::CAtlMap<int64, NetLib::cUDPSession*>::CPair* pPair = m_pUDPSessionTable.Lookup(user_idx);

	m_udpSessionTableLock.ReleaseLockShared();

	if(pPair == nullptr)
		return FALSE;

	pUDPSession = pPair->m_value;

	if(pUDPSession)
	{
		m_udpSessionTableLock.AcquireLockExclusive();

		m_pUDPSessionTable.RemoveKey(user_idx);

		m_udpSessionTableLock.ReleaseLockExclusive();

		m_pUDPSessionPooler->Push(pUDPSession);
	}

	return true;
}

size_t NetLib::cUDPSessionManager::GetTotalUser()
{
	return m_pUDPSessionTable.GetCount();
}

// 비정상 세션 삭제용 타임값
#define MAX_UDP_ERROR_SESSION_INTERVAL 10000
//////////////////////////////////////////////////////////////////////////
// 여기서 임계시간보다 많은 시간이 지나버린 세션을 삭제하도록 한다.
bool NetLib::cUDPSessionManager::DeleteTimeOutSession()
{
	ULONGLONG ullCurrentTime = 0;
	ullCurrentTime = GetTickCount64();

	POSITION				pos = m_pUDPSessionTable.GetStartPosition();
	NetLib::cUDPSession*			pSession;
	SessionTable::CPair*	pair;

	bool isDelete = false;
	while (pos != nullptr)
	{
		pair = m_pUDPSessionTable.GetNext(pos);
		pSession = pair->m_value;
		if((ullCurrentTime - pSession->GetTime() > MAX_UDP_ERROR_SESSION_INTERVAL))
		{
			// Map에서 삭제한다.
			RemoveAndPushSessionByKey(pair->m_key);

			isDelete = true;
		}
	}

	return isDelete;
}

NetLib::cUDPSession* NetLib::cUDPSessionManager::GetSessionByKey(UINT Entity)
{
	NetLib::cUDPSession* pSession = nullptr;
	SessionTable::CPair* pair = nullptr;

	// 키로 CPair객체를 찾는다.
	pair = m_pUDPSessionTable.Lookup(Entity);
	if(pair == nullptr)
	{
		return nullptr;
	}

	pSession = pair->m_value;

	return pSession;
}