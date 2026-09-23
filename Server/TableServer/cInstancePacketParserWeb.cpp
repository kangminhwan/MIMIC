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
//#include "../Include/Netlib/RestSdkHttp/restsdkHttp.h"

#include "cSystemStub.h"

void cInstancePacketParser::WebInit()
{
	/*memset(&m_fpHandlerWebArray, 0x00, sizeof(m_fpHandlerWebArray));

	m_fpHandlerWebArray[Common::MsgID::Msg_TCPChattingLogin]				=	&CONNECT_CLIENT_WEB_REQUEST_FP;
	m_fpHandlerWebArray[Common::MsgID::Msg_ChattingChangeCharacter]		=	&CHANGE_CHARACTER_WEB_REQUEST_FP;*/
}

void cInstancePacketParser::WebProcess(NetLib::cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex)
{
	//사용하면서 검사합니다.
	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();

	if ((nCommand < General::SysPacket_Heartbeat) || (nCommand > General::SysPacket_End))
	{
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("cInstancePacketParser::Process() 6. Command[%u], Length[%u], Command Range over!"),
				nCommand,
				nLength);
		}
		return;
	}

	if (pContext->IsActive() == false)
	{
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("cInstancePacketParser::Process() 7. stop Packet Processing Command[%u], Length[%u]"),
				nCommand,
				nLength);
		}
		return;
	}

	/*if (m_fpHandlerWebArray[nCommand])
	{
		(m_fpHandlerWebArray[nCommand])(pContext, nCommand, pData, nLength, this, nThreadIndex);
	}*/
	if (m_fpHandlerStub[nCommand])
	{
		(m_fpHandlerStub[nCommand])(pContext, nCommand, pData, nLength, nThreadIndex);
	}
	else
	{
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("cInstancePacketParser::Process() 8. Command[%u], Length[%u], this packet is not defined!"),
				nCommand,
				nLength);
		}
	}
}

void cInstancePacketParser::WebProcess(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex)
{
	if ((nCommand < General::SysPacket_Heartbeat) || (nCommand > General::SysPacket_End))
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			_T("cInstancePacketParser::WebProcess 6. Command[%u], Length[%u], Command Range over!"),
			nCommand,
			nLength);
		return;
	}

	if (m_fpHandlerServerStub[nCommand])
	{
		(m_fpHandlerServerStub[nCommand])(nID, nCommand, pData, nLength, nThreadIndex);
	}
	else
	{
		NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("cInstancePacketParser::Process() 8. Command[%u], Length[%u], this packet is not defined!"),
				nCommand,
				nLength);
		}
	}
}

/*
웹에게 패킷을 보낼때 사용됩니다.
이 함수 안에서 최종적으로 ServerHeader 클래스로 래핑하고 보냅니다.
*/
Server::ServiceStatusCode cInstancePacketParser::WebPostRequest(const std::string& url,
	google::protobuf::Message& _postMessage,
	UINT nCommand,
	int iServerID,
	int64 AID,
	std::string& data,
	const int default_time_out_second)
{
	if (url.length() <= 0)
	{
		return Server::ServiceStatusCode::ServiceStatus_EndpointRejected;
	}

	char strPostData[CSDef::EDef::MAX_BUFFER_1024_LEN] = { 0, };
	char strPostMessage[CSDef::EDef::MAX_BUFFER_1024_LEN] = { 0, };
	if (!_postMessage.SerializeToArray(strPostMessage, sizeof(strPostMessage)))
	{
		return Server::ServiceStatusCode::ServiceStatus_MessageEncodeFailed;
	}

	/*
	Server::ServerHeader serverHeader;
	serverHeader.set_data(strPostMessage);
	serverHeader.set_datasize((google::protobuf::uint32)_postMessage.ByteSize());
	serverHeader.set_msgid((ServerCommon::SERVER_MSG_ID)nCommand);
	serverHeader.set_errorcode(Server::ServiceStatusCode::ServiceStatus_Success);
	serverHeader.set_user_idx((google::protobuf::int64)AID);
	serverHeader.set_serverid((google::protobuf::int32)iServerID);

	if (!serverHeader.SerializeToArray(strPostData, sizeof(strPostData)))
	{
		return Server::ServiceStatusCode::ServiceStatus_PostBodyEncodeFailed;
	}

	std::string innerdata;

	ULONGLONG llRequestStart = ::GetTickCount64();

	NetLib::restsdkHttp::RequestHttp(CSNet::WEBREQ_METHOD::POST, url, strPostData, innerdata, default_time_out_second);

	ULONGLONG llRequestEnd = ::GetTickCount64();

	ULONGLONG llSum = llRequestEnd - llRequestStart;

	if (llSum > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_WEB_TIME_OUT)
	{
		return Server::ServiceStatusCode::ServiceStatus_RequestTimedOut;
	}

	Server::ServerHeader _serverHeader;
	if (!_serverHeader.ParseFromArray(innerdata.c_str(), (int)innerdata.size()))
	{
		return Server::ServiceStatusCode::ServiceStatus_WebPayloadParseFailed;
	}

	data = _serverHeader.data();

	return _serverHeader.errorcode();
	*/

	return Server::ServiceStatusCode::ServiceStatus_Success;
}