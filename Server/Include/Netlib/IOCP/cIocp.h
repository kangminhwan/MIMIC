#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cIOCP
{
protected:
	HANDLE	m_hCompletionPort;

private:
	void	Init();
	void	Destroy();

public:
	bool	Create();
	void	Close();

	bool	AssocInstance(HANDLE hSocket, ULONG_PTR dwIoKey);

	bool	GetIocpStatus(	LPDWORD dwIoSize,
							PULONG_PTR lpCompletionKey,
							LPOVERLAPPED* lpOverlapped,
							DWORD dwMilliseconds = INFINITE);

	HANDLE	GetComepletionPort() { return m_hCompletionPort; }

public:
	cIOCP();
	~cIOCP();
};

END_NETLIB