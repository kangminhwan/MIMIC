#include "../../Include/Netlib/Queue/cCommandQueueElement.h"
#include "../../Include/Netlib/UdpModule/cUDPIocpOv.h"

NetLib::cCommandQueueElement::cCommandQueueElement(const int BufferSize) :
	cBaseQueueElement(BufferSize)
{
	Init();
}


NetLib::cCommandQueueElement::~cCommandQueueElement()
{
	Destroy();
}

void NetLib::cCommandQueueElement::Init()
{
	MakeInitialize();
}

void NetLib::cCommandQueueElement::Destroy()
{
	MakeInitialize();
}

void NetLib::cCommandQueueElement::MakeInitialize()
{
	m_nID = 0;
	m_nCommand = 0;
	m_pContext = NULL;
	m_pUDPDispatcher = NULL;
	m_uiEntity = 0;
	memset(&m_sockinfoIPv4, 0x00, sizeof(sockaddr_in));
	memset(&m_sockinfoIPv6, 0x00, sizeof(sockaddr_in6));
	m_bUseIPv6 = FALSE;
}


//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

BOOL NetLib::cCommandQueueElement::CopyData(const NetLib::cIocpContext* pContext,
											const UINT nCommand,
											const BYTE* lpBuffer,
											const UINT nLength)
{
	if(NetLib::cBaseQueueElement::CopyData(lpBuffer, nLength))
	{
		m_pContext = const_cast<NetLib::cIocpContext*>(pContext);
		m_nID = 0;
		m_nCommand = nCommand;
		m_pUDPDispatcher = NULL;
		return TRUE;
	}
	return FALSE;
}

BOOL NetLib::cCommandQueueElement::CopyData(const UINT nID,
											const UINT nCommand,
											const BYTE* lpBuffer,
											const UINT nLength)
{
	if(NetLib::cBaseQueueElement::CopyData(lpBuffer, nLength))
	{
		m_pContext = NULL;
		m_nID = nID;
		m_nCommand = nCommand;
		m_pUDPDispatcher = NULL;
		return TRUE;
	}
	return FALSE;
}

BOOL NetLib::cCommandQueueElement::CopyData(const NetLib::cUDPDispatcher* pUDPDispatcher,
											const UINT nCommand,
											const UINT Entity,
											const UINT uiPacketSeq,
											const NetLib::cUDPIocpOv* pUdpIocpOv,
											const BYTE* lpBuffer,
											const UINT nLength)
{
	if(NetLib::cBaseQueueElement::CopyData(lpBuffer, nLength))
	{
		m_pContext = NULL;
		m_nID = 0;
		m_nCommand = nCommand;
		m_pUDPDispatcher = const_cast<NetLib::cUDPDispatcher*>(pUDPDispatcher);
		m_uiEntity = Entity;
		m_uiPacketSeq = uiPacketSeq;
		/*if(pUdpIocpOv->m_bUseIPv6)
		{
			memcpy(&m_sockinfoIPv6, &pUdpIocpOv->m_SockAddrIPv6, sizeof(sockaddr_in6));
			m_bUseIPv6 = TRUE;
		}
		else*/
			memcpy(&m_sockinfoIPv4, &pUdpIocpOv->m_SockAddrIPv4, sizeof(sockaddr_in));
		//m_sockinfo = const_cast<sockaddr_in*>(sock);
		return TRUE;
	}
	return FALSE;
}