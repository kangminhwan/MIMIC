#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cInterfaceIocpContext;
class cSession;
class cSessionManager
{
public:
	// atl map
	cSRWLock_CriticalSection m_Pending_Lock;
	ATL::CAtlMap<int64, cSession*> m_PendingSessionTable;

	//std::vector<cSession*> m_SessionTable;
	ATL::CAtlMap<int64, cSession*> m_SessionTable;
	cSRWLock_CriticalSection m_Session_Lock;

private:
	std::vector<cSession*> m_SessionVec;
	UINT		m_uicurrent_Key;
	UINT		m_uiMaxUserLimit;
	UINT		m_uiBackLogCnt;
	UINT		m_uiSocketPoolSize;
	BOOL		m_bRejectSession;//연결된 세션 reject!!

protected:
	// stl queue
	cMemPooler<cSession>* m_pSessionPooler;

public:
	ATL::CAtlMap<__int64, int> m_DamagePacketCaptureOn;// 릴리즈 버젼에서는 이놈으로 켜있는지 확인한다. 확인뒤에는 바로 꺼버린다.

#ifdef USE_LOGIN_WAIT_QUEUE
public:
	void InsertLoginQueue(cInterfaceIocpContext* pContext) { m_LoginWaitQueue.push_back(pContext); }
	int GetLoginQueueSize() { return m_LoginWaitQueue.size(); }

protected:
	std::vector<cInterfaceIocpContext*> m_LoginWaitQueue;
	cCriticalSection m_LoginCriticalSection;
#endif

public:
	/*
	ServerManager 가 생성이되고
	ServerConfiguration의 값이 채워진다음에 콜을 하면 됩니다.
	*/
	void Init();

	void Destroy();

public:
	void ClearSession();
	cSession* AllocateSession(int64 playerIdx, cInterfaceIocpContext* pContext);
	bool Remove(int64 playerIdx);
	void RemoveOnly(int64 playerIdx);

	size_t GetSessionCount()
	{
		return m_SessionTable.GetCount();
	}

	void GetSessionCountBySessionType(UINT& serversessions, UINT& clientsessions, UINT& unknownsessions, UINT& agentsessions, UINT& toolsessions);
	void GetSessionCountBySessionTypeUpgrade(UINT& serversessions, UINT& clientsessions, UINT& unknownsessions, UINT& agentsessions, UINT& toolsessions);
	POSITION SessionCountLoop(int64 key, UINT& serversessions, UINT& clientsessions, UINT& unknownsessions, UINT& agentsessions, UINT& toolsessions);

	size_t GetPoolerSize()
	{
		if(m_pSessionPooler)
			return m_pSessionPooler->GetRemainPoolCnt();

		return 0;
	}

	cSession* Get(int64 playerIdx);
	cSession* GetPending(int64 playerIdx);

	UINT	GetMaxUser() { return m_uiMaxUserLimit; }

	void PushSession(cSession* pSession)
	{
		m_pSessionPooler->Push(pSession);
	}

	cSession* PopSession()
	{
		return m_pSessionPooler->Pop();
	}

	ATL::CAtlMap<int64, cSession*>* GetSessionTable()
	{
		return &m_SessionTable;
	}

	void SetRejectSession(BOOL bReject)
	{
		m_bRejectSession = bReject;
	}

	const BOOL IsRejectSession()
	{
		return m_bRejectSession;
	}

public:
	bool PushPendingSession(int64 playerIdx);
	bool RemovePending(int64 playerIdx); //pending 찾아서 로그아웃하고 날리기
	bool PendingToPooler(int64 playerIdx); //펜딩세션 풀러로 보내기
	cSession* GetPendingSessionAndSyncContext(int64 playerIdx, NetLib::cInterfaceIocpContext* pContext);	// Pending 세션을 찾고 Context 와 연결한다.
	cSession* GetReLoginSessionAndSyncContext(int64 playerIdx, NetLib::cInterfaceIocpContext* pContext);	// 그전 세션을 찾고 Context 와 연결한다.
	cSession* GetPendingSession(int64 playerIdx);
	void CheckPendingSession();
	int GetPendingSessionCount() { return m_PendingSessionTable.GetCount(); }
	std::vector<NetLib::cSession*> GetSessions();

public:
	cSessionManager();
	~cSessionManager();
};

END_NETLIB