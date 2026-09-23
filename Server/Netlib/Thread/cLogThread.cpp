#include "../../Include/Netlib/Thread/cLogThread.h"
#include "../../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Manager/cLogManager.h"
#include "../../Include/Netlib/Manager/cThreadManager.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Queue/cLogQueueElement.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Alert/cInterfaceAlert.h"

#include <future>
#include <functional>

NetLib::cLogThread::cLogThread()
{
	Init();
}


NetLib::cLogThread::~cLogThread()
{
	Destroy();
	NetLib::cBaseThread::Destroy();
}

void NetLib::cLogThread::Init()
{
	NetLib::cBaseThread::SetClassName(_T("cLogThread"));
	m_LogReCreateMinute = 0;
	m_llCreationTick = 0;
}

void NetLib::cLogThread::Destroy()
{
	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
	if(pLogQueue)
	{
		for (BYTE n = 0; n < m_byThreadCount; ++n)
		{
			PostQueuedCompletionStatus(pLogQueue->GetCompletionPort(), 0, NULL, nullptr);
		}

		WaitForMultipleObjects(m_byThreadCount, m_hThread, TRUE, INFINITE);
	}
}

#if(_WIN32_WINNT >= 0x0600) 
#define GET_TICK_COUNT GetTickCount64
#else
#define GET_TICK_COUNT GetTickCount
#endif 
//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

