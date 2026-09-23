#include "../../Include/Netlib/IOCP/cSocket.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/IOCP/cIocp.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Common/cHeader.h"

NetLib::cSocket::cSocket() : 
#if defined(VIRTUAL_NAGLE_ON)
	m_pSendIocpOv(NULL),
#endif
	m_hSocket(INVALID_SOCKET)
{
	Init();
}


NetLib::cSocket::~cSocket()
{
	Destroy();
}

void NetLib::cSocket::Init()
{
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(!pServerManager) return;

	ServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if(!pConfig)
	{
		Trace(_T("cSocket::Init() failed, no serverconfig data"));
		::MessageBox(NULL, _T("cSocket::Init() failed, no serverconfig data"), _T("No Server Config"), MB_ICONEXCLAMATION);
		return;
	}

	int nMaxSendOvCnt = pConfig->nPerSocket_OvPoolCnt;

	//m_cSendOvlPool.CreatePool( MAX_BUFFER_SIZE/10, MAX_BUFFER_SIZE );
	//m_cSendOvlPool.CreatePool( nMaxSendOvCnt/10, nMaxSendOvCnt );
	m_cSendOvlPool.CreatePool(0, nMaxSendOvCnt, nMaxSendOvCnt);

	m_olReceive.SetOperation(E_IO_RECEIVE);
	m_olAccept.SetOperation(E_IO_ACCEPT);
	m_olDisconnect.SetOperation(E_IO_DISCONNECT);
	m_olConnect.SetOperation(E_IO_CONNECT);
	CreateSocket();

#ifdef VIRTUAL_NAGLE_ON_OFF
	m_bVirtualNagleOnOff = FALSE;
#endif
}

void NetLib::cSocket::Destroy()
{
	CloseSocket();

	m_olReceive.Destroy();
	m_olAccept.Destroy();
	m_olConnect.Destroy();
	m_cSendOvlPool.DestroyPool();
}

//////////////////////////////////////////////////////////////////////
// Operation 
//////////////////////////////////////////////////////////////////////

BOOL NetLib::cSocket::CreateSocket(BOOL bUseIPv6)
{
	//CloseSocket();
	if(bUseIPv6)
		m_hSocket = WSASocket(	AF_INET6,
								SOCK_STREAM,
								IPPROTO_TCP,
								NULL,
								NULL,
								WSA_FLAG_OVERLAPPED);
	else
		m_hSocket = WSASocket(	AF_INET,
								SOCK_STREAM,
								IPPROTO_TCP,
								NULL,
								NULL,
								WSA_FLAG_OVERLAPPED);

	m_bUseIPv6 = bUseIPv6;

	if(m_hSocket == INVALID_SOCKET)
	{
		m_bSocketClosed.store(true);
		return FALSE;
	}

	m_bSocketClosed.store(false);

	int  iSockBuffer = 0;
	BOOL bSockOpt = TRUE;

	if(setsockopt(	m_hSocket,
					IPPROTO_TCP,
					TCP_NODELAY,
					reinterpret_cast<char*>(&bSockOpt),
					sizeof(bSockOpt)) == SOCKET_ERROR)
	{
		CloseSocket();
		return FALSE;
	}

	/*if( setsockopt(	m_hSocket,
	SOL_SOCKET,
	SO_SNDBUF,
	reinterpret_cast<char*>(&bSockOpt),
	sizeof(iSockBuffer)) == SOCKET_ERROR )
	{
	CloseSocket();
	return FALSE;
	}*/

	BOOL bReuseOpt = 1;
	if(setsockopt(	m_hSocket,
					SOL_SOCKET,
					SO_REUSEADDR,
					reinterpret_cast<char*>(&bReuseOpt),
					sizeof(bReuseOpt)) == SOCKET_ERROR)
	{
		CloseSocket();
		return FALSE;
	}

	/*if( setsockopt(	m_hSocket,
	SOL_SOCKET,
	SO_RCVBUF,
	reinterpret_cast<char*>(&bSockOpt),
	sizeof(iSockBuffer)) == SOCKET_ERROR )
	{
	CloseSocket();
	return FALSE;
	}*/

	LINGER  lingerStruct;

	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;	// TIME_WAIT : 0 sec

	if(setsockopt(	m_hSocket,
					SOL_SOCKET,
					SO_LINGER,
					(char*)&lingerStruct,
					sizeof(lingerStruct)))
	{
		CloseSocket();
		return FALSE;
	}
	return TRUE;
}

