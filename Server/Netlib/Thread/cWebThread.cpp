#include "../../Include/Netlib/Thread/cWebThread.h"
#include "../../Include/Netlib/Common/cInterfacePacketParser.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Manager/cThreadManager.h"
#include "../../Include/Netlib/MiniDump/cMiniDump.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Queue/cWebQueue.h"
#include "../../Include/Netlib/Queue/cWebQueueElement.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"

NetLib::cWebThread::cWebThread() :
	m_pInterfacePacketParser(nullptr)
{
	Init();
}

NetLib::cWebThread::~cWebThread()
{
	Destroy();
	cBaseThread::Destroy();
}

void NetLib::cWebThread::Init()
{
	cBaseThread::SetClassName(_T("cWebThread"));

}

void NetLib::cWebThread::Destroy()
{
	cBaseThread::Terminate();
	PushWebThreadExit();
	WaitThreadTemination();
}

void NetLib::cWebThread::PushWebThreadExit()
{
	NetLib::cWebQueue* pWebQueue = NetLib::cSingleton<NetLib::cWebQueue>::ExistsInstance();
	if(pWebQueue == nullptr)
	{
		return;
	}

	for (int n = 0; n < m_byThreadCount; ++n)
	{
		PostQueuedCompletionStatus(pWebQueue->GetCompletionPort(), 0, NULL, nullptr);
	}
}

bool NetLib::cWebThread::WaitThreadTemination()
{
	DWORD dwWaitRef = WaitForMultipleObjects(m_byThreadCount, m_hThread, TRUE, INFINITE);
	if(dwWaitRef != WAIT_OBJECT_0)
	{
		return false;
	}

	return true;
}

void NetLib::cWebThread::SetParserInstancePtr(NetLib::cInterfacePacketParser* pInterface)
{
	m_pInterfacePacketParser = pInterface;
}

NetLib::cInterfacePacketParser* NetLib::cWebThread::GetparserInstancePtr() const
{
	return m_pInterfacePacketParser;
}

void NetLib::cWebThread::Process(UINT uiThreadArray)
{
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(pServerManager == nullptr)
	{
		assert(false && "cWebThread::Process Failed. ServerManager is nullptr");
		return;
	}

	NetLib::cMiniDump* pMiniDumb = NetLib::cSingleton<NetLib::cMiniDump>::ExistsInstance();
	if(pMiniDumb == nullptr)
	{
		assert(false && "cWebThread::Process Failed. cMiniDumb is nullptr");
		return;
	}

	if(m_pInterfacePacketParser == nullptr)
	{
		assert(false && "cWebThread::Process Failed. cInterfacePacetParser is nullptr");
		return;
	}

	NetLib::cWebQueue* pWebQueue = NetLib::cSingleton<NetLib::cWebQueue>::ExistsInstance();
	if(pWebQueue == nullptr)
	{
		assert(false && "cWebThread::Process Failed. cWebQueue is nullptr");
		return;
	}

	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
	if(pLogQueue)
	{
		pLogQueue->PushCommand(LOG_GRADE::LOG_CRI, _T("cWebThread Running:: [thread %lu]"), GetCurrentThreadId());
	}

	stThreadMonitor* pThreadMonitor = pThreadMonitor =
		NetLib::cSingleton<NetLib::cThreadManager>::GetInstance()->GetThreadMonitor(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_Web, uiThreadArray);
	if (pThreadMonitor == nullptr)
	{
		assert(false && "cWebThread::Process Failed. ThreadMonitor is nullptr");
		return;
	}

	NetLib::cWebQueueElement* pElement = nullptr;

	while (!m_bTerminated)
	{
		if(m_pInterfacePacketParser == nullptr)
		{
			assert(false && "cWebThread::Process Failed. cInterfacePacetParser is nullptr");
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI, _T("cWebThread::Process is Failed cInterfacePacketParser is nullptr [thread %lu]"), GetCurrentThreadId());
			return;
		}

		pElement = reinterpret_cast<NetLib::cWebQueueElement*>(pWebQueue->PopQueue());
		if(pElement == nullptr)
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, _T("cWebThread::Process End Exit [thread %lu]"),GetCurrentThreadId());
			break;
		}

		__try
		{
			pThreadMonitor->UpdateTick();

			NetLib::cIocpContext* pContext = pElement->GetContext();
			if(pContext)
			{
				if(pContext->IsActive())
				{
					__try
					{
						m_pInterfacePacketParser->WebProcess(	pContext,
																pElement->GetCommand(),
																pElement->GetData(),
																pElement->GetLength(),
																uiThreadArray);
					}
					__except (pMiniDumb->ExceptionHandler(GetExceptionInformation()))
					{
						if(pLogQueue)
						{
							pLogQueue->PushCommand(	LOG_GRADE::LOG_CRI, 
													_T("cWebThread::Process in m_pInterfacePacketParser->Process exception #1 occured command[%u] PacketLength[%d] [thread %u]"),
													pElement->GetCommand(), 
													pElement->GetLength(), 
													GetCurrentThreadId());
						}
					}
				}
			}
			else
			{
				__try
				{
					m_pInterfacePacketParser->WebProcess(	pElement->GetID(),
															pElement->GetCommand(),
															pElement->GetData(),
															pElement->GetLength(),
															uiThreadArray);
				}
				__except (pMiniDumb->ExceptionHandler(GetExceptionInformation()))
				{
					if(pLogQueue)
					{
						pLogQueue->PushCommand(	LOG_GRADE::LOG_CRI,
												_T("cWebThread::Process in m_pInterfacePacketParser->Process exception #2 occured command[%u] PacketLength[%d] [thread %u]"),
												pElement->GetCommand(),
												pElement->GetLength(),
												GetCurrentThreadId());
					}
				}
			}
		}
		__finally
		{
			pWebQueue->Free(pElement);

			pThreadMonitor->EndTick();
		}
	}
}