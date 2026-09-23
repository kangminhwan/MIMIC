#include "../../Include/Netlib/Queue/cIocpQueue.h"

NetLib::cIocpQueue::cIocpQueue()
{
	m_dwTimeOutGQCS = INFINITE;
	Init();
}

NetLib::cIocpQueue::cIocpQueue(DWORD dwTimeOutGQCS)
{
	m_dwTimeOutGQCS = dwTimeOutGQCS;
	Init();
}


NetLib::cIocpQueue::~cIocpQueue()
{
	Destroy();
}

bool NetLib::cIocpQueue::Init()
{
	m_hCompletionPort = CreateIoCompletionPort(	INVALID_HANDLE_VALUE,
												NULL,
												0,
												0);
	if(!m_hCompletionPort)
		return false;

	m_AliveTick = ::GetTickCount64();

	m_bExitThread = false;

	return true;
}

void NetLib::cIocpQueue::Destroy()
{
	if(m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
}

void NetLib::cIocpQueue::SetExitThread()
{
	m_bExitThread = true;
}

BOOL NetLib::cIocpQueue::PushQueue(const ULONG_PTR dwCompletionKey, const DWORD dwBytes)
{
	if (m_bExitThread)
	{
		return false;
	}

	return PostQueuedCompletionStatus(	m_hCompletionPort,
										dwBytes,
										dwCompletionKey,
										NULL);
}


PULONG_PTR NetLib::cIocpQueue::PopQueue(DWORD dwTimeOutGQCS)
{
	PULONG_PTR 		lpCompletionKey = nullptr;
	DWORD			dwBytes = 0;
	LPOVERLAPPED	lpOverlapped = NULL;

	if(m_hCompletionPort)
	{
		BOOL bSuccess = GetQueuedCompletionStatus(	m_hCompletionPort,
													&dwBytes,
													reinterpret_cast<PULONG_PTR>(&lpCompletionKey),
													&lpOverlapped,
													m_dwTimeOutGQCS == dwTimeOutGQCS ? m_dwTimeOutGQCS : dwTimeOutGQCS);
	}

	return lpCompletionKey;
}