#include "../../Include/Netlib/Network/cNetWork.h"
#include "../../Include/Netlib/Network/cServerSocket.h"
#include "../../Include/Netlib/Network/cContextPooler.h"
#include "../../Include/Netlib/IOCP/cIocp.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Common/cSingleton.h"

NetLib::cNetWork::cNetWork() :
	nConnectedUserCount(0)
{
	Init();
}


NetLib::cNetWork::~cNetWork()
{
	Destroy();
}

void NetLib::cNetWork::Init()
{
}

void NetLib::cNetWork::Destroy()
{
	DestroyServerSocket();
}

bool NetLib::cNetWork::NetworkStartup()
{
	WSADATA WsaData;
	if(WSAStartup(MAKEWORD(2, 2), &WsaData) == SOCKET_ERROR)
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("WSAStartup Error :: %lu"), WSAGetLastError());
		return false;
	}
	return true;
}

void NetLib::cNetWork::NetworkCleanup()
{
	WSACleanup();
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

// Socket 생성
bool NetLib::cNetWork::CreateServerIPv4Socket(WORD nPort, int iBacklog, UINT& nID, bool bUsingOutputBufferIntoAcceptRequest)
{
	NetLib::cServerSocket* pServerSocket = new NetLib::cServerSocket;
	if(pServerSocket)
	{
		// ws2_32 초기화0
		if(!pServerSocket->Startup())
			goto CreateServerSocketError;

		// 서버소켓 생성
		if(!pServerSocket->CreateSocket())
			goto CreateServerSocketError;

		if(!pServerSocket->Bind(nPort))
			//if( !pServerSocket->Bind(nPort, "192.168.11.22") )
		{
			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: Bind(%d) failed"), nPort);
			goto CreateServerSocketError;
		}

		// Backlog카운트 만큼 Accept대기
		if(!pServerSocket->Listen(iBacklog))
			goto CreateServerSocketError;

		// AcceptEx를 사용하도록 소켓의 모드 제어.
		if(!pServerSocket->LoadAcceptEx())
			goto CreateServerSocketError;

		// Accept 할때 Connect하는 쪽에서 보내는 버퍼를 받을것인지 안받을것인지를 제어
		pServerSocket->SetUsingOutputBufferIntoAcceptRequest(bUsingOutputBufferIntoAcceptRequest);

		m_cServerSocketList.push_back(pServerSocket);

		nID = (UINT)m_cServerSocketList.size() - 1;

		NetLib::cIOCP* pIOCP = NetLib::cSingleton<NetLib::cIOCP>::GetInstance();
		if(pIOCP->AssocInstance(reinterpret_cast<HANDLE>(pServerSocket->GetSocketHandle()), reinterpret_cast<UINT_PTR>(pServerSocket)) == false)
			goto CreateServerSocketError;

		// nBackLog만큼 Accept요청
		if(!BackLogAcceptRequest(nID))
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("BackLogAcceptRequest failed."));
			//NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("ServerManager::ServerListen() failed."));
			return false;
		}
	}

	return true;

CreateServerSocketError:
	if(pServerSocket)
		delete pServerSocket;

	return false;
}

bool NetLib::cNetWork::CreateServerIPv6Socket(WORD nPort, int iBacklog, UINT& nID, bool bUsingOutputBufferIntoAcceptRequest)
{
	NetLib::cServerSocket* pServerSocket16 = new NetLib::cServerSocket;
	if(pServerSocket16)
	{
		if(!pServerSocket16->CreateSocket(TRUE))
			goto CreateServerSocketError;

		if(!pServerSocket16->Bind(nPort, TRUE))
		{
			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: IPv6 Bind(%d) failed"), nPort);
			goto CreateServerSocketError;
		}

		if(!pServerSocket16->Listen(iBacklog))
			goto CreateServerSocketError;

		if(!pServerSocket16->LoadAcceptEx())
			goto CreateServerSocketError;

		pServerSocket16->SetUsingOutputBufferIntoAcceptRequest(bUsingOutputBufferIntoAcceptRequest);

		m_cServerSocketList.push_back(pServerSocket16);

		nID = (UINT)m_cServerSocketList.size() - 1;

		NetLib::cIOCP* pIOCP = NetLib::cSingleton<NetLib::cIOCP>::GetInstance();
		if(pIOCP->AssocInstance(reinterpret_cast<HANDLE>(pServerSocket16->GetSocketHandle()), reinterpret_cast<ULONG_PTR>(pServerSocket16)) == false)
			goto CreateServerSocketError;

		// nBackLog만큼 Accept요청
		if(!BackLogAcceptRequest(nID))
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("BackLogAcceptRequest failed."));
			//NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("ServerManager::ServerListen() failed."));
			return false;
		}
	}

	return true;

