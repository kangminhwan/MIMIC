#include "../../Include/Netlib/IOCP/cIocp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NetLib::cIOCP::cIOCP(void)
{
	Init();
}

NetLib::cIOCP::~cIOCP(void)
{
	Destroy();
}

void NetLib::cIOCP::Init()
{
	m_hCompletionPort = NULL;
}

void NetLib::cIOCP::Destroy()
{
	Close();
}

//////////////////////////////////////////////////////////////////////
// Operation
//////////////////////////////////////////////////////////////////////

// create new I/O completion port
bool NetLib::cIOCP::Create()
{
	m_hCompletionPort = CreateIoCompletionPort(	INVALID_HANDLE_VALUE,
												NULL,
												0,
												0);

	return (m_hCompletionPort ? true : false);
}

// destroy I/O completion port
void NetLib::cIOCP::Close()
{
	if(m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
}

// associate an instance
bool NetLib::cIOCP::AssocInstance(HANDLE hSocket, ULONG_PTR dwIoKey)
{
	HANDLE	hIocpPort = CreateIoCompletionPort(	hSocket,
												m_hCompletionPort,
												dwIoKey,
												0);

	return (hIocpPort ? true : false);
}

bool NetLib::cIOCP::GetIocpStatus(	LPDWORD dwIoSize,
									PULONG_PTR lpCompletionKey,
									LPOVERLAPPED* lpOverlapped,
									DWORD dwMilliseconds)
{
	return GetQueuedCompletionStatus(	m_hCompletionPort,
										dwIoSize,
										lpCompletionKey,
										lpOverlapped,
										dwMilliseconds) ? true : false;
}