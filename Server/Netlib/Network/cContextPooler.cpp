#include "../../Include/Netlib/Network/cContextPooler.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Common/cInterfacePacketParser.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Session/cSession.h"
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#include "../../Include/Netlib/Manager/cSessionManager.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"

NetLib::cContextPooler::cContextPooler()
{
	Init();
}


NetLib::cContextPooler::~cContextPooler()
{
	Destory();
}

void NetLib::cContextPooler::Init()
{
}

#ifdef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM

void NetLib::cContextPooler::Destory()
{
	m_cIocpContextPool.DestroyPool();

	POSITION pos = m_contexttable.GetStartPosition(thread_type::ty_all);
	while (pos != nullptr)
	{
		NetLib::cIocpContext* pIocpContext = m_contexttable.GetValueAt(pos);
		if (pIocpContext != nullptr)
		{
			delete pIocpContext;
			pIocpContext = nullptr;
		}

		m_contexttable.GetNext(pos, thread_type::ty_all);
	}

#if defined(DEVELOPMENT)

	pos = m_atlmapContext.GetStartPosition();

	while (pos != nullptr)
	{
		ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapContext.GetAt(pos);
		if (pPair != nullptr)
		{
			NetLib::cIocpContext* pIocpContext = pPair->m_value;
			if (pIocpContext != nullptr)
			{
				delete pIocpContext;
				pPair->m_value = nullptr;
			}
		}

		m_atlmapContext.GetNext(pos);
	}
	m_atlmapContext.RemoveAll();

#endif

}

#else

void NetLib::cContextPooler::Destory()
{
	m_cIocpContextPool.DestroyPool();

	//삭제
	{
		POSITION pos = m_atlListPendingContext.GetHeadPosition();

		while (pos != nullptr)
		{
			NetLib::cIocpContext** pIocpContext = &m_atlListPendingContext.GetAt(pos);
			if(*pIocpContext != nullptr)
			{
				delete *pIocpContext;
				*pIocpContext = nullptr;
			}

			m_atlListPendingContext.GetNext(pos);
		}

		m_atlListPendingContext.RemoveAll();
	}

#ifdef USE_TIME_WAIT
	// Time Wait 상태의 Context 삭제
	{
		POSITION pos = m_atlmapTimeWaitContextTable.GetStartPosition();

		while (pos != nullptr)
		{
			ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapTimeWaitContextTable.GetAt(pos);
			if (pPair != nullptr)
			{
				NetLib::cIocpContext* pIocpContext = pPair->m_value;
				if (pIocpContext != nullptr)
				{
					delete pIocpContext;
					pPair->m_value = nullptr;
				}
			}

			m_atlmapTimeWaitContextTable.GetNext(pos);
		}
		m_atlmapTimeWaitContextTable.RemoveAll();
	}
#endif

	//접속해있는 Context삭제
	{
		POSITION pos = m_atlmapConnectedContextTable.GetStartPosition();
		
		while (pos != nullptr)
		{
			ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.GetAt(pos);
			if(pPair != nullptr)
			{
				NetLib::cIocpContext* pIocpContext = pPair->m_value;
				if(pIocpContext != nullptr)
				{
					delete pIocpContext;
					pPair->m_value = nullptr;
				}
			}

			m_atlmapConnectedContextTable.GetNext(pos);
		}
		m_atlmapConnectedContextTable.RemoveAll();
	}

#if defined(DEVELOPMENT)
	POSITION pos = m_atlmapContext.GetStartPosition();

	while (pos != nullptr)
	{
		ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapContext.GetAt(pos);
		if(pPair != nullptr)
		{
			NetLib::cIocpContext* pIocpContext = pPair->m_value;
			if(pIocpContext != nullptr)
			{
				delete pIocpContext;
				pPair->m_value = nullptr;
			}
		}

		m_atlmapContext.GetNext(pos);
	}
	m_atlmapContext.RemoveAll();
#endif
}

#endif

//////////////////////////////////////////////////////////////////////
// Operation
//////////////////////////////////////////////////////////////////////

void NetLib::cContextPooler::Create(int iMaximum)
{
	m_cIocpContextPool.CreatePool(iMaximum, iMaximum);

	m_contexttable.Init(iMaximum);
	//m_cFailedAcceptContextPool.CreatePool( 0, 0 );

	// Context가 제대로 생성이 되었는지 출력.
	/*while( true )
	{
	cIocpContext* pContext = Pop();
	if( pContext == NULL ) break;

	Trace(_T("Entity(%u) Pop Success"), pContext->GetEntity() );
	}*/

#ifdef VIRTUAL_NAGLE_ON_OFF
	if(iMaximum >= 500)
		m_bVirtualNagleOnOff = TRUE;
	else
		m_bVirtualNagleOnOff = FALSE;
#endif
	return;
}

NetLib::cIocpContext* NetLib::cContextPooler::Pop()
{
	NetLib::cIocpContext* pContext = m_cIocpContextPool.Remove();
	if(pContext == nullptr)
	{
		return nullptr;
	}
#ifdef VIRTUAL_NAGLE_ON_OFF
	if(m_bVirtualNagleOnOff)
		pContext->m_bVirtualNagleOnOff = m_bVirtualNagleOnOff;
#endif

#if defined(DEVELOPMENT)
	NetLib::cCSLock Lock(&m_Lock);
	m_atlmapContext.SetAt(pContext->GetEntity(), pContext);
#endif

	return pContext;
}