bool NetLib::cSocket::ReCreateSocket(BOOL bUseIPv6)
{
	// 소켓종료 후 리스너/컨텍스트 설정에 맞는 새 소켓을 만든다.
	CloseSocket();

	return (CreateSocket(bUseIPv6) == TRUE);
}

void NetLib::cSocket::CloseSocket()
{
	bool expected = false;
	if (!m_bSocketClosed.compare_exchange_strong(expected, true))
		return; // 이미 닫힌 상태였음

	const SOCKET hSocket = m_hSocket;
	m_hSocket = INVALID_SOCKET;

	if (hSocket != INVALID_SOCKET)
	{
		shutdown(hSocket, SD_BOTH);
		closesocket(hSocket);
	}
}

void NetLib::cSocket::Disconnect(E_IO_OPERATION eOperation)
{
	if (m_bSocketClosed.load())
		return;

	const SOCKET hSocket = m_hSocket;
	if (hSocket == INVALID_SOCKET)
		return;
	BOOL bResult = FALSE;

	CleanOverlapped(E_IO_DISCONNECT);

	// 강제 종료나 정상 종료나 Overlapped버퍼는 같은것을 사용합니다.
	m_olDisconnect.SetOperation(eOperation); // 20090501

	bResult = TransmitFile(	hSocket,
							NULL,
							0,
							0,
							&(m_olDisconnect.m_Overlapped),
							NULL,
							TF_DISCONNECT | TF_REUSE_SOCKET);

	if(!bResult)
	{
		DWORD dwError = WSAGetLastError();
		if(dwError != ERROR_IO_PENDING)
		{
			// 에러 발생..
			PrintLastError(__FILE__, __LINE__);
		}
	}
}

BOOL NetLib::cSocket::ConnectToServer(char* IPAddr, UINT uPort, int nIPHint, NetLib::cIocpContext* pIocpContext, BOOL bPrivate)
{
	INT             nReturn;
	BOOL            bReturn;
	DWORD           dwBytes = 0;
	LPFN_CONNECTEX  lpfnConnectEx = NULL;
	GUID            GuidConnectEx = WSAID_CONNECTEX;

	// Overlap의 Connection을 위한 함수포인터인 lpfnConnectEx을 얻어온다. 
	nReturn = ::WSAIoctl(	m_hSocket,
							SIO_GET_EXTENSION_FUNCTION_POINTER,
							&GuidConnectEx,
							sizeof(GuidConnectEx),
							&lpfnConnectEx,
							sizeof(lpfnConnectEx),
							&dwBytes,
							NULL,
							NULL);

	if(SOCKET_ERROR == nReturn)
	{
		return FALSE;
	}

	m_olConnect.m_Operation = E_IO_CONNECT;

	// Note : 소켓주소구조체를 구성해서 bind작업을 한다.( connect 작업이 되지 않음 ) 
	//        bind시에는 Local주소중 하나를 사용해주고 
	//        Protocol family만 지정해주면 된다. 
	char localip[21] = { 0 };

	// bind를 사설에 할지, 공인에 할지 결정해야 한다.
	if (GetAddress(localip, nIPHint, bPrivate) == FALSE)
		return FALSE;

	SOCKADDR_IN    localAddr;
	::ZeroMemory(&localAddr, sizeof(SOCKADDR_IN));
	localAddr.sin_family = AF_INET;
	inet_pton(localAddr.sin_family, localip, &localAddr.sin_addr.s_addr);

	nReturn = ::bind(m_hSocket, (struct sockaddr *)&localAddr, sizeof(localAddr));
	if(SOCKET_ERROR == nReturn)
	{
		return FALSE;
	}

	// 입력받은 서버의 ip와 port로 소켓주소 구조체를 구성한다. 
	SOCKADDR_IN si_addr;
	::ZeroMemory(&si_addr, sizeof(SOCKADDR_IN));
	si_addr.sin_family = AF_INET;
	si_addr.sin_port = htons(uPort);
	inet_pton(si_addr.sin_family, IPAddr, &si_addr.sin_addr.s_addr);


	// 입력받은 IOCP핸들과 socket핸들을 Associate한다. 
	NetLib::cIOCP* pIOCP = NetLib::cSingleton<NetLib::cIOCP>::ExistsInstance();
	if(pIOCP == nullptr)
		return FALSE;

	pIocpContext->SetConnectorStatus(E_IOCP_CONNECTOR_STATUS_TRY_CONNECT);

	if(pIOCP->AssocInstance(reinterpret_cast<HANDLE>(GetSockHandle()), reinterpret_cast<ULONG_PTR>(pIocpContext)) == FALSE)
	{
		return FALSE;
	}

	DWORD iSentBytes = 0;
	// Note : Connect에 관한 Overlap작업을 요청한다. 
	bReturn = lpfnConnectEx(	m_hSocket,
								(struct sockaddr *)&si_addr,
								sizeof(struct sockaddr),
								nullptr,
								0,
								&iSentBytes,
								&(m_olConnect.m_Overlapped));

	if(FALSE == bReturn && (WSAGetLastError() != WSA_IO_PENDING))
	{
		return FALSE;
	}

	return TRUE;
}

