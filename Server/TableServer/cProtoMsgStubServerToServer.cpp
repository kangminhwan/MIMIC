#include "cProtoMsgStub.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Queue/cCommandQueue.h"
#include "../Include/Netlib/Queue/cWebQueue.h"
#include "../Include/Netlib/IOCP/cIocpConnector.h"
#include "../Include/Netlib/Session/cSession.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Manager/ServerManager.h"
#include "../Include/Netlib/Manager/cCommandQueueManager.h"
#include "../Include/Netlib/Network/cContextPooler.h"
#include "../Include/Netlib/RestSdkHttp/restsdkHttp.h"
#include "../Include/Netlib/IOCP/cIocpConnector.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"


#include "cProtoUtil.h"
#include "cInstancePacketParser.h"
#include "Query.h"
#include "AuthCodeGenerator.h"
#include "TimeUtils.h"
#include "cGameRoom.h"
#include "cGameRoomManager.h"
#include "cClientSession.h"
#include "cVirtualSession.h"
#include "cDataLoader.h"
#include "UniqueKey.h"
#include "cQADecks.h"
#include "XorEncryption.h"
#include "cConfigReader.h"
#include "StringUtil.h"
#include "cLostLimitManager.h"
#include "MadePlatform.h"
#include "cLoungeEvent.h"
#include "cRedisController.h"
#include "cFriendManager.h"
#include "cOpsManager.h"
#include "cMaintenanceManager.h"

// Task 처리 테스트중
#include <iostream>
#include <future>
#include <functional>

// std::format
#include <format>

#include <google/protobuf/util/json_util.h>

// 호출되는 순간은 IOCP CONNECTOR 를 사용하는 서버에서 호출된다.
// 접속이 되고 나면 인증 메시지를 보내 접속된 서버에 등록을 요청한다.
void cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	uint32 nIP = pContext->GetIP();
	if ( CheckPrivateIP( nIP ) == false ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER Failed. Private IP Only [ %d.%d.%d.%d ]" , nIP & 255 , nIP >> 8 & 255 , nIP >> 16 & 255 , nIP >> 24 & 255 );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}

	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if ( pServerManager == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER Failed. ServerManager is nullptr" ); return;
	}

	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if ( pConfig == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER Failed. TServerConfiguration is nullptr" ); return;
	}

	char* strPublicIP = pServerManager->GetPublicIP();
	if ( strPublicIP == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER Failed. PublicIP is nullptr" ); return;
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER Success. IP [ %d.%d.%d.%d ]" , nIP & 255 , nIP >> 8 & 255 , nIP >> 16 & 255 , nIP >> 24 & 255 );

	// 접속을 한 서버에게 자신을 인증 해준다.
	Server::REQ_AUTHENTICATION _req_auth;

	_req_auth.set_server_connect_ip( strPublicIP );
	_req_auth.set_servergid( pConfig->GID_FOR_MANAGE );
	_req_auth.set_serverid( pConfig->SID_FOR_MANAGE );
	_req_auth.set_serverport( pConfig->wDefaultServerPort );
	_req_auth.set_server_type( pConfig->ServerType );
	_req_auth.set_server_connect_public_ip( pConfig->PublicIpAddres );

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
	pGOOGLE_PROTO_BUFFER->Clear();
	if ( _req_auth.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cInstancePacketParser::SYS_NET_CONNECT_TO_SERVER_FP() Failed. if (_req_auth.SerializeToArray(pGOOGLE_PROTO_BUFFER->SerializeBuffer, sizeof(pGOOGLE_PROTO_BUFFER->SerializeBuffer)) == false)" );
		return;
	}

	NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
		static_cast< NetLib::cIocpContext* >( pContext ) ,
		General::Packet_NodeAuthRequest , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _req_auth.ByteSizeLong() );

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( _req_auth , &serializedData );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , serializedData.c_str() );
}

