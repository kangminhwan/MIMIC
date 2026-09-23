#include "../../Include/Netlib/UdpModule/cUDPIocpOv.h"

NetLib::cUDPIocpOv::cUDPIocpOv(E_IO_OPERATION eOperation)
{
	Init(G_DEFIOBUFFERLEN, eOperation);
}


NetLib::cUDPIocpOv::~cUDPIocpOv()
{
	Destroy();
}

void NetLib::cUDPIocpOv::Init(const UINT uiBufferLength, E_IO_OPERATION eOperation)
{
	m_pBuffer = nullptr;
	m_uiBufferLength = uiBufferLength;

	memset(&m_WsaBuf, 0x00, sizeof(m_WsaBuf));
	memset(&m_Overlapped, 0x00, sizeof(WSAOVERLAPPED));

	if(Alloc(m_uiBufferLength))
	{
		m_WsaBuf.buf = reinterpret_cast<char*>(m_pBuffer);
		m_WsaBuf.len = m_uiBufferLength;

		if(m_pBuffer != nullptr)
		{
			memset(m_pBuffer, 0x00, m_uiBufferLength);
		}
	}
	else
	{
		return;
	}

	m_uiDataSize = 0;

	SetOperation(eOperation);

	memset(&m_SockAddrIPv4, 0x00, sizeof(sockaddr));
	//memset(&m_SockAddrIPv6, 0x00, sizeof(sockaddr_in6));
	m_nSockAddrLenIPv4 = sizeof(sockaddr);
	//m_nSockAddrLenIPv6 = sizeof(sockaddr_in6);
	//m_bUseIPv6 = FALSE;
}

void NetLib::cUDPIocpOv::Destroy()
{
	if(m_pBuffer)
	{
		delete[]m_pBuffer;
		m_pBuffer = NULL;
	}

	memset(&m_WsaBuf, 0, sizeof(m_WsaBuf));
	memset(&m_Overlapped, 0, sizeof(WSAOVERLAPPED));
}

void NetLib::cUDPIocpOv::Clean()
{
	if(m_pBuffer)
	{
		memset(m_pBuffer, 0x00, m_uiBufferLength);
		memset(&m_Overlapped, 0x00, sizeof(WSAOVERLAPPED));
		memset(&m_SockAddrIPv4, 0x00, sizeof(sockaddr_in));
		//memset(&m_SockAddrIPv6, 0x00, sizeof(sockaddr_in6));
		//m_bUseIPv6 = FALSE;
	}

	m_uiDataSize = 0;
}

void NetLib::cUDPIocpOv::SetIPv6()
{
	//m_bUseIPv6 = TRUE;
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

bool NetLib::cUDPIocpOv::Alloc(const UINT uiBufferLength)
{
	if(m_pBuffer)
	{
		delete[]m_pBuffer;
		m_pBuffer = NULL;
	}

	m_pBuffer = new BYTE[uiBufferLength];
	if(m_pBuffer)
	{
		m_uiBufferLength = uiBufferLength;
		return true;
	}
	return false;
}

// 데이터를 Copy함, 패킷을 전송할경우 처음에 꼭 CopyBuffer로써 데이터를 카피해여야 한다.
bool NetLib::cUDPIocpOv::CopyBuffer(const BYTE* pBuffer, const UINT uiBufferLength)
{
	if(uiBufferLength > m_uiBufferLength) return false;

	m_WsaBuf.len = uiBufferLength;
	m_uiDataSize = uiBufferLength;
	memcpy(m_WsaBuf.buf, pBuffer, uiBufferLength);
	return true;
}

// 데이터를 Append시킨다.
bool NetLib::cUDPIocpOv::AppendBuffer(const BYTE* pBuffer, const UINT uiBufferLength)
{
	if(m_uiDataSize + uiBufferLength > m_uiBufferLength) return false;

	m_WsaBuf.len = m_uiDataSize + uiBufferLength;
	memcpy(m_WsaBuf.buf + m_uiDataSize, pBuffer, uiBufferLength);
	return true;
}
