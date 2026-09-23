#pragma once
#include "../IOCP/cOverlapped.h"

BEGIN_NETLIB

class cUDPIocpOv : public cOverlapped
{
public:
	sockaddr_in	m_SockAddrIPv4;
	//sockaddr_in6 m_SockAddrIPv6;
	WSABUF		m_WsaBuf;
	int			m_nSockAddrLenIPv4;
	//int			m_nSockAddrLenIPv6;
	UINT		m_uiBufferLength;
	BYTE*		m_pBuffer;
	UINT		m_uiDataSize;
	//BOOL        m_bUseIPv6;

public:
	void	Init(const UINT uiBufferLength = G_DEFIOBUFFERLEN, E_IO_OPERATION eOperation = E_IO_NONE);
	void	Destroy();
	void	Clean();
	void    SetIPv6();

public:
	bool	Alloc(const UINT uiBufferLength);
	bool	CopyBuffer(const BYTE* pBuffer, const UINT uiBufferLength);
	bool	AppendBuffer(const BYTE* pBuffer, const UINT uiBufferLength);
	//BOOL    GetIPv6() { return m_bUseIPv6; }

public:
	cUDPIocpOv(E_IO_OPERATION eOperation = E_IO_NONE);
	virtual ~cUDPIocpOv();
};

END_NETLIB