E_ERROR_SEND NetLib::cSocket::StoredOvPoolSend()
{
	// 저장되어 있는 풀에 내용이 있는가?
	cIocpOv* pSndOvl = m_cStoredOvPool.Pop();
	if(!pSndOvl)
	{
		// 보낼것이 없기때문에 OK..
		return E_ERROR_SEND_OK;
	}

	DWORD dwBytes = 0, dwFlags = 0;
	int iRet = SOCKET_ERROR;

	while (pSndOvl)
	{
		iRet = WSASend(	m_hSocket,
						&(pSndOvl->m_WsaBuf),
						1,
						&dwBytes,
						dwFlags,
						&(pSndOvl->m_Overlapped),
						NULL);

		if(iRet == SOCKET_ERROR)
		{
			DWORD dwLastError = WSAGetLastError();

			if(dwLastError == WSAEWOULDBLOCK)
			{
				// 또 실패했다. 다시 집어 넣자.
				m_cStoredOvPool.Push(pSndOvl);
				return E_ERROR_SEND_WOULDBLOCK;
			}

			if(dwLastError != WSA_IO_PENDING)
			{
				m_cStoredOvPool.Push(pSndOvl);
				return E_ERROR_SEND_SOCKET_ERROR;
			}
		}

		pSndOvl = m_cStoredOvPool.Pop();
	}

	// 남아 있던 모든 Ov완료되었음..
	return E_ERROR_SEND_OK;
}

BOOL NetLib::cSocket::ReceiveRequest()
{
	DWORD dwFlags = 0, dwRecvBytes = 0;

	CleanOverlapped(E_IO_RECEIVE);

	int iRet = WSARecv(	m_hSocket,
						static_cast<LPWSABUF>(&m_olReceive.m_WsaBuf),
						1,
						&dwRecvBytes,
						&dwFlags,
						GetOverlapped(E_IO_RECEIVE),
						NULL);

	if(iRet == SOCKET_ERROR)
	{
		if(WSAGetLastError() != WSA_IO_PENDING)
		{
			DWORD dwError = PrintLastError(__FILE__, __LINE__);
			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: WSARecv error[%u], socket[%d]"), dwError, m_hSocket);
			return FALSE;
		}
	}
	return TRUE;
}

void NetLib::cSocket::CleanOverlapped(E_IO_OPERATION eOperation)
{
	switch (eOperation)
	{
	case E_IO_RECEIVE:
		m_olReceive.Clean();
		break;

	case E_IO_ACCEPT:
		m_olAccept.Clean();
		break;

	case E_IO_DISCONNECT:
		m_olDisconnect.Clean();
		break;
	}
}

LPWSAOVERLAPPED	NetLib::cSocket::GetOverlapped(E_IO_OPERATION eOperation)
{
	switch (eOperation)
	{
	case E_IO_RECEIVE:
		return &(m_olReceive.m_Overlapped);
		break;

	case E_IO_ACCEPT:
		return &(m_olAccept.m_Overlapped);
		break;

	case E_IO_DISCONNECT:
		return &(m_olDisconnect.m_Overlapped);
		break;
	}
	return NULL;
}

void NetLib::cSocket::SendCompleted(NetLib::cIocpOv* pSndOvl)
{
	PushSendOv(pSndOvl);
	//m_cSendOvlPool.Push( pSndOvl );		
}