void NetLib::cContextPooler::Push(NetLib::cIocpContext* pContext)
{
#if defined(DEVELOPMENT)
	{
		NetLib::cCSLock Lock(&m_Lock);
		ATL::CAtlMap<UINT, cIocpContext*>::CPair* pPair = m_atlmapContext.Lookup(pContext->GetEntity());
		if (pPair != nullptr)
		{
			m_atlmapContext.RemoveKey(pContext->GetEntity());
		}
	}
#endif

	pContext->Clear();
#ifdef USE_PUSH_FRONT_MEMPOOLER
	m_cIocpContextPool.PushFront(pContext);
#else
	m_cIocpContextPool.Push(pContext);
#endif

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "NetLib::cContextPooler::Push. The context entered the pooler. Entity [ %u ]", pContext->GetEntity());
}

void NetLib::cContextPooler::PushErrorContext(cIocpContext* pContext)
{
	if (pContext != nullptr)
	{
#if defined(DEVELOPMENT)
		{
			NetLib::cCSLock Lock(&m_Lock);
			ATL::CAtlMap<UINT, cIocpContext*>::CPair* pPair = m_atlmapContext.Lookup(pContext->GetEntity());
			if (pPair != nullptr)
			{
				m_atlmapContext.RemoveKey(pContext->GetEntity());
		}
	}
#endif

		pContext->Clear();

#ifdef USE_PUSH_FRONT_MEMPOOLER
		m_cIocpContextPool.PushFront(pContext);
#else
		m_cIocpContextPool.Push(pContext);
#endif

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "NetLib::cContextPooler::PushErrorAcceptContext. The context entered the pooler. Entity [ %u ]", pContext->GetEntity());
	}
}

#ifdef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM

void NetLib::cContextPooler::PushContext(NetLib::cIocpContext* pContext, bool bTimeOut)
{
	if (pContext)
	{
		pContext->CleanOverlapped(E_IO_OPERATION::E_IO_ACCEPT);
		pContext->CleanOverlapped(E_IO_OPERATION::E_IO_RECEIVE);

		pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_DISCONNECTED);

		//	E_CONTEXT_TYPE 이 서버 였다면 바로 Context table 에서 사라져야합니다.
		//	이 쪽에서 Client 로 변경하지않고 완전하게 Context Pooler 로 들어갈때 Clear 가 되기때문에 
		//	이 쪽에서 변경하는걸 주석처리했습니다.
		//pContext->SetContextType(E_CONTEXT_TYPE::E_CONTEXT_CLIENT);

		//	이 함수가 실행이 되야 Context table 에서 완전하게 반환이 됩니다.
		pContext->SetWorkerThreadSignal(true);


		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cContextPooler::PushContext. The context Worker Thread Signal. Entity [ %u ]", pContext->GetEntity());
	}
}

bool NetLib::cContextPooler::ConnectContext(NetLib::cIocpContext* pContext, UINT uithreadindex)
{
	if (pContext == nullptr)
	{
		assert(false && "NetLib::cContextPooler::ConnectContext Failed. pContext Parameter is nullptr");
		return false;
	}

	pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_CONNECTED);
	pContext->SetCurrentPacketTick();
	pContext->SetUniqueKey();
	pContext->EnterContextTable();

	if (false == m_contexttable.insert(pContext, thread_type::ty_all, uithreadindex + thread_type::ty_cur_thread))
		return false;
#if defined(DEVELOPMENT)
	NetLib::cCSLock DevelopLock(&m_Lock);
	ATL::CAtlMap<UINT, cIocpContext*>::CPair* pContextPair = m_atlmapContext.Lookup(pContext->GetEntity());
	if (pContextPair != nullptr)
	{
		m_atlmapContext.RemoveKey(pContext->GetEntity());
	}
#endif

	return true;
}

//	옮기고자 하는 스레드에서 호출 해야합니다.
bool NetLib::cContextPooler::ChangeCommandContext(NetLib::cIocpContext* pContext, UINT uithreadindex)
{
	if (pContext == nullptr)
	{
		assert(false && "NetLib::cContextPooler::ChangeCommandContext Failed. pContext Parameter is nullptr");
		return false;
	}

	if (pContext->GetCommandQueueIndex() == uithreadindex)
	{
		return true;
	}

	pContext->SetCommandQueueIndex(uithreadindex);

	m_contexttable.insert(pContext, thread_type::ty_cur_thread, uithreadindex + thread_type::ty_cur_thread);

	return true;
}

