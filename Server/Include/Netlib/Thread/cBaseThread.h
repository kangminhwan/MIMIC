#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

#define MAX_THREAD_COUNT		100		// 쓰레드 최대 생성제한
#define TRY_THREAD_TERMINATE	10		// 단위, context switching 쓰레드 종료시 기다림시도 최대 숫자, 넘으면 강제종료로 들어감.
#define MAX_THREAD_WAIT			100		// 최대 쓰레드 대기 시간, 이시간뒤에는 일단 기상함.

class cBaseThread
{
protected:
	UINT	m_uiThreadID[MAX_THREAD_COUNT];
	HANDLE	m_hThread[MAX_THREAD_COUNT];
	UINT	m_uiThreadArray[MAX_THREAD_COUNT];
	DWORD   m_dwTlsIndex[MAX_THREAD_COUNT];

	BYTE	m_byThreadCount;

	volatile bool	m_bTerminated;

	HANDLE	m_hEvent;
	HANDLE	m_hCloseEvent;

	tstring	m_tzClassName;

	//std::map<UINT, int>	m_pThreadID_Map;
	//ATL::CAtlMap<UINT, int> m_pThreadID_Map;

	// TSL 에서 쓸 DWORD 변수..
	//DWORD	m_dwTlsIndex;

public:
	void	Init();
	void	Destroy();

public:
    bool	IsTerminated()		{	return m_bTerminated;	}
	void	Terminate()			{	m_bTerminated = true;	}
	void	ForceThreadKill();
	bool	WaitForAllThreadTermination();

	bool	StartThread(const int nThreadCnt, BOOL bCheckPriorityHigh = FALSE);		// Thread 시작
	int		SetEvent()			{ return ::SetEvent(m_hEvent) ; }					// Event를 보내서 쓰레드를 시그널상태로.
	int		SetCloseEvent()		{ return ::SetEvent(m_hCloseEvent) ; }				// Event를 보내서 쓰레드를 종료상태로.

	void	SendThreadCloseEvent();													// 활성화 되어 있는 모든 쓰레드들에 종료 이벤트 날림.

	void	SetClassName( tstring tzName )
	{
		m_tzClassName = tzName;
	}

protected:
    virtual void	Process(UINT ThreadArray) = 0;
	static unsigned __stdcall ThreadProc( void* lpHandle );
public:
	cBaseThread();
	virtual ~cBaseThread();
};

END_NETLIB