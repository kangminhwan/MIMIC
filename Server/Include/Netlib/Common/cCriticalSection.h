// cCriticalSection.h: interface for the cCriticalSection class.
// 디파인된 코어이상의 CPU가 있을경우는 spinlock을 시도한다.
// 그이외에는 critical lock만 시도한다.
//////////////////////////////////////////////////////////////////////
#pragma once

BEGIN_NETLIB

const BYTE MINIMUM_CPU_CNT_FOR_SPIN_LOCK_ON = 0x02;
const DWORD SPIN_LOCK_TRY_CNT = 4000;

class cCriticalSection
{
public:
	cCriticalSection()
	{
		if(Is_SMP_System())
		{
#if( defined( _WIN32_WINNT ) && ( _WIN32_WINNT >= 0x0403 ) )
			// 멀티코어에서 유용한..
			InitializeCriticalSectionAndSpinCount(&m_CriticalSection, SPIN_LOCK_TRY_CNT);
#else
			InitializeCriticalSection(&m_CriticalSection);
#endif
		}
		else
		{
			InitializeCriticalSection(&m_CriticalSection);
		}
	}

	~cCriticalSection() { DeleteCriticalSection(&m_CriticalSection); }

public:
	void	Lock() { EnterCriticalSection(&m_CriticalSection); }
	void	Unlock() { LeaveCriticalSection(&m_CriticalSection); }

	static bool	Is_SMP_System()
	{
		SYSTEM_INFO		TSystemInfo;

		GetSystemInfo(&TSystemInfo);

		DWORD dwCpuCnt = TSystemInfo.dwNumberOfProcessors;

		if(dwCpuCnt >= MINIMUM_CPU_CNT_FOR_SPIN_LOCK_ON) // 멀티 코어 일경우만 스핀락을 시도한다.
			return true;

		return false;
	}

private:
	CRITICAL_SECTION	m_CriticalSection;
};

class cCSLock
{
	cCriticalSection* m_pCriticalSection;
	BOOL m_bThreadSafe;

private:
	cCSLock() {}

public:
	cCSLock(cCriticalSection* pCriticalSecection, BOOL bThreadSafe = TRUE)
	{
		m_pCriticalSection = pCriticalSecection;
		m_bThreadSafe = bThreadSafe;

		if(m_bThreadSafe)
			m_pCriticalSection->Lock();
	}

	~cCSLock()
	{
		if (m_bThreadSafe)
			m_pCriticalSection->Unlock();
	}
};

END_NETLIB