//void NetLib::cLogThread::Process(UINT ThreadArray)
//{
//	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
//	if(!pServerManager)
//		return;
//
//	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
//	if(!pConfig)
//		return;
//
//	m_LogReCreateMinute = pConfig->nReCreateLogMinute;
//	const ULONGLONG llCreateInterval = static_cast<ULONGLONG>(m_LogReCreateMinute * 60 * 1000);
//
//	m_llCreationTick = GET_TICK_COUNT() + llCreateInterval;
//
//	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
//	if(!pLogQueue)
//	{
//		pServerManager->SendMessageToListBox(RED, _T("End LogThread, The reason no cLogQueue ThreadId %lu"), GetCurrentThreadId());
//		return;
//	}
//
//	NetLib::cLogManager* pLogMgr = NetLib::cSingleton<NetLib::cLogManager>::ExistsInstance();
//	if(!pLogMgr)
//	{
//		pServerManager->SendMessageToListBox(RED, _T("End LogThread, The reason no cLogMgr ThreadId %lu"), GetCurrentThreadId());
//		return;
//	}
//
//	stThreadMonitor* pThreadMonitor = nullptr;
//	NetLib::cThreadManager* pThreadManager = NetLib::cSingleton<NetLib::cThreadManager>::ExistsInstance();
//	if (pThreadManager == nullptr)
//		throw "NetLib::cLogThread::Process cThreadManager is nullptr";
//
//	pThreadMonitor = pThreadManager->GetThreadMonitor(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_LOG, ThreadArray);
//	if (pThreadMonitor == nullptr)
//		throw "NetLib::cLogThread::Process ThreadMonitor is nullptr";
//
//	pThreadMonitor->UpdateTick();
//
//	NetLib::cLogQueueElement* pLogElement = NULL;
//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, _T("cLogThread Running:: [thread %lu]"), GetCurrentThreadId());
//
//	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);
//
//	while (!m_bTerminated)
//	{
//		pLogElement = reinterpret_cast<NetLib::cLogQueueElement*>(pLogQueue->PopQueue());
//
//		if(pLogElement == nullptr)
//		{
//			return;
//		}
//
//		if(pLogElement)
//		{
//			if(m_LogReCreateMinute > 0)
//			{
//				if(GET_TICK_COUNT() > m_llCreationTick)
//				{
//					pLogMgr->Open();
//					m_llCreationTick = GET_TICK_COUNT() + llCreateInterval;
//				}
//			}
//			//if(pLogElement->bCheckNewFile)
//			//	pLogMgr->Open();
//
//			// LogThread에서 파일, 리스트박스, 또는 양쪽다 출력할지 결정
//			//pServerManager->SendLogMessage(pLogElement->GetStringColor(), pLogElement->GetStringBuffer());
//
//#ifdef USE_LOG_FILE
//			//pLogMgr->WriteLog(pLogElement->GetStringBuffer());
//			//if(pLogElement->GetStringColor() != EStringColor::BLUE)
//			if (pLogElement->GetLogGrade() >= pConfig->LogGrade)
//			{
//				pServerManager->SendLogMessage(pLogElement->GetStringColor(), pLogElement->GetStringBuffer());
//
//				// Alert 는 로그파일에 저장하지 않는다.
//				if ( pLogElement->GetLogGrade() != LOG_GRADE::LOG_ALERT )
//					pLogMgr->WriteLog(pLogElement->GetStringBufferWithCarrigeReturn(), pLogElement->bUserLog);
//
//				// Alert 메시지는 함수를 호출한다.
//				// Alert 등급일때만 호출한다.
//				if ( m_interfaceAlert != nullptr && pLogElement->GetLogGrade() > LOG_GRADE::LOG_SYSTEM ) 
//				{
//					TCHAR* pStringBuffer = pLogElement->GetStringBuffer();
//
//					// 비동기로 메시지 전송한다.
//					std::future<BOOL> result = std::async( [pStringBuffer]() {
//						auto alertInstance = NetLib::cSingleton<cLogThread>::GetInstance()->GetAlertInterface();
//						return static_cast<BOOL>(alertInstance->SendAlert( pStringBuffer ));
//					} );
//				}
//			}
//#endif
//
//			if(m_LogObservers.size())
//			{
//				std::list<NetLib::cInterfaceIocpContext*>::iterator iterS = m_LogObservers.begin();
//				std::list<NetLib::cInterfaceIocpContext*>::iterator iterE = m_LogObservers.end();
//				std::list<NetLib::cInterfaceIocpContext*>::iterator iterTemp;
//
//				NetLib::cInterfaceIocpContext* pObserverContext = NULL;
//
//				stOBSERVER_LOG obserber_log;
//				while (iterS != iterE)
//				{
//					pObserverContext = *iterS;
//
//					// 이터레이터 임시변수에 저장, 삭제용 변수
//					iterTemp = iterS;
//					++iterS;
//
//					if(pObserverContext == NULL || pObserverContext->IsActive() == FALSE)
//					{
//						m_LogObservers.erase(iterTemp);
//						continue;
//					}
//
//					obserber_log.Clear();
//					//_tcscpy_s(obserber_log.m_tzLog, CSDef::MAX_OBSERVER_BUFFER_SIZE, pLogElement->GetStringBuffer());
//					memcpy(obserber_log.m_tzLog, pLogElement->GetStringBuffer(), sizeof(TCHAR) * CSDef::MAX_OBSERVER_BUFFER_SIZE);
//					obserber_log.m_tzLog[CSDef::MAX_OBSERVER_BUFFER_SIZE - 1] = 0;
//
//					packet.Clear();
//					packet.MakeManualEncrypt(CSNet::OBSERVER_LOG, reinterpret_cast<BYTE*>(&obserber_log), sizeof(obserber_log), 0);
//					pObserverContext->SendRequest(packet.GetBuffer(), packet.GetLength());
//				}
//			}
//			pLogQueue->Free(pLogElement);
//		}
//
//		// 로그 쓰레드 모니터링 업데이트
//		pThreadMonitor->UpdateTick();
//	}
//
//	return;
//}

