#pragma once
#include "../Thread/cBaseThread.h"

BEGIN_NETLIB

class cScheduler : public cBaseThread
{
private:
	cCriticalSection m_csLock;
	std::list<stSchedule*> m_listSchedule;

	LARGE_INTEGER m_Frequency;
	BOOL m_bUseHighResolutionTimer;

private:
	void Init();
	void Destroy();
	bool GetScheduleEvent();
	bool GetScheduleEventHighResolution();

protected:
	virtual void Process(UINT ThreadArray) override;

public:
	//void AddShceduleEvent(const UINT timer_id, DWORD msec, bool bAutoDeleOn = false);
	void AddShceduleEvent(const UINT timer_id, DWORD msec, bool bAutoDeleOn, bool bRandomCommand);
	void SetHighResolutionTimer() { m_bUseHighResolutionTimer = TRUE; }

	bool StartScheduler()
	{
		return cBaseThread::StartThread(1);
	}

	void SchedulerSleep(int ms);

public:
	cScheduler(const BOOL bUseHighResolutionTimer = FALSE);
	virtual ~cScheduler();
};

END_NETLIB