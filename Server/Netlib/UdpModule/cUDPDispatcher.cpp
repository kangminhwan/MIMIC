#include "../../Include/Netlib/UdpModule/cUDPDispatcher.h"
#include "../../Include/Netlib/UdpModule/cUDPSession.h"
#include "../../Include/Netlib/UdpModule/cUDPSessionManager.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Network/cPacketStack.h"

NetLib::cUDPDispatcher::cUDPDispatcher()
{
	m_cSendIoCompletionData.CreatePool(10, 10000);
	m_cRecvIoCompletionData.CreatePool(10, 10000);

	m_hUdpSocketIPv4 = INVALID_SOCKET;
	//m_hUdpSocketIPv6 = INVALID_SOCKET;
}


NetLib::cUDPDispatcher::~cUDPDispatcher()
{
	m_cSendIoCompletionData.DestroyPool();
	m_cRecvIoCompletionData.DestroyPool();
	std::list<NetLib::cUDPIOCompletionData*>::iterator iter = m_listTemplate.begin();
	std::list<NetLib::cUDPIOCompletionData*>::iterator iterEnd = m_listTemplate.end();
	for (; iter != iterEnd; ++iter)
	{
		delete (*iter);
		(*iter) = nullptr;
	}
}

size_t NetLib::cUDPDispatcher::GetRemainSendPoolSize()
{
	return m_cSendIoCompletionData.GetRemainPoolCnt();
}

size_t NetLib::cUDPDispatcher::GetRemainReceivePoolSize()
{
	return m_cRecvIoCompletionData.GetRemainPoolCnt();
}

void NetLib::cUDPDispatcher::SetUDPSocket(SOCKET UdpSocketIPv4)
{
	m_hUdpSocketIPv4 = UdpSocketIPv4;
}

//void NetLib::cUDPDispatcher::SetUDPSocket(SOCKET UdpSocketIPv4, SOCKET UdpSocketIPv6)
//{
//	m_hUdpSocketIPv4 = UdpSocketIPv4;
//	m_hUdpSocketIPv6 = UdpSocketIPv6;
//}

bool NetLib::cUDPDispatcher::SendIOCompletion(NetLib::cUDPIOCompletionData* pCompletionData)
{
	if(pCompletionData == nullptr)
	{
		return false;
	}

	pCompletionData->Clear();
	m_cSendIoCompletionData.Push(pCompletionData);

	return true;
}

bool NetLib::cUDPDispatcher::RecvIOCompletion(NetLib::cUDPIOCompletionData* pCompletionData)
{
	if(pCompletionData == nullptr)
	{
		return false;
	}

	DeleteTemplate(pCompletionData);

	pCompletionData->Clear();
	m_cRecvIoCompletionData.Push(pCompletionData);

	return true;
}


//////////////////////////////////////////////////////////////////////////
// 소켓을 통한 Send와 관련된 함수들

E_ERROR_SEND NetLib::cUDPDispatcher::SendPacket(NetLib::cUDPSession* pSession, UINT nCommand, BYTE* pData, UINT uiDataSize, const UINT uiPacketSeq)
{
	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_UDP);

#if defined(PACKET_ANALYZE_ON)
	packet.Make(nCommand, pData, uiDataSize, 0);
#else
	packet.Make(nCommand, pData, uiDataSize, 0);
#endif

	if(uiPacketSeq)
		packet.SetPacketSeq(uiPacketSeq);

	return SendRequest(pSession, packet.GetBuffer(), packet.GetLength());

}