void NetLib::cLogThread::Process(UINT ThreadArray)
{
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if (!pServerManager)
		return;

	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if (!pConfig)
		return;

	m_LogReCreateMinute = pConfig->nReCreateLogMinute;
	const ULONGLONG llCreateInterval = static_cast<ULONGLONG>(m_LogReCreateMinute * 60 * 1000);

	m_llCreationTick = GET_TICK_COUNT() + llCreateInterval;

	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
	if (!pLogQueue)
	{
		pServerManager->SendMessageToListBox(RED, _T("End LogThread, The reason no cLogQueue ThreadId %lu"), GetCurrentThreadId());
		return;
	}

	NetLib::cLogManager* pLogMgr = NetLib::cSingleton<NetLib::cLogManager>::ExistsInstance();
	if (!pLogMgr)
	{
		pServerManager->SendMessageToListBox(RED, _T("End LogThread, The reason no cLogMgr ThreadId %lu"), GetCurrentThreadId());
		return;
	}

	NetLib::cThreadManager* pThreadManager = NetLib::cSingleton<NetLib::cThreadManager>::ExistsInstance();
	if (pThreadManager == nullptr)
		throw "NetLib::cLogThread::Process cThreadManager is nullptr";

	stThreadMonitor* pThreadMonitor = pThreadManager->GetThreadMonitor(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_LOG, ThreadArray);
	if (pThreadMonitor == nullptr)
		throw "NetLib::cLogThread::Process ThreadMonitor is nullptr";

	pThreadMonitor->UpdateTick();

	NetLib::cLogQueueElement* pLogElement = NULL;
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, _T("cLogThread Running:: [thread %lu]"), GetCurrentThreadId());

	while (!m_bTerminated)
	{
		try
		{
			pLogElement = reinterpret_cast<NetLib::cLogQueueElement*>(pLogQueue->PopQueue());

			if (pLogElement == nullptr)
			{
				Sleep(10);  // 큐가 비어있으면 잠시 대기
				continue;
			}

			if (m_LogReCreateMinute > 0)
			{
				if (GET_TICK_COUNT() > m_llCreationTick)
				{
					pLogMgr->Open();
					m_llCreationTick = GET_TICK_COUNT() + llCreateInterval;
				}
			}

#ifdef USE_LOG_FILE
			if (pLogElement->GetLogGrade() >= pConfig->LogGrade)
			{
				pServerManager->SendLogMessage(pLogElement->GetStringColor(), pLogElement->GetStringBuffer());

				if (pLogElement->GetLogGrade() != LOG_GRADE::LOG_ALERT)
					pLogMgr->WriteLog(pLogElement->GetStringBufferWithCarrigeReturn(), pLogElement->bUserLog);

				if (m_interfaceAlert != nullptr && pLogElement->GetLogGrade() > LOG_GRADE::LOG_SYSTEM)
				{
					TCHAR* pStringBuffer = pLogElement->GetStringBuffer();

					std::future<BOOL> result = std::async([pStringBuffer]() {
						auto alertInstance = NetLib::cSingleton<cLogThread>::GetInstance()->GetAlertInterface();
						return static_cast<BOOL>(alertInstance->SendAlert(pStringBuffer));
						});
				}
			}
#endif
			pLogQueue->Free(pLogElement);

			pThreadMonitor->UpdateTick();
		}
		catch (const std::exception& e)
		{
			pLogQueue->Free(pLogElement);

			pThreadMonitor->UpdateTick();

			// 예외 메시지를 로그로 기록
			pServerManager->SendMessageToListBox(RED, _T("Exception in LogThread: %s"), e.what());
		}
		catch (...)
		{
			pLogQueue->Free(pLogElement);

			pThreadMonitor->UpdateTick();

			// 알 수 없는 예외를 로그로 기록
			pServerManager->SendMessageToListBox(RED, _T("Unknown exception in LogThread"));
		}
	}

	return;
}

void NetLib::cLogThread::InsertObserver(NetLib::cInterfaceIocpContext* pObserverContext)
{
	if(pObserverContext->IsActive() == FALSE)
		return;

	m_LogObservers.push_back(pObserverContext);
}

void NetLib::cLogThread::SetAlert( cInterfaceAlert* pInterfaceAlert )
{
	m_interfaceAlert = pInterfaceAlert;
}
