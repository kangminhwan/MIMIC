#pragma once
#include "../Common/cInterfaceIocpContext.h"
#include "cSocket.h"
#include "../Buffer/cBuffer.h"

BEGIN_NETLIB

class cIocpContext : public cInterfaceIocpContext, public cSocket
{
private:
	friend class cWorkerThread;
	friend class cContextPooler;

private:
	volatile bool		m_bActive;
	bool				m_bAssociated;
	bool				m_bCrypt;
	bool				m_bConnector;
	bool				m_bEnterContextTable;
	bool				m_bWorkerThreadSignal;

	UINT				m_uiEntity;
	UINT				m_uiSequence;
	static int			m_uiEntityNum;

	int64				m_i64AllocateSlot;

	BYTE				m_byEncryptKey;
	BYTE				m_byDecryptKey;

	ULONGLONG			m_llUniquekey;
	// 클라이언트의 현재 상태를 표시하는 flag
	E_CONNECT_STATUS	m_eContextStatus;

	// Context 의 타입
	E_CONTEXT_TYPE		m_eContextType;

	POSITION			m_pos;	//	ContextPooler의 position 정보

#ifdef USING_MULTI_THREAD
	UINT		m_uRoomNum;			// 이소켓이 속해 있는 방의 번호
	UINT		m_uCommandQueueIndex;//이소켓이 속해 있는 컨텍스트 인덱스
#endif

protected:
	cBuffer	m_cStorage;

public:
	std::vector<BYTE*> m_DamagePackets;

private:
	void	Init(bool bConnector = false);

	void	SetPos(POSITION _pos) { m_pos = _pos; }
	POSITION GetPos() { return m_pos; }

public:
	void	Destroy();

	void	SetEntity(UINT Entity) { m_uiEntity = Entity; }

	void	SetSequence(UINT uiSequence) { m_uiSequence = uiSequence; }

	UINT	GetSequence() { return ++m_uiSequence; }

	void	Associate(bool bAssociated) { m_bAssociated = bAssociated; }
	bool	IsAssociated() { return m_bAssociated; }

	bool	IsEnterContextTable() { return m_bEnterContextTable; }
	void	EnterContextTable() { m_bEnterContextTable = true; }

	bool	WorkerThreadSignal() { return m_bWorkerThreadSignal; }
	void	SetWorkerThreadSignal(bool bSignal) { m_bWorkerThreadSignal = bSignal; }

#ifdef USING_MULTI_THREAD
	//virtual void	SetContextRoom(UINT uRoomNum) override;
	virtual void	SetCommandQueueIndex(UINT uiCommandQueueIndex) override { m_uCommandQueueIndex = uiCommandQueueIndex; }
	virtual void	SetDefaultCommandQueueIndex() override;
	virtual UINT	GetContextRoom() override { return m_uRoomNum; }
	virtual UINT	GetCommandQueueIndex() override { return m_uCommandQueueIndex; }
#endif

public:
	

	// 임시 버퍼 처리 펑션모음
public:
	BOOL	CreateStorage(UINT uiBuffLen = G_DEFIOBUFFERLEN) { return m_cStorage.Create(uiBuffLen); }
	void	DestroyStorage() { m_cStorage.Destroy(); }
	BOOL	StoreBuffer(BYTE* pBuffer, UINT uiLength) { return m_cStorage.Copy(pBuffer, uiLength); }
	BOOL	AppendBuffer(BYTE* pBuffer, UINT uiLength) { return m_cStorage.Append(pBuffer, uiLength); }
	void	CleanStorage() { m_cStorage.Erase(); }
	BYTE*	GetStorageBuffer() { return m_cStorage.GetBuffer(); }
	UINT	GetStorageLength() { return m_cStorage.GetLength(); }
	UINT	GetStorageMaxLength() {return m_cStorage.GetMaxLength();}

	BYTE	GetEncryptKey() { return m_byEncryptKey; }
	BYTE	GetDecryptKey() { return m_byDecryptKey; }

	// cInterfaceIocpContext의 virtual 함수들
private:
	virtual void SetContextStatus(E_CONNECT_STATUS eContextStatus) override { m_eContextStatus = eContextStatus; }//m_llUniquekey.ContextKey.ContextType = static_cast<unsigned char>(m_eContextStatus); }
	virtual void SetContextType(E_CONTEXT_TYPE eContextType) override { m_eContextType = eContextType; }

public:
	virtual void Clear() override 
	{ 
#ifdef PACKET_ANALYZE_ON
		cPacketAnalyzer::Clear();
#endif
		Init(m_bConnector); 
	}

