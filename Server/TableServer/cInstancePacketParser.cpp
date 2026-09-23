#include "cInstancePacketParser.h"

#include <sstream>
// 자동 등록 ? Convert 함수 + AutoRegister 정의 헤더
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

#include "cSystemStub.h"
#include "cProtoMsgStub.h"
#include "cProtoUtil.h"
#include "cGameVersionChecker.h"
#include "Query.h"
#include "StringUtil.h"

#include <google/protobuf/util/json_util.h>

cInstancePacketParser::cInstancePacketParser()
{
	Init();
	WebInit();
	ServerInit();
}

cInstancePacketParser::~cInstancePacketParser()
{
	NetLib::cVector<IStub*>::const_iterator iter = m_vecStub.begin();
	NetLib::cVector<IStub*>::const_iterator iter_end = m_vecStub.end();
	for (; iter != iter_end; ++iter) {
		if ((*iter) != nullptr) {
			delete (*iter);
		}
	}
}

void cInstancePacketParser::Init()
{
	//	Protobuf Message 처리Stub
	IStub* pStub = new cProtoMsgStub();
	pStub->bindmethod(m_fpHandlerStub);
	pStub->bindmethod(m_fpHandlerServerStub);
	pStub->SetOwner(this);
	m_vecStub.push_back(pStub);

	// 스케줄 관련 Stub
	// 시스템 메시지 관련 Stub
	pStub = new cSystemStub();
	pStub->bindmethod(m_fpHandlerServerStub);
	pStub->SetOwner(this);
	m_vecStub.push_back(pStub);

	CreateClientSession();
	//InitializeGameVersionChecker();
}

void cInstancePacketParser::NotifyContextLogout(UINT Entity)
{
#ifdef USING_MULTI_THREAD
	NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr(0);
	if (pCommandQueue == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, _T("NotifySessionLogout is Failed. CommandQueue is nullptr"));
		return;
	}
#else
	NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueue>::ExistsInstance();
	if (pCommandQueue == nullptr)
	{
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("NotifySessionLogout is Failed. CommandQueue is nullptr"));
		}
		return;
	}
#endif

	NetLib::cIocpContext* pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->FindConnectedContext(Entity, -1);
	if (pContext == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NotifySessionLogout is Failed. cIocpContext is nullptr");
		return;
	}

	pCommandQueue->PushCommand(pContext, CSNet::ProtocolCommand::SYS_NET_DISCONNECT);
}

void cInstancePacketParser::CreateClientSession()
{
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if (!pServerManager)
	{
		return;
	}

	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if (!pConfig)
	{
		::OutputDebugString(_T("cInstancePacketParser::CreateClientSession() failed. no Server Config"));
		::MessageBox(NULL, _T("cInstancePacketParser::CreateClientSession() failed. no Server Config"), _T("No Server Config"), MB_ICONEXCLAMATION);
		return;
	}

	NetLib::cSessionManager* pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::ExistsInstance();
	if (pSessionManager == nullptr)
	{
		return;
	}
	pSessionManager->Init();

	for (int n = 0; n < pConfig->wMaxUser; ++n)
	{
		pSessionManager->PushSession(cClientSession::CreateSession());
	}
}

void cInstancePacketParser::InitializeGameVersionChecker()
{
	//NetLib::cSingleton<cGameVersionChecker>::GetInstance();

	// 버젼 데이터를 불러온다.
	std::vector<Server::Version> versions;
	auto result = QueryManager::GetSetVersionsAsync( versions );
	result.wait();

	if ( result.get() == FALSE )
		throw std::runtime_error( "InitializeGameVersionChecker Failed" );

	for ( const auto& version : versions ) {

		NetLib::cSingleton<cGameVersionChecker>::GetInstance()->PushVersion( version );

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( version , &serializedData );

		auto korean = StringUtil::Utf8ToWide( serializedData );

		std::wcout << korean << std::endl;
	}

	// 테스트 코드
	//NetLib::cSingleton<cGameVersionChecker>::GetInstance()->CompareVersions( General::StoreChannel::StoreChannel_GooglePlay, "0.062");
}

void cInstancePacketParser::Process( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{

	if ( ( nCommand < General::SysPacket_None ) || ( nCommand >= General::Packet_End ) )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_GRADE::LOG_CRI ,
			_T( "cInstancePacketParser::Process() 1. Command %u, Length %u, Command Rage over!" ) ,
			nCommand , nLength );
		return;
	}

	if ( pContext->IsActive() == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_GRADE::LOG_CRI ,
			_T( "cInstancePacketParser::Process() 2. Stop Packet Processing Command %u, Length %u" ) ,
			nCommand , nLength );
		return;
	}

	if ( m_fpHandlerStub[ nCommand ] )
	{
		// 시작 시간 기록
		auto start = std::chrono::high_resolution_clock::now();

		// 함수 호출
		m_fpHandlerStub[ nCommand ]( pContext , nCommand , pData , nLength , nThreadIndex );

		// 끝 시간 기록
		auto end = std::chrono::high_resolution_clock::now();

		// 시간 차이 계산 (밀리초 단위로 출력)
		auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( end - start );
		if ( duration.count() > 800 )
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			   LOG_GRADE::LOG_CRI ,
			   _T( "PacketDuration: Command %u, Length %u, DelayTime %u" ) ,
			   nCommand , nLength , duration.count() );
		}
		//( m_fpHandlerStub[ nCommand ] )( pContext , nCommand , pData , nLength , nThreadIndex );
	}
	else
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_GRADE::LOG_CRI ,
			_T( "cInstancePacketParser::Process() 3. Command %u, Length %u, this packet is not defined!" ) ,
			nCommand , nLength );
		SendUnDefinedPacket( pContext , nCommand , nThreadIndex );
	}

}

//	에러 메시지를 날리고 bDisconnectFlag 이 값이 True면 DisConnect 합니다.
void cInstancePacketParser::SendErrorWithDisConnect(UINT nCommand,
	const int nErrorCode,
	std::string& ErrorString,
	UINT nThreadIndex,
	NetLib::cInterfaceIocpContext* pContext,
	google::protobuf::Message& _message,
	BOOL bDisconnectFlag)
{
	NetLib::cCommandQueueManager* pCommandManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	if (pCommandManager == nullptr)
	{
		assert(false && "cInstancePacketParser::SendErrorWithDisConnect Failed. cCommandQueueManager is nullptr.");
		return;
	}

	if (General::ResultCode_IsValid(nErrorCode) == false)
	{
		if (bDisconnectFlag)
		{
			pCommandManager->PushCommand(static_cast<NetLib::cIocpContext*>(pContext), CSNet::ProtocolCommand::SYS_NET_DISCONNECT);
		}
		assert(false && "cInstancePacketParser::SendErrorWithDisConnect Failed. Invalid ErrorCode.");
		return;
	}

	if (General::SysPacketID_IsValid(static_cast<int>(nCommand)) == false)
	{
		if (bDisconnectFlag)
		{
			pCommandManager->PushCommand(static_cast<NetLib::cIocpContext*>(pContext), CSNet::ProtocolCommand::SYS_NET_DISCONNECT);
		}
		assert(false && "cInstancePacketParser::SendErrorWithDisConnect Failed. Invalid Command.");
		return;
	}

	General::ResultCode eErrorCode = static_cast<General::ResultCode>(nErrorCode);

	SendBuffer(pContext , nThreadIndex , nCommand , _message , eErrorCode , ErrorString);

	if (bDisconnectFlag)
	{
		pCommandManager->PushCommand(static_cast<NetLib::cIocpContext*>(pContext), CSNet::ProtocolCommand::SYS_NET_DISCONNECT);
	}
}

//최종적으로 클라이언트에게 보낼때 이 함수를 사용합니다.
bool cInstancePacketParser::SendBuffer(NetLib::cInterfaceIocpContext* pContext,
	UINT nThreadIndex,
	UINT nCommand,
	google::protobuf::Message& _message,
	General::ResultCode errorCode,
	const std::string& errorString,
	e_thread_type thread)
{
	//사용하면서 검사합니다.
	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();

	if (pContext == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "SendBuffer is Failed. pContext is nullptr nCommand[%u]", nCommand);

		return false;
	}

	PmNet::PktBase protoBase;
	protoBase.set_err_kind( errorCode );
	protoBase.set_errtag( errorString );

	//에러 스트링이 길이가 0보다 커야지 변환 시도해야합니다.
	//내부적으로 0보다 작으면 디버그 모드에서는 Assert가 생깁니다.
	if (errorString.length() > 0)
	{
		//이쪽에 에러스트링을 보낼것인지 안보낼것이지를 확인하는 if문이 들어가야합니다.
		//protoutil::cProtoUtil::SetProtoBuffer_String<PmNet::PktBase>(&protoBase,
			//errorString,
			//&PmNet::PktBase::set_errorstring);
	}

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = nullptr;

	switch (thread)
	{
	case e_thread_type::e_command_thread:
		pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer(nThreadIndex);
		break;
	case e_thread_type::e_web_thread:
		pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetWebProtobufBuffer(nThreadIndex);
		break;
	case e_thread_type::e_udp_thread:
		pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetUdpProtobufBuffer(nThreadIndex);
		break;
	default:
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "SendBuffer is Failed. Thread Type Default!!");
		return false;
	}

	if ( pGOOGLE_PROTO_BUFFER == nullptr )
		return false;

	if (!_message.SerializeToArray(pGOOGLE_PROTO_BUFFER->DataBuffer, (int)sizeof(pGOOGLE_PROTO_BUFFER->DataBuffer)))
	{
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("SendBuffer is Failed. ProtoBuffer Message Serialize Failed. nCommand[%u], Entity[%4d], Socket[%u]"),
				nCommand,
				pContext->GetEntity(),
				(unsigned int)pContext->GetSockHandle());
		}
		return false;
	}
	UINT uiMessageSize = (UINT)_message.ByteSizeLong();

	//   3) wire 송신
	// 변환 실패(Conv 미등록) 시 protobuf 경로로 폴백.


	protoBase.set_payload(pGOOGLE_PROTO_BUFFER->DataBuffer, uiMessageSize);
	protoBase.set_payload_size(uiMessageSize);

	if (!protoBase.SerializeToArray(pGOOGLE_PROTO_BUFFER->SerializeBuffer, (int)sizeof(pGOOGLE_PROTO_BUFFER->SerializeBuffer)))
	{
		if (pLogQueue)
		{
			pLogQueue->PushCommand(LOG_GRADE::LOG_CRI,
				_T("SendBuffer is Failed. ProtoBuffer pvpResponse Serialize Failed. nCommand[%u], Entity[%4d], Socket[%u]"),
				nCommand,
				pContext->GetEntity(),
				(unsigned int)pContext->GetSockHandle());
		}
		return false;
	}

	UINT uiSize = (UINT)protoBase.ByteSizeLong();

	SendPacket(pContext, nCommand, reinterpret_cast<BYTE*>(pGOOGLE_PROTO_BUFFER->SerializeBuffer), uiSize);

	return true;
}

bool cInstancePacketParser::CheckPrivateIP(NetLib::cInterfaceIocpContext* pContext, bool ExcuteDisConnect, char* CallerInfo)
{
	if (::CheckPrivateIP(pContext->GetIP()) == false)
	{
		char szSessionInfos[CSDef::EDef::MAX_BUFFER_128_LEN] = { 0, };
		NetLib::cSession* pSession = pContext->GetSession();
		if (pSession != nullptr)
		{
			pSession->CodedSessionInfo(szSessionInfos, sizeof(szSessionInfos));
		}
		else
		{
			uint32 uiIP = pContext->GetIP();
			inet_ntop(AF_INET, &uiIP, szSessionInfos, sizeof(szSessionInfos));
		}
		// 누가 요청했는지 IP와 Context Entity Session이 있다면 정보를 같이 남겨줍니다.
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "%s Invalid IP Entity [ %u ] %s",
			CallerInfo, pContext->GetEntity(), szSessionInfos);

		//	
		if (ExcuteDisConnect == true)
		{
			NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->PushCommand(static_cast<NetLib::cIocpContext*>(pContext), CSNet::ProtocolCommand::SYS_NET_DISCONNECT);
		}

		return false;
	}

	return true;
}


void cInstancePacketParser::SendUnDefinedPacket( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , UINT nThreadIndex )
{
	if ( pContext == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_GRADE::LOG_CRI ,
			"SendUnDefinedPacket: pContext is nullptr for command %u" , nCommand );
		return;
	}

	if ( !pContext->IsActive() )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
			LOG_GRADE::LOG_CRI ,
			"SendUnDefinedPacket: Context is not active for command %u" , nCommand );
		return;
	}

	// Create an empty protobuf message for UnDefined response
	PmNet::PktBase undefinedMessage;
	undefinedMessage.set_err_kind( General::ResultCode::Result_PacketNotSupported );
	undefinedMessage.set_payload_size( 0 );

	if ( pContext->GetSession() == nullptr )
		return;
	if ( pContext->GetSession()->GetSessionType() != Sessions::SESSION_CLIENT )
		return;
	// Send the UnDefined packet response
	bool sendResult = SendBuffer( pContext , nThreadIndex , nCommand , undefinedMessage ,
		General::ResultCode::Result_PacketNotSupported , "Unknown packet command" , e_thread_type::e_command_thread );

	if ( sendResult )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
				LOG_GRADE::LOG_CRI ,
			"UnDefined packet sent for unknown command %u" , nCommand );
	}
	else
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
				LOG_GRADE::LOG_CRI ,
			"Failed to send UnDefined packet for command %u" , nCommand );
	}
}