void NetLib::cSocket::GetPeerAddress(DWORD lNumberOfBytesTransferred, BOOL bUseIPv6)
{
	if(bUseIPv6)
	{
		SOCKADDR_IN6 *localAddr, *remoteAddr;
		int localLen = 0, remoteLen = 0;

		GetAcceptExSockaddrs(	GetAcceptBuffer(),
								//GetAcceptBufferLen() > 0 ? (DWORD)(GetAcceptBufferLen() - ((sizeof(sockaddr_in6) + 16) * 2)) : 0,
								//AcceptEx 호출 하는 부분을 보시면 주석이있습니다.
								lNumberOfBytesTransferred == 0 ? 0 : (DWORD)(GetAcceptBufferLen() - ((sizeof(sockaddr_in6) + 16) * 2)),
								sizeof(SOCKADDR_IN6) + 16,
								sizeof(SOCKADDR_IN6) + 16,
								(LPSOCKADDR*)&localAddr,
								&localLen,
								(LPSOCKADDR*)&remoteAddr,
								&remoteLen);

		memset(m_IPv6, 0x00, sizeof(m_IPv6));
		inet_ntop(	AF_INET6, 
					&(((struct sockaddr_in6 *)remoteAddr)->sin6_addr),
					m_IPv6, 
					INET6_ADDRSTRLEN);

		memcpy(&m_sockaddr_in6, remoteAddr, sizeof(m_sockaddr_in6));
		m_nIP = 0;// IPv6는 데이터를 달리 해야함
		m_nPeerPort = htons(remoteAddr->sin6_port);
		//remoteAddr->sin6_addr.u.Word; // 2바이트 어레이 8개가 실질적인 주소이다. 128비트임
	}
	else
	{
		SOCKADDR_IN *localAddr, *remoteAddr;
		int localLen = 0, remoteLen = 0;

		GetAcceptExSockaddrs(	GetAcceptBuffer(),
								//GetAcceptBufferLen() > 0 ? (DWORD)(GetAcceptBufferLen() - ((sizeof(sockaddr_in) + 16) * 2)) : 0,
								//AcceptEx 호출 하는 부분을 보시면 주석이있습니다.
								lNumberOfBytesTransferred == 0 ? 0 : (DWORD)(GetAcceptBufferLen() - ((sizeof(sockaddr_in) + 16) * 2)),
								sizeof(sockaddr_in) + 16,
								sizeof(sockaddr_in) + 16,
								(LPSOCKADDR*)&localAddr,
								&localLen,
								(LPSOCKADDR*)&remoteAddr,
								&remoteLen);

		memcpy(&m_sockaddr_in, remoteAddr, sizeof(m_sockaddr_in));
		m_nIP = remoteAddr->sin_addr.S_un.S_addr;
		m_nPeerPort = htons(remoteAddr->sin_port);
	}
}

void NetLib::cSocket::MakeConnectPacket(BYTE* pData, size_t stSize)
{
	cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

	packet.Make(0, pData, (UINT)stSize, GetSendPacketCnt());

	m_olConnect.CopyBuffer(packet.GetBuffer(), packet.GetLength());
}

#if defined(VIRTUAL_NAGLE_ON)
bool NetLib::cSocket::PopNewIocpOv()
{
	if(!m_pSendIocpOv)
	{
		m_pSendIocpOv = m_cSendOvlPool.Pop();

		if(!m_pSendIocpOv)
		{
			// SendPool에 더이상 버퍼가 없다면
			// 문제가 생긴 클라이언트로 판단..접속을 종료
			// 그렇지 않을경우 SendOv를 더 생성해주어야 하는데 
			// 그럴경우 SendOv가 MAX_BUFFER_SIZE버퍼를 넘어서기 때문..
			return false;
		}

		// 새로 꺼내온 IocpOv에 대해서만 초기화를 해준다.
		m_pSendIocpOv->Clean();
		m_pSendIocpOv->SetOperation(E_IO_SEND);
	}

	return true;
}