void NetLib::cContextPooler::AliveContextCheck(UINT uithreadindex)
{
	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] Alive Context Check Start Thread Index[ %u ]", uithreadindex);

	ULONGLONG ulTick = ::GetTickCount64();

	POSITION oldPos = nullptr;
	POSITION pos = m_contexttable.GetStartPosition(uithreadindex + thread_type::ty_cur_thread);
	while (pos)
	{
		oldPos = pos;
		//	TODO 새로 변경될 로직
		//	상황 1. (	Command Thread 변경이 이루어져 GetCommandQueueIndex 가 맞지 않은 상태	)
		//	1 : CommandQueueChangeSucceeded 확인 합니다. 
		//	2 : 변경이 제대로 이루어 졌다면 해당 스레드에서 삭제 시도 합니다.
		//	3 : 변경이 제대로 이루어 지지 않았다면 삭제를 시도 하지 않습니다.
		//	상황 2. (	CommandQueueIndex 가 이루어 져있는대 이상 상황	)
		//	1 : CommandQueueChangeSucceeded 확인했지만 false가 일정 시간 이상으로 가면
		//	2 : 해당 스레드에서 ping check 확인 후에 팅겨 냅니다.

		m_contexttable.GetNext(pos, uithreadindex + thread_type::ty_cur_thread);
		NetLib::cIocpContext* pContext = m_contexttable.GetValueAt(oldPos);
		if (pContext->GetCommandQueueIndex() == uithreadindex)
		{
			if (E_CONTEXT_TYPE::E_CONTEXT_CLIENT == pContext->GetContextType())
			{
				AliveClientContext(pContext, ulTick, thread_type::ty_cur_thread, uithreadindex);
			}
			else if (E_CONTEXT_TYPE::E_CONTEXT_SERVER == pContext->GetContextType())
			{
				AliveServerContext(pContext, ulTick, thread_type::ty_cur_thread, uithreadindex);
			}
			else
			{
				if (E_CONTEXT_TYPE::E_CONTEXT_NONE <= pContext->GetContextType() && pContext->GetContextType() < E_CONTEXT_TYPE::E_CONTEXT_MAX)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] E_CONTEXT_TYPE  [ %s ]", G_CONTEXT_TYPE[pContext->GetContextType()]);
				}
				else
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] E_CONTEXT_TYPE  [ %d ]", static_cast<int>(pContext->GetContextType()));
				}
			}
		}
		else
		{
			//	자신이 관리하는 Context 가 아니면 삭제 합시다요~
			event_erase e_event = m_contexttable.erase(pContext->GetEntity(), thread_type::ty_another_thread, uithreadindex + thread_type::ty_cur_thread);
			if (event_erase::ee_thread_erase_no == e_event)
			{
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] Context Table 에서 삭제 실패 #1 Context Entity %u", pContext->GetEntity());
			}
		}
	}

	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] Alive Context Check End Thread Index[ %u ]", uithreadindex);
}

// 슬롯 서버 전용
// 소켓에 응답이 없는 클라이언트의 세션을 바로 정리 한다.
void NetLib::cContextPooler::SlotServerAliveContextCheck(UINT uithreadindex)
{
	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] Alive Context Check Start Thread Index[ %u ]", uithreadindex);

	ULONGLONG ulTick = ::GetTickCount64();

	POSITION oldPos = nullptr;
	POSITION pos = m_contexttable.GetStartPosition(uithreadindex + thread_type::ty_cur_thread);
	while (pos)
	{
		oldPos = pos;
		//	TODO 새로 변경될 로직
		//	상황 1. (	Command Thread 변경이 이루어져 GetCommandQueueIndex 가 맞지 않은 상태	)
		//	1 : CommandQueueChangeSucceeded 확인 합니다. 
		//	2 : 변경이 제대로 이루어 졌다면 해당 스레드에서 삭제 시도 합니다.
		//	3 : 변경이 제대로 이루어 지지 않았다면 삭제를 시도 하지 않습니다.
		//	상황 2. (	CommandQueueIndex 가 이루어 져있는대 이상 상황	)
		//	1 : CommandQueueChangeSucceeded 확인했지만 false가 일정 시간 이상으로 가면
		//	2 : 해당 스레드에서 ping check 확인 후에 팅겨 냅니다.

		m_contexttable.GetNext(pos, uithreadindex + thread_type::ty_cur_thread);
		NetLib::cIocpContext* pContext = m_contexttable.GetValueAt(oldPos);
		if (pContext->GetCommandQueueIndex() == uithreadindex)
		{
			if (E_CONTEXT_TYPE::E_CONTEXT_CLIENT == pContext->GetContextType())
			{
				SlotServerAliveClientContext(pContext, ulTick, thread_type::ty_cur_thread, uithreadindex);
			}
			else if (E_CONTEXT_TYPE::E_CONTEXT_SERVER == pContext->GetContextType())
			{
				AliveServerContext(pContext, ulTick, thread_type::ty_cur_thread, uithreadindex);
			}
			else
			{
				if (E_CONTEXT_TYPE::E_CONTEXT_NONE <= pContext->GetContextType() && pContext->GetContextType() < E_CONTEXT_TYPE::E_CONTEXT_MAX)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] E_CONTEXT_TYPE  [ %s ]", G_CONTEXT_TYPE[pContext->GetContextType()]);
				}
				else
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] E_CONTEXT_TYPE  [ %d ]", static_cast<int>(pContext->GetContextType()));
				}
			}
		}
		else
		{
			//	자신이 관리하는 Context 가 아니면 삭제 합시다요~
			event_erase e_event = m_contexttable.erase(pContext->GetEntity(), thread_type::ty_another_thread, uithreadindex + thread_type::ty_cur_thread);
			if (event_erase::ee_thread_erase_no == e_event)
			{
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] Context Table 에서 삭제 실패 #1 Context Entity %u", pContext->GetEntity());
			}
		}
	}

	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] Alive Context Check End Thread Index[ %u ]", uithreadindex);
}

POSITION NetLib::cContextPooler::GetStartPosition(UINT uithreadindex)
{
	return  m_contexttable.GetStartPosition(uithreadindex + thread_type::ty_cur_thread);
}

POSITION NetLib::cContextPooler::GetNextPosition(POSITION& prevPos, UINT uithreadindex)
{
	return m_contexttable.GetNext(prevPos, uithreadindex + thread_type::ty_cur_thread);
}

NetLib::cIocpContext* NetLib::cContextPooler::GetValue(POSITION pos)
{
	if (pos == nullptr)
		return nullptr;

	return m_contexttable.GetValueAt(pos);
}

