#include "../../Include/Netlib/UdpModule/cIocpUDP.h"
#include "../../Include/Netlib/UdpModule/cUDPDispatcher.h"
#include "../../Include/Netlib/UdpModule/cUDPSessionManager.h"
#include "../../Include/Netlib/UdpModule/cUDPSession.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Common/cInterfaceLog.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Common/cHeader.h"
#include "../../Include/Netlib/Common/cUdpHeader.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Network/cContextPooler.h"
#include "../../Include/Netlib/Session/cSession.h"

#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#endif

#define PACKETSIZE 40
#define TIMER_EVENT_MESSAGE			0x12345678
#define TERMINATE_THREAD_MESSAGE	0x12345679

NetLib::cIocpUDP::cIocpUDP() :
	pInterfaceLog(nullptr),
	m_nUdpPort(0),
	m_hCompletionPort(nullptr),
	m_hIocpPort(nullptr),
	m_iWorkerThreadCnt(0),
	m_iThreadArrayNum(0),
	m_pPerThreadPacket(nullptr),
	m_pUDPDispatcher(nullptr),
	m_pUDPSessionMgr(nullptr),
	m_uiThreadID(nullptr)
{
	Init();
}


NetLib::cIocpUDP::~cIocpUDP()
{
	Destroy();
}

void NetLib::cIocpUDP::Init()
{
	Startup();

	m_hUdpSocketIPv4 = INVALID_SOCKET;
	//m_hUdpSocketIPv6 = INVALID_SOCKET;

	m_hUdpSocketIPv4 = CreateIPv4Socket();
	ExtendSockBuffer(m_hUdpSocketIPv4);// IPv4 UDP 소켓 확장

	//m_hUdpSocketIPv6 = CreateIPv6Socket();
	//ExtendSockBuffer(m_hUdpSocketIPv6);// IPv6 UDP 소켓 확장

	m_pUDPDispatcher = new cUDPDispatcher;
	//m_pUDPDispatcher->SetUDPSocket(m_hUdpSocketIPv4, m_hUdpSocketIPv6);
	m_pUDPDispatcher->SetUDPSocket(m_hUdpSocketIPv4);

	//m_pUDPSessionMgr = cUDPSessionManager::GetInstance();
	m_pUDPSessionMgr = cSingleton<cUDPSessionManager>::GetInstance();

	m_dwTimerCompletionKey = TIMER_EVENT_MESSAGE;
	m_dwFinishThreadCompletionKey = TERMINATE_THREAD_MESSAGE;

	for (int n = 0; n<MAX_WORKER_THREADS; ++n)
	{
		m_hUDPThread[n] = INVALID_HANDLE_VALUE;
	}
}

void NetLib::cIocpUDP::Startup()
{
	WSADATA WsaData;

	if(WSAStartup(MAKEWORD(2, 2), &WsaData) == SOCKET_ERROR)
	{
		PrintLastError(__FILE__, __LINE__);
		return;
	}
}

SOCKET NetLib::cIocpUDP::CreateIPv4Socket()
{
	CloseSocket(m_hUdpSocketIPv4);

	return WSASocket(AF_INET,
		SOCK_DGRAM,
		IPPROTO_UDP,
		NULL,
		NULL,
		WSA_FLAG_OVERLAPPED);
}

//SOCKET NetLib::cIocpUDP::CreateIPv6Socket()
//{
//	CloseSocket(m_hUdpSocketIPv6);
//
//	return WSASocket(AF_INET6,
//		SOCK_DGRAM,
//		IPPROTO_UDP,
//		NULL,
//		NULL,
//		WSA_FLAG_OVERLAPPED);
//}

void NetLib::cIocpUDP::CloseSocket(SOCKET udpsocket)
{
	if(udpsocket != INVALID_SOCKET)
	{
		//shutdown( m_hUdpSocket, SD_BOTH );
		closesocket(udpsocket);

		udpsocket = INVALID_SOCKET;
	}
}

