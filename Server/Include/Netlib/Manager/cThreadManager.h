#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cCommandThread;
class cWorkerThread;
class cLogThread;
class cWebThread;
class cThreadManager
{
protected:
	cCommandThread* m_pCommandThread;
	cWorkerThread*  m_pWorkerThread;
	cLogThread*		m_pLogThread;
	cWebThread*		m_pWebThread;

private:
	int m_iCommandThreadCnt;
	int	m_iWorkerThreadCnt;
	int m_iWebThreadCnt;
	int m_iLogThreadCnt;
	std::vector<stThreadMonitor*> m_thread_monitor_array_list;

	const ULONGLONG m_ui64CommandGap = 3000;
	const ULONGLONG m_ui64LogGap = 10000;
	const ULONGLONG m_ui64WebGap = 10000;
	const ULONGLONG m_ui64WorkerGap = 1000;

public:
	void Init();
	void Destroy();

public:
	bool CreateServerThread();
	void DestroyServerThread();

private:
	bool StartCommandThread();
	bool StartWorkerThread();
	bool StartLogThread();
	bool StartWebThread();

	void StopCommandThread();
	void StopWorkerThread();
	void StopWebThread();

public:
	void SetCommandThreadCnt(int iThreadCnt);
	void SetWorkerThreadCnt(int iThreadCnt);
	void SetWebThreadCnt(int iThreadCnt);
	void SetLogThreadCnt(int iThreadCnt = 1);

	cCommandThread* GetCommandThreadPtr() { return m_pCommandThread; }
	cWorkerThread*	GetWorkerThreadPtr() { return m_pWorkerThread; }
	cLogThread*		GetLogThreadPtr() { return m_pLogThread; }
	cWebThread*		GetWebThreadPtr() { return m_pWebThread; }

	stThreadMonitor* GetThreadMonitor(E_SERVER_THREAD_TYPE eType, int iThreadArratNum);
	void ReadThreadStatus(E_SERVER_THREAD_TYPE eType);

public:
	cThreadManager();
	~cThreadManager();
};

END_NETLIB