void NetLib::cContextPooler::AliveClientContext(cIocpContext* pContext, ULONGLONG ulTick, thread_type etype, UINT uithreadindex)
{
	ULONGLONG currentpacketTick = pContext->GetCurrentPacketTick();
	E_CONNECT_STATUS eStatus = pContext->GetContextStatus();

	ULONGLONG uI64IntervalTick = ulTick - currentpacketTick;
	if (ulTick < currentpacketTick)
		uI64IntervalTick = 0;
	switch (eStatus)
	{
	case E_CONNECT_CONNECTED:
	{
		BOOL bSessionPending = FALSE;

		if (pContext->GetIosPendingDelayTime())
		{
			// 기대결과는 GetIosPendingDelayTime이 현재 Tick 보다 큰동안에는 펜딩상태로 들어가지 않는 것이다.
			if (ulTick > pContext->GetIosPendingDelayTime())
				bSessionPending = TRUE;
		}
		else if (uI64IntervalTick > CSDef::E_TIMER_INTERVAL::E_5_SECOND)
		{
			NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->PushCommand(reinterpret_cast<NetLib::cIocpContext*>(pContext), CSNet::SYS_NET_DISCONNECT);
			bSessionPending = TRUE;
			//if (uI64IntervalTick > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_PING_CHECK)
			if (false == b_Check_Tick) {
				b_Check_Tick = true;

				//민환
				////다른애들 리셋
				//NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
				//if (contextPooler == nullptr)
				//	return;
				//
				//POSITION startPos = contextPooler->GetStartPosition(uithreadindex);
				//if (startPos != nullptr) {

				//	while (startPos)
				//	{
				//		NetLib::cIocpContext* t_pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue(startPos);
				//		E_CONNECT_STATUS eStatus = t_pContext->GetContextStatus();
				//		if (eStatus == E_CONNECT_STATUS::E_CONNECT_CONNECTED)
				//		{
				//			t_pContext->SetCurrentPacketTick();
				//		}

				//		contextPooler->GetNextPosition(startPos, uithreadindex);
				//	}
				//}
			}
		}

		//if (uI64IntervalTick > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_SERVER_TO_CLIENT_PING_CHECK)
		if(bSessionPending == TRUE)
		{
			pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_CONVERTING_TO_PENDING);

			//	세션 분리 작업
			/*NetLib::cSession* pSession = pContext->GetSession();
			if (pSession != nullptr)
			{
				pSession->SessionLogout(pContext->GetEntity());
				pSession->SendMyOfflineToFriendMap();
				pSession->DisConnectContext(pContext->GetEntity(), uithreadindex);
			}
			pContext->SetSession(nullptr);
			pContext->Disconnect();*/

			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Converting To Pending. Context Entity[ %d ] Interval Tick [ %I64u ] Thread Index[ %u ]", pContext->GetEntity(), uI64IntervalTick, uithreadindex);
		}
	}
	break;
	case E_CONNECT_CONVERTING_TO_PENDING:
	{
//#ifndef INCREASE_CLIENT_TIME_OUT
//		if ((ulTick - currentpacketTick) > (60 * 1000) ) // 1분으로 연장한다.
//#else
		if ((ulTick - currentpacketTick) > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_OUT_CONTEXT)
//#endif
		{
			/*NetLib::cSession* pSession = pContext->GetSession();
			if (pSession != nullptr)
			{
				pSession->SessionLogout(pContext->GetEntity());
				pSession->SendMyOfflineToFriendMap();
				pSession->DisConnectContext(pContext->GetEntity(), uithreadindex);
			}
			pContext->SetSession(nullptr);
			pContext->Disconnect();*/
#ifndef USE_FORCE_DISCONNECT

			pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_TIME_WAIT);

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Converting To Pending Failed. Context Status Time Wait Entity[ %4d ] Interval Tick [ %I64u ]", pContext->GetEntity(), uI64IntervalTick);

#else
			//	TODO 새로 변경될 로직
			//	상황 1.	(	Disconnect 응답이 있을때	)
			//	1 : DisConnect 응답이 있다면 pooler에 넣습니다.
			//	상황 2.	(	Disconnect 응답이 없을때	)
			//	1 : DisConnect 응답이 없다면 ForceDisconnect를 호출합니다.
			//	2 : Worker Thread에서 응답이 있으면 pooler에 넣습니다.
			//	Ping이 일정시간동안 안와서 위 쪽에서 Disconnect를 했습니다.
			//	그리고도 반응이 없는 Context는 이 쪽에서 ForceDisconnect로 소켓 재생성합니다.

			pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_FORCE_DISCONNECT);

			pContext->ForceDisconnect();

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Converting To Pending Failed. Context Force DisConnect Excute Entity[ %4d ] Interval Tick [ %I64u ]", pContext->GetEntity(), uI64IntervalTick);
#endif
		}
	}
		break;
	case E_CONNECT_FORCE_DISCONNECT:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Context Status Pending. Entity[ %4d ] Interval Tick [ %I64u ]", pContext->GetEntity(), uI64IntervalTick);
	}
		break;
	case E_CONNECT_TIME_WAIT:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Context Status Time Wait. Entity[ %4d ] Interval Tick [ %I64u ]", pContext->GetEntity(), uI64IntervalTick);
		if ((ulTick - currentpacketTick) > CSDef::E_TIMER_INTERVAL::E_IOS_PENDING_DELAY_TIME)
		{
			PushContext(pContext);
		}
	}
		break;
	case E_CONNECT_DISCONNECTED:
	{
		if (pContext->WorkerThreadSignal() == true)
		{
			event_erase e_event = m_contexttable.erase(pContext->GetEntity(), thread_type::ty_all, uithreadindex + etype);
			if (e_event == event_erase::ee_push_ok)
			{
				Push(pContext);
			}
			else
			{
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] Context Table 에서 다른 Command Thread 에서 해당 Context 를 삭제하기 까지 대기 합니다. Context Entity %u", pContext->GetEntity());
			}
		}
		else
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, 
				"[ContextPooler] Context Entity [ %u ] Worker Thread Signal [ %s ]", 
				pContext->GetEntity(), 
				pContext->WorkerThreadSignal() == true ? "true" : "false");
		}
	}
		break;
	case E_CONNECT_NONE:
	default:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] Context Status [ %d ] Entity[ %4d ]", eStatus, pContext->GetEntity());
		assert(false && "[ContextPooler] Context Status Error");
	}
		break;
	}
}