// IOCP Connector 로 접속을 요청한 서버에서 보내는 메시지
void cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	uint32 nIP = pContext->GetIP();
	uint32 REMOTEPORT = pContext->GetPORT();

	if ( CheckPrivateIP( nIP ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Failed. Invalid Private IP[ %d.%d.%d.%d ]" , nIP & 255 , nIP >> 8 & 255 , nIP >> 16 & 255 , nIP >> 24 & 255 );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}

	Server::REQ_AUTHENTICATION _req_auth;
	if ( _req_auth.ParseFromArray( pData , nLength ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION ParseFromArray Failed." );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( _req_auth , &serializedData );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , serializedData.c_str() );

	TCHAR tzConnectIP[ CSDef::EDef::MAX_IP_ADDRESS_LEN ] = { 0, };
	ConvertIP( tzConnectIP , static_cast< int >( sizeof( TCHAR ) * CSDef::EDef::MAX_IP_ADDRESS_LEN ) , nIP );
	int64 allocatedslot = 0;
	ConnectorInfo* pConnectorInfo = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->RegisterServer( static_cast< BYTE >( _req_auth.server_type() ) , tzConnectIP , _req_auth.server_connect_public_ip(), _req_auth.serverport() , REMOTEPORT , pContext , _req_auth.servergid() , _req_auth.serverid() , allocatedslot );
	if ( pConnectorInfo == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			_T( "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Connector Register Failed. (from connectIP:%s, PORT:%d, remotePORT:%d, TYPE:%d)" ) ,
			tzConnectIP , _req_auth.serverport() , REMOTEPORT , _req_auth.server_type() );

		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}
	else
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			_T( "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Connector Register Success. (from connectIP:%s, PORT:%d, remotePORT:%d, TYPE:%d)" ) ,
			tzConnectIP , _req_auth.serverport() , REMOTEPORT , _req_auth.server_type() );
	}

	NetLib::cSession* pSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->AllocateSession( allocatedslot , pContext );
	if ( pSession != nullptr )
	{
		pContext->SetSession( pSession );
		//pContext->SetContextType( E_CONTEXT_TYPE::E_CONTEXT_SERVER );

		pSession->SetContext( pContext );
		pSession->SetConnectorInfo( pConnectorInfo );
		pSession->SetSessionType( Sessions::SESSION_SERVER );
		pSession->SetServerType( static_cast< E_SERVER_TYPE >( _req_auth.server_type() ) );
		pSession->SetServerID_And_ServerGroupID( _req_auth.servergid() , _req_auth.servergid() );
	}

	Server::RES_AUTHENTICATION _res_auth;
	_res_auth.set_resultcode( 1 );
	_res_auth.set_server_key( static_cast< google::protobuf::int64 >( allocatedslot ) );
	_res_auth.set_connectionservertype( static_cast< google::protobuf::int32 >( E_SERVER_TYPE::LOBBY_SERVER ) );

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
	// Null guard — GetProtobufBuffer returns nullptr when nThreadIndex is out of range
	// or the buffer pool is not initialized yet. Calling Clear() on null caused the
	// 2026-06-01 15:20:08 dump (access violation in VCRUNTIME140!memset).
	// Log the offending nThreadIndex so we can chase the root cause.
	if ( pGOOGLE_PROTO_BUFFER == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"SERVER_MSG_REQ_AUTHENTICATION: GetProtobufBuffer returned nullptr (nThreadIndex=%u)" ,
			nThreadIndex );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}
	pGOOGLE_PROTO_BUFFER->Clear();
	if ( _res_auth.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Failed." );
		return;
	}

	NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
		static_cast< NetLib::cIocpContext* >( pContext ) ,
		General::Packet_NodeAuthResponse , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _res_auth.ByteSizeLong() );

	TServerConfiguration* pConfig = NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->GetConfiguration();
	if ( pConfig == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Failed. ServerConfiguration is nullptr." );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}

	switch ( static_cast< E_SERVER_TYPE >( _req_auth.server_type() ) )
	{
	case E_SERVER_TYPE::LOBBY_SERVER:
	{
		// 손실제한 가지고 있는 모든 것들을 접속한 서버에게 내려준다.

		const int batch_size = 20; // 한 번에 전송할 개수
		int count = 0;

		auto lost_limits = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetServerLostLimits();
		
		Server::LostLimitSyncLobbyInitReq lost_limits_response;
		auto add_lost_limit = lost_limits_response.mutable_lost_limits();

		for ( auto iter = lost_limits->begin(); iter != lost_limits->end(); ++iter ) {

			General::LossLimitProfile* pLostLimit = iter->second;
			if ( pLostLimit == nullptr ) continue;

			
			auto adder = add_lost_limit->Add();
			adder->CopyFrom( *pLostLimit );
			++count;

			if ( count == batch_size ) {

				pGOOGLE_PROTO_BUFFER->Clear();
				if ( lost_limits_response.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Send GMsg_LostLimitSyncLobbyInit Failed." );
				}
				else {
					NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
					static_cast< NetLib::cIocpContext* >( pContext ) ,
					General::Packet_NodeLossCapBootstrap , pGOOGLE_PROTO_BUFFER->SerializeBuffer , lost_limits_response.ByteSizeLong() );
				}

				// 새로운 요청 객체 생성
				lost_limits_response.Clear();
				add_lost_limit = lost_limits_response.mutable_lost_limits();

				// 카운트 초기화
				count = 0;
			}
		}

		// 남은 데이터를 전송
		if ( count > 0 ) {
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( lost_limits_response.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Send GMsg_LostLimitSyncLobbyInit Failed." );
			}
			else {
				NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
				static_cast< NetLib::cIocpContext* >( pContext ) ,
				General::Packet_NodeLossCapBootstrap , pGOOGLE_PROTO_BUFFER->SerializeBuffer , lost_limits_response.ByteSizeLong() );
			}
		}
		
	}
	break;

	{

	}
	break;
	default:
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Failed. Invalid Connection ServerType [ %d ]" , _req_auth.server_type() );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}
	
	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION_FP() Success. [ResultCode %d] [ServerMapKey : %d] [ServerType : %d]" , static_cast< int >( _res_auth.resultcode() ) , _res_auth.server_key() , _res_auth.connectionservertype() );
}

