#include "../../Include/Netlib/Scheduler/cScheduler.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#else
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#endif

NetLib::cScheduler::cScheduler(const BOOL bUseHighResolutionTimer)
{
	m_bUseHighResolutionTimer = bUseHighResolutionTimer;

	Init();
}

NetLib::cScheduler::~cScheduler()
{
	Destroy();
}

void NetLib::cScheduler::Init()
{
	tstring className = _T("Scheduler");
	NetLib::cBaseThread::SetClassName( className );

	// 외부에서 제어 가능하도록, freqeuncy를 무조건 초기화 한다.
//	if(m_bUseHighResolutionTimer)
	QueryPerformanceFrequency(&m_Frequency);
}

void NetLib::cScheduler::Destroy()
{
	Terminate();

	WaitForMultipleObjects(m_byThreadCount, m_hThread, TRUE, INFINITE);

	std::list<stSchedule*>::iterator iterScheduler;
	stSchedule* pSchedule = NULL;
	while (true)
	{
		if(m_listSchedule.begin() == m_listSchedule.end()) return;

		iterScheduler = m_listSchedule.begin();

		pSchedule = *iterScheduler;
		if(pSchedule)
		{
			m_listSchedule.erase(iterScheduler);
			delete pSchedule;
		}
	}

	m_listSchedule.clear();
}

/*
bRandomCommand ON 일경우, 커맨드 쓰레드중에 랜덤으로 1개에게 커멘드를 전송하게 됩니다.
*/
void NetLib::cScheduler::AddShceduleEvent(const UINT timer_id, DWORD msec, bool bAutoDeleOn, bool bRandomCommand)
{
	// 스케쥴러는 Sleep(1) 로 작동중입니다. 처리상 10 ~ 16까지로 처리가 됩니다.
	if (msec < 20)
		throw "cScheduler::AddShceduleEvent msec must over 20ms";

	//// 동일한 MSec을 사용하는 타이머가 있을 경우, 이셉션 발생 시킵니다.
	//std::list<stSchedule*>::iterator iter = m_listSchedule.begin();
	//for (; iter != m_listSchedule.end(); ++iter)
	//{
	//	stSchedule* pScheduleEle = *iter;
	//	if(pScheduleEle == nullptr)
	//		throw "NetLib::cScheduler::AddShceduleEvent Detect, m_listSchedule has nullptr";

	//	if (pScheduleEle->dwMSec == msec)
	//	{
	//		TCHAR szTest[256] = { 0, };
	//		wsprintf(szTest, _T("dwMSec[ %lu ] cScheduler already has same timer\n"), pScheduleEle->dwMSec);
	//		::MessageBox(NULL, szTest, _T("ERROR"), NULL);
	//		exit(0);
	//	}
	//}

	m_csLock.Lock();

	stSchedule* pSchedule = new stSchedule();

	if(pSchedule != NULL)
	{
		// 초기값 셋팅
		pSchedule->dwStart = ::GetTickCount64();
		pSchedule->dwLastEvent = pSchedule->dwStart;
		pSchedule->dwMSec = static_cast<ULONGLONG>(msec);
		pSchedule->uTimerID = timer_id;
		pSchedule->bRandomCommand = bRandomCommand;

		if (m_bUseHighResolutionTimer)
		{
			LARGE_INTEGER start;
			QueryPerformanceCounter(&start);

			pSchedule->nextTick = start.QuadPart + (m_Frequency.QuadPart * msec / 1000);// 종료 QuadPart 를 알아온다.
		}
		else
			pSchedule->nextTick = ::GetTickCount64() + msec; // 돌아갈 다음 Tick

		pSchedule->bAutoDelete = bAutoDeleOn; // true 이면 딜리트 자동으로 시킴

		m_listSchedule.push_back(pSchedule);
	}

	m_csLock.Unlock();
}

