#pragma once
#include "../Common/cPacketAnalyzer.h"
#include "cIocpOv.h"
#include "cOverlapped.h"

BEGIN_NETLIB

class cSocket 
#if defined(PACKET_ANALYZE_ON)
	: public cPacketAnalyzer
#endif
{
protected:
	SOCKET		m_hSocket;
	cIocpOv		m_olReceive;
	cIocpOv		m_olAccept;
	cOverlapped	m_olDisconnect;
	cIocpOv		m_olConnect;
	
	cMemPooler<cIocpOv>	m_cSendOvlPool;
	cMemPooler<cIocpOv> m_cStoredOvPool;
#ifdef VIRTUAL_NAGLE_ON_OFF
	std::vector<cIocpOv*> m_packetCollecting;
public:
	void SendVirtualNalePackets();
	BOOL m_bVirtualNagleOnOff;
#endif

protected:
	uint32		m_nIP;
	uint32		m_nPeerPort;

	char		m_IPv6[INET6_ADDRSTRLEN];
	sockaddr_in6 m_sockaddr_in6;
	sockaddr_in m_sockaddr_in;
	BOOL		m_bUseIPv6;

	UINT	m_uiPortID;

	std::atomic<bool> m_bSocketClosed;
public:
	void	Init();
	void	Destroy();

public:
	virtual void Disconnect(E_IO_OPERATION eOperation = E_IO_DISCONNECT);
	BOOL ConnectToServer(char* IPAddr, UINT uPort, int nIPHint, class cIocpContext* pIocpContext, BOOL bPrivate = TRUE);

public:
	BOOL			CreateSocket(BOOL bUseIPv6 = FALSE);
	bool			ReCreateSocket(BOOL bUseIPv6 = FALSE);
	void			CloseSocket();
	BOOL			ReceiveRequest();
	E_ERROR_SEND	SendRequest(const BYTE* pData, UINT uiDataSize);
	E_ERROR_SEND	StoredOvPoolSend();
	void			PushSendOv(cIocpOv* pSndOvl);
	SOCKET			GetSockHandle() { return m_hSocket; }
	void			MakeConnectPacket(BYTE* pData, size_t stSize);

public:
	void	CleanOverlapped(E_IO_OPERATION eOperation);
	LPWSAOVERLAPPED	GetOverlapped(E_IO_OPERATION eOperation);
	BYTE*	GetWsaBuffer() { return (reinterpret_cast<BYTE*>(m_olReceive.m_WsaBuf.buf)); }
	BYTE*	GetAcceptBuffer() { return (reinterpret_cast<BYTE*>(m_olAccept.m_WsaBuf.buf)); }
	size_t GetAcceptBufferLen() { return (size_t)m_olAccept.m_uiMaxBufferLength; }
	void	SendCompleted(cIocpOv* pIocpOv);
	size_t		GetSendOvlCnt() { return m_cSendOvlPool.GetRemainPoolCnt(); }
	void			GetPeerAddress(DWORD lNumberOfBytesTransferred, BOOL bUseIPv6 = FALSE);
	sockaddr_in*	GetSockaddr_in() { return &m_sockaddr_in; }
	sockaddr_in6*	GetSockaddr_in6() { return &m_sockaddr_in6; }
	char*			GetIPv6() { return m_IPv6; }
	BOOL			IsUseIPv6() { return m_bUseIPv6; }
	uint32			GetPeerPort() { return m_nPeerPort; }

	void	SetPortID(UINT uiID) { m_uiPortID = uiID; }
	UINT	GetPortID() { return m_uiPortID; }

	void IncreasePoolSize(const int iMaxPoolSize)
	{
		m_cSendOvlPool.IncreasePoolSize(iMaxPoolSize);
	}

#if defined(VIRTUAL_NAGLE_ON)
protected:
	cIocpOv*	m_pSendIocpOv;
public:
	bool	PopNewIocpOv();
	E_ERROR_SEND Send();
	E_ERROR_SEND UnCheckSend();
#endif

public:
	cSocket();
	virtual ~cSocket();
};

END_NETLIB