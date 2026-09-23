#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cServerSocket
{
protected:
	LONG	m_lAcceptingCount;
	LONG	m_lBacklog;
	SOCKET	m_hServerSocket;
	LPFN_ACCEPTEX m_lpfnAcceptEx;
	BOOL	m_bUseIPv6;

	bool	m_bUsingOutputBufferIntoAcceptRequest;

public:
	void	Init();
	void	Destroy();

public:
	bool	Startup();
	void	Cleanup();

	bool	CreateSocket(BOOL bUseIPv6 = FALSE);
	void	CloseSocket();

	bool	Bind(WORD uBindPort, BOOL bUseIPv6 = FALSE, char* lpAddress = NULL);
	bool	Listen(int iBacklog = SOMAXCONN);
	bool	LoadAcceptEx();

	bool	AcceptRequest(SOCKET hSocket, PVOID lpOutputBuffer, LPOVERLAPPED lpOverlapped, BOOL bUseIPv6 = FALSE, DWORD dwSize = 0);
	BOOL    IsUseIPv6() { return m_bUseIPv6; }

public:
	void	SetSocketHandle(SOCKET hSocket) { m_hServerSocket = hSocket; }
	SOCKET	GetSocketHandle() { return m_hServerSocket; }

	void	IncreaseAcceptingCount() { InterlockedIncrement(&m_lAcceptingCount); }
	void	DecreaseAcceptingCount() { InterlockedDecrement(&m_lAcceptingCount); }

	LONG	GetAcceptingCount() { return m_lAcceptingCount; }
	LONG	GetBacklog() { return m_lBacklog; }

	void	SetUsingOutputBufferIntoAcceptRequest(bool bUsingOutputBufferIntoAcceptRequest) { m_bUsingOutputBufferIntoAcceptRequest = bUsingOutputBufferIntoAcceptRequest; }
	bool	GetUsingOutputBufferIntoAcceptRequest() { return m_bUsingOutputBufferIntoAcceptRequest; }
private:
	std::atomic<bool> m_bShutdown{ false };

public:
	void MarkShutdown()
	{
		m_bShutdown = true;
	}

	bool IsShutdown() const
	{
		return m_bShutdown.load();
	}
public:
	cServerSocket();
	virtual ~cServerSocket();
};

END_NETLIB