void NetLib::cScheduler::SchedulerSleep(int ms)
{
	if (m_bUseHighResolutionTimer)
	{
		/*LARGE_INTEGER m_Frequency;
		QueryPerformanceFrequency(&m_Frequency);*/

		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);

		//UINT loop = 0;

		while (true)
		{
			LARGE_INTEGER end;
			QueryPerformanceCounter(&end);
			double gap = (double)((double)(end.QuadPart - start.QuadPart) / (double)m_Frequency.QuadPart * 1000.0);

			if (gap >= ms)
				break;

			//++loop;
		}
	}
	else
	{
		/*if (ms < 20)
			throw "NetLib::cScheduler::SchedulerSleep ms must over 20ms";*/

		Sleep(ms);
	}
}

void NetLib::cScheduler::Process(UINT ThreadArray)
{
	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
	if (pLogQueue != nullptr)
	{
		pLogQueue->PushCommand(LOG_GRADE::LOG_INFO, "Scheduler Thread Running [thread %lu]", GetCurrentThreadId());
	}
	else
	{
		printf("Scheduler Thread Running [thread %lu]", GetCurrentThreadId());
	}

	ULONGLONG presentTick = ::GetTickCount64();

	while (!m_bTerminated)
	{
		if(m_bUseHighResolutionTimer)
			GetScheduleEventHighResolution();
		else
			GetScheduleEvent();
		
		SchedulerSleep(1);

//#ifdef _DEBUG
//		ULONGLONG beforeTick = presentTick;
//		presentTick = ::GetTickCount64();
//
//		if (beforeTick != presentTick)
//		{
//			ULONGLONG gap = presentTick - beforeTick;
//
//			TCHAR szTest[128];
//			wsprintf(szTest, _T("gap[%lu]ms cScheduler::Process, GetTickCount64 diff\n"), gap);
//			::OutputDebugString(szTest);
//		}
//		else
//		{
//			ULONGLONG gap = presentTick - beforeTick;
//
//			TCHAR szTest[128];
//			wsprintf(szTest, _T("gap[%lu]ms cScheduler::Process, GetTickCount64 same\n"), gap);
//			::OutputDebugString(szTest);
//
//		}
//#endif
	}

	if (pLogQueue != nullptr)
	{
		pLogQueue->PushCommand(LOG_GRADE::LOG_INFO, "Scheduler Thread Exit [thread %lu]", GetCurrentThreadId());
	}
	else
	{
		printf("Scheduler Thread Exit [thread %lu]", GetCurrentThreadId());
	}

	return;
}

bool NetLib::cScheduler::GetScheduleEvent()
{
	m_csLock.Lock();

	if(!m_listSchedule.size())
	{
		m_csLock.Unlock();
		return false;
	}

	std::list<stSchedule*>::iterator iterS = m_listSchedule.begin();
	std::list<stSchedule*>::iterator iterE = m_listSchedule.end();
	std::list<stSchedule*>::iterator iterTemp;

	stSchedule* pSchedule = NULL;

	ULONGLONG presentTick = ::GetTickCount64();

	while (iterS != iterE)
	{
		pSchedule = *iterS;

		// 이터레이터 임시변수에 저장, 삭제용 변수
		iterTemp = iterS;
		++iterS;

		if(pSchedule)
		{
			// 현재 시간이 인터벌값보다 커졌다..실행하자구
			//if(dwPresentTime > pSchedule->dwLastEvent + pSchedule->dwInterval)
			if (presentTick >= pSchedule->nextTick)
			{
				/*if (pSchedule->dwInterval == 1000)
				{
					TCHAR szTest[128];
					wsprintf(szTest, _T("Interval[%lu], timegap[%lu]실제 처리 까지 걸린시간\n"), pSchedule->dwInterval, presentTick - pSchedule->nextTick);
					::OutputDebugString(szTest);
				}*/
				
				// 현재시간 저장
				pSchedule->dwLastEvent = presentTick;
				//pSchedule->nextTick = presentTick + pSchedule->dwInterval;
				pSchedule->nextTick += pSchedule->dwMSec;

				/*if (pSchedule->dwInterval == 1000)
				{
					TCHAR szTest[128];
					wsprintf(szTest, _T("%lu 스케쥴이벤트\n"), pSchedule->dwInterval);
					::OutputDebugString(szTest);
				}*/

				// 이벤트 푸쉬


#ifdef USING_MULTI_THREAD
				NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();

				if(!pCommandQueueManager)
					continue;

				if(pSchedule->bRandomCommand == false)
					pCommandQueueManager->BroadCastCommandScheduleJob((UINT)0, CSNet::SYS_SCHEDULE_MSG, pSchedule, sizeof(stSchedule));
				else
					pCommandQueueManager->RandomCastCommandScheduleJob((UINT)0, CSNet::SYS_SCHEDULE_MSG, pSchedule, sizeof(stSchedule));
#else
				NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueue>::ExistsInstance();

				if(!pCommandQueue)
					continue;

				pCommandQueue->PushCommand((UINT)0, CSNet::SYS_SCHEDULE_MSG, (BYTE*)pSchedule, sizeof(stSchedule));
#endif
				//cSingleton<cLogQueue>::GetInstance()->PushCommand( _T("%d 스케쥴러 이벤트 전달"), pSchedule->dwInterval );

				// AutoDelete On 인놈들은 삭제해주자.
				if(pSchedule->bAutoDelete)
				{
					delete pSchedule;
					m_listSchedule.erase(iterTemp);
				}
			}
		}
	}

	m_csLock.Unlock();

	return true;
}