	virtual volatile bool	IsActive() override { return m_bActive; }
	virtual bool	IsCrypt() override { return m_bCrypt; }
	virtual void	SetActive(volatile bool bActive) override { m_bActive = bActive; }
	virtual void    SetCrypt(bool bCrypt) override { m_bCrypt = bCrypt; }
	bool	IsConnector() { return m_bConnector; }
	virtual void	Disconnect(E_IO_OPERATION eOperation = E_IO_DISCONNECT) override;
	virtual UINT	GetEntity() override { return m_uiEntity; }
	virtual uint32  GetIP() override { return cSocket::m_nIP; }
	virtual uint32  GetPORT() override { return cSocket::m_nPeerPort; }
	virtual char* GetIPv6() override { return cSocket::GetIPv6(); }
	virtual sockaddr_in6*  GetSockInfo6() override { return cSocket::GetSockaddr_in6(); }
	virtual sockaddr_in*  GetSockInfo() override { return cSocket::GetSockaddr_in(); }
	virtual BOOL IsUseIPv6() override { return cSocket::IsUseIPv6(); }
	virtual void IncreasePoolSize(const int iMaxPoolSize) override { cSocket::IncreasePoolSize(iMaxPoolSize); }
	virtual E_CONNECT_STATUS GetContextStatus() override { return m_eContextStatus; }
	virtual E_CONTEXT_TYPE GetContextType() override { return m_eContextType; }

	virtual void SetUniqueKey() override;
	virtual ULONGLONG GetUniqueKey() override;
	virtual ULONGLONG GetUniqueKey() const override;

	virtual bool	ReCreateSocket(BOOL bUseIPv6 = FALSE) override
	{
		// 소켓을 재생성했으면 다시 IOCP에 등록시켜 주기위해 필요하다..
		Associate(false);
		return cSocket::ReCreateSocket(bUseIPv6);
	}

	virtual void	ForceDisconnect() override;
	virtual void	Reset(DWORD lNumberOfBytesTransferred) override;
	virtual SOCKET	GetSockHandle() override { return cSocket::GetSockHandle(); }
	virtual size_t	GetSendOvlCnt() override { return cSocket::GetSendOvlCnt(); }

	virtual E_ERROR_SEND SendRequest(BYTE* pData, UINT uiDataSize) override;

	std::vector<BYTE*>* GetDamagePacketList() { return &m_DamagePackets; }
	virtual void PrintDamangePacketCapture() override;

	virtual void ChangeToServerContextType() override { m_eContextType = E_CONTEXT_TYPE::E_CONTEXT_SERVER; }

#if defined(PACKET_ANALYZE_ON)
	virtual void PacketAnalizeNow(TPacketAnalyze& refPacketAnalyze) override { cSocket::PacketAnalizeNow(refPacketAnalyze); }

	virtual int GetRecvPacketCnt() override { return cSocket::GetRecvPacketCnt(); }

	virtual int GetSendPacketCnt() override { return cSocket::GetSendPacketCnt(); }

	virtual void IncRecvPacket(DWORD dwRecvDataSize) override { cSocket::IncRecvPacket(dwRecvDataSize); return; }
#endif

	virtual void SetAllocateSlot(int64 _i64AllocateSlot) override { 
		m_i64AllocateSlot = _i64AllocateSlot; 
	}
	virtual int64 GetAllocateSlot() override { return m_i64AllocateSlot; }

#ifdef VIRTUAL_NAGLE_ON_OFF
	virtual void SetVirtualNagleOnOff(BOOL bVirtualNagleOnOff) override { cSocket::m_bVirtualNagleOnOff = bVirtualNagleOnOff; }
	virtual void SendVirtualNalePackets() override { NetLib::cSocket::SendVirtualNalePackets(); }
#endif

	//IOCP Connector 상태처리
protected:
	E_IOCP_CONNECTOR_STATUS m_eConnectorStatus;
	char m_strConnectorIP[CSDef::EDef::MAX_IP_ADDRESS_LEN];
	UINT m_uiConnectorPort;
	int  m_nConnectorIPHint;

public:
	void SetConnectorStatus(E_IOCP_CONNECTOR_STATUS status) { 
		m_eConnectorStatus = status; 
	}
	E_IOCP_CONNECTOR_STATUS GetConnectorStatus() { return m_eConnectorStatus; }
	void SetConnectorInfo(const std::string& Ip, UINT Port);
	char* GetConnectorTargetIP() { return m_strConnectorIP; }
	UINT GetConnectorTargetPort() { return m_uiConnectorPort; }
	const int GetConnectorIPHint() { return m_nConnectorIPHint; }
	void DestroyAccept();

	
private:
	std::atomic<bool> m_bClosing{ false };
public:
	bool IsClosing() const { return m_bClosing.load(); }

	void MarkClosing()
	{
		bool expected = false;
		if (!m_bClosing.compare_exchange_strong(expected, true))
			return; // 이미 종료 중이면 무시
	}
	void ResetClosing() { m_bClosing.store(false); }
public:
	cIocpContext();
	cIocpContext(bool bConnector, const char* TargetIP, UINT Port);
	virtual ~cIocpContext();
};

END_NETLIB