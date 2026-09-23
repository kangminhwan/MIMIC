#pragma once
#include "../Common/Netlib.h"
#include "../UdpModule/cUDPSession.h"

BEGIN_NETLIB

class cInterfaceIocpContext;

class cSession
{
protected:
	cInterfaceIocpContext* m_pContext;
	__int64 m_allocatedslot;				// 서버 타입 세션일경우, 서버상에 map파일위치
	UINT m_port;							// 해당 서버의 접속포트
	TCHAR m_ipaddress[CSDef::MAX_IP_ADDRESS_LEN];					// IP Address
	ConnectorInfo* m_pConnectionInfo;

	int64 m_llalollocatedTableslot;			// 세션 테이블의 위치
	Sessions m_eSessionType;
	E_SERVER_TYPE m_eServerType;

	E_SESSION_STATUS m_eSessionStatus;

#ifndef USE_PACKET_TICK_CHECK_CONTEXT
	ULONGLONG m_ullLastPacketTick;
#endif

	UINT m_uCommandQueueIndex; // 이 세션이 속해 있는 커맨드 인덱스

	ULONGLONG m_ullCennection; // 접속 시간 설정
	ULONGLONG m_ullPendingTime; // Pending 시간 설정

	// ManagerServer Only
	__int64 m_ServerID;//SID
	int m_ServerGroupID;//GID

public:
	virtual void Init();
	virtual void Clear();
	virtual void SessionLogout(UINT Entity, BOOL bForce = 0) {}
	virtual void DisConnectContext(UINT Entity, UINT nThreadIndex) {} // <= ContextPooler에서도 호출을 해줍니다. 무조건 구현을 해줘야합니다.
	virtual void DisConnectContextImmediately(UINT Entity, UINT nThreadIndex) {}
	virtual void SessionReConnect(UINT Entity) {}
	virtual size_t CodedSessionInfo(char* pCodedBuffer, size_t nBufferSize);
	virtual bool isRemoveReady() { return true; }
	void Destroy();

public:
	inline void SetContext(cInterfaceIocpContext* pContext) { m_pContext = pContext; }
	inline cInterfaceIocpContext* GetContext() { return m_pContext; }

	inline void SetCommandQueueIndex(UINT uCommandQueueIndex) { m_uCommandQueueIndex = uCommandQueueIndex; }
	inline UINT GetCommandQueueIndex() { return m_uCommandQueueIndex; }

	// 세션 타입 설정
	inline void SetSessionType(const Sessions session) { m_eSessionType = session; }
	inline Sessions GetSessionType() { return m_eSessionType; }

	// 서버 타입 설정, 서버일 경우만 설정한다.
	inline void SetServerType(const E_SERVER_TYPE type) {
		m_eServerType = type;
	}

	inline E_SERVER_TYPE GetServerType() {
		return m_eServerType;
	}

	inline void SetServerID_And_ServerGroupID(const __int64 servereid, const int groupid) {
		m_ServerID = servereid;
		m_ServerGroupID = groupid;
	}

	inline int GetServerGroupID() { return m_ServerGroupID; }
	inline __int64 GetServerID() { return m_ServerID; }
	inline void SetAllocatedSessionTableSlot(const int64 allocatedSlot) { m_llalollocatedTableslot = allocatedSlot; }
	inline int64 GetAllocatedSessionTableSlot() const { return m_llalollocatedTableslot; }
	inline void SetSessionStatus(E_SESSION_STATUS eSessionStatus) { m_eSessionStatus = eSessionStatus; }
	inline E_SESSION_STATUS GetSessionStatus() { return m_eSessionStatus; }

	virtual void SendMyOfflineToFriendMap() {};

#ifndef USE_PACKET_TICK_CHECK_CONTEXT	
	void SetCurrentPacketTick()
	{
		m_ullLastPacketTick = ::GetTickCount64();
	}
	ULONGLONG GetCurrentPacketTick()
	{
		return m_ullLastPacketTick;
	}
#endif

	inline void SetConnectedTime() { m_ullCennection = ::GetTickCount64(); }
	inline ULONGLONG GetConnectedTime() { return m_ullCennection; }
	inline void SetPendingTime() { m_ullPendingTime = GetTickCount64(); }
	inline ULONGLONG GetPendingTime() { return m_ullPendingTime; }
	inline TCHAR* GetIPAddress() { return m_ipaddress; }

	// 서버일때만 씌이는 펑션
public:
	// 케넥션 관리자 cConnectorManager에서 사용되는, 서버상의 map 파일 위치
	inline void SetAllocatedServerSlot(const __int64 allocatedslot) { m_allocatedslot = allocatedslot; }
	inline __int64 GetAllocatedServerSlot() { return m_allocatedslot; }
	inline void SetConnectorInfo(ConnectorInfo* info) { m_pConnectionInfo = info; }
	inline ConnectorInfo* GetConnectorInfo() { return m_pConnectionInfo; }

//#pragma region UDP Support
//private:
//	NetLib::cUDPSession* m_pUDPSession;
//	UINT m_udpTotal;
//	UINT m_udpTotalSize;
//
//public:
//	void SetUDPSession(NetLib::cUDPSession* pUDPSession);
//	NetLib::cUDPSession* GetUDPSession();
//#pragma endregion UDP Support

public:
	cSession();
	virtual ~cSession();
};

END_NETLIB