bool NetLib::cIocpUDP::ExtendSockBuffer(SOCKET udpsocket)
{
	if(udpsocket == INVALID_SOCKET)
		return false;

	int nSendBuf;
	int nRecvBuf;
	int state;
	int nBufferlen;

	int  iSockBuffer = 1;

	state = setsockopt(udpsocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iSockBuffer, sizeof(iSockBuffer));
	if(state == SOCKET_ERROR)	return false;

	// SendBuffer
	nBufferlen = sizeof(nSendBuf);
	state = getsockopt(udpsocket, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuf, &nBufferlen);
	if(state == SOCKET_ERROR)	return false;
	//printf("기본 전송 버퍼의 크기: %d", nSendBuf);

	nSendBuf *= 10;
	//nSendBuf = 0;

	state = setsockopt(udpsocket, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuf, nBufferlen);
	if(state == SOCKET_ERROR)	return false;

	state = getsockopt(udpsocket, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuf, &nBufferlen);
	//Trace( _T("cUdpSocket::ExtendSockBuffer() 수정된 send 버퍼의 크기: %d\n"), nSendBuf);


	// RecvBuffer
	nBufferlen = sizeof(nRecvBuf);
	state = getsockopt(udpsocket, SOL_SOCKET, SO_RCVBUF, (char*)&nRecvBuf, &nBufferlen);
	//printf("기본 수신 버퍼의 크기: %d", nRecvBuf);

	nRecvBuf *= 10;
	//nRecvBuf = 0;

	state = setsockopt(udpsocket, SOL_SOCKET, SO_RCVBUF, (char*)&nRecvBuf, nBufferlen);
	if(state == SOCKET_ERROR)	return false;

	state = getsockopt(udpsocket, SOL_SOCKET, SO_SNDBUF, (char*)&nRecvBuf, &nBufferlen);
	//Trace( _T("cUdpSocket::ExtendSockBuffer() 수정된 recv 버퍼의 크기: %d\n"), nRecvBuf);

	return true;
}

void NetLib::cIocpUDP::Destroy()
{
	for (int n = 0; n < m_iWorkerThreadCnt; ++n)
	{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, nullptr);
	}

	if(m_pPerThreadPacket)
	{
		delete[]m_pPerThreadPacket;
		m_pPerThreadPacket = nullptr;
	}

	if(m_uiThreadID)
	{
		delete[]m_uiThreadID;
		m_uiThreadID = nullptr;
	}

	if( m_pUDPDispatcher )
	{
		delete m_pUDPDispatcher;
		m_pUDPDispatcher = nullptr;
	}

	WSACleanup();
}

bool NetLib::cIocpUDP::CreateIocp()
{
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if(m_hCompletionPort == nullptr)
	{
		PrintLastError(__FILE__, __LINE__);
		return false;
	}

	// m_hUdpSocketIPv4 IOCP 등록
	DWORD dwCompletionKey = 1;
	m_hIocpPort = CreateIoCompletionPort(	reinterpret_cast<HANDLE>(m_hUdpSocketIPv4),
											m_hCompletionPort,
											reinterpret_cast<ULONG_PTR>(&dwCompletionKey),
											0);

	if(m_hIocpPort == nullptr)
	{
		PrintLastError(__FILE__, __LINE__);
		return false;
	}

	//// m_hUdpSocketIPv6 IOCP 등록
	//dwCompletionKey = 2;
	//m_hIocpPort = CreateIoCompletionPort(	reinterpret_cast<HANDLE>(m_hUdpSocketIPv6),
	//										m_hCompletionPort,
	//										reinterpret_cast<ULONG_PTR>(&dwCompletionKey),
	//										0);

	//if(m_hIocpPort == nullptr)
	//{
	//	PrintLastError(__FILE__, __LINE__);
	//	return false;
	//}

	return true;
}

bool NetLib::cIocpUDP::Bind(int iPort)
{
	SOCKADDR_IN sin;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(iPort);
	sin.sin_addr.s_addr = INADDR_ANY;

	if(bind(	m_hUdpSocketIPv4,
				(LPSOCKADDR)&sin,
				sizeof(sin)) == SOCKET_ERROR)
	{
		PrintLastError(__FILE__, __LINE__);
		closesocket(m_hUdpSocketIPv4);
		return false;
	}

	m_nUdpPort = iPort;

	return true;
}

