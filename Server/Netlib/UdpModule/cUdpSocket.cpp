#include "../../Include/Netlib/UdpModule/cUdpSocket.h"

NetLib::cUdpSocket::cUdpSocket() :
	m_hUdpSocket(INVALID_SOCKET)
{
	Init();
}


NetLib::cUdpSocket::~cUdpSocket()
{
	Destroy();
}

void NetLib::cUdpSocket::Init()
{
	CreateSocket();
	ExtendSockBuffer();
}

void NetLib::cUdpSocket::Destroy()
{
	CloseSocket();
}

//////////////////////////////////////////////////////////////////////
// Operation 
//////////////////////////////////////////////////////////////////////

BOOL NetLib::cUdpSocket::CreateSocket()
{
	CloseSocket();

	m_hUdpSocket = WSASocket(AF_INET,
		SOCK_DGRAM,
		IPPROTO_UDP,
		NULL,
		NULL,
		WSA_FLAG_OVERLAPPED);

	if(m_hUdpSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	return TRUE;
}

bool NetLib::cUdpSocket::ExtendSockBuffer()
{
	int nSendBuf;
	int nRecvBuf;
	int state;
	int nBufferlen;

	int  iSockBuffer = 1;

	state = setsockopt(m_hUdpSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iSockBuffer, sizeof(iSockBuffer));
	if(state == SOCKET_ERROR)	return false;

	// SendBuffer
	nBufferlen = sizeof(nSendBuf);
	state = getsockopt(m_hUdpSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuf, &nBufferlen);
	if(state == SOCKET_ERROR)	return false;
	//printf("기본 전송 버퍼의 크기: %d", nSendBuf);

	nSendBuf *= 10;
	//nSendBuf = 0;

	state = setsockopt(m_hUdpSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuf, nBufferlen);
	if(state == SOCKET_ERROR)	return false;

	state = getsockopt(m_hUdpSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuf, &nBufferlen);
	//Trace( _T("cUdpSocket::ExtendSockBuffer() 수정된 send 버퍼의 크기: %d\n"), nSendBuf);


	// RecvBuffer
	nBufferlen = sizeof(nRecvBuf);
	state = getsockopt(m_hUdpSocket, SOL_SOCKET, SO_RCVBUF, (char*)&nRecvBuf, &nBufferlen);
	//printf("기본 수신 버퍼의 크기: %d", nRecvBuf);

	nRecvBuf *= 10;
	//nRecvBuf = 0;

	state = setsockopt(m_hUdpSocket, SOL_SOCKET, SO_RCVBUF, (char*)&nRecvBuf, nBufferlen);
	if(state == SOCKET_ERROR)	return false;

	state = getsockopt(m_hUdpSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nRecvBuf, &nBufferlen);
	//Trace( _T("cUdpSocket::ExtendSockBuffer() 수정된 recv 버퍼의 크기: %d\n"), nRecvBuf);

	return true;
}

SOCKET	NetLib::cUdpSocket::GetSockHandle()
{ 
	return m_hUdpSocket; 
}

void NetLib::cUdpSocket::CloseSocket()
{
	if(m_hUdpSocket != INVALID_SOCKET)
	{
		//shutdown( m_hUdpSocket, SD_BOTH );
		closesocket(m_hUdpSocket);

		m_hUdpSocket = INVALID_SOCKET;
	}
}

void NetLib::cUdpSocket::Disconnect()
{
	closesocket(m_hUdpSocket);
}