void cProtoMsgStub::SERVER_MSG_RES_AUTHENTICATION( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	uint32 nIP = pContext->GetIP();

	if ( CheckPrivateIP( nIP ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_RES_AUTHENTICATION Failed. Invalid Private IP[ %d.%d.%d.%d ]" , nIP & 255 , nIP >> 8 & 255 , nIP >> 16 & 255 , nIP >> 24 & 255 );
		return;
	}

	Server::RES_AUTHENTICATION _res_auth;
	if ( _res_auth.ParseFromArray( pData , nLength ) == false )
	{
		return;
	}

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( _res_auth , &serializedData );
	TraceA( "cProtoMsgStub::SERVER_MSG_RES_AUTHENTICATION : " + serializedData );

	switch ( static_cast< E_SERVER_TYPE >( _res_auth.connectionservertype() ) )
	{
	case E_SERVER_TYPE::LOBBY_SERVER:
	{
	}
	break;

	{
	}
	break;
	default:
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_RES_AUTHENTICATION Failed. Invalid Connection Server Type [ %hd ]" , _res_auth.connectionservertype() );
		return;
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::SERVER_MSG_RES_AUTHENTICATION Success. [ResultCode %d] [ServerMapKey : %I64d] [ServerType : %d]" , static_cast< int >( _res_auth.resultcode() ) , _res_auth.server_key() , _res_auth.connectionservertype() );
}

void cProtoMsgStub::RoomListSync( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::RoomListSync _room_list_sync;
	if ( _room_list_sync.ParseFromArray( pData , nLength ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::RoomListSync() ParseFromArray Failed." );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( _room_list_sync , &serializedData );
	TraceA( "cProtoMsgStub::RoomListSync : " + serializedData );

	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::RoomListSync() Receive Success." );
}

// 다른 로비서버에서 채널 목록 요청
void cProtoMsgStub::RoomListToLobby( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::RoomListToLobbyReq server_request;
	Server::RoomListToLobbyRes server_response;
	if ( !server_request.ParseFromArray( pData , nLength ) ) {
		//SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , server_response );
		return;
	}

	// 전송자 정보
	server_response.set_player_idx( server_request.player_idx() );

#pragma region cProtoMsgStub::RoomList 와 동일
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	const PmNet::ChamberIndexRQ& request = server_request.request();
	PmNet::ChamberIndexRS response;
	response.set_ctx_buf( request.ctx_buf() );

	std::map<std::string , int> channel_ids;
	for ( auto channel : request.ch_tokens() ) {
		channel_ids.insert( std::pair<std::string , int>( channel , 0 ) );
		auto channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( channel );

		// 관리하는 채널이 아니면 위임을 보냄
		if ( channelData.server_id() != configReader->SID_FOR_MANAGE ) {
			return;
		}
		// channel id 가 잘못된 경우라면 처리 불가
		if ( channel.size() == 0 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}

	int pagingSize = request.page_sz() > 0 ? request.page_sz() : 10; // default size 10 으로 셋팅
	int startIndex = request.start_pos();
	General::PlayCategory gameType = request.match_kind();


	int totalCount;
	int totalPage;
	google::protobuf::RepeatedField<General::RoomListEntry> roomList =
		NetLib::cSingleton<cGameRoomManager>::GetInstance()->GetRoomList( gameType , channel_ids , totalCount , totalPage , nThreadIndex , startIndex , pagingSize );

	response.set_match_kind( gameType );
	for ( auto channel : channel_ids ) {
		response.add_ch_tokens( channel.first );
	}
	response.set_page_sz( pagingSize );
	response.set_start_pos( startIndex );
	response.set_total_cnt( totalCount );
	for ( auto roomListInfo : roomList ) {
		auto roomListInfoPtr = response.add_chamber_list();
		roomListInfoPtr->CopyFrom( roomListInfo );
	}

#pragma endregion cProtoMsgStub::RoomList 와 동일

	auto add_response = server_response.mutable_response();
	add_response->CopyFrom( response );

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
	pGOOGLE_PROTO_BUFFER->Clear();
	if ( server_response.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::RoomListToLobby Failed." );
		return;
	}

	E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeSpaceQueryResult , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_response.ByteSizeLong() );

	if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	/*NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
		static_cast< NetLib::cIocpContext* >( pContext ) ,
		General::Packet_NodeSpaceQueryResult , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_response.ByteSizeLong() );*/
}

// 다른 로비서버에서 받은 데이터를 플레이어 에게 전달
void cProtoMsgStub::RoomListToLobbyResponse( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::RoomListToLobbyRes server_response;
	if ( !server_response.ParseFromArray( pData , nLength ) ) {
		return;
	}

	// 플레이어 세션 검색
	const uint64& playerIdx = server_response.player_idx();

	cClientSession* pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( playerIdx ) );
	if ( pClientSession == nullptr )
		return;

	NetLib::cIocpContext* pClientContext = reinterpret_cast< NetLib::cIocpContext* >( pClientSession->GetContext() );
	if ( pClientContext == nullptr )
	{
		//TODO context 가 없습니다. 전송할 곳이 없습니다.
		return;
	}

	PmNet::ChamberIndexRS response = server_response.response();
	//SendMessageAndLogWrite( General::ResultCode::Result_Success , pClientContext , nThreadIndex , General::PacketID::Packet_SpaceList , response );
	std::string errorString;
	GetOwner()->SendBuffer( pClientContext , nThreadIndex , General::PacketID::Packet_SpaceList , response , General::ResultCode::Result_Success , errorString );
}

// 다른 서버에서 LostLimit 데이터를 LostLimit Manager 에 Set 한다.
// 생성 시간을 비교해서, 먼저 생성된 것이 우선한다.
void cProtoMsgStub::LostLimitSync( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::LostLimitSyncReq server_request;
	if ( !server_request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	const General::LossLimitProfile& receive_lost_limit = server_request.lost_limit();
	const uint64& account_idx = receive_lost_limit.account_id();

	// 아직 캐슁된 데이터가 없다. 캐슁 처리
	auto current_lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( account_idx );
	if ( current_lostLimit == nullptr ) {
		// 바로 Set 처리
		NetLib::cSingleton<cLostLimitManager>::GetInstance()->SyncLostLimit( receive_lost_limit );
	}
	else {

		// 로컬의 생성시간이 더 큰 경우라면 받은 데이터가 더 오래된 데이터 이다.
		// 같을 때도 갱신 해버린다.
		if ( current_lostLimit->created_at() >= receive_lost_limit.created_at() )
			NetLib::cSingleton<cLostLimitManager>::GetInstance()->SyncLostLimit( receive_lost_limit );
	}
}

void cProtoMsgStub::LostLimitSyncLobbyInit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::LostLimitSyncLobbyInitReq server_request;
	if ( !server_request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	for ( const auto& receive_lost_limit : server_request.lost_limits() ) {

		//const General::LossLimitProfile& receive_lost_limit = server_request.lost_limit();
		const uint64& account_idx = receive_lost_limit.account_id();

		// 아직 캐슁된 데이터가 없다. 캐슁 처리
		auto current_lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( account_idx );
		if ( current_lostLimit == nullptr ) {
			// 바로 Set 처리
			NetLib::cSingleton<cLostLimitManager>::GetInstance()->SyncLostLimit( receive_lost_limit );
		}
		else {

			// 로컬의 생성시간이 더 큰 경우라면 받은 데이터가 더 오래된 데이터 이다.
			if ( current_lostLimit->created_at() >= receive_lost_limit.created_at() )
				NetLib::cSingleton<cLostLimitManager>::GetInstance()->SyncLostLimit( receive_lost_limit );
		}
	}
}

void cProtoMsgStub::LostLimitUpdate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::LostLimitUpdateReq server_request;
	if ( !server_request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	const uint64& account_idx = server_request.account_idx();

	// 아직 캐슁된 데이터가 없다. 캐슁 처리
	auto current_lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( account_idx );
	if ( current_lostLimit == nullptr ) {
		// No cached entry yet. A full-snapshot update (update_lost_limit) carries the
		// complete profile, so cache it directly. Delta-only messages (add_lost_chip /
		// add_price) have no base value to apply against, so they are dropped here.
		if ( server_request.update_lost_limit() ) {
			NetLib::cSingleton<cLostLimitManager>::GetInstance()->SyncLostLimit( server_request.lost_limits() );
		}
		return;
	}
	else {

		if ( server_request.add_lost_chip() != 0 ) {

			int64 daily_lost_chip = current_lostLimit->daily_chip_loss();
			daily_lost_chip += server_request.add_lost_chip();

			// 손실금액 가감 처리
			current_lostLimit->set_daily_chip_loss( daily_lost_chip );
		}


if ( server_request.add_price() != 0 ) {

			int64 current_buy = current_lostLimit->monthly_purchase_total();
			current_buy += server_request.add_price();

			// 구매금액 누적
			current_lostLimit->set_monthly_purchase_total( current_buy );
		}
		
	}

	// 바로 Set 처리
	if ( server_request.update_lost_limit() ) {

		NetLib::cSingleton<cLostLimitManager>::GetInstance()->SyncLostLimit( server_request.lost_limits() );
	}

	if ( server_request.limit_out() )
	{
		if ( server_request.refresh_time_of_lost_limit() != "" )
		{
			current_lostLimit->set_loss_limit_reset_at( server_request.refresh_time_of_lost_limit() );
		}
		int64 set_chip = current_lostLimit->loss_limit_amount();
		if ( current_lostLimit->daily_chip_loss() > set_chip )
		{
			NetLib::cSingleton<cMaintenanceManager>::GetInstance()->PushbackLostLimit( server_request.account_guid() );
		}
	}

}

void cProtoMsgStub::SyncFriendInfo( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::MateDetail friendInfo;
	if ( !friendInfo.ParseFromArray( pData , nLength ) ) {
		return;
	}

	NetLib::cSingleton<cFriendManager>::GetInstance()->SetFriendInfo( friendInfo );

#ifdef _DEBUG

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( friendInfo , &serializedData );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , serializedData.c_str() );
#endif
}

void cProtoMsgStub::SyncFriendInfoStatus( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::SyncFriendInfoStatus friendInfoStatus;
	if ( !friendInfoStatus.ParseFromArray( pData , nLength ) ) {
		return;
	}

	NetLib::cSingleton<cFriendManager>::GetInstance()->SyncFriendInfoStatus( friendInfoStatus );

#ifdef _DEBUG

	//std::string serializedData;
	//google::protobuf::util::MessageToJsonString( friendInfoStatus , &serializedData );
	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , serializedData.c_str() );

#endif
}

void cProtoMsgStub::OpsPlayerKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::OpsPlayerKickReq request;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	E_DB_TYPE db_type = static_cast< E_DB_TYPE >( request.db_type() );

	const uint64& playerIdx = request.kick_player_idx();
	const std::string& query = request.query();

	// 정상 세션
	auto player = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( playerIdx ) );
	if ( player != nullptr ) {

		auto context = player->GetContext();

		// TODO 운영툴로 Kick
	// 상황에 따라 즉시 쿼리를 실행하는 경우와
	// 게임 종료뒤에 쿼리를 실행하는 경우로 나뉘어 져야 한다.
		std::u8string operation_message = u8"서비스 이용이 제한되었습니다.";
		std::string_view operation_message_utf8View( reinterpret_cast< const char* >( operation_message.data() ) , operation_message.size() );

		PmNet::ServiceNotice system_message;
		system_message.set_notice( operation_message_utf8View.data() );
		system_message.set_expel_flag( true );

		// 시스템 메시지 발송
		if ( context != nullptr ) {

			std::string errorString;
			GetOwner()->SendBuffer( context , nThreadIndex , General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_Success , errorString );
		}

		player->SessionLogout( pContext->GetEntity() );
		player->DisConnectContext( pContext->GetEntity() , nThreadIndex );

		// 쿼리 즉시 실행
		if ( query.size() != 0 ) {

			auto result = QueryManager::UpdateExecuteQueryAsync( db_type , query );
			result.wait();
		}

		if ( context != nullptr ) {

			context->Disconnect();
		}
		return;
	}
	// 비정상 세션
	else {

		//// 펜딩 세션에 남아 있는지 체크하고
		//// 이경우에는 플레이 종료뒤에 쿼리가 실행되도록 해주어야 한다.
		//auto session = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPendingSession( playerIdx );
		//if ( session != nullptr ) {

		//	// 쿼리 등록
		//	if ( query.size() != 0 ) {


		//	}

		//	return;
		//}
	}

	// 1번 로비서버에서 처리를 못했음으로 2번서버로 의뢰를 한다.
	request.set_ops_code( NetLib::cSingleton<AuthCodeGenerator>::GetInstance()->generate( 16 ) );

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
	pGOOGLE_PROTO_BUFFER->Clear();
	if ( request.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::OpsPlayerKick Send Failed." );
	}
	else {
		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->BroadCastToConnectors(
			E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeUserRemoveCommand , pGOOGLE_PROTO_BUFFER->SerializeBuffer , request.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

			// 로비서버 전송 실패
		}
	}

	// 오퍼레이션을 등록한다.
	NetLib::cSingleton<cOpsManager>::GetInstance()->PushOperation( request.ops_code() , request );

	// 슬롯서버에도 플레이어의 Kick을 명령한다.

}

