#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cCommandQueue;
class cIocpContext;
class cUDPDispatcher;
class cUDPIocpOv;
class cCommandQueueManager
{
private:
	cCommandQueue**			m_pCommandQueue;
	int						m_nCommandQueueCnt;
	int						m_nServerCommandArray;

public:
	bool	Init(const int nCommandQueueCnt, const int nCommandQueuePoolSize);
	int		GetCommandQueueCnt() { return m_nCommandQueueCnt; }
	int		GetServerCommandQueueArray() { return m_nServerCommandArray; }
	cCommandQueue* GetCommandQueuePtr(const int nThreadArray);
	int		GetRoomThreadNumber(int nRoomNo);

	void	PrintAliveStatus();

	bool	PushCommand(cIocpContext* pContext,
		const UINT nCommand,
		const BYTE* lpBuffer = NULL,
		const WORD nLength = 0);

	bool	PushCommand(const UINT nID,
		const UINT nCommand,
		const BYTE* lpBuffer = NULL,
		const WORD nLength = 0);

	void	BroadCastCommand(const UINT nID,
		const UINT nCommand,
		const BYTE* lpBuffer = NULL,
		const WORD nLength = 0);

	void	BroadCastCommandScheduleJob(const UINT nID,
		const UINT nCommand,
		stSchedule* pSchedule,
		const WORD nLength = 0);

	void	RandomCastCommandScheduleJob(const UINT nID,
		const UINT nCommand,
		stSchedule* pSchedule,
		const WORD nLength = 0);

	bool	PushCommand(const int nRoomNo,
		const UINT nID,
		const UINT nCommand,
		const BYTE* lpBuffer = NULL,
		const WORD nLength = 0);

	bool	PushCommand(const cUDPDispatcher* pUDPDispatcher,
		const UINT nCommand,
		const UINT Entity,
		const UINT uiPacketSeq,
		const cUDPIocpOv* pUdpIocpOv,
		const BYTE* lpBuffer = NULL,
		const WORD nLength = 0);

public:
	cCommandQueueManager();
	~cCommandQueueManager();
};

END_NETLIB