// 슬롯서버는 펜딩세션 처리 하지 않습니다.
void NetLib::cContextPooler::SlotServerAliveClientContext(cIocpContext* pContext, ULONGLONG ulTick, thread_type etype, UINT uithreadindex)
{
	ULONGLONG currentpacketTick = pContext->GetCurrentPacketTick();
	E_CONNECT_STATUS eStatus = pContext->GetContextStatus();

	ULONGLONG uI64IntervalTick = ulTick - currentpacketTick;
	if (ulTick < currentpacketTick)
		uI64IntervalTick = 0;

	switch (eStatus)
	{
	case E_CONNECT_CONNECTED:
	{

		BOOL bSessionPending = FALSE;

		// 비정상 세션을 정리하고 나면 
		if (false == b_Check_Tick)
		{
			if (uI64IntervalTick > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_CHECK_PENDING_CONTEXT) {

				b_Check_Tick = true;
					
				//	세션 분리 작업
				NetLib::cSession* pSession = pContext->GetSession();
				if (pSession != nullptr)
				{
					pSession->SendMyOfflineToFriendMap();
					pSession->DisConnectContext(pContext->GetEntity(), uithreadindex);
				}

				pContext->SetSession(nullptr);
				pContext->Disconnect();

				PushContext(pContext);
				//다른애들 리셋
				/*NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
				if (contextPooler == nullptr)
					return;
				POSITION startPos = contextPooler->GetStartPosition(uithreadindex);
				if (startPos != nullptr) {

					while (startPos)
					{
						NetLib::cIocpContext* t_pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue(startPos);
						E_CONNECT_STATUS eStatus = t_pContext->GetContextStatus();
						if (eStatus == E_CONNECT_STATUS::E_CONNECT_CONNECTED)
						{

							t_pContext->SetCurrentPacketTick();
						}

						contextPooler->GetNextPosition(startPos, uithreadindex);
					}
				}*/

				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] SlotServerAliveClientContext Remove Disconnected Context. Entity[ %d ] Interval Tick [ %I64u ] Thread Index[ %u ]", pContext->GetEntity(), uI64IntervalTick, uithreadindex);
			}
		}
		else
		{
			if (uI64IntervalTick > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_CHECK_PENDING_CONTEXT) {

				//	세션 분리 작업
				NetLib::cSession* pSession = pContext->GetSession();
				if (pSession != nullptr)
				{
					pSession->SendMyOfflineToFriendMap();
					pSession->DisConnectContext(pContext->GetEntity(), uithreadindex);
				}

				pContext->SetSession(nullptr);
				pContext->Disconnect();

				PushContext(pContext);

				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] SlotServerAliveClientContext Remove Disconnected Context. Entity[ %d ] Interval Tick [ %I64u ] Thread Index[ %u ]", pContext->GetEntity(), uI64IntervalTick, uithreadindex);
			}
		}
	}
	break;
	case E_CONNECT_CONVERTING_TO_PENDING:
	{
		pContext->SetSession(nullptr);

		PushContext(pContext);

		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] SlotServerAliveClientContext Remove Pending Context. Entity[ %d ] Interval Tick [ %I64u ] Thread Index[ %u ]", pContext->GetEntity(), uI64IntervalTick, uithreadindex);
	}
	break;
	case E_CONNECT_FORCE_DISCONNECT:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Context Status Pending. Entity[ %4d ] Interval Tick [ %I64u ]", pContext->GetEntity(), uI64IntervalTick);
	}
	break;
	case E_CONNECT_TIME_WAIT:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Context Status Time Wait. Entity[ %4d ] Interval Tick [ %I64u ]", pContext->GetEntity(), uI64IntervalTick);
	}
	break;
	case E_CONNECT_DISCONNECTED:
	{
		if (pContext->WorkerThreadSignal() == true)
		{
			event_erase e_event = m_contexttable.erase(pContext->GetEntity(), thread_type::ty_all, uithreadindex + etype);
			if (e_event == event_erase::ee_push_ok)
			{
				Push(pContext);
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] Context Table Push Success. Context Entity %u", pContext->GetEntity());
			}
			else
			{
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ ContextPooler ] Context Table 에서 다른 Command Thread 에서 해당 Context 를 삭제하기 까지 대기 합니다. Context Entity %u", pContext->GetEntity());
			}
		}
		else
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
				"[ContextPooler] Context Entity [ %u ] Worker Thread Signal [ %s ]",
				pContext->GetEntity(),
				pContext->WorkerThreadSignal() == true ? "true" : "false");
		}
	}
	break;
	case E_CONNECT_NONE:
	default:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] Context Status [ %d ] Entity[ %4d ]", eStatus, pContext->GetEntity());
		assert(false && "[ContextPooler] Context Status Error");
	}
	break;
	}
}