E_ERROR_SEND NetLib::cSocket::Send()
{
	if(!m_pSendIocpOv)
		return E_ERROR_SEND_ERROR;

	if(!m_pSendIocpOv->CheckSend())
		return E_ERROR_SEND_YET;

	DWORD dwBytes = 0, dwFlags = 0;

	int iRet = WSASend(	m_hSocket,
						&(m_pSendIocpOv->m_WsaBuf),
						1,
						&dwBytes,
						dwFlags,
						&(m_pSendIocpOv->m_Overlapped),
						NULL);

	if(iRet == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();

		if(dwLastError == WSAEWOULDBLOCK) // There are too many outstanding overlapped I/O requests. 
		{
			// 이경우는 재전송해주어야 함..
			m_cStoredOvPool.Push(m_pSendIocpOv);
			return E_ERROR_SEND_WOULDBLOCK;
		}

		if(dwLastError != WSA_IO_PENDING)
		{
			m_cSendOvlPool.Push(m_pSendIocpOv);
			return E_ERROR_SEND_SOCKET_ERROR;
		}
	}
#if defined(PACKET_ANALYZE_ON)
	IncSendPacket((DWORD)m_pSendIocpOv->m_uiDataSize);
#endif

	m_pSendIocpOv = NULL;

	if(!PopNewIocpOv())
		return E_ERROR_SEND_SEND_POOL_EMPTY;

	return E_ERROR_SEND_OK;
}

E_ERROR_SEND NetLib::cSocket::UnCheckSend()
{
	if(!m_pSendIocpOv)
		return E_ERROR_SEND_ERROR;

	if(!m_pSendIocpOv->m_uiDataSize)
		return E_ERROR_SEND_YET;

	DWORD dwBytes = 0, dwFlags = 0;

	int iRet = WSASend(	m_hSocket,
						&(m_pSendIocpOv->m_WsaBuf),
						1,
						&dwBytes,
						dwFlags,
						&(m_pSendIocpOv->m_Overlapped),
						NULL);

	if(iRet == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();

		if(dwLastError == WSAEWOULDBLOCK) // There are too many outstanding overlapped I/O requests. 
		{
			// 이경우는 재전송해주어야 함..
			m_cStoredOvPool.Push(m_pSendIocpOv);
			return E_ERROR_SEND_WOULDBLOCK;
		}

		if(dwLastError != WSA_IO_PENDING)
		{
			m_cSendOvlPool.Push(m_pSendIocpOv);
			return E_ERROR_SEND_SOCKET_ERROR;
		}
	}
#if defined(PACKET_ANALYZE_ON)
	IncSendPacket((DWORD)m_pSendIocpOv->m_uiDataSize);
#endif

	m_pSendIocpOv = NULL;

	return E_ERROR_SEND_OK;
}