void cProtoMsgStub::SyncOpsPlayerKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::OpsPlayerKickReq request;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	Server::OpsPlayerKickRes response;
	response.set_ops_code( request.ops_code() );

	E_DB_TYPE db_type = static_cast< E_DB_TYPE >( request.db_type() );

	const uint64& playerIdx = request.kick_player_idx();
	const std::string& query = request.query();

	// 정상 세션
	auto player = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( playerIdx ) );
	if ( player != nullptr ) {

		auto context = player->GetContext();

		// TODO 운영툴로 Kick
		// 상황에 따라 즉시 쿼리를 실행하는 경우와
		// 게임 종료뒤에 쿼리를 실행하는 경우로 나뉘어 져야 한다.
		std::u8string operation_message = u8"서비스 이용이 제한되었습니다.";
		std::string_view operation_message_utf8View( reinterpret_cast< const char* >( operation_message.data() ) , operation_message.size() );

		PmNet::ServiceNotice system_message;
		system_message.set_notice( operation_message_utf8View.data() );
		system_message.set_expel_flag( true );

		// 시스템 메시지 발송
		if ( context != nullptr ) {

			std::string errorString;
			GetOwner()->SendBuffer( context , nThreadIndex , General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_Success , errorString );
		}

		player->SessionLogout( 0 );

		// 쿼리 즉시 실행
		if ( query.size() != 0 ) {

			auto result = QueryManager::UpdateExecuteQueryAsync( db_type , query );
			result.wait();
		}

		if ( context != nullptr ) {

			context->Disconnect();
		}

		// 정상 처리로 등록
		response.set_reply_type( Server::ReplyType::reply_type_executed );
	}
	// 비정상 세션
	else {

		// 펜딩 세션에 남아 있는지 체크하고
		// 이경우에는 플레이 종료뒤에 쿼리가 실행되도록 해주어야 한다.
		auto session = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPendingSession( playerIdx );
		if ( session != nullptr ) {

			// 쿼리 등록
			if ( query.size() != 0 ) {


			}

			// 정상 처리로 등록
			response.set_reply_type( Server::ReplyType::reply_type_executed );
		}
		else
		{
			// 처리 못함
			response.set_reply_type( Server::ReplyType::reply_type_not_found_player );
		}
	}

	//std::string errorString;
	//GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_NodeUserRemoveResult , response , General::ResultCode::Result_Success , errorString );