//bool NetLib::cIocpUDP::BindIPv6(int iPort)
//{
//	SOCKADDR_IN6 sin;
//	memset(&sin, 0x00, sizeof(SOCKADDR_IN6));
//
//	sin.sin6_family = AF_INET6;
//	sin.sin6_flowinfo = 0;
//	sin.sin6_port = htons(iPort);
//	sin.sin6_addr = in6addr_any;
//
//	if(bind(	m_hUdpSocketIPv6,
//				(LPSOCKADDR)&sin,
//				sizeof(sin)) == SOCKET_ERROR)
//	{
//		PrintLastError(__FILE__, __LINE__);
//		closesocket(m_hUdpSocketIPv6);
//		return false;
//	}
//
//	return true;
//}

UINT NetLib::cIocpUDP::WokerThread()
{
	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_UDP);
	BOOL	bSuccess = FALSE;
	DWORD	dwTransferredBytes;

	NetLib::cUDPIOCompletionData*	completionData = NULL;
	NetLib::cUDPIocpOv* pIocpOv = nullptr;

	ULONG_PTR	lpCompletionKey = 0;

	int* pPerThreadPacket = NULL;

	DWORD dwCurrentThread = ::GetCurrentThreadId();

	m_csLock.Lock();
	pPerThreadPacket = &m_pPerThreadPacket[m_iThreadArrayNum];
	*pPerThreadPacket = 0;

	++m_iThreadArrayNum;
	m_csLock.Unlock();

	if(!pInterfaceLog)
		return 0;

	NetLib::cCommandQueueManager* pCommandQueueManager = cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	if (!pCommandQueueManager)
	{
		throw("NetLib::cIocpUDP::WokerThread() cCommandQueueManager is nullptr");
	}

	while (true)
	{
		bSuccess = GetQueuedCompletionStatus(	m_hCompletionPort,
												&dwTransferredBytes,
												&lpCompletionKey,
												(LPOVERLAPPED*)&pIocpOv,
												INFINITE);

		// ExitCode
		if(lpCompletionKey == NULL)
		{
			return 0;
		}

		// TimerEvent처리, 외부에서 PostQueuedCompletionStatus로 호출
		if(lpCompletionKey == TIMER_EVENT_MESSAGE)
		{
			// 타이머 이벤트 처리.
			//if( pInterfacePacketParserUDP->GetTimerEvent() )
			if(m_pUDPSessionMgr->DeleteTimeOutSession())
			{
				pInterfaceLog->PushCommand(LOG_NOR, _T("GetTimerEvent() delete udp session, success"));
			}
			continue;
		}
		else if(lpCompletionKey == TERMINATE_THREAD_MESSAGE)
		{
			return 0;
		}

		pIocpOv = reinterpret_cast<NetLib::cUDPIocpOv*>(reinterpret_cast<BYTE*>(pIocpOv) - sizeof(BYTE*));

		completionData = CONTAINING_RECORD(pIocpOv, NetLib::cUDPIOCompletionData, m_pUDPIocpOv);

		pIocpOv = completionData->GetUDPIocpOv();
		//BOOL bUseIPv6 = pIocpOv->GetIPv6();

		if(!completionData && !bSuccess)
		{
			if(WSAGetLastError() != WAIT_TIMEOUT)
			{
				pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP lpOverlapped NULL, bSuccess NULL "));
				PrintLastError(__FILE__, __LINE__);
			}
			continue;
		}

		if(pIocpOv->GetOperation() == E_IO_RECEIVE)
		{
			if(bSuccess)
			{
				if(dwTransferredBytes == 0)
				{
					pInterfaceLog->PushCommand(LOG_NOR, _T("GQCS dwTransferredBytes = 0"));
					pInterfaceLog->PushCommand(LOG_NOR, _T("GQCS error %u"), PrintLastError());
					m_pUDPDispatcher->RecvIOCompletion(completionData);
					m_pUDPDispatcher->ReceiveRequest();
					continue;
				}

				++(*pPerThreadPacket);

				if(!packet.CopyPacket(reinterpret_cast<BYTE*>(pIocpOv->m_WsaBuf.buf), dwTransferredBytes))
				{
					// 디코딩 실패
					NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("cWorkerThread::PushCommand::Decrypt Failed"));
					//OnClose( pIocpContext );
					return false;
				}

				packet.Decrypt();

				NetLib::cHeader* pHeader = reinterpret_cast<NetLib::cHeader*>(packet.GetBuffer());
				UINT command = packet.GetUDPCommand();

				// 정상적인 패킷일 경우에만 UDP 패킷 처리를 합니다.
				if(CheckPacket(pHeader, dwTransferredBytes))
				{
					// Entity로 세션을 찾아 옵시다. how? cContextPooler를 통해
					NetLib::cCommandQueue* pCommandQueue = cSingleton<NetLib::cContextPooler>::GetInstance()->FindCommandQueueArray(pHeader->GetEntity(), -1);

					if (pCommandQueue != nullptr)
						pCommandQueue->PushCommand(m_pUDPDispatcher,
							command,
							pHeader->GetEntity(),
							pHeader->GetPacketNum(),
							pIocpOv,
							//&updateudp.sockaddr,
							packet.GetBody(),
							packet.GetPayLoad());
				}
				else
				{
					pInterfaceLog->PushCommand(LOG_NOR, _T("UDP Packet Invalid Command:%u, Length:%u"), command, dwTransferredBytes);
				}
			}

			m_pUDPDispatcher->RecvIOCompletion(completionData);

			// ReceiveRequest 를 다시 걸어줌.
			//while (m_pUDPDispatcher->ReceiveRequest(bUseIPv6) != E_ERROR_SEND_OK);
			while (m_pUDPDispatcher->ReceiveRequest() != E_ERROR_SEND_OK);
		}
		else if(pIocpOv->GetOperation() == E_IO_SEND)
		{
			if(!bSuccess)
			{
				// 실패 했을 경우에는 삭제해 주어야 하는데;
				NetLib::cHeader* pUdpHeader = reinterpret_cast<NetLib::cHeader*>(pIocpOv->m_WsaBuf.buf);
				//m_pUDPSessionMgr->RemoveSessionByKey(pUdpHeader->GetEntity());
			}
			m_pUDPDispatcher->SendIOCompletion(completionData);
		}
		else
		{
			pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP 이메세지는 모꼬.."));
		}
	}
	return 0;
}