void NetLib::cContextPooler::AliveServerContext(cIocpContext* pContext, ULONGLONG ulTick, thread_type etype, UINT uithreadindex)
{
	
	ULONGLONG currentpacketTick = pContext->GetCurrentPacketTick();
	E_CONNECT_STATUS eStatus = pContext->GetContextStatus();

	ULONGLONG uI64IntervalTick = ulTick - currentpacketTick;

	switch (eStatus)
	{
	case E_CONNECT_CONNECTED:
		break;
	case E_CONNECT_DISCONNECTED:
	{
		if (pContext->WorkerThreadSignal() == true)
		{
			event_erase e_event = m_contexttable.erase(pContext->GetEntity(), thread_type::ty_all, uithreadindex + etype);
			if (e_event == event_erase::ee_push_ok)
			{
				Push(pContext);
			}
		}
		else
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
				"[ContextPooler] Server Context Entity [ %u ] Worker Thread Signal [ %s ]",
				pContext->GetEntity(),
				pContext->WorkerThreadSignal() == true ? "true" : "false");
		}
	}
		break;
	case E_CONNECT_CONVERTING_TO_PENDING:
	case E_CONNECT_FORCE_DISCONNECT:
	case E_CONNECT_TIME_WAIT:
		break;
	case E_CONNECT_NONE:
	default:
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "[ContextPooler] [ Server Context ] Context Status [ %d ] Entity[ %4d ]", eStatus, pContext->GetEntity());
		assert(false && "[ContextPooler] Server Context Status Error");
	}
		break;
	}
}

NetLib::cIocpContext* NetLib::cContextPooler::FindConnectedContext(UINT Entity, int nthreadindex)
{
	return m_contexttable.find(Entity, static_cast<UINT>(nthreadindex + thread_type::ty_cur_thread));
}

NetLib::cSession* NetLib::cContextPooler::FindConnectedSession(UINT Entity, int nthreadindex)
{
	NetLib::cIocpContext* pContext = m_contexttable.find(Entity, static_cast<UINT>(nthreadindex + thread_type::ty_cur_thread));
	return pContext == nullptr ? nullptr : pContext->GetSession();
}

NetLib::cCommandQueue* NetLib::cContextPooler::FindCommandQueueArray(UINT Entity, int nthreadindex)
{
	NetLib::cIocpContext* pContext = m_contexttable.find(Entity, static_cast<UINT>(nthreadindex + thread_type::ty_cur_thread));
	if (pContext == nullptr)
	{
		return nullptr;
	}

	return NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr(pContext->GetCommandQueueIndex());
}

#else

void NetLib::cContextPooler::PushContext(NetLib::cIocpContext* pContext, bool bTimeOut)
{
	if (pContext)
	{
		pContext->CleanOverlapped(E_IO_OPERATION::E_IO_ACCEPT);
		pContext->CleanOverlapped(E_IO_OPERATION::E_IO_RECEIVE);

		E_CONNECT_STATUS eStatus = pContext->GetContextStatus();

		if (E_CONNECT_STATUS::E_CONNECT_NONE == eStatus)
		{
			Push(pContext);
		}
#ifdef USE_TIME_WAIT
		else if (E_CONNECT_STATUS::E_CONNECT_TIME_WAIT == eStatus && bTimeOut)
		{
			PushTimeWait(pContext);
		}
#endif
		else
		{
			PushPendingList(pContext);
		}
	}
}

void NetLib::cContextPooler::CheckPendingList()
{

	NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	if (pCommandQueueManager == nullptr)
	{
		assert(false && "NetLib::cContextPooler::CheckPendingList is Failed. CommandQueueManager is nullptr");
		return;
	}

	NetLib::cUnionLock Lock(&m_PendingContextLock, FALSE);
	//종료된 Context를 정상적으로 쓰기 위해 검사를 하는 곳입니다.
	POSITION pos = m_atlListPendingContext.GetHeadPosition();
	while (pos != nullptr)
	{
		NetLib::cIocpContext* pContext = m_atlListPendingContext.GetAt(pos);
		if (pContext != nullptr)
		{
			if (pContext->GetCurrentPacketTick() >= CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_CHECK_PENDING_CONTEXT)
			{
				//Context 의 상태가 DisConnected 상태가 아니라면 Session 객체랑 연결이 되어있는 상태입니다.
				//Session 객체를 정리를 해주어야합니다.
				pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_DISCONNECTED);
				NetLib::cSession* pSession = pContext->GetSession();
				if (pSession == nullptr)
				{
					//Session이 비어있는상태면 더이상 초기화 루틴을 탈 필요가 없습니다.
					//바로 Context Clear 해주고 Pooler에 Push 해줍니다.
					POSITION OldPos = pos;
					m_atlListPendingContext.GetNext(pos);
					m_atlListPendingContext.RemoveAt(OldPos);

					pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_NONE);
					PushContext(pContext);
					continue;
				}
				else
				{
					assert(false && "NetLib::cContextPooler::CheckPendingList() ERROR");
				}
			}
		}

		m_atlListPendingContext.GetNext(pos);
	}
}

