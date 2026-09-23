#include "../../Include/Netlib/Thread/cCommandThread.h"
#include "../../Include/Netlib/Common/cInterfacePacketParser.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/MiniDump/cMiniDump.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Queue/cCommandQueueElement.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Manager/cThreadManager.h"
#include "../../Include/Netlib/Alert/cInterfaceAlert.h"
#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#endif

#include <format>

NetLib::cCommandThread::cCommandThread() :
	m_pInterfacePacketParser(nullptr)
{
	Init();
}


NetLib::cCommandThread::~cCommandThread()
{
	Destroy();
	NetLib::cBaseThread::Destroy();
}

void NetLib::cCommandThread::Init()
{
	NetLib::cBaseThread::SetClassName(_T("cCommandThread"));
}

void NetLib::cCommandThread::Destroy()
{
	NetLib::cBaseThread::Terminate();

	WaitForMultipleObjects(m_byThreadCount, m_hThread, TRUE, INFINITE);
}

void NetLib::cCommandThread::SetParserInstancePtr(NetLib::cInterfacePacketParser* pInterface)
{
	m_pInterfacePacketParser = pInterface;
}

void NetLib::cCommandThread::SetAlert( cInterfaceAlert* pInterfaceAlert )
{
	m_interfaceAlert = pInterfaceAlert;
}

NetLib::cInterfacePacketParser* NetLib::cCommandThread::GetParserInstancePtr() const
{
	return m_pInterfacePacketParser;
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

void NetLib::cCommandThread::Process(UINT ThreadArray)
{
	//cServerDlg* pServerDlg = cSingleton<cServerDlg>::GetInstance();
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(pServerManager == nullptr)
		return;

	NetLib::cMiniDump* m_pMiniDump = NetLib::cSingleton<NetLib::cMiniDump>::ExistsInstance();
	if(m_pMiniDump == nullptr)
		return;

	stThreadMonitor* pThreadMonitor = nullptr;
	NetLib::cThreadManager* pThreadManager = NetLib::cSingleton<NetLib::cThreadManager>::ExistsInstance();
	if (pThreadManager == nullptr)
		throw "NetLib::cCommandThread::Process cThreadManager is nullptr";
	
	pThreadMonitor = pThreadManager->GetThreadMonitor(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_COMMAND, ThreadArray);
	if (pThreadMonitor == nullptr)
		throw "NetLib::cCommandThread::Process ThreadMonitor is nullptr";

	pThreadMonitor->UpdateTick();

#ifdef USING_MULTI_THREAD
	NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	NetLib::cCommandQueue* pCommandQueue = nullptr;
	if(pCommandQueueManager)
	{
		pCommandQueue = pCommandQueueManager->GetCommandQueuePtr(ThreadArray);
	}
#else
	cCommandQueue* pCommandQueue = cSingleton<cCommandQueue>::ExistsInstance();
#endif
	if(!pCommandQueue)
	{
		pServerManager->SendMessageToListBox(RED, _T("cCommandThread detect cCommandQueue not created ThreadId %lu"), GetCurrentThreadId());
		return;
	}

	NetLib::cCommandQueueElement* pCommand = NULL;

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("CommandThread Running:: [thread %lu]"), GetCurrentThreadId());
	//pServerManager->SendMessageToListBox( BLACK, _T("CommandThread Running:: [thread %lu]"), GetCurrentThreadId() );

	//pInterfaceProcess = cSingleton<cStarServerProcess>::GetInstance();
	//GetExceptionInformation();

	// CommandThread 의 GQCS 타임아웃을 1초로 처리하도록 수정, 이유 커맨드 쓰레드의 모니터링용
	static DWORD dwTimeOutGQCS = 1000;

	while (!m_bTerminated)
	{
		if(!m_pInterfacePacketParser)
			continue;

		pCommand = reinterpret_cast<NetLib::cCommandQueueElement*>(pCommandQueue->PopQueue(dwTimeOutGQCS));
		if(pCommand)
		{
			//pServerDlg->SendMessageToListBox( _T("CommandThread Got Message") );
			__try
			{
				// process command
				NetLib::cIocpContext* pContext = pCommand->GetContext();
				if(pContext)
				{
					if(pContext->IsActive())
					{
						__try
						{

							m_pInterfacePacketParser->Process(	pContext,
																pCommand->GetCommand(),
																pCommand->GetData(),
																pCommand->GetLength(),
																ThreadArray);
						}
						__except (m_pMiniDump->ExceptionHandler(GetExceptionInformation()))
						{
							NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("m_pInterfacePacketParser->Process exception #1 occured command[%d] PacketLength[%d] [thread %lu]"),
								pCommand->GetCommand(), pCommand->GetLength(), GetCurrentThreadId());

							// Alert 메시지는 함수를 호출한다.
							// Alert 등급일때만 호출한다.
														
							if ( m_interfaceAlert != nullptr ) {
								//m_interfaceAlert->SendAlert( _T( "Server Created Dump File. Please Check The Server"));
								m_interfaceAlert->SendDumpAlert( m_pMiniDump->stackTrace );
							}
						}
					}
					else
					{
						//pServerDlg->SendMessageToListBox( _T("IsActive() false but got message") );
					}
				}
				else
				{
					__try
					{
						if(!pCommand->GetDispatcher())
						{
							m_pInterfacePacketParser->Process(	pCommand->GetID(),
																pCommand->GetCommand(),
																pCommand->GetData(),
																pCommand->GetLength(),
																ThreadArray,
																nullptr);
						}
						else
						{
							m_pInterfacePacketParser->Process(	pCommand->GetDispatcher(),
																pCommand->GetCommand(),
																pCommand->GetEntity(),
																pCommand->GetPacketSeq(),
																pCommand,
																pCommand->GetData(),
																pCommand->GetLength(),
																ThreadArray);
						}
					}
					__except (m_pMiniDump->ExceptionHandler(GetExceptionInformation()))
					{
						NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("m_pInterfacePacketParser->Process exception #2 occured command[%d] PacketLength[%d] [thread %lu]"),
							pCommand->GetCommand(), pCommand->GetLength(), GetCurrentThreadId());
					}
				}
			}
			__finally
			{
				pCommandQueue->Free(pCommand);
				pCommandQueue->UpdateAliveTick();
			}
		}
		else
		{
			/*	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, _T("CommandThread Running:: [thread %lu]"), GetCurrentThreadId());
			pServerManager->SendMessageToListBox( RED, _T("CommandThread PopQueue() Failed") );*/

			// GQCS TimeOut 처리
			pCommandQueue->UpdateAliveTick();
		}
	}

	return;
}