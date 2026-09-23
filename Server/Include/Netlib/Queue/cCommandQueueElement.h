#pragma once
#include "cBaseQueueElement.h"

BEGIN_NETLIB

class cIocpContext;
class cUDPDispatcher;
class cUDPIocpOv;
class cCommandQueueElement : public cBaseQueueElement
{
private:
	UINT	m_nID;
	UINT	m_nCommand;
	UINT	m_uiPacketSeq;
	cIocpContext*	m_pContext;
	cUDPDispatcher* m_pUDPDispatcher;
	UINT m_uiEntity;
	sockaddr_in m_sockinfoIPv4;
	sockaddr_in6 m_sockinfoIPv6;
	BOOL	m_bUseIPv6;

private:
	void	Init();
	void	Destroy();

public:
	BOOL	CopyData(	const cIocpContext* pContext,
						const UINT nCommand,
						const BYTE* lpPacket = NULL,
						const UINT nLength = 0);

	BOOL	CopyData(	const UINT nID,
						const UINT nCommand,
						const BYTE* lpPacket = NULL,
						const UINT nLength = 0);

	BOOL	CopyData(	const cUDPDispatcher* pUDPDispatcher,
						const UINT nCommand,
						const UINT Entity,
						const UINT uiPacketSeq,
						const cUDPIocpOv* pUdpIocpOv,
						const BYTE* lpBuffer,
						const UINT nLength);

	void	SetContext(cIocpContext* pContext) { m_pContext = pContext; }

public:
	//IIocpContext* GetContext()	{	return m_pContext;							}
	cIocpContext*	GetContext() { return m_pContext; }
	cUDPDispatcher* GetDispatcher() { return m_pUDPDispatcher; }
	UINT			GetEntity() { return m_uiEntity; }
	UINT			GetPacketSeq() { return m_uiPacketSeq; }
	sockaddr_in*	GetSockInfoIPv4() { return &m_sockinfoIPv4; }
	sockaddr_in6*	GetSockInfoIPv6() { return &m_sockinfoIPv6; }
	UINT			GetID() { return m_nID; }
	UINT			GetCommand() { return m_nCommand; }
	UINT			GetLength() { return cBaseQueueElement::GetLength(); }
	BYTE*			GetData() { return cBaseQueueElement::GetData(); }
	BOOL			UseIPv6() { return m_bUseIPv6; }

	void	MakeInitialize();

public:
	cCommandQueueElement(const int BufferSize = G_NET_BUFFER_SIZE_BASIC);
	virtual ~cCommandQueueElement();
};

END_NETLIB