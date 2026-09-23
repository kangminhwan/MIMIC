#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Common/cHeader.h"
#include "../../Include/Netlib/Common/cUdpHeader.h"

NetLib::cPacketStack::cPacketStack(CSNet::E_PROTOCOL eProtocolType) :
	m_uiLength(0),
	m_eProtocolType(eProtocolType)
{
	Create();
}


NetLib::cPacketStack::~cPacketStack()
{
}

bool NetLib::cPacketStack::Create()
{
	//memset(m_Buffer, 0x00, CSDef::E_NETWORK_PACKET_MAXSENDBUFFERLEN);
	memset(m_Buffer, 0x00, sizeof(m_Buffer));

	m_uiMaxLength = G_MAXSENDBUFFERLEN;

	m_pHeader = reinterpret_cast<NetLib::cHeader*>(m_Buffer);
	m_pUdpHeader = reinterpret_cast<NetLib::cUdpHeader*>(m_Buffer);

	return true;
}

void NetLib::cPacketStack::Clear()
{
	if(m_uiLength > 0)
		memset(m_Buffer, 0x00, m_uiLength);
	else
		memset(m_Buffer, 0x00, sizeof(m_Buffer));
		//memset(m_Buffer, 0x00, CSDef::E_NETWORK_PACKET_MAXSENDBUFFERLEN);

	m_uiLength = 0;
}

BOOL NetLib::cPacketStack::CopyPacket(const BYTE* pData, const UINT uiLength)
{
	if(uiLength > m_uiMaxLength)
		return FALSE;

	//memcpy(reinterpret_cast<BYTE*>(&m_cHeader), pData, sizeof(cHeader));
	memcpy(m_Buffer, pData, uiLength);
	m_uiLength = uiLength;

	return TRUE;
}

void NetLib::cPacketStack::Make(const UINT nCommand, const BYTE *lpData, const UINT uiLength, UINT uiPacketNumber, const UINT Entity, CSNet::E_PROTOCOL nType)
{
	if((sizeof(cHeader) + uiLength) > m_uiMaxLength)
	{
		printf("cPacketStack::Make sizeof(cHeader) + uiLength Too Big\n");
		return;
	}

	if(m_eProtocolType == CSNet::E_PROTOCOL::E_TCP)
	{
		// CheckSum 2byte 
		WORD wCheck = 0;
		m_pHeader->SetPacketHeader(uiLength, nCommand, wCheck, uiPacketNumber);

		// Header 복사
		//CopyMemory( m_pBuffer, reinterpret_cast<BYTE*>(&m_cHeader), sizeof(cHeader) );

		// Payload복사
		if((lpData != NULL) && (uiLength > 0))
		{
			CopyMemory(m_Buffer + sizeof(NetLib::cHeader), lpData, uiLength);
		}
		
		m_uiLength = uiLength + sizeof(NetLib::cHeader);
	}
	else
	{
		// CheckSum 2byte 
		WORD wCheck = 0;
		m_pUdpHeader->SetPacketHeader(uiLength, nCommand, uiPacketNumber, Entity);

		// Header 복사
		//CopyMemory( m_pBuffer, reinterpret_cast<BYTE*>(&m_cUdpHeader), sizeof(cUdpHeader) );

		// Payload복사
		if((lpData != NULL) && (uiLength > 0))
		{
			CopyMemory(m_Buffer + sizeof(NetLib::cUdpHeader), lpData, uiLength);
		}

		m_uiLength = uiLength + sizeof(NetLib::cUdpHeader);
	}

	Encrypt();
}

void NetLib::cPacketStack::MakeManualEncrypt(const UINT nCommand, const BYTE *lpData, const UINT uiLength, UINT uiPacketNumber, const UINT Entity, CSNet::E_PROTOCOL nType)
{
	if(m_eProtocolType == CSNet::E_PROTOCOL::E_TCP)
	{
		// CheckSum 2byte 
		WORD wCheck = 0;
		m_pHeader->SetPacketHeader(uiLength, nCommand, wCheck, uiPacketNumber);

		// Header 복사
		//CopyMemory( m_pBuffer, reinterpret_cast<BYTE*>(&m_cHeader), sizeof(cHeader) );

		// Payload복사
		if((lpData != NULL) && (uiLength > 0))
		{
			CopyMemory(m_Buffer + sizeof(NetLib::cHeader), lpData, uiLength);
		}

		m_uiLength = uiLength + sizeof(NetLib::cHeader);
	}
	else
	{
		// CheckSum 2byte 
		WORD wCheck = 0;
		m_pUdpHeader->SetPacketHeader(uiLength, nCommand, uiPacketNumber, Entity);

		// Header 복사
		//CopyMemory( m_pBuffer, reinterpret_cast<BYTE*>(&m_cUdpHeader), sizeof(cUdpHeader) );

		// Payload복사
		if((lpData != NULL) && (uiLength > 0))
		{
			CopyMemory(m_Buffer + sizeof(NetLib::cUdpHeader), lpData, uiLength);
		}

		m_uiLength = uiLength + sizeof(NetLib::cUdpHeader);
	}
}

void NetLib::cPacketStack::Encrypt()
{
#ifdef _CRYPT
	for (auto n = TCP_HEADER_SIZE; n < m_uiLength; ++n)
	{
		m_Buffer[n] = m_Buffer[n] ^ _packetMask;
	}
#endif
}

void NetLib::cPacketStack::Decrypt()
{
#ifdef _CRYPT
	for (UINT n = (UINT)sizeof(NetLib::cHeader); n < m_uiLength; ++n)
	{
		m_Buffer[n] = m_Buffer[n] ^ _packetMask;
	}
#endif
}

UINT NetLib::cPacketStack::GetHeaderSize() { return sizeof(NetLib::cHeader); }
BYTE* NetLib::cPacketStack::GetBody() { return m_Buffer + sizeof(NetLib::cHeader); }

UINT NetLib::cPacketStack::GetCommand() { return m_pHeader->GetCommand(); }
UINT NetLib::cPacketStack::GetPayLoad() { return m_pHeader->GetPayload(); }
void NetLib::cPacketStack::SetPacketSeq(const UINT seq) { return m_pHeader->SetPacketSequence(seq); }

UINT NetLib::cPacketStack::GetUDPHeaderSize() { return sizeof(NetLib::cUdpHeader); }
BYTE* NetLib::cPacketStack::GetUDPBody() { return m_Buffer + sizeof(NetLib::cUdpHeader); }
UINT NetLib::cPacketStack::GetUDPCommand() { return m_pUdpHeader->GetCommand(); }
UINT NetLib::cPacketStack::GetUDPPayLoad() { return m_pUdpHeader->GetPayload(); }