CreateServerSocketError:
	if(pServerSocket16)
		delete pServerSocket16;

	return false;
}

// Socket 삭제
void NetLib::cNetWork::DestroyServerSocket()
{
	std::vector<NetLib::cServerSocket*>::iterator it;
	for (UINT i = 0; i < m_cServerSocketList.size(); i++)
	{
		NetLib::cServerSocket* pServerSocket = m_cServerSocketList[i];
		if(pServerSocket)
		{
			delete pServerSocket;
			pServerSocket = NULL;
		}
	}
	m_cServerSocketList.clear();
}

// Accept요청
bool NetLib::cNetWork::AcceptRequest(UINT nID)
{

	// ServerSocket을 얻어낸다
	NetLib::cServerSocket* pServerSocket = m_cServerSocketList.at(nID);
	if(!pServerSocket)
	{
		// 이 경우는 발생할 수 없다.
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("[%lu]NetLib::AcceptRequest failed - critical no ServerSocket!!!"), GetCurrentThreadId());
		return false;
	}

	// backlog가 이미 확보 되어 있으면, 더이상 신청 하지 않음
	if(pServerSocket->GetAcceptingCount() >= pServerSocket->GetBacklog())
		return false;

	// 10번 루프 돌면서 풀러에서 받아온다.
	cIocpContext* pIocpContext = NULL;
	for (int nLoop = 0; nLoop<10; ++nLoop)
	{
		pIocpContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->Pop();
		if(pIocpContext)
			break;

		Sleep(1);
	}

	// 루프를 돌아도 못 가져 올 경우에는 컨텍스트 생성한다.
	if(pIocpContext == nullptr)
		pIocpContext = new NetLib::cIocpContext();

	if(pIocpContext == nullptr)
		return false;

	pIocpContext->SetPortID(nID);
	pIocpContext->CleanOverlapped(E_IO_ACCEPT);

	if(pServerSocket->IsUseIPv6())
		pIocpContext->ReCreateSocket(pServerSocket->IsUseIPv6());

	DWORD dwSize = 0;

	// AcceptEx 를 할때 버퍼를 같이 받아야할 Listen 소켓인지 검사합니다.
	if (pServerSocket->GetUsingOutputBufferIntoAcceptRequest())
	{
		size_t stSocketAddrSize = pServerSocket->IsUseIPv6() ? sizeof(SOCKADDR_IN6) : sizeof(sockaddr_in);
		dwSize = static_cast<DWORD>(pIocpContext->GetAcceptBufferLen() - (stSocketAddrSize + 16) * 2);
	}

	while (!pServerSocket->AcceptRequest(	pIocpContext->NetLib::cSocket::GetSockHandle(),
											pIocpContext->GetAcceptBuffer(),
											pIocpContext->GetOverlapped(E_IO_ACCEPT),
											pServerSocket->IsUseIPv6(),
											dwSize))
	{
		// AcceptEx가 실패한놈들은 다른곳에다가 모아둔다.
		//pContextPooler->FailedAcceptPush( pIocpContext );
		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("[%lu]NetLib::AcceptRequest failed - AcceptEx error"), GetCurrentThreadId());

		if(pServerSocket->IsUseIPv6())
			pIocpContext->ReCreateSocket(pServerSocket->IsUseIPv6());
		else
			pIocpContext->ReCreateSocket();

		Sleep(1);
	}
	return true;
}