E_ERROR_SEND NetLib::cUDPDispatcher::SendRequest(NetLib::cUDPSession* pSession, BYTE* pData, UINT uiDataSize)
{
	if(!pSession) return E_ERROR_SEND_ERROR;

	// 메모리 풀과 세션을 연결해주는 객체를 하나 꺼낸다.
	NetLib::cUDPIOCompletionData* pCompletionData = m_cSendIoCompletionData.Pop();
	if(!pCompletionData)
	{
		// SendPool에 더이상 버퍼가 없다
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("cUDPDispatcher::SendRequest Faile becuase not enough Completion Pool"));
		return E_ERROR_SEND_SEND_POOL_EMPTY;
	}

	NetLib::cUDPIocpOv* pSndOvl = pCompletionData->GetUDPIocpOv();
	pCompletionData->SetUDPSession(pSession);

	// 메모리풀에서 꺼낸 버퍼를 초기화 하고 데이타를 복사한다
	pSndOvl->SetOperation(E_IO_SEND);

	if(!pSndOvl->CopyBuffer(pData, uiDataSize))
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("cIocpUDP::SendRequest CopyBuffer Failed"));
		SendIOCompletion(pCompletionData);
		return E_ERROR_SEND_DATA_SIZE_OVER;
	}

	// 주소값 카피
	DWORD dwFlags = 0, dwSendBytes = 0;
	int iRet = SOCKET_ERROR;

	/*if(pSession->UseIPv6())
	{
		memset(&pSndOvl->m_SockAddrIPv6, 0x00, sizeof(SOCKADDR_IN6));

		pSndOvl->m_SockAddrIPv6.sin6_family = AF_INET6;
		pSndOvl->m_SockAddrIPv6.sin6_flowinfo = 0;
		pSndOvl->m_SockAddrIPv6.sin6_port = htons(pSession->GetPort());
		IN6_ADDR addrinfo;
		InetPtonA(AF_INET6, pSession->GetIPv6(), &addrinfo);
		pSndOvl->m_SockAddrIPv6.sin6_addr = addrinfo;

		iRet = WSASendTo(	m_hUdpSocketIPv6,
							&pSndOvl->m_WsaBuf,
							1,
							&dwSendBytes,
							dwFlags,
							(SOCKADDR*)&(pSndOvl->m_SockAddrIPv6),
							pSndOvl->m_nSockAddrLenIPv6,
							(LPWSAOVERLAPPED)&pSndOvl->m_Overlapped,
							NULL);
	}
	else*/
	{
		memset(&pSndOvl->m_SockAddrIPv4, 0x00, sizeof(SOCKADDR_IN));

		pSndOvl->m_SockAddrIPv4.sin_family = AF_INET;
		pSndOvl->m_SockAddrIPv4.sin_addr.s_addr = pSession->GetIP();
		pSndOvl->m_SockAddrIPv4.sin_port = htons(pSession->GetPort());

		iRet = WSASendTo(	m_hUdpSocketIPv4,
							&pSndOvl->m_WsaBuf,
							1,
							&dwSendBytes,
							dwFlags,
							(SOCKADDR*)&(pSndOvl->m_SockAddrIPv4),
							pSndOvl->m_nSockAddrLenIPv4,
							(LPWSAOVERLAPPED)&pSndOvl->m_Overlapped,
							NULL);
	}

	if(iRet == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();

		if(dwLastError == WSAEWOULDBLOCK)
		{
			// 이경우는 재전송해주어야 함..
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_NOR, _T("cIocpUDP::SendRequest WSASendTo EWouldBlock error"));
			SendIOCompletion(pCompletionData);

			return E_ERROR_SEND_WOULDBLOCK;
		}

		if(dwLastError != WSA_IO_PENDING)
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_NOR, _T("cIocpUDP::SendRequest WSASendTo error %u"), PrintLastError());
			SendIOCompletion(pCompletionData);
			return E_ERROR_SEND_SOCKET_ERROR;
		}
	}
	return E_ERROR_SEND_OK;
}

void NetLib::cUDPDispatcher::BroadCast(NetLib::cUDPSession* pSessionOfMe, BYTE* pData, UINT uiDataSize)
{
	NetLib::cUDPSessionManager* sessionMgr = NetLib::cSingleton<NetLib::cUDPSessionManager>::GetInstance();
	NetLib::cUDPSessionManager::SessionTable* sessionTbl = sessionMgr->GetSessionTable();
	POSITION pos = NULL;

	pos = sessionTbl->GetStartPosition();
	NetLib::cUDPSession* pSession = NULL;
	while (pos != NULL)
	{
		pSession = sessionTbl->GetNextValue(pos);

		// 나 자신은 Pass한다.
		if(pSession == pSessionOfMe) continue;
		SendRequest(pSession, pData, uiDataSize);
	}
}