bool NetLib::cContextPooler::ConnectContext(NetLib::cIocpContext* pContext, UINT nThreadIndex)
{
	if (pContext == nullptr)
	{
		assert(false && "NetLib::cContextPooler::ConnectContext Failed. pContext Parameter is nullptr");
		return false;
	}

	NetLib::cUnionLock Lock(&m_ConnectedtableLock, FALSE);
	ATL::CAtlMap<UINT, cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.Lookup(pContext->GetEntity());

	if (pPair != nullptr)
	{
		//pPair 가 존재하면 제대로 삭제가 되지않은겁니다.
		//Debug Mode에서 assert를 걸었습니다. 이 부분에서 어서트가 발생한다면 제대로 추적해서 
		//발생이 안되게 고치시면됩니다.
		//그리고 m_value값마다 다르게 assert를 하게 하고있습니다.
		//Debug에서는 assert로 뜨지만 만약 Release에서 발생하게되면 알게 하기 위해 로그를 남기도록했습니다.
#ifdef _DEBUG
		if (pPair->m_value != nullptr)
		{
			assert(false && "NetLib::cContextPooler::ConnectContext Failed. IocpContext already exists instance.");
		}
		else
		{
			assert(false && "NetLib::cContextPooler::ConnectContext Failed. ATL::CAtlMap<UINT, cIocpContext*>::CPair already exists instance.");
		}
#else
		NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
		if (pLogQueue)
		{
			if (pPair->m_value != nullptr)
			{
				pLogQueue->PushUserCommand((int64)(pContext->GetEntity()),
					LOG_GRADE::LOG_CRI,
					_T("NetLib::cContextPooler::ConnectContext Failed. IocpContext already exists instance."));
			}
			else
			{
				pLogQueue->PushUserCommand((int64)(pContext->GetEntity()),
					LOG_GRADE::LOG_CRI,
					_T("NetLib::cContextPooler::ConnectContext Failed. ATL::CAtlMap<UINT, cIocpContext*>::CPair already exists instance."));
			}
		}

#endif
		return false;
	}

	pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_CONNECTED);
	pContext->SetCurrentPacketTick();
	pContext->SetUniqueKey();

	m_atlmapConnectedContextTable.SetAt(pContext->GetEntity(), pContext);

#if defined(DEVELOPMENT)
	NetLib::cCSLock DevelopLock(&m_Lock);
	ATL::CAtlMap<UINT, cIocpContext*>::CPair* pContextPair = m_atlmapContext.Lookup(pContext->GetEntity());
	if (pContextPair != nullptr)
	{
		m_atlmapContext.RemoveKey(pContext->GetEntity());
	}
#endif

	return true;
}

void NetLib::cContextPooler::PushPendingList(NetLib::cIocpContext* pContext)
{
	if (pContext != nullptr)
	{
		NetLib::cUnionLock Lock(&m_PendingContextLock, FALSE);
		E_CONNECT_STATUS eStatus = pContext->GetContextStatus();
		if (E_CONNECT_STATUS::E_CONNECT_CONNECTED == eStatus ||
			E_CONNECT_STATUS::E_CONNECT_CONVERTING_TO_PENDING == eStatus)
		{
			this->RemoveContext(pContext->GetEntity());
		}
#ifdef USE_TIME_WAIT
		else if (E_CONNECT_STATUS::E_CONNECT_TIME_WAIT == eStatus)
		{
			this->RemoveTimeWaitContext(pContext->GetEntity());
		}
#endif
		else
		{
			E_CONNECT_STATUS eStatus = pContext->GetContextStatus();
			if (E_CONNECT_STATUS::E_CONNECT_DISCONNECTED == eStatus)
			{
				/*
				 * 이 경우는 어떻게 들어오는지 알고있습니다.
				 * 확인용도로 assert를 걸어놓았습니다.
				 * 이전에 한번 이쪽으로 탓었던 이유는 
				 * ForceDisConnect할때 조건은 이미 한번 DisConnect함수를 호출하고
				 * 클라쪽에서 TransmitFile 을 응답반지 못하면 워커 스레드쪽에서 DisConnect쪽 루틴을 타지 못합니다.
				 * 이 와중에 CheckPingContext 함수에서 PushContext를 시도해서 Pending쪽으로 Context를 옮겨두고
				 * FoceDisConnect함수는 Active false일때는 루틴을 실행하지 않았습니다.(이전 코드에서는 지금은 Active 상태 체크 하는 코드 뺏습니다.)
				 * 일정 시간이 지난다음에 TransmitFile이 타임아웃이나면서 워커 스레드쪽에서 DisConnect처리가 된겁니다.
				 * 그래서 이쪽으로 들어오게 된겁니다.
				 * 다른 경우도 있을수 있으니 assert는 계속 걸어두겠습니다.
				*/
				assert(false && "NetLib::cContextPooler::PushPendingList Failed. Context State E_CONNECT_DISCONNECTED.");
				return;
			}
			else
			{
				/*
				 * 이 경우는 절대 들어올수 없습니다.
				 * 이 함수를 콜하는 쪽에서 이 경우에 올 수 있는것을 차단하기 때문입니다.
				*/
				assert(false && "NetLib::cContextPooler::PushPendingList Failed");
				return;
			}
		}

		pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_PENDING);
		pContext->SetCurrentPacketTick();
		m_atlListPendingContext.AddTail(pContext);

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "NetLib::cContextPooler::Push. The context entered the PendingList. Entity [ %u ]", pContext->GetEntity());
	}
}

bool NetLib::cContextPooler::RemoveContext(UINT Entity)
{
	NetLib::cUnionLock Lock(&m_ConnectedtableLock, FALSE);
	ATL::CAtlMap<UINT, cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.Lookup(Entity);
	if(pPair == nullptr)
	{
		return true;
	}

	//pair 는 있는대 value 가 nullptr일때 assert
	if (pPair->m_value == nullptr)
	{
		assert(false && "NetLib::cContextPooler::RemoveContext Not exists Key");
		return false;
	}

	return m_atlmapConnectedContextTable.RemoveKey(Entity);
}