bool NetLib::cNetWork::BackLogAcceptRequest(UINT nID)
{
	// ServerSocket을 얻어낸다
	NetLib::cServerSocket* pServerSocket = m_cServerSocketList.at(nID);
	if(!pServerSocket)
	{
		// 이 경우는 발생할 수 없다.
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("[%lu]NetLib::BackLogAcceptRequest failed - critical no ServerSocket!!!"), GetCurrentThreadId());
		return false;
	}

	// backlog만큼 Accept요청
	NetLib::cContextPooler* pContextPooler = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance();

	while (pServerSocket->GetAcceptingCount() < pServerSocket->GetBacklog())
	{
		NetLib::cIocpContext* pIocpContext = pContextPooler->Pop();
		if(!pIocpContext)
		{
			// 메모리 풀이 비어있다.
			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("[%lu]NetLib::BackLogAcceptRequest failed - ContextPool empty"), GetCurrentThreadId());
			break;
			//continue;
		}

		pIocpContext->SetPortID(nID);
		pIocpContext->CleanOverlapped(E_IO_ACCEPT);

		// 만약에 cIocpContext가 IPv4라면 소켓을 IPv6용으로 재생성해서 처리합니다.
		if(pServerSocket->IsUseIPv6())
			pIocpContext->ReCreateSocket(pServerSocket->IsUseIPv6());

		DWORD dwSize = 0;

		// AcceptEx 를 할때 버퍼를 같이 받아야할 Listen 소켓인지 검사합니다.
		if (pServerSocket->GetUsingOutputBufferIntoAcceptRequest())
		{
			size_t stSocketAddrSize = pServerSocket->IsUseIPv6() ? sizeof(SOCKADDR_IN6) : sizeof(sockaddr_in);
			dwSize = static_cast<DWORD>(pIocpContext->GetAcceptBufferLen() - (stSocketAddrSize + 16) * 2);
		}

		if(!pServerSocket->AcceptRequest(	pIocpContext->NetLib::cSocket::GetSockHandle(),
											pIocpContext->GetAcceptBuffer(),
											pIocpContext->GetOverlapped(E_IO_ACCEPT),
											pServerSocket->IsUseIPv6(),
											dwSize))
		{
			Sleep(1);

			pIocpContext->ReCreateSocket(pServerSocket->IsUseIPv6());

			// AcceptEx가 실패한놈들은 다른곳에다가 모아둔다.
			pContextPooler->PushContext(pIocpContext);

			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("[%lu]NetLib::BackLogAcceptRequest failed - AcceptEx error"), GetCurrentThreadId());
			continue;
		}
		/*else
		{
		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( RED, _T("[%lu]NetLib::GetAcceptingCount %d"), GetCurrentThreadId(), pServerSocket->GetAcceptingCount() );
		}*/
	}

	if(pServerSocket->GetAcceptingCount() == 0)
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("[%lu]NetLib::BackLogAcceptRequest failed - network error"), GetCurrentThreadId());
	}

	return true;
}

// Accept 요청이 완료된 후의 처리
void NetLib::cNetWork::AcceptCompleted(cIocpContext* pContext)
{
	if (pContext == nullptr)
	{
		return;
	}

	UINT nID = pContext->GetPortID();

	// ServerSocket을 얻어낸다
	NetLib::cServerSocket* pServerSocket = m_cServerSocketList.at(nID);
	if(pServerSocket)
	{
		pServerSocket->DecreaseAcceptingCount();
	}

	//NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->InsertContext(pContext);
}

NetLib::cIocpContext* NetLib::cNetWork::PopContext()
{
	return NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->Pop();
}

// 모든 일을 마친 Context에 대한 처리
void NetLib::cNetWork::FinishAndAccept(NetLib::cIocpContext* pIocpContext)
{
	UINT nID = pIocpContext->GetPortID();
	NetLib::cContextPooler* pContextPooler = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance();

	pContextPooler->PushContext(pIocpContext);

	//cSingleton<cLogQueue>::GetInstance()->PushCommand( _T("IocpContext Push, entity[%lu]"), pIocpContext->GetEntity());

	// Accept요청이 없을 경우 요청하도록 함
	NetLib::cServerSocket* pServerSocket = m_cServerSocketList.at(nID);
	if(pServerSocket)
	{
		AcceptRequest(nID);
	}
}

void NetLib::cNetWork::FinishErrorContext(cIocpContext* pIocpContext)
{
	UINT nID = pIocpContext->GetPortID();
	NetLib::cContextPooler* pContextPooler = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance();

	pContextPooler->PushErrorContext(pIocpContext);
	
	// Accept요청이 없을 경우 요청하도록 함
	NetLib::cServerSocket* pServerSocket = m_cServerSocketList.at(nID);
	if (pServerSocket)
	{
		AcceptRequest(nID);
	}
}

LONG NetLib::cNetWork::AcceptCountReport()
{
	LONG lAcceptCount = 0;

	std::vector<cServerSocket*>::const_iterator conIter = m_cServerSocketList.begin();
	std::vector<cServerSocket*>::const_iterator conIterEnd = m_cServerSocketList.end();

	for (; conIter != conIterEnd; ++conIter)
	{
		if ((*conIter) != nullptr)
		{
			lAcceptCount += (*conIter)->GetAcceptingCount();
		}
	}

	return lAcceptCount;
}