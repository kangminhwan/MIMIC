#pragma once
#include "../Common/Netlib.h"
#include "cUDPSession.h"

BEGIN_NETLIB

class cUDPSession;
class cUDPSessionManager
{
public:
	typedef CAtlMap<int64, cUDPSession*>		SessionTable;
private:
	SessionTable	m_pUDPSessionTable;
	int				m_iClientIndex;
	POSITION		m_pos;
	cSRWLock_CriticalSection m_udpSessionTableLock;

	// stl queue
	cMemPooler<cUDPSession>*		m_pUDPSessionPooler;

public:
	void Init();
	void Destroy();

	//cUDPSession* AddSessionIPv4(SOCKADDR_IN* sockAddr, UINT entity);
	cUDPSession* AddSessionIPv4(SOCKADDR_IN* sockAddr, const int64 user_idx);
	//cUDPSession* AddSessionIPv6(SOCKADDR_IN6* sockAddr, UINT entity);

	BOOL RemoveAndPushSessionByKey(const int64 user_idx);

	// 세션 맵 가져오기
	SessionTable* GetSessionTable() { return &m_pUDPSessionTable; }

	// MemPool에 Push..
	void PushSession(cUDPSession* pSession)
	{
		if (m_pUDPSessionPooler == nullptr)
		{
			assert(false && "cUDPSession::PushSession is Failed. UDPSessionPooler is nullptr");
			return;
		}

		m_pUDPSessionPooler->Push(pSession);
	}

	// MemPool에서 Pop..
	cUDPSession* PopSession()
	{
		if (m_pUDPSessionPooler == nullptr)
		{
			assert(false && "cUDPSession::PushSession is Failed. UDPSessionPooler is nullptr");
			return nullptr;
		}

		return m_pUDPSessionPooler->Pop();
	}

	size_t GetRemainUDPSessionCount()
	{
		if (m_pUDPSessionPooler == nullptr)
		{
			return 0;
		}

		return m_pUDPSessionPooler->GetRemainPoolCnt();
	}

	cUDPSession* GetSessionByKey(UINT Entity);
	size_t GetTotalUser();
	bool DeleteTimeOutSession();

public:
	cUDPSessionManager();
	~cUDPSessionManager();
};

END_NETLIB