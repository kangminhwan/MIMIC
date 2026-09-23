#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cHeader;
class cUdpHeader;
class cInterfaceLog;
class cUDPDispatcher;
class cUDPSessionManager;
class cCommandQueue;
class cIocpUDP
{
private:
	int					m_nUdpPort;
	HANDLE				m_hCompletionPort;
	HANDLE				m_hIocpPort;
	SOCKET				m_hUdpSocketIPv4;
	//SOCKET				m_hUdpSocketIPv6;
	cUDPDispatcher*		m_pUDPDispatcher;
	cUDPSessionManager* m_pUDPSessionMgr;
	cCriticalSection	m_csLock;

	unsigned int* m_uiThreadID;
	HANDLE		m_hUDPThread[MAX_WORKER_THREADS];

	int			m_iWorkerThreadCnt;
	int			m_iThreadArrayNum;
	int*		m_pPerThreadPacket; // 쓰레드당 몇개의 패킷을 받는 지 알고 싶을때..

	cInterfaceLog* pInterfaceLog;

	DWORD		m_dwTimerCompletionKey;
	DWORD		m_dwFinishThreadCompletionKey;

private:
	void Init();
	void Destroy();
	void Startup();

public:
	static unsigned int __stdcall WorkerThread(void *pVoid);
	UINT WokerThread();

	SOCKET CreateIPv4Socket();
	//SOCKET CreateIPv6Socket();
	void CloseSocket(SOCKET udpsocket);
	bool ExtendSockBuffer(SOCKET udpsocket);

	bool CreateIocp();
	bool Bind(int iPort);
	//bool BindIPv6(int iPort);
	int GetBindingUdpPort() { return m_nUdpPort; }

	bool StartIocpUDPModule(int iServerPort, int iWorkerThredCnt);
	void EndIocpUDPModule();
	bool WaitThreadTermination();

	int* GetPerThreadPacketPtr() { return m_pPerThreadPacket; }

	void SetInterfaceLog(cInterfaceLog* pInterface) { pInterfaceLog = pInterface; }

	void SetTimerEvent();

	bool CheckPacket(cHeader* pHeader, UINT nLength);

public:
	cIocpUDP();
	~cIocpUDP();
};

END_NETLIB