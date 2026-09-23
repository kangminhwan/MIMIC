#include "../../Include/Netlib/UdpModule/cUDPSession.h"

NetLib::cUDPSession::cUDPSession()
{
	Clear();
}


NetLib::cUDPSession::~cUDPSession()
{
}

NetLib::cUDPSession::cUDPSession(DWORD dwIP, WORD wPort)
{
	m_dwIP = dwIP;
	m_wPort = wPort;
	m_ullLastTime = GetTickCount64(); // 생성하면서 현재시간으로 셋팅해줌..	
}

void NetLib::cUDPSession::Clear()
{
	m_dwIP = 0;
	m_wPort = 0;
	m_ullLastTime = 0;
	m_uiEntity = 0;
	memset(m_IPv6, 0x00, sizeof(m_IPv6));
	memset(m_IPv4, 0x00, sizeof(m_IPv4));
	m_bUseIPv6 = FALSE;
	m_uiPacketSeq = 0;
	m_uiPacketCount = 0;
}

NetLib::cUDPSession::cUDPSession(sockaddr_in& sockAddr)
{
	m_dwIP = sockAddr.sin_addr.s_addr;
	m_wPort = ntohs(sockAddr.sin_port);
	m_ullLastTime = ::GetTickCount64(); // 생성하면서 현재시간으로 셋팅해줌..	
}

NetLib::cUDPSession::cUDPSession(const NetLib::cUDPSession& udpSession)
{
	m_dwIP = udpSession.m_dwIP;
	m_wPort = udpSession.m_wPort;
	m_ullLastTime = udpSession.m_ullLastTime;
}

NetLib::cUDPSession& NetLib::cUDPSession::operator = (const NetLib::cUDPSession& udpSession)
{
	m_dwIP = udpSession.m_dwIP;
	m_wPort = udpSession.m_wPort;
	m_ullLastTime = udpSession.m_ullLastTime;

	return *this;
}

void NetLib::cUDPSession::SetAddrIPv4(sockaddr_in* sockAddr)
{
	m_dwIP = sockAddr->sin_addr.s_addr;
	m_wPort = ntohs(sockAddr->sin_port);

	m_IPv4[0] = sockAddr->sin_addr.S_un.S_un_b.s_b1;
	m_IPv4[1] = sockAddr->sin_addr.S_un.S_un_b.s_b2;
	m_IPv4[2] = sockAddr->sin_addr.S_un.S_un_b.s_b3;
	m_IPv4[3] = sockAddr->sin_addr.S_un.S_un_b.s_b4;

	m_bUseIPv6 = FALSE;
}

void NetLib::cUDPSession::SetAddrIPv6(sockaddr_in6* sockAddr)
{
	memset(m_IPv6, 0x00, sizeof(m_IPv6));
	inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sockAddr)->sin6_addr), m_IPv6, INET6_ADDRSTRLEN);
	m_wPort = ntohs(sockAddr->sin6_port);
	m_bUseIPv6 = TRUE;
}

BOOL NetLib::operator==(const NetLib::cUDPSession& udpSession1, const NetLib::cUDPSession& udpSession2)
{
	if(udpSession1.m_dwIP != udpSession2.m_dwIP)
		return false;

	if(udpSession1.m_wPort != udpSession2.m_wPort)
		return false;

	return true;
}