bool NetLib::cIocpUDP::CheckPacket(NetLib::cHeader* pHeader, UINT nLength)
{
	assert(pInterfaceLog);

	if(!pHeader)
	{
#ifdef _DEBUG
		pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP::CheckPacket Failed. pHeader is null"));
#endif
		return false;
	}

	// Entity가 없으면 패킷 무시
	if(!pHeader->GetEntity())
	{
#ifdef _DEBUG
		pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP::CheckPacket Failed. Entity not set"));
#endif
		return false;
	}

	// 패킷의 헤더길이 검사
	if(nLength < sizeof(NetLib::cHeader))
	{
#ifdef _DEBUG
		pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP::CheckPacket Failed. nLength:%d < sizeof(cHeader):%d"), nLength, sizeof(NetLib::cHeader));
#endif
		return false;
	}

	// 패킷의 유효성 검사
	if(!pHeader->IsPerfect())
	{
#ifdef _DEBUG
		pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP::CheckPacket Failed. !(pHeader->IsPerfect())"));
#endif
		return false;
	}

	if(pHeader->GetPayload() > G_DEFIOBUFFERLEN)
	{
#ifdef _DEBUG
		pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP::CheckPacket Failed. ( pHeader->GetPayload():%d > G_DEFIOBUFFERLEN )"), pHeader->GetPayload(), G_DEFIOBUFFERLEN);
#endif
		return false;
	}

	// 패킷 헤더에 따른 길이 검사
	if(nLength != sizeof(NetLib::cHeader) + pHeader->GetPayload())
	{
#ifdef _DEBUG
		pInterfaceLog->PushCommand(LOG_NOR, _T("cIocpUDP::CheckPacket Failed. ( nLength:%d != sizeof(cUdpHeader):%d + pHeader->GetPayload() ) "), nLength, sizeof(NetLib::cUdpHeader), pHeader->GetPayload());
#endif
		return false;
	}

	return true;
}

