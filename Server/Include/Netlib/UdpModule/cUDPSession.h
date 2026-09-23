#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cUDPSession
{
protected:
	// 접속한 아이피
	DWORD	m_dwIP;
	char	m_IPv6[INET6_ADDRSTRLEN];
	BYTE	m_IPv4[4];

	// 접속한 포트
	WORD	m_wPort;

	// 마지막으로 패킷이 날라온 시간
	ULONGLONG	m_ullLastTime;

	// 자신의 Entity값
	UINT	m_uiEntity;

	BOOL    m_bUseIPv6;

	// 마지막으로 받은 패킷의 Last Sequence Number
	UINT	m_uiPacketSeq;
	UINT    m_uiPacketCount;

public:
	virtual void Clear();

	cUDPSession& operator=(const cUDPSession& udpSession);
	friend BOOL operator==(const cUDPSession& udpSession1, const cUDPSession& udpSession2);

	void	SetEntity(UINT Entity) { m_uiEntity = Entity; }
	UINT	GetEntity() { return m_uiEntity; }
	WORD	GetPort() { return m_wPort; }
	DWORD	GetIP() { return m_dwIP; }
	char*	GetIPv6() { return m_IPv6; }
	ULONGLONG   GetTime() { return m_ullLastTime; }
	void	SetAddrIPv4(sockaddr_in* sockAddr);
	void	SetAddrIPv6(sockaddr_in6* sockAddr);
	void	SetLastRecvedPacketTime(ULONGLONG ullLastTime) { m_ullLastTime = ullLastTime; }
	BOOL	UseIPv6() { return m_bUseIPv6; }
	UINT	GetPacketSeq()	{ return m_uiPacketSeq; }
	void	SetPacketSeq(UINT _uiPacketSeq) { m_uiPacketSeq = _uiPacketSeq; }
	void	IncreasePacketCount() { ++m_uiPacketCount; }
	UINT	GetPacketCount() { return m_uiPacketCount; }

public:
	cUDPSession();
	cUDPSession(DWORD dwIP, WORD wPort);
	cUDPSession(sockaddr_in& sockAddr);
	cUDPSession(const cUDPSession& udpSession);
	virtual ~cUDPSession();
};

END_NETLIB