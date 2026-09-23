#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cIocpQueue
{
private:
	bool	Init();
	void	Destroy();

protected:
	DWORD	m_dwTimeOutGQCS;
	HANDLE	m_hCompletionPort;
	ULONGLONG m_AliveTick;

	bool m_bExitThread;

protected:
	BOOL	PushQueue(const ULONG_PTR dwCompletionKey, const DWORD dwBytes);
	void SetExitThread();


public:
	PULONG_PTR PopQueue(DWORD dwTimeOutGQCS = INFINITE);

	void	UpdateAliveTick() { m_AliveTick = ::GetTickCount64(); }
	ULONGLONG GetAliveTick() { return m_AliveTick; }
	HANDLE GetCompletionPort() { return m_hCompletionPort; }
public:
	cIocpQueue();
	cIocpQueue(DWORD dwTimeOutGQCS); // 생성하면서 GQCS 타임아웃값을 설정
	virtual ~cIocpQueue();
};

END_NETLIB