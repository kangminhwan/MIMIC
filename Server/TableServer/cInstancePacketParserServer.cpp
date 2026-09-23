#include "cInstancePacketParser.h"
#include "cClientSession.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Manager/cCommandQueueManager.h"
#include "../Include/Netlib/Manager/ServerManager.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Queue/cCommandQueue.h"
#include "../Include/Netlib/Queue/cWebQueue.h"
#include "../Include/Netlib/Network/cContextPooler.h"
#include "../Include/Netlib/Network/cPacketStack.h"

void cInstancePacketParser::ServerInit()
{
	//memset(&m_fpHandlerServerArray, 0x00, sizeof(m_fpHandlerServerArray));

	////스케쥴 처리 함수
	//m_fpHandlerServerArray[CSNet::ProtocolCommand::SYS_SCHEDULE_MSG]							=	&SYS_SCHEDULE_MSG_FP;

	////Context를 완벽히 정리후에 push_front 합니다.
	//m_fpHandlerServerArray[CSNet::ProtocolCommand::SYS_NET_SESSION_LOG_OUT]						=	&SYS_NET_SESSION_LOG_OUT_FP;

	////아이템 픽업 메시지 처리 함수
	//m_fpHandlerServerArray[ServerCommon::SERVER_MSG_ID::SERVER_MSG_PICKED_UP_ITEM]			=	&PICKUP_ITEM_FP;

	////실시간 공지
	//m_fpHandlerServerArray[ServerCommon::SERVER_MSG_ID::SERVER_MSG_SYNC_NOTICE]			=	&SYNC_NOTICE_FP;

	//m_fpHandlerArray[ CSNet::ProtocolCommand::SYS_NET_CONNECT_TO_SERVER ] = &SYS_NET_CONNECT_TO_SERVER_FP;
}

void cInstancePacketParser::Process(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex, NetLib::mRedis* pRedis)
{
	if ((nCommand < General::SysPacket_Heartbeat) || (nCommand > General::SysPacket_End))
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_GRADE::LOG_CRI,
			_T("cInstancePacketParserServer::Process() 4, Command %u, Length, %u, Command Range Over!"),
			nCommand,
			nLength);
		return;
	}

#if defined(PROTOCOL_TRACE_ON) && defined(_DEBUG)

#endif

	/*if (m_fpHandlerServerArray[nCommand])
	{
		(m_fpHandlerServerArray[nCommand])(nID, nCommand, pData, nLength, nThreadIndex, this);
	}*/
	if (m_fpHandlerServerStub[nCommand])
	{
		(m_fpHandlerServerStub[nCommand])(nID, nCommand, pData, nLength, nThreadIndex);
	}
	else
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_CRI,
			_T("cInstanceServerParser::Process 5, Command %u, Length %u this packet is not defined!"),
			nCommand,
			nLength);
	}
}