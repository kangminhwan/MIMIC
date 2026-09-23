#pragma once
#include "Netlib.h"

BEGIN_NETLIB

class cSession;
class cInterfaceIocpContext
{
public:
	virtual volatile bool IsActive() = 0;
	virtual bool IsCrypt() = 0;
	virtual void SetActive(volatile bool bActive) = 0;
	virtual void SetCrypt(bool bCrypt) = 0;

	virtual void Disconnect(E_IO_OPERATION eOperation = E_IO_DISCONNECT) = 0;
	virtual	UINT GetEntity() = 0;

	virtual bool ReCreateSocket(BOOL bUseIPv6 = FALSE) = 0;

	virtual void ForceDisconnect() = 0;

	virtual void Reset(DWORD lNumberOfBytesTransferred) = 0;

	virtual SOCKET GetSockHandle() = 0;

	virtual size_t	GetSendOvlCnt() = 0;

	virtual E_ERROR_SEND SendRequest(BYTE* lpData, UINT nDataSize) = 0;

protected:
	virtual void SetContextStatus(E_CONNECT_STATUS eContextStatus) = 0;
	virtual void SetContextType(E_CONTEXT_TYPE eContextType) = 0;

public:
	virtual E_CONNECT_STATUS GetContextStatus() = 0;
	virtual E_CONTEXT_TYPE GetContextType() = 0;

	virtual uint32 GetIP() = 0;
	virtual uint32  GetPORT() = 0;
	virtual char* GetIPv6() = 0;
	virtual sockaddr_in6*  GetSockInfo6() = 0;
	virtual sockaddr_in*  GetSockInfo() = 0;
	virtual BOOL IsUseIPv6() = 0;
	virtual void IncreasePoolSize(const int iMaxPoolSize) = 0;

	virtual void PrintDamangePacketCapture() = 0;

	virtual void SetUniqueKey() = 0;
	virtual ULONGLONG GetUniqueKey() = 0;
	virtual ULONGLONG GetUniqueKey() const = 0;

	virtual void ChangeToServerContextType() = 0;

#ifdef USING_MULTI_THREAD
	//virtual void SetContextRoom(UINT uRoomNum) = 0;
	virtual void SetCommandQueueIndex(UINT uiCommandQueueIndex) = 0;
	virtual void SetDefaultCommandQueueIndex() = 0;
	virtual UINT GetContextRoom() = 0;
	virtual UINT GetCommandQueueIndex() = 0;
#endif

#if defined(PACKET_ANALYZE_ON)
	virtual void PacketAnalizeNow(TPacketAnalyze& refPacketAnalyze) = 0;
	virtual int GetRecvPacketCnt() = 0;
	virtual int GetSendPacketCnt() = 0;
	virtual void IncRecvPacket(DWORD dwRecvDataSize) = 0;
#endif

	virtual void SetAllocateSlot(int64 nAllocateSlot) = 0;
	virtual int64 GetAllocateSlot() = 0;

	void SetSession(cSession* pSession)
	{
		m_pSession = pSession;
	}

	cSession* GetSession()
	{
		return m_pSession;
	}

	void SetDamagePacketCaptureOn(const BOOL bOn) { m_bDamagePacketCapture = bOn; }

public:
	cInterfaceIocpContext() {}
	virtual ~cInterfaceIocpContext() = 0{}

protected:
	cSession* m_pSession; // 접속했을때, 클라이언트의 정보참조용..
	BOOL m_bDamagePacketCapture;

#ifdef USE_PACKET_TICK_CHECK_CONTEXT
protected:
	ULONGLONG m_uiLastPacketTick;// 최종 패킷 전송시간, 비정상 접속 처리용
	ULONGLONG m_ullPendingDelayIOS;// IOS는 백그라운드에서 핑을 못쏘기 때문에, 30초동안은 펜딩상태 체크하지 않습니다.

public:
	void SetCurrentPacketTick()
	{
		m_uiLastPacketTick = ::GetTickCount64();
	}
	ULONGLONG GetCurrentPacketTick()
	{
		return m_uiLastPacketTick;
	}
	void SetIosPendingDelayTime(ULONGLONG pendingTime)// 단위 ms
	{
		if (pendingTime == 0)
			m_ullPendingDelayIOS = 0;
		else
			m_ullPendingDelayIOS = GetTickCount64() + pendingTime;
	}
	ULONGLONG GetIosPendingDelayTime()
	{
		return m_ullPendingDelayIOS;
	}
#endif
#ifdef VIRTUAL_NAGLE_ON_OFF
public:
	virtual void SetVirtualNagleOnOff(BOOL bVirtualNagleOnOff) = 0;
	virtual void SendVirtualNalePackets() = 0;
#endif
};

END_NETLIB