bool NetLib::cScheduler::GetScheduleEventHighResolution()
{
	m_csLock.Lock();

	if (!m_listSchedule.size())
	{
		m_csLock.Unlock();
		return false;
	}

	std::list<stSchedule*>::iterator iterS = m_listSchedule.begin();
	std::list<stSchedule*>::iterator iterE = m_listSchedule.end();
	std::list<stSchedule*>::iterator iterTemp;

	stSchedule* pSchedule = NULL;

	LARGE_INTEGER present;
	QueryPerformanceCounter(&present);

	while (iterS != iterE)
	{
		pSchedule = *iterS;

		// 이터레이터 임시변수에 저장, 삭제용 변수
		iterTemp = iterS;
		++iterS;

		if (pSchedule)
		{
			// 현재 시간이 인터벌값보다 커졌다..실행하자구
			if (static_cast<ULONGLONG>(present.QuadPart) >= pSchedule->nextTick)
			{
				// 현재시간 저장
				//pSchedule->dwLastEvent = presentTick;
				pSchedule->nextTick += m_Frequency.QuadPart * pSchedule->dwMSec / 1000;// 종료 QuadPart 를 알아온다.

#ifdef USING_MULTI_THREAD
				NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->BroadCastCommandScheduleJob((UINT)0, CSNet::SYS_SCHEDULE_MSG, pSchedule, sizeof(stSchedule));
#else
				NetLib::cSingleton<NetLib::cCommandQueue>::GetInstance()->PushCommand((UINT)0, CSNet::SYS_SCHEDULE_MSG, (BYTE*)pSchedule, sizeof(stSchedule));
#endif

//#ifdef _DEBUG
//				// 스트링 가공자체에 시간이 소요되니, CASE문에 맞을때만 스트링을 가공하도록 한다.
//				switch (pSchedule->dwMSec)
//				{
//				case 1:
//					{
//						/*TCHAR szTest[128];
//						wsprintf(szTest, _T("%lu 스케쥴이벤트\n"), pSchedule->dwMSec);
//						::OutputDebugString(szTest);*/
//					}
//					break;
//				case 33:
//					{
//						/*TCHAR szTest[128];
//						wsprintf(szTest, _T("%lu 스케쥴이벤트\n"), pSchedule->dwMSec);
//						::OutputDebugString(szTest);*/
//					}
//					break;
//				case 1000:
//					{
//						/*TCHAR szTest[128];
//						wsprintf(szTest, _T("%lu 스케쥴이벤트\n"), pSchedule->dwMSec);
//						::OutputDebugString(szTest);*/
//					}
//					break;
//				case 5000:
//					{
//						/*TCHAR szTest[128];
//						wsprintf(szTest, _T("%lu 스케쥴이벤트\n"), pSchedule->dwMSec);
//						::OutputDebugString(szTest);*/
//					}
//					break;
//				default:
//					break;
//				}
//#endif

				// AutoDelete On 인놈들은 삭제해주자.
				if (pSchedule->bAutoDelete)
				{
					delete pSchedule;
					m_listSchedule.erase(iterTemp);
				}
			}
		}
	}

	m_csLock.Unlock();

	return true;
}