unsigned int __stdcall NetLib::cIocpUDP::WorkerThread(void *pVoid)
{
	NetLib::cIocpUDP *pIocp = static_cast<NetLib::cIocpUDP *>(pVoid);

	if(!pIocp)
	{
		Trace(_T("UDP Threads Exit\n"));
		return 0;
	}

	pIocp->WokerThread();

	Trace(_T("UDP Threads Exit\n"));
	return 1;
}

bool NetLib::cIocpUDP::StartIocpUDPModule(int iServerPort, int iWorkerThredCnt)
{
	m_iWorkerThreadCnt = iWorkerThredCnt;

	// Iocp 생성
	if(!CreateIocp())
	{
		pInterfaceLog->PushCommand(LOG_CRI, _T("cIocpUDP CreateIocp Failed"));
		return false;
	}

	// Bind
	if(!Bind(iServerPort))
	{
		pInterfaceLog->PushCommand(LOG_CRI, _T("cIocpUDP Bind Failed"));
		return false;
	}

	/*if(!BindIPv6(iServerPort))
	{
		pInterfaceLog->PushCommand(LOG_CRI, _T("cIocpUDP BindIPv6 Failed"));
		return false;
	}*/

	m_pPerThreadPacket = new int[m_iWorkerThreadCnt];

	m_uiThreadID = new unsigned int[m_iWorkerThreadCnt];


	for (int n = 0; n<m_iWorkerThreadCnt; ++n)
	{
		m_hUDPThread[n] = (HANDLE)_beginthreadex(	NULL,
													0,
													WorkerThread,
													this,
													0,
													&m_uiThreadID[n]);

		if(m_hUDPThread[n] == INVALID_HANDLE_VALUE)
		{
			return false;
		}
	}

	// IocpOv에 대해 멀티쓰레드일경우 Lock을 적용해야 한다.
	BOOL bThreadSafe = FALSE;

	if(m_iWorkerThreadCnt > 1)
	{
		bThreadSafe = TRUE;
	}

	// Thread 갯수만큼 Recv걸어둔다.
	//cIocpOv* pIocpOv = NULL;
	for (int n = 0; n<m_iWorkerThreadCnt; ++n)
	{
		m_pUDPDispatcher->ReceiveRequest();		// IPv4 Receive
		m_pUDPDispatcher->ReceiveRequest(TRUE);	// IPv6 Receive
	}

	NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(BLACK, _T("cIocpUDP Start Success, Port(%d) UDP WorkerThreadCnt(%d)\n"), iServerPort, m_iWorkerThreadCnt);

	return true;
}

void NetLib::cIocpUDP::EndIocpUDPModule()
{
	DWORD dwCompletionkey = TERMINATE_THREAD_MESSAGE;
	for (int n = 0; n<m_iWorkerThreadCnt; ++n)
	{
		//PostQueuedCompletionStatus( m_hCompletionPort, 0, dwCompletionkey , NULL );
		PostQueuedCompletionStatus(m_hCompletionPort, 0, m_dwFinishThreadCompletionKey, NULL);
	}
}

void NetLib::cIocpUDP::SetTimerEvent()
{
	DWORD dwCompletionkey = TIMER_EVENT_MESSAGE;
	//PostQueuedCompletionStatus( m_hCompletionPort, 0, dwCompletionkey , NULL );
	PostQueuedCompletionStatus(m_hCompletionPort, 0, m_dwTimerCompletionKey, NULL);
}

bool NetLib::cIocpUDP::WaitThreadTermination()
{
	DWORD dwWaitRet = 0xFFFFFFFF;

	dwWaitRet = WaitForMultipleObjects(m_iWorkerThreadCnt, (CONST HANDLE*)m_hUDPThread, TRUE, INFINITE);
	CloseSocket(m_hUdpSocketIPv4);
	//CloseSocket(m_hUdpSocketIPv6);

	if(dwWaitRet != WAIT_OBJECT_0)
	{
		return false;
	}

	return true;
}