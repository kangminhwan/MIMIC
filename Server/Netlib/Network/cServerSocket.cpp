#include "../../Include/Netlib/Network/cServerSocket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NetLib::cServerSocket::cServerSocket()
{
	Init();
}

NetLib::cServerSocket::~cServerSocket()
{
	Destroy();
}

void NetLib::cServerSocket::Init()
{
	m_lBacklog = 0;
	m_lAcceptingCount = 0;
	m_lpfnAcceptEx = NULL;
	SetSocketHandle(INVALID_SOCKET);
	m_bUseIPv6 = FALSE;
	m_bUsingOutputBufferIntoAcceptRequest = false;
	m_bShutdown = false;
}

void NetLib::cServerSocket::Destroy()
{
	MarkShutdown(); // 더 이상 Accept 요청 넣지 않도록
	CloseSocket();
	Cleanup();
	//m_bShutdown = false;
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

// initiates use of WS2_32.DLL 
bool NetLib::cServerSocket::Startup()
{
	WSADATA WsaData;
	if(WSAStartup(MAKEWORD(2, 2), &WsaData) == SOCKET_ERROR)
	{
		return false;
	}
	m_bShutdown = false;
	return true;
}

void NetLib::cServerSocket::Cleanup()
{
	WSACleanup();
}

// creates a socket 
bool NetLib::cServerSocket::CreateSocket(BOOL bUseIPv6)
{
	if(bUseIPv6)
	{
		m_hServerSocket = WSASocket(	AF_INET6,
										SOCK_STREAM,
										IPPROTO_TCP,
										NULL,
										NULL,
										WSA_FLAG_OVERLAPPED);

		m_bUseIPv6 = TRUE;
	}
	else
	{
		m_hServerSocket = WSASocket(	AF_INET,
										SOCK_STREAM,
										IPPROTO_TCP,
										NULL,
										NULL,
										WSA_FLAG_OVERLAPPED);

		m_bUseIPv6 = FALSE;
	}

	if(m_hServerSocket != INVALID_SOCKET)
	{
		// reuseaddr 셋팅
		int  iSockBuffer = 1;

		if(setsockopt(	m_hServerSocket,
						SOL_SOCKET,
						SO_REUSEADDR,
						(char*)&iSockBuffer,
						sizeof(iSockBuffer)) == SOCKET_ERROR)
		{
			CloseSocket();
			return false;
		}

		return true;
	}

	return false;
}

// close a socket 
void NetLib::cServerSocket::CloseSocket()
{
	if(m_hServerSocket != INVALID_SOCKET)
	{
		//shutdown( m_hServerSocket, SD_BOTH );
		closesocket(m_hServerSocket);

		SetSocketHandle(INVALID_SOCKET);
	}
}

// associates a local address with a socket
bool NetLib::cServerSocket::Bind(WORD uBindPort, BOOL bUseIPv6, char* lpAddress)
{
	if(bUseIPv6)
	{
		SOCKADDR_IN6 sin;
		memset(&sin, 0x00, sizeof(SOCKADDR_IN6));

		sin.sin6_family = AF_INET6;
		sin.sin6_flowinfo = 0;
		sin.sin6_port = htons(uBindPort);
		//sin.sin6_addr = in6addr_any;
		//sin.sin6_scope_id = if_nametoindex("eth0"); 

		if(lpAddress)
			inet_pton(AF_INET6, lpAddress, &sin);	// address must IPv6 address, "2001:720:1500:1::a100"
		else
			sin.sin6_addr = in6addr_any;

		if(bind(	m_hServerSocket,
					(SOCKADDR*)&sin,
					sizeof(sin)) == SOCKET_ERROR)
		{
			CloseSocket();
			return false;
		}
	}
	else
	{
		SOCKADDR_IN sin;

		sin.sin_family = AF_INET;
		sin.sin_port = htons(uBindPort);

		if(lpAddress)
		{
			inet_pton(sin.sin_family, lpAddress, &sin.sin_addr.s_addr);
		}
		else
		{
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		}

		if(bind(	m_hServerSocket,
					(SOCKADDR*)&sin,
					sizeof(sin)) == SOCKET_ERROR)
		{
			CloseSocket();
			return false;
		}
	}

	BOOL on = TRUE; // AcceptEx호출 이전에 OS가 connection을 받아들이지 못하도록 설정, netstat상에 대기중인 포트가 보이지 않게됨
	if(setsockopt(	m_hServerSocket,
					SOL_SOCKET,
					SO_CONDITIONAL_ACCEPT,
					(char*)&on,
					sizeof(on)) != 0)
	{
		CloseSocket();
		return false;
	}

	LINGER  lingerStruct;

	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;	// TIME_WAIT : 0 sec

	if(setsockopt(	m_hServerSocket,
					SOL_SOCKET,
					SO_LINGER,
					(char*)&lingerStruct,
					sizeof(lingerStruct)) == SOCKET_ERROR)
	{
		CloseSocket();
		return false;
	}

	//// reuseaddr 셋팅
	//int  iSockBuffer = 0;
	//BOOL bSockOpt = TRUE;
	//if( setsockopt(	m_hServerSocket, 
	//	SOL_SOCKET, 
	//	SO_REUSEADDR, 
	//	reinterpret_cast<char*>(&bSockOpt), 
	//	sizeof(iSockBuffer)) == SOCKET_ERROR ) 
	//{
	//	CloseSocket();
	//	return false;
	//}

	return true;
}

// listening for an incoming connection
bool NetLib::cServerSocket::Listen(int iBacklog)
{
	if(listen(m_hServerSocket, iBacklog) == SOCKET_ERROR)
	{
		CloseSocket();
		return false;
	}

	m_lBacklog = iBacklog;
	return true;
}

bool NetLib::cServerSocket::LoadAcceptEx()
{
	if(m_hServerSocket != INVALID_SOCKET)
	{
		DWORD dwBytes = 0;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;

		int iRet = WSAIoctl(	m_hServerSocket,
								SIO_GET_EXTENSION_FUNCTION_POINTER,
								&GuidAcceptEx,
								sizeof(GuidAcceptEx),
								&m_lpfnAcceptEx,
								sizeof(m_lpfnAcceptEx),
								&dwBytes,
								NULL,
								NULL);
		if(iRet != SOCKET_ERROR)	return true;
	}
	return false;
}

// 비동기 Accept를 미리 해둔다.
// Accept가 들어오면 워커쓰레드로 들어오게 된다.
bool NetLib::cServerSocket::AcceptRequest(SOCKET hSocket, PVOID lpOutputBuffer, LPOVERLAPPED lpOverlapped, BOOL bUseIPv6, DWORD dwSize)
{
	if (IsShutdown()) // 💣 이미 서버 소켓이 닫혔으면 더 이상 Accept 시도 안 함
		return false;
	DWORD dwBytes = 0;

	BOOL bResult = FALSE;

	if(bUseIPv6)
		bResult = AcceptEx(	m_hServerSocket,
							hSocket,
							lpOutputBuffer,
							//OutputBuffer 를 설정하고 Size를 넣어주면 해당 버퍼에 뒤에 데이터는 서버의 로컬 주소 및 클라이언트의 원격 주소를 수신하는 데이터가 오게 됩니다.
							//OutputBuffer의 사이즈에서 서버의 로컬 주소와 클라이언트의 원격 주소의 길이 만큼 빼준 값을 넣어주면 됩니다.
							// 사이즈가 0 이 들어가게되면 데이타가 들어올때까지 기다리지 않고 바로 접속을 받아들인다
							//stSize > 0 ? (DWORD)(stSize - ((sizeof(SOCKADDR_IN6) + 16) * 2)) : 0,
							//이유는 데이터를 Connect 할때 보내지 않는 Client는 AcceptEx 함수가 타임아웃 상태가 될때까지 대기상태입니다.
							//확인한 결과 40초 이상이였습니다.
							//	함수 외부에서 (DWORD)(stSize - ((sizeof(SOCKADDR_IN6) + 16) * 2))
							//	각 ip버전 별로 계산을 해서 dwSize 변수에 채워서 함수 콜을 해야합니다.
							//	그리고 아무것도 받지 않을때는 0을 채워주면 됩니다.
							dwSize,
							sizeof(SOCKADDR_IN6) + 16,
							sizeof(SOCKADDR_IN6) + 16,
							&dwBytes,
							lpOverlapped);
	else
		bResult = AcceptEx(	m_hServerSocket,
							hSocket,
							lpOutputBuffer,
							//	OutputBuffer 를 설정하고 Size를 넣어주면 해당 버퍼에 뒤에 데이터는 서버의 로컬 주소 및 클라이언트의 원격 주소를 수신하는 데이터가 오게 됩니다.
							//	OutputBuffer의 사이즈에서 서버의 로컬 주소와 클라이언트의 원격 주소의 길이 만큼 빼준 값을 넣어주면 됩니다.
							//	사이즈가 0 이 들어가게되면 데이타가 들어올때까지 기다리지 않고 바로 접속을 받아들인다
							//	stSize > 0 ? (DWORD)(stSize - ((sizeof(sockaddr_in) + 16) * 2)) : 0,//접속 이슈가 생겨서 주석 처리
							//	이유는 데이터를 Connect 할때 보내지 않는 Client는 AcceptEx 함수가 타임아웃 상태가 될때까지 대기상태입니다.
							//	확인한 결과 40초 이상이였습니다.
							//	함수 외부에서 (DWORD)(stSize - ((sizeof(sockaddr_in) + 16) * 2))
							//	각 ip버전 별로 계산을 해서 dwSize 변수에 채워서 함수 콜을 해야합니다.
							//	그리고 아무것도 받지 않을때는 0을 채워주면 됩니다.
							dwSize,
							sizeof(sockaddr_in) + 16,
							sizeof(sockaddr_in) + 16,
							&dwBytes,
							lpOverlapped);


	if(bResult == TRUE)
	{
		IncreaseAcceptingCount();
		return true;
	}
	else
	{
		DWORD dwError = GetLastError();
		if(ERROR_IO_PENDING == dwError)	// ERROR_IO_PENDING은 처리가 된것이다.
		{
			IncreaseAcceptingCount();
			return true;
		}

		// ERROR_IO_PENDING이 아니면 에러메시지 출력
		TCHAR szError[CSDef::MAX_ERROR_STRING_BUFFER_LEN] = { 0, };
		GetErrorString(szError, _countof(szError));
		_tprintf(_T("%s\n"), szError);
	}

	return false;
}
