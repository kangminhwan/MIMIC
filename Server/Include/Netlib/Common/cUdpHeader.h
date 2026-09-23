#pragma once
#include "Netlib.h"

BEGIN_NETLIB

//const WORD AF_UDP_PACKET_IDENTITY = 0x6699;
const WORD AF_UDP_PACKET_IDENTITY = 0x6B2E; // 테스트용으로 일단 헤더 맞춤

											// packet header
class cUdpHeader
{
protected:
	UINT	m_uiCommand;		// packet command
	UINT	m_uiPacketNum;		// Packet Number
	UINT	m_uiPayload;		// Packet Size
	UINT	m_uIdentity;		// identity value
								//WORD	m_wCheckSum;		// CRC 2 BYTE
	UINT	m_uiEntity;			// from context



public:
	cUdpHeader()
	{
		m_uiCommand = 0;
		m_uIdentity = AF_UDP_PACKET_IDENTITY;
		//m_wCheckSum = 0;
		m_uiPayload = 0;
		m_uiPacketNum = 0;
	}

	void SetIdentity() { m_uIdentity = AF_UDP_PACKET_IDENTITY; }
	void SetPacketHeader(UINT uiPayload = 0,
		UINT Command = 0,
		UINT PacketNum = 0,
		UINT Entity = 0)
	{
		SetIdentity();
		m_uiPayload = uiPayload;
		m_uiCommand = Command;
		//m_wCheckSum = wCheckSum;
		m_uiPacketNum = PacketNum;
		m_uiEntity		= Entity;
	}

	bool IsPerfect() { return m_uIdentity == AF_UDP_PACKET_IDENTITY; }
	WORD GetIdentity() { return m_uIdentity; }
	UINT GetPayload() { return m_uiPayload; }
	UINT GetCommand() { return m_uiCommand; }
	UINT GetPacketNum() { return m_uiPacketNum; }
	UINT GetEntity()	{ return m_uiEntity;		}
};

END_NETLIB