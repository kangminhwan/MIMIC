#pragma once
#include "cUDPIOCompletionData.h"

BEGIN_NETLIB

class cUDPDispatcher
{
private:
	SOCKET m_hUdpSocketIPv4;
	//SOCKET m_hUdpSocketIPv6;
	cMemPooler<cUDPIOCompletionData>	m_cRecvIoCompletionData;
	cMemPooler<cUDPIOCompletionData>	m_cSendIoCompletionData;


	std::list<cUDPIOCompletionData*> m_listTemplate;
private:
	void InsertTemplate(cUDPIOCompletionData* pTemplate);
	void DeleteTemplate(cUDPIOCompletionData* pTemplate);
public:
	//void SetUDPSocket(SOCKET UdpSocketIPv4, SOCKET UdpSocketIPv6);
	void SetUDPSocket(SOCKET UdpSocketIPv4);

	// IO의 완료함수
	bool SendIOCompletion(cUDPIOCompletionData* pCompletionData);
	bool RecvIOCompletion(cUDPIOCompletionData* pCompletionData);

	//////////////////////////////////////////////////////////////////////////
	// Send 관련 함수
	E_ERROR_SEND SendRequest(cUDPSession* pSession, BYTE* pData, UINT uiDataSize);
	E_ERROR_SEND SendPacket(cUDPSession* pSession, UINT nCommand, BYTE* pData, UINT uiDataSize, const UINT uiPacketSeq = 0);
	void		 BroadCast(cUDPSession* pSessionOfMe, BYTE* pData, UINT uiDataSize);
	void		 BroadCast(BYTE* pData, UINT uiDataSize);

	//////////////////////////////////////////////////////////////////////////
	// Recv 관련 함수
	E_ERROR_SEND ReceiveRequest(BOOL bUseIPv6 = FALSE);

	size_t GetRemainSendPoolSize();
	size_t GetRemainReceivePoolSize();

public:
	cUDPDispatcher();
	~cUDPDispatcher();
};

END_NETLIB