E_ERROR_SEND NetLib::cSocket::SendRequest(const BYTE* pData, UINT uiDataSize)
{
	// 메모리풀에서 사용할 버퍼를 꺼낸다
	if(!PopNewIocpOv())
		return E_ERROR_SEND_SEND_POOL_EMPTY;

	// 현재 사용중인 IocpOv의 버퍼가 요구된 데이터를 다 처리할 수 있는지 체크해본다.

	UINT uiRemainDataSize = uiDataSize;
	UINT uiSent = 0;
	E_ERROR_SEND eResult;

	if(m_pSendIocpOv->m_uiDataSize)
	{
		// 먼저 AppendBuffer부터 해줌
		uiSent = m_pSendIocpOv->AppendBuffer(pData, uiDataSize);
		uiRemainDataSize -= uiSent;

		eResult = Send();

#if defined(PROTOCOL_TRACE_ON) &&  defined(_DEBUG)
		ServerManager::WriteProtocolCommandString((INT*)((VOID*)pData), false, this->GetPeerPort(), "cSocket");
#endif

		if((eResult == E_ERROR_SEND_OK) || (eResult == E_ERROR_SEND_YET))
			return E_ERROR_SEND_OK;
		else
			return eResult;

	}

	while (uiRemainDataSize > 0)
	{
		uiSent = m_pSendIocpOv->CopyBuffer(pData, uiDataSize);
		uiRemainDataSize -= uiSent;

		eResult = Send();

		if((eResult == E_ERROR_SEND_OK) || (eResult == E_ERROR_SEND_YET))
			return E_ERROR_SEND_OK;
		else
			return eResult;
	}

	return E_ERROR_SEND_OK;
}
#else
E_ERROR_SEND NetLib::cSocket::SendRequest(const BYTE* pData, UINT uiDataSize)
{
	DWORD dwBytes = 0, dwFlags = 0;

	// 메모리풀에서 사용할 버퍼를 꺼낸다
	NetLib::cIocpOv* pSndOvl = NULL;
	if(uiDataSize <= G_NET_BUFFER_SIZE_BASIC)
		pSndOvl = m_cSendOvlPool.Pop();
	else
	{
		pSndOvl = m_cSendOvlPool.Pop_32K();
		//if(pSndOvl == nullptr)
		//	pSndOvl = new NetLib::cIocpOv(G_NET_BUFFER_SIZE_32K, E_IO_NONE);// 16K버퍼 생성
	}

	if(!pSndOvl)
	{
		// SendPool에 더이상 버퍼가 없다면
		// 문제가 생긴 클라이언트로 판단..접속을 종료
		// 그렇지 않을경우 SendOv를 더 생성해주어야 하는데 
		// 그럴경우 SendOv가 MAX_BUFFER_SIZE버퍼를 넘어서기 때문..
		return E_ERROR_SEND_SEND_POOL_EMPTY;
	}

	// 메모리풀에서 꺼낸 버퍼를 초기화 하고 데이타를 복사한다
	pSndOvl->Clean();
	pSndOvl->SetOperation(E_IO_SEND);

	if(!pSndOvl->CopyBuffer(pData, uiDataSize))
	{
		PushSendOv(pSndOvl);
		return E_ERROR_SEND_DATA_SIZE_OVER;
	}

#ifdef VIRTUAL_NAGLE_ON_OFF
	if(m_bVirtualNagleOnOff)
	{
		m_packetCollecting.push_back(pSndOvl);
		return E_ERROR_SEND_OK;
	}
#endif

	int iRet = WSASend(	m_hSocket,
						&(pSndOvl->m_WsaBuf),
						1,
						&dwBytes,
						dwFlags,
						&(pSndOvl->m_Overlapped),
						NULL);

	if(iRet == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();

		if(dwLastError == WSAEWOULDBLOCK) // There are too many outstanding overlapped I/O requests. 
		{
			// 이경우는 재전송해주어야 함..
			//m_cStoredOvPool.Push( pSndOvl );
			PushSendOv(pSndOvl);
			return E_ERROR_SEND_WOULDBLOCK;
		}

		if(dwLastError != WSA_IO_PENDING)
		{
			PushSendOv(pSndOvl);
			return E_ERROR_SEND_SOCKET_ERROR;
		}
	}
#if defined(PACKET_ANALYZE_ON)
	IncSendPacket((DWORD)uiDataSize);
#endif

	//#ifdef _DEBUG
	//	cHeader* pHeader = reinterpret_cast<cHeader*>(pSndOvl->m_WsaBuf.buf);
	//
	//	cLogQueue* pLogQueue = cSingleton<cLogQueue>::ExistsInstance();
	//
	//	if(pLogQueue)
	//	{
	//		if(pHeader->GetCommand() != CSNet::CS_PING || pHeader->GetCommand() != CSNet::SC_PONG)
	//			pLogQueue->PushCommand(LOG_CRI, _T("cSocket::SendRequest E_IO_SEND Request, Command:%u, SendOvCnt:%d"), pHeader->GetCommand(), GetSendOvlCnt());
	//	}
	//#endif

	return E_ERROR_SEND_OK;
}

#ifdef VIRTUAL_NAGLE_ON_OFF
void NetLib::cSocket::SendVirtualNalePackets()
{
	cIocpOv* pSndOvl = NULL;
	DWORD dwBytes = 0, dwFlags = 0;

	for (int n = 0; n<m_packetCollecting.size(); ++n)
	{
		pSndOvl = m_packetCollecting[n];
		if(pSndOvl == nullptr)
			continue;

		int iRet = WSASend(	m_hSocket,
							&(pSndOvl->m_WsaBuf),
							1,
							&dwBytes,
							dwFlags,
							&(pSndOvl->m_Overlapped),
							NULL);
	}

	m_packetCollecting.clear();
}
#endif

void NetLib::cSocket::PushSendOv(NetLib::cIocpOv* pSndOvl)
{
	if(pSndOvl == nullptr)
		return;

	if(pSndOvl->m_uiMaxBufferLength > G_NET_BUFFER_SIZE_BASIC)
		m_cSendOvlPool.Push_32K(pSndOvl);
	else
		m_cSendOvlPool.Push(pSndOvl);
}
#endif