void NetLib::cContextPooler::AliveContextCheck(ULONGLONG ulTick, UINT nThreadIndex)
{
	NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	if(pCommandQueueManager == nullptr)
	{
		assert(false && "NetLib::cContextPooler::CheckPingContext is Failed. CommandQueueManager is nullptr");
		return;
	}

	NetLib::cUnionLock Lock(&m_ConnectedtableLock, TRUE);

	//Ping check
	POSITION pos = m_atlmapConnectedContextTable.GetStartPosition();
	while (pos != nullptr)
	{
		ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.GetAt(pos);
		if (pPair != nullptr)
		{
			NetLib::cIocpContext* pContext = pPair->m_value;
			if (pContext != nullptr)
			{
				if (pContext->GetCommandQueueIndex() == nThreadIndex)
				{
					//Context Type 이 Server 가 아닌것만 검사합니다.
					E_CONTEXT_TYPE eType = pContext->GetContextType();
					if (E_CONTEXT_TYPE::E_CONTEXT_SERVER != eType)
					{

#ifndef USE_PACKET_TICK_CHECK_CONTEXT
						ULONGLONG currentpacketTick = 0;

						NetLib::cSession* pSession = pContext->GetSession();
						if (pSession == nullptr)
						{
							currentpacketTick = pContext->GetCurrentPacketTick();
						}
						else
						{
							currentpacketTick = pSession->GetCurrentPacketTick();
						}
#else 
						ULONGLONG currentpacketTick = pContext->GetCurrentPacketTick();
#endif
						if ((ulTick - currentpacketTick) > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_SERVER_TO_CLIENT_PING_CHECK)
						{
							E_CONNECT_STATUS eStatus = pContext->GetContextStatus();

							if (E_CONNECT_STATUS::E_CONNECT_CONNECTED == eStatus)
							{
								pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_CONVERTING_TO_PENDING);

								//	세션 분리 작업
								NetLib::cSession* pSession = pContext->GetSession();
								if (pSession != nullptr)
								{
									pSession->DisConnectContext(pContext->GetEntity(), nThreadIndex);
								}

								pContext->SetSession(nullptr);
								pContext->Disconnect();

								cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Converting To Pending. Context Entity[ %d ]", pContext->GetEntity());
							}
							else
							{
								//	Ping이 일정시간동안 안와서 위 쪽에서 Disconnect를 했습니다.
								//	그리고도 반응이 없는 Context는 이 쪽에서 ForceDisconnect로 소켓 재생성합니다.
								if (pContext->IsActive() == false || (ulTick - currentpacketTick) >= CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_CHECK_PENDING_CONTEXT)
								{

#ifndef USE_TIME_WAIT
									pContext->ForceDisconnect();

									NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Converting To Pending Failed. Context Force DisConnect Excute Entity[ %4d ]", pContext->GetEntity());
#else
									pContext->SetContextStatus(E_CONNECT_STATUS::E_CONNECT_TIME_WAIT);
									PushContext(pContext, true);

									NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "[ContextPooler] Converting To Pending Failed. Context Time Wait Entity[ %4d ]", pContext->GetEntity());

									POSITION OldPos = pos;
									m_atlmapConnectedContextTable.GetNext(pos);
									m_atlmapConnectedContextTable.RemoveAtPos(OldPos);
									continue;
#endif
								}
							}
						}
					}	//	if (E_CONTEXT_TYPE::E_CONTEXT_SERVER != eType)
				}	//	if (pContext->GetCommandQueueIndex() == nThreadIndex)
			}	//	if (pContext != nullptr)
		}
		m_atlmapConnectedContextTable.GetNext(pos);
	}
}

NetLib::cIocpContext* NetLib::cContextPooler::FindConnectedContext(UINT Entity)
{
	NetLib::cUnionLock Lock(&m_ConnectedtableLock, TRUE);
	ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.Lookup(Entity);
	if(pPair != nullptr)
	{
		return pPair->m_value;
	}
	return nullptr;
}

NetLib::cSession* NetLib::cContextPooler::FindConnectedSession(UINT Entity)
{
	NetLib::cUnionLock Lock(&m_ConnectedtableLock, TRUE);
	ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.Lookup(Entity);
	if (pPair != nullptr)
	{
		return pPair->m_value == nullptr ? nullptr : pPair->m_value->GetSession();
	}
	return nullptr;
}

NetLib::cCommandQueue* NetLib::cContextPooler::FindCommandQueueArray(UINT Entity)
{
	NetLib::cUnionLock Lock(&m_ConnectedtableLock, TRUE);
	ATL::CAtlMap<UINT, NetLib::cIocpContext*>::CPair* pPair = m_atlmapConnectedContextTable.Lookup(Entity);
	if (pPair != nullptr)
	{
		NetLib::cIocpContext* pContext = pPair->m_value;
		if (pContext == nullptr)
			return nullptr;

		UINT commandthread_array = pContext->GetCommandQueueIndex();

		return NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr(commandthread_array);
	}

	return nullptr;
}

#endif

#ifdef USE_TIME_WAIT

void NetLib::cContextPooler::PushTimeWait(NetLib::cIocpContext* pContext)
{
	NetLib::cCSLock Lock(&m_TimeWaitLock);

	m_atlmapTimeWaitContextTable.SetAt(pContext->GetEntity(), pContext);
}

bool NetLib::cContextPooler::RemoveTimeWaitContext(UINT Entity)
{
	NetLib::cCSLock Lock(&m_TimeWaitLock);

	ATL::CAtlMap<UINT, cIocpContext*>::CPair* pPair = m_atlmapTimeWaitContextTable.Lookup(Entity);
	if (pPair == nullptr)
	{
		return true;
	}

	//pair 는 있는대 value 가 nullptr일때 assert
	if (pPair->m_value == nullptr)
	{
		assert(false && "NetLib::cContextPooler::RemoveContext Not exists Key");
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "NetLib::cContextPooler::RemoveTimeWaitContext. Time Wait Context to Pending state Change Success. Entity [ %u ]", Entity);

	return m_atlmapTimeWaitContextTable.RemoveKey(Entity);
}

#endif