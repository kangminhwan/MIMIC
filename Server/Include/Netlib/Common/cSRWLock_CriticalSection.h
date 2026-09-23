#pragma once

BEGIN_NETLIB

/*
* Slim Read Write Lock 을 사용할 경우 Dead Lock 에 주의 해야합니다.
* 다른 락의 경우 같은 스레드에서 이중 락을 사용할 경우 Dead Lock 에 안걸리고 바로 지나갑니다.
* Slim Read Write Lock 은 같은 스레드에서 이중 락을 사용할 경우 바로 Dead Lock 에 빠집니다.
* Dead Lock 에 주의하세요!!!
*/
class cSRWLock_CriticalSection
{
private:
	BOOL m_bUseAcquireSRWLock;
	SRWLOCK m_SRWLock;
	cCriticalSection m_CriticalLock;
public:
	cSRWLock_CriticalSection() :
		m_bUseAcquireSRWLock(FALSE)
	{
		// Windows vista, Windows Server 2008 빌드 버전 => Major 6, Minor 0
		// Windows 7, Windows Server 2008 R2 빌드 버전 => Major 6, Minor 1
		// Windows 8, Windows Server 2012 빌드 버전 => Major 6, Minor 2
		// Windows 8.1, Windows Server 2012 R2 빌드 버전 => Major 6, Minor 3
		// Windows 10 빌드 버전 => Major 10, Minor 0
		// Platrom 은 윈도우 NT 로 확인

		// AcquireSRWLock 은 빌드버전 6.0부터 지원이 됩니다.(vista, window server 2008)
		// 저희는 Windows 7부터 사용되게 확인을 합니다.

		OSVERSIONINFOEX osver = { 0 };
		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		DWORDLONG dwlConditionMask = 0;
		DWORD masks = 0;

		osver.dwMajorVersion = 6;
		dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);

		osver.dwMinorVersion = 1;
		dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

		osver.dwPlatformId = VER_PLATFORM_WIN32_NT;
		dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);

		masks = VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID;

		m_bUseAcquireSRWLock = VerifyVersionInfo(&osver, masks, dwlConditionMask);

		if (m_bUseAcquireSRWLock == TRUE)
		{
			InitializeSRWLock(&m_SRWLock);
		}
	}

	~cSRWLock_CriticalSection()
	{

	}

	// read_writer only Lock
	void AcquireLockExclusive()
	{
		if (m_bUseAcquireSRWLock == TRUE)
		{
			::AcquireSRWLockExclusive(&m_SRWLock);
		}
		else
		{
			m_CriticalLock.Lock();
		}
	}

	// read_writer only Release
	void ReleaseLockExclusive()
	{
		if (m_bUseAcquireSRWLock == TRUE)
		{
			::ReleaseSRWLockExclusive(&m_SRWLock);
		}
		else
		{
			m_CriticalLock.Unlock();
		}
	}

	// read-only Lock
	void AcquireLockShared()
	{
		if (m_bUseAcquireSRWLock == TRUE)
		{
			::AcquireSRWLockShared(&m_SRWLock);
		}
		else
		{
			m_CriticalLock.Lock();
		}
	}

	// read-only Release
	void ReleaseLockShared()
	{
		if (m_bUseAcquireSRWLock == TRUE)
		{
			::ReleaseSRWLockShared(&m_SRWLock);
		}
		else
		{
			m_CriticalLock.Unlock();
		}
	}
};

/*
* Slim Read Write Lock 을 사용할 경우 Dead Lock 에 주의 해야합니다.
* 다른 락의 경우 같은 스레드에서 이중 락을 사용할 경우 Dead Lock 에 안걸리고 바로 지나갑니다.
* Slim Read Write Lock 은 같은 스레드에서 이중 락을 사용할 경우 바로 Dead Lock 에 빠집니다.
* Dead Lock 에 주의하세요!!!
*/
class cUnionLock
{
private:
	cSRWLock_CriticalSection* m_pLock;
	BOOL m_bRead_Only;
public:
	cUnionLock(cSRWLock_CriticalSection* pLock, BOOL bRead_Only = TRUE) :
		m_pLock(pLock),
		m_bRead_Only(bRead_Only)
	{
		if (m_bRead_Only == TRUE)
		{
			m_pLock->AcquireLockShared();
		}
		else
		{
			m_pLock->AcquireLockExclusive();
		}
	}

	~cUnionLock()
	{
		if (m_bRead_Only == TRUE)
		{
			m_pLock->ReleaseLockShared();
		}
		else
		{
			m_pLock->ReleaseLockExclusive();
		}
	}
};

END_NETLIB