void NetLib::cUDPDispatcher::BroadCast(BYTE* pData, UINT uiDataSize)
{
	NetLib::cUDPSessionManager* sessionMgr = NetLib::cSingleton<NetLib::cUDPSessionManager>::GetInstance();
	NetLib::cUDPSessionManager::SessionTable* sessionTbl = sessionMgr->GetSessionTable();
	POSITION pos = NULL;

	pos = sessionTbl->GetStartPosition();
	NetLib::cUDPSession* pSession = NULL;
	while (pos != NULL)
	{
		pSession = sessionTbl->GetNextValue(pos);
		SendRequest(pSession, pData, uiDataSize);
	}
}


//////////////////////////////////////////////////////////////////////////
// 리시브 요청
E_ERROR_SEND NetLib::cUDPDispatcher::ReceiveRequest(BOOL bUseIPv6)
{
	NetLib::cUDPIOCompletionData* completionData = m_cRecvIoCompletionData.Pop();
	if(completionData == nullptr)
	{
		// cUDPIOCompetion Pool에 데이터가 없다
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("cUDPDispatcher::ReceiveRequest Faile becuase not enough Completion Pool"));
		return E_ERROR_SEND_SEND_POOL_EMPTY;
	}

	InsertTemplate(completionData);

	NetLib::cUDPIocpOv* pRecvOvl = completionData->GetUDPIocpOv();
	if(pRecvOvl == nullptr)
		return E_ERROR_SEND_ERROR;

	pRecvOvl->Clean();
	pRecvOvl->SetOperation(E_IO_RECEIVE);

	DWORD dwFlags = 0, dwRecvBytes = 0;
	int iRet = SOCKET_ERROR;

	/*if(bUseIPv6)
	{
		pRecvOvl->SetIPv6();

		iRet = WSARecvFrom(	m_hUdpSocketIPv6,
							&(pRecvOvl->m_WsaBuf),
							1,
							&dwRecvBytes,
							&dwFlags,
							(SOCKADDR*)&pRecvOvl->m_SockAddrIPv6,
							&pRecvOvl->m_nSockAddrLenIPv6,
							(LPWSAOVERLAPPED)&pRecvOvl->m_Overlapped,
							NULL);
	}
	else*/
	{
		iRet = WSARecvFrom(	m_hUdpSocketIPv4,
							&(pRecvOvl->m_WsaBuf),
							1,
							&dwRecvBytes,
							&dwFlags,
							(SOCKADDR*)&pRecvOvl->m_SockAddrIPv4,
							&pRecvOvl->m_nSockAddrLenIPv4,
							(LPWSAOVERLAPPED)&pRecvOvl->m_Overlapped,
							NULL);
	}

	if(iRet == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if(error == WSAEWOULDBLOCK)
		{
			// 이경우는 재전송해주어야 함..
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_NOR, _T("cUDPDispatcher WSARecvFrom WSAEWOULDBLOCK error"));
			RecvIOCompletion(completionData);

			return E_ERROR_SEND_WOULDBLOCK;
		}

		if(error != WSA_IO_PENDING)
		{
			PrintLastError();
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_NOR, _T("cUDPDispatcher SOCKET error code[%d]"), error);
			RecvIOCompletion(completionData);

			return E_ERROR_SEND_SOCKET_ERROR;
		}

	}
	return E_ERROR_SEND_OK;
}

void NetLib::cUDPDispatcher::InsertTemplate(NetLib::cUDPIOCompletionData* pTemplate)
{
	if(pTemplate)
	{
		m_listTemplate.push_back(pTemplate);
	}
}

void NetLib::cUDPDispatcher::DeleteTemplate(NetLib::cUDPIOCompletionData* pTemplate)
{
	if(pTemplate)
	{
		std::list<NetLib::cUDPIOCompletionData*>::iterator iter = m_listTemplate.begin();
		for (; iter != m_listTemplate.end();)
		{
			if((*iter) == pTemplate)
			{
				iter = m_listTemplate.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
}