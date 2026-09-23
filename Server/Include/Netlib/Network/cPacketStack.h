#pragma once
#include "../Common/Netlib.h"
#include "../Common/cHeader.h"

BEGIN_NETLIB

class cPacketStack
{
	static const int TCP_HEADER_SIZE = sizeof( NetLib::cHeader );

private:
	//BYTE	m_Buffer[CSDef::E_NETWORK_PACKET_MAXSENDBUFFERLEN];
	BYTE	m_Buffer[ G_MAXSENDBUFFERLEN ];
	
	UINT	m_uiLength;
	UINT    m_uiMaxLength;
	CSNet::E_PROTOCOL m_eProtocolType;

	// 헤더 정보
	class cHeader* m_pHeader;
	class cUdpHeader* m_pUdpHeader;

public:
	bool Create();
	void Clear();

public:
	// 패킷 정보
	BYTE*	GetBuffer() { return m_Buffer; }
	UINT	GetLength() { return m_uiLength; }

	BOOL CopyPacket(const BYTE* pData, const UINT uiLength);

	UINT GetHeaderSize();
	BYTE* GetBody();
	UINT GetCommand();
	UINT GetPayLoad();
	void SetPacketSeq(const UINT seq);

	UINT GetUDPHeaderSize();
	BYTE* GetUDPBody();
	UINT GetUDPCommand();
	UINT GetUDPPayLoad();

	// 패킷 생성
	void MakeManualEncrypt(	const UINT nCommand,
							const BYTE *lpData,
							const UINT uiLength,
							UINT uiPacketNumber,
							const UINT Entity = 0,
							CSNet::E_PROTOCOL eType = CSNet::E_PROTOCOL::E_TCP);

	void	Make(	const UINT nCommand,
					const BYTE *lpData,
					const UINT uiLength,
					UINT uiPacketNumber,
					const UINT Entity = 0,
					CSNet::E_PROTOCOL eType = CSNet::E_PROTOCOL::E_TCP);

	// 복호화
	void	Decrypt();
	void	Encrypt();

public:
	cPacketStack(CSNet::E_PROTOCOL eProtocolType);
	~cPacketStack();
};

END_NETLIB