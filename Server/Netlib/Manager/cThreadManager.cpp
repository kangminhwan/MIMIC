#include "../../Include/Netlib/Manager/cThreadManager.h"
#include "../../Include/Netlib/Thread/cCommandThread.h"
#include "../../Include/Netlib/Thread/cWorkerThread.h"
#include "../../Include/Netlib/Thread/cLogThread.h"
#include "../../Include/Netlib/Thread/cWebThread.h"
#include "../../Include/Netlib./Common/cSingleton.h"

NetLib::cThreadManager::cThreadManager() :
	m_iCommandThreadCnt(0),
	m_iWorkerThreadCnt(0),
	m_iWebThreadCnt(0),
	m_pCommandThread(nullptr),
	m_pWorkerThread(nullptr),
	m_pLogThread(nullptr)
{
	Init();
}


NetLib::cThreadManager::~cThreadManager()
{
	Destroy();
}

void NetLib::cThreadManager::Init()
{
	m_pCommandThread = new NetLib::cCommandThread;
	m_pWorkerThread = new NetLib::cWorkerThread;
	m_pWebThread = new NetLib::cWebThread;
	m_pLogThread = NetLib::cSingleton<NetLib::cLogThread>::GetInstance();

	// thread 타입별로 nullptr 미리 생성
	for (int n = 0; n < E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_MAX; ++n)
	{
		m_thread_monitor_array_list.push_back(nullptr);
	}
}

void NetLib::cThreadManager::Destroy()
{
	if(m_pWorkerThread)
	{
		delete m_pWorkerThread;
		m_pWorkerThread = nullptr;
	}

	if (m_pWebThread)
	{
		delete m_pWebThread;
		m_pWebThread = nullptr;
	}

	if(m_pCommandThread)
	{
		delete m_pCommandThread;
		m_pCommandThread = nullptr;
	}

	if(m_pLogThread)
	{
		NetLib::cSingleton<NetLib::cLogThread>::DeleteInstance();
		m_pLogThread = nullptr;
	}

	if (m_thread_monitor_array_list.size() > 0)
	{
		std::vector<stThreadMonitor*>::iterator iter = m_thread_monitor_array_list.begin();
		std::vector<stThreadMonitor*>::iterator iterend = m_thread_monitor_array_list.end();
		for (; iter != iterend; ++iter)
		{
			if ((*iter) != nullptr)
			{
				delete[] (*iter);
				(*iter) = nullptr;
			}
		}
	}
}

bool NetLib::cThreadManager::CreateServerThread()
{
	if(!StartLogThread())
		return false;

	if(!StartCommandThread())
		return false;

	if(!StartWebThread())
		return false;

	if(!StartWorkerThread())
		return false;

	return true;
}

void NetLib::cThreadManager::DestroyServerThread()
{
	StopWorkerThread();
	StopWebThread();
	StopCommandThread();
}

bool NetLib::cThreadManager::StartCommandThread()
{
	if(!m_pCommandThread)
		return false;

	// 특별히 커맨드 쓰레드의 갯수를 지정하지 않으면 1로 설정한다.
	if(m_iCommandThreadCnt == 0)
		m_iCommandThreadCnt = 1;

	if(m_pCommandThread)
		m_pCommandThread->StartThread(m_iCommandThreadCnt, TRUE);

	return true;
}

bool NetLib::cThreadManager::StartWorkerThread()
{
	if(!m_pWorkerThread) return false;

	// 워커 스레드의 갯수를 지정하지 않으면 ( 현재 프로세서의 갯수에 * 2 ) + 1 을 한다.
	if(m_iWorkerThreadCnt == 0)
	{
		SYSTEM_INFO		SystemInfo;

		GetSystemInfo(&SystemInfo);

		m_iWorkerThreadCnt = (SystemInfo.dwNumberOfProcessors * 2) + 1;
	}

	if(m_pWorkerThread)
	{
		m_pWorkerThread->InitializeBuffer(m_iWorkerThreadCnt);
		m_pWorkerThread->StartThread(m_iWorkerThreadCnt);
	}

	return true;
}

bool NetLib::cThreadManager::StartLogThread()
{
	if(!m_pLogThread) return false;

	if(m_pLogThread)
		m_pLogThread->StartThread(1);

	return true;
}

bool NetLib::cThreadManager::StartWebThread()
{
	if(m_pWebThread == nullptr)
	{
		return false;
	}

	if(m_iWebThreadCnt == 0)
	{
		// Web Thread 를 안쓰는 경우가 있습니다.
		return true;
	}

	if(m_pWebThread)
	{
		m_pWebThread->StartThread(m_iWebThreadCnt, TRUE);
	}

	return true;
}

void NetLib::cThreadManager::StopCommandThread()
{
	bool bThreadTermination = false;

	if(m_pCommandThread)
	{
		m_pCommandThread->SendThreadCloseEvent();

		bThreadTermination = m_pCommandThread->WaitForAllThreadTermination();

		if(!bThreadTermination)
			m_pCommandThread->ForceThreadKill();

		delete m_pCommandThread;
		m_pCommandThread = nullptr;
	}
}

void NetLib::cThreadManager::StopWebThread()
{
	bool bThreadTermination = false;

	if(m_pWebThread)
	{
		m_pWebThread->SendThreadCloseEvent();

		bThreadTermination = m_pWebThread->WaitForAllThreadTermination();

		if(bThreadTermination == false)
		{
			m_pWebThread->ForceThreadKill();
		}

		delete m_pWebThread;
		m_pWebThread = nullptr;
	}
}

void NetLib::cThreadManager::StopWorkerThread()
{
	bool bThreadTermination = false;

	m_pWorkerThread->PushWorkerThreadExit();

	if(m_pWorkerThread->WaitThreadTermination())
	{
		return;
	}
	else
	{
		return;
	}
}

void NetLib::cThreadManager::SetCommandThreadCnt(int iThreadCnt)
{ 
	m_iCommandThreadCnt = iThreadCnt;
	int iType = E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_COMMAND;
	m_thread_monitor_array_list[iType] = new stThreadMonitor[iThreadCnt];

	size_t dd = sizeof(m_thread_monitor_array_list[iType]);

	// description 미리 설정해둠
	char buffer[65];
	for (int n = 0; n < iThreadCnt; ++n)
	{
		_itoa_s(n + 1, buffer, 65, 10);
		m_thread_monitor_array_list[iType][n].description = "CommandThread ThreadNum ";
		m_thread_monitor_array_list[iType][n].description.append(buffer);
	}
}

void NetLib::cThreadManager::SetWorkerThreadCnt(int iThreadCnt)
{ 
	m_iWorkerThreadCnt = iThreadCnt;
	int iType = E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_WORKER;
	m_thread_monitor_array_list[iType] = new stThreadMonitor[iThreadCnt];

	// description 미리 설정해둠
	char buffer[65];
	for (int n = 0; n < iThreadCnt; ++n)
	{
		_itoa_s(n + 1, buffer, 65, 10);
		m_thread_monitor_array_list[iType][n].description = "WorkerThread ThreadNum ";
		m_thread_monitor_array_list[iType][n].description.append(buffer);
	}
}

void NetLib::cThreadManager::SetWebThreadCnt(int iThreadCnt)
{ 
	m_iWebThreadCnt = iThreadCnt;
	int iType = E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_Web;
	m_thread_monitor_array_list[iType] = new stThreadMonitor[iThreadCnt];

	// description 미리 설정해둠
	char buffer[65];
	for (int n = 0; n < iThreadCnt; ++n)
	{
		_itoa_s(n + 1, buffer, 65, 10);
		m_thread_monitor_array_list[iType][n].description = "WebThread ThreadNum ";
		m_thread_monitor_array_list[iType][n].description.append(buffer);
	}
}

void NetLib::cThreadManager::SetLogThreadCnt(int iThreadCnt)
{
	m_iLogThreadCnt = iThreadCnt;
	int iType = E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_LOG;
	m_thread_monitor_array_list[iType] = new stThreadMonitor[iThreadCnt];

	// description 미리 설정해둠
	char buffer[65];
	for (int n = 0; n < iThreadCnt; ++n)
	{
		_itoa_s(n + 1, buffer, 65, 10);
		m_thread_monitor_array_list[iType][n].description = "LogThread ThreadNum ";
		m_thread_monitor_array_list[iType][n].description.append(buffer);
	}
}

stThreadMonitor* NetLib::cThreadManager::GetThreadMonitor(E_SERVER_THREAD_TYPE eType, int iThreadArratNum)
{
	// eType, iThreadNum 두개다 validation check
	if (eType < E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_COMMAND || eType >= E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_MAX)
		return nullptr;

	stThreadMonitor* pBase = m_thread_monitor_array_list[eType];
	if (pBase == nullptr)
		return nullptr;

	if (eType > m_thread_monitor_array_list.size() - 1)
		return nullptr;

	pBase += iThreadArratNum;

	return pBase;
	//return m_thread_monitor_array_list[eType];
}

void NetLib::cThreadManager::ReadThreadStatus(E_SERVER_THREAD_TYPE eType)
{
	int iThreadCnt = 0;

	ULONGLONG ui64TickGap = 0;

	switch (eType)
	{
	case E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_COMMAND:
			iThreadCnt = m_iCommandThreadCnt;
			ui64TickGap = m_ui64CommandGap;
			break;
	case E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_WORKER:
			iThreadCnt = m_iWorkerThreadCnt;
			ui64TickGap = m_ui64WorkerGap;
			break;
	case E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_LOG:
			iThreadCnt = m_iLogThreadCnt;
			ui64TickGap = m_ui64LogGap;
			break;
	case E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_Web:
			iThreadCnt = m_iWebThreadCnt;
			ui64TickGap = m_ui64WebGap;
			break;
	default:
		return;
	}

	// 3초 이상 놀고 있는 쓰레드들은 상태이상을 출력해 준다.
	for (int n = 0; n < iThreadCnt; ++n)
	{
		// lastTick 이 0이면, 체크 하지 않는다.
		if (m_thread_monitor_array_list[eType][n].lastTick == 0)
			continue;

		ULONGLONG gap = ::GetTickCount64() - m_thread_monitor_array_list[eType][n].lastTick;
		if (gap > ui64TickGap)
		{
			printf("%s not working idle time is [%I64u]ms, you need to check server alive\n",
				m_thread_monitor_array_list[eType][n].description.c_str(),
				gap);
		}
	}
}