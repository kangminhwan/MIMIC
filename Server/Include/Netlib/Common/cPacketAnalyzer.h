#pragma once
#include "Netlib.h"

BEGIN_NETLIB

class cPacketAnalyzer
{
protected:
	ULONGLONG m_ullStartTime;
	DWORD m_dwEndTime;
	DWORD m_dwPresentTime;

	DWORD m_dwRecvDataSize;
	DWORD m_dwSendDataSize;
	DWORD m_dwConnectionTime;

	int		m_iRecvPacketCnt;
	int		m_iSendPacketCnt;

public:
	cPacketAnalyzer()
	{
		Clear();
	}

	virtual ~cPacketAnalyzer()
	{
	}

	virtual void Clear()
	{
		m_ullStartTime = 0;
		m_dwEndTime = 0;
		m_dwPresentTime = 0;

		m_iRecvPacketCnt = 0;
		m_iSendPacketCnt = 0;

		m_dwRecvDataSize = 0;
		m_dwSendDataSize = 0;
		m_dwConnectionTime = 0;
	}

	void IncRecvPacket(DWORD dwRecvDataSize)
	{
		m_dwRecvDataSize += dwRecvDataSize;
		++m_iRecvPacketCnt;
	}
	void IncSendPacket(DWORD dwSendDataSize)
	{
		m_dwSendDataSize += dwSendDataSize;
		++m_iSendPacketCnt;
	}

	int GetRecvPacketCnt()
	{
		return m_iRecvPacketCnt;
	}
	int GetSendPacketCnt()
	{
		return m_iSendPacketCnt;
	}

	void SetStartTime()
	{
		cPacketAnalyzer::Clear();

		m_ullStartTime = ::GetTickCount64();
	}

	void PacketAnalizeNow(TPacketAnalyze& refPacketAnalyze)
	{
		// 몇초만에 분석하는건가?
		ULONGLONG m_ullConnectionTime = ::GetTickCount64() - m_ullStartTime;
		ULONGLONG iEvaluateSecond = m_ullConnectionTime / 1000;

		float fRecvPacketPerMinute = (float)m_iRecvPacketCnt / iEvaluateSecond * 60;
		float fSecvPacketPerMinute = (float)m_iSendPacketCnt / iEvaluateSecond * 60;

		refPacketAnalyze.ullConnectionTime = m_ullConnectionTime;
		refPacketAnalyze.dwRecvDataSize = m_dwRecvDataSize;
		refPacketAnalyze.dwSendDataSize = m_dwSendDataSize;
		refPacketAnalyze.dwRecvPacketPerMinute = (DWORD)fRecvPacketPerMinute;
		refPacketAnalyze.dwSendPacketPerMinute = (DWORD)fSecvPacketPerMinute;
	}
};

END_NETLIB