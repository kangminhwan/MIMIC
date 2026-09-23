#pragma once
#include "Netlib.h"

BEGIN_NETLIB

const UINT HEADER_MAGIC = 0x6B2E;
const BYTE _packetMask = 0xA7;

// packet header
class cHeader
{
protected:
	UINT	m_uIdentity;		// identity value
	UINT	m_uiEntity;			// from context
	UINT	m_uiNonce;			// reserved for future use
	UINT	m_uiCommand;		// packet command
	UINT	m_uiPayload;		// Packet Size
	//WORD	m_wCheckSum;		// CRC 2 BYTE
	UINT	m_uiPacketNum;		// Packet Number

public:
	cHeader()
	{
		m_uiCommand = 0;
		m_uIdentity = 0;
		//m_wCheckSum = 0;
		m_uiPayload = 0;
		m_uiPacketNum = 0;
		m_uiEntity = 0;
		m_uiNonce = 0;
	}

public:
	void SetIdentity() { m_uIdentity = HEADER_MAGIC; }
	void SetPacketSequence(const UINT sequence) { m_uiPacketNum = sequence; }
	void SetCommand(const UINT command) { m_uiCommand = command; }
	void SetPacketHeader(UINT uiPayload,
		UINT Command,
		//WORD wCheckSum = 0,
		UINT PacketNum,
		UINT Entity)
	{
		SetIdentity();
		m_uiPayload = uiPayload;
		m_uiCommand = Command;
		//m_wCheckSum = wCheckSum;
		m_uiPacketNum = PacketNum;
		m_uiEntity = Entity;
	}

	bool IsPerfect() { return (m_uIdentity == HEADER_MAGIC); }
	UINT GetIdentity() { return m_uIdentity; }
	UINT GetPayload() { return m_uiPayload; }
	UINT GetCommand() { return m_uiCommand; }
	UINT GetPacketNum() { return m_uiPacketNum; }
	UINT GetEntity() { return m_uiEntity;  }

	bool CheckPacket()
	{
		if(IsPerfect() == false)
		{
			return false;
		}

		if(GetPayload() >= G_NET_BUFFER_SIZE_32K)
		{
			return false;
		}
		return true;
	}
};

END_NETLIB