//	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
//	pGOOGLE_PROTO_BUFFER->Clear();
//	if ( response.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::SyncOpsPlayerKick Send Failed." );
//	}
//	else {
//		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->BroadCastToConnectors(
//			E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeUserRemoveResult , pGOOGLE_PROTO_BUFFER->SerializeBuffer , response.ByteSizeLong() );
//
//		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {
//
//			// 로비서버 전송 실패
//		}
//	}
}

// 다른 서버로 처리를 요청한 부분에 대한 응답을 받음
// 응답이 만약에 찾지 못했음이면 쿼리를 실행하고 끝낸다.
void cProtoMsgStub::SyncOpsPlayerKickResponse( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::OpsPlayerKickRes response;
	if ( !response.ParseFromArray( pData , nLength ) ) {
		return;
	}

	if ( response.ops_code().size() == 0 )
		return;

	Server::OpsPlayerKickReq ops_operation = NetLib::cSingleton<cOpsManager>::GetInstance()->GetOperation( response.ops_code() );

	// 다른 로비서버에서도 플레이어를 찾지 못했다.
	if ( response.reply_type() == Server::ReplyType::reply_type_not_found_player ) {

		// 쿼리를 실행한다.
		if ( ops_operation.query().size() != 0 ) {       

			E_DB_TYPE db_type = static_cast< E_DB_TYPE >( ops_operation.db_type() );
			auto result = QueryManager::UpdateExecuteQueryAsync( db_type , ops_operation.query() );
			result.wait();
		}
	}

	NetLib::cSingleton<cOpsManager>::GetInstance()->RemoveOperation( response.ops_code() );
}

void cProtoMsgStub::OpsMarketKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::OpsMarketKickReq request;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	// 마켓에 해당하는 모든 플레이어들을 Kick 처리

}


void cProtoMsgStub::SyncOpsMarketKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::OpsMarketKickReq request;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}
}

void cProtoMsgStub::SyncPlayerInfo( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::SyncPlayerInfo _request;
	PmNet::AlignMemberDetailRS res;
	if ( _request.ParseFromArray( pData , nLength ) == false )
	{
		return;
	}

	const uint64& playerIdx = _request.player_idx();
	NetLib::cSession* connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( playerIdx ) );
	if(connectedClientSession == nullptr )
		connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPending( static_cast< int64 >( playerIdx ) );
		
	if ( connectedClientSession != nullptr ) {
		if ( cClientSession* pClientSession = dynamic_cast< cClientSession* >( connectedClientSession ) )
		{
			General::ParticipantProfile& _player = pClientSession->GetPlayerRef();
			Server::ParticipantProfileInternal& _playerExt = pClientSession->GetPlayerExtRef();
			std::future<BOOL> result = QueryManager::GetPlayerByPlayerIdxAsync( playerIdx , _player , _playerExt );
				
			result.wait();

			if ( false == result.get() )
			{
				return;
			}
			General::ParticipantProfile* responsePlayer = res.mutable_member_info();
			responsePlayer->CopyFrom( _player );

			GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_ProfileNotice , res , General::ResultCode::Result_Success , "" );
			//PmNet::Ledger _records;
			//int _reads = 0;

			//const uint64 _player_idx = _player.member_id();

			////NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "9999999999999999999999999999" );
			////Sleep( 100 );

			//result = std::async( [_player_idx , &_reads , &_records]() {
			//	return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
			//} );

			//result.wait();

			//if ( FALSE == result.get() ) {

			//}
		}
	}

	/*if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}*/
}


void cProtoMsgStub::SyncGameVersionUpdate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	NetLib::cSingleton<cInstancePacketParser>::GetInstance()->InitializeGameVersionChecker();
}
void cProtoMsgStub::SyncPlayerLoginNoti( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::SyncPlayerLoginNoti _request;
	if ( _request.ParseFromArray( pData , nLength ) == false )
	{
		return;
	}

	const uint64& playerIdx = _request.player_idx();

	// 중복 접속 확인 처리
	auto connectedClientSession = static_cast< cClientSession* >(NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( playerIdx ) ));
	if ( connectedClientSession != nullptr ) {

		// Context 가 있으면 디스커넥트 처리
		if ( connectedClientSession->GetContext() != nullptr && connectedClientSession->GetContext()->GetSession()!= nullptr ) {
			if ( connectedClientSession->GetContext()->GetSession()->GetSessionType() != Sessions::SESSION_CLIENT )
				return;
			connectedClientSession->SetRoomOutReason( "" );
			const uint64& roomNumber = connectedClientSession->GetJoinedRoomNumber();
			const uint64& playerIdx = connectedClientSession->GetPlayerIdx();

			// 접속을 끊어낸다.
			PmNet::DupTunnelRS duplicatedSessionRes;
			try {
				if ( auto session = dynamic_cast< cClientSession* >( connectedClientSession->GetContext()->GetSession() ) )
				{
					uint64 t_playeridx = session->GetPlayerIdx();
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SyncPlayerLoginNoti. PlayerIdx [ %llu ], contextIndex [ %llu ] " , playerIdx , t_playeridx );
				}
				else
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_CRI , _T( "!!!!!!!!!!!!!!!!!!!castingfail!!!!!!!!!!!!!!!!!!" ) );
			}
			catch ( const std::exception& e )
			{
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_CRI , _T( "SyncPlayerLoginNoti Standard exception: %s" ) , e.what() );
			}
			
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
			NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( playerIdx );
			//connectedClientSession->DisConnectContext( connectedClientSession->GetContext()->GetEntity() , 0 );
			// 세션 삭제 및 풀러로 이동
			//NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->RemoveOnly( playerIdx );

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::Login Duplicated Session roomNumber == 0 Success. PlayerIdx [ %llu ]" , playerIdx );
			// 기존 Session 과 Context 의 연결고리를 끊습니다.
			//connectedClientSession->GetContext()->SetSession( nullptr );
			//connectedClientSession->SetContext( nullptr );
		}
	}
}

void cProtoMsgStub::OpsUpdateLostLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	Server::OpsLostLimitReq request;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	// Account 정보 부터 가져 온다.
	std::future<General::LossLimitProfile> lostLimitResult = QueryManager::GetLostLimitAsync( request.account_guid() );
	lostLimitResult.wait();

	General::LossLimitProfile _lost_limit = lostLimitResult.get();
	if ( _lost_limit.account_id() == 0 ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "OpsUpdateLostLimit Fail account"); return;
	}

	auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( _lost_limit.account_id() );
	if ( lostLimit == nullptr )
		return;
	lostLimit->CopyFrom( _lost_limit );

	// 손실한도 로비 서버간 동기화 처리
	Server::LostLimitSyncReq _lost_limit_sync;
	_lost_limit_sync.mutable_lost_limit()->CopyFrom( _lost_limit );

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
	pGOOGLE_PROTO_BUFFER->Clear();
	if ( _lost_limit_sync.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Server::LostLimitSyncReq SerializeToArray Failed." );
		//return;
	}
	else {

		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_sync.ByteSizeLong() );

		// 서버간 동기화 실패
		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

		}
	}

	Server::LostLimitUpdateReq _lost_limit_update;
	_lost_limit_update.set_update_lost_limit( true );
	auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
	add_lost_limit->CopyFrom( _lost_limit );

	GOOGLE_PROTOBUF_BUFFER* tGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
	tGOOGLE_PROTO_BUFFER->Clear();
	if ( _lost_limit_update.SerializeToArray( tGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( tGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::ChangeLostLimit Send GMsg_LostLimitUpdate Failed." );
	}

}
