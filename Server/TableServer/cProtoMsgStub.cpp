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
#include "../Include/Netlib/Redis/cRedisManager.h"
#include "../Include/Netlib/Thread/cThreadPool.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include <sstream>

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
#include "cMoneyLogInstance.h"
#include "cAssetLog.h"
#include "cAdsLog.h"
#include "cSocialLog.h"
#include "cMessageLog.h"
#include "cGameVersionChecker.h"
#include "cMaintenanceManager.h"
#include "TrieNode.h"
#include "IpTimeQueue.h"
#include "cLoginEventManager.h"

// Task 처리 테스트중
#include <iostream>
#include <future>
#include <functional>
#include <tuple>
// std::format
#include <format>

#include <google/protobuf/util/json_util.h>
//#include "cPinballLogInstance.h"

cProtoMsgStub::cProtoMsgStub()
{
}


cProtoMsgStub::~cProtoMsgStub()
{
}

void cProtoMsgStub::SendMessageAndLogWrite( const General::ResultCode& errorCode , NetLib::cInterfaceIocpContext* pContext , UINT nThreadIndex , General::PacketID messageId , google::protobuf::Message& _message )
{
	std::string errorString;

	if ( errorCode != General::ResultCode::Result_Success ) {
		std::string enumString = protoutil::cProtoUtil::GetEnumString( errorCode );
		std::string messageIdString = protoutil::cProtoUtil::GetEnumString( messageId );
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _message , &serializedData );
		errorString = std::format( "[ {} ] failed. [ {} ] errorcode. return Msg = {}" , messageIdString.c_str() , enumString.c_str() , serializedData.c_str() );
		// 로그큐 크래시 방지를 위해 특수문자 제거
		std::string safeErrorString = StringUtil::RemoveSpecialCharactersAndSpaces( errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , safeErrorString.c_str() );
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , static_cast< UINT >( messageId ) , _message , errorCode , errorString );
}

void cProtoMsgStub::bindmethod( std::function<void( NetLib::cInterfaceIocpContext* /*pContext*/ , UINT /*nCommand*/ , BYTE* /*pData*/ , UINT /*nLength*/ , UINT /*nThreadIndex*/ )>* fp )
{
	if ( fp == nullptr )
	{
		return;
	}

	if ( fp[ CSNet::ProtocolCommand::SYS_NET_CONNECT ] != nullptr )
	{
		assert( false && "cScheduleStub::bindmethod" );
	}
	if ( fp[ CSNet::ProtocolCommand::SYS_NET_DISCONNECT ] != nullptr )
	{
		assert( false && "cScheduleStub::bindmethod" );
	}
	if ( fp[ CSNet::ProtocolCommand::SYS_NET_WEB_LOGIN_REQUEST_SUCCESS ] != nullptr )
	{
		assert( false && "cScheduleStub::bindmethod" );
	}
	if ( fp[ CSNet::ProtocolCommand::SYS_NET_CHANGE_COMMAND_INDEX_CONTEXT ] != nullptr )
	{
		assert( false && "cScheduleStub::bindmethod" );
	}
	if ( fp[ General::SysPacket_Heartbeat ] != nullptr )
	{
		assert( false && "cScheduleStub::bindmethod" );
	}

	fp[ CSNet::ProtocolCommand::SYS_NET_CONNECT ] = std::bind( &cProtoMsgStub::SYS_NET_CONNECT , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ CSNet::ProtocolCommand::SYS_NET_DISCONNECT ] = std::bind( &cProtoMsgStub::SYS_NET_DISCONNECT , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ General::PacketID::Packet_LabDisconnect ] = std::bind( &cProtoMsgStub::CHEAT_DISCONNECT , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ CSNet::ProtocolCommand::SYS_NET_CONNECT_TO_SERVER ] = std::bind( &cProtoMsgStub::SYS_NET_CONNECT_TO_SERVER , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeAuthRequest ] = std::bind( &cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeAuthResponse ] = std::bind( &cProtoMsgStub::SERVER_MSG_RES_AUTHENTICATION , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeSpaceSync ] = std::bind( &cProtoMsgStub::RoomListSync , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeSpaceQuery ] = std::bind( &cProtoMsgStub::RoomListToLobby , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeSpaceQueryResult ] = std::bind( &cProtoMsgStub::RoomListToLobbyResponse , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeLossCapSync ] = std::bind( &cProtoMsgStub::LostLimitSync , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeLossCapBootstrap ] = std::bind( &cProtoMsgStub::LostLimitSyncLobbyInit , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeLossCapUpdate ] = std::bind( &cProtoMsgStub::LostLimitUpdate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeContactSync ] = std::bind( &cProtoMsgStub::SyncFriendInfo , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeContactStateSync ] = std::bind( &cProtoMsgStub::SyncFriendInfoStatus , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeUserRemoveCommand ] = std::bind( &cProtoMsgStub::SyncOpsPlayerKick , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeMarketRemoveCommand ] = std::bind( &cProtoMsgStub::SyncOpsMarketKick , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeProfileSync ] = std::bind( &cProtoMsgStub::SyncPlayerInfo , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeAccessNotice ] = std::bind( &cProtoMsgStub::SyncPlayerLoginNoti , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeRevisionNotice ] = std::bind( &cProtoMsgStub::SyncGameVersionUpdate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 운영툴
	fp[ General::PacketID::Packet_AdminUserRemove ] = std::bind( &cProtoMsgStub::OpsPlayerKick , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_AdminMarketRemove ] = std::bind( &cProtoMsgStub::OpsMarketKick , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_AdminLossCapUpdate ] = std::bind( &cProtoMsgStub::OpsUpdateLostLimit , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ General::PacketID::Packet_LinkProbe ] = std::bind( &cProtoMsgStub::Ping , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LinkEcho ] = std::bind( &cProtoMsgStub::PingClient , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeHeartbeat ] = std::bind( &cProtoMsgStub::Ping , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_IdentityEnroll ] = std::bind( &cProtoMsgStub::CreateAccount , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ChannelBind ] = std::bind( &cProtoMsgStub::CreatePlatform , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ChannelLookup ] = std::bind( &cProtoMsgStub::GetCreatedPlatformsByCid , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ChannelPrecheck ] = std::bind( &cProtoMsgStub::GetCreatedPlatformsByCidForCreate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_DirectEnroll ] = std::bind( &cProtoMsgStub::CreateMADEPlatform , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ General::PacketID::Packet_AccessOpen ] = std::bind( &cProtoMsgStub::Login , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_AccessResume ] = std::bind( &cProtoMsgStub::ReLogin , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NodeTransfer ] = std::bind( &cProtoMsgStub::TransferServer , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LocalSecretUpdate ] = std::bind( &cProtoMsgStub::MADEPassChange , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SecondKeyReset ] = std::bind( &cProtoMsgStub::ResetSubPassword , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_TermsAccept ] = std::bind( &cProtoMsgStub::UpdateTermsAgree , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_CredentialExtend ] = std::bind( &cProtoMsgStub::UpdateCiExpiryTime , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ProfileClose ] = std::bind( &cProtoMsgStub::WithdrawalGame , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ProfileCloseCancel ] = std::bind( &cProtoMsgStub::CancelWithdrawalGame , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_AliasUpdate ] = std::bind( &cProtoMsgStub::NickChange , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_NotifyOptionUpdate ] = std::bind( &cProtoMsgStub::UpdatePush , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ General::PacketID::Packet_AttendanceClaim ] = std::bind( &cProtoMsgStub::CheckLoginReward , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleProfileLoad ] = std::bind( &cProtoMsgStub::VIPLoadUser , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleProfileSave ] = std::bind( &cProtoMsgStub::VIPSaveUser , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleInboxState ] = std::bind( &cProtoMsgStub::ChangeMailState , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleDailyExpiry ] = std::bind( &cProtoMsgStub::ChangeDailyExpired , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );


	// 테이블
	fp[ General::PacketID::Packet_SpaceCreate ] = std::bind( &cProtoMsgStub::RoomCreate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_PrivateSpaceCreate ] = std::bind( &cProtoMsgStub::FriendRoomCreate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SpaceList ] = std::bind( &cProtoMsgStub::RoomList , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SpaceEnter ] = std::bind( &cProtoMsgStub::RoomJoin , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SpaceMove ] = std::bind( &cProtoMsgStub::MoveRoom , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SpaceLeave ] = std::bind( &cProtoMsgStub::RoomOut , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LeaveReserve ] = std::bind( &cProtoMsgStub::RoomOutReserve , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_RoundOpen ] = std::bind( &cProtoMsgStub::PlayStart , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_StakeSubmit ] = std::bind( &cProtoMsgStub::PlayBet , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	//fp[Common::GMsgID::GMsg_RoomTryVote] = std::bind( &cProtoMsgStub::RoomTryVote , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5  );
	//fp[Common::GMsgID::GMsg_RoomVoteAnswer] = std::bind( &cProtoMsgStub::RoomVoteAnswer , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5  );
	fp[ General::PacketID::Packet_RoundJoin ] = std::bind( &cProtoMsgStub::GameParticipate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_RoundJoinCancel ] = std::bind( &cProtoMsgStub::CancelGameParticipate , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_StakeCancel ] = std::bind( &cProtoMsgStub::CancelBet , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_WatcherModeEnter ] = std::bind( &cProtoMsgStub::TrasferToWatcher , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_WatcherModeCancel ] = std::bind( &cProtoMsgStub::CancelTrasferToWatcher , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_WatcherEnter ] = std::bind( &cProtoMsgStub::RoomJoinAsWatcher , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SeatSlotMove ] = std::bind( &cProtoMsgStub::MoveSlot , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_RabbitHunt ] = std::bind( &cProtoMsgStub::RabbitHunt , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ShowHand ] = std::bind( &cProtoMsgStub::ShowHand , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ResultViewDone ] = std::bind( &cProtoMsgStub::ResultComplete , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ General::PacketID::Packet_EmoteSend ] = std::bind( &cProtoMsgStub::SendEmoticon , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_UserRemove ] = std::bind( &cProtoMsgStub::KickOutPlayer , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_UserRemoveCancel ] = std::bind( &cProtoMsgStub::CancelKickOutPlayer , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 핀볼
	//fp[ Common::GMsgID::GMsg_EnterPinball ] = std::bind( &cProtoMsgStub::EnterPinball , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	//fp[ Common::GMsgID::GMsg_PinballAck ] = std::bind( &cProtoMsgStub::PinballAck , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	//fp[ Common::GMsgID::GMsg_BuyPinball ] = std::bind( &cProtoMsgStub::BuyPinball , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	//fp[ Common::GMsgID::GMsg_LeavePinball ] = std::bind( &cProtoMsgStub::LeavePinball , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );


	// 슬롯
	/*fp[General::PacketID::Packet_ReelRoll ] = std::bind( &cProtoMsgStub::Spin , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[General::PacketID::Packet_ReelOpen ] = std::bind( &cProtoMsgStub::EnterSlotGame , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[General::PacketID::Packet_RollAck ] = std::bind( &cProtoMsgStub::SpinAck , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[General::PacketID::Packet_ReelClose ] = std::bind( &cProtoMsgStub::LeaveSlotGame , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[Common::GMsgID::GMsg_ChangeTotalBet ] = std::bind( &cProtoMsgStub::ChangeTotalBet , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[Common::GMsgID::GMsg_MakeAckRemainSpins ] = std::bind( &cProtoMsgStub::MakeAckRemainSpins , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[Common::GMsgID::GMsg_BuyGrandSpinPopup ] = std::bind( &cProtoMsgStub::BuyGrandSpinPopup , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[General::PacketID::Packet_PlayCreditRefresh ] = std::bind( &cProtoMsgStub::GetVirtualCoin , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );*/

	// 이벤트 관려

	// 플레이어 데이터 갱신
	fp[ General::PacketID::Packet_AvatarEquip ] = std::bind( &cProtoMsgStub::PlayerSetAvatar , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SecondKeySet ] = std::bind( &cProtoMsgStub::PlayerSetSubPasswd , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SecondKeyChange ] = std::bind( &cProtoMsgStub::PlayerChangeSubPasswd , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_SecondKeyClear ] = std::bind( &cProtoMsgStub::PlayerRemoveSubPasswd , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// Lobby 데이터 갱신
	fp[ General::PacketID::Packet_LobbyRefresh ] = std::bind( &cProtoMsgStub::UpdateLobby , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	fp[ General::PacketID::Packet_DailyReset ] = std::bind( &cProtoMsgStub::DailyRefresh , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 우편함
	fp[ General::PacketID::Packet_InboxList ] = std::bind( &cProtoMsgStub::MailBox , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_InboxOpen ] = std::bind( &cProtoMsgStub::MailOpen , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_BoardNotice ] = std::bind( &cProtoMsgStub::NoticeMessage , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 미션, 업적
	fp[ General::PacketID::Packet_TaskList ] = std::bind( &cProtoMsgStub::GetQuests , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_TaskRewardClaim ] = std::bind( &cProtoMsgStub::GetQuestReward , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 무료 충전소
	fp[ General::PacketID::Packet_FreeCreditClaim ] = std::bind( &cProtoMsgStub::GetFreeCharge , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 적립 통장
	fp[ General::PacketID::Packet_PointWithdraw ] = std::bind( &cProtoMsgStub::WithdrawRakeback , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 상점 구매
	fp[ General::PacketID::Packet_StorePurchase ] = std::bind( &cProtoMsgStub::BuyShop , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 손실 한도
	fp[ General::PacketID::Packet_LossCapUpdate ] = std::bind( &cProtoMsgStub::ChangeLostLimit , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LossCapTimeCheck ] = std::bind( &cProtoMsgStub::CheckLostLimitTime , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LossCapOptionsList ] = std::bind( &cProtoMsgStub::GetLostLimitOptions , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 안전 금고 입, 출금
	fp[ General::PacketID::Packet_VaultDeposit ] = std::bind( &cProtoMsgStub::SafeMoneyDeposit , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_VaultWithdraw ] = std::bind( &cProtoMsgStub::SafeMoneyWithdraw , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 광고
	fp[ General::PacketID::Packet_SponsorView ] = std::bind( &cProtoMsgStub::AdWatch , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 플레이어 정보 보기
	fp[ General::PacketID::Packet_ProfileRead ] = std::bind( &cProtoMsgStub::GetPlayerInfo , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// 친구
	fp[ General::PacketID::Packet_ContactAdd ] = std::bind( &cProtoMsgStub::AddFriend , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ContactRemove ] = std::bind( &cProtoMsgStub::DeleteFriend , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LobbyRoster ] = std::bind( &cProtoMsgStub::LobbyPlayerList , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ContactList ] = std::bind( &cProtoMsgStub::FriendList , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ProfileSearch ] = std::bind( &cProtoMsgStub::SearchPlayer , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	//cheat
	fp[ General::PacketID::Packet_LabCreditUpdate ] = std::bind( &cProtoMsgStub::CheatMoney , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LabSpaceEnter ] = std::bind( &cProtoMsgStub::RoomJoin , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_LabCardSwap ] = std::bind( &cProtoMsgStub::CheatCardChange , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	//fp[ Common::GMsgID::GMsg_CheatPinball ] = std::bind( &cProtoMsgStub::CHEAT_PINBALL , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	//fp[ Common::GMsgID::GMsg_CheatRoulette ] = std::bind( &cProtoMsgStub::CHEAT_Roulette , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleSpaceNumber ] = std::bind( &cProtoMsgStub::GetRoomNumber , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	// QA
	fp[ General::PacketID::Packet_ConsoleDeckPreset ] = std::bind( &cProtoMsgStub::CardDeckCheat , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleAliasLookup ] = std::bind( &cProtoMsgStub::UidSearchByNickname , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleLossCapReset ] = std::bind( &cProtoMsgStub::InitLostLimit , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleBuyCapReset ] = std::bind( &cProtoMsgStub::InitBuyLimit , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ General::PacketID::Packet_ConsoleAccess ] = std::bind( &cProtoMsgStub::QALogin , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );

	//ErrorLog
	fp[ General::PacketID::Packet_RequestFault ] = std::bind( &cProtoMsgStub::ClientErrorLog , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );


}

// Session 이 있는지 체크한다.
bool cProtoMsgStub::CheckSession( NetLib::cInterfaceIocpContext* pContext )
{
	cClientSession* pClientSession = static_cast< cClientSession* >( pContext->GetSession() );
	if ( pClientSession == nullptr ) {
		cInstancePacketParser* pParser = NetLib::cSingleton<cInstancePacketParser>::GetInstance();
		int nThreadIndex = pContext->GetCommandQueueIndex();

		PmNet::SysFaultRS response;
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "Session Error Without Login" );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_CoreFault , response , General::ResultCode::Result_SessionFault , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
	else
	{
		if ( !NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( pClientSession->GetPlayerIdx() ) || pContext->GetSession() != NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( pClientSession->GetPlayerIdx() ) )
		{
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "Session Error NO Table" );
			PmNet::SysFaultRS response;
			int nThreadIndex = pContext->GetCommandQueueIndex();
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_CoreFault , response , General::ResultCode::Result_SessionFault , errorString );
			return true;
		}
	}
	return pClientSession == nullptr ? true : false;
}

bool cProtoMsgStub::HasSession( NetLib::cInterfaceIocpContext* pContext )
{
	cClientSession* pClientSession = static_cast< cClientSession* >( pContext->GetSession() );

	return pClientSession != nullptr;
}

cClientSession* cProtoMsgStub::GetSession( NetLib::cInterfaceIocpContext* pContext )
{
	return static_cast< cClientSession* >( pContext->GetSession() );
}

//void cProtoMsgStub::bindmethod(std::function<void(UINT /*nID*/, UINT /*nCommand*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp)
//{
//	if (fp == nullptr)
//	{
//		return;
//	}
//
//	if (fp[CSNet::ProtocolCommand::SYS_NET_SESSION_LOG_OUT] != nullptr)
//	{
//		assert(false && "cScheduleStub::bindmethod");
//	}
//
//	fp[CSNet::ProtocolCommand::SYS_NET_SESSION_LOG_OUT] = std::bind(&cSystemStub::SYS_NET_SESSION_LOG_OUT, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
//}

//void cProtoMsgStub::SYS_NET_SESSION_LOG_OUT(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex)
//{
//	if (nLength != sizeof(Sys_Net_Session_Log_Out_Data))
//	{
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cInstancePacketParser::SYS_NET_SESSION_LOG_OUT Failed. nLength != sizeof(Sys_Net_Session_Log_Out_Data)");
//		return;
//	}
//
//	Sys_Net_Session_Log_Out_Data* pPtr = reinterpret_cast<Sys_Net_Session_Log_Out_Data*>(pData);
//
//	NetLib::cSessionManager* pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::ExistsInstance();
//	if (pSessionManager == nullptr)
//	{
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cInstancePacketParser::SYS_NET_SESSION_LOG_OUT Failed. SessionManager is nullptr");
//		return;
//	}
//
//	pSessionManager->RemovePending(pPtr->llAllocatedSessionSlot);
//
//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, "cInstancePacketParser::SYS_NET_SESSION_LOG_OUT Success. AccountIDX[ %I64d ]", pPtr->llAllocatedSessionSlot);
//}

void cProtoMsgStub::SYS_NET_CONNECT( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	NetLib::cContextPooler* pContextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
	if ( pContextPooler == nullptr )
	{
		assert( false && "SYS_NET_CONNECT is Failed. ContextPooler is nullptr" );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SYS_NET_CONNECT is Failed. ContextPooler is nullptr" );
		return;
	}

	pContextPooler->ConnectContext( reinterpret_cast< NetLib::cIocpContext* >( pContext ) , nThreadIndex );

	char szSessionInfos[ CSDef::EDef::MAX_BUFFER_128_LEN ] = { 0, };
	uint32 uiIP = pContext->GetIP();
	inet_ntop( AF_INET , &uiIP , szSessionInfos , sizeof( szSessionInfos ) );

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
		"[CommandThread] SYS_NET_CONNECT Success. Connect Context [ %d ] IP [ %s ]" ,
		pContext->GetEntity() , szSessionInfos );
}

void cProtoMsgStub::SYS_NET_DISCONNECT( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	const UINT Entity = pContext->GetEntity();
	auto allocateSlot = pContext->GetAllocateSlot();
	cClientSession* pSession = static_cast< cClientSession* >( pContext->GetSession() );
	uint64 playeridx = 0;
	if ( pSession != nullptr )
	{
		playeridx = pSession->GetPlayerIdx();
		/*pSession->SessionLogout(Entity);
		pSession->DisConnectContext( Entity , nThreadIndex );*/
		pSession->DisConnectContext( Entity , nThreadIndex );
		pSession->SyncFriend_Online( true , pSession , General::ContactState::ContactState_Offline );
		NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( pSession->GetPlayerIdx() );
		Server::SyncPlayerInfo syncPlayerLoginNoti;
		syncPlayerLoginNoti.set_player_idx( playeridx );

		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( syncPlayerLoginNoti.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SyncPlayerLoginNoti SyncPlayerInfo  SerializeToArray Failed." );
		}

		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeProfileSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncPlayerLoginNoti.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

			// 로비서버 전송 실패
		}

	}

	if ( pContext->GetContextType() == E_CONTEXT_TYPE::E_CONTEXT_SERVER )
	{
		if ( FALSE == NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->DeRegisterServer( static_cast< BYTE >( E_SERVER_TYPE::LOBBY_SERVER ) , pContext->GetAllocateSlot() ) )
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
				_T( "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Connector DeRegisterServer Failed. GetAllocateSlot %I64d" , pContext->GetAllocateSlot() ) );
		}
	}

	pContext->SetSession( nullptr );
	pContext->Disconnect();

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SYS_NET_DISCONNECT Context DISCONNECT Entity[ %u ], player_idx[ %u ]" , Entity , playeridx );
}

void cProtoMsgStub::CHEAT_DISCONNECT( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	const UINT Entity = pContext->GetEntity();
	cClientSession* pSession = static_cast< cClientSession* >( pContext->GetSession() );
	uint64 playeridx = 0;
	if ( pSession != nullptr )
	{
		playeridx = pSession->GetPlayerIdx();
		pSession->SetContext( nullptr );
	}

	pContext->SetSession( nullptr );
	pContext->Disconnect();

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SYS_NET_DISCONNECT Context DISCONNECT Entity[ %u ], player_idx[ %u ]" , Entity , playeridx );
}


// Ping 은 서버에서 클라에게 요청하고, Response 값을 분석해서 Latency 를 측정한다.
void cProtoMsgStub::Ping( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::HeartbeatRS response;
	if ( !response.ParseFromArray( pData , nLength ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Ping Parse Error. Entity = [ %d ]" , pContext->GetEntity() ); return;
	}

	if ( response.hb_val() == 0 )
	{
		// 비어 있으면 클라이언트 backping 으로 간주	
	}
	else
	{
		// 클라이언트에게 쏘아 보낸 서버 시간을 계산해서 Latency 를 체크 한다.
		uint64 sendTickCount = response.hb_val();
		ULONGLONG currentTickCount = ::GetTickCount64();
		uint64 latency = currentTickCount - sendTickCount;

		// Latency가 비정상적으로 큰 경우에만 출력한다. 1초로 설정한다.
		/*if ( latency > 1000  )
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Ping Received. Packet Duration = %I64d ms, Entity = [ %d ]" ,
				latency , pContext->GetEntity() );*/
	}
	NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->SetCheckTick( false );
	// 마지막으로 클라가 쏜 패킷 시간을 저장해 둔다.
	pContext->SetCurrentPacketTick();
}

// Ping 은 서버에서 클라에게 요청하고, Response 값을 분석해서 Latency 를 측정한다.
void cProtoMsgStub::PingClient( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::HeartbeatRQ req;
	if ( !req.ParseFromArray( pData , nLength ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Ping Parse Error. Entity = [ %d ]" , pContext->GetEntity() ); return;
	}

	if ( req.hb_val() == 0 )
	{
		// 비어 있으면 클라이언트 backping 으로 간주	
	}
	else
	{
		// ping 패킷을 직접 쏘아 줍니다.
		PmNet::HeartbeatRS  response;
		response.set_hb_val( req.hb_val() );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_LinkEcho , response , General::ResultCode::Result_Success , "" );
	}

	// 마지막으로 클라가 쏜 패킷 시간을 저장해 둔다.
	pContext->SetCurrentPacketTick();
}

void cProtoMsgStub::CreateAccount( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::BuildProfileRQ request;
	PmNet::BuildProfileRS response;
	if ( !request.ParseFromArray( pData , nLength ) )
	{
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[CreateAccount] Parsing failed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_TransportParseFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 수신 데이터
	General::IdentityCheckType authType = request.auth_kind();
	string ci = request.ci();			// 본인 인증 Key ( Nice Api CI
	string enctime = request.enc_ts(); // 본인 인증 시간
	std::string errorString;
	std::string _account_guid;

	// authCode 가 empty 가 아니라면, 기존의 account 에서 검색한다.
	if ( false == ci.empty() ) {
		std::vector<std::string> _delete_ids;
		std::future<BOOL> dresult = QueryManager::DeletedByCidAsync( ci , _delete_ids );
		dresult.wait();

		for ( auto _platform_guid : _delete_ids )
		{
			QueryManager::DeleteAccountAsync( _platform_guid );
		}

		std::vector<std::string> _made_ids;
		std::future<BOOL> sresult = QueryManager::SelectByCidAsync( ci , _made_ids );
		sresult.wait();

		if ( FALSE == sresult.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage ) {
			if ( _made_ids.size() >= 3 ) {
				SendMessageAndLogWrite( General::ResultCode::Result_AccountCreationLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}
		//string verification_result = verify_ci( ci , "api_key" , "api_url;" );
		//if ( verification_result.find( "\"status\":\"success\"" ) != std::string::npos )
		//{
		//	//성공
		//}
		//else
		//{
		//	//실패
		//	std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[CreateAccount] Verify Ci Failed." );
		//	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_IdentityVerificationFailed , errorString );
		//	return;
		//}

		std::future<std::string> result = std::async( [ci]() {
			return QueryManager::AccountGetByAuthCode( ci );
		} );

		result.wait();

		_account_guid = result.get();

		if ( false == _account_guid.empty() ) {
			response.set_auth_kind( General::IdentityCheckType::IdentityCheck_KoreanResident );
			response.set_profile_uid( _account_guid );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
			return;
		}

		/*
		else {
			errorString = protoutil::cProtoUtil::ErrorCodeString( "find account failed" );
			pParser->SendBuffer( pParser , pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		}
		*/
	}


#ifdef _DEBUG

	// 플랫폼 로그인 테스트
	//PlatformAuthReq _request;
	//_request.set_outlet_kind( General::AccessChannelType::AccessChannel_Android );

	//std::string url = "";
	//char szWebUrl[ CSDef::EDef::MAX_BUFFER_256_LEN ] = { 0, };
	//StringCbPrintfA( szWebUrl ,
	//	sizeof( szWebUrl ) ,
	//	"http://%s:%d/%s/%s" ,
	//	"localhost" ,
	//	35582 ,
	//	"Game" ,
	//	"ServerRequest" );

	//std::string data;

	//std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( szWebUrl ,
	//		_request ,
	//		static_cast< UINT >( General::PacketID::Packet_PlatformVerify ) ,
	//		0 ,
	//		0 ,
	//		data ,
	//		10 );

	//web_result.wait();

#endif

	// 계정을 생성한다.
	// 클라이언트가 account_guid 를 발급받은 상태라면, 이부분은 생략되면 된다.
	// 16자리 account token 생성한다.
	_account_guid = NetLib::cSingleton<AuthCodeGenerator>::GetInstance()->generate( 16 );
	int _db_index = 1;
	uint64_t _user_account_idx = 0;

	string _name_auth_type = "";
	switch ( authType )
	{
	case General::IdentityCheckType::IdentityCheck_KoreanResident:
	{
		_name_auth_type = "NameAuthType_Korea";
		break;
	}
	default:
	{
		// 현재 로써는 딱히 처리 하지 않고, default 처리함
		_name_auth_type = "NameAuthType_Korea";
	}
	}

	uint64 _lost_limit = 100000000000; // 천억
	int _lost_limit_change_count = 0;
	string _refresh_loss_limit = TimeUtils::TomorrowStartTimeString(); // 다음날 0시 초기화
	uint64 _buy_limit = 1000000; // 구매 제한 디폴트 70만원
	string _refresh_buy_limit = TimeUtils::NextMonthStartTimeString(); // 다음달에 초기화, 변경 할 수 있는지 잘 모름

	// Async 코드
	std::future<BOOL> result = QueryManager::CreateAccountAsync( ci , _name_auth_type , _account_guid , _db_index , _lost_limit , _lost_limit_change_count , _refresh_loss_limit , _buy_limit , _refresh_buy_limit , _user_account_idx );
	result.wait();

	if ( FALSE == result.get() ) {
		errorString = protoutil::cProtoUtil::ErrorCodeString( "create account failed" );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "CreateAccount Failed. account_guid = %s", _account_guid);
		return;
	}

	response.set_auth_kind( General::IdentityCheckType::IdentityCheck_KoreanResident );
	response.set_profile_uid( _account_guid );

	string requestno = request.req_seq();
	string receivedata = request.recv_blob();

	string server_receivedata = "";
	if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
	{
		//레디스 검증정보 받아오기
		NetLib::cSingleton<cRedisController>::GetInstance()->GetAuthData( requestno , server_receivedata );

		//검증이 이상한 유저
		if ( receivedata != server_receivedata )
		{
			std::string t_error = std::format( "GetAuthData FAIL Error [requestno : {} , receivedata : {}]" , requestno , receivedata );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , t_error.c_str() );
			SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthTimedOut , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		else
			NetLib::cSingleton<cRedisController>::GetInstance()->DeleteAuthData( requestno );
	}

	//계정생성텀이 맥스값보다 작을경우 로그
	std::string ip = ::ConvertIP( pContext->GetIP() );
	if ( NetLib::cSingleton<IpTimeQueue>::GetInstance()->GetMaxTime() > NetLib::cSingleton<IpTimeQueue>::GetInstance()->findIP( ip ) )
	{
		auto result = QueryManager::InsertAbusingLog( ip );
		if ( false == result.get() )
		{
			std::string t_error = std::format( "InsertAbusingLog Error [ {} ]" , ip );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , t_error.c_str() );
		}
	}
	else
	{
		NetLib::cSingleton<IpTimeQueue>::GetInstance()->push( ip );
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "CreateAccount Success. account_guid = %s", _account_guid);
}

void cProtoMsgStub::CreatePlatform( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::BuildOutletRQ request;
	PmNet::BuildOutletRS response;
	if ( !request.ParseFromArray( pData , nLength ) )
	{
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[CreatePlatform] Parsing failed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_TransportParseFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	General::AccessChannelType platformType = request.outlet_kind();
	string _accountGuid = request.profile_uid();
	string _platformAuthCode = request.outlet_token();
	string nickName = request.alias_label();
	BYTE _platform_code = static_cast< int >( platformType );
	BYTE _push_token_os = static_cast< int >( platformType );
	string _push_token = request.alert_token();
	string _platform_guid;
	int _db_index = 1;
	uint64_t _platform_idx = 0;
	uint64 _player_idx = 0;
	uint64 _avatar_idx = 0;

	switch ( platformType )
	{
	case General::AccessChannelType::AccessChannel_Android:
	case General::AccessChannelType::AccessChannel_IOS:
	case General::AccessChannelType::AccessChannel_PC:
	case General::AccessChannelType::AccessChannel_LocalAccount:
	{
		// 인증 처리

		break;
	}
	default:
	{
		// 에러 메시지 전송하고 종료

		return;
	}
	}

	if ( _platformAuthCode.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ChannelAuthTokenRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}


// _accountGuid 가 비어 있는 경우, _platformAuthCode 로 계정을 검색
	if ( _accountGuid.empty() )
	{
		std::string _account_guid;
		std::string _platform_guid;
		auto result = QueryManager::FindAccountByPlatformAuthCodeAsync( _platformAuthCode , _account_guid , _platform_guid );
		result.wait();

		// 시스템 에러임
		if ( FALSE == result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 계정을 찾지 못함, 본인 인증 필요
		// 계정은 존재하나 플랫폼 계정이 없는 경우도 포함 ( 본인 인증 부터 다시 )
		if ( _account_guid.empty() || _platform_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// _platformAuthCode 로 계정 및 플랫폼 검색이 성공 하였다.
		// response 처리
		response.set_profile_uid( _account_guid );
		response.set_outlet_uid( _platform_guid );
		//std::string errorString = protoutil::cProtoUtil::ErrorCodeString("");
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
	}
	else
	{
		/*std::vector<std::string> _made_ids;
		std::future<BOOL> result = QueryManager::SelectByAccountIdAsync( _accountGuid , _made_ids );
		result.wait();

		if ( FALSE == result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( _made_ids.size() >= 3 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_AccountCreationLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}*/

		// _accountGuid 가 존재하고, _platformAuthCode 가 비어 있지 않으면 platform 데이터를 조회해서 가져온다.
		if ( false == _platformAuthCode.empty() ) {
			std::future<BOOL> result = std::async( [_accountGuid , _platformAuthCode , &_platform_guid , &_platform_code , &nickName , &_push_token_os , &_push_token , &_db_index , &_platform_idx]() {
				return QueryManager::PlatformGet( _accountGuid , _platformAuthCode , _platform_guid , _platform_code , nickName , _push_token_os , _push_token , _db_index , _platform_idx ); // platform 데이터를 리턴해주도록 변경 필요
			} );

			result.wait();

			// TRUE 면 platform 데이터가 이미 존재
			if ( result.get() ) {
			}
		}

		// _platform_guid 가 비어 있으면 플랫폼 생성이 안된 경우
		if ( _platform_guid.empty() )
		{
			_platform_guid = NetLib::cSingleton<AuthCodeGenerator>::GetInstance()->generate( 16 ); // 16자리 platform token 생성한다.

			std::wstring wideStr = StringUtil::ConvertToWide( nickName );
			std::string utf8StrAgain = StringUtil::ConvertToUtf8( wideStr );

			std::string _made_id = _platform_guid; // made_id unique 때문에 이런 방식을 취함
			std::string _made_password;
			std::string _sub_password;

			// Async 코드
			std::future<BOOL> result = std::async( [_accountGuid , _platform_guid , _platformAuthCode , _platform_code , _made_id , _made_password , _push_token_os , _push_token , _db_index , _sub_password , &_platform_idx]() {
				return QueryManager::CreatePlatform( _accountGuid , _platform_guid , _platformAuthCode , _platform_code , _made_id , "" , _push_token_os , _push_token , _db_index , _sub_password , _platform_idx );
			} );

			result.wait();

			//if ( TRUE == result.get() )
			{
				// CreatePlatform 이 성공하면, 플레이어 캐릭터를 생성한다.
				// default value set
				General::ParticipantProfile player;
				Server::ParticipantProfileInternal playerExt;
				//player.set_loss_limit( 0 );
				//player.set_refresh_time_of_loss_limit( TimeUtils::GetCurrentDateTime() );
				//player.set_buy_limit( 0 );
				//player.set_refresh_time_of_buy_limit( TimeUtils::GetCurrentDateTime() );
				player.set_kick_ticket_balance( 0 );
				player.set_equipped_avatar_id( 1 );
				player.set_membership_enabled( false );
				player.set_membership_expires_at( TimeUtils::GetCurrentDateTime() );
				player.set_chip_refill_uses( 0 );
				player.set_coin_refill_uses( 0 );
				//player.set_sub_passwd_activated( false );
				//player.set_aux_key( "" );

				uint64 defaultChip = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_InitialChip );
				uint64 defaultCoin = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_InitialCoin );

#ifdef _DEBUG
				player.set_member_level( 1 );
				player.set_experience_points( 0 );
				player.set_wallet_chips( defaultChip );
				player.set_wallet_coins( defaultCoin ); // 오천만 코인으로 변경해둠
				playerExt.set_remaining_reel_coin( 0 ); // 슬롯 짜투리 코인 저장용
				player.set_wallet_gems( 0 );
#else
				player.set_member_level( 1 );
				player.set_experience_points( 0 );
				player.set_wallet_chips( defaultChip );
				player.set_wallet_coins( defaultCoin );
				playerExt.set_remaining_reel_coin( 0 ); // 슬롯 짜투리 코인 저장용
				player.set_wallet_gems( 0 );
#endif
				player.set_display_name( _platform_guid );
				player.set_attendance_streak_days( 0 ); // 로그인 하면서 Day 1 리워드 내려주도록 하자.

				const auto& tomorrowStartTm = TimeUtils::TomorrowStartTimeTM();
				std::string dailyRefreshTime = TimeUtils::TMToString( tomorrowStartTm );

				const auto& nextMonthStartTm = TimeUtils::NextMonthStartTimeTM();
				std::string monthlyRefreshTime = TimeUtils::TMToString( nextMonthStartTm );

				// Async 코드
				result = std::async( [_accountGuid , _platform_guid , player , playerExt , dailyRefreshTime , monthlyRefreshTime , &_player_idx]() {
					//QueryManager* queryManager = new QueryManager();
					return QueryManager::InsertPlayer( _accountGuid , _platform_guid ,
					player.display_name() ,
					player.member_level() ,
					player.experience_points() ,
					//player.loss_limit() ,
					//player.refresh_time_of_loss_limit() ,
					//player.buy_limit() ,
					//player.refresh_time_of_buy_limit() ,
						player.wallet_chips() ,
						player.wallet_coins() ,
						player.wallet_gems() ,
						player.kick_ticket_balance() ,
						player.equipped_avatar_id() ,
						player.membership_enabled() ,
						player.membership_expires_at() ,
						player.chip_refill_uses() ,
						player.coin_refill_uses() ,
						//player.sub_passwd_activated() ,
						//player.aux_key() ,
						playerExt.remaining_reel_coin() ,
						dailyRefreshTime ,
						monthlyRefreshTime ,
						_player_idx );
				} );

				result.wait();

				if ( result.get() )
				{
					player.set_member_id( _player_idx );

					// 플레이어 데이터를 ClientSession 에다가 저장해 둔다.
				}
				else
				{
					// 플레이어 데이터 생성 실패
				}

				// 이곳에서 부터는 _player_idx 로 생성할 것들을 생성한다.

				// Avatar
				// free 아바타 전체 생성
				std::vector<std::future<BOOL>> results;
				const auto& avatars = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatars();
				for ( auto& avatar : avatars ) {

					// free 인 경우는 99년 까지 착용하도록
					std::string _expiry_date;
					if ( avatar.free_available() )
						_expiry_date = "2099-12-31 23:59:59";
					else
						_expiry_date = TimeUtils::GetCurrentDateTime();

					results.push_back( QueryManager::InsertAvatarAsync( _player_idx , avatar.avatar_ref_id() , _expiry_date , _avatar_idx ) );
				}

				for ( auto& result : results ) {

					result.wait();

					if ( result.get() )
					{
					}
					else
					{
						// 아바타 생성 실패
					}
				}

				// response 처리
				response.set_profile_uid( _accountGuid );
				response.set_outlet_uid( _platform_guid );
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
				GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );

				/*NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "CreatePlatform Success. account_guid = %s, platform_guid = %s",
					StringUtil::RemoveSpecialCharactersAndSpaces( _accountGuid ),
					StringUtil::RemoveSpecialCharactersAndSpaces( _platform_guid ) );*/

					//
				{
					int code = 10000;
					std::string loginTypeString = "";

					//std::string ipString = ::ConvertIP( pContext->GetIP() );
					//ipString = "";
					// playerPlatform
					int platform_code;
					std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( _accountGuid , _platform_guid , platform_code );
					platform_code_result.wait();

					if ( FALSE == platform_code_result.get() ) {
					}
					std::string idpcode = General::AccessChannelType_Name( static_cast< General::AccessChannelType > ( platform_code ) );

					auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
					std::string marketString = valueDescriptor->name();

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						_platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid **
						"" , // asset **
						"" , // cmd
						request.dev_meta() , // device
						"" , // gmsessid
						marketString , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다. **
						"" , // ipaddr
						request.os_ver() , // osver
						idpcode , // pf
						_accountGuid , // svcuid
						"" , // accountid
						0 , // _player.wallet_chips() + _player.vault_chips() , // chip
						0 , // _player.wallet_chips() , // chip_g
						0 , // _player.vault_chips() , // chip_s
						0 , // _player.wallet_coins() + _player.vault_coins() , // coin
						0 , // _player.wallet_coins() , // coin_g
						0 , // _player.vault_coins() , // coin_s
						0 , // slotcoin
						0 , // _player.wallet_gems() + _player.paid_gems() , // gem
						0 , // _player.wallet_gems() , // gem_f
						0 , // _player.paid_gems() , // gem_p
						0 , // _player.kick_ticket_balance() , // kickoutticket
						0 , // _player.experience_points() , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						loginTypeString , // logintype
						"" , // type
						"" , // _player.display_name() , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						request.app_ver() , // ver
						"" ); // etc

					log_result.wait();


					int code_10801 = 10801;
					//auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
					//std::string marketString = valueDescriptor->name();

					std::future<BOOL> log_result_10801 = QueryManager::InsertAccountLog(
						code_10801 , // code
						_platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						request.dev_meta() , // request.dev_meta() , // device
						"" , // gmsessid
						marketString , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						request.os_ver() , // request.os_ver() , // osver
						idpcode , // pf
						"" , // svcuid
						_platformAuthCode , // accountid
						0 , //_player.wallet_chips() + _player.vault_chips() , // chip
						0 , //_player.wallet_chips() , // chip_g
						0 , //_player.vault_chips() , // chip_s
						0 , //_player.wallet_coins() + _player.vault_coins() , // coin
						0 , //_player.wallet_coins() , // coin_g
						0 , //_player.vault_coins() , // coin_s
						0 , // slotcoin
						0 , //_player.wallet_gems() + _player.paid_gems() , // gem
						0 , //_player.wallet_gems() , // gem_f
						0 , //_player.paid_gems() , // gem_p
						0 , //_player.kick_ticket_balance() , // kickoutticket
						0 , //_player.experience_points() , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						"" , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						request.app_ver() , // ver
						"" ); // etc

					log_result_10801.wait();
				}
			}
		}
		else
		{
			// _platform_guid 가 empty 가 아닌 경우
			response.set_profile_uid( _accountGuid );
			response.set_outlet_uid( _platform_guid );
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );

			/*NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "CreatePlatform Success. already created account_guid = %s, platform_guid = %s" ,
					StringUtil::RemoveSpecialCharactersAndSpaces( _accountGuid ) ,
					StringUtil::RemoveSpecialCharactersAndSpaces( _platform_guid ) );*/
			return;
		}

	}

}

void cProtoMsgStub::GetCreatedPlatformsByCid( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::FetchBuiltOutletsByCidRQ request;
	PmNet::FetchBuiltOutletsByCidRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string cid = request.ci();

	std::vector<std::string> _made_ids;
	std::future<BOOL> result = QueryManager::SelectMADEByCidAsync( cid , _made_ids );
	result.wait();

	if ( FALSE == result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	/*if ( _made_ids.size() >= 3 ) {
		response.set_built_outlet_cnt( 5 );
		SendMessageAndLogWrite( General::ResultCode::Result_AccountCreationLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}*/

	response.set_built_outlet_cnt( _made_ids.size() );
	for ( auto& _made_id : _made_ids ) {
		response.add_native_ids( _made_id );
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	// 특문 섞인 경우 출력 안함
	if ( StringUtil::HasPercent( cid ) )
		return;

	/*NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "GetCreatedPlatformsByCid Success. ci = %s, find count = %d" ,
		StringUtil::RemoveSpecialCharactersAndSpaces( cid ) , _made_ids.size() );*/
}


void cProtoMsgStub::GetCreatedPlatformsByCidForCreate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::FetchBuiltOutletsByCidRQ request;
	PmNet::FetchBuiltOutletsByCidRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string cid = request.ci();
	std::vector<std::string> _delete_ids;
	std::future<BOOL> sresult = QueryManager::DeletedByCidAsync( cid , _delete_ids );
	for ( auto _platform_guid : _delete_ids )
	{
		QueryManager::DeleteAccountAsync( _platform_guid );
	}
	std::vector<std::string> _made_ids;
	std::future<BOOL> result = QueryManager::SelectByCidAsync( cid , _made_ids );
	result.wait();

	if ( FALSE == result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage ) {
		if ( _made_ids.size() >= 3 ) {
			response.set_built_outlet_cnt( 5 );
			SendMessageAndLogWrite( General::ResultCode::Result_AccountCreationLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}

	response.set_built_outlet_cnt( _made_ids.size() );
	for ( auto& _made_id : _made_ids ) {
		response.add_native_ids( _made_id );
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	// 특문 섞인 경우 출력 안함
	if ( StringUtil::HasPercent( cid ) )
		return;

	/*NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "GetCreatedPlatformsByCid Success. ci = %s, find count = %d" ,
		StringUtil::RemoveSpecialCharactersAndSpaces( cid ) , _made_ids.size() );*/
}

void cProtoMsgStub::CreateMADEPlatform( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::BuildNativeOutletRQ request;
	PmNet::BuildNativeOutletRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string made_id = request.native_id();
	std::string made_password = request.native_key();
	const bool& check_duplicate = request.dup_check();
	response.set_dup_check( check_duplicate );


	// ID 중복 체크
	if ( check_duplicate )
	{
		std::vector<std::string> _find_ids;
		std::future<BOOL> result = QueryManager::SelectMADEIdAsync( made_id , _find_ids );
		result.wait();

		if ( result.get() )
		{
			if ( _find_ids.size() ) {
				// 중복 검출
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdAlreadyUsed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
				return;
			}
			else
			{
				GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
			}
		}
		else
		{
			SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			return;
		}
	}
	// 자체 플랫폼 생성
	else
	{

		General::IdentityCheckType authType = General::IdentityCheckType::IdentityCheck_KoreanResident;

		// 졔약 조건 체크

		// ID 유효성 체크 -> 영문자와 숫자로 조합 5자에서 12자의 문자열
		if ( false == MADEPlatform::ValidateId( made_id ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdPolicyBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 패스워드 유효성 체크
		if ( false == MADEPlatform::isValidPassword( made_password ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessSecretPolicyBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		//std::wstring wideStr = StringUtil::ConvertToWide( made_password );

		// AccountGuid 검색
		const std::string& ci = request.ci();

		std::vector<std::string> _delete_ids;
		std::future<BOOL> sresult = QueryManager::DeletedByCidAsync( ci , _delete_ids );
		for ( auto _platform_guid : _delete_ids )
		{
			QueryManager::DeleteAccountAsync( _platform_guid );
		}

		std::future<std::string> result = std::async( [ci]() {
			return QueryManager::AccountGetByAuthCode( ci );
		} );

		result.wait();


		//아이디 갯수검사
		{
			std::vector<std::string> _made_ids;
			std::future<BOOL> result = QueryManager::SelectByCidAsync( request.ci() , _made_ids );
			result.wait();

			if ( FALSE == result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage ) {
				if ( _made_ids.size() >= 3 ) {
					//response.set_dup_check( 5 );
					SendMessageAndLogWrite( General::ResultCode::Result_AccountCreationLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}
			}
		}

		std::string _account_guid = result.get();
		if ( _account_guid.empty() ) {

			_account_guid = NetLib::cSingleton<AuthCodeGenerator>::GetInstance()->generate( 16 );
			int _db_index = 1;
			uint64_t _user_account_idx = 0;

			string _name_auth_type = "";
			switch ( authType )
			{
			case General::IdentityCheckType::IdentityCheck_KoreanResident:
			{
				_name_auth_type = "NameAuthType_Korea";
				break;
			}
			default:
			{
				// 현재 로써는 딱히 처리 하지 않고, default 처리함
				_name_auth_type = "NameAuthType_Korea";
			}
			}

			uint64 _lost_limit = 100000000000; // 천억
			int _lost_limit_change_count = 0;
			string _refresh_loss_limit = TimeUtils::TomorrowStartTimeString(); // 다음날 0시 초기화
			uint64 _buy_limit = 1000000; // 구매 제한 디폴트 70만원
			string _refresh_buy_limit = TimeUtils::NextMonthStartTimeString(); // 다음달에 초기화, 변경 할 수 있는지 잘 모름

			// Async 코드
			std::future<BOOL> result = QueryManager::CreateAccountAsync( ci , _name_auth_type , _account_guid , _db_index , _lost_limit , _lost_limit_change_count , _refresh_loss_limit , _buy_limit , _refresh_buy_limit , _user_account_idx );
			result.wait();

			if ( FALSE == result.get() )
			{
				// 계정 생성 실패
				SendMessageAndLogWrite( General::ResultCode::Result_AccountProvisionFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			response.set_profile_uid( _account_guid );
		}


		string requestno = request.req_seq();
		string receivedata = request.recv_blob();

		string server_receivedata = "";
		if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
		{
			//레디스 검증정보 받아오기
			NetLib::cSingleton<cRedisController>::GetInstance()->GetAuthData( requestno , server_receivedata );

			//검증이 이상한 유저
			if ( receivedata != server_receivedata )
			{
				std::string t_error = std::format( "GetAuthData FAIL Error [requestno : {} , receivedata : {}]" , requestno , receivedata );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , t_error.c_str() );
				SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthTimedOut , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			else
				NetLib::cSingleton<cRedisController>::GetInstance()->DeleteAuthData( requestno );
		}


		//계정생성텀이 맥스값보다 작을경우 로그
		std::string ip = ::ConvertIP( pContext->GetIP() );
		if ( NetLib::cSingleton<IpTimeQueue>::GetInstance()->GetMaxTime() > NetLib::cSingleton<IpTimeQueue>::GetInstance()->findIP( ip ) )
		{
			auto result = QueryManager::InsertAbusingLog( ip );
			if ( false == result.get() )
			{
				std::string t_error = std::format( "InsertAbusingLog Error [ {} ]" , ip );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , t_error.c_str() );
			}
		}
		else
		{
			NetLib::cSingleton<IpTimeQueue>::GetInstance()->push( ip );
		}


		std::string _platform_guid = NetLib::cSingleton<AuthCodeGenerator>::GetInstance()->generate( 16 ); // 16자리 platform token 생성한다.
		BYTE _push_token_os = 0;
		string _push_token;
		int _db_index = 1;
		int _platform_code = General::AccessChannelType::AccessChannel_LocalAccount; // 자체 플랫폼
		uint64 _platform_idx = 0;
		string _platformAuthCode = _platform_guid; // 자체 인증이라 없다. _platform_guid 로 대체한다.
		uint64 _player_idx = 0;
		uint64 _avatar_idx = 0;
		std::string _sub_password;

		// 암호화된 필드의 DB 저장시에 오류가 나고 있음
		std::string passwordEncryption = MADEPlatform::EncryptPassword( made_password );

		/*std::string query = QueryManager::GenerateCreatePlatform(_account_guid , _platform_guid , _platformAuthCode , _platform_code , made_id , passwordEncryption , _push_token_os , _push_token , _db_index , _platform_idx);
		QueryManager::PlayerExecuteQuery( query );*/

		// Async 코드
		std::future<BOOL> platform_result = std::async( [_account_guid , _platform_guid , _platformAuthCode , _platform_code , made_id , made_password , _push_token_os , _push_token , _db_index , _sub_password , &_platform_idx]() {
			return QueryManager::CreatePlatform( _account_guid , _platform_guid , _platformAuthCode , _platform_code , made_id , made_password , _push_token_os , _push_token , _db_index , _sub_password , _platform_idx );
		} );

		platform_result.wait();

		if ( FALSE == platform_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		uint64 defaultChip = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_InitialChip );
		uint64 defaultCoin = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_InitialCoin );

		// CreatePlatform 이 성공하면, 플레이어 캐릭터를 생성한다.
		General::ParticipantProfile player;
		Server::ParticipantProfileInternal playerExt;
		player.set_kick_ticket_balance( 0 );
		player.set_equipped_avatar_id( 1 );
		player.set_membership_enabled( false );
		player.set_membership_expires_at( TimeUtils::GetCurrentDateTime() );
		player.set_chip_refill_uses( 0 );
		player.set_coin_refill_uses( 0 );
#ifdef _DEBUG
		player.set_member_level( 1 );
		player.set_experience_points( 0 );
		player.set_wallet_chips( defaultChip );
		player.set_wallet_coins( defaultCoin ); // 오천만 코인으로 변경해둠
		playerExt.set_remaining_reel_coin( 0 ); // 슬롯 짜투리 코인 저장용
		player.set_wallet_gems( 0 );
#else
		player.set_member_level( 1 );
		player.set_experience_points( 0 );
		player.set_wallet_chips( defaultChip );
		player.set_wallet_coins( defaultCoin );
		playerExt.set_remaining_reel_coin( 0 ); // 슬롯 짜투리 코인 저장용
		player.set_wallet_gems( 0 );
#endif
		player.set_display_name( _platform_guid );
		player.set_attendance_streak_days( 0 ); // 로그인 하면서 Day 1 리워드 내려주도록 하자.

		const auto& tomorrowStartTm = TimeUtils::TomorrowStartTimeTM();
		std::string dailyRefreshTime = TimeUtils::TMToString( tomorrowStartTm );

		const auto& nextMonthStartTm = TimeUtils::NextMonthStartTimeTM();
		std::string monthlyRefreshTime = TimeUtils::TMToString( nextMonthStartTm );

		// Async 코드
		std::future<BOOL> player_result = std::async( [_account_guid , _platform_guid , player , playerExt , dailyRefreshTime , monthlyRefreshTime , &_player_idx]() {
			return QueryManager::InsertPlayer( _account_guid , _platform_guid ,
			player.display_name() ,
			player.member_level() ,
			player.experience_points() ,
			player.wallet_chips() ,
			player.wallet_coins() ,
			player.wallet_gems() ,
			player.kick_ticket_balance() ,
			player.equipped_avatar_id() ,
			player.membership_enabled() ,
			player.membership_expires_at() ,
			player.chip_refill_uses() ,
			player.coin_refill_uses() ,
			playerExt.remaining_reel_coin() ,
			dailyRefreshTime ,
			monthlyRefreshTime ,
			_player_idx );
		} );

		player_result.wait();

		if ( player_result.get() )
		{
			player.set_member_id( _player_idx );

			// 플레이어 데이터를 ClientSession 에다가 저장해 둔다.
		}
		else
		{
			// 플레이어 데이터 생성 실패
		}

		// 이곳에서 부터는 _player_idx 로 생성할 것들을 생성한다.

		// Avatar
		// free 아바타 전체 생성
		std::vector<std::future<BOOL>> results;
		const auto& avatars = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatars();
		for ( auto& avatar : avatars ) {

			// free 인 경우는 99년 까지 착용하도록
			std::string _expiry_date;
			if ( avatar.free_available() )
				_expiry_date = "2099-12-31 23:59:59";
			else
				_expiry_date = TimeUtils::GetCurrentDateTime();

			results.push_back( QueryManager::InsertAvatarAsync( _player_idx , avatar.avatar_ref_id() , _expiry_date , _avatar_idx ) );
		}

		for ( auto& result : results ) {

			result.wait();

			if ( result.get() )
			{
			}
			else
			{
				// 아바타 생성 실패
			}
		}

		// response 처리
		response.set_profile_uid( _account_guid );
		response.set_outlet_uid( _platform_guid );
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );

		/*NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "CreateMADEPlatform Success. account_guid = %s, platform_guid = %s" ,
			StringUtil::RemoveSpecialCharactersAndSpaces( _account_guid ) , StringUtil::RemoveSpecialCharactersAndSpaces( _platform_guid ) );*/

		{
			int code = 10801;
			std::string loginTypeString = "";

			//std::string ipString = ::ConvertIP( pContext->GetIP() );
			//ipString = "";
			std::string idpcode = "";

			//auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
			//std::string marketString = valueDescriptor->name();

			std::future<BOOL> log_result = QueryManager::InsertAccountLog(
				code , // code
				_platform_guid , // platform_guid , // uid
				"" , // adminid
				"" , // adid
				"" , // asset
				"" , // cmd
				"" , // request.dev_meta() , // device
				"" , // gmsessid
				"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
				"" , // ipaddr
				"" , // request.os_ver() , // osver
				"" , // marketString , // pf
				"" , // svcuid
				made_id , // accountid
				0 , //_player.wallet_chips() + _player.vault_chips() , // chip
				0 , //_player.wallet_chips() , // chip_g
				0 , //_player.vault_chips() , // chip_s
				0 , //_player.wallet_coins() + _player.vault_coins() , // coin
				0 , //_player.wallet_coins() , // coin_g
				0 , //_player.vault_coins() , // coin_s
				0 , // slotcoin
				0 , //_player.wallet_gems() + _player.paid_gems() , // gem
				0 , //_player.wallet_gems() , // gem_f
				0 , //_player.paid_gems() , // gem_p
				0 , //_player.kick_ticket_balance() , // kickoutticket
				0 , //_player.experience_points() , // exp
				0 , // friend_cnt
				"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				"" , // logintype
				"" , // type
				"" , // nickname
				"" , // deletetime
				"" , // registered
				0 , // block_hour
				0 , // left_count
				0 , // limit_type
				"" , // reqtm
				"" , // second_pw
				"" , // reason
				"" , // result
				"" , // hash
				request.app_ver() , // ver
				"" ); // etc

			log_result.wait();
		}

	}
}

void cProtoMsgStub::Login( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{

	PmNet::SigninRQ request;
	PmNet::SigninRS response;

	if ( !request.ParseFromArray( pData , nLength ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"Login: ParseInbound failed for PmNet.SigninRQ" );
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 로그인 전에 이루어 지는 상황이라 세션이 있으면 세션 에러로 튕겨 낸다.
	if ( HasSession( pContext ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SessionFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string s_ipString = ::ConvertIP( pContext->GetIP() );

	// 로그인 검증 account_guid, platform_guid 로 캐릭터를 로딩한다.
	string account_guid = request.profile_uid();
	string platform_guid = request.outlet_uid();
	string device_info = request.dev_meta();
	string os_version = request.os_ver();
	string game_version = request.app_ver();
	string made_id = request.native_id();
	string MADE_passwd = request.native_key();
	string input_sub_password = request.aux_key();
	string _expected_withdrawal_date;
	General::StoreChannel mrk = request.store_kind();
	int _cancle_withdrawal_first_join = 0;
	std::string _platform_authcode = "";

	BYTE _platform_code = 0;
	UINT roomNumber = 0;
#ifdef _DEBUG

	std::string error = std::format( "LOGIN version : {} , Market : {} " , game_version , ( int ) mrk );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , error.c_str() );
#endif

	try
	{
		// 점검 체크
		if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenance( request.store_kind() , s_ipString , game_version ) ) {

			const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();

			std::string errorString;
			PmNet::ServiceNotice system_message;
			system_message.set_notice( message.message() );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_ServicePaused , errorString );
			pContext->SetSession( nullptr );
			pContext->Disconnect();
			return;
		}
		// 버젼 체크
		Server::ClientUpdatePolicy _version_update = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->CompareVersions( request.store_kind() , game_version );
		switch ( _version_update )
		{
		case Server::ClientUpdatePolicy::ClientUpdate_Required:
		{
			const auto& server_version = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->GetVersion( request.store_kind() );

			// 버젼 업데이트 알림
			PmNet::BuildBulletinRS versionResponse;

			versionResponse.set_notice( server_version.message() );
			versionResponse.set_min_ver( server_version.version_min() );
			versionResponse.set_latest_ver( server_version.version_latest() );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ClientRevisionNotice , versionResponse , General::ResultCode::Result_ClientRevisionBlocked , server_version.message() );
			return;
		}
		break;
		case Server::ClientUpdatePolicy::ClientUpdate_Recommended:
		{
		}
		break;
		case Server::ClientUpdatePolicy::ClientUpdate_CheckFailed:
		{
			SendMessageAndLogWrite( General::ResultCode::Result_ClientRevisionBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		break;
		}


		//Server::MaintenanceMessage message;
		//QueryManager::GetMaintenanceMessage( message );


		// 자체 로그인 이면 패스워드가 맞는지 확인한다.
		if ( made_id.size() != 0 )
		{
			int _passwd_retry_count = 0;
			std::string _find_password;
			int _sub_passwd_retry_count = 0;
			std::string _sub_password;
			BOOL _game_play_agree = FALSE;
			BOOL _personal_info_agree = FALSE;
			BOOL _advertise_push_agree = FALSE;
			BOOL _night_advertise_push_agree = FALSE;

			std::future<BOOL> MADE_result = QueryManager::FindMADEPlatformAsync( made_id , account_guid , platform_guid , _platform_code , _find_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count ,
				_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

			MADE_result.wait();

			if ( FALSE == MADE_result.get() ) {

				// 플랫폼 찾기 실패
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "11111111111111111111111111111111");
			//Sleep( 100 );

			if ( account_guid.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			if ( _passwd_retry_count >= 5 ) {
				response.set_key_miss_cnt( _passwd_retry_count );
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessRetryLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			response.set_alert_optin( _advertise_push_agree );
			response.set_night_alert_optin( _night_advertise_push_agree );

			// 패스워드가 잘못되었습니다.
			if ( false == MADE_passwd._Equal( _find_password ) ) {
				++_passwd_retry_count;
				response.set_key_miss_cnt( _passwd_retry_count );

				if ( _passwd_retry_count < 5 )
					SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessPasswordMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
				else
					SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessRetryLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );

				// 10802 작성

				int code = 10802;

				//std::string ipString = ::ConvertIP( pContext->GetIP() );

				auto log_result = QueryManager::InsertAccountLog(
					code , // code
					platform_guid , // platform_guid , // uid
					"" , // adminid
					"" , // adid
					"" , // asset
					"" , // cmd
					device_info , // request.dev_meta() , // device
					"" , // gmsessid
					"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
					"" , // ipaddr
					"" , // request.os_ver() , // osver
					"" , // marketString , // pf
					"" , // svcuid
					made_id , // accountid
					0 , // chip
					0 , // chip_g
					0 , // chip_s
					0 , // coin
					0 , // coin_g
					0 , // coin_s
					0 , // slotcoin
					0 , // gem
					0 , // gem_f
					0 , // gem_p
					0 , // kickoutticket
					0 , // exp
					0 , // friend_cnt
					"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
					"" , // logintype
					"" , // type
					" " , // nickname
					"" , // deletetime
					"" , // registered
					0 , // block_hour
					0 , // left_count
					0 , // limit_type
					"" , // reqtm
					"" , // second_pw
					"" , // reason
					"" , // result
					"" , // hash
					game_version , // ver
					std::to_string( _passwd_retry_count ) ); // etc

				log_result.wait();

				QueryManager::UpdateMADEPasswordRetryCount( account_guid , platform_guid , _passwd_retry_count );
				return;
			}

			// 약관 동의 몽땅 FALSE 면 약관 동의 에러 메시지 보내줌
			if ( !_game_play_agree && !_personal_info_agree && !_advertise_push_agree && !_night_advertise_push_agree ) {
				SendMessageAndLogWrite( General::ResultCode::Result_TermsConsentRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있으면 입력이 필요하다는걸 알려 준다.
			if ( ( false == _sub_password.empty() && _sub_password.size() > 0 ) && input_sub_password.empty() ) {
				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}
				else
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "222222222222222222222222222222" );
			//Sleep( 100 );

			// 2 차 비밀번호가 설정되어 있는 경우 패스워드 비교
			if ( false == _sub_password.empty() && _sub_password.size() > 0 ) {

				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				// 입력받은 2차 비밀번호 해슁처리
				string hashedPassword = MADEPlatform::GenerateSha256( input_sub_password );
				if ( false == _sub_password._Equal( hashedPassword ) ) {

					++_sub_passwd_retry_count;
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

					// 2차 비밀번호가 틀렸습니다.
					if ( _sub_passwd_retry_count < 5 )
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}
					else
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}

					QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , _sub_passwd_retry_count );

					//10502 작성
					int code = 10502;

					int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
					//std::string ipString = ::ConvertIP( pContext->GetIP() );

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						device_info , // request.dev_meta() , // device
						"" , // gmsessid
						"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						os_version , // request.os_ver() , // osver
						"" , // marketString , // pf
						"" , // svcuid
						account_guid , // accountid
						0 , // chip
						0 , // chip_g
						0 , // chip_s
						0 , // coin
						0 , // coin_g
						0 , // coin_s
						0 , // slotcoin
						0 , // gem
						0 , // gem_f
						0 , // gem_p
						0 , // kickoutticket
						0 , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						" " , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						game_version , // ver
						std::to_string( subpasswd_retrycnt ) ); // etc

					log_result.wait();
					return;
				}
			}

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "3333333333333333333333333333333333" );
			//Sleep( 100 );

			// 로그인에 성공한 경우 이므로 패스워드 리트라이 카운트를 초기화 해준다.
			if ( _passwd_retry_count > 0 )
				QueryManager::UpdateMADEPasswordRetryCount( account_guid , platform_guid , 0 );

			// 2차 비밀번호가 있고 로그인에 성공한 경우 패스워드 리트라이 카운트 초기화
			if ( false == _sub_password.empty() && _sub_passwd_retry_count > 0 )
				QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , 0 );
		}
		else
		{
			_platform_code = static_cast< BYTE >( General::AccessChannelType::AccessChannel_LocalAccount );
			int _sub_passwd_retry_count = 0;
			std::string _sub_password;
			BOOL _game_play_agree = FALSE;
			BOOL _personal_info_agree = FALSE;
			BOOL _advertise_push_agree = FALSE;
			BOOL _night_advertise_push_agree = FALSE;
			std::future<BOOL> MADE_result = QueryManager::FindPlatformAsync( account_guid , platform_guid , _platform_authcode , _sub_password , _sub_passwd_retry_count ,
				_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

			MADE_result.wait();


if ( FALSE == MADE_result.get() ) {

				// 플랫폼 찾기 실패
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			response.set_alert_optin( _advertise_push_agree );
			response.set_night_alert_optin( _night_advertise_push_agree );

			if ( account_guid.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있으면 입력이 필요하다는걸 알려 준다.
			if ( ( false == _sub_password.empty() && _sub_password.size() > 0 ) && input_sub_password.empty() ) {
				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}
				else
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있는 경우 패스워드 비교
			if ( false == _sub_password.empty() && _sub_password.size() > 0 ) {

				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				// 입력받은 2차 비밀번호 해슁처리
				string hashedPassword = MADEPlatform::GenerateSha256( input_sub_password );
				if ( false == _sub_password._Equal( hashedPassword ) ) {

					++_sub_passwd_retry_count;
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

					// 2차 비밀번호가 틀렸습니다.
					if ( _sub_passwd_retry_count < 5 )
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}
					else
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}

					QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , _sub_passwd_retry_count );

					//10502 작성
					int code = 10502;

					int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
					//std::string ipString = ::ConvertIP( pContext->GetIP() );w

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						device_info , // request.dev_meta() , // device
						"" , // gmsessid
						"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						os_version , // request.os_ver() , // osver
						"" , // marketString , // pf
						"" , // svcuid
						account_guid , // accountid
						0 , // chip
						0 , // chip_g
						0 , // chip_s
						0 , // coin
						0 , // coin_g
						0 , // coin_s
						0 , // slotcoin
						0 , // gem
						0 , // gem_f
						0 , // gem_p
						0 , // kickoutticket
						0 , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						" " , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						game_version , // ver
						std::to_string( subpasswd_retrycnt ) ); // etc

					log_result.wait();
					return;
				}
			}

			// 약관 동의 몽땅 FALSE 면 약관 동의 에러 메시지 보내줌
			if ( !_game_play_agree && !_personal_info_agree && !_advertise_push_agree && !_night_advertise_push_agree ) {
				SendMessageAndLogWrite( General::ResultCode::Result_TermsConsentRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있으면 입력이 필요하다는걸 알려 준다.
			if ( ( false == _sub_password.empty() && _sub_password.size() > 0 ) && input_sub_password.empty() ) {
				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}
				else
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있는 경우 패스워드 비교
			if ( false == _sub_password.empty() ) {

				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				// 입력받은 2차 비밀번호 해슁처리
				string hashedPassword = MADEPlatform::GenerateSha256( input_sub_password );
				if ( false == _sub_password._Equal( hashedPassword ) ) {

					++_sub_passwd_retry_count;
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

					// 2차 비밀번호가 틀렸습니다.
					if ( _sub_passwd_retry_count < 5 )
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}
					else
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}

					QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , _sub_passwd_retry_count );

					//10502 작성
					int code = 10502;

					int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
					//std::string ipString = ::ConvertIP( pContext->GetIP() );

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						device_info , // request.dev_meta() , // device
						"" , // gmsessid
						"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						os_version , // request.os_ver() , // osver
						"" , // marketString , // pf
						"" , // svcuid
						account_guid , // accountid
						0 , // chip
						0 , // chip_g
						0 , // chip_s
						0 , // coin
						0 , // coin_g
						0 , // coin_s
						0 , // slotcoin
						0 , // gem
						0 , // gem_f
						0 , // gem_p
						0 , // kickoutticket
						0 , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						" " , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						game_version , // ver
						std::to_string( subpasswd_retrycnt ) ); // etc

					log_result.wait();
					return;
				}
			}

			// 2차 비밀번호가 있고 로그인에 성공한 경우 패스워드 리트라이 카운트 초기화
			if ( false == _sub_password.empty() && _sub_passwd_retry_count > 0 )
				QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , 0 );

		}

		if ( account_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( platform_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelGuidMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		// 업데이트 푸쉬토큰(user_platform)
		if ( request.alert_token() != "" )
		{
			std::future<BOOL> token_result = QueryManager::UpdatePushToken( platform_guid , request.store_kind() , request.alert_token() );
			token_result.wait();

			if ( FALSE == token_result.get() ) {
				//SendMessageAndLogWrite( General::ResultCode::Result_RestrictionLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

		}

		// 탈퇴예정인 계정 또는 탈퇴처리된 계정이면 접속 불가
		if ( !_expected_withdrawal_date.empty() ) {

			// 기간 확인
			std::time_t _withdrawal_date = TimeUtils::StringToTimeTM( _expected_withdrawal_date );
			std::time_t now = std::time( nullptr );

			// 탈퇴 기간 만료후에는 접속 불가
			if ( now >= _withdrawal_date ) {
				QueryManager::DeleteAccountAsync( platform_guid );
				SendMessageAndLogWrite( General::ResultCode::Result_ClosedAccountBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			else {
				// 탈퇴 예정일을 클라이언트에게 전송
				response.set_quit_planned_ts( _expected_withdrawal_date );
				SendMessageAndLogWrite( General::ResultCode::Result_AccountClosureCancelAvailable , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "44444444444444444444444444444444444444444" );
		//Sleep( 100 );

		// 제재 처리용 플랫폼 uid 클라에게 전달
		response.set_outlet_uid( platform_guid );

		// Account 제재 기간 확인
		std::string sanction_period_start;
		std::string sanction_period_end;
		std::string sanction_reason;
		std::future<BOOL> sanction_result = QueryManager::SelectAccountSanctionAsync( account_guid , sanction_period_start , sanction_period_end , sanction_reason );
		sanction_result.wait();

		if ( FALSE == sanction_result.get() ) {
			//SendMessageAndLogWrite( General::ResultCode::Result_RestrictionLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		else {

			std::time_t end_time = TimeUtils::StringToTimeTM( sanction_period_end );
			std::time_t now = std::time( nullptr );

			// 제재 기간 내라면 실패 메시지 보내준다.
			if ( sanction_period_end.size() > 0 && now < end_time ) {

				response.set_block_begin( sanction_period_start );
				response.set_block_end( sanction_period_end );
				response.set_block_cause( sanction_reason );
				SendMessageAndLogWrite( General::ResultCode::Result_AccessRestrictionActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}

		sanction_period_start.clear();
		sanction_period_end.clear();
		sanction_reason.clear();
		sanction_result = QueryManager::SelectPlatformSanctionAsync( platform_guid , sanction_period_start , sanction_period_end , sanction_reason );
		sanction_result.wait();

		if ( FALSE == sanction_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_RestrictionLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		else {

			std::time_t end_time = TimeUtils::StringToTimeTM( sanction_period_end );
			std::time_t now = std::time( nullptr );

			// 제재 기간 내라면 실패 메시지 보내준다.
			if ( sanction_period_end.size() > 0 && now < end_time ) {

				response.set_block_begin( sanction_period_start );
				response.set_block_end( sanction_period_end );
				response.set_block_cause( sanction_reason );
				SendMessageAndLogWrite( General::ResultCode::Result_AccessRestrictionActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "5555555555555555555555555555555555555555" );
		//Sleep( 100 );

		// 본인 인증 만료 시간 확인
		// 1년이 넘었으면 로그인 오류 발생
		auto ci_expiry_result = QueryManager::FindCiExpiryTimeAsync( account_guid );
		ci_expiry_result.wait();

		std::string ci_expiry_time_string = ci_expiry_result.get();
		std::time_t _ci_expiry_time = TimeUtils::StringToTimeTM( ci_expiry_time_string );
		std::time_t t_ci_expiry_time = TimeUtils::SelectTimeStartTimeTM( _ci_expiry_time );

		std::time_t now = std::time( nullptr );
		if ( now > t_ci_expiry_time ) {
			SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthExpired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// Account 정보 부터 가져 온다.
		std::future<General::LossLimitProfile> lostLimitResult = QueryManager::GetLostLimitAsync( account_guid );
		lostLimitResult.wait();

		General::LossLimitProfile _lost_limit = lostLimitResult.get();
		if ( _lost_limit.account_id() == 0 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "6666666666666666666666666666666666" );
		//Sleep( 100 );

		// 계정 인덱스 설정
		const uint64 account_idx = _lost_limit.account_id();

		General::ParticipantProfile _player;
		Server::ParticipantProfileInternal _playerExt;

		std::future<BOOL> result = std::async( [account_guid , platform_guid , &_player , &_playerExt]() {
			//QueryManager* queryManager = new QueryManager();
			return QueryManager::GetPlayer( account_guid , platform_guid , _player , _playerExt );
		} );

		result.wait();
		//_player.set_wallet_coins( 100000000 );
		if ( false == result.get() )
		{
			// Player 정보를 읽어 들이지 못했다.
			// 로그인 실패 통보

			// 10802 작성

			int code = 10802;

			//std::string ipString = ::ConvertIP( pContext->GetIP() );

			auto log_result = QueryManager::InsertAccountLog(
				code , // code
				platform_guid , // platform_guid , // uid
				"" , // adminid
				"" , // adid
				"" , // asset
				"" , // cmd
				device_info , // request.dev_meta() , // device
				"" , // gmsessid
				"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
				"" , // ipaddr
				"" , // request.os_ver() , // osver
				"" , // marketString , // pf
				"" , // svcuid
				made_id.size() != 0 ? made_id : _platform_authcode , // accountid
				0 , // chip
				0 , // chip_g
				0 , // chip_s
				0 , // coin
				0 , // coin_g
				0 , // coin_s
				0 , // slotcoin
				0 , // gem
				0 , // gem_f
				0 , // gem_p
				0 , // kickoutticket
				0 , // exp
				0 , // friend_cnt
				"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				"" , // logintype
				"" , // type
				" " , // nickname
				"" , // deletetime
				"" , // registered
				0 , // block_hour
				0 , // left_count
				0 , // limit_type
				"" , // reqtm
				"" , // second_pw
				"" , // reason
				"" , // result
				"" , // hash
				game_version , // ver
				"" ); // etc

			log_result.wait();

			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ Login ] Player loading failed." );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

			// 접속을 종료시킨다.
			NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
			pCommandQueueManager->PushCommand( static_cast< NetLib::cIocpContext* >( pContext ) , CSNet::ProtocolCommand::SYS_NET_DISCONNECT );
			return;
		}

		// 플레이어에 ci 만료기한 세팅하기
		_player.set_identity_check_expires_at( TimeUtils::TMToString( t_ci_expiry_time ) );

		// 5개월 체크
		std::string daily_string = _player.daily_reset_at();
		std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( daily_string );
		std::time_t today_start_time = TimeUtils::TodayStartTimeTM();
		int month = 5;
		std::string s_month = NetLib::cSingleton<cDataLoader>::GetInstance()->GetSystemData( "DailyExpiredMonth" );
		if ( s_month != "" )
			month = std::stoi( s_month );
		else
			month = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDailyExpiredMonth();

		if ( TimeUtils::IsMonthsApart( cur_refresh_time , today_start_time , month ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthExpired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		std::string ip_string = ::ConvertIP( pContext->GetIP() );
		/*std::string ip_string = "";
		auto result_ip  = QueryManager::GetPlayerIPAsync( _player.member_id() , ip_string );
		result_ip.wait();
		if ( false == result_ip.get() )
		{

		}
		if ( ip_string == "" )
		{
			auto t_ip = ::ConvertIP( pContext->GetIP() );
			auto t_result_ip = QueryManager::InsertPlayerIPLog( _player.member_id() , t_ip );
			t_result_ip.wait();
			if ( false == t_result_ip.get() )
			{

			}
			ip_string = t_ip;
		}*/


		auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
		std::string marketString = valueDescriptor->name();

		// 테이블 게임 플레이 중이었으면 해당 서버로 보내준다.
		auto myInfo = NetLib::cSingleton<cFriendManager>::GetInstance()->GetFriendInfo( _player.member_id() );
		if ( myInfo != nullptr ) {

			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			// channel id 가 존재하고 서버아이디가 다르면 해당 서버로 보내준다.
			/*if ( myInfo->playing_channel_id().size() &&
				myInfo->server_id() != 0 &&
				myInfo->server_id() != configReader->SID_FOR_MANAGE ) {*/

				// 마지막에 접속했던 서버로 보내 줍니다.
			if ( myInfo->node_id() != 0 &&
				myInfo->node_id() != configReader->SID_FOR_MANAGE ) {
				auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , myInfo->node_id() );
				if ( server_info == nullptr ) {
					SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				int code = 0;

				/*
				- 로그인 타입 : 로그인 접속 형태를 출력
				JOIN : 최초 가입 후 접속 시
				F-LOGIN : 당일 최초 로그인 시
				LOGIN : 당일 재접속 시
				RESTORE : 탈퇴 취소 후 접속 시
				*/

				std::string loginTypeString;

				if ( response.first_signin() ) {
					code = 10101;
					loginTypeString = "JOIN";
				}
				else {

					std::string daily_string = _player.daily_reset_at();
					std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( daily_string );
					std::time_t today_start_time = TimeUtils::TodayStartTimeTM();
					if ( cur_refresh_time <= today_start_time )
						code = 10101;
					else
						code = 10102;

					// 탈퇴 철회 후 첫 로그인
					if ( _cancle_withdrawal_first_join == 1 )
					{
						loginTypeString = "RESTORE";
						QueryManager::PlatformCancleWithdrawalFirstJoin( account_guid , platform_guid , 0 );
					}
					else
					{
						loginTypeString = "LOGIN";
					}
				}

				////std::string ipString = ::ConvertIP( pContext->GetIP() );
				std::string friendCount = QueryManager::GetFriendCount( _player.member_id() );

				string joinTime = "";
				std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( request.profile_uid() , request.outlet_uid() , joinTime );
				player_join_time.wait();

				if ( FALSE == player_join_time.get() ) {

				}

				int platform_code = 4;
				std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( account_guid , platform_guid , platform_code );
				platform_code_result.wait();

				if ( FALSE == platform_code_result.get() ) {
					platform_code = 4;
				}
				auto platformDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( platform_code );
				std::string platformString = platformDescriptor->name();

				auto log_result = QueryManager::InsertAccountLog(
					   code , // code
					   platform_guid , // uid
					   "" , // adminid
					   "" , // adid
					   "" , // asset
					   "" , // cmd
					   request.dev_meta() , // device
					   "" , // gmsessid
					   marketString , // idpcode
					   ip_string , // ipaddr
					   request.os_ver() , // osver
					   platformString , // pf
					   "" , // svcuid
					   "" , // accountid
					   _player.wallet_chips() + _player.vault_chips() , // chip
					   _player.wallet_chips() , // chip_g
					   _player.vault_chips() , // chip_s
					   _player.wallet_coins() + _player.vault_coins() , // coin
					   _player.wallet_coins() , // coin_g
					   _player.vault_coins() , // coin_s
					   _playerExt.remaining_reel_coin() , // slotcoin
					   _player.wallet_gems() + _player.paid_gems() , // gem
					   _player.wallet_gems() , // gem_f
					   _player.paid_gems() , // gem_p
					   _player.kick_ticket_balance() , // kickoutticket
					   _player.experience_points() , // exp
					   std::stoi( friendCount ) , // friend_cnt
					   joinTime , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
					   loginTypeString , // logintype
					   "" , // type
					   _player.display_name() , // nickname
					   "" , // deletetime
					   "" , // registered
					   0 , // block_hour
					   0 , // left_count
					   0 , // limit_type
					   "" , // reqtm
					   input_sub_password.empty() ? "" : "PASSED" , // second_pw
					   "" , // reason
					   "" , // result
					   "" , // hash
					   request.app_ver() , // ver
					   "" ); // etc

				log_result.wait();

				//GMsg_MoveLobby
				PmNet::ShiftAtriumRS* _move_response = response.mutable_shift_atrium_res();
				_move_response->set_atrium_node_ip( server_info->publicIpAddress );
				_move_response->set_endpoint( server_info->nPort );
				_move_response->set_profile_uid( account_guid );
				_move_response->set_outlet_uid( platform_guid );
				GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
				//GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );
				return;
			}
		}
		// 중복 접속 확인 처리 NEW
		NetLib::cSession* connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( _player.member_id() ) );
		if ( connectedClientSession == nullptr )
		{
			connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPending( static_cast< int64 >( _player.member_id() ) );
			if ( connectedClientSession != nullptr && connectedClientSession->GetContext() != nullptr )
			{
				PmNet::DupTunnelRS dupResponse;
				//dupMp._crc = 0;  // 원본 protobuf 의 _crc 와 동일 (decoy)
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
				GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , dupResponse , General::ResultCode::Result_Success , errorString );
			}
		}
		else
		{
			if ( connectedClientSession->GetContext() != nullptr )
			{
				PmNet::DupTunnelRS dupResponse;
				//dupMp._crc = 0;  // 원본 protobuf 의 _crc 와 동일 (decoy)
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
				GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , dupResponse , General::ResultCode::Result_Success , errorString );
			}
		}

		//// 중복 접속 확인 처리
		//bool is_duplicate = false;
		//auto connectedClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( _player.member_id() ) ) );
		//if ( connectedClientSession != nullptr ) {
		//	//is_duplicate = true;
		//	// Context 가 있으면 디스커넥트 처리
		//	if ( connectedClientSession->GetContext() != nullptr ) {
		//		auto contextSession = static_cast< cClientSession* >( connectedClientSession->GetContext()->GetSession() );
		//		if ( contextSession == connectedClientSession ) {
		//			connectedClientSession->SetRoomOutReason( "" );
		//			const uint64& roomNumber = connectedClientSession->GetJoinedRoomNumber();
		//			const uint64& playerIdx = connectedClientSession->GetPlayerIdx();

		//			if ( roomNumber != 0 ) {

		//				cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
		//				if ( pGameRoom != nullptr ) {

		//					// 관전자와 플레이중인 사람으로 나눈다.
		//					if ( pGameRoom->isWatcher( playerIdx ) ) {

		//						// 관전자는 즉시 내보낸다.
		//						NetLib::cSingleton<cGameRoomManager>::GetInstance()->RemoveWatcherResource( connectedClientSession );

		//						// 이유저의 접속을 끊어낸다.
		//						PmNet::DupTunnelRS duplicatedSessionRes;

		//						std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		//						GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
		//						connectedClientSession->GetContext()->Disconnect();

		//						// 세션 삭제 및 풀러로 이동
		//						NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( playerIdx );

		//						connectedClientSession->RoomOutReset();
		//						NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::Login Duplicated Session pGameRoom->isWatcher Success. PlayerIdx [ %llu ]" , playerIdx );
		//					}
		//					else {

		//						// 플레이중인 방의 상태에 따라 처리한다.

		//						if ( General::RoomState::RoomState_Waiting == pGameRoom->GetRoomStatus() ) {

		//							// 방이 Wait 상태이다. 즉시 내보낸다.
		//							if ( NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( connectedClientSession ) ) {

		//								PmNet::DupTunnelRS duplicatedSessionRes;

		//								std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		//								GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
		//								connectedClientSession->GetContext()->Disconnect();

		//								NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( playerIdx );

		//								connectedClientSession->RoomOutReset();

		//								NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::Login Duplicated Session UserGameRoomOut Success. PlayerIdx [ %llu ]" , playerIdx );
		//							}
		//							else {

		//								// 플레이 중인 유저이다.
		//								// 플레이 중인 유저의 세션만 끊어 낸다.
		//								PmNet::DupTunnelRS duplicatedSessionRes;

		//								std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		//								GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
		//								connectedClientSession->GetContext()->Disconnect();

		//								NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::Login Duplicated Session UserGameRoomOut Failed. Room Playing Status PlayerIdx [ %llu ]" , playerIdx );
		//							}
		//						}
		//						else {
		//							/*GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_DuplicateAccessBlocked , "error" );
		//							return;*/
		//							PmNet::DupTunnelRS duplicatedSessionRes;

		//							std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		//							GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
		//							//connectedClientSession->GetContext()->Disconnect();

		//						}
		//					}
		//				}
		//			}
		//			else {

		//				// 접속을 끊어낸다.
		//				PmNet::DupTunnelRS duplicatedSessionRes;

		//				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		//				GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
		//				connectedClientSession->GetContext()->Disconnect();

		//				// 세션 삭제 및 풀러로 이동
		//				NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( playerIdx );

		//				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::Login Duplicated Session roomNumber == 0 Success. PlayerIdx [ %llu ]" , playerIdx );
		//			}
		//		}
		//	}
		//}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "77777777777777777777777777777777777" );
		//Sleep( 100 );

		std::map<int , General::AvatarProfile> _avatarsMap;
		const uint64& playerIdx = _player.member_id();

		result = std::async( [playerIdx , &_avatarsMap]() {
			return QueryManager::AvatarsGet( playerIdx , _avatarsMap );
		} );

		result.wait();

		if ( false == result.get() )
		{
			// Player 정보를 읽어 들이지 못했다.
			// 로그인 실패 통보

			// 10802 작성

			int code = 10802;

			//std::string ipString = ::ConvertIP( pContext->GetIP() );

			auto log_result = QueryManager::InsertAccountLog(
				code , // code
				platform_guid , // platform_guid , // uid
				"" , // adminid
				"" , // adid
				"" , // asset
				"" , // cmd
				device_info , // request.dev_meta() , // device
				"" , // gmsessid
				"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
				ip_string , // ipaddr
				"" , // request.os_ver() , // osver
				"" , // marketString , // pf
				"" , // svcuid
				made_id.size() != 0 ? made_id : _platform_authcode , // accountid
				0 , // chip
				0 , // chip_g
				0 , // chip_s
				0 , // coin
				0 , // coin_g
				0 , // coin_s
				0 , // slotcoin
				0 , // gem
				0 , // gem_f
				0 , // gem_p
				0 , // kickoutticket
				0 , // exp
				0 , // friend_cnt
				"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				"" , // logintype
				"" , // type
				" " , // nickname
				"" , // deletetime
				"" , // registered
				0 , // block_hour
				0 , // left_count
				0 , // limit_type
				"" , // reqtm
				"" , // second_pw
				"" , // reason
				"" , // result
				"" , // hash
				game_version , // ver
				"" ); // etc

			log_result.wait();

			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ Login ] Player avatar loading failed." );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

			// 접속을 종료시킨다.
			NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
			pCommandQueueManager->PushCommand( static_cast< NetLib::cIocpContext* >( pContext ) , CSNet::ProtocolCommand::SYS_NET_DISCONNECT );
			return;
		}
		else
		{
			// 아바타 목록이 동일하지 않으면 아바타를 생성한다.
			std::vector<std::future<BOOL>> results;
			const auto& avatars = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatars();
			if ( _avatarsMap.size() != avatars.size() ) {

				for ( auto& avatar : avatars ) {

					auto iter = _avatarsMap.find( avatar.avatar_ref_id() );
					if ( iter == _avatarsMap.end() ) {

						// 아바타가 없다. 생성한다.
						uint64 _avatar_idx;

						// free 인 경우는 99년 까지 착용하도록
						std::string _expiry_date;
						if ( avatar.free_available() )
							_expiry_date = "2099-12-31 23:59:59";
						else
							_expiry_date = TimeUtils::GetCurrentDateTime();

						results.push_back( QueryManager::InsertAvatarAsync( playerIdx , avatar.avatar_ref_id() , _expiry_date , _avatar_idx ) );
					}
				}

				for ( auto& result : results )
					result.wait();

				// 아바타 재로딩
				_avatarsMap.clear();

				result = std::async( [playerIdx , &_avatarsMap]() {
					return QueryManager::AvatarsGet( playerIdx , _avatarsMap );
				} );

				result.wait();

				if ( FALSE == result.get() ) {

				}
			}

			// 아바타 데이터를 채워 넣는다.
			for ( auto& avatarPair : _avatarsMap ) {
				auto& _avatar = avatarPair.second;
				const auto& avatar_data = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatar( _avatar.avatar_ref_id() );
				_avatar.set_free_available( avatar_data.free_available() );
				//avatar.set_avatar_label( avatar_data.avatar_label() );
			}
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "88888888888888888888888888888888" );
		//Sleep( 100 );

		// QATool
#ifdef _DEBUG
		auto iterCoin = cClientSession::m_qa_player_coins.find( _player.member_id() );
		if ( iterCoin != cClientSession::m_qa_player_coins.end() ) {
			_player.set_wallet_coins( iterCoin->second );
		}

		auto iterChip = cClientSession::m_qa_player_chips.find( _player.member_id() );
		if ( iterChip != cClientSession::m_qa_player_chips.end() ) {
			_player.set_wallet_chips( iterChip->second );
		}
#endif


for ( const auto& avatarPair : _avatarsMap ) {
			auto& _avatar = avatarPair.second;
			General::AvatarProfile* addAvatar = response.add_skins();
			addAvatar->CopyFrom( _avatar );
		}

		PmNet::Ledger _records;
		int _reads = 0;

		const uint64 _player_idx = _player.member_id();

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "9999999999999999999999999999" );
		//Sleep( 100 );

		result = std::async( [_player_idx , &_reads , &_records]() {
			return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
		} );

		result.wait();

		if ( FALSE == result.get() ) {

		}

		// 기본 데이터를 만든다.
		if ( _reads == 0 ) {

			// Async 코드
			std::future<BOOL> createRecordsResult = QueryManager::CreateRecordsAsync( _player_idx );

			createRecordsResult.wait();

			if ( false == createRecordsResult.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			result = std::async( [_player_idx , &_reads , &_records]() {
				//QueryManager* queryManager = new QueryManager();
				return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
			} );

			result.wait();

			if ( false == result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}
		//else if ( _reads == 5 )
		//{
		//	std::future<BOOL> createRecordsResult = QueryManager::CreatePinballRecordsAsync( _player_idx );

		//	createRecordsResult.wait();

		//	if ( false == createRecordsResult.get() ) {
		//		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}

		//	result = std::async( [_player_idx , &_reads , &_records]() {
		//		//QueryManager* queryManager = new QueryManager();
		//		return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
		//	} );

		//	result.wait();

		//	if ( false == result.get() ) {
		//		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}
		//}

		//if ( _reads == 6)
		//{
		//	std::future<BOOL> createRecordsResult = QueryManager::CreateRouletteRecordsAsync( _player_idx );

		//	createRecordsResult.wait();

		//	if ( false == createRecordsResult.get() ) {
		//		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}

		//	result = std::async( [_player_idx , &_reads , &_records]() {
		//		//QueryManager* queryManager = new QueryManager();
		//		return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
		//	} );

		//	result.wait();

		//	if ( false == result.get() ) {
		//		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}
		//}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "1010101010101010101010101010" );
		//Sleep( 100 );

		//BOOL QueryManager::PlayerGetJokboRecords( const uint64& player_idx , int& reads , PmNet::Ledger& records )

		int _jokbo_reads = 0;
		result = std::async( [_player_idx , &_jokbo_reads , &_records]() {
			return QueryManager::PlayerGetJokboRecords( _player_idx , _jokbo_reads , _records );
		} );

		result.wait();

		if ( FALSE == result.get() ) {

		}

		// 기본 데이터를 만든다.
		if ( _jokbo_reads == 0 ) {

			// 내부에서 한방에 처리하고 리턴한다.
			if ( QueryManager::CreateJokboRecords( _player_idx ) ) {

				result = std::async( [_player_idx , &_jokbo_reads , &_records]() {
					return QueryManager::PlayerGetJokboRecords( _player_idx , _jokbo_reads , _records );
				} );

				result.wait();

				if ( FALSE == result.get() ) {

				}
			}
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "11-11-11-11-11" );
		//Sleep( 100 );

		int _quest_reads = 0;
		std::map<uint32 , General::TaskProgress> missions;
		std::map<uint32 , General::TaskProgress> lounges;
		std::map<uint32 , General::TaskProgress> achieves;
		//QueryManager::PlayerGetQuests( _player_idx , _quest_reads , missions , lounges , achieves );
		result = std::async( [_player_idx , &_quest_reads , &missions , &lounges , &achieves]() {
			return QueryManager::PlayerGetQuests( _player_idx , _quest_reads , missions , lounges , achieves );
		} );

		result.wait();

		if ( FALSE == result.get() ) {

		}

		if ( _quest_reads == 0 ) {
			QueryManager::CreateQuests( _player_idx );
			//QueryManager::PlayerGetQuests( _player_idx , _quest_reads , missions , lounges , achieves );

			result = std::async( [_player_idx , &_quest_reads , &missions , &lounges , &achieves]() {
				return QueryManager::PlayerGetQuests( _player_idx , _quest_reads , missions , lounges , achieves );
			} );

			result.wait();

			if ( FALSE == result.get() ) {

			}
		}

		PmNet::Ledger* responseRecords = response.mutable_ledger();
		responseRecords->CopyFrom( _records );

		// 세션 등록
		NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
		if ( !pServerManager )
			return;

		// 내부에서 Session을 찾으면 파라미터로 넘겨준 Context에게 Session을 셋해줍니다.
		// 파라미터로 넘겨준 Context에게도 찾은 Session을 Set 해줍니다.
		//cClientSession* pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPendingSessionAndSyncContext( _player.member_id() , pContext ) );
		cClientSession* pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetReLoginSessionAndSyncContext( _player.member_id() , pContext ) );
		if ( pClientSession != nullptr ) {

			// Pending 세션을 찾은 경우
			NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr( pClientSession->GetCommandQueueIndex() );

			General::ParticipantProfile* responsePlayer = response.mutable_member_info();
			responsePlayer->CopyFrom( pClientSession->GetPlayer() );
			pClientSession->GetPlayerExtRef().set_ranking_reward_claimed_date( _playerExt.ranking_reward_claimed_date() );
			UINT roomNumber = pClientSession->GetJoinedRoomNumber();
			if ( roomNumber != 0 ) {
				cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
				if ( !pGameRoom || !pGameRoom->GetGameInterface() ) {
					pClientSession->SetPlayer( account_idx , _player , _playerExt );
				}
			}
			else {
				pClientSession->SetPlayer( account_idx , _player , _playerExt );
			}

			pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online , "" );

		}
		else {

			// Pending 세션이 없는 경우, Context 와 Session 신규 연결
			pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->AllocateSession( static_cast< int64 >( _player.member_id() ) , pContext ) );
			if ( pClientSession != nullptr ) {

				// Context 와 엮어 줍니다.
				pClientSession->SetGuids( account_guid , platform_guid );
				pClientSession->SetPlayer( account_idx , _player , _playerExt );	// 플레이어 정보
				pClientSession->SetAvatars( _avatarsMap );	// 아바타 목록
				pClientSession->SetRecords( _records );	// 플레이어 전적
				pClientSession->SetMissionAndAchieve( missions , lounges , achieves ); // 미션, 업적
				pClientSession->LoadQuestsOnLogin(); // 누락된 미션 생성
				General::BenefitTier t_class = General::BenefitTier::BenefitTier_None;
				// MemberShipClass Expire처리
				if ( _player.membership_tier() != General::BenefitTier::BenefitTier_Basic )
				{
					if ( pClientSession->MemberShipExpired() ) {
						t_class = _player.membership_tier();
						pClientSession->SetMemberShipClass( General::BenefitTier::BenefitTier_Basic );
						response.set_tier_stale( true );

						// 초과 재화 우편 지급
						pClientSession->SetExpiredCoin( General::PlayCategory::PlayCategory_None , pClientSession->GetCoin() );
						pClientSession->SetExpiredChip( General::PlayCategory::PlayCategory_None );

						pClientSession->PlayerUpdateAsync();
					}
				}

				General::ParticipantProfile* responsePlayer = response.mutable_member_info();
				responsePlayer->CopyFrom( _player );
				if ( response.tier_stale() == true )
					_player.set_membership_tier( t_class );

				// 세션 타입 설정
				pClientSession->SetSessionType( Sessions::SESSION_CLIENT );

				// 친구 관리자에 자신의 정보 등록
				PmNet::MateDetail friendInfo;
				friendInfo.set_mate_state( General::ContactState::ContactState_Online );

				const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
				friendInfo.set_node_id( configReader->SID_FOR_MANAGE );

				// 플레이어 데이터 카피
				auto add_player = friendInfo.mutable_member_info();
				add_player->CopyFrom( _player );

				// 플레이어 전적 데이터 카피
				auto records = friendInfo.mutable_ledger();
				records->CopyFrom( _records );

				NetLib::cSingleton<cFriendManager>::GetInstance()->SetFriendInfo( friendInfo );

				// 로비의 친구 상태 갱신
				{
					const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

					Server::SyncFriendInfoStatus syncFriendInfoStatus;
					syncFriendInfoStatus.set_player_idx( pClientSession->GetPlayerIdx() );
					syncFriendInfoStatus.set_friend_status( General::ContactState::ContactState_Online );
					syncFriendInfoStatus.set_server_id( configReader->SID_FOR_MANAGE );
					syncFriendInfoStatus.set_player_chip( pClientSession->GetChip() );
					syncFriendInfoStatus.set_player_coin( pClientSession->GetCoin() );
					syncFriendInfoStatus.set_player_safe_chip( pClientSession->GetSafeChip() );
					syncFriendInfoStatus.set_player_safe_coin( pClientSession->GetSafeCoin() );
					General::ParticipantProfile* responsePlayer = syncFriendInfoStatus.mutable_player_data();
					responsePlayer->CopyFrom( pClientSession->GetPlayer() );

					NetLib::cSingleton<cFriendManager>::GetInstance()->SyncFriendInfoStatus( syncFriendInfoStatus );

					// 각 로비서버들에게도 쏘아줌
					GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
					pGOOGLE_PROTO_BUFFER->Clear();
					if ( syncFriendInfoStatus.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
						NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::Login SyncFriendInfoStatus SerializeToArray Failed." );
					}

					E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
									E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeContactStateSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncFriendInfoStatus.ByteSizeLong() );

					if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

						// 로비서버 전송 실패
					}
				}

			}
			else {
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ Login ] Login failed with system error" );
				GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
				errorString = protoutil::cProtoUtil::ErrorCodeString( "[ Login ] AllocateSession returns nullptr. need check why session pool is emtpy" );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
				return;
			}
		}
		pClientSession->SoftClear();
		// 슬롯 서버들에게 플레이어 로그인 통지
		// 접속 세션이 있다면 끊어 내라
		{
			Server::SyncPlayerLoginNoti syncPlayerLoginNoti;
			syncPlayerLoginNoti.set_player_idx( pClientSession->GetPlayerIdx() );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( syncPlayerLoginNoti.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::Login SyncPlayerLoginNoti SerializeToArray Failed." );
			}

			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						   E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeAccessNotice , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncPlayerLoginNoti.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

				// 로비서버 전송 실패
			}
		}
		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "13-13-13-13-13" );
		//Sleep( 100 );

		// Server::LostLimit 데이터를 캐슁한다.
		auto add_lost_limit = response.mutable_loss_cap();
		auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( _lost_limit.account_id() );
		if ( lostLimit != nullptr )
		{
			lostLimit->set_monthly_purchase_total( _lost_limit.monthly_purchase_total() );
			pClientSession->SetLostLimit( lostLimit );
			add_lost_limit->CopyFrom( *lostLimit );
		}
		else
		{
			// 아직 캐슁된 데이터가 없다. 캐슁 처리
			auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->SetLostLimit( _lost_limit );
			if ( lostLimit == nullptr )
			{
				// 이 경우라면 로그인을 실패내고 손실한도를 다시 재처리 하자.
				SendMessageAndLogWrite( General::ResultCode::Result_LossLimitLoadFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			add_lost_limit->CopyFrom( *lostLimit );
			pClientSession->SetLostLimit( lostLimit );

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
		}

		// 출석 일수가 0 이면 플레이어 최초의 로그인 이다.
		if ( pClientSession->GetAttendanceDays() == 0 ) {
			pClientSession->AttendanceDay1Reward();
			response.set_first_signin( true );
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "14-14-14-14-14" );
		//Sleep( 100 );

		// 레벨업 테스트
		//pClientSession->UpdateLevel(6);

		//유저로그인이벤트 정보 로드
		//로그인 이벤트 정보 로드
		std::set<string>* receive_list = &pClientSession->receive_login_reward_list;
		receive_list->clear();
		std::future<BOOL> player_event_reward_result = QueryManager::GetLoginRewardByUserIDAsync( pClientSession->GetPlayerIdx() , *receive_list );
		player_event_reward_result.wait();

		if ( FALSE == player_event_reward_result.get() ) {
			//로그필요
			//SendMessageAndLogWrite( General::ResultCode::FindRecievedLoginEventFailed, pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		//이벤트 정보 로드 (캐시에서)
		std::vector<PmNet::DropDetail>* event_list = &pClientSession->login_event_list;
		*event_list = NetLib::cSingleton<cLoginEventManager>::GetInstance()->GetLoginEventList();
		pClientSession->CheckLoginReward();
		//이벤트보상 뿌려주기 클라에서체크
		//CheckLoginReward( pContext );

		// Account Guid 응답에 포함
		// Platform Guid 응답에 포함
		response.set_profile_uid( account_guid );
		response.set_outlet_uid( platform_guid );

		// 메일 로딩
		//QueryManager::CreateTestMails( _player_idx );
		std::vector<PmNet::InboxDetail> mail_list;
		std::future<BOOL> mail_result = QueryManager::MailBoxGetAsync( _player_idx , mail_list );
		mail_result.wait();

		// 추가코드 시작
		// 40101 메시지 삭제 로그 - 기한 만료
		// 기간 만료된 메시지 리스트
		std::vector<uint64> expired_mail_list = QueryManager::GetExpiredMessageList( _player_idx );
		// 기간 만료된 메시지 삭제 로그
		for ( const uint64& mail_idx : expired_mail_list ) {
			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			std::string daily_coin_limit_mail_count_str = "";
			if ( mail_type == "7" ) {
				int daily_coin_limit_mail_count = pClientSession->GetPlayerExt().coin_limit_mail_count();
				daily_coin_limit_mail_count_str = std::to_string( daily_coin_limit_mail_count );
			}

			cMessageLog messageLogInstance( pClientSession , 40101 );
			messageLogInstance.SetData( reg_date , daily_coin_limit_mail_count_str , count );
			messageLogInstance.SetMsgid( std::to_string( mail_idx ) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetReward( reward_type_string );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetEtc( "expire" );
		}
		// 기간 만료된 메시지 삭제
		if ( !expired_mail_list.empty() ) {
			BOOL delete_result = QueryManager::DeleteMailBox( expired_mail_list );
			if ( delete_result == FALSE ) {
				// 삭제 실패
			}
			else {
				// 삭제 성공
			}
		}
		// 추가코드 끝

		if ( FALSE == mail_result.get() ) {

			// 메일박스 받아오기 실패 로그 처리
		}

		if ( mail_list.size() ) {

			pClientSession->SetMailBoxOnLogin( mail_list );

			// 메일 존재 레드닷 처리
			response.set_inbox_pending( true );
		}

		// OS 정보
		pClientSession->SetOSInfo( request.os_ver() );
		pClientSession->SetIp( ip_string );

		// 디바이스 정보
		pClientSession->SetDeviceInfo( request.dev_meta() );

		// 결제 처리용 마켓 처리 추가
		pClientSession->SetMarket( request.store_kind() );

		// 게임 버젼 셋팅
		pClientSession->SetGameVersion( game_version );

		// 라운지 이벤트 체크
		auto lounge_event = response.mutable_atrium_drop();
		NetLib::cSingleton<cLoungeEvent>::GetInstance()->CopyLoungeEvent( lounge_event );

		string currentKstTime = TimeUtils::GetCurrentKSTDateTimeString();
		response.set_svr_ts( currentKstTime );// "2024-12-23 19:00:00"
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );

#ifdef _DEBUG
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( response , &serializedData );
		TraceA( serializedData );
#endif

		// 획득 제한 시간 5일
		//std::u8string room_title = u8"초과금이 지급되었습니다.";
		//std::string_view room_title_utf8View( reinterpret_cast< const char* >( room_title.data() ) , room_title.size() );
		//std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 5 );

		//result = QueryManager::PlayerExecuteQueryAsync( QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin , General::InboxReason::InboxReason_BalanceLimit , 10000 , 0 , room_title_utf8View.data() , getLimitTimeString));
		//result.wait();

		//if ( false == result.get() ) {

		//	// 쿼리 실패에 대한 처리
		//}

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_NOR , "Login Success. playerIdx %llu" , pClientSession->GetPlayerIdx() );

		{
			// 로그인 진입후에 로그인 로그 처리

			auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
			std::string marketString = valueDescriptor->name();

			int code = 0;

			/*
			- 로그인 타입 : 로그인 접속 형태를 출력
			JOIN : 최초 가입 후 접속 시
			F-LOGIN : 당일 최초 로그인 시
			LOGIN : 당일 재접속 시
			RESTORE : 탈퇴 취소 후 접속 시
			*/
			std::string loginTypeString;
			if ( response.first_signin() ) {
				loginTypeString = "JOIN";
				code = 10101;
			}
			else {

				std::string daily_string = _player.daily_reset_at();
				std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( daily_string );
				std::time_t today_start_time = TimeUtils::TodayStartTimeTM();
				if ( cur_refresh_time <= today_start_time )
					code = 10101;
				else
					code = 10102;

				// 탈퇴 철회 후 첫 로그인
				if ( _cancle_withdrawal_first_join == 1 )
				{
					loginTypeString = "RESTORE";
					QueryManager::PlatformCancleWithdrawalFirstJoin( account_guid , platform_guid , 0 );
				}
				else
				{
					loginTypeString = "LOGIN";
				}
			}

			//std::string ipString = ::ConvertIP( pContext->GetIP() );
			std::string friendCount = QueryManager::GetFriendCount( pClientSession->GetPlayerIdx() );

			string joinTime = "";
			std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( account_guid , platform_guid , joinTime );
			player_join_time.wait();

			if ( FALSE == player_join_time.get() ) {

			}

			pClientSession->SetJoinTime( joinTime );

			int platform_code = 4;
			std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( account_guid , platform_guid , platform_code );
			platform_code_result.wait();

			if ( FALSE == platform_code_result.get() ) {
				platform_code = 4;
			}
			auto platformDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( platform_code );
			std::string platformString = platformDescriptor->name();

			auto log_result = QueryManager::InsertAccountLog(
				   code , // code
				   platform_guid , // uid
				   "" , // adminid
				   "" , // adid
				   "" , // asset
				   "" , // cmd
				   request.dev_meta() , // device
				   "" , // gmsessid
				   marketString , // idpcode
				   ip_string , // ipaddr
				   request.os_ver() , // osver
				   platformString , // pf
				   "" , // svcuid
				   "" , // accountid
				   _player.wallet_chips() + _player.vault_chips() , // chip
				   _player.wallet_chips() , // chip_g
				   _player.vault_chips() , // chip_s
				   _player.wallet_coins() + _player.vault_coins() , // coin
				   _player.wallet_coins() , // coin_g
				   _player.vault_coins() , // coin_s
				   _playerExt.remaining_reel_coin() , // slotcoin
				   _player.wallet_gems() + _player.paid_gems() , // gem
				   _player.wallet_gems() , // gem_f
				   _player.paid_gems() , // gem_p
				   _player.kick_ticket_balance() , // kickoutticket
				   _player.experience_points() , // exp
				   std::stoi( friendCount ) , // friend_cnt
				   joinTime , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				   loginTypeString , // logintype
				   "" , // type
				   _player.display_name() , // nickname
				   "" , // deletetime
				   "" , // registered
				   0 , // block_hour
				   0 , // left_count
				   0 , // limit_type
				   "" , // reqtm
				   input_sub_password.empty() ? "" : "PASSED" , // second_pw
				   "" , // reason
				   "" , // result
				   "" , // hash
				   pClientSession->GetGameVersion() , // ver
				   "" ); // etc

			log_result.wait();
		}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "15-15-15-15-15" );
		//Sleep( 100 );
		std::string log = "";
		log = "playerLOGIN : " + std::to_string( _player_idx );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
		// 플레이 중인 방이 있었으면 방으로 난입 시켜 준다.
		//if ( pClientSession->GetPinballGame() != nullptr )
		//{
		//	pClientSession->LeavePinball(false);
		//}
		roomNumber = pClientSession->GetJoinedRoomNumber();
		if ( roomNumber != 0 ) {

			log = "player roomNumber : " + std::to_string( roomNumber );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
			cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
			if ( pGameRoom == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			IGame* pGame = pGameRoom->GetGameInterface();
			if ( pGame == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			/*if ( !pGameRoom->isPlayer( playerIdx ) || pGameRoom->isWatcher( playerIdx ) )
			{
				pGameRoom->RemoveWatcher( playerIdx );
				pClientSession->SetJoinedRoomNumber( 0 );
				pClientSession->GameReset();
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;

			}*/
			const auto& channelData = pGame->GetChannelData();
			log = "player room test1 : " + std::to_string( roomNumber );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
			PmNet::ChamberEnterRS RoomJoinresponse;
			RoomJoinresponse.mutable_reconnect_state()->set_reattach_flag( true );
			RoomJoinresponse.set_ch_token( channelData.id() );
			RoomJoinresponse.mutable_reconnect_state()->set_check_call_flag( pClientSession->isCheck() );
			RoomJoinresponse.set_view_hold_flag( pGame->IsWatcherReservation( pClientSession->GetPlayerIdx() ) );
			switch ( pGame->GetGameType() )
			{
			
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				// 방에 진입 시킨다.
				General::ResultCode errorCode = pGameRoom->RoomReJoin( pClientSession );
				log = "player room test2 : " + std::to_string( roomNumber );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );

				if ( errorCode == General::ResultCode::Result_Success ) {

					pGameRoom->GetGameRoomDetailOnRejoin( RoomJoinresponse , playerIdx );

					std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomReJoin ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

					SendMessageAndLogWrite( General::ResultCode::Result_Success , pContext , nThreadIndex , General::PacketID::Packet_SpaceEnter , RoomJoinresponse );

					pGame->OnIntruding( pClientSession );

					pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , channelData.id() );

				}
				else {
					SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , General::PacketID::Packet_SpaceEnter , RoomJoinresponse ); return;
				}
			}
			break;
			}
		}
	}
	catch ( std::exception& e )
	{
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( request , &serializedData );

		std::string errorString = std::format( "Login Unknown Exception. ErrorInfo = {}, Rejoin RoomNumber = {}, Request = {}" , e.what() , roomNumber , serializedData );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
	catch ( ... )
	{
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( request , &serializedData );

		std::string errorString = std::format( "Login Unknown Exception, Request = {}, Rejoin RoomNumber = {}" , serializedData , roomNumber );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
}

void cProtoMsgStub::ReLogin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{

	PmNet::SigninRQ request;
	PmNet::ResumeSigninRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 로그인 전에 이루어 지는 상황이라 세션이 있으면 세션 에러로 튕겨 낸다.
	if ( HasSession( pContext ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SessionFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string s_ipString = ::ConvertIP( pContext->GetIP() );

	// 로그인 검증 account_guid, platform_guid 로 캐릭터를 로딩한다.
	string account_guid = request.profile_uid();
	string platform_guid = request.outlet_uid();
	string device_info = request.dev_meta();
	string os_version = request.os_ver();
	string game_version = request.app_ver();
	string made_id = request.native_id();
	string MADE_passwd = request.native_key();
	string input_sub_password = request.aux_key();
	string _expected_withdrawal_date;
	int _cancle_withdrawal_first_join = 0;
	string _platform_authcode = "";

	BYTE _platform_code = 0;
	UINT roomNumber = 0;

	try
	{
		// 점검 체크
		if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenance( request.store_kind() , s_ipString , game_version ) ) {

			const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();

			std::string errorString;
			PmNet::ServiceNotice system_message;
			system_message.set_notice( message.message() );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_ServicePaused , errorString );
			pContext->SetSession( nullptr );
			pContext->Disconnect();
			return;
		}

		//Server::MaintenanceMessage message;
		//QueryManager::GetMaintenanceMessage( message );


		// 자체 로그인 이면 패스워드가 맞는지 확인한다.
		if ( made_id.size() != 0 )
		{
			int _passwd_retry_count = 0;
			std::string _find_password;
			int _sub_passwd_retry_count = 0;
			std::string _sub_password;
			BOOL _game_play_agree = FALSE;
			BOOL _personal_info_agree = FALSE;
			BOOL _advertise_push_agree = FALSE;
			BOOL _night_advertise_push_agree = FALSE;

			std::future<BOOL> MADE_result = QueryManager::FindMADEPlatformAsync( made_id , account_guid , platform_guid , _platform_code , _find_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count ,
				_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

			MADE_result.wait();

			if ( FALSE == MADE_result.get() ) {

				// 플랫폼 찾기 실패
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "11111111111111111111111111111111");
			//Sleep( 100 );

			if ( account_guid.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			if ( _passwd_retry_count >= 5 ) {
				response.set_key_miss_cnt( _passwd_retry_count );
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessRetryLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 패스워드가 잘못되었습니다.
			if ( false == MADE_passwd._Equal( _find_password ) ) {
				++_passwd_retry_count;
				response.set_key_miss_cnt( _passwd_retry_count );

				if ( _passwd_retry_count < 5 )
					SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessPasswordMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
				else
					SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessRetryLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );

				// 10802 작성

				int code = 10802;

				//std::string ipString = ::ConvertIP( pContext->GetIP() );

				auto log_result = QueryManager::InsertAccountLog(
					code , // code
					platform_guid , // platform_guid , // uid
					"" , // adminid
					"" , // adid
					"" , // asset
					"" , // cmd
					device_info , // request.dev_meta() , // device
					"" , // gmsessid
					"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
					"" , // ipaddr
					"" , // request.os_ver() , // osver
					"" , // marketString , // pf
					"" , // svcuid
					made_id , // accountid
					0 , // chip
					0 , // chip_g
					0 , // chip_s
					0 , // coin
					0 , // coin_g
					0 , // coin_s
					0 , // slotcoin
					0 , // gem
					0 , // gem_f
					0 , // gem_p
					0 , // kickoutticket
					0 , // exp
					0 , // friend_cnt
					"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
					"" , // logintype
					"" , // type
					" " , // nickname
					"" , // deletetime
					"" , // registered
					0 , // block_hour
					0 , // left_count
					0 , // limit_type
					"" , // reqtm
					"" , // second_pw
					"" , // reason
					"" , // result
					"" , // hash
					game_version , // ver
					std::to_string( _passwd_retry_count ) ); // etc

				log_result.wait();

				QueryManager::UpdateMADEPasswordRetryCount( account_guid , platform_guid , _passwd_retry_count );
				return;
			}

			// 약관 동의 몽땅 FALSE 면 약관 동의 에러 메시지 보내줌
			if ( !_game_play_agree && !_personal_info_agree && !_advertise_push_agree && !_night_advertise_push_agree ) {
				SendMessageAndLogWrite( General::ResultCode::Result_TermsConsentRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있으면 입력이 필요하다는걸 알려 준다.
			if ( ( false == _sub_password.empty() && _sub_password.size() > 0 ) && input_sub_password.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "222222222222222222222222222222" );
			//Sleep( 100 );

			// 2 차 비밀번호가 설정되어 있는 경우 패스워드 비교
			if ( false == _sub_password.empty() && _sub_password.size() > 0 ) {

				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				// 입력받은 2차 비밀번호 해슁처리
				string hashedPassword = MADEPlatform::GenerateSha256( input_sub_password );
				if ( false == _sub_password._Equal( hashedPassword ) ) {

					++_sub_passwd_retry_count;
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

					// 2차 비밀번호가 틀렸습니다.
					if ( _sub_passwd_retry_count < 5 )
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}
					else
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}

					QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , _sub_passwd_retry_count );

					//10502 작성
					int code = 10502;

					int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
					//std::string ipString = ::ConvertIP( pContext->GetIP() );

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						device_info , // request.dev_meta() , // device
						"" , // gmsessid
						"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						os_version , // request.os_ver() , // osver
						"" , // marketString , // pf
						"" , // svcuid
						account_guid , // accountid
						0 , // chip
						0 , // chip_g
						0 , // chip_s
						0 , // coin
						0 , // coin_g
						0 , // coin_s
						0 , // slotcoin
						0 , // gem
						0 , // gem_f
						0 , // gem_p
						0 , // kickoutticket
						0 , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						" " , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						game_version , // ver
						std::to_string( subpasswd_retrycnt ) ); // etc

					log_result.wait();
					return;
				}
			}

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "3333333333333333333333333333333333" );
			//Sleep( 100 );

			// 로그인에 성공한 경우 이므로 패스워드 리트라이 카운트를 초기화 해준다.
			if ( _passwd_retry_count > 0 )
				QueryManager::UpdateMADEPasswordRetryCount( account_guid , platform_guid , 0 );

			// 2차 비밀번호가 있고 로그인에 성공한 경우 패스워드 리트라이 카운트 초기화
			if ( false == _sub_password.empty() && _sub_passwd_retry_count > 0 )
				QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , 0 );
		}
		else
		{
			_platform_code = static_cast< BYTE >( General::AccessChannelType::AccessChannel_LocalAccount );
			int _sub_passwd_retry_count = 0;
			std::string _sub_password;
			BOOL _game_play_agree = FALSE;
			BOOL _personal_info_agree = FALSE;
			BOOL _advertise_push_agree = FALSE;
			BOOL _night_advertise_push_agree = FALSE;
			std::future<BOOL> MADE_result = QueryManager::FindPlatformAsync( account_guid , platform_guid , _platform_authcode , _sub_password , _sub_passwd_retry_count ,
				_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

			MADE_result.wait();


if ( FALSE == MADE_result.get() ) {

				// 플랫폼 찾기 실패
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			if ( account_guid.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있으면 입력이 필요하다는걸 알려 준다.
			if ( ( false == _sub_password.empty() && _sub_password.size() > 0 ) && input_sub_password.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있는 경우 패스워드 비교
			if ( false == _sub_password.empty() && _sub_password.size() > 0 ) {

				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				// 입력받은 2차 비밀번호 해슁처리
				string hashedPassword = MADEPlatform::GenerateSha256( input_sub_password );
				if ( false == _sub_password._Equal( hashedPassword ) ) {

					++_sub_passwd_retry_count;
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

					// 2차 비밀번호가 틀렸습니다.
					if ( _sub_passwd_retry_count < 5 )
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}
					else
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}

					QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , _sub_passwd_retry_count );

					//10502 작성
					int code = 10502;

					int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
					//std::string ipString = ::ConvertIP( pContext->GetIP() );

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						device_info , // request.dev_meta() , // device
						"" , // gmsessid
						"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						os_version , // request.os_ver() , // osver
						"" , // marketString , // pf
						"" , // svcuid
						account_guid , // accountid
						0 , // chip
						0 , // chip_g
						0 , // chip_s
						0 , // coin
						0 , // coin_g
						0 , // coin_s
						0 , // slotcoin
						0 , // gem
						0 , // gem_f
						0 , // gem_p
						0 , // kickoutticket
						0 , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						" " , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						game_version , // ver
						std::to_string( subpasswd_retrycnt ) ); // etc

					log_result.wait();
					return;
				}
			}

			// 약관 동의 몽땅 FALSE 면 약관 동의 에러 메시지 보내줌
			if ( !_game_play_agree && !_personal_info_agree && !_advertise_push_agree && !_night_advertise_push_agree ) {
				SendMessageAndLogWrite( General::ResultCode::Result_TermsConsentRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있으면 입력이 필요하다는걸 알려 준다.
			if ( ( false == _sub_password.empty() && _sub_password.size() > 0 ) && input_sub_password.empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 2 차 비밀번호가 설정되어 있는 경우 패스워드 비교
			if ( false == _sub_password.empty() ) {

				if ( _sub_passwd_retry_count >= 5 ) {
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
					SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}

				// 입력받은 2차 비밀번호 해슁처리
				string hashedPassword = MADEPlatform::GenerateSha256( input_sub_password );
				if ( false == _sub_password._Equal( hashedPassword ) ) {

					++_sub_passwd_retry_count;
					response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

					// 2차 비밀번호가 틀렸습니다.
					if ( _sub_passwd_retry_count < 5 )
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}
					else
					{
						SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
					}

					QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , _sub_passwd_retry_count );

					//10502 작성
					int code = 10502;

					int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
					//std::string ipString = ::ConvertIP( pContext->GetIP() );

					auto log_result = QueryManager::InsertAccountLog(
						code , // code
						platform_guid , // platform_guid , // uid
						"" , // adminid
						"" , // adid
						"" , // asset
						"" , // cmd
						device_info , // request.dev_meta() , // device
						"" , // gmsessid
						"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
						"" , // ipaddr
						os_version , // request.os_ver() , // osver
						"" , // marketString , // pf
						"" , // svcuid
						account_guid , // accountid
						0 , // chip
						0 , // chip_g
						0 , // chip_s
						0 , // coin
						0 , // coin_g
						0 , // coin_s
						0 , // slotcoin
						0 , // gem
						0 , // gem_f
						0 , // gem_p
						0 , // kickoutticket
						0 , // exp
						0 , // friend_cnt
						"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
						"" , // logintype
						"" , // type
						" " , // nickname
						"" , // deletetime
						"" , // registered
						0 , // block_hour
						0 , // left_count
						0 , // limit_type
						"" , // reqtm
						"" , // second_pw
						"" , // reason
						"" , // result
						"" , // hash
						game_version , // ver
						std::to_string( subpasswd_retrycnt ) ); // etc

					log_result.wait();
					return;
				}
			}

			// 2차 비밀번호가 있고 로그인에 성공한 경우 패스워드 리트라이 카운트 초기화
			if ( false == _sub_password.empty() && _sub_passwd_retry_count > 0 )
				QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , 0 );

		}

		if ( account_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( platform_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelGuidMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 탈퇴예정인 계정 또는 탈퇴처리된 계정이면 접속 불가
		//if ( !_expected_withdrawal_date.empty() ) {

		//	// 기간 확인
		//	std::time_t _withdrawal_date = TimeUtils::StringToTimeTM( _expected_withdrawal_date );
		//	std::time_t now = std::time( nullptr );

		//	// 탈퇴 기간 만료후에는 접속 불가
		//	if ( now >= _withdrawal_date ) {
		//		SendMessageAndLogWrite( General::ResultCode::Result_ClosedAccountBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}
		//	else {
		//		// 탈퇴 예정일을 클라이언트에게 전송
		//		response.set_quit_planned_ts( _expected_withdrawal_date );
		//		SendMessageAndLogWrite( General::ResultCode::Result_AccountClosureCancelAvailable , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}
		//}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "44444444444444444444444444444444444444444" );
		//Sleep( 100 );

		// 제재 처리용 플랫폼 uid 클라에게 전달
		response.set_outlet_uid( platform_guid );

		// Account 제재 기간 확인
		//std::string sanction_period_start;
		//std::string sanction_period_end;
		//std::string sanction_reason;
		//std::future<BOOL> sanction_result = QueryManager::SelectAccountSanctionAsync( account_guid , sanction_period_start , sanction_period_end , sanction_reason );
		//sanction_result.wait();

		//if ( FALSE == sanction_result.get() ) {
		//	//SendMessageAndLogWrite( General::ResultCode::Result_RestrictionLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}
		//else {

		//	std::time_t end_time = TimeUtils::StringToTimeTM( sanction_period_end );
		//	std::time_t now = std::time( nullptr );

		//	// 제재 기간 내라면 실패 메시지 보내준다.
		//	if ( sanction_period_end.size() > 0 && now < end_time ) {

		//		response.set_block_begin( sanction_period_start );
		//		response.set_block_end( sanction_period_end );
		//		response.set_block_cause( sanction_reason );
		//		SendMessageAndLogWrite( General::ResultCode::Result_AccessRestrictionActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}
		//}

		//sanction_period_start.clear();
		//sanction_period_end.clear();
		//sanction_reason.clear();
		//sanction_result = QueryManager::SelectPlatformSanctionAsync( platform_guid , sanction_period_start , sanction_period_end , sanction_reason );
		//sanction_result.wait();

		//if ( FALSE == sanction_result.get() ) {
		//	SendMessageAndLogWrite( General::ResultCode::Result_RestrictionLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}
		//else {

		//	std::time_t end_time = TimeUtils::StringToTimeTM( sanction_period_end );
		//	std::time_t now = std::time( nullptr );

		//	// 제재 기간 내라면 실패 메시지 보내준다.
		//	if ( sanction_period_end.size() > 0 && now < end_time ) {

		//		response.set_block_begin( sanction_period_start );
		//		response.set_block_end( sanction_period_end );
		//		response.set_block_cause( sanction_reason );
		//		SendMessageAndLogWrite( General::ResultCode::Result_AccessRestrictionActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}
		//}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "5555555555555555555555555555555555555555" );
		//Sleep( 100 );

		// 본인 인증 만료 시간 확인
		// 1년이 넘었으면 로그인 오류 발생
		//auto ci_expiry_result = QueryManager::FindCiExpiryTimeAsync( account_guid );
		//ci_expiry_result.wait();

		//std::string ci_expiry_time_string = ci_expiry_result.get();
		//std::time_t _ci_expiry_time = TimeUtils::StringToTimeTM( ci_expiry_time_string );
		//std::time_t t_ci_expiry_time = TimeUtils::SelectTimeStartTimeTM( _ci_expiry_time );

		//std::time_t now = std::time( nullptr );
		//if ( now > t_ci_expiry_time ) {
		//	SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthExpired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}

		//// Account 정보 부터 가져 온다.
		//std::future<General::LossLimitProfile> lostLimitResult = QueryManager::GetLostLimitAsync( account_guid );
		//lostLimitResult.wait();

		//General::LossLimitProfile _lost_limit = lostLimitResult.get();
		//if ( _lost_limit.account_id() == 0 ) {
		//	SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}

		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "6666666666666666666666666666666666" );
		//Sleep( 100 );

		// 계정 인덱스 설정
		//const uint64 account_idx = _lost_limit.account_id();

		General::ParticipantProfile _player;
		Server::ParticipantProfileInternal _playerExt;

		std::future<BOOL> result = std::async( [account_guid , platform_guid , &_player , &_playerExt]() {
			//QueryManager* queryManager = new QueryManager();
			return QueryManager::GetPlayer( account_guid , platform_guid , _player , _playerExt );
		} );

		result.wait();
		//_player.set_wallet_coins( 100000000 );
		if ( false == result.get() )
		{
			// Player 정보를 읽어 들이지 못했다.
			// 로그인 실패 통보

			// 10802 작성

			int code = 10802;

			//std::string ipString = ::ConvertIP( pContext->GetIP() );

			auto log_result = QueryManager::InsertAccountLog(
				code , // code
				platform_guid , // platform_guid , // uid
				"" , // adminid
				"" , // adid
				"" , // asset
				"" , // cmd
				device_info , // request.dev_meta() , // device
				"" , // gmsessid
				"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
				"" , // ipaddr
				"" , // request.os_ver() , // osver
				"" , // marketString , // pf
				"" , // svcuid
				made_id.size() != 0 ? made_id : _platform_authcode , // accountid
				0 , // chip
				0 , // chip_g
				0 , // chip_s
				0 , // coin
				0 , // coin_g
				0 , // coin_s
				0 , // slotcoin
				0 , // gem
				0 , // gem_f
				0 , // gem_p
				0 , // kickoutticket
				0 , // exp
				0 , // friend_cnt
				"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				"" , // logintype
				"" , // type
				" " , // nickname
				"" , // deletetime
				"" , // registered
				0 , // block_hour
				0 , // left_count
				0 , // limit_type
				"" , // reqtm
				"" , // second_pw
				"" , // reason
				"" , // result
				"" , // hash
				game_version , // ver
				"" ); // etc

			log_result.wait();

			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ Login ] Player loading failed." );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

			// 접속을 종료시킨다.
			NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
			pCommandQueueManager->PushCommand( static_cast< NetLib::cIocpContext* >( pContext ) , CSNet::ProtocolCommand::SYS_NET_DISCONNECT );
			return;
		}
		std::string ip_string = ::ConvertIP( pContext->GetIP() );
		/*std::string ip_string = "";
		auto result_ip  = QueryManager::GetPlayerIPAsync( _player.member_id() , ip_string );
		result_ip.wait();
		if ( false == result_ip.get() )
		{

		}
		if ( ip_string == "" )
		{
			auto t_ip = ::ConvertIP( pContext->GetIP() );
			auto t_result_ip = QueryManager::InsertPlayerIPLog( _player.member_id() , t_ip );
			t_result_ip.wait();
			if ( false == t_result_ip.get() )
			{

			}
			ip_string = t_ip;
		}*/

		// 중복 접속 확인 처리 NEW
		NetLib::cSession* connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( _player.member_id() ) );
		if ( connectedClientSession == nullptr )
		{
			connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPending( static_cast< int64 >( _player.member_id() ) );
			if ( connectedClientSession != nullptr && connectedClientSession->GetContext() != nullptr )
			{
				/*PmNet::DupTunnelRS duplicatedSessionRes;
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );*/

				PmNet::SysFaultRS sysErrorRes;
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "Session Error duplicatedSession" );
				GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_CoreFault , sysErrorRes , General::ResultCode::Result_Success , errorString );
			}
		}
		else
		{
			if ( connectedClientSession->GetContext() != nullptr )
			{
				/*PmNet::DupTunnelRS duplicatedSessionRes;
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );*/
				PmNet::SysFaultRS sysErrorRes;
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "Session Error duplicatedSession" );
				GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_CoreFault , sysErrorRes , General::ResultCode::Result_Success , errorString );
			}
		}

		auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
		std::string marketString = valueDescriptor->name();


		// QATool
#ifdef _DEBUG
		auto iterCoin = cClientSession::m_qa_player_coins.find( _player.member_id() );
		if ( iterCoin != cClientSession::m_qa_player_coins.end() ) {
			_player.set_wallet_coins( iterCoin->second );
		}

		auto iterChip = cClientSession::m_qa_player_chips.find( _player.member_id() );
		if ( iterChip != cClientSession::m_qa_player_chips.end() ) {
			_player.set_wallet_chips( iterChip->second );
		}
#endif
		const uint64 _player_idx = _player.member_id();

		// 세션 등록
		NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
		if ( !pServerManager )
			return;

		// 내부에서 Session을 찾으면 파라미터로 넘겨준 Context에게 Session을 셋해줍니다.
		// 파라미터로 넘겨준 Context에게도 찾은 Session을 Set 해줍니다.
		cClientSession* pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetReLoginSessionAndSyncContext( _player.member_id() , pContext ) );
		if ( pClientSession != nullptr ) {

			// Pending 세션을 찾은 경우
			NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr( pClientSession->GetCommandQueueIndex() );

			General::ParticipantProfile* responsePlayer = response.mutable_member_info();
			responsePlayer->CopyFrom( pClientSession->GetPlayer() );
			pClientSession->GetPlayerExtRef().set_ranking_reward_claimed_date( _playerExt.ranking_reward_claimed_date() );
		}
		else  //팬딩세션을 못찾은경우
		{
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_SessionExpired , errorString );
			return;
		}

		// Account Guid 응답에 포함
		// Platform Guid 응답에 포함
		response.set_profile_uid( account_guid );
		response.set_outlet_uid( platform_guid );

		// OS 정보
		pClientSession->SetOSInfo( request.os_ver() );
		pClientSession->SetIp( ip_string );

		// 디바이스 정보
		pClientSession->SetDeviceInfo( request.dev_meta() );

		// 결제 처리용 마켓 처리 추가
		pClientSession->SetMarket( request.store_kind() );

		// 게임 버젼 셋팅
		pClientSession->SetGameVersion( game_version );

		// 재로그인시 클리어
		pClientSession->SoftClear();

		string currentKstTime = TimeUtils::GetCurrentKSTDateTimeString();
		response.set_svr_ts( currentKstTime );// "2024-12-23 19:00:00"


#ifdef _DEBUG
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( response , &serializedData );
		TraceA( serializedData );
#endif

		// 획득 제한 시간 5일
		//std::u8string room_title = u8"초과금이 지급되었습니다.";
		//std::string_view room_title_utf8View( reinterpret_cast< const char* >( room_title.data() ) , room_title.size() );
		//std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 5 );

		//result = QueryManager::PlayerExecuteQueryAsync( QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin , General::InboxReason::InboxReason_BalanceLimit , 10000 , 0 , room_title_utf8View.data() , getLimitTimeString));
		//result.wait();

		//if ( false == result.get() ) {

		//	// 쿼리 실패에 대한 처리
		//}

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_NOR , "ReLogin Success. playerIdx %llu" , pClientSession->GetPlayerIdx() );

		{
			// 로그인 진입후에 로그인 로그 처리

			auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
			std::string marketString = valueDescriptor->name();

			int code = 0;

			/*
			- 로그인 타입 : 로그인 접속 형태를 출력
			JOIN : 최초 가입 후 접속 시
			F-LOGIN : 당일 최초 로그인 시
			LOGIN : 당일 재접속 시
			RESTORE : 탈퇴 취소 후 접속 시
			*/
			std::string loginTypeString;
			if ( response.first_signin() ) {
				loginTypeString = "JOIN";
				code = 10101;
			}
			else {

				std::string daily_string = _player.daily_reset_at();
				std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( daily_string );
				std::time_t today_start_time = TimeUtils::TodayStartTimeTM();
				if ( cur_refresh_time <= today_start_time )
					code = 10101;
				else
					code = 10102;

				// 탈퇴 철회 후 첫 로그인
				if ( _cancle_withdrawal_first_join == 1 )
				{
					loginTypeString = "RESTORE";
					QueryManager::PlatformCancleWithdrawalFirstJoin( account_guid , platform_guid , 0 );
				}
				else
				{
					loginTypeString = "LOGIN";
				}
			}

			//std::string ipString = ::ConvertIP( pContext->GetIP() );
			std::string friendCount = QueryManager::GetFriendCount( pClientSession->GetPlayerIdx() );

			string joinTime = "";
			std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( account_guid , platform_guid , joinTime );
			player_join_time.wait();

			if ( FALSE == player_join_time.get() ) {

			}

			pClientSession->SetJoinTime( joinTime );

			int platform_code = 4;
			std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( account_guid , platform_guid , platform_code );
			platform_code_result.wait();

			if ( FALSE == platform_code_result.get() ) {
				platform_code = 4;
			}
			auto platformDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( platform_code );
			std::string platformString = platformDescriptor->name();

			auto log_result = QueryManager::InsertAccountLog(
				   code , // code
				   platform_guid , // uid
				   "" , // adminid
				   "" , // adid
				   "" , // asset
				   "" , // cmd
				   request.dev_meta() , // device
				   "" , // gmsessid
				   marketString , // idpcode
				   ip_string , // ipaddr
				   request.os_ver() , // osver
				   platformString , // pf
				   "" , // svcuid
				   "" , // accountid
				   _player.wallet_chips() + _player.vault_chips() , // chip
				   _player.wallet_chips() , // chip_g
				   _player.vault_chips() , // chip_s
				   _player.wallet_coins() + _player.vault_coins() , // coin
				   _player.wallet_coins() , // coin_g
				   _player.vault_coins() , // coin_s
				   _playerExt.remaining_reel_coin() , // slotcoin
				   _player.wallet_gems() + _player.paid_gems() , // gem
				   _player.wallet_gems() , // gem_f
				   _player.paid_gems() , // gem_p
				   _player.kick_ticket_balance() , // kickoutticket
				   _player.experience_points() , // exp
				   std::stoi( friendCount ) , // friend_cnt
				   joinTime , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				   loginTypeString , // logintype
				   "" , // type
				   _player.display_name() , // nickname
				   "" , // deletetime
				   "" , // registered
				   0 , // block_hour
				   0 , // left_count
				   0 , // limit_type
				   "" , // reqtm
				   input_sub_password.empty() ? "" : "PASSED" , // second_pw
				   "" , // reason
				   "" , // result
				   "" , // hash
				   pClientSession->GetGameVersion() , // ver
				   "" ); // etc

			log_result.wait();
		}

		//내보내기
		{
			Server::SyncPlayerLoginNoti syncPlayerLoginNoti;
			syncPlayerLoginNoti.set_player_idx( pClientSession->GetPlayerIdx() );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( syncPlayerLoginNoti.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::Login SyncPlayerLoginNoti SerializeToArray Failed." );
			}

			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						   E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeAccessNotice , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncPlayerLoginNoti.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

				// 로비서버 전송 실패
			}
		}


		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "15-15-15-15-15" );
		//Sleep( 100 );

		// 플레이 중인 방이 있었으면 방으로 난입 시켜 준다.
		roomNumber = pClientSession->GetJoinedRoomNumber();
		if ( roomNumber != 0 ) {

			cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
			if ( pGameRoom == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			IGame* pGame = pGameRoom->GetGameInterface();
			if ( pGame == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			/*if ( !pGameRoom->isPlayer( _player_idx ) || pGameRoom->isWatcher( _player_idx ) )
			{
				pGameRoom->RemoveWatcher( _player_idx );
				pClientSession->SetJoinedRoomNumber( 0 );
				pClientSession->GameReset();
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;

			}*/
			const auto& channelData = pGame->GetChannelData();

			PmNet::ChamberEnterRS RoomJoinresponse;
			RoomJoinresponse.mutable_reconnect_state()->set_reattach_flag( true );
			RoomJoinresponse.set_ch_token( channelData.id() );
			RoomJoinresponse.mutable_reconnect_state()->set_check_call_flag( pClientSession->isCheck() );
			pGameRoom->RoomOutReservation( _player_idx , pClientSession , false , true );
			// 방에 진입 시킨다.
			General::ResultCode errorCode = pGameRoom->RoomReJoin( pClientSession );
			pGameRoom->GetGameRoomDetailOnRejoin( RoomJoinresponse , _player_idx );
			if ( errorCode != General::ResultCode::Result_Success )
			{
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			PmNet::ChamberEnterRS* t_roomjoinres = response.mutable_chamber_enter_res();
			t_roomjoinres->CopyFrom( RoomJoinresponse );
			t_roomjoinres->set_view_hold_flag( pGame->IsWatcherReservation( pClientSession->GetPlayerIdx() ) );

			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
			switch ( pGame->GetGameType() )
			{
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				if ( errorCode == General::ResultCode::Result_Success ) {
					std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomReJoin ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
					pGame->OnIntruding( pClientSession , false , true );
				}
				pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , channelData.id() );
			}
			break;
			}

		}
		else
		{
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
		}


	}
	catch ( std::exception& e )
	{
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( request , &serializedData );

		std::string errorString = std::format( "Login Unknown Exception. ErrorInfo = {}, Rejoin RoomNumber = {}, Request = {}" , e.what() , roomNumber , serializedData );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
	catch ( ... )
	{
		std::string serializedData;
		google::protobuf::util::MessageToJsonString( request , &serializedData );

		std::string errorString = std::format( "Login Unknown Exception, Request = {}, Rejoin RoomNumber = {}" , serializedData , roomNumber );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
}

void cProtoMsgStub::TransferServer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::MigrateNodeRQ request;
	PmNet::MigrateNodeRS response;
	PmNet::SigninRQ loginRequest;

	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 로그인 전에 이루어 지는 상황이라 세션이 있으면 세션 에러로 튕겨 낸다.
	if ( HasSession( pContext ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SessionFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 로그인 검증 account_guid, platform_guid 로 캐릭터를 로딩한다.
	string account_guid = request.profile_uid();
	string platform_guid = request.outlet_uid();
	string game_version = request.app_ver();
	string _platform_authcode = "";
	string made_id = loginRequest.native_id();

#ifdef _DEBUG

	std::string error = std::format( "Tranmsfer version : {} , Market : {} " , game_version , ( int ) request.store_kind() );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , error.c_str() );
#endif
	{
		int _sub_passwd_retry_count = 0;
		std::string _sub_password;
		BOOL _game_play_agree = FALSE;
		BOOL _personal_info_agree = FALSE;
		BOOL _advertise_push_agree = FALSE;
		BOOL _night_advertise_push_agree = FALSE;
		string _expected_withdrawal_date;
		int _cancle_withdrawal_first_join;
		std::future<BOOL> MADE_result = QueryManager::FindPlatformAsync( account_guid , platform_guid , _platform_authcode , _sub_password , _sub_passwd_retry_count ,
			_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

		MADE_result.wait();

		if ( FALSE == MADE_result.get() ) {

			// 플랫폼 찾기 실패
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		response.set_alert_optin( _advertise_push_agree );
		response.set_night_alert_optin( _night_advertise_push_agree );

	}
	// 점검 체크
	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenance( request.store_kind() , ::ConvertIP( pContext->GetIP() ) , game_version ) ) {

		const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();

		std::string errorString;
		PmNet::ServiceNotice system_message;
		system_message.set_notice( message.message() );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_ServicePaused , errorString );
		pContext->SetSession( nullptr );
		pContext->Disconnect();
		return;
	}

	if ( account_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( platform_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ChannelGuidMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// Account 정보 부터 가져 온다.
	std::future<General::LossLimitProfile> lostLimitResult = QueryManager::GetLostLimitAsync( account_guid );
	lostLimitResult.wait();

	General::LossLimitProfile _lost_limit = lostLimitResult.get();
	if ( _lost_limit.account_id() == 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 계정 인덱스 설정
	const uint64 account_idx = _lost_limit.account_id();

	General::ParticipantProfile _player;
	Server::ParticipantProfileInternal _playerExt;

	std::future<BOOL> result = std::async( [account_guid , platform_guid , &_player , &_playerExt]() {
		//QueryManager* queryManager = new QueryManager();
		return QueryManager::GetPlayer( account_guid , platform_guid , _player , _playerExt );
	} );

	result.wait();
	//_player.set_wallet_coins( 100000000 );
	//_player.set_rakeback_balance( 100000000 );
	// 
	// 
		// 본인 인증 만료 시간 확인
		// 1년이 넘었으면 로그인 오류 발생
	auto ci_expiry_result = QueryManager::FindCiExpiryTimeAsync( account_guid );
	ci_expiry_result.wait();

	std::string ci_expiry_time_string = ci_expiry_result.get();
	std::time_t _ci_expiry_time = TimeUtils::StringToTimeTM( ci_expiry_time_string );
	std::time_t t_ci_expiry_time = TimeUtils::SelectTimeStartTimeTM( _ci_expiry_time );

	/*
	std::time_t now = std::time( nullptr );
	if ( now > _ci_expiry_time ) {
		SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthExpired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	*/

	_player.set_identity_check_expires_at( TimeUtils::TMToString( t_ci_expiry_time ) );
	std::string ip_string = ::ConvertIP( pContext->GetIP() );
	/*std::string ip_string = "";
	auto result_ip  = QueryManager::GetPlayerIPAsync( _player.member_id() , ip_string );
	result_ip.wait();
	if ( false == result_ip.get() )
	{

	}
	if ( ip_string == "" )
	{
		auto t_ip = ::ConvertIP( pContext->GetIP() );
		auto t_result_ip = QueryManager::InsertPlayerIPLog( _player.member_id() , t_ip );
		t_result_ip.wait();
		if ( false == t_result_ip.get() )
		{

		}
		ip_string = t_ip;
	}*/

	if ( false == result.get() )
	{
		// Player 정보를 읽어 들이지 못했다.
		// 로그인 실패 통보

		// 10802 작성

		int code = 10802;

		//std::string ipString = ::ConvertIP( pContext->GetIP() );

		auto log_result = QueryManager::InsertAccountLog(
			code , // code
			platform_guid , // platform_guid , // uid
			"" , // adminid
			"" , // adid
			"" , // asset
			"" , // cmd
			loginRequest.dev_meta() , // request.dev_meta() , // device
			"" , // gmsessid
			"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
			ip_string , // ipaddr
			"" , // request.os_ver() , // osver
			"" , // marketString , // pf
			"" , // svcuid
			made_id.size() != 0 ? made_id : _platform_authcode , // accountid
			0 , // chip
			0 , // chip_g
			0 , // chip_s
			0 , // coin
			0 , // coin_g
			0 , // coin_s
			0 , // slotcoin
			0 , // gem
			0 , // gem_f
			0 , // gem_p
			0 , // kickoutticket
			0 , // exp
			0 , // friend_cnt
			"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
			"" , // logintype
			"" , // type
			" " , // nickname
			"" , // deletetime
			"" , // registered
			0 , // block_hour
			0 , // left_count
			0 , // limit_type
			"" , // reqtm
			"" , // second_pw
			"" , // reason
			"" , // result
			"" , // hash
			game_version , // ver
			"" ); // etc

		log_result.wait();

		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ TransferServer ] Player loading failed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		// 접속을 종료시킨다.
		NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
		pCommandQueueManager->PushCommand( static_cast< NetLib::cIocpContext* >( pContext ) , CSNet::ProtocolCommand::SYS_NET_DISCONNECT );
		return;
	}
	// 중복 접속 확인 처리 NEW
	NetLib::cSession* connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( _player.member_id() ) );
	if ( connectedClientSession == nullptr )
	{
		connectedClientSession = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetPending( static_cast< int64 >( _player.member_id() ) );
		if ( connectedClientSession != nullptr && connectedClientSession->GetContext() != nullptr )
		{
			PmNet::DupTunnelRS dupResponse;
			//dupMp._crc = 0;  // decoy
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , dupResponse , General::ResultCode::Result_Success , errorString );
		}
	}
	else
	{
		if ( connectedClientSession->GetContext() != nullptr )
		{
			PmNet::DupTunnelRS dupResponse;
			//dupMp._crc = 0;  // decoy
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
			GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , dupResponse , General::ResultCode::Result_Success , errorString );
		}
	}
	//bool is_duplicate = false;
	//// 중복 접속 확인 처리
	//auto connectedClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( static_cast< int64 >( _player.member_id() ) ) );
	//if ( connectedClientSession != nullptr ) {
	//	//is_duplicate = true;
	//	if ( connectedClientSession->GetContext() != nullptr ) {

	//		auto contextSession = static_cast< cClientSession* >( connectedClientSession->GetContext()->GetSession() );
	//		if ( contextSession == connectedClientSession ) {
	//			const uint64& roomNumber = connectedClientSession->GetJoinedRoomNumber();
	//			const uint64& playerIdx = connectedClientSession->GetPlayerIdx();

	//			if ( roomNumber != 0 ) {

	//				cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	//				if ( pGameRoom != nullptr ) {

	//					// 관전자와 플레이중인 사람으로 나눈다.
	//					if ( pGameRoom->isWatcher( playerIdx ) ) {

	//						// 관전자는 즉시 내보낸다.
	//						NetLib::cSingleton<cGameRoomManager>::GetInstance()->RemoveWatcherResource( connectedClientSession );

	//						// 이유저의 접속을 끊어낸다.
	//						PmNet::DupTunnelRS duplicatedSessionRes;

	//						std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
	//						GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
	//						connectedClientSession->GetContext()->Disconnect();

	//						// 세션 삭제 및 풀러로 이동
	//						NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( playerIdx );

	//						connectedClientSession->RoomOutReset();

	//						NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer Duplicated Session pGameRoom->isWatcher Success. PlayerIdx [ %llu ]" , playerIdx );
	//					}
	//					else {

	//						// 플레이중인 방의 상태에 따라 처리한다.

	//						if ( General::RoomState::RoomState_Waiting == pGameRoom->GetRoomStatus() ) {

	//							// 방이 Wait 상태이다. 즉시 내보낸다.
	//							if ( NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( connectedClientSession ) ) {

	//								PmNet::DupTunnelRS duplicatedSessionRes;

	//								std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
	//								GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
	//								connectedClientSession->GetContext()->Disconnect();

	//								NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( playerIdx );

	//								connectedClientSession->RoomOutReset();

	//								NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer Duplicated Session UserGameRoomOut Success. PlayerIdx [ %llu ]" , playerIdx );
	//							}
	//							else {

	//								// 플레이 중인 유저이다.
	//								// 플레이 중인 유저의 세션만 끊어 낸다.
	//								PmNet::DupTunnelRS duplicatedSessionRes;

	//								std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
	//								GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
	//								connectedClientSession->GetContext()->Disconnect();

	//								NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer Duplicated Session UserGameRoomOut Failed. Room Playing Status PlayerIdx [ %llu ]" , playerIdx );
	//							}
	//						}
	//						else {
	//							//GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_DuplicateAccessBlocked , "error" );
	//							//return;
	//							PmNet::DupTunnelRS duplicatedSessionRes;

	//							std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
	//							GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
	//							//connectedClientSession->GetContext()->Disconnect();
	//							//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer Duplicated Session No RoomState_Waiting Success. PlayerIdx [ %llu ]" , playerIdx );
	//							////NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->RemoveOnly( playerIdx );
	//						}
	//					}
	//				}
	//			}
	//			else {

	//				// 접속을 끊어낸다.
	//				PmNet::DupTunnelRS duplicatedSessionRes;

	//				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
	//				GetOwner()->SendBuffer( connectedClientSession->GetContext() , nThreadIndex , General::PacketID::Packet_AccessDuplicateClose , duplicatedSessionRes , General::ResultCode::Result_Success , errorString );
	//				connectedClientSession->GetContext()->Disconnect();

	//				// 세션 삭제 및 풀러로 이동
	//				NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Pushpenssion( playerIdx );

	//				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer Duplicated Session No roomNumber == 0 Success. PlayerIdx [ %llu ]" , playerIdx );
	//			}
	//		}
	//	}
	//}
	std::map<int , General::AvatarProfile> _avatarsMap;
	const uint64& playerIdx = _player.member_id();

	result = std::async( [playerIdx , &_avatarsMap]() {
		return QueryManager::AvatarsGet( playerIdx , _avatarsMap );
	} );

	result.wait();

	if ( false == result.get() )
	{
		// Player 정보를 읽어 들이지 못했다.
		// 로그인 실패 통보

		// 10802 작성

		int code = 10802;

		//std::string ipString = ::ConvertIP( pContext->GetIP() );

		auto log_result = QueryManager::InsertAccountLog(
			code , // code
			platform_guid , // platform_guid , // uid
			"" , // adminid
			"" , // adid
			"" , // asset
			"" , // cmd
			loginRequest.dev_meta() , // request.dev_meta() , // device
			"" , // gmsessid
			"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
			ip_string , // ipaddr
			"" , // request.os_ver() , // osver
			"" , // marketString , // pf
			"" , // svcuid
			made_id.size() != 0 ? made_id : _platform_authcode , // accountid
			0 , // chip
			0 , // chip_g
			0 , // chip_s
			0 , // coin
			0 , // coin_g
			0 , // coin_s
			0 , // slotcoin
			0 , // gem
			0 , // gem_f
			0 , // gem_p
			0 , // kickoutticket
			0 , // exp
			0 , // friend_cnt
			"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
			"" , // logintype
			"" , // type
			" " , // nickname
			"" , // deletetime
			"" , // registered
			0 , // block_hour
			0 , // left_count
			0 , // limit_type
			"" , // reqtm
			"" , // second_pw
			"" , // reason
			"" , // result
			"" , // hash
			game_version , // ver
			"" ); // etc

		log_result.wait();

		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ TransferServer ] Player avatar loading failed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		// 접속을 종료시킨다.
		NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
		pCommandQueueManager->PushCommand( static_cast< NetLib::cIocpContext* >( pContext ) , CSNet::ProtocolCommand::SYS_NET_DISCONNECT );
		return;
	}
	else
	{
		// 아바타 목록이 동일하지 않으면 아바타를 생성한다.
		std::vector<std::future<BOOL>> results;
		const auto& avatars = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatars();
		if ( _avatarsMap.size() != avatars.size() ) {

			for ( auto& avatar : avatars ) {

				auto iter = _avatarsMap.find( avatar.avatar_ref_id() );
				if ( iter == _avatarsMap.end() ) {

					// 아바타가 없다. 생성한다.
					uint64 _avatar_idx;

					// free 인 경우는 99년 까지 착용하도록
					std::string _expiry_date;
					if ( avatar.free_available() )
						_expiry_date = "2099-12-31 23:59:59";
					else
						_expiry_date = TimeUtils::GetCurrentDateTime();

					results.push_back( QueryManager::InsertAvatarAsync( playerIdx , avatar.avatar_ref_id() , _expiry_date , _avatar_idx ) );
				}
			}

			for ( auto& result : results )
				result.wait();

			// 아바타 재로딩
			_avatarsMap.clear();

			result = std::async( [playerIdx , &_avatarsMap]() {
				return QueryManager::AvatarsGet( playerIdx , _avatarsMap );
			} );

			result.wait();
		}

		// 아바타 데이터를 채워 넣는다.
		for ( auto& avatarPair : _avatarsMap ) {
			auto& _avatar = avatarPair.second;
			const auto& avatar_data = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatar( _avatar.avatar_ref_id() );
			_avatar.set_free_available( avatar_data.free_available() );
			//avatar.set_avatar_label( avatar_data.avatar_label() );
		}
	}

	// QATool
#ifdef _DEBUG
	auto iterCoin = cClientSession::m_qa_player_coins.find( _player.member_id() );
	if ( iterCoin != cClientSession::m_qa_player_coins.end() ) {
		_player.set_wallet_coins( iterCoin->second );
	}

	auto iterChip = cClientSession::m_qa_player_chips.find( _player.member_id() );
	if ( iterChip != cClientSession::m_qa_player_chips.end() ) {
		_player.set_wallet_chips( iterChip->second );
	}
#endif

	/*General::ParticipantProfile* responsePlayer = response.mutable_member_info();
	responsePlayer->CopyFrom( _player );*/

	for ( const auto& avatarPair : _avatarsMap ) {
		auto& _avatar = avatarPair.second;
		General::AvatarProfile* addAvatar = response.add_skins();
		addAvatar->CopyFrom( _avatar );
	}

	PmNet::Ledger _records;
	int _reads = 0;

	const uint64 _player_idx = _player.member_id();

	result = std::async( [_player_idx , &_reads , &_records]() {
		return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
	} );

	result.wait();

	// 기본 데이터를 만든다.
	if ( _reads == 0 ) {

		// Async 코드
		std::future<BOOL> createRecordsResult = QueryManager::CreateRecordsAsync( _player_idx );

		createRecordsResult.wait();

		if ( false == createRecordsResult.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		result = std::async( [_player_idx , &_reads , &_records]() {
			//QueryManager* queryManager = new QueryManager();
			return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
		} );

		result.wait();

		if ( false == result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}

	//BOOL QueryManager::PlayerGetJokboRecords( const uint64& player_idx , int& reads , PmNet::Ledger& records )

	int _jokbo_reads = 0;
	result = std::async( [_player_idx , &_jokbo_reads , &_records]() {
		return QueryManager::PlayerGetJokboRecords( _player_idx , _jokbo_reads , _records );
	} );

	result.wait();

	// 기본 데이터를 만든다.
	if ( _jokbo_reads == 0 ) {

		// 내부에서 한방에 처리하고 리턴한다.
		if ( QueryManager::CreateJokboRecords( _player_idx ) ) {

			result = std::async( [_player_idx , &_jokbo_reads , &_records]() {
				return QueryManager::PlayerGetJokboRecords( _player_idx , _jokbo_reads , _records );
			} );

			result.wait();
		}
	}

	int _quest_reads = 0;
	std::map<uint32 , General::TaskProgress> missions;
	std::map<uint32 , General::TaskProgress> lounges;
	std::map<uint32 , General::TaskProgress> achieves;
	QueryManager::PlayerGetQuests( _player_idx , _quest_reads , missions , lounges , achieves );

	if ( _quest_reads == 0 ) {
		QueryManager::CreateQuests( _player_idx );
		QueryManager::PlayerGetQuests( _player_idx , _quest_reads , missions , lounges , achieves );
	}

	PmNet::Ledger* responseRecords = response.mutable_ledger();
	responseRecords->CopyFrom( _records );

	// 세션 등록
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if ( !pServerManager )
		return;

	// 내부에서 Session을 찾으면 파라미터로 넘겨준 Context에게 Session을 셋해줍니다.
	// 파라미터로 넘겨준 Context에게도 찾은 Session을 Set 해줍니다.
	cClientSession* pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->GetReLoginSessionAndSyncContext( _player.member_id() , pContext ) );
	if ( pClientSession != nullptr ) {

		// Pending 세션을 찾은 경우
		NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueuePtr( pClientSession->GetCommandQueueIndex() );
		General::ParticipantProfile* responsePlayer = response.mutable_member_info();
		responsePlayer->CopyFrom( pClientSession->GetPlayer() );
		pClientSession->GetPlayerExtRef().set_ranking_reward_claimed_date( _playerExt.ranking_reward_claimed_date() );
		UINT roomNumber = pClientSession->GetJoinedRoomNumber();
		if ( roomNumber != 0 ) {
			cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
			if ( !pGameRoom || !pGameRoom->GetGameInterface() ) {
				pClientSession->SetPlayer( account_idx , _player , _playerExt );
			}
		}
		else {
			pClientSession->SetPlayer( account_idx , _player , _playerExt );
		}

	}
	else {

		// Pending 세션이 없는 경우, Context 와 Session 신규 연결
		pClientSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->AllocateSession( static_cast< int64 >( _player.member_id() ) , pContext ) );
		if ( pClientSession == nullptr ) {
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ TransferServer ] Login failed wity system error" );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
			errorString = protoutil::cProtoUtil::ErrorCodeString( "[ TransferServer ] AllocateSession returns nullptr. need check why session pool is emtpy" );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
			return;
		}

		pClientSession->SetGuids( account_guid , platform_guid );
		pClientSession->SetPlayer( account_idx , _player , _playerExt );	// 플레이어 정보

		//pClientSession->LoadQuestsOnLogin(); // 누락된 미션 생성
		General::ParticipantProfile* responsePlayer = response.mutable_member_info();
		responsePlayer->CopyFrom( pClientSession->GetPlayer() );
	}
	pClientSession->SetAvatars( _avatarsMap );	// 아바타 목록
	pClientSession->SetRecords( _records );	// 플레이어 전적
	pClientSession->SetMissionAndAchieve( missions , lounges , achieves ); // 미션, 업적
	// 세션 타입 설정
	pClientSession->SetSessionType( Sessions::SESSION_CLIENT );

	pClientSession->SoftClear();

	// 친구 관리자에 자신의 정보 등록
	PmNet::MateDetail friendInfo;
	friendInfo.set_mate_state( General::ContactState::ContactState_Online );

	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	friendInfo.set_node_id( configReader->SID_FOR_MANAGE );

	// 플레이어 데이터 카피
	auto add_player = friendInfo.mutable_member_info();
	add_player->CopyFrom( _player );

	// 플레이어 전적 데이터 카피
	auto records = friendInfo.mutable_ledger();
	records->CopyFrom( _records );

	NetLib::cSingleton<cFriendManager>::GetInstance()->SetFriendInfo( friendInfo );

	// 로비의 친구 상태 갱신
	{
		const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

		Server::SyncFriendInfoStatus syncFriendInfoStatus;
		syncFriendInfoStatus.set_player_idx( pClientSession->GetPlayerIdx() );
		syncFriendInfoStatus.set_friend_status( General::ContactState::ContactState_Online );
		syncFriendInfoStatus.set_server_id( configReader->SID_FOR_MANAGE );
		syncFriendInfoStatus.set_player_chip( pClientSession->GetChip() );
		syncFriendInfoStatus.set_player_coin( pClientSession->GetCoin() );
		syncFriendInfoStatus.set_player_safe_chip( pClientSession->GetSafeChip() );
		syncFriendInfoStatus.set_player_safe_coin( pClientSession->GetSafeCoin() );
		General::ParticipantProfile* responsePlayer = syncFriendInfoStatus.mutable_player_data();
		responsePlayer->CopyFrom( pClientSession->GetPlayer() );

		NetLib::cSingleton<cFriendManager>::GetInstance()->SyncFriendInfoStatus( syncFriendInfoStatus );

		// 각 로비서버들에게도 쏘아줌
		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( syncFriendInfoStatus.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer SerializeToArray Failed." );
		}

		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeContactStateSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncFriendInfoStatus.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

			// 로비서버 전송 실패
		}
	}

	// 슬롯 서버들에게 플레이어 로그인 통지
	// 접속 세션이 있다면 끊어 내라
	{
		Server::SyncPlayerLoginNoti syncPlayerLoginNoti;
		syncPlayerLoginNoti.set_player_idx( pClientSession->GetPlayerIdx() );

		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( syncPlayerLoginNoti.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::TransferServer SyncPlayerLoginNoti SerializeToArray Failed." );
		}

		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
								   E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeAccessNotice , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncPlayerLoginNoti.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

			// 로비서버 전송 실패
		}
	}

	// Server::LostLimit 데이터를 캐슁한다.
	auto add_lost_limit = response.mutable_loss_cap();
	auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( _lost_limit.account_id() );
	if ( lostLimit != nullptr )
	{
		lostLimit->set_monthly_purchase_total( _lost_limit.monthly_purchase_total() );
		pClientSession->SetLostLimit( lostLimit );
		add_lost_limit->CopyFrom( *lostLimit );
	}
	else
	{
		// 아직 캐슁된 데이터가 없다. 캐슁 처리
		auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->SetLostLimit( _lost_limit );
		if ( lostLimit == nullptr )
		{
			// 이 경우라면 로그인을 실패내고 손실한도를 다시 재처리 하자.
			SendMessageAndLogWrite( General::ResultCode::Result_LossLimitLoadFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		add_lost_limit->CopyFrom( *lostLimit );
		pClientSession->SetLostLimit( lostLimit );

		add_lost_limit->CopyFrom( *lostLimit );
		pClientSession->SetLostLimit( lostLimit );

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
	}

	//이벤트 정보 로드 (캐시에서)
	//UpdateLoginEvent( pContext );

	std::vector<PmNet::DropDetail>* event_list = &pClientSession->login_event_list;
	*event_list = NetLib::cSingleton<cLoginEventManager>::GetInstance()->GetLoginEventList();

	//유저 로그인이벤트정보 로드
	//UpdateUserLoginEvent( pContext );

	//로그인 이벤트 정보 로드
	std::set<string>* receive_list = &pClientSession->receive_login_reward_list;
	receive_list->clear();
	std::future<BOOL> player_event_reward_result = QueryManager::GetLoginRewardByUserIDAsync( pClientSession->GetPlayerIdx() , *receive_list );
	player_event_reward_result.wait();

	if ( FALSE == player_event_reward_result.get() ) {
		//로그필요
		//SendMessageAndLogWrite( General::ResultCode::FindRecievedLoginEventFailed, pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 메일 로딩
	//QueryManager::CreateTestMails( _player_idx );
	std::vector<PmNet::InboxDetail> mail_list;
	std::future<BOOL> mail_result = QueryManager::MailBoxGetAsync( _player_idx , mail_list );
	mail_result.wait();

	if ( FALSE == mail_result.get() ) {

		// 메일박스 받아오기 실패 로그 처리
	}

	if ( mail_list.size() ) {

		pClientSession->SetMailBoxOnLogin( mail_list );

		// 메일 존재 레드닷 처리
		response.set_inbox_pending( true );
	}

	// OS 정보
	pClientSession->SetOSInfo( loginRequest.os_ver() );

	pClientSession->SetIp( ip_string );

	// 디바이스 정보
	pClientSession->SetDeviceInfo( loginRequest.dev_meta() );

	// 결제 처리용 마켓 처리 추가
	pClientSession->SetMarket( request.store_kind() );

	// 게임 버젼 셋팅
	pClientSession->SetGameVersion( game_version );

	string joinTime = "";
	std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( pClientSession->GetAccountGuid() , pClientSession->GetPlatformGuid() , joinTime );
	player_join_time.wait();

	if ( FALSE == player_join_time.get() ) {
	}
	pClientSession->SetJoinTime( joinTime );

	// 라운지 이벤트 체크
	auto lounge_event = response.mutable_atrium_drop();
	NetLib::cSingleton<cLoungeEvent>::GetInstance()->CopyLoungeEvent( lounge_event );

	string currentKstTime = TimeUtils::GetCurrentKSTDateTimeString();
	response.set_svr_ts( currentKstTime );// "2024-12-23 19:00:00"
	std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "" );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );

#ifdef _DEBUG
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );
#endif

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_NOR , "TransferServer Success. playerIdx %I64d" , pClientSession->GetPlayerIdx() );

	// 플레이 중인 방이 있었으면 방으로 난입 시켜 준다.
	//if ( pClientSession->GetPinballGame() != nullptr )
	//{
	//	pClientSession->LeavePinball( false );
	//}
	UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	if ( roomNumber != 0 ) {

		cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
		if ( pGameRoom == nullptr ) {
			SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		IGame* pGame = pGameRoom->GetGameInterface();
		if ( pGame == nullptr ) {
			SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		/*if ( !pGameRoom->isPlayer( playerIdx ) || pGameRoom->isWatcher( playerIdx ) )
		{
			pGameRoom->RemoveWatcher( playerIdx );
			pClientSession->SetJoinedRoomNumber( 0 );
			pClientSession->GameReset();
			SendMessageAndLogWrite( General::ResultCode::Result_Success , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;

		}*/
		const auto& channelData = pGame->GetChannelData();

		PmNet::ChamberEnterRS RoomJoinresponse;
		RoomJoinresponse.mutable_reconnect_state()->set_reattach_flag( true );
		RoomJoinresponse.set_ch_token( channelData.id() );
		RoomJoinresponse.set_view_hold_flag( pGame->IsWatcherReservation( pClientSession->GetPlayerIdx() ) );

		switch ( pGame->GetGameType() )
		{
		
		case General::PlayCategory::PlayCategory_TexasHoldem:
		{
			// 방에 진입 시킨다.
			General::ResultCode errorCode = pGameRoom->RoomReJoin( pClientSession );


			if ( errorCode == General::ResultCode::Result_Success ) {

				pGameRoom->GetGameRoomDetailOnRejoin( RoomJoinresponse , playerIdx );

				std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomReJoin ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

				SendMessageAndLogWrite( General::ResultCode::Result_Success , pContext , nThreadIndex , General::PacketID::Packet_SpaceEnter , RoomJoinresponse );

				pGame->OnIntruding( pClientSession );
				pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , channelData.id() );
			}
			else {
				SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , General::PacketID::Packet_SpaceEnter , RoomJoinresponse ); return;
			}
		}
		break;
		}

		// 로비의 친구 상태 갱신
		{
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::SyncFriendInfoStatus syncFriendInfoStatus;
			syncFriendInfoStatus.set_player_idx( pClientSession->GetPlayerIdx() );
			syncFriendInfoStatus.set_playing_channel_id( channelData.id() );
			syncFriendInfoStatus.set_friend_status( General::ContactState::ContactState_InPlay );
			syncFriendInfoStatus.set_server_id( configReader->SID_FOR_MANAGE );
			syncFriendInfoStatus.set_player_chip( pClientSession->GetChip() );
			syncFriendInfoStatus.set_player_coin( pClientSession->GetCoin() );
			syncFriendInfoStatus.set_player_safe_chip( pClientSession->GetSafeChip() );
			syncFriendInfoStatus.set_player_safe_coin( pClientSession->GetSafeCoin() );
			General::ParticipantProfile* responsePlayer = syncFriendInfoStatus.mutable_player_data();
			responsePlayer->CopyFrom( pClientSession->GetPlayer() );

			NetLib::cSingleton<cFriendManager>::GetInstance()->SyncFriendInfoStatus( syncFriendInfoStatus );

			// 각 로비서버들에게도 쏘아줌
			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( syncFriendInfoStatus.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "MoveRoom Server::SyncFriendInfoStatus SerializeToArray Failed." );
			}

			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
							E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeContactStateSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncFriendInfoStatus.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

				// 로비서버 전송 실패
			}
		}
	}
}

void cProtoMsgStub::MADEPassChange( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	// 로그인 전에 이루어 지는 상황이라 세션이 없다.
	/*if ( CheckSession( pContext ) )
		return;*/

	PmNet::NativeKeySwapRQ request;
	PmNet::NativeKeySwapRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );
	cClientSession* pClientSession = GetSession( pContext );
	std::string made_id = request.native_id();
	std::string made_password = request.native_key();

	// 패스워드 유효성 체크
	if ( false == MADEPlatform::isValidPassword( made_password ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessSecretPolicyBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// AccountGuid 검색
	const std::string& ci = request.ci();

	std::future<std::string> result = std::async( [ci]() {
		return QueryManager::AccountGetByAuthCode( ci );
	} );

	result.wait();

	std::string _account_guid = result.get();

	// 어카운트를 찾지 못했습니다.
	if ( _account_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	std::string MADE_account_guid;

	std::string _platform_guid;
	BYTE _platform_code = 0;
	std::string _made_password;
	int _passwd_retry_count;
	std::string _sub_password;
	int _sub_passwd_retry_count = 0;
	BOOL _game_play_agree = FALSE;
	BOOL _personal_info_agree = FALSE;
	BOOL _advertise_push_agree = FALSE;
	BOOL _night_advertise_push_agree = FALSE;
	string _expected_withdrawal_date;
	int _cancle_withdrawal_first_join = 0;

	std::future<BOOL> platform_result = QueryManager::FindMADEPlatformAsync( made_id , MADE_account_guid , _platform_guid , _platform_code , _made_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count ,
			_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );
	platform_result.wait();

	if ( FALSE == platform_result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( MADE_account_guid != _account_guid )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	if ( QueryManager::ResetMADEPassword( _account_guid , _platform_guid , made_password ) )
	{
		//10803 작성

		int code = 10803;

		//std::string ipString = ::ConvertIP( pContext->GetIP() );

		auto log_result = QueryManager::InsertAccountLog(
			code , // code
			_platform_guid , // platform_guid , // uid
			"" , // adminid
			"" , // adid
			"" , // asset
			"" , // cmd
			"" , // request.dev_meta() , // device
			"" , // gmsessid
			"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
			"" , // ipaddr
			"" , // request.os_ver() , // osver
			"" , // marketString , // pf
			"" , // svcuid
			made_id , // accountid
			0 , // chip
			0 , // chip_g
			0 , // chip_s
			0 , // coin
			0 , // coin_g
			0 , // coin_s
			0 , // slotcoin
			0 , // gem
			0 , // gem_f
			0 , // gem_p
			0 , // kickoutticket
			0 , // exp
			0 , // friend_cnt
			"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
			"" , // logintype
			"" , // type
			" " , // nickname
			"" , // deletetime
			"" , // registered
			0 , // block_hour
			0 , // left_count
			0 , // limit_type
			"" , // reqtm
			"" , // second_pw
			"" , // reason
			"" , // result
			"" , // hash
			"" , // ver
			"" ); // etc

		log_result.wait();

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
	}
	else
	{
		// 패스워드 변경 실패
	}
}

// 세션 체크 하지 않음
void cProtoMsgStub::ResetSubPassword( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::ReissueAuxKeyRQ request;
	PmNet::ReissueAuxKeyRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// AccountGuid 검색
	const std::string& ci = request.ci();

	std::future<std::string> result = std::async( [ci]() {
		return QueryManager::AccountGetByAuthCode( ci );
	} );

	result.wait();

	std::string _account_guid = result.get();

	// 어카운트를 찾지 못했습니다.
	if ( _account_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string MADE_account_guid;
	std::string _platform_guid;
	BYTE _platform_code = 0;
	std::string _made_password;
	int _passwd_retry_count;
	std::string _sub_password;
	int _sub_passwd_retry_count = 0;
	BOOL _game_play_agree = FALSE;
	BOOL _personal_info_agree = FALSE;
	BOOL _advertise_push_agree = FALSE;
	BOOL _night_advertise_push_agree = FALSE;
	string _expected_withdrawal_date;
	int _cancle_withdrawal_first_join = 0;

	// 자체 계정일때
	if ( request.native_id().size() )
	{
		std::future<BOOL> platform_result = QueryManager::FindMADEPlatformAsync( request.native_id() , MADE_account_guid , _platform_guid , _platform_code , _made_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count ,
			_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );
		platform_result.wait();

		if ( FALSE == platform_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		if ( _account_guid != MADE_account_guid )
		{
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( QueryManager::UpdateSubPassword( _account_guid , _platform_guid , "" ) )
		{
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
		}
		else
		{
			// 패스워드 변경 실패
#ifdef _DEBUG
			std::string serializedData;
			google::protobuf::util::MessageToJsonString( response , &serializedData );
			TraceA( "ResetSubPassword Failed" + serializedData );
#endif
		}
	}
	else
	{
		if ( request.ci().size() == 0 || request.outlet_uid().size() == 0 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_IdentityAndChannelRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		std::future<BOOL> platform_result = QueryManager::FindPlatformByCidAndPlatformGuidAsync( request.ci() , request.outlet_uid() , _sub_password , _sub_passwd_retry_count );
		platform_result.wait();

		if ( FALSE == platform_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( QueryManager::UpdateSubPassword( _account_guid , request.outlet_uid() , "" ) )
		{
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
		}
		else
		{
			// 패스워드 변경 실패
#ifdef _DEBUG
			std::string serializedData;
			google::protobuf::util::MessageToJsonString( response , &serializedData );
			TraceA( "ResetSubPassword Failed" + serializedData );
#endif
		}

		_platform_guid = request.outlet_uid();
	}

	//여기에 10503 작성
	int code = 10503;

	//std::string ipaddr = pContext != nullptr ? ::ConvertIP( pContext->GetIP() ) : "";

	auto log_result2 = QueryManager::InsertAccountLog(
		code , // code
		_platform_guid , // platform_guid , // uid
		"" , // adminid
		"" , // adid
		"" , // asset
		"" , // cmd
		request.dev_meta() , // request.dev_meta() , // device
		"" , // gmsessid
		"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
		"" , // ipaddr
		request.os_ver() , // request.os_ver() , // osver
		"" , // marketString , // pf
		"" , // svcuid
		_account_guid , // accountid
		0 , // chip
		0 , // chip_g
		0 , // chip_s
		0 , // coin
		0 , // coin_g
		0 , // coin_s
		0 , // slotcoin
		0 , // gem
		0 , // gem_f
		0 , // gem_p
		0 , // kickoutticket
		0 , // exp
		0 , // friend_cnt
		"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
		"" , // logintype
		"" , // type
		" " , // nickname
		"" , // deletetime
		"" , // registered
		0 , // block_hour
		0 , // left_count
		0 , // limit_type
		"" , // reqtm
		"" , // second_pw
		"" , // reason
		"" , // result
		"" , // hash
		request.app_ver() , // ver
		"" ); // etc

	log_result2.wait();
}

void cProtoMsgStub::UpdateTermsAgree( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::RefreshTermsOptinRQ request;
	PmNet::RefreshTermsOptinRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string _account_guid;
	std::string _platform_guid;
	std::string _made_password;
	int _passwd_retry_count;
	std::string _sub_password;
	int _sub_passwd_retry_count = 0;
	request.match_optin();
	request.pii_optin();
	request.promo_alert_optin();
	request.night_promo_alert_optin();

	// 자체 계정일때
	if ( request.native_id().size() )
	{
		// ci, made_id 로 검증 필요

		std::future<BOOL> agree_result = QueryManager::UpdateTermsAgree( request.native_id() , request.match_optin() , request.pii_optin() , request.promo_alert_optin() , request.night_promo_alert_optin() );
		agree_result.wait();

		if ( FALSE == agree_result.get() ) {

			// UpdateTermsAgree 변경 실패
#ifdef _DEBUG
			std::string serializedData;
			google::protobuf::util::MessageToJsonString( response , &serializedData );
			TraceA( "UpdateTermsAgree Failed" + serializedData );
#endif
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			return;
		}
		else {
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
		}
	}
	else
	{
		//if ( request.ci().size() == 0 || request.outlet_uid().size() == 0 ) {
		if ( request.outlet_uid().size() == 0 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_IdentityAndChannelRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		std::future<BOOL> agree_result = QueryManager::UpdateTermsAgreeByPlatformGuid( request.outlet_uid() , request.match_optin() , request.pii_optin() , request.promo_alert_optin() , request.night_promo_alert_optin() );
		agree_result.wait();

		if ( FALSE == agree_result.get() ) {

			// UpdateTermsAgree 변경 실패
#ifdef _DEBUG
			std::string serializedData;
			google::protobuf::util::MessageToJsonString( response , &serializedData );
			TraceA( "UpdateTermsAgree Failed" + serializedData );
#endif
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			return;
		}
		else {
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
		}
	}
}

void cProtoMsgStub::UpdateCiExpiryTime( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::RefreshCiExpireRQ request;
	PmNet::RefreshCiExpireRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	string ci = request.ci();
	string made_id = request.native_id();
	string platform_guid = request.outlet_uid();

	std::string MADE_account_guid;
	std::string _platform_guid;
	BYTE _platform_code = 0;
	std::string _made_password;
	int _passwd_retry_count;
	std::string _sub_password;
	int _sub_passwd_retry_count = 0;
	BOOL _game_play_agree = FALSE;
	BOOL _personal_info_agree = FALSE;
	BOOL _advertise_push_agree = FALSE;
	BOOL _night_advertise_push_agree = FALSE;
	string _expected_withdrawal_date;
	int _cancle_withdrawal_first_join = 0;
	string _account_guid = "";

	if ( request.native_id().size() )
	{
		// 1단계 CI 체크
		std::future<std::string> result = std::async( [ci]() {
			return QueryManager::AccountGetByAuthCode( ci );
		} );

		result.wait();

		_account_guid = result.get();

		if ( _account_guid.empty() )
		{
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		std::future<BOOL> platform_result = QueryManager::FindMADEPlatformAsync( request.native_id() , MADE_account_guid , _platform_guid , _platform_code , _made_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count ,
			_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );
		platform_result.wait();

		if ( FALSE == platform_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 2단계 어카운트 ID 체크
		if ( _account_guid != MADE_account_guid )
		{
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		platform_guid = _platform_guid;
	}
	else
	{
		// 1단계 CI 체크
		std::future<std::string> result = std::async( [ci]() {
			return QueryManager::AccountGetByAuthCode( ci );
		} );

		result.wait();

		_account_guid = result.get();

		if ( _account_guid.empty() )
		{
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 2단계 플랫폼ID 체크
		int _platform_code = 0;
		std::future<BOOL> MADE_result = QueryManager::GetPlayerPlatformAsync( _account_guid , platform_guid , _platform_code );
		MADE_result.wait();

		if ( false == MADE_result.get() )
		{
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}

	std::future<string> result = QueryManager::FindExpiryTimeByCiAsync( ci );
	result.wait();

	std::string ci_expiry_time_string = result.get();
	if ( ci_expiry_time_string.empty() || ci_expiry_time_string.size() == 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string one_year_expiry_time_string = TimeUtils::OneYearLaterTime();

	if ( FALSE == QueryManager::UpdateNiceAuthExpiryTime( ci , one_year_expiry_time_string ) ) {

		// 나이스 인증 만료 시간 연창 실패
		SendMessageAndLogWrite( General::ResultCode::Result_IdentityAuthLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	std::time_t today_time = TimeUtils::TodayStartTimeTM();
	QueryManager::UpdateDailyRefreshTime( TimeUtils::TMToString( today_time ) , platform_guid );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	if ( made_id.size() != 0 )
	{

	}
	else
	{

	}
}

// 게임 탈퇴
void cProtoMsgStub::WithdrawalGame( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	PmNet::QuitMatchRQ request;
	PmNet::QuitMatchRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cClientSession* pClientSession = GetSession( pContext );

	const std::string& account_guid = pClientSession->GetAccountGuid();
	const std::string& platform_guid = pClientSession->GetPlatformGuid();

	// 플랫폼만 탈퇴 처리를 한다.
	// 7일간의 탈퇴 철회 기간이 존재한다.
	std::string sevenDaysLater = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );

	if ( FALSE == QueryManager::PlatformWithdrawalGame( account_guid , platform_guid , sevenDaysLater ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AccountClosureFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	// 탈퇴 예약 로그
	{
		const General::ParticipantProfile& _player = pClientSession->GetPlayerRef();
		const Server::ParticipantProfileInternal& _playerExt = pClientSession->GetPlayerExtRef();

		int code = 10201;
		std::string loginTypeString = "";

		//std::string ipString = ::ConvertIP( pContext->GetIP() );

		std::string idpcode = "";

		int platform_code;
		std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( account_guid , platform_guid , platform_code );
		platform_code_result.wait();

		if ( FALSE == platform_code_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// Player Platform
		const General::AccessChannelType _platform_type = static_cast< General::AccessChannelType > ( platform_code );
		auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );

		auto valueMaketDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( pClientSession->GetMarket() );
		std::string marketString = valueMaketDescriptor->name();

		auto log_result = QueryManager::InsertAccountLog(
			code , // code
			platform_guid , // platform_guid , // uid
			"" , // adminid
			"" , // adid
			"" , // asset
			"" , // cmd
			pClientSession->GetDeviceInfo() , // request.dev_meta() , // device
			"" , // gmsessid
			marketString , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
			pClientSession->GetIp() , // ipaddr
			pClientSession->GetOSInfo() , // request.os_ver() , // osver
			valueDescriptor->name() , // marketString , // pf
			"" , // svcuid
			account_guid , // accountid
			_player.wallet_chips() + _player.vault_chips() , // chip
			_player.wallet_chips() , // chip_g
			_player.vault_chips() , // chip_s
			_player.wallet_coins() + _player.vault_coins() , // coin
			_player.wallet_coins() , // coin_g
			_player.vault_coins() , // coin_s
			_playerExt.remaining_reel_coin() , // slotcoin
			_player.wallet_gems() + _player.paid_gems() , // gem
			_player.wallet_gems() , // gem_f
			_player.paid_gems() , // gem_p
			_player.kick_ticket_balance() , // kickoutticket
			_player.experience_points() , // exp
			0 , // friend_cnt
			pClientSession->GetJoinTime() , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
			loginTypeString , // logintype
			"" , // type
			_player.display_name() , // nickname
			sevenDaysLater , // deletetime
			"OK" , // registered
			0 , // block_hour
			0 , // left_count
			0 , // limit_type
			"" , // reqtm
			"" , // second_pw
			"" , // reason
			"" , // result
			"" , // hash
			pClientSession->GetGameVersion() , // ver
			"" ); // etc

		log_result.wait();
	}
}

void cProtoMsgStub::CancelWithdrawalGame( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	/*if ( CheckSession( pContext ) )
		return;*/

	PmNet::RevokeQuitMatchRQ request;
	PmNet::RevokeQuitMatchRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	string account_guid = request.profile_uid();
	string platform_guid = request.outlet_uid();
	string made_id = request.native_id();
	string MADE_passwd = request.native_key();
	//string input_sub_password = request.aux_key();
	string _expected_withdrawal_date;

	BYTE _platform_code = 0;

	// 자체 로그인 이면 패스워드가 맞는지 확인한다.
	if ( made_id.size() != 0 )
	{
		int _passwd_retry_count = 0;
		std::string _find_password;
		int _sub_passwd_retry_count = 0;
		std::string _sub_password;
		BOOL _game_play_agree = FALSE;
		BOOL _personal_info_agree = FALSE;
		BOOL _advertise_push_agree = FALSE;
		BOOL _night_advertise_push_agree = FALSE;
		int _cancle_withdrawal_first_join = 0;

		std::future<BOOL> MADE_result = QueryManager::FindMADEPlatformAsync( made_id , account_guid , platform_guid , _platform_code , _find_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count ,
			_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

		MADE_result.wait();

		if ( FALSE == MADE_result.get() ) {

			// 플랫폼 찾기 실패
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( account_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( _passwd_retry_count >= 5 ) {
			response.set_key_miss_cnt( _passwd_retry_count );
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessRetryLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 패스워드가 잘못되었습니다.
		if ( false == MADE_passwd._Equal( _find_password ) ) {
			++_passwd_retry_count;
			response.set_key_miss_cnt( _passwd_retry_count );

			if ( _passwd_retry_count < 5 )
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessPasswordMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			else
				SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessRetryLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );

			QueryManager::UpdateMADEPasswordRetryCount( account_guid , platform_guid , _passwd_retry_count );
			return;
		}

		// 로그인에 성공한 경우 이므로 패스워드 리트라이 카운트를 초기화 해준다.
		if ( _passwd_retry_count > 0 )
			QueryManager::UpdateMADEPasswordRetryCount( account_guid , platform_guid , 0 );

		// 2차 비밀번호가 있고 로그인에 성공한 경우 패스워드 리트라이 카운트 초기화
		if ( false == _sub_password.empty() && _sub_passwd_retry_count > 0 )
			QueryManager::UpdateSubPasswordRetryCount( account_guid , platform_guid , 0 );
	}
	else
	{
		_platform_code = static_cast< BYTE >( General::AccessChannelType::AccessChannel_LocalAccount );
		int _sub_passwd_retry_count = 0;
		string _platform_authcode;
		std::string _sub_password;
		BOOL _game_play_agree = FALSE;
		BOOL _personal_info_agree = FALSE;
		BOOL _advertise_push_agree = FALSE;
		BOOL _night_advertise_push_agree = FALSE;
		int _cancle_withdrawal_first_join = 0;

		std::future<BOOL> MADE_result = QueryManager::FindPlatformAsync( account_guid , platform_guid , _platform_authcode , _sub_password , _sub_passwd_retry_count ,
			_game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );

		MADE_result.wait();

		if ( FALSE == MADE_result.get() ) {

			// 플랫폼 찾기 실패
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( account_guid.empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessIdRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}

	if ( account_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_VerifiedAccountRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( platform_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ChannelGuidMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( !_expected_withdrawal_date.empty() ) {

		// 기간 확인
		std::time_t _withdrawal_date = TimeUtils::StringToTimeTM( _expected_withdrawal_date );
		std::time_t now = std::time( nullptr );

		// 탈퇴 기간 만료후에는 구제 불가
		if ( now >= _withdrawal_date ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ClosedAccountBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}

	// 탈퇴 취소 처리
	QueryManager::PlatformWithdrawalGame( account_guid , platform_guid , "" );
	QueryManager::PlatformCancleWithdrawalFirstJoin( account_guid , platform_guid , 1 );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::NickChange( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	PmNet::AliasSwapRQ request;
	PmNet::AliasSwapRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cClientSession* pClientSession = GetSession( pContext );

	std::string before_nickname = pClientSession->GetNickName();

	response.set_dup_check( request.dup_check() );
	response.set_alias_label( request.alias_label() );

	// 닉네임 길이 검증 강화
	if ( request.alias_label().size() == 0 || request.alias_label().size() > 64 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AliasFormatRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// UTF-8 문자 수 검증 (최대 20자)
	std::wstring w_nick_temp = StringUtil::Utf8ToWide( request.alias_label() );
	if ( w_nick_temp.length() > 20 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AliasFormatRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 허용된 문자만 포함하는지 검증 (정규식 사용)
	if ( !StringUtil::isValidString( request.alias_label() ) ) {
		// 디버깅: 실패한 닉네임의 문자 코드 분석 로그 출력
		//std::string debugInfo = StringUtil::debugStringCharCodes( request.alias_label() );
		// TODO: 로그 시스템에 맞게 수정 필요
		// LogWrite( LogLevel::DEBUG , "Invalid nickname analysis: %s" , debugInfo.c_str() );

		SendMessageAndLogWrite( General::ResultCode::Result_AliasFormatRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	// 금지어 검사
	Trie* trie = NetLib::cSingleton<Trie>::GetInstance();
	const std::wstring nick = StringUtil::Utf8ToWide( request.alias_label() );
	if ( trie->containsRestrictedWord( nick ) )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_AliasFormatRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	/*std::string removedString = request.alias_label();
	StringUtil::RemoveZeroWidthSpace( removedString );

	std::wstring w_nick_name = StringUtil::Utf8ToWide( request.alias_label() );
	w_nick_name = StringUtil::RemoveZeroWhiteSpace( w_nick_name );*/

	const std::string change_nick = StringUtil::RemoveInvisibleSpaces( request.alias_label() );

	const uint64& playerIdx = pClientSession->GetPlayerIdx();

	// 중복 체크 요청인경우에는 중복 체크
	if ( request.dup_check() )
	{
		std::vector<std::string> _find_nicks;
		std::future<BOOL> result = QueryManager::SelectNickNameAsync( change_nick , _find_nicks );
		result.wait();

		if ( result.get() )
		{
			if ( _find_nicks.size() ) {
				// 닉네임이 검색됨
				SendMessageAndLogWrite( General::ResultCode::Result_AliasAlreadyUsed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			}
			else {
				// 검색된 닉 없음 변경 가능
				GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
			}
		}
		else
		{
			SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		}
	}
	else
	{
		// _nickname_change_prohibite_expiry_time 시간이 지났는지 확인
		std::string expiry_time = pClientSession->GetNicknameChangeProhibiteExpiryTime();
		std::time_t cur_expiry_time = TimeUtils::StringToTimeTM( expiry_time );
		std::time_t now = std::time( nullptr );

		// 닉네임 변경가능한 시간이 되지 않았다.
		if ( cur_expiry_time > now ) {
			SendMessageAndLogWrite( General::ResultCode::Result_AliasUpdateCooldownActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( FALSE == QueryManager::UpdateNickName( playerIdx , change_nick ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_AliasUpdateFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		}
		else {

			// 닉네임 변경 가능한 시간 셋팅
			std::string nextExpiryTime = TimeUtils::NextMonthStartTimeString();
			pClientSession->SetNicknameChangeProhibiteExpiryTime( nextExpiryTime );
			pClientSession->SetNickName( change_nick );

			std::future<BOOL> result = pClientSession->SavePlayer();
			result.wait();

			response.set_alias_lock_until( nextExpiryTime );

			//10301 작성
			int code = 10301;
			string joinTime;
			std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( pClientSession->GetAccountGuid() , pClientSession->GetPlatformGuid() , joinTime );
			player_join_time.wait();

			if ( FALSE == player_join_time.get() ) {
			}
			//std::string ipString = ::ConvertIP( pContext->GetIP() );

			auto log_result = QueryManager::InsertAccountLog(
				code , // code
				pClientSession->GetPlatformGuid() , // platform_guid , // uid
				"" , // adminid
				"" , // adid
				"" , // asset
				"" , // cmd
				"" , // request.dev_meta() , // device
				"" , // gmsessid
				"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
				pClientSession->GetIp() , // ipaddr
				"" , // request.os_ver() , // osver
				"" , // marketString , // pf
				"" , // svcuid
				"" , // accountid
				0 , // chip
				0 , // chip_g
				0 , // chip_s
				0 , // coin
				0 , // coin_g
				0 , // coin_s
				0 , // slotcoin
				0 , // gem
				0 , // gem_f
				0 , // gem_p
				0 , // kickoutticket
				0 , // exp
				0 , // friend_cnt
				joinTime , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				"" , // logintype
				"" , // type
				change_nick , // nickname
				"" , // deletetime
				"" , // registered
				0 , // block_hour
				0 , // left_count
				0 , // limit_type
				"" , // reqtm
				"" , // second_pw
				"" , // reason
				"" , // result
				"" , // hash
				pClientSession->GetGameVersion() , // ver
				before_nickname ); // etc

			log_result.wait();

			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
		}
	}
}

void cProtoMsgStub::UpdatePush( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	PmNet::RefreshAlertRQ request;
	PmNet::RefreshAlertRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );
	cClientSession* pClientSession = GetSession( pContext );
	string guid = pClientSession->GetPlatformGuid();

	std::future<BOOL> agree_result = QueryManager::UpdatePushAgreeByPlatformGuid( guid , request.alert_optin() , request.night_promo_alert_optin() );
	agree_result.wait();

	if ( FALSE == agree_result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_alert_optin( request.alert_optin() );
	response.set_night_promo_alert_optin( request.night_promo_alert_optin() );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

}
void cProtoMsgStub::RoomCreate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	PmNet::ChamberBuildRQ request;
	PmNet::ChamberBuildRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cClientSession* pClientSession = GetSession( pContext );

	// 이미 방에 조인해 있는지 체크
	UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	if ( roomNumber != 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomAlreadyJoined , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string _channel_id = request.ch_token();

	// 데이터 파일 로딩
	auto channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( _channel_id );
	if ( channelData.id().empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto distributions = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelDistribution();
	auto mapPair = distributions.find( channelData.game_type() );
	if ( mapPair == distributions.end() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 소유한 채널이 아니면 다른 로비 서버로 이동후 방을 생성하도록 한다.
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	if ( mapPair->second != configReader->SID_FOR_MANAGE ) {

		auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , mapPair->second );
		if ( server_info == nullptr ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		//GMsg_MoveLobby
		PmNet::ShiftAtriumRS _move_response;
		_move_response.set_atrium_node_ip( server_info->publicIpAddress );
		_move_response.set_endpoint( server_info->nPort );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );

		// 이동할 서버의 정보를 받아온다.
		/*E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeSpaceQuery , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_request.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}*/


		return;
	}

	// 채널 입장 조건 체크
	uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

	if ( channelData.money_min() > playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 홀덤 입장 조건 체크 디버그 모드일때 작동 안하도록 처리
	if ( channelData.money_max() < playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	General::PlayCategory gameType = channelData.game_type();
	General::RoomAccessMode roomType = General::RoomAccessMode::RoomAccess_Public;	// 일반방, 친구방
	General::AssetKind moneyType = channelData.money_type();
	uint64 seedMoneyValue = channelData.seed_money();

	General::BetPolicy bettingRuleType = request.wager_rule();
	General::RuleProfile gameRuleType = request.match_rule();
	request.seat_cap();

	const int32 show_down_delay_ms_per_player = request.reveal_delay_per_member();
	const int32 show_down_community_delay_ms_per = request.reveal_shared_delay();

	// 빈방 검색
	uint64 joinedRoomNumberbefore = 0;
	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->PopEmptyRoom( roomType , pClientSession , nThreadIndex );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomUnavailable , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	int max_players = request.seat_cap();

	// 방 생성시에 0 인경우, 인원 최대값으로 셋팅
	if ( max_players == 0 ) {
		switch ( gameType )
		{
		case General::PlayCategory::PlayCategory_TexasHoldem:
			max_players = 9;
			break;
		}
	}

	bool createFlag = true;
	if ( false == NetLib::cSingleton<cGameRoomManager>::GetInstance()->OccupiedRoom( channelData , roomType , bettingRuleType ,
		show_down_delay_ms_per_player , show_down_community_delay_ms_per , pGameRoom , nThreadIndex , pContext->GetIP() , max_players , createFlag ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomUnavailable , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	//cheat
	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ischeat( pClientSession->GetPlayerIdx() ) )
	{
		General::ResultCode errorCode = pGameRoom->RoomJoinCheat( pClientSession );
	}

	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->isban( pClientSession->GetPlayerIdx() ) )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	response.set_ch_token( channelData.id() );
	response.set_chamber_no( pGameRoom->GetRoomNumber() );
	General::RoomSnapshot* pRoomInfo = response.mutable_chamber_info();
	pGameRoom->RoomInfoCopy( pRoomInfo );

	auto game = pGameRoom->GetGameInterface();
	switch ( gameType )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// 방에 진입 시킨다.
		General::ResultCode errorCode = pGameRoom->RoomJoin( pClientSession );
		if ( errorCode == General::ResultCode::Result_Success ) {
			std::vector<cClientSession*> players = pGameRoom->GetGameInterface()->GetPlayersSessionList();

			response.set_chamber_no( pGameRoom->GetRoomNumber() );
			response.mutable_chamber_info()->CopyFrom( pGameRoom->m_roomInfo );

			// 최근 기록을 response 에 복사
			auto histories = pGameRoom->GetGameInterface()->GetRecentlyPlayedGames();
			for ( auto history : histories ) {
				General::WinningHandHistory* pJokbo = response.mutable_chamber_info()->add_winning_history();
				pJokbo->CopyFrom( history );
			}

			// 방을 최초로 점유 한 경우 보스 결정하고 시작한다.
			pGameRoom->GetGameInterface()->SetBossFirst();
			response.set_lead_idx( pGameRoom->GetGameInterface()->GetBossPlayerIdx() );

			// 방장 결정
			pGameRoom->GetGameInterface()->SetMaster();
			response.set_captain_idx( pGameRoom->GetGameInterface()->GetMasterPlayerIdx() );

			for ( auto player : players ) {
				General::ParticipantProfile* pPlayer = response.add_members();
				if ( player != nullptr ) {
					player->CopyPlayer( pPlayer );
				}
				else {
					// 비어 있는 플레이어를 목록에 넣어주기 위해, 디폴트 인스턴스 생성
					General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
					pPlayer->CopyFrom( emptyPlayer );
				}
			}

			std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomJoin ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

			SendMessageAndLogWrite( General::ResultCode::Result_Success , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		}
		else {
			SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}
	break;
	}

	pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , _channel_id );
}

void cProtoMsgStub::FriendRoomCreate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	PmNet::MateChamberBuildRQ request;
	PmNet::MateChamberBuildRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cClientSession* pClientSession = GetSession( pContext );

	// 이미 방에 조인해 있는지 체크
	UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	if ( roomNumber != 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomAlreadyJoined , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string _channel_id = request.ch_token();

	// 데이터 파일 로딩
	auto channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( _channel_id );
	if ( channelData.id().empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 소유한 채널이 아니면 다른 로비 서버로 이동후 방을 생성하도록 한다.
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	if ( channelData.server_id() != configReader->SID_FOR_MANAGE ) {

		auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , channelData.server_id() );
		if ( server_info == nullptr ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		//GMsg_MoveLobby
		PmNet::ShiftAtriumRS _move_response;
		_move_response.set_atrium_node_ip( server_info->publicIpAddress );
		_move_response.set_endpoint( server_info->nPort );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );

		// 이동할 서버의 정보를 받아온다.
		/*E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeSpaceQuery , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_request.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}*/


		return;
	}

#ifdef CHANNEL_JOIN_CONDITION_CHECK

	// 채널 입장 조건 체크
	uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

	if ( channelData.money_min() > playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 홀덤 입장 조건 체크 디버그 모드일때 작동 안하도록 처리
	if ( channelData.money_max() < playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

#endif

	General::PlayCategory gameType = channelData.game_type();
	General::RoomAccessMode roomType = General::RoomAccessMode::RoomAccess_FriendOnly;	// 일반방, 친구방
	General::AssetKind moneyType = channelData.money_type();
	uint64 seedMoneyValue = channelData.seed_money();

	const General::BetPolicy& bettingRuleType = request.wager_rule();

	//const int& friendRoomNumber = NetLib::cSingleton<cGameRoomManager>::GetInstance()->CreateFriendRoomNumber( configReader->SID_FOR_MANAGE );
	const int& friendRoomNumber = NetLib::cSingleton<cGameRoomManager>::GetInstance()->CreateFriendRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->PopFriendRoom();
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomUnavailable , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	pGameRoom->SetRoomNum( friendRoomNumber );
	pGameRoom->SetRoomType( roomType );

	// 방 생성시에 0 인경우, 인원 최대값으로 셋팅
	int max_players = 0;
	switch ( gameType )
	{
	default:
	{
		SendMessageAndLogWrite( General::ResultCode::Result_FriendRoomProvisionFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	break;
	}

	bool createFlag = true;
	if ( false == NetLib::cSingleton<cGameRoomManager>::GetInstance()->OccupiedRoom( channelData , roomType , bettingRuleType ,
		0 , 0 , pGameRoom , nThreadIndex , pContext->GetIP() , max_players , createFlag ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomUnavailable , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	response.set_ch_token( channelData.id() );
	response.set_chamber_no( pGameRoom->GetRoomNumber() );
	General::RoomSnapshot* pRoomInfo = response.mutable_chamber_info();
	pGameRoom->RoomInfoCopy( pRoomInfo );

	auto game = pGameRoom->GetGameInterface();
	switch ( gameType )
	{
	}

}

void cProtoMsgStub::RoomList( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ChamberIndexRQ request;
	PmNet::ChamberIndexRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	General::PlayCategory gameType = request.match_kind();

	// 채널아이디는 한번에 두개 이상 요청하지 못합니다.
	/*if ( request.ch_tokens().size() > 2 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}*/
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

	auto distributions = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelDistribution();
	switch ( gameType )
	{
	
		//case Common::GameType::GameType_Roulette:
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	
	{
		auto mapPair = distributions.find( gameType );
		if ( mapPair == distributions.end() ) {
			//throw std::invalid_argument( "Table Game Type Distribution Error" );
			return;
		}

		// 소유한 채널이 아니면 다른 로비 서버에 목록을 요청 합니다.
		if ( mapPair->second != configReader->SID_FOR_MANAGE ) {

			Server::RoomListToLobbyReq server_request;
			auto add_request = server_request.mutable_request();
			add_request->CopyFrom( request );

			server_request.set_player_idx( pClientSession->GetPlayerIdx() );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( server_request.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::RoomListToLobby SerializeToArray Failed." ); return;
			}

			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeSpaceQuery , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_request.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			/*NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
			static_cast< NetLib::cIocpContext* >( pContext ) ,
			General::Packet_NodeSpaceQuery , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_request.ByteSizeLong() );*/

			// 다른 로비 서버로 처리를 위임
			return;
		}
	}
	break;
	}

	std::map<std::string , int> channel_ids;
	for ( auto channel : request.ch_tokens() ) {
		channel_ids.insert( std::pair<std::string , int>( channel , 0 ) );

		// channel id 가 잘못된 경우라면 처리 불가
		if ( channel.size() == 0 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		auto channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( channel );

		// 관리하는 채널이 아니면 위임을 보냄
		if ( channelData.server_id() != configReader->SID_FOR_MANAGE ) {

			Server::RoomListToLobbyReq server_request;
			auto add_request = server_request.mutable_request();
			add_request->CopyFrom( request );

			server_request.set_player_idx( pClientSession->GetPlayerIdx() );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( server_request.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false )
			{
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cProtoMsgStub::RoomListToLobby SerializeToArray Failed." );
				return;
			}

			NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket( E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeSpaceQuery , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_request.ByteSizeLong() );

			/*NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetSendPacket( E_SERVER_TYPE::LOBBY_SERVER ,
			static_cast< NetLib::cIocpContext* >( pContext ) ,
			General::Packet_NodeSpaceQuery , pGOOGLE_PROTO_BUFFER->SerializeBuffer , server_request.ByteSizeLong() );*/

			// 다른 로비 서버로 처리를 위임
			return;
		}
	}

	int pagingSize = request.page_sz() > 0 ? request.page_sz() : 10; // default size 10 으로 셋팅
	int startIndex = request.start_pos();

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

	SendMessageAndLogWrite( General::ResultCode::Result_Success , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );

#if defined(_DEBUG)

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );

#endif
}

void cProtoMsgStub::RoomJoin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	General::PacketID msgId = static_cast< General::PacketID >( nCommand );

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ChamberEnterRQ request;
	PmNet::ChamberEnterRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string _channel_id = request.ch_token();

	if ( request.chamber_no() == 0 ) {

		const int32 show_down_delay_ms_per_player = request.reveal_delay_per_member();
		const int32 show_down_community_delay_ms_per = request.reveal_shared_delay();

		Server::Channel channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( _channel_id );
		if ( channelData.id().empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 소유한 채널이 아니면 다른 로비 서버로 이동후 방을 생성하도록 한다.
		const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
		if ( channelData.server_id() != configReader->SID_FOR_MANAGE ) {

			auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , channelData.server_id() );
			if ( server_info == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//GMsg_MoveLobby
			PmNet::ShiftAtriumRS _move_response;
			_move_response.set_atrium_node_ip( server_info->publicIpAddress );
			_move_response.set_endpoint( server_info->nPort );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );

			return;
		}
	}
	else {

		// 친구 방은 로바만 생성가능
		// 1번 로비 서버로 보냅니다.
		const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

		if ( ( 999 < request.chamber_no() && request.chamber_no() < 10000 )
			&& ( 1 != configReader->SID_FOR_MANAGE ) ) {

			auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , 1 );
			if ( server_info == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//GMsg_MoveLobby
			PmNet::ShiftAtriumRS _move_response;
			_move_response.set_atrium_node_ip( server_info->publicIpAddress );
			_move_response.set_endpoint( server_info->nPort );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );

			return;

		}

		// 이 경우에 서버 아이디가 없으면 방 조인을 허락하지 않습니다.
		//if ( request.node_id() == 0 ) {
		//	SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}

		//if ( request.node_id() != configReader->SID_FOR_MANAGE ) {

		//	auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , request.node_id() );
		//	if ( server_info == nullptr ) {
		//		SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//	}

		//	//GMsg_MoveLobby
		//	PmNet::ShiftAtriumRS _move_response;
		//	_move_response.set_atrium_node_ip( server_info->publicIpAddress );
		//	_move_response.set_endpoint( server_info->nPort );
		//	GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );

		//	return;
		//}
	}


	// 이미 방에 조인해 있는지 체크
	UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	if ( roomNumber != 0 ) {
		General::ResultCode errorCode = General::ResultCode::Result_RoomAlreadyJoined;
		std::string enumString = protoutil::cProtoUtil::GetEnumString( errorCode );
		std::string errorString = std::format( "[ RoomJoin ] failed. {}." , enumString.c_str() );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , errorCode , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cGameRoom* pGameRoom = nullptr;
	uint32 uIp = pContext->GetIP(); // 같은 IP를 같은 방에 매칭 못하게 하기 위함
	bool occupied = false;

	// 방을 번호로 찾았지만
	// 방이 사라졌을 수도 있다.
	pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingRoom( request.chamber_no() , nThreadIndex );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomJoin ] failed. NoRoom." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomEntryTargetGone , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		//SendMessageAndLogWrite( General::ResultCode::Result_RoomEntryTargetGone , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	// 게임이 생성되지 않은 방이면 조인 실패
	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Chip )
	{
		if ( gameInstance->CheckSameCI( pClientSession->GetAccountGuid() ) && E_SERVER_STAGE::LOCAL != NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
		{
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomJoin ] failed. SameCI." );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_SameIdentityRoomEntryBlocked , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
			return;
		}
	}
	// 방번호로 조인하는 경우에는 채널 ID 를 클라에게서 받지 않는다.
	_channel_id = pGameRoom->GetGameInterface()->GetChannelId();

	Server::Channel channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( _channel_id );
	General::PlayCategory gameType = channelData.game_type();			// 로우바둑이, 홀덤
	General::RoomAccessMode roomType = General::RoomAccessMode::RoomAccess_Public;	// 일반방, 친구방
	General::AssetKind moneyType = channelData.money_type();
	uint64 seedMoneyValue = channelData.seed_money();
	Server::ChannelPlayMode channel_content_type = channelData.channel_content_type(); // 채널 컨텐츠 타입 ( 일반 , 라운지 구분 )

	// 채널 입장 조건 체크
	uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

	// 설정한 금액 이상만 입장 가능
	if ( channelData.money_min() > playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 설정 금액부터 입장 금지
	if ( channelData.money_max() <= playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( gameType )
	{
	default:
	{
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	break;
	}

	pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , _channel_id );
}

void cProtoMsgStub::MoveRoom( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ShiftChamberRQ request;
	PmNet::ChamberEnterRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	if ( request.ch_token().empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomEntryFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 이미 방에 조인해 있는지 체크
	UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	if ( roomNumber != 0 ) {
		General::ResultCode errorCode = General::ResultCode::Result_RoomAlreadyJoined;
		std::string enumString = protoutil::cProtoUtil::GetEnumString( errorCode );
		std::string errorString = std::format( "[ RoomJoin ] failed. {}." , enumString.c_str() );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , errorCode , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	Server::Channel channelData;
	std::string _channel_id = request.ch_token();
	const int32 show_down_delay_ms_per_player = request.reveal_delay_per_member();
	const int32 show_down_community_delay_ms_per = request.reveal_shared_delay();

	bool occupied = false;

	// 데이터 파일 로딩
	channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( _channel_id );
	if ( channelData.id().empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 소유한 채널이 아니면 다른 로비 서버로 이동후 방을 생성하도록 한다.
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	if ( channelData.server_id() != configReader->SID_FOR_MANAGE ) {

		auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , channelData.server_id() );
		if ( server_info == nullptr ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		//GMsg_MoveLobby
		PmNet::ShiftAtriumRS _move_response;
		_move_response.set_atrium_node_ip( server_info->publicIpAddress );
		_move_response.set_endpoint( server_info->nPort );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );
		return;
	}

	General::PlayCategory gameType = channelData.game_type();			// 로우바둑이, 홀덤
	General::RoomAccessMode roomType = General::RoomAccessMode::RoomAccess_Public;	// 일반방, 친구방
	General::AssetKind moneyType = channelData.money_type();
	uint64 seedMoneyValue = channelData.seed_money();
	Server::ChannelPlayMode channel_content_type = channelData.channel_content_type(); // 채널 컨텐츠 타입 ( 일반 , 라운지 구분 )

	// TODO Join 만 날리는 경우에는 Default BettingRuleType 을 결정하도록 한다.
	General::BetPolicy bettingRueType = General::BetPolicy::BetPolicy_HoldemStandard;

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->PopPlayingRoom( channelData , bettingRueType , pClientSession , nThreadIndex );
	if ( pGameRoom == nullptr ) {
		// Pop a room
		// 방이 없는 경우는 쓰레드에 방이 부족한 경우 일 수도 있다.
		// 요청자의 쓰레드를 이동시켜 버려 해소가 가능하다.
		pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->PopEmptyRoom( roomType , pClientSession , nThreadIndex );

		// 방 점유 상태가 아닌 경우 사용중인 방으로 변경
		if ( pGameRoom != nullptr ) {
			occupied = true;
			uint32 uIp = 0;
			bool createFlag = false; // false 면 다음 로직에서 인원 셋팅하도록 되어 있다.
			NetLib::cSingleton<cGameRoomManager>::GetInstance()->OccupiedRoom( channelData , roomType , bettingRueType ,
				show_down_delay_ms_per_player , show_down_community_delay_ms_per , pGameRoom , nThreadIndex , uIp ,
				NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiPlayerCount , createFlag );
		}
	}

	// 방을 못 찾았다.
	if ( pGameRoom == nullptr || pGameRoom->GetGameInterface() == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomJoin ] failed. NoRoom." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomUnavailable , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	// 바카라 이외의 게임은 방 진입
	auto game = pGameRoom->GetGameInterface();
	if ( pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Chip )
	{
		if ( game->CheckSameCI( pClientSession->GetAccountGuid() ) && E_SERVER_STAGE::LOCAL != NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
		{
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomJoin ] failed. SameCI." );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_SameIdentityRoomEntryBlocked , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
			return;
		}
	}
	switch ( gameType )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// 방이 대기 상태면 바로 JOIN 처리
		if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting )
		{
			// 채널 입장 조건 체크
			uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

			// 설정한 금액 이상만 입장 가능
			if ( channelData.money_min() > playerMoney ) {
				SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 설정 금액부터 입장 금지
			if ( channelData.money_max() <= playerMoney ) {
				SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 방에 진입 시킨다.
			General::ResultCode errorCode = pGameRoom->RoomJoin( pClientSession );
			if ( errorCode == General::ResultCode::Result_Success ) {
				std::vector<cClientSession*> players = pGameRoom->GetGameInterface()->GetPlayersSessionList();

				response.set_chamber_no( pGameRoom->GetRoomNumber() );
				response.mutable_chamber_info()->CopyFrom( pGameRoom->m_roomInfo );
				response.set_ch_token( channelData.id() );

				// 최근 기록을 response 에 복사
				auto histories = pGameRoom->GetGameInterface()->GetRecentlyPlayedGames();
				for ( auto history : histories ) {
					General::WinningHandHistory* pJokbo = response.mutable_chamber_info()->add_winning_history();
					pJokbo->CopyFrom( history );
				}

				// 방을 최초로 점유 한 경우 보스 결정하고 시작한다.
				if ( occupied ) {
					pGameRoom->GetGameInterface()->SetBossFirst();
					pGameRoom->GetGameInterface()->SetMaster();
				}

				response.set_lead_idx( pGameRoom->GetGameInterface()->GetBossPlayerIdx() );
				response.set_captain_idx( pGameRoom->GetGameInterface()->GetMasterPlayerIdx() );

				for ( auto player : players ) {
					General::ParticipantProfile* pPlayer = response.add_members();
					if ( player != nullptr ) {
						player->CopyPlayer( pPlayer );
					}
					else {
						// 비어 있는 플레이어를 목록에 넣어주기 위해, 디폴트 인스턴스 생성
						General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
						pPlayer->CopyFrom( emptyPlayer );
					}
				}

				// 로그 추가
				std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomJoin ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

				// 방의 상태와 남은 시간은 알려준다.
				PmNet::MatchStateSwapRS statusResponse;
				game->GetCurrentStatus( statusResponse );
				GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );

				// 방전체에 통보
				//pGameRoom->BroadCastToAllPlayer( static_cast< General::PacketID >( nCommand ) , response );
				pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );
			}
			else {
				SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}
		// TODO 관전 모드로 진입하면서, 플레이 참가 예약까지 되어야 한다.
		// 홀덤, 로우바둑이는 플레이 중인 유저가 없으면 관전 모드 허용하지 않는다.
		else
		{
			// 채널 입장 조건 체크
			uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

			// 설정한 금액 이상만 입장 가능
			if ( channelData.money_min() > playerMoney ) {
				SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 설정 금액부터 입장 금지
			if ( channelData.money_max() <= playerMoney ) {
				SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			switch ( channel_content_type )
			{
			case Server::ChannelPlayMode::ChannelPlayMode_Standard:
			default:
			{
				General::ResultCode errorCode = pGameRoom->RegisterWatcher( pClientSession );
				if ( errorCode == General::ResultCode::Result_Success ) {

					// 슬롯이 비어 있으면 예약을 시켜주고 끝낸다.
					// 플레이 + 플레이 예약자가 방에서 플레이 할 수 있는 MAX 값인가?
					int reservedCount = game->GetReservationPlayerPlayerCount();
					int playerCount = game->GetMemberCnt();
					int maxPlayerCount = pGameRoom->GetMaxRoomPlayerCnt();

					General::ResultCode errorCode = General::ResultCode::Result_UnexpectedCondition;
					if ( reservedCount + playerCount >= maxPlayerCount ) {
						SendMessageAndLogWrite( General::ResultCode::Result_SubSeatOccupied , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
					}

					// 자리 예약
					errorCode = game->SlotReservation( pClientSession );
					if ( errorCode != General::ResultCode::Result_Success ) {
						SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
					}

					bool is_watcher = true;
					pGameRoom->GetGameRoomDetail( response , is_watcher );

					std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ MoveRoom ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

					// 방의 상태와 남은 시간은 알려준다.
					PmNet::MatchStateSwapRS statusResponse;
					game->GetCurrentStatus( statusResponse );
					GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );

					// 방전체에 통보
					pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

					// 본인에게도 통보
					//GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_SpaceEnter , response , General::ResultCode::Result_Success , "" );

					game->OnIntruding( pClientSession );
				}
			}
			break;
			}
		}

	}
	break;
	}

	pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , _channel_id );
}

// 방나가기
// 플레이 중이면 나가기 예약, 그외에는 바로 나가기 가능
void cProtoMsgStub::RoomOut( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ChamberLeaveRQ request;
	PmNet::ChamberLeaveRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		response.set_member_idx( pClientSession->GetPlayerIdx() );
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOut ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}
	/*if ( pGameRoom->GetGameInterface()->GetGameStep() == Server::PlayPhase::PlayPhase_Judgement )
	{
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOut ] failed. GameSetp is Result." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLeaveFailed , errorString );
		return;
	}*/
	// cGameRoomManager 의 User UserGameRoomOut 을 사용하도록 수정한다.
	if ( false == NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( pClientSession ) ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOut ] Parsing failed. The data may be corrupted." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLeaveFailed , errorString );
		//cheat
		if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ischeat( pClientSession->GetPlayerIdx() ) )
		{
			pGameRoom->RemoveCheatUser( pClientSession->GetPlayerIdx() );
			/*pClientSession->SetJoinedRoomNumber( 0 );
			PmNet::ChamberLeaveRS _res;
			_res.set_chamber_no( roomNumber );
			_res.set_member_idx( pClientSession->GetPlayerIdx() );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_SpaceLeave , _res , General::ResultCode::Result_Success , "" );*/
			return;
		}
	}
	else
	{

	}

}

void cProtoMsgStub::RoomOutReserve( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ChamberLeaveHoldRQ request;
	PmNet::ChamberLeaveHoldRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOutReserve ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	if ( request.hold() && request.hold_revoke() || !request.hold() && !request.hold_revoke() ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOutReserve ] failed. InvalidOperation." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_ActionRejected , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	auto game = pGameRoom->GetGameInterface();
	{
		// RoomState_Waiting 이면 즉시 나감
		// RoomState_InPlay 이면 나가기 예약
		switch ( pGameRoom->GetRoomStatus() )
		{
		case General::RoomState::RoomState_InPlay:
			pGameRoom->RoomOutReservation( pClientSession->GetPlayerIdx() , pClientSession , request.hold() , request.hold_revoke() );
			break;
		default:
		{
			std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOutReserve ] failed. InvalidOperation." );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_ActionRejected , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		}
		break;
		}
	}


}

// 플레이를 시작하라. 호스트만 보낼 수 있다.
void cProtoMsgStub::PlayStart( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MatchKickoffRQ request;
	PmNet::MatchKickoffRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ PlayStart ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	IGame* pGame = pGameRoom->GetGameInterface();
	if ( pGame == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_PlayStartFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	General::ResultCode startResult = pGame->StartGame( pClientSession );
	if ( General::ResultCode::Result_Success != startResult ) {
		std::string enumString = protoutil::cProtoUtil::GetEnumString<General::ResultCode>( startResult );
		std::string errorString = std::format( "[ PlayStart ] failed. PlaytStartFailed. ErrorInfo {}" , enumString.c_str() );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , startResult , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	std::string errorString = "[ PlayStart ] Success RoomNumber [ " + std::to_string( roomNumber ) + " ]";
	errorString = protoutil::cProtoUtil::ErrorCodeString( errorString.c_str() );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
}

void cProtoMsgStub::PlayBet( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MatchWagerRQ request;
	PmNet::MatchWagerRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	const uint64& playerIdx = pClientSession->GetPlayerIdx();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ PlayBet ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	IGame* pGame = pGameRoom->GetGameInterface();
	if ( pGame == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ PlayBet ] failed. Game Interface is nullptr." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	// 블랙잭 이면 서브 플레이어의 베팅인지 확인이 필요
	{
		General::ResultCode betResult = pGame->UserBet( pClientSession , request );
		if ( General::ResultCode::Result_Success != betResult ) {
			General::TableAction requestBet = request.wager_kind();
			std::string betString = protoutil::cProtoUtil::GetEnumString<General::TableAction>( requestBet );
			std::string enumString = protoutil::cProtoUtil::GetEnumString<General::ResultCode>( betResult );
			std::string errorString = std::format( "[ PlayBet ] failed. User Betting [ {} ] Failed. ErrorInfo {}" , betString.c_str() , enumString.c_str() );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , betResult , errorString );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
			return;
		}

		/*std::string errorString = "[ PlayBet ] Success RoomNumber [ " + std::to_string( roomNumber ) + " ]";
		errorString = protoutil::cProtoUtil::ErrorCodeString( errorString.c_str() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );*/
	}


}


void cProtoMsgStub::GameParticipate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MatchEngageRQ request;
	PmNet::ChamberEnterRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	const int slotNumber = request.seat_no(); // 1 ~ 7 ( 바카라 )

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ GameParticipate ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	IGame* pGame = pGameRoom->GetGameInterface();
	if ( pGame == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ GameParticipate ] failed. Game Interface is nullptr." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	// 데이터 파일 로딩
	auto channelData = pGame->GetChannelData();
	if ( channelData.id().empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 채널 데이터를 클라에게 내려준다.
	response.set_ch_token( channelData.id() );

	// 채널 입장 조건 체크
	uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

	if ( channelData.money_min() > playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 홀덤 입장 조건 체크 디버그 모드일때 작동 안하도록 처리
	if ( channelData.money_max() < playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( pGameRoom->GetGameType() )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// 관전자 상태가 아니면 실패
		if ( false == pGameRoom->isWatcher( playerIdx ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_WatchModeRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 방상태가 대기 상태이고 진입 가능 상태이면
		if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting && General::ResultCode::Result_Success == pGameRoom->JoinPossible() )
		{
			// 방에 진입 시킨다.
			General::ResultCode errorCode = pGameRoom->RoomJoin( pClientSession , slotNumber );
			if ( errorCode == General::ResultCode::Result_Success )
			{
				pGameRoom->GetGameRoomDetail( response );

				std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ GameParticipate ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

				// Watcher 에 등록되어 있으면 삭제한다.
				pGameRoom->RemoveWatcher( playerIdx );

				// 방전체에 통보
				pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

				// 관전자 카운트 알림
				/*PmNet::InformObserverCntRS _watcher_res;
				_watcher_res.set_observer_cnt( pGameRoom->GetWatcherCnt() );
				pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

				// 참여한 플레이어 에게 남은 카드 통보
				pGame->LeftCardNoti( pClientSession );

				// 납입 시와 동일하게 패킷 발송
				// 커뮤니티 카드는 보내지 않도록 한다.
				//bool ignoreCommunity = true;
				//pGame->OnIntruding( pClientSession , ignoreCommunity );
				pGame->OnIntruding( pClientSession );

				// 방의 상태와 남은 시간은 알려준다.
				PmNet::MatchStateSwapRS statusResponse;
				pGame->GetCurrentStatus( statusResponse );
				GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );
			}
			else {
				SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}
		else
		{
			// 플레이 + 플레이 예약자가 방에서 플레이 할 수 있는 MAX 값인가?
			int reservedCount = pGame->GetReservationPlayerPlayerCount();
			int playerCount = pGame->GetMemberCnt();
			int maxPlayerCount = pGameRoom->GetMaxRoomPlayerCnt();

			General::ResultCode errorCode = General::ResultCode::Result_UnexpectedCondition;
			if ( reservedCount + playerCount < maxPlayerCount ) {
				// 자리 예약
				errorCode = pGame->SlotReservation( pClientSession );
			}
			else {
				// 빈자리 검색 실패시 우선 순위 예약
				errorCode = pGame->InsertParticipationQueue( pClientSession );
			}

			// 참여 예약
			//General::ResultCode errorCode = pGame->InsertParticipationQueue( pClientSession );

			// 예약 또는 우선순위 큐 등록 예약 실패
			if ( errorCode != General::ResultCode::Result_Success ) {
				SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			cHoldem* hgame = static_cast< cHoldem* >( pGame );
			if ( hgame->GetGameStep() == Server::PlayPhase::PlayPhase_Judgement || hgame->GetGameStep() == Server::PlayPhase::PlayPhase_HoldemShowdown )
				pGameRoom->GetGameRoomDetail( response , false );
			else
				pGameRoom->GetGameRoomDetail( response , true );

			std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ GameParticipate ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

			// Watcher 에 등록되어 있으면 삭제한다.
			//pGameRoom->RemoveWatcher( playerIdx );

			// 방전체에 통보
			pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );
			// 관전자 카운트 알림
			/*PmNet::InformObserverCntRS _watcher_res;
			int32_t watcher_count = pGameRoom->GetWatcherCnt() - reservedCount - 1;
			_watcher_res.set_observer_cnt( watcher_count < 0 ? 0 : watcher_count );
			pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/
			// 참여한 플레이어 에게 남은 카드 통보
			//pGame->LeftCardNoti( pClientSession );

			// 납입 시와 동일하게 패킷 발송
			//pGame->OnIntruding( pClientSession );

			// 방의 상태와 남은 시간은 알려준다.
			/*PmNet::MatchStateSwapRS statusResponse;
			pGame->GetCurrentStatus( statusResponse );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );*/
		}
	}
	break;
	}
}

// 참가 대기 취소 프로토콜 입니다.
void cProtoMsgStub::CancelGameParticipate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::RevokeMatchEngageRQ request;
	PmNet::RevokeMatchEngageRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	const int slotNumber = request.seat_no();

	response.set_member_idx( playerIdx );
	response.set_seat_no( slotNumber );

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ CancelGameParticipate ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	IGame* pGame = pGameRoom->GetGameInterface();
	if ( pGame == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ CancelGameParticipate ] failed. Game Interface is nullptr." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	// 플레이 중이면 실패 처리 한다.
	if ( General::RoomState::RoomState_Waiting == pGameRoom->GetRoomStatus() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( pGameRoom->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// 참가 대기 취소
		if ( pGame->RemoveParticipationQueue( pClientSession ) ) {
			pGameRoom->BroadCastToAllPlayer( General::Packet_RoundJoinCancel , response ); return;
		}

		// 착석 대기 예약 취소
		if ( pGame->RemoveReservation( pClientSession ) ) {

			// 방전체에 통보
			pGameRoom->BroadCastToAllPlayer( General::Packet_RoundJoinCancel , response ); return;
		}
	}
	break;
	}
}

void cProtoMsgStub::CancelBet( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::RevokeWagerRQ request;
	PmNet::RevokeWagerRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	IGame* pGame = pGameRoom->GetGameInterface();
	if ( pGame == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( pGame->GetGameType() )
	{
	}
}


// 플레이 상태에서 관전모드로 변경
void cProtoMsgStub::TrasferToWatcher( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ToObserverShiftRQ request;
	PmNet::ToObserverShiftRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 관전모드가 허락된 채널만 처리
	const auto& channelData = gameInstance->GetChannelData();
	if ( channelData.channel_content_type() == Server::ChannelPlayMode::ChannelPlayMode_Standard ) {
		SendMessageAndLogWrite( General::ResultCode::Result_WatchAllowedInLoungeOnly , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( gameInstance->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// 바둑이, 홀덤 => 1명뿐인 플레이어의 관전 모드 진입을 막는다.
		if ( pGameRoom->GetJoinedCnt() == 1 && pGameRoom->isPlayer( playerIdx ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_WatchTransferPlayerMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		if ( General::ResultCode::Result_Success != gameInstance->TransferWatcher( pClientSession ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}
	break;
	}

	std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, WatcherCount = {}, PlayerIdx = {} ] [ TrasferToWatcher ] Player Success." ,
		pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pGameRoom->GetWatcherCnt() , pClientSession->GetPlayerIdx() );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
}

void cProtoMsgStub::CancelTrasferToWatcher( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::RevokeToObserverShiftRQ request;
	PmNet::RevokeToObserverShiftRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 관전모드가 허락된 채널만 처리
	const auto& channelData = gameInstance->GetChannelData();
	if ( channelData.channel_content_type() == Server::ChannelPlayMode::ChannelPlayMode_Standard ) {
		SendMessageAndLogWrite( General::ResultCode::Result_WatchAllowedInLoungeOnly , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( gameInstance->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	
	
	{
		const auto& errorCode = gameInstance->CancelWatcherReservation( pClientSession );
		if ( errorCode != General::ResultCode::Result_Success ) {
			SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		response.set_member_idx( playerIdx );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_WatcherModeCancel , response , General::ResultCode::Result_Success , "" );
	}
	break;
	/*case Common::GameType::GameType_Roulette:
	{
		const auto& errorCode = gameInstance->CancelWatcherReservation( pClientSession );
		if ( errorCode != General::ResultCode::Result_Success ) {
			SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		response.set_member_idx( playerIdx );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_WatcherModeCancel , response , General::ResultCode::Result_Success , "" );
	}
	break;*/
	}

	std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, WatcherCount = {}, PlayerIdx = {} ] [ CancelTrasferToWatcher ] Player Success." ,
		pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pGameRoom->GetWatcherCnt() , pClientSession->GetPlayerIdx() );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
}

void cProtoMsgStub::RoomJoinAsWatcher( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );
	//pClientSession->GetPlayer().
	PmNet::ChamberEnterAsObserverRQ request;
	PmNet::ChamberEnterRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64& playerIdx = pClientSession->GetPlayerIdx();
	const UINT& roomNumber = pClientSession->GetJoinedRoomNumber();
	bool  isban = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->isban( pClientSession->GetPlayerIdx() );
	// 데이터 파일 로딩
	const std::string& _channel_id = request.ch_token();
	const auto& channelData = NetLib::cSingleton<cDataLoader>::GetInstance()->GetChannelById( _channel_id );

	// 방에 진입해 있는데 진입을 시도 했다.
	if ( roomNumber != 0 ) {
		//if(NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( pClientSession ))
		//	pClientSession->RoomOutReset();
		SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 손실한도 발생자
	if ( pClientSession->IsOverLostLimit() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( request.chamber_no() == 0 ) {

		if ( channelData.id().empty() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 소유한 채널이 아니면 다른 로비 서버로 이동후 방을 생성하도록 한다.
		const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
		if ( channelData.server_id() != configReader->SID_FOR_MANAGE ) {

			auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , channelData.server_id() );
			if ( server_info == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//GMsg_MoveLobby
			PmNet::ShiftAtriumRS _move_response;
			_move_response.set_atrium_node_ip( server_info->publicIpAddress );
			_move_response.set_endpoint( server_info->nPort );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );
			return;
		}
	}
	else {

		// 이 경우에 서버 아이디가 없으면 방 조인을 허락하지 않습니다.
		if ( request.node_id() == 0 ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

		if ( request.node_id() != configReader->SID_FOR_MANAGE ) {

			auto server_info = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->GetServerInfo( E_SERVER_TYPE::LOBBY_SERVER , request.node_id() );
			if ( server_info == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ChannelNodeDisconnected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			//GMsg_MoveLobby
			PmNet::ShiftAtriumRS _move_response;
			_move_response.set_atrium_node_ip( server_info->publicIpAddress );
			_move_response.set_endpoint( server_info->nPort );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_LobbyRedirect , _move_response , General::ResultCode::Result_Success , "" );

			return;
		}
	}


	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( request.chamber_no() );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomEntryTargetGone , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	IGame* pGameInstance = pGameRoom->GetGameInterface();
	if ( pGameInstance == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( channelData.channel_content_type() == Server::ChannelPlayMode::ChannelPlayMode_Standard ) {
		SendMessageAndLogWrite( General::ResultCode::Result_WatchAllowedInLoungeOnly , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}


const General::PlayCategory& gameType = pGameInstance->GetGameType();
	switch ( gameType )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{

		uint64 minmoney = pGameInstance->GetMinMoney();
		//if ( pGameInstance->GetGameStep() == General::RoomState::RoomState_Waiting && minmoney <= pClientSession->GetMoney(pGameRoom->m_roomInfo.money_type()) )
		if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting && minmoney <= pClientSession->GetMoney( pGameRoom->m_roomInfo.asset_kind() ) && !isban )
		{
			// 방에 진입 시킨다.
			General::ResultCode errorCode = pGameRoom->RoomJoin( pClientSession );
			if ( errorCode == General::ResultCode::Result_Success ) {
				pGameRoom->GetGameRoomDetail( response );

				// 로그 추가
				std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomJoin ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

				// 방의 상태와 남은 시간은 알려준다.
				PmNet::MatchStateSwapRS statusResponse;
				pGameInstance->GetCurrentStatus( statusResponse );
				GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );

				// 방전체에 통보
				pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

				// 관전자 카운트 알림
				/*PmNet::InformObserverCntRS _watcher_res;
				_watcher_res.set_observer_cnt( pGameRoom->GetWatcherCnt() );
				pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/
			}
			else {
				General::ResultCode errorCode = pGameRoom->RegisterWatcher( pClientSession );
				if ( errorCode == General::ResultCode::Result_Success ) {

					// 슬롯이 비어 있으면 예약을 시켜주고 끝낸다.
					// 플레이 + 플레이 예약자가 방에서 플레이 할 수 있는 MAX 값인가?
					int reservedCount = pGameInstance->GetReservationPlayerPlayerCount();
					int playerCount = pGameInstance->GetMemberCnt();
					int maxPlayerCount = pGameRoom->GetMaxRoomPlayerCnt();

					General::ResultCode errorCode = General::ResultCode::Result_UnexpectedCondition;
					if ( reservedCount + playerCount >= maxPlayerCount ) {
						//SendMessageAndLogWrite( General::ResultCode::Result_SubSeatOccupied , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
					}
					else
					{
						//General::RoomAccessMode roomType = pGameRoom->m_roomInfo.room_type();
						//uint64 seedMoney = pGameRoom->m_roomInfo.seed_money_value();
						if ( minmoney <= pClientSession->GetMoney( pGameRoom->m_roomInfo.asset_kind() ) )
						{
							// 자리 예약
							errorCode = pGameInstance->SlotReservation( pClientSession );
							if ( errorCode != General::ResultCode::Result_Success ) {
								SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
							}
						}
					}

					bool is_watcher = true;
					pGameRoom->GetGameRoomDetail( response , is_watcher );

					std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ MoveRoom ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

					// 방의 상태와 남은 시간은 알려준다.
					PmNet::MatchStateSwapRS statusResponse;
					pGameInstance->GetCurrentStatus( statusResponse );
					GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );

					// 방전체에 통보
					pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

					// 관전자 카운트 알림
					/*PmNet::InformObserverCntRS _watcher_res;
					_watcher_res.set_observer_cnt( pGameRoom->GetWatcherCnt() - reservedCount );
					pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

					// 본인에게도 통보
					//GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_SpaceEnter , response , General::ResultCode::Result_Success , "" );

					pGameInstance->OnIntruding( pClientSession );
				}
				else
				{
					SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}
			}
		}
		// TODO 관전 모드로 진입하면서, 플레이 참가 예약까지 되어야 한다.
		// 홀덤, 로우바둑이는 플레이 중인 유저가 없으면 관전 모드 허용하지 않는다.
		else
		{
			General::ResultCode errorCode = pGameRoom->RegisterWatcher( pClientSession );
			if ( errorCode == General::ResultCode::Result_Success ) {

				// 슬롯이 비어 있으면 예약을 시켜주고 끝낸다.
				// 플레이 + 플레이 예약자가 방에서 플레이 할 수 있는 MAX 값인가?
				int reservedCount = pGameInstance->GetReservationPlayerPlayerCount();
				int playerCount = pGameInstance->GetMemberCnt();
				int maxPlayerCount = pGameRoom->GetMaxRoomPlayerCnt();

				//General::ResultCode errorCode = General::ResultCode::Result_UnexpectedCondition;
				if ( reservedCount + playerCount >= maxPlayerCount ) {
					//SendMessageAndLogWrite( General::ResultCode::Result_SubSeatOccupied , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
				}
				else
				{
					General::RoomAccessMode roomType = pGameRoom->m_roomInfo.access_mode();
					//uint64 seedMoney = pGameRoom->m_roomInfo.seed_money_value();
					if ( minmoney <= pClientSession->GetMoney( pGameRoom->m_roomInfo.asset_kind() ) && !isban )
					{
						// 자리 예약
						errorCode = pGameInstance->SlotReservation( pClientSession );
						if ( errorCode != General::ResultCode::Result_Success ) {
							SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
						}
					}
				}

				bool is_watcher = true;
				pGameRoom->GetGameRoomDetail( response , is_watcher );

				std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ MoveRoom ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

				// 방의 상태와 남은 시간은 알려준다.
				PmNet::MatchStateSwapRS statusResponse;
				pGameInstance->GetCurrentStatus( statusResponse );
				GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );

				// 방전체에 통보
				pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

				// 관전자 카운트 알림
				/*PmNet::InformObserverCntRS _watcher_res;
				_watcher_res.set_observer_cnt( pGameRoom->GetWatcherCnt() - reservedCount );
				pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

				// 본인에게도 통보
				//GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_SpaceEnter , response , General::ResultCode::Result_Success , "" );

				pGameInstance->OnIntruding( pClientSession );
			}
			else
			{
				SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
		}

		// 플레이 + 플레이 예약자가 방에서 플레이 할 수 있는 MAX 값인가?
		//int reservedCount = pGameInstance->GetReservationPlayerPlayerCount();
		//int playerCount = pGameInstance->GetMemberCnt();
		//int maxPlayerCount = pGameRoom->GetMaxRoomPlayerCnt();

		//General::ResultCode errorCode = General::ResultCode::Result_UnexpectedCondition;
		//if ( reservedCount + playerCount < maxPlayerCount ) {
		//	// 자리 예약
		//	errorCode = pGameInstance->SlotReservation( pClientSession );
		//}
		//else
		//{

		//	 errorCode = pGameRoom->RegisterWatcher( pClientSession );
		//}
		////General::ResultCode errorCode = pGameRoom->RegisterWatcher( pClientSession );
		//if ( errorCode == General::ResultCode::Result_Success ) {
		//	std::vector<cClientSession*> players = pGameRoom->GetGameInterface()->GetPlayersSessionList();

		//	response.set_chamber_no( pGameRoom->GetRoomNumber() );
		//	response.mutable_chamber_info()->CopyFrom( pGameRoom->m_roomInfo );
		//	response.set_ch_token( channelData.id() );

		//	// 방 세부 정보 가져 오기
		//	bool is_watcher = true;
		//	pGameRoom->GetGameRoomDetail( response , is_watcher );

		//	// 최근 기록을 response 에 복사
		//	auto histories = pGameRoom->GetGameInterface()->GetRecentlyPlayedGames();
		//	for ( auto history : histories ) {
		//		General::WinningHandHistory* pJokbo = response.mutable_chamber_info()->add_win_history();
		//		pJokbo->CopyFrom( history );
		//	}

		//	/*response.set_lead_idx( pGameRoom->GetGameInterface()->GetBossPlayerIdx() );
		//	response.set_captain_idx( pGameRoom->GetGameInterface()->GetMasterPlayerIdx() );*/

		//	//for ( auto player : players ) {
		//	//	General::ParticipantProfile* pPlayer = response.add_members();
		//	//	if ( player != nullptr ) {
		//	//		player->CopyPlayer( pPlayer );
		//	//	}
		//	//	else {
		//	//		// 비어 있는 플레이어를 목록에 넣어주기 위해, 디폴트 인스턴스 생성
		//	//		General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
		//	//		pPlayer->CopyFrom( emptyPlayer );
		//	//	}
		//	//}

		//	std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomJoinAsWatcher ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
		//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		//	pGameInstance->OnIntruding( pClientSession );

		//	GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_SpaceEnter , response , General::ResultCode::Result_Success , errorString );
		//	//pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

		//	// RoomJoin 보다 먼저 날려봅니다.
		//	//pGameInstance->OnIntruding( pClientSession );
		//}
		//else {
		//	SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}if ( errorCode == General::ResultCode::Result_Success ) {
		//	std::vector<cClientSession*> players = pGameRoom->GetGameInterface()->GetPlayersSessionList();

		//	response.set_chamber_no( pGameRoom->GetRoomNumber() );
		//	response.mutable_chamber_info()->CopyFrom( pGameRoom->m_roomInfo );
		//	response.set_ch_token( channelData.id() );

		//	// 방 세부 정보 가져 오기
		//	bool is_watcher = true;
		//	pGameRoom->GetGameRoomDetail( response , is_watcher );

		//	// 최근 기록을 response 에 복사
		//	auto histories = pGameRoom->GetGameInterface()->GetRecentlyPlayedGames();
		//	for ( auto history : histories ) {
		//		General::WinningHandHistory* pJokbo = response.mutable_chamber_info()->add_win_history();
		//		pJokbo->CopyFrom( history );
		//	}

		//	/*response.set_lead_idx( pGameRoom->GetGameInterface()->GetBossPlayerIdx() );
		//	response.set_captain_idx( pGameRoom->GetGameInterface()->GetMasterPlayerIdx() );*/

		//	//for ( auto player : players ) {
		//	//	General::ParticipantProfile* pPlayer = response.add_members();
		//	//	if ( player != nullptr ) {
		//	//		player->CopyPlayer( pPlayer );
		//	//	}
		//	//	else {
		//	//		// 비어 있는 플레이어를 목록에 넣어주기 위해, 디폴트 인스턴스 생성
		//	//		General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
		//	//		pPlayer->CopyFrom( emptyPlayer );
		//	//	}
		//	//}

		//	std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ RoomJoinAsWatcher ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
		//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		//	pGameInstance->OnIntruding( pClientSession );

		//	GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_SpaceEnter , response , General::ResultCode::Result_Success , errorString );
		//	//pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

		//	// RoomJoin 보다 먼저 날려봅니다.
		//	//pGameInstance->OnIntruding( pClientSession );
		//}
		//else {
		//	SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		//}
	}
	break;
	}
	pClientSession->IncreaseRoomJoinCount();

	pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , _channel_id );
}

// 관전모드에서만 슬롯 예약이 가능하다.
// 자리 이동 카운트 다 써버리면 더 이상 사용이 불가능하다.
void cProtoMsgStub::MoveSlot( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::SeatSlideRQ request;
	PmNet::ChamberEnterRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	IGame* pGame = pGameRoom->GetGameInterface();
	if ( pGame == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 데이터 파일 로딩
	auto channelData = pGame->GetChannelData();
	if ( channelData.id().empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 채널 데이터를 클라에게 내려준다.
	response.set_ch_token( channelData.id() );

	// 채널 입장 조건 체크
	uint64 playerMoney = pClientSession->GetMoney( channelData.money_type() );

	if ( channelData.money_min() > playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 홀덤 입장 조건 체크 디버그 모드일때 작동 안하도록 처리
	if ( channelData.money_max() < playerMoney ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FundsEntryLimitExceeded , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 관전자가 아니면 실패
	if ( false == pGameRoom->isWatcher( playerIdx ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_WatchModeRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cHoldem* pHoldem = static_cast< cHoldem* >( pGame );
	if ( pHoldem == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	int slot_number_to_move = request.seat_to_slide();

	// 방상태가 대기 상태이고 진입 가능 상태이면
	if ( ( General::RoomState::RoomState_Waiting == pGameRoom->GetRoomStatus() ) && ( General::ResultCode::Result_Success == pGameRoom->JoinPossible() ) )
	{
		// 방에 진입 시킨다.
		General::ResultCode errorCode = pGameRoom->RoomJoin( pClientSession , slot_number_to_move );
		if ( errorCode == General::ResultCode::Result_Success )
		{
			pGameRoom->GetGameRoomDetail( response );

			std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ GameParticipate ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

			// Watcher 에 등록되어 있으면 삭제한다.
			pGameRoom->RemoveWatcher( playerIdx );

			// 방전체에 통보
			pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

			// 참여한 플레이어 에게 남은 카드 통보
			pGame->LeftCardNoti( pClientSession );

			// 납입 시와 동일하게 패킷 발송
			pGame->OnIntruding( pClientSession );

			// 방의 상태와 남은 시간은 알려준다.
			PmNet::MatchStateSwapRS statusResponse;
			pGame->GetCurrentStatus( statusResponse );
			GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );
		}
		else
		{
			SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			return;
		}
	}
	else
	{
		// 플레이 + 플레이 예약자가 방에서 플레이 할 수 있는 MAX 값인가?
		int reservedCount = pGame->GetReservationPlayerPlayerCount();
		int playerCount = pGame->GetMemberCnt();
		int maxPlayerCount = pGameRoom->GetMaxRoomPlayerCnt();

		General::ResultCode errorCode = General::ResultCode::Result_UnexpectedCondition;
		if ( reservedCount + playerCount < maxPlayerCount ) {
			// 자리 예약
			errorCode = pGame->SlotReservation( pClientSession , slot_number_to_move );
		}
		//else {
		//	// 빈자리 검색 실패시 우선 순위 예약
		//	errorCode = pGame->InsertParticipationQueue( pClientSession );
		//}

		// 예약 또는 우선순위 큐 등록 예약 실패
		if ( errorCode != General::ResultCode::Result_Success ) {
			SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		pGameRoom->GetGameRoomDetail( response );

		std::string errorString = std::format( "[ RoomNumber = {}, PlayerCount = {}, PlayerIdx = {} ] [ GameParticipate ] Player Success." , pGameRoom->GetRoomNumber() , pGameRoom->GetJoinedCnt() , pClientSession->GetPlayerIdx() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		// Watcher 에 등록되어 있으면 삭제한다.
		//pGameRoom->RemoveWatcher( playerIdx );

		// 방전체에 통보
		pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

		// 참여한 플레이어 에게 남은 카드 통보
		//pGame->LeftCardNoti( pClientSession );

		// 납입 시와 동일하게 패킷 발송
		//pGame->OnIntruding( pClientSession );

		// 방의 상태와 남은 시간은 알려준다.
		/*PmNet::MatchStateSwapRS statusResponse;
		pGame->GetCurrentStatus( statusResponse );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , "" );*/
	}
}


// 이패킷은 따로 응답하지 않습니다.
void cProtoMsgStub::ResultComplete( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	PmNet::OutcomeCompleteRQ request;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		return;
	}

	cClientSession* pClientSession = GetSession( pContext );

	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		return;
	}

	// 게임이 생성되지 않은 방이면 조인 실패
	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr ) {
		return;
	}

	switch ( gameInstance->GetGameType() )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		cHoldem* pHoldem = static_cast< cHoldem* >( gameInstance );

		if ( pHoldem->GetGameStep() == Server::PlayPhase::PlayPhase_Judgement )
			pHoldem->SetGameStepEnd();
	}
	break;
	default:
		return;
	}
}

void cProtoMsgStub::SendEmoticon( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::PushStickerRQ request;
	PmNet::PushStickerRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const int emoticonId = request.sticker_id();
	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	response.set_sticker_id( emoticonId );
	response.set_member_idx( playerIdx );

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	pGameRoom->BroadCastToAllPlayer( General::Packet_EmoteSend , response );
}

// 홀덤 래빗헌팅 요청 - 프리플랍 폴드로 판이 끝나 커뮤니티 카드가 없는 경우, 결과창에서 남은 덱을 눈요기로 공개한다.
void cProtoMsgStub::RabbitHunt( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::RabbitHuntRQ request;
	PmNet::RabbitHuntRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr || gameInstance->GetGameType() != General::PlayCategory::PlayCategory_TexasHoldem ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cHoldem* pHoldem = static_cast< cHoldem* >( gameInstance );

	std::vector<General::PlayingCard> rabbitCards;
	General::ResultCode errorCode = pHoldem->RabbitHunt( pClientSession , rabbitCards );
	if ( errorCode != General::ResultCode::Result_Success ) {
		SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	for ( const auto& card : rabbitCards ) {
		auto pCard = response.add_rabbit_cards();
		pCard->CopyFrom( card );
	}

	pGameRoom->BroadCastToAllPlayer( General::Packet_RabbitHunt , response );
}

// 홀덤 내 패 보여주기 요청 - 쇼다운 없이(상대 전원 폴드로) 이긴 유저가 결과창에서 자발적으로 자기 패를 공개한다.
void cProtoMsgStub::ShowHand( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ShowHandRQ request;
	PmNet::ShowHandRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr || gameInstance->GetGameType() != General::PlayCategory::PlayCategory_TexasHoldem ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cHoldem* pHoldem = static_cast< cHoldem* >( gameInstance );

	const General::HandRevealScope revealScope = request.reveal_scope();

	std::vector<General::PlayingCard> handCards;
	General::ResultCode errorCode = pHoldem->VoluntaryShowHand( pClientSession , revealScope , handCards );
	if ( errorCode != General::ResultCode::Result_Success ) {
		SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	response.set_member_idx( pClientSession->GetPlayerIdx() );
	response.set_reveal_scope( revealScope );
	for ( const auto& card : handCards ) {
		auto pCard = response.add_hand_tiles();
		pCard->CopyFrom( card );
	}

	pGameRoom->BroadCastToAllPlayer( General::Packet_ShowHand , response );
}

void cProtoMsgStub::KickOutPlayer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::ExpelMemberRQ request;
	PmNet::ExpelMemberRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	const uint64& playerIdx = pClientSession->GetPlayerIdx();
	const uint64& kickPlayerIdx = request.expel_member_idx();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ KickOutPlayer ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	IGame* pGameInterface = pGameRoom->GetGameInterface();
	if ( pGameInterface == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ KickOutPlayer ] failed. Game Interface is nullptr." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	switch ( pGameInterface->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
		break;
	default: {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	}

	// 자기 자신은 강퇴 못합니다.
	if ( request.expel_member_idx() == pClientSession->GetPlayerIdx() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SelfKickRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 티켓 체크
	const uint32 tickets = pClientSession->GetKickOutTicketCount();
	if ( tickets < 1 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_KickTicketInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 방장이 아니면 실행 불가
	if ( false == pGameInterface->isMaster( pClientSession ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_KickHostPermissionRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 이미 강제 퇴장 등록된 플레이어 인지 확인
	if ( pGameInterface->isKickedPlayer( kickPlayerIdx ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_PlayerAlreadyKicked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cClientSession* pKickOutSesseion = pGameInterface->GetPlayerALLSession( kickPlayerIdx );
	if ( pKickOutSesseion == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	//강퇴권쿨타임이 안지났을경우
	std::string cool = pClientSession->GetPlayerExtRef().kickout_cooldown_until();
	std::time_t now = std::time( nullptr );
	std::time_t s_cool = TimeUtils::StringToTimeTM( cool );
	if ( s_cool > now )
	{
		response.set_expel_cooldown( pClientSession->GetPlayerExt().kickout_cooldown_until() );
		SendMessageAndLogWrite( General::ResultCode::Result_KickCooldownActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	response.set_expel_member_idx( kickPlayerIdx );
	response.set_expel_alias( pKickOutSesseion->GetNickName() );

	pClientSession->GetPlayerExtRef().set_kickout_cooldown_until( TimeUtils::GetTimeOneHourLater() );

	switch ( pGameInterface->GetGameType() )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		auto holdem = static_cast< cHoldem* >( pGameInterface );

		// 방이 대기 상태이면 바로 Kick
		if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting ) {

			// 바로 나가는 플레이어에게도 KickOut 전송
			std::string errorMessage;
			pGameRoom->SendRequest( pKickOutSesseion , General::Packet_UserRemove , response , General::ResultCode::Result_Success , errorMessage );

			NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( pKickOutSesseion );
		}
		else// 플레이도중
		{
			if ( pKickOutSesseion->isDie() || pGameRoom->isWatcher( kickPlayerIdx ) )//다이일경우 바로나가기 
			{
				// 바로 나가는 플레이어에게도 KickOut 전송
				std::string errorMessage;
				pGameRoom->SendRequest( pKickOutSesseion , General::Packet_UserRemove , response , General::ResultCode::Result_Success , errorMessage );
				NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( pKickOutSesseion );
			}
			else
			{
				holdem->RegisterKickPlayer( kickPlayerIdx );
			}
		}
	}
	break;
	}

	// Kick Out 변동 로그
	cAssetLog assetLogInstance( pClientSession , 30301 );

	// kick out ticket 소모
	pClientSession->UseKickOutTicketCount();

	if ( pClientSession->PlayerUpdateAsync() ) {

		// 입장 금지 플레이어 등록
		pGameInterface->AddJoinProhibited( kickPlayerIdx );

		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_UserRemove , response , General::ResultCode::Result_Success , "" );
	}
}

void cProtoMsgStub::CancelKickOutPlayer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::RevokeExpelMemberRQ request;
	PmNet::RevokeExpelMemberRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	const UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	const uint64& playerIdx = pClientSession->GetPlayerIdx();
	const uint64& kickPlayerIdx = request.expel_member_idx();

	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
	if ( pGameRoom == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ CancelKickOutPlayer ] failed. FindRoomFailed." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	IGame* pGameInterface = pGameRoom->GetGameInterface();
	if ( pGameInterface == nullptr ) {
		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ CancelKickOutPlayer ] failed. Game Interface is nullptr." );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
		return;
	}

	switch ( pGameInterface->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
		break;
	default: {
		SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	}

	// 자기 자신은 강퇴 못합니다.
	if ( request.expel_member_idx() == pClientSession->GetPlayerIdx() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SelfKickRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 티켓 체크
	/*const uint32 tickets = pClientSession->GetKickOutTicketCount();
	if ( tickets < 1 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_KickTicketInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}*/

	// 방장이 아니면 실행 불가
	if ( false == pGameInterface->isMaster( pClientSession ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_KickHostPermissionRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 강제 퇴장인 플레이어 인지 확인
	if ( false == pGameInterface->isKickedPlayer( kickPlayerIdx ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_KickTargetNotMarked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	switch ( pGameInterface->GetGameType() )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		auto holdem = static_cast< cHoldem* >( pGameInterface );

		// 방이 대기 상태 이면 이미 강퇴가 되었다고 판단한다.
		if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		holdem->CancelKickPlayer( kickPlayerIdx );
	}
	break;
	}

	pClientSession->GetPlayerExtRef().set_kickout_cooldown_until( TimeUtils::GetCurrentDateTime() );
	// Kick Out 변동 로그
	cAssetLog assetLogInstance( pClientSession , 30301 );

	// kick out ticket 도로 돌려주자....
	uint32 current_kickout_ticket = pClientSession->GetKickOutTicketCount();

	pClientSession->SetKickOutTicketCount( current_kickout_ticket + 1 );

	if ( pClientSession->PlayerUpdateAsync() ) {

		// 입장 금지 플레이어 등록
		pGameInterface->CancelJoinProhibited( kickPlayerIdx );

		response.set_expel_member_idx( kickPlayerIdx );

		GetOwner()->SendBuffer( pContext , nThreadIndex , General::Packet_UserRemoveCancel , response , General::ResultCode::Result_Success , "" );
	}
}

//void cProtoMsgStub::RoomTryVote( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
//{
//	if ( CheckSession( pContext ) ) return;
//
//	cClientSession* pClientSession = GetSession( pContext );
//
//	PmNet::ChamberPollAttemptRQ request;
//	PmNet::ChamberPollAttemptRS response;
//	response.set_ctx_buf( request.ctx_buf() );
//	if ( !request.ParseFromArray( pData , nLength ) ) {
//		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
//	}
//
//	// 자기 자신에게 투표 할 수 없다.
//	const uint64 issuerPlayerIdx = request.poll_target_idx();
//	if ( issuerPlayerIdx == pClientSession->GetPlayerIdx() ) {
//		SendMessageAndLogWrite( General::ResultCode::Result_SelfBallotRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
//	}
//
//	const int roomNumber = pClientSession->GetJoinedRoomNumber();
//	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
//	if ( pGameRoom == nullptr ) {
//		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomTryVote ] failed. FindRoomFailed." );
//		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//		return;
//	}
//
//	IGame* gameInterface = pGameRoom->GetGameInterface();
//	if ( gameInterface == nullptr ) {
//		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomTryVote ] failed. Game Interface is nullptr." );
//		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//		return;
//	}
//
//	switch ( request.poll_kind() )
//	{
//	case General::BallotKind::BallotKind_KickOut:
//	{
////#ifndef _DEBUG
//		// 플레이어가 4인 이상 인가?
//		if ( pGameRoom->GetJoinedCnt() < 4 ) {
//			SendMessageAndLogWrite( General::ResultCode::Result_BallotPlayerCountInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
//		}
////#endif
//
//		// 플레이 카운트가 3번 이상인가?
//		if ( pClientSession->GetRoomPlayingCount() < 3 ) {
//			SendMessageAndLogWrite( General::ResultCode::Result_BallotPlayCountInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
//		}
//
//		const int32 playerCount = gameInterface->GetMemberCnt();
//		General::ResultCode errorCode = gameInterface->StartVote( General::BallotKind::BallotKind_KickOut, playerCount , issuerPlayerIdx , pClientSession->GetPlayerIdx() );
//		if ( errorCode != General::ResultCode::Result_Success ) {
//			SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
//		}
//
//		// 방이 대기 상태 이면 즉시 실행한다.
//		if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting ) {
//			auto vote = gameInterface->GetCurrentVote();
//			if ( vote != nullptr ) {
//				int32 voteIndex = vote->GetVoteIndex();
//				gameInterface->VoteActionByPlayerVote( voteIndex );
//			}
//		}
//	}
//	break;
//	default:
//	{
//		SendMessageAndLogWrite( General::ResultCode::Result_BallotTypeUnknown , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
//		return;
//	}
//	}
//
//	
//}
//
//void cProtoMsgStub::RoomVoteAnswer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
//{
//	if ( CheckSession( pContext ) )
//		return;
//
//	cClientSession* pClientSession = GetSession( pContext );
//
//	PmNet::ChamberPollReplyRQ request;
//	PmNet::ChamberPollReplyRS response;
//	response.set_ctx_buf( request.ctx_buf() );
//	if ( !request.ParseFromArray( pData , nLength ) ) {
//		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
//		return;
//	}
//
//	const int roomNumber = pClientSession->GetJoinedRoomNumber();
//	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
//	if ( pGameRoom == nullptr ) {
//		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomVoteAnswer ] failed. FindRoomFailed." );
//		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_RoomLookupFailed , errorString );
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//		return;
//	}
//
//	IGame* gameInterface = pGameRoom->GetGameInterface();
//	if ( gameInterface == nullptr ) {
//		std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomVoteAnswer ] failed. Game Interface is nullptr." );
//		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_UnexpectedCondition , errorString );
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//		return;
//	}
//
//	// vote index 로 검색한다.
//	const int32 voteIndex = request.poll_idx();
//	auto vote = gameInterface->GetVote( voteIndex );
//	if ( vote == nullptr ) {
//		SendMessageAndLogWrite( General::ResultCode::Result_BallotRecordMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
//		return;
//	}
//
//	if ( false == gameInterface->PlayerVote( voteIndex , pClientSession->GetPlayerIdx() , request.agree_flag() ) ) {
//		SendMessageAndLogWrite( General::ResultCode::Result_BallotSubmitFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
//		return;
//	}
//
//	PmNet::ChamberPollReplyRS _res;
//	_res.set_poll_target_idx( vote->GetIssuerPlayerIdx() );
//	_res.set_poll_idx( voteIndex );
//	_res.set_yes_cnt( vote->GetAgreeCount() );
//	_res.set_no_cnt( vote->GetAgainstCount() );
//	_res.set_ok_flag( vote->isSuccessVote() );
//	pGameRoom->BroadCastToAllPlayer( Common::GMsg_RoomVoteAnswer , _res );
//
//	// 과반이 넘었는지 확인해서 투표의 액션을 실행한다.
//	if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting ) {
//		gameInterface->VoteActionByPlayerVote( voteIndex );
//	}
//}

void cProtoMsgStub::PlayerSetAvatar( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MemberApplySkinRQ request;
	PmNet::MemberApplySkinRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	if ( false == pClientSession->hasAvatar( request.skin_id() ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AvatarNotOwned , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::future<BOOL> result = pClientSession->PlayerSetAvatar( request.skin_id() );
	result.wait();

	if ( result.get() )
	{
		const auto& avatar = pClientSession->SetAvatar( request.skin_id() );

		auto add_avatar = response.mutable_skin();
		add_avatar->CopyFrom( avatar );

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		std::string errorString = "[ PlayerSetAvatar PlayerIdx {} ]" + std::to_string( pClientSession->GetPlayerIdx() );
		errorString += "[ Avatar Id {} }" + std::to_string( request.skin_id() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
	else
	{
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_StorageFault , "" );

		std::string errorString = "[ PlayerSetAvatar Update Failed PlayerIdx {} ]" + std::to_string( pClientSession->GetPlayerIdx() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
}

void cProtoMsgStub::PlayerSetSubPasswd( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MemberApplyAuxKeyRQ request;
	PmNet::MemberApplyAuxKeyRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string sub_passwd = request.aux_key();

	// 패스워드 정책에 어긋남
	if ( false == MADEPlatform::isValidPassword( sub_passwd ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyPolicyBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 암호화 ( 복호화도 동일한 함수 사용 )
	//std::string _encryptionPasswd = cXorEncryption::EncryptDecrypt( sub_passwd );

	std::string _account_guid = pClientSession->GetAccountGuid();
	std::string _platform_guid = pClientSession->GetPlatformGuid();

	if ( _account_guid.empty() || _platform_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LoginCredentialRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// sha256 해싱
	std::string _hashPasswd = MADEPlatform::GenerateSha256( sub_passwd );

	if ( QueryManager::UpdateSubPassword( _account_guid , _platform_guid , _hashPasswd ) )
	{
		//10501 작성
		int code = 10501;

		//std::string ipString = ::ConvertIP( pContext->GetIP() );

		string joinTime;
		std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( pClientSession->GetAccountGuid() , pClientSession->GetPlatformGuid() , joinTime );
		player_join_time.wait();

		if ( FALSE == player_join_time.get() ) {
		}

		auto log_result = QueryManager::InsertAccountLog(
			code , // code
			_platform_guid , // platform_guid , // uid
			"" , // adminid
			"" , // adid
			"" , // asset
			"" , // cmd
			"" , // request.dev_meta() , // device
			"" , // gmsessid
			"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
			pClientSession->GetIp() , // ipaddr
			"" , // request.os_ver() , // osver
			"" , // marketString , // pf
			"" , // svcuid
			_account_guid , // accountid
			0 , // chip
			0 , // chip_g
			0 , // chip_s
			0 , // coin
			0 , // coin_g
			0 , // coin_s
			0 , // slotcoin
			0 , // gem
			0 , // gem_f
			0 , // gem_p
			0 , // kickoutticket
			0 , // exp
			0 , // friend_cnt
			joinTime , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
			"" , // logintype
			"UPDATE" , // type
			" " , // nickname
			"" , // deletetime
			"" , // registered
			0 , // block_hour
			0 , // left_count
			0 , // limit_type
			"" , // reqtm
			"" , // second_pw
			"" , // reason
			"" , // result
			"" , // hash
			pClientSession->GetGameVersion() , // ver
			"EMPTY" ); // etc
		log_result.wait();

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		std::string errorString = std::format( "[ PlayerSetSubPasswd Success PlayerIdx {} ]" , std::to_string( pClientSession->GetPlayerIdx() ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
	else
	{
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_StorageFault , "" );

		std::string errorString = std::format( "[ PlayerSetSubPasswd Failed PlayerIdx {} ]" , std::to_string( pClientSession->GetPlayerIdx() ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
}

void cProtoMsgStub::PlayerChangeSubPasswd( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MemberSwapAuxKeyRQ request;
	PmNet::MemberSwapAuxKeyRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const std::string& _account_guid = pClientSession->GetAccountGuid();
	const std::string& _platform_guid = pClientSession->GetPlatformGuid();

	std::string input_sub_passwd = request.cur_aux_key();

	// sha256 해싱
	std::string _input_hashPasswd = MADEPlatform::GenerateSha256( input_sub_passwd );

	// 현재 패스워드 읽어오기
	int _sub_passwd_retry_count = 0;
	std::string _current_sub_password;
	std::string _sub_password_update_time = "";
	std::future<BOOL> result = QueryManager::FindSubPasswordByPlatformGuidAsync( _platform_guid , _current_sub_password , _sub_passwd_retry_count , _sub_password_update_time );
	result.wait();
	if ( result.get() )
	{
		// 현재 패스워드가 동일한지 비교
		if ( false == _current_sub_password._Equal( _input_hashPasswd ) ) {
			SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
	}
	else
	{
		// DB Error
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 입력 받은 패스워드 정책 확인
	std::string input_new_sub_passwd = request.new_aux_key();
	if ( FALSE == MADEPlatform::isValidPassword( input_new_sub_passwd ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LocalAccessSecretPolicyBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}


	// sha256 해싱
	std::string _input_new_hashPasswd = MADEPlatform::GenerateSha256( input_new_sub_passwd );

	if ( true == _current_sub_password._Equal( _input_new_hashPasswd ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SecondKeySameAsBefore , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	if ( QueryManager::UpdateSubPassword( _account_guid , _platform_guid , _input_new_hashPasswd ) )
	{
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		std::string errorString = std::format( "[ PlayerChangeSubPasswd Success PlayerIdx {} ]" , std::to_string( pClientSession->GetPlayerIdx() ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		//10501 작성
		int code = 10501;

		//std::string ipString = ::ConvertIP( pContext->GetIP() );

		auto log_result = QueryManager::InsertAccountLog(
			code , // code
			_platform_guid , // platform_guid , // uid
			"" , // adminid
			"" , // adid
			"" , // asset
			"" , // cmd
			"" , // request.dev_meta() , // device
			"" , // gmsessid
			"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
			pClientSession->GetIp() , // ipaddr
			"" , // request.os_ver() , // osver
			"" , // marketString , // pf
			"" , // svcuid
			_account_guid , // accountid
			0 , // chip
			0 , // chip_g
			0 , // chip_s
			0 , // coin
			0 , // coin_g
			0 , // coin_s
			0 , // slotcoin
			0 , // gem
			0 , // gem_f
			0 , // gem_p
			0 , // kickoutticket
			0 , // exp
			0 , // friend_cnt
			pClientSession->GetJoinTime() , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
			"" , // logintype
			"UPDATE" , // type
			" " , // nickname
			"" , // deletetime
			"" , // registered
			0 , // block_hour
			0 , // left_count
			0 , // limit_type
			"" , // reqtm
			"" , // second_pw
			"" , // reason
			"" , // result
			"" , // hash
			pClientSession->GetGameVersion() , // ver
			"EMPTY" ); // etc
		log_result.wait();
	}
	else
	{
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_StorageFault , "" );

		std::string errorString = std::format( "[ PlayerChangeSubPasswd Failed PlayerIdx {} ]" , std::to_string( pClientSession->GetPlayerIdx() ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	}
}
//직접해지
void cProtoMsgStub::PlayerRemoveSubPasswd( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MemberDropAuxKeyRQ request;
	PmNet::MemberDropAuxKeyRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::string sub_passwd = request.cur_aux_key();
	if ( sub_passwd.empty() && sub_passwd.size() > 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyInputRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string input_sub_paswd = MADEPlatform::GenerateSha256( sub_passwd );

	std::string _account_guid = pClientSession->GetAccountGuid();
	std::string _platform_guid = pClientSession->GetPlatformGuid();

	if ( _account_guid.empty() || _platform_guid.empty() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_LoginCredentialRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 현재 패스워드 읽어오기
	int _sub_passwd_retry_count = 0;
	std::string _current_sub_password;
	std::string _sub_password_update_time = "";
	std::future<BOOL> result = QueryManager::FindSubPasswordByPlatformGuidAsync( _platform_guid , _current_sub_password , _sub_passwd_retry_count , _sub_password_update_time );
	result.wait();
	if ( result.get() )
	{
		if ( _sub_passwd_retry_count >= 5 ) {
			response.set_aux_key_miss_cnt( _sub_passwd_retry_count );
			SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 현재 패스워드가 동일한지 비교
		if ( false == _current_sub_password._Equal( input_sub_paswd ) ) {
			++_sub_passwd_retry_count;
			response.set_aux_key_miss_cnt( _sub_passwd_retry_count );

			// 2차 비밀번호가 틀렸습니다.
			if ( _sub_passwd_retry_count < 5 )
			{
				SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			}
			else
			{
				SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyResetRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			}

			//10502 작성

			int code = 10502;

			int subpasswd_retrycnt = _sub_passwd_retry_count > 5 ? 5 : _sub_passwd_retry_count;
			//std::string ipString = ::ConvertIP( pContext->GetIP() );

			auto log_result = QueryManager::InsertAccountLog(
				code , // code
				_platform_guid , // platform_guid , // uid
				"" , // adminid
				"" , // adid
				"" , // asset
				"" , // cmd
				pClientSession->GetDeviceInfo() , // request.dev_meta() , // device
				"" , // gmsessid
				"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
				pClientSession->GetIp() , // ipaddr
				pClientSession->GetOSInfo() , // request.os_ver() , // osver
				"" , // marketString , // pf
				"" , // svcuid
				_account_guid , // accountid
				0 , // chip
				0 , // chip_g
				0 , // chip_s
				0 , // coin
				0 , // coin_g
				0 , // coin_s
				0 , // slotcoin
				0 , // gem
				0 , // gem_f
				0 , // gem_p
				0 , // kickoutticket
				0 , // exp
				0 , // friend_cnt
				"" , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
				"" , // logintype
				"" , // type
				" " , // nickname
				"" , // deletetime
				"" , // registered
				0 , // block_hour
				0 , // left_count
				0 , // limit_type
				"" , // reqtm
				"" , // second_pw
				"" , // reason
				"" , // result
				"" , // hash
				pClientSession->GetGameVersion() , // ver
				std::to_string( subpasswd_retrycnt ) ); // etc

			log_result.wait();


			QueryManager::UpdateSubPasswordRetryCount( _account_guid , _platform_guid , _sub_passwd_retry_count );
			return;
		}
	}
	else
	{
		// DB Error
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 리트라이 카운트를 초기화 해준다.
	if ( FALSE == QueryManager::UpdateSubPasswordRetryCount( _account_guid , _platform_guid , 0 ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 2차 비밀번호를 초기화 해준다.
	if ( FALSE == QueryManager::RemoveSubPasswordRetryCount( _platform_guid ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_StorageFault , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	//10501 작성

	//std::string ipString = ::ConvertIP( pContext->GetIP() );

	string joinTime;
	std::future<BOOL> player_join_time = QueryManager::GetPlayerJoinTimeAsync( pClientSession->GetAccountGuid() , pClientSession->GetPlatformGuid() , joinTime );
	player_join_time.wait();

	if ( FALSE == player_join_time.get() ) {
	}

	int code = 10501;
	auto log_result = QueryManager::InsertAccountLog(
	code , // code
	_platform_guid , // platform_guid , // uid
	"" , // adminid
	"" , // adid
	"" , // asset
	"" , // cmd
	"" , // request.dev_meta() , // device
	"" , // gmsessid
	"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
	pClientSession->GetIp() , // ipaddr
	"" , // request.os_ver() , // osver
	"" , // marketString , // pf
	"" , // svcuid
	_account_guid , // accountid
	0 , // chip
	0 , // chip_g
	0 , // chip_s
	0 , // coin
	0 , // coin_g
	0 , // coin_s
	0 , // slotcoin
	0 , // gem
	0 , // gem_f
	0 , // gem_p
	0 , // kickoutticket
	0 , // exp
	0 , // friend_cnt
	joinTime , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
	"" , // logintype
	"REMOVE" , // type
	" " , // nickname
	"" , // deletetime
	"" , // registered
	0 , // block_hour
	0 , // left_count
	0 , // limit_type
	"" , // reqtm
	"" , // second_pw
	"" , // reason
	"" , // result
	"" , // hash
	pClientSession->GetGameVersion() , // ver
	_sub_password_update_time ); // etc // 원래 REMOVE 할때 2차비번설정시간을 저장해야하는데 저장하는곳이없어서 일단 EMPTY로 두기로했음 2024 07 30 01:22
	log_result.wait();

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::UpdateLobby( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	// 패킷 데이터 안전성 검증
	//if ( pData == nullptr || nLength == 0 || nLength > 1048576 ) { // 1MB 제한
	//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , 
	//		std::format( "UpdateLobby: Invalid packet data. pData={}, nLength={}", 
	//			static_cast<void*>(pData), nLength ).c_str() );
	//	return;
	//}

	cClientSession* pClientSession = GetSession( pContext );
	if ( pClientSession == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "UpdateLobby: pClientSession is nullptr" );
		return;
	}

	PmNet::RefreshAtriumRQ request;
	PmNet::RefreshAtriumRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// Daily 초기화 처리
	BOOL updateMailBox;
	pClientSession->UpdateLobby( updateMailBox );
	pClientSession->SetEsterEgg( false );
	// 메일 박스 보상 알림
	if ( updateMailBox ) {
		response.set_inbox_refresh( updateMailBox );
	}

	if ( pClientSession->GetJoinedRoomNumber() == 0 )
		pClientSession->UpdateRefillMoney( response );
	if ( response.refuel_stacks() > 0 ) {
		cAssetLog assetLogInstance( pClientSession , 30201 );

		int max_chip_refill_count = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillDailyCount );
		uint64 refill_chip_limit = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillThreshold );

		uint64 refill_chips = max_chip_refill_count - response.stack_refuel_left();

		assetLogInstance.SetRefill( response.refuel_stacks() , refill_chips , max_chip_refill_count , response.stack_refuel_left() );
	}
	else if ( response.refuel_tokens() > 0 ) {
		cAssetLog assetLogInstance( pClientSession , 30202 );

		int max_coin_refill_count = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_CoinRefillDailyCount );
		uint64 refill_coin_limit = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_CoinRefillThreshold );

		uint64 refill_coin = max_coin_refill_count - response.token_refuel_left();

		assetLogInstance.SetRefill( response.refuel_tokens() , refill_coin , max_coin_refill_count , response.token_refuel_left() );
	}

	// 일일미션, 라운지 미션 갱신 처리
	pClientSession->RequestMissionAndAchieve( General::TaskCategory::TaskCategory_Mission , response.mutable_daily_goals() );
	pClientSession->RequestMissionAndAchieve( General::TaskCategory::TaskCategory_LoungeMission , response.mutable_atrium_goals() );
	pClientSession->RequestMissionAndAchieve( General::TaskCategory::TaskCategory_Achievement , response.mutable_trophies() );

	// MemberShipClass Expire처리
	General::BenefitTier t_class = General::BenefitTier::BenefitTier_None;
	General::ParticipantProfile _player = pClientSession->GetPlayer();
	if ( _player.membership_tier() != General::BenefitTier::BenefitTier_Basic )
	{
		if ( pClientSession->MemberShipExpired() ) {
			t_class = _player.membership_tier();
			pClientSession->SetMemberShipClass( General::BenefitTier::BenefitTier_Basic );
			response.set_tier_stale( true );

			// 초과 재화 우편 지급
			pClientSession->SetExpiredCoin( General::PlayCategory::PlayCategory_None , pClientSession->GetCoin() );
			pClientSession->SetExpiredChip( General::PlayCategory::PlayCategory_None );

			pClientSession->PlayerUpdateAsync();
		}
	}

	// Lobby 에서 갱신 해줄 전적 관련 데이터 처리
	auto records = response.mutable_ledger();
	pClientSession->CopyRecords( records );

	//pClientSession->GetPlayer().set_wallet_coins( 100000000 );
	//pClientSession->GetPlayer().set_rakeback_balance( 100000000 );
	auto player = response.mutable_member_info();
	pClientSession->CopyPlayer( player );
	if ( response.tier_stale() == true )
		player->set_membership_tier( t_class );
	pClientSession->CheckLostLimitTime();
	// 손실한도 무조건 내려줌
	auto add_lost_limit = response.mutable_loss_cap();
	pClientSession->CopyLostLimit( add_lost_limit );

	pClientSession->CheckLoginReward();//로그인보상체크

	//const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

	//// 친구 매니저 내 캐쉬 갱신
	//PmNet::MateDetail friendInfo;
	//if ( pClientSession->GetJoinedRoomNumber() == 0 )
	//	friendInfo.set_mate_state( General::ContactState::ContactState_Online );
	//else
	//	friendInfo.set_mate_state( General::ContactState::ContactState_InPlay );

	//friendInfo.set_node_id( configReader->SID_FOR_MANAGE );

	//// 플레이어 데이터 카피
	//auto add_player = friendInfo.mutable_member_info();
	//add_player->CopyFrom( pClientSession->GetPlayer() );

	//// 플레이어 전적 데이터 카피
	//records = friendInfo.mutable_ledger();
	//pClientSession->CopyRecords( records );

	//서버시간
	string currentKstTime = TimeUtils::GetCurrentKSTDateTimeString();
	response.set_svr_ts( currentKstTime );// "2024-12-23 19:00:00"

	//NetLib::cSingleton<cFriendManager>::GetInstance()->SetFriendInfo( friendInfo );
	//pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_InPlay , std::to_string( configReader->SID_FOR_MANAGE ) );

	//슬롯 랭킹이벤트 내려주기


// 이미지 공지 메시지 추가
	std::vector<Server::MaintenanceMessage> imageMessages = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetAllMaintenance_ImageMessages();

	General::StoreChannel player_market = pClientSession->GetMarket();

	for ( const auto& imageMessage : imageMessages )
	{
		bool show = false;
		switch ( player_market ) {
		case General::StoreChannel::StoreChannel_GooglePlay: show = imageMessage.play_store();
			break;
		case General::StoreChannel::StoreChannel_AppleAppStore: show = imageMessage.app_store();
			break;
		case General::StoreChannel::StoreChannel_OneStore: show = imageMessage.one_store();
			break;
		case General::StoreChannel::StoreChannel_PC: show = imageMessage.pc();
			break;
		}

		if ( false == show )
			continue;

		int i_version_min = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( imageMessage.version_min() );
		int i_version_max = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( imageMessage.version_max() );

		if ( false == NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenanceVersion( i_version_min , i_version_max , pClientSession->GetGameVersion() ) )
			continue;

		response.add_img_bulletins( imageMessage.img_file_name() );
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	pClientSession->GetPlayerExtRef().set_kickout_cooldown_until( TimeUtils::GetCurrentDateTime() );

	std::string errorString = std::format( "[ UpdateLobby Success PlayerIdx {} ]" , std::to_string( pClientSession->GetPlayerIdx() ) );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenance( pClientSession->GetMarket() , pClientSession->GetIp() , pClientSession->GetGameVersion() ) ) {
		const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();
		std::string errorString;
		PmNet::ServiceNotice system_message;
		system_message.set_notice( message.message() );
		pClientSession->SendRequest( General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_ServicePaused , errorString );
		pClientSession->SessionLogout( pContext->GetEntity() );
		pClientSession->DisConnectContext( pContext->GetEntity() , nThreadIndex );
		pContext->Disconnect();
		if ( pContext->GetContextType() == E_CONTEXT_TYPE::E_CONTEXT_SERVER )
		{
			if ( FALSE == NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->DeRegisterServer( static_cast< BYTE >( E_SERVER_TYPE::LOBBY_SERVER ) , pContext->GetAllocateSlot() ) )
			{
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
					_T( "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Connector DeRegisterServer Failed. GetAllocateSlot %I64d" , pContext->GetAllocateSlot() ) );
			}
		}
	}

	

#ifdef _DEBUG
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );
#endif
}

void cProtoMsgStub::DailyRefresh( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	// Daily 초기화 처리
	BOOL updateMailBox;
	pClientSession->UpdateLobby( updateMailBox );
}

void cProtoMsgStub::MailBox( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ) // 메일 목록
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::InboxIndexRQ request;
	PmNet::InboxIndexRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::vector<PmNet::InboxDetail> mail_list;
	QueryManager::MailBoxGet( pClientSession->GetPlayerIdx() , mail_list );
	pClientSession->SetMailBoxOnLogin( mail_list );

	int pagingSize = request.page_sz() > 0 ? request.page_sz() : 10; // default size 10 으로 셋팅

	General::ResultCode errorCode = pClientSession->RequestMailBox( pagingSize , request.page() , response );
	if ( errorCode != General::ResultCode::Result_Success ) {
		SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	//// 40201 초과금 획득코인 메시지 발송 로그
	//for(const auto& mail : mail_list){
	//	if(mail.mail_type() == 7){
	//		const General::ParticipantProfile player = pClientSession->GetPlayer();
	//		int daily_coin_limit_mail_count = player.coin_limit_mail_count();
	//		std::string daily_coin_limit_mail_count_str = std::to_string(daily_coin_limit_mail_count);
	//		
	//		auto message_data = QueryManager::GetMessageData(mail.mail_idx());
	//		std::string reward_type_string = std::get<0>(message_data);
	//		std::string count = std::get<3>(message_data);
	//		std::string reg_date = std::get<4>(message_data);		

	//		cMessageLog messageLogInstance( pClientSession , 40201 );
	//		messageLogInstance.SetData( reg_date, daily_coin_limit_mail_count_str, count );
	//		messageLogInstance.SetMsgid( std::to_string(mail.mail_idx()) );
	//		messageLogInstance.SetReward( reward_type_string );	
	//	}
	//}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , serializedData.c_str() );
}

void cProtoMsgStub::MailOpen( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )// 메일 오픈
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::InboxReadRQ request;
	PmNet::InboxReadRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	//30105 시작
//	cAssetLog assetLogInstance( pClientSession , 30105 );

	General::ResultCode errorCode = pClientSession->OpenMail( request.inbox_idx_list() , response );
	if ( errorCode != General::ResultCode::Result_Success ) {
		SendMessageAndLogWrite( errorCode , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	//// mail_idx_list를 문자열로 변환
 //   std::ostringstream oss;
 //   for (int i = 0; i < response.inbox_idx_list_size(); ++i) {
 //       if (i > 0) {
 //           oss << ","; // 구분자 추가
 //       }
 //       oss << response.inbox_idx_list(i);
 //   }
 //   std::string m_mail_idx_list = oss.str();

	//std::ostringstream oss1;
 //   for (int i = 0; i < response.avatar_ids_size(); ++i) {
 //       if (i > 0) {
 //           oss1 << ","; // 구분자 추가
 //       }
 //       oss1 << response.avatar_ids(i);
 //   }
 //   std::string m_avatar_ids = oss1.str();

	//std::ostringstream oss2;
 //   for (int i = 0; i < response.item_nos_size(); ++i) {
 //       if (i > 0) {
 //           oss2 << ","; // 구분자 추가
 //       }
	//	
 //       oss2 << response.item_nos(i);
 //   }
 //   std::string m_item_ids = oss2.str();
	//
	//std::string rwd_list =  "chips : " + std::to_string(response.stacks()) + ", coin : " + std::to_string(response.tokens()) + ", avatar_ids : " + m_avatar_ids + ", item_ids : " + m_item_ids;

	//assetLogInstance.SetMailOpen(m_mail_idx_list, rwd_list);
	// 30105 끝

	// 추가 코드 시작
	//// 40101 메시지 삭제 로그 - 메시지 수령
	//for (int i = 0; i < response.inbox_idx_list_size(); ++i) {

	//	uint64_t _mail_idx = response.inbox_idx_list(i);
	//	auto message_data = QueryManager::GetMessageData(_mail_idx);

	//	std::string reward_type_string = std::get<0>(message_data);
	//	std::string mail_type = std::get<1>(message_data);
	//	std::string mail_type_string = std::get<2>(message_data);
	//	std::string count = std::get<3>(message_data);
	//	std::string reg_date = std::get<4>(message_data);
	//	
	//	std::string daily_coin_limit_mail_count_str = "";
	//	if (mail_type == "7"){
	//		const General::ParticipantProfile player = pClientSession->GetPlayer();
	//		int daily_coin_limit_mail_count = player.coin_limit_mail_count();
	//		daily_coin_limit_mail_count_str = std::to_string(daily_coin_limit_mail_count);
	//	}

	//	cMessageLog messageLogInstance( pClientSession , 40101 );
	//	messageLogInstance.SetData( reg_date, daily_coin_limit_mail_count_str, count );
	//	messageLogInstance.SetMsgid( std::to_string(_mail_idx) );
	//	messageLogInstance.SetMsgtype( mail_type );
	//	messageLogInstance.SetReward( reward_type_string );
	//	messageLogInstance.SetSender( mail_type_string );
	//	messageLogInstance.SetEtc( "accept" );
	//}
	//// 추가 코드 끝

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::NoticeMessage( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )// 쪽지함 ( 공지 메시지 )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::BulletinIndexRQ request;
	PmNet::BulletinIndexRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	std::vector<PmNet::BulletinDetail> notices;
	General::StoreChannel player_market = pClientSession->GetMarket();

	if ( QueryManager::GetNoticeMessageInfo( notices ) == TRUE )
	{
		for ( const auto& notice : notices )
		{
			bool show = false;
			switch ( player_market ) {
			case General::StoreChannel::StoreChannel_GooglePlay: show = notice.ps_avail();
				break;
			case General::StoreChannel::StoreChannel_AppleAppStore: show = notice.as_avail();
				break;
			case General::StoreChannel::StoreChannel_OneStore: show = notice.os_avail();
				break;
			case General::StoreChannel::StoreChannel_PC: show = notice.pc_avail();
				break;
			}

			if ( false == show )
				continue;

			PmNet::BulletinDetail* _notice = response.add_bulletins();
			_notice->CopyFrom( notice );
		}
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::GetQuests( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::FetchGoalsRQ request;
	PmNet::FetchGoalsRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// Daily 초기화 처리
	BOOL updateMailBox;
	pClientSession->UpdateLobby( updateMailBox );

	General::TaskCategory achieve_type = request.trophy_kind();

	pClientSession->RequestMissionAndAchieve( achieve_type , response );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::GetQuestReward( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::FetchGoalBountyRQ request;
	PmNet::FetchGoalBountyRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// Daily 초기화 처리
	BOOL updateMailBox;
	pClientSession->UpdateLobby( updateMailBox );

	const int quest_id = request.goal_id();
	const General::TaskCategory achieve_type = request.trophy_kind();

	int mCode = 0;

	switch ( achieve_type )
	{
	case General::TaskCategory::TaskCategory_Mission:
	{
		mCode = 30103;
	}
	break;
	case General::TaskCategory::TaskCategory_LoungeMission:
	{
		mCode = 30103;
	}
	break;
	case General::TaskCategory::TaskCategory_Achievement:
	{
		mCode = 30104;
	}
	break;
	}

	std::string achieveTypeString = protoutil::cProtoUtil::GetEnumString( achieve_type );

	uint64 get_coin;
	uint64 get_chip;

	if ( FALSE == pClientSession->GetQuestReward( achieve_type , quest_id , get_coin , get_chip ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TaskRewardGrantFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string rwd_list;
	if ( get_coin )
	{
		response.set_bounty_token( get_coin );
		rwd_list = std::format( "Coin: {}" , get_coin );
	}

	if ( get_chip )
	{
		response.set_bounty_stack( get_chip );
		rwd_list = std::format( "Chip: {}" , get_coin );
	}

	cAssetLog assetLogInstance( pClientSession , mCode );
	assetLogInstance.SetRewardQuest( std::to_string( quest_id ) , achieveTypeString , rwd_list );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::GetFreeCharge( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::FetchBonusRefuelRQ request;
	PmNet::FetchBonusRefuelRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const General::AssetKind& moneyType = request.fund_kind();

	uint64 _get_money;
	uint64 _charged;
	std::set<std::string>& list = NetLib::cSingleton<cDataLoader>::GetInstance()->GetVipList();
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	//if ( configReader->SID_FOR_MANAGE == 4 && true == list.contains( pClientSession->GetPlatformGuid()) && 2 == pClientSession->GetRoomJoinCount() && true == pClientSession->GetEsterEgg())
	if ( true == list.contains( pClientSession->GetPlatformGuid() ) )
	{
		std::set<std::string>& log_list = NetLib::cSingleton<cDataLoader>::GetInstance()->GetVipLOGList();
		_get_money = 100000000;
		_charged = 100000000;
		pClientSession->GetPlayerRef().set_wallet_coins( 100000000 );
		pClientSession->GetPlayerRef().set_vault_coins( 0 );
		response.set_fund_kind( moneyType );
		response.set_fund_amount( _get_money );
		response.set_refuel_amount( _charged );
		response.set_token_refuel_next( pClientSession->CoinFreeChargeTime() );
		response.set_stack_refuel_next( pClientSession->ChipFreeChargeTime() );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );


		PmNet::AlignMemberDetailRS res;
		General::ParticipantProfile* responsePlayer = res.mutable_member_info();
		General::ParticipantProfile& _player = pClientSession->GetPlayerRef();
		responsePlayer->CopyFrom( _player );
		GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ProfileNotice , res , General::ResultCode::Result_Success , "" );
		std::future<BOOL> result = pClientSession->SavePlayer();
		result.wait();

		if ( false == result.get() ) {
			// 쿼리 실패에 대한 처리
		}
		log_list.insert( pClientSession->GetPlatformGuid() + "_" + TimeUtils::GetCurrentDateTime() );
		list.erase( pClientSession->GetPlatformGuid() );
		return;

	}
	else if ( FALSE == pClientSession->GetFreeCharge( moneyType , _get_money , _charged ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FreeChargeRequestFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}


	response.set_fund_kind( moneyType );
	response.set_fund_amount( _get_money );
	response.set_refuel_amount( _charged );
	response.set_token_refuel_next( pClientSession->CoinFreeChargeTime() );
	response.set_stack_refuel_next( pClientSession->ChipFreeChargeTime() );

	cAssetLog assetLogInstance( pClientSession , 30106 );
	assetLogInstance.SetChargeAmount( moneyType , std::to_string( _get_money ) );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

#ifdef _DEBUG
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );
#endif

	//switch ( moneyType )
	//{
	//case General::AssetKind::AssetKind_Coin:
	//{
	//	
	//}
	//break;
	//case General::AssetKind::AssetKind_Chip:
	//{

	//}
	//break;
	//}

}

void cProtoMsgStub::WithdrawRakeback( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::DrainCashbackRQ request;
	PmNet::DrainCashbackRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64& withdraw_coin = request.drain_token();
	const uint64& cur_rakeback_coin = pClientSession->GetRakeBack();
	if ( withdraw_coin > cur_rakeback_coin ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RakebackBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 코인 보유 재화 한도가 넘는지 확인
	if ( pClientSession->GetCoin() + pClientSession->GetSafeCoin() + withdraw_coin > pClientSession->GetMaxHoldingCoin() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_CoinCapacityReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cMoneyLogInstance logInstance( pClientSession , Server::AssetLedgerSource::AssetLedger_Rakeback );

	// 레이크백 출금
	cAssetLog assetLogInstance( pClientSession , 30911 );

	// 코인 적립
	if ( FALSE == pClientSession->UseRakeBack( withdraw_coin ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_RakebackBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	QueryManager::PlayerMoneyUpdate( pClientSession->GetPlayerIdx() , pClientSession->GetCoin() , pClientSession->GetChip() , pClientSession->GetRakeBack() , pClientSession->GetGem() , 0 , 0 , pClientSession->GetPaidGem() );

	response.set_drain_token( withdraw_coin );
	response.set_cur_token( pClientSession->GetCoin() );
	response.set_cashback_balance( pClientSession->GetRakeBack() );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

#ifdef _DEBUG
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );
#endif
}
void cProtoMsgStub::BuyShopThread( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	cMoneyLogInstance logInstance( pClientSession , Server::AssetLedgerSource::AssetLedger_Shop );

	PmNet::MarketPurchaseRQ request;
	PmNet::MarketPurchaseRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// QA 서버용 응답 처리
	/*response.set_inbox_got( false );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_StorePurchaseFailed , "" );
	return;*/


	// Product ID 체크
	switch ( request.market_item_kind() )
	{
	case General::StoreProductKind::StoreProduct_BenefitTier:
		break;
	case General::StoreProductKind::StoreProduct_Item:
	{
		// 다이아로 상품 구매
		cAssetLog assetLogInstance( pClientSession , 30102 );
		std::string productDetail; // 상품 상세 정보

		std::string buy_shop_try;
		google::protobuf::util::MessageToJsonString( request , &buy_shop_try );
		TraceA( "cProtoMsgStub::BuyShop Buy Item Request" );
		TraceA( buy_shop_try );

		std::string logString = std::format( "[ cProtoMsgStub::BuyShop Buy Item Request {} ]" , buy_shop_try );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , logString.c_str() );

		// 다이아 구매 상품 검색
		const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetItemProducts();
		auto iterPair = products.find( request.item_no() );
		if ( iterPair == products.end() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		auto& product = iterPair->second;

		switch ( product.buy_money_type() )
		{
		case General::AssetKind::AssetKind_Gem:
		{
			// 보유 다이아가 충분한가?
			if ( FALSE == pClientSession->UseGemAndPaidGem( product.money_value() ) ) {
				SendMessageAndLogWrite( General::ResultCode::Result_GemBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}


		}
		break;
		default:
		{
			// 상점 구매 다른 재화 타입 오류 처리
			SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		break;
		}

		// 구매 조건 체크
		General::ResultCode _error_code = pClientSession->CheckCondition( product );
		if ( _error_code != General::ResultCode::Result_Success ) {
			SendMessageAndLogWrite( _error_code , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 상점 물품 지급
		bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
		pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );
		if ( _error_code != General::ResultCode::Result_Success ) {
			SendMessageAndLogWrite( _error_code , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		// 획득 보상 목록 상세
		assetLogInstance.SetShopProduct( product , productDetail );

		if ( updatePlayer ) {
			auto add_player = response.mutable_refresh_member();
			pClientSession->CopyPlayer( add_player );
		}

		if ( updateAvatar ) {
			pClientSession->CopyAvatars( response );
		}

		if ( updateMailBox ) {
			response.set_inbox_got( true );
		}

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		logString.clear();
		google::protobuf::util::MessageToJsonString( request , &logString );
		TraceA( "cProtoMsgStub::BuyShop Buy Item Success" );
		TraceA( logString );

		return;
	}
	break;
	case General::StoreProductKind::StoreProduct_AppStore:
	{
		// 캐시 상품 구매
		std::string productDetail; // 상품 상세 정보

		std::string buy_shop_try;
		google::protobuf::util::MessageToJsonString( request , &buy_shop_try );
		TraceA( "cProtoMsgStub::BuyShop InAppPurchase Request" );
		TraceA( buy_shop_try );

		std::string logString = std::format( "[ cProtoMsgStub::BuyShop InAppPurchase Request {} ]" , buy_shop_try );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , logString.c_str() );

		Server::ShopProduct product;

		// 인앱 상품 검색
		switch ( pClientSession->GetMarket() )
		{
		case General::StoreChannel::StoreChannel_GooglePlay:
		{
			const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetGoogleStoreProducts();
			auto iterPair = products.find( request.item_no() );
			if ( iterPair == products.end() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			product = iterPair->second;
		}break;
		case General::StoreChannel::StoreChannel_OneStore:
		{
			const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetOneStoreStoreProducts();
			auto iterPair = products.find( request.item_no() );
			if ( iterPair == products.end() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			product = iterPair->second;
		}break;
		case General::StoreChannel::StoreChannel_AppleAppStore:
		{
			const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAppleStoreProducts();
			auto iterPair = products.find( request.item_no() );
			if ( iterPair == products.end() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			product = iterPair->second;
		}break;
		}

		const uint64& player_idx = pClientSession->GetPlayerIdx();

		const int32& price = product.price();

		if ( true == pClientSession->CheckLostLimitPrice( price ) )
		{
			SendMessageAndLogWrite( General::ResultCode::Result_LossLimitPlayerBlocked , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// Platform Type 구분 필요
		std::string account_guid;
		std::string platform_guid;
		std::future<BOOL> result_guid = QueryManager::GetPlayerGuidsAsync( player_idx , account_guid , platform_guid );
		result_guid.wait();

		if ( FALSE == result_guid.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		int platform_code;
		std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( account_guid , platform_guid , platform_code );
		platform_code_result.wait();

		if ( FALSE == platform_code_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// Player Platform
		const General::AccessChannelType _platform_type = static_cast< General::AccessChannelType > ( platform_code );

		// 구분 signature 가 있는 경우는 구글 결졔임
		// TODO Maket, Platform 으로 나눠어야함
		switch ( pClientSession->GetMarket() )
		{
		case General::StoreChannel::StoreChannel_GooglePlay:
		{
			if ( request.sig().size() == 0 ) {
				SendMessageAndLogWrite( General::ResultCode::Result_PaymentSignatureRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			const std::string& product_id = product.google_product_id();

			// 영수증 검증 요청은 빌링 서버로 던진다.
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::BuyShopAuthReq _server_req;
			_server_req.set_shop_product_type( request.market_item_kind() );
			_server_req.set_product_id( product_id );
			_server_req.set_developer_pay_load( request.dev_payload() );
			_server_req.set_receipt( request.recv_proof() );
			_server_req.set_signature( request.sig() );
			_server_req.set_market( General::StoreChannel::StoreChannel_GooglePlay );
			_server_req.set_platform( General::AccessChannelType::AccessChannel_Android );

			std::basic_ostringstream<TCHAR> oss;
			oss << _T( "http://" ) << configReader->webserver.szDNS << _T( ":" )
				<< configReader->webserver.nPort << _T( "/" )
				<< configReader->webserver.szController << _T( "/" )
				<< configReader->webserver.szAction;
			std::basic_string<TCHAR> turl = oss.str();

			std::string url = StringUtil::ConvertToString( turl );

			//printf( url.c_str() );

			//Sleep( 10000 );

			//std::string url = "http://172.31.11.202:35582/Game/ServerRequest";

			std::string data;
			std::string billingError;

			std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( url ,
					_server_req ,
					static_cast< UINT >( General::PacketID::Packet_StoreVerify ) ,
					0 ,
					0 ,
					data ,
					billingError ,
					nThreadIndex ,
					25 );

			web_result.wait();
			response.set_bill_fail( billingError );
			if ( Server::ServiceStatusCode::ServiceStatus_Success != web_result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 데이터 복호화
			Server::BuyShopAuthRes _server_res;
			if ( _server_res.ParseFromArray( data.c_str() , static_cast< int >( data.size() ) ) == false ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cAssetLog assetLogInstance( pClientSession , 30101 );

			// 상점 물품 지급
			bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
			Server::ProductData _product_data = pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

			if ( updatePlayer ) {
				auto add_player = response.mutable_refresh_member();
				pClientSession->CopyPlayer( add_player );
			}

			if ( updateAvatar ) {
				pClientSession->CopyAvatars( response );
			}

			if ( updateMailBox ) {
				response.set_inbox_got( true );
			}

			// 구매 제한 금액 증가
			pClientSession->UpdateLostLimitPrice( price );
			// 로그 작업
			{
				assetLogInstance.SetProductOrderID( _server_res.order_id() );

				std::string requestString;
				google::protobuf::util::MessageToJsonString( request , &requestString );

				std::string responseString;
				google::protobuf::util::MessageToJsonString( response , &responseString );

				//int testSize = requestString.size();

				std::string productString;
				protoutil::cProtoUtil::ProtobufToJson( _product_data , productString );

				auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );
				std::string platformString = valueDescriptor->name();

				valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( General::StoreChannel::StoreChannel_GooglePlay );
				std::string marketString = valueDescriptor->name();

				QueryManager::InsertPaylog(
				account_guid ,
				platform_guid ,
				_server_res.order_id() , //const std::string & order_id ,
				product.price() ,
				product_id ,
				productString , // const std::string & product_data ,
				marketString ,
				platformString ,
				_server_res.purchase_time() , //const std::string & purchase_date ,
				requestString ,
				responseString );
				assetLogInstance.SetShopProduct( product , productDetail );
			}
		}
		break;
		// request.sig() 가 빈경우 원스토어 결제로 처리
		case General::StoreChannel::StoreChannel_OneStore:
		{
			const std::string& product_id = product.onestore_product_id();

			// 영수증 검증 요청은 빌링 서버로 던진다.
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::BuyShopAuthReq _server_req;
			_server_req.set_shop_product_type( request.market_item_kind() );
			_server_req.set_product_id( product_id );
			_server_req.set_developer_pay_load( request.dev_payload() );
			_server_req.set_receipt( request.recv_proof() );
			_server_req.set_signature( request.sig() );
			_server_req.set_market( General::StoreChannel::StoreChannel_OneStore );
			_server_req.set_platform( General::AccessChannelType::AccessChannel_Android );
			_server_req.set_is_sandbox( request.sandbox_flag() );

			std::basic_ostringstream<TCHAR> oss;
			oss << _T( "http://" ) << configReader->webserver.szDNS << _T( ":" )
				<< configReader->webserver.nPort << _T( "/" )
				<< configReader->webserver.szController << _T( "/" )
				<< configReader->webserver.szAction;
			std::basic_string<TCHAR> turl = oss.str();

			std::string url = StringUtil::ConvertToString( turl );

			//printf( url.c_str() );

			//Sleep( 10000 );

			//std::string url = "http://172.31.11.202:35582/Game/ServerRequest";

			std::string data;
			std::string billingError;

			std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( url ,
					_server_req ,
					static_cast< UINT >( General::PacketID::Packet_StoreVerify ) ,
					0 ,
					0 ,
					data ,
					billingError ,
					nThreadIndex ,
					25 );

			web_result.wait();
			response.set_bill_fail( billingError );
			if ( Server::ServiceStatusCode::ServiceStatus_Success != web_result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 데이터 복호화
			Server::BuyShopAuthRes _server_res;
			if ( _server_res.ParseFromArray( data.c_str() , static_cast< int >( data.size() ) ) == false ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cAssetLog assetLogInstance( pClientSession , 30101 );

			// 상점 물품 지급
			bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
			Server::ProductData _product_data = pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

			if ( updatePlayer ) {
				auto add_player = response.mutable_refresh_member();
				pClientSession->CopyPlayer( add_player );
			}

			if ( updateAvatar ) {
				pClientSession->CopyAvatars( response );
			}

			if ( updateMailBox ) {
				response.set_inbox_got( true );
			}

			// 구매 제한 금액 증가
			pClientSession->UpdateLostLimitPrice( price );
			// 로그 작업
			{
				assetLogInstance.SetProductOrderID( _server_res.order_id() );

				std::string requestString;
				google::protobuf::util::MessageToJsonString( request , &requestString );

				std::string responseString;
				google::protobuf::util::MessageToJsonString( response , &responseString );

				google::protobuf::util::JsonPrintOptions options;
				options.always_print_primitive_fields = true; // Include fields with default values
				options.preserve_proto_field_names = true;    // Preserve field names from the proto file

				std::string productString;
				protoutil::cProtoUtil::ProtobufToJson( _product_data , productString );

				auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );
				std::string platformString = valueDescriptor->name();

				valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( General::StoreChannel::StoreChannel_OneStore );
				std::string marketString = valueDescriptor->name();

				QueryManager::InsertPaylog(
				account_guid ,
				platform_guid ,
				_server_res.order_id() , //const std::string & order_id ,
				product.price() ,
				product_id ,
				productString , // const std::string & product_data ,
				marketString ,
				platformString ,
				_server_res.purchase_time() , //const std::string & purchase_date ,
				requestString ,
				responseString );

			}
			assetLogInstance.SetShopProduct( product , productDetail );

		}
		break;
		case General::StoreChannel::StoreChannel_AppleAppStore:
		{
			if ( request.sig().empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_AppleSignatureMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			const std::string& product_id = product.apple_product_id();

			// 영수증 검증 요청은 빌링 서버로 던진다.
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::BuyShopAuthReq _server_req;
			_server_req.set_shop_product_type( request.market_item_kind() );
			_server_req.set_product_id( product_id );
			_server_req.set_developer_pay_load( request.dev_payload() );
			_server_req.set_receipt( request.recv_proof() );
			_server_req.set_signature( request.sig() );
			_server_req.set_market( General::StoreChannel::StoreChannel_AppleAppStore );
			_server_req.set_platform( General::AccessChannelType::AccessChannel_IOS );
			_server_req.set_is_sandbox( request.sandbox_flag() );

			std::basic_ostringstream<TCHAR> oss;
			oss << _T( "http://" ) << configReader->webserver.szDNS << _T( ":" )
				<< configReader->webserver.nPort << _T( "/" )
				<< configReader->webserver.szController << _T( "/" )
				<< configReader->webserver.szAction;
			std::basic_string<TCHAR> turl = oss.str();
			std::string url = StringUtil::ConvertToString( turl );

			std::string data;
			std::string billingError;

			std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( url ,
					_server_req ,
					static_cast< UINT >( General::PacketID::Packet_StoreVerify ) ,
					0 ,
					0 ,
					data ,
					billingError ,
					nThreadIndex ,
					25 );

			web_result.wait();
			response.set_bill_fail( billingError );
			if ( Server::ServiceStatusCode::ServiceStatus_Success != web_result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 데이터 복호화
			Server::BuyShopAuthRes _server_res;
			if ( _server_res.ParseFromArray( data.c_str() , static_cast< int >( data.size() ) ) == false ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 영수증 결제 확인. IOS 의 경우 market, transaction 으로 체크하도록 한다.
			// Reject incomplete verification results before recording or granting a purchase.
			if ( _server_res.transaction_i_d().empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( General::StoreChannel::StoreChannel_AppleAppStore );
			std::string marketString = valueDescriptor->name();

			unsigned int payCheck = QueryManager::InsertPayCheck(
				account_guid ,
				platform_guid ,
				marketString ,
				_server_res.transaction_i_d() , // apple 의 경우에는 transactionid 를 orderid 대신에 기입
				product_id );

			// 지급된 영수증
			if ( ( unsigned int ) 1062 == payCheck ) {
				SendMessageAndLogWrite( General::ResultCode::Result_UsedReceiptPurchaseRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			if ( ( unsigned int ) 0 != payCheck ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cAssetLog assetLogInstance( pClientSession , 30101 );

			// 상점 물품 지급
			bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
			Server::ProductData _product_data = pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

			if ( updatePlayer ) {
				auto add_player = response.mutable_refresh_member();
				pClientSession->CopyPlayer( add_player );
			}

			if ( updateAvatar ) {
				pClientSession->CopyAvatars( response );
			}

			if ( updateMailBox ) {
				response.set_inbox_got( true );
			}

			// 구매 제한 금액 증가
			pClientSession->UpdateLostLimitPrice( price );
			// 로그 작업
			{
				assetLogInstance.SetProductOrderID( _server_res.transaction_i_d() );

				std::string requestString;
				google::protobuf::util::MessageToJsonString( request , &requestString );

				std::string responseString;
				google::protobuf::util::MessageToJsonString( response , &responseString );

				// 애플 영수증을 응답 로그에 포함 시킴
				// 클라에게는 전송하지 않음
				responseString += "Apple Receipt : " + _server_res.apple_receipt();

				google::protobuf::util::JsonPrintOptions options;
				options.always_print_primitive_fields = true; // Include fields with default values
				options.preserve_proto_field_names = true;    // Preserve field names from the proto file

				std::string productString;
				protoutil::cProtoUtil::ProtobufToJson( _product_data , productString );

				auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );
				std::string platformString = valueDescriptor->name();

				QueryManager::InsertPaylog(
				account_guid ,
				platform_guid ,
				_server_res.transaction_i_d() , // apple 의 경우에는 transactionid 를 orderid 대신에 기입
				product.price() ,
				product_id ,
				productString , // const std::string & product_data ,
				marketString ,
				platformString ,
				_server_res.purchase_time() , //const std::string & purchase_date ,
				requestString ,
				responseString );
			}

			assetLogInstance.SetShopProduct( product , productDetail );
		}
		break;
		}

		// 획득 보상 목록 상세

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		buy_shop_try.clear();
		google::protobuf::util::MessageToJsonString( request , &buy_shop_try );
		TraceA( "cProtoMsgStub::BuyShop Success" );
		TraceA( buy_shop_try );

		return;
	}
	break;
	}
}
void cProtoMsgStub::BuyShop( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	NetLib::cSingleton<cThreadPooler::cThreadPool>::GetInstance()->EnqueueJob(
		[pContext , nCommand , pData , nLength , nThreadIndex , this]()
	{
		// 'this->'를 써서 비(非)static 멤버 함수 호출
		this->BuyShopThread( pContext , nCommand , pData , nLength , nThreadIndex );
	}
	);
	return;
	//NetLib::cSingleton<cThreadPool>::GetInstance()->enqueue(
	//	std::bind(
	//		&cProtoMsgStub::BuyShopThread , // 멤버 함수 포인터
	//		this ,                          // 호출 대상 객체 (this)
	//		pContext , nCommand , pData , nLength , nThreadIndex
	//	)
	//);


if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );

	cMoneyLogInstance logInstance( pClientSession , Server::AssetLedgerSource::AssetLedger_Shop );

	PmNet::MarketPurchaseRQ request;
	PmNet::MarketPurchaseRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// QA 서버용 응답 처리
	/*response.set_inbox_got( false );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_StorePurchaseFailed , "" );
	return;*/


	// Product ID 체크
	switch ( request.market_item_kind() )
	{
	case General::StoreProductKind::StoreProduct_BenefitTier:
		break;
	case General::StoreProductKind::StoreProduct_Item:
	{
		// 다이아로 상품 구매
		cAssetLog assetLogInstance( pClientSession , 30102 );
		std::string productDetail; // 상품 상세 정보

		std::string buy_shop_try;
		google::protobuf::util::MessageToJsonString( request , &buy_shop_try );
		TraceA( "cProtoMsgStub::BuyShop Buy Item Request" );
		TraceA( buy_shop_try );

		std::string logString = std::format( "[ cProtoMsgStub::BuyShop Buy Item Request {} ]" , buy_shop_try );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , logString.c_str() );

		// 다이아 구매 상품 검색
		const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetItemProducts();
		auto iterPair = products.find( request.item_no() );
		if ( iterPair == products.end() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		auto& product = iterPair->second;

		switch ( product.buy_money_type() )
		{
		case General::AssetKind::AssetKind_Gem:
		{
			// 보유 다이아가 충분한가?
			if ( FALSE == pClientSession->UseGemAndPaidGem( product.money_value() ) ) {
				SendMessageAndLogWrite( General::ResultCode::Result_GemBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}


		}
		break;
		default:
		{
			// 상점 구매 다른 재화 타입 오류 처리
			SendMessageAndLogWrite( General::ResultCode::Result_ActionRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}
		break;
		}

		// 구매 조건 체크
		General::ResultCode _error_code = pClientSession->CheckCondition( product );
		if ( _error_code != General::ResultCode::Result_Success ) {
			SendMessageAndLogWrite( _error_code , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 상점 물품 지급
		bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
		pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

		// 획득 보상 목록 상세
		assetLogInstance.SetShopProduct( product , productDetail );

		if ( updatePlayer ) {
			auto add_player = response.mutable_refresh_member();
			pClientSession->CopyPlayer( add_player );
		}

		if ( updateAvatar ) {
			pClientSession->CopyAvatars( response );
		}

		if ( updateMailBox ) {
			response.set_inbox_got( true );
		}

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		logString.clear();
		google::protobuf::util::MessageToJsonString( request , &logString );
		TraceA( "cProtoMsgStub::BuyShop Buy Item Success" );
		TraceA( logString );

		return;
	}
	break;
	case General::StoreProductKind::StoreProduct_AppStore:
	{
		// 캐시 상품 구매
		std::string productDetail; // 상품 상세 정보

		std::string buy_shop_try;
		google::protobuf::util::MessageToJsonString( request , &buy_shop_try );
		TraceA( "cProtoMsgStub::BuyShop InAppPurchase Request" );
		TraceA( buy_shop_try );

		std::string logString = std::format( "[ cProtoMsgStub::BuyShop InAppPurchase Request {} ]" , buy_shop_try );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , logString.c_str() );

		Server::ShopProduct product;

		// 인앱 상품 검색
		switch ( pClientSession->GetMarket() )
		{
		case General::StoreChannel::StoreChannel_GooglePlay:
		{
			const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetGoogleStoreProducts();
			auto iterPair = products.find( request.item_no() );
			if ( iterPair == products.end() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			product = iterPair->second;
		}break;
		case General::StoreChannel::StoreChannel_OneStore:
		{
			const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetOneStoreStoreProducts();
			auto iterPair = products.find( request.item_no() );
			if ( iterPair == products.end() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			product = iterPair->second;
		}break;
		case General::StoreChannel::StoreChannel_AppleAppStore:
		{
			const auto& products = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAppleStoreProducts();
			auto iterPair = products.find( request.item_no() );
			if ( iterPair == products.end() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_ProductLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}
			product = iterPair->second;
		}break;
		}

		const uint64& player_idx = pClientSession->GetPlayerIdx();

		const int32& price = product.price();

		// Platform Type 구분 필요
		std::string account_guid;
		std::string platform_guid;
		std::future<BOOL> result = QueryManager::GetPlayerGuidsAsync( player_idx , account_guid , platform_guid );
		result.wait();

		if ( FALSE == result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		int platform_code;
		std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( account_guid , platform_guid , platform_code );
		platform_code_result.wait();

		if ( FALSE == platform_code_result.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// Player Platform
		const General::AccessChannelType _platform_type = static_cast< General::AccessChannelType > ( platform_code );

		// 구분 signature 가 있는 경우는 구글 결졔임
		// TODO Maket, Platform 으로 나눠어야함
		switch ( pClientSession->GetMarket() )
		{
		case General::StoreChannel::StoreChannel_GooglePlay:
		{
			if ( request.sig().size() == 0 ) {
				SendMessageAndLogWrite( General::ResultCode::Result_PaymentSignatureRequired , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			const std::string& product_id = product.google_product_id();

			// 영수증 검증 요청은 빌링 서버로 던진다.
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::BuyShopAuthReq _server_req;
			_server_req.set_shop_product_type( request.market_item_kind() );
			_server_req.set_product_id( product_id );
			_server_req.set_developer_pay_load( request.dev_payload() );
			_server_req.set_receipt( request.recv_proof() );
			_server_req.set_signature( request.sig() );
			_server_req.set_market( General::StoreChannel::StoreChannel_GooglePlay );
			_server_req.set_platform( General::AccessChannelType::AccessChannel_Android );

			std::basic_ostringstream<TCHAR> oss;
			oss << _T( "http://" ) << configReader->webserver.szDNS << _T( ":" )
				<< configReader->webserver.nPort << _T( "/" )
				<< configReader->webserver.szController << _T( "/" )
				<< configReader->webserver.szAction;
			std::basic_string<TCHAR> turl = oss.str();

			std::string url = StringUtil::ConvertToString( turl );

			//printf( url.c_str() );

			//Sleep( 10000 );

			//std::string url = "http://172.31.11.202:35582/Game/ServerRequest";

			std::string data;
			std::string billingError;

			std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( url ,
					_server_req ,
					static_cast< UINT >( General::PacketID::Packet_StoreVerify ) ,
					0 ,
					0 ,
					data ,
					billingError ,
					nThreadIndex ,
					2 );

			web_result.wait();
			response.set_bill_fail( billingError );
			if ( Server::ServiceStatusCode::ServiceStatus_Success != web_result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 데이터 복호화
			Server::BuyShopAuthRes _server_res;
			if ( _server_res.ParseFromArray( data.c_str() , static_cast< int >( data.size() ) ) == false ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cAssetLog assetLogInstance( pClientSession , 30101 );

			// 상점 물품 지급
			bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
			Server::ProductData _product_data = pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

			if ( updatePlayer ) {
				auto add_player = response.mutable_refresh_member();
				pClientSession->CopyPlayer( add_player );
			}

			if ( updateAvatar ) {
				pClientSession->CopyAvatars( response );
			}

			if ( updateMailBox ) {
				response.set_inbox_got( true );
			}

			// 구매 제한 금액 증가
			pClientSession->UpdateLostLimitPrice( price );
			// 로그 작업
			{
				assetLogInstance.SetProductOrderID( _server_res.order_id() );

				std::string requestString;
				google::protobuf::util::MessageToJsonString( request , &requestString );

				std::string responseString;
				google::protobuf::util::MessageToJsonString( response , &responseString );

				//int testSize = requestString.size();

				std::string productString;
				protoutil::cProtoUtil::ProtobufToJson( _product_data , productString );

				auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );
				std::string platformString = valueDescriptor->name();

				valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( General::StoreChannel::StoreChannel_GooglePlay );
				std::string marketString = valueDescriptor->name();

				QueryManager::InsertPaylog(
				account_guid ,
				platform_guid ,
				_server_res.order_id() , //const std::string & order_id ,
				product.price() ,
				product_id ,
				productString , // const std::string & product_data ,
				marketString ,
				platformString ,
				_server_res.purchase_time() , //const std::string & purchase_date ,
				requestString ,
				responseString );
				assetLogInstance.SetShopProduct( product , productDetail );
			}
		}
		break;
		// request.sig() 가 빈경우 원스토어 결제로 처리
		case General::StoreChannel::StoreChannel_OneStore:
		{
			const std::string& product_id = product.onestore_product_id();

			// 영수증 검증 요청은 빌링 서버로 던진다.
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::BuyShopAuthReq _server_req;
			_server_req.set_shop_product_type( request.market_item_kind() );
			_server_req.set_product_id( product_id );
			_server_req.set_developer_pay_load( request.dev_payload() );
			_server_req.set_receipt( request.recv_proof() );
			_server_req.set_signature( request.sig() );
			_server_req.set_market( General::StoreChannel::StoreChannel_OneStore );
			_server_req.set_platform( General::AccessChannelType::AccessChannel_Android );
			_server_req.set_is_sandbox( request.sandbox_flag() );

			std::basic_ostringstream<TCHAR> oss;
			oss << _T( "http://" ) << configReader->webserver.szDNS << _T( ":" )
				<< configReader->webserver.nPort << _T( "/" )
				<< configReader->webserver.szController << _T( "/" )
				<< configReader->webserver.szAction;
			std::basic_string<TCHAR> turl = oss.str();

			std::string url = StringUtil::ConvertToString( turl );

			//printf( url.c_str() );

			//Sleep( 10000 );

			//std::string url = "http://172.31.11.202:35582/Game/ServerRequest";

			std::string data;
			std::string billingError;

			std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( url ,
					_server_req ,
					static_cast< UINT >( General::PacketID::Packet_StoreVerify ) ,
					0 ,
					0 ,
					data ,
					billingError ,
					nThreadIndex ,
					2 );

			web_result.wait();
			response.set_bill_fail( billingError );
			if ( Server::ServiceStatusCode::ServiceStatus_Success != web_result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 데이터 복호화
			Server::BuyShopAuthRes _server_res;
			if ( _server_res.ParseFromArray( data.c_str() , static_cast< int >( data.size() ) ) == false ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cAssetLog assetLogInstance( pClientSession , 30101 );

			// 상점 물품 지급
			bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
			Server::ProductData _product_data = pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

			if ( updatePlayer ) {
				auto add_player = response.mutable_refresh_member();
				pClientSession->CopyPlayer( add_player );
			}

			if ( updateAvatar ) {
				pClientSession->CopyAvatars( response );
			}

			if ( updateMailBox ) {
				response.set_inbox_got( true );
			}

			// 구매 제한 금액 증가
			pClientSession->UpdateLostLimitPrice( price );
			// 로그 작업
			{
				assetLogInstance.SetProductOrderID( _server_res.order_id() );

				std::string requestString;
				google::protobuf::util::MessageToJsonString( request , &requestString );

				std::string responseString;
				google::protobuf::util::MessageToJsonString( response , &responseString );

				google::protobuf::util::JsonPrintOptions options;
				options.always_print_primitive_fields = true; // Include fields with default values
				options.preserve_proto_field_names = true;    // Preserve field names from the proto file

				std::string productString;
				protoutil::cProtoUtil::ProtobufToJson( _product_data , productString );

				auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );
				std::string platformString = valueDescriptor->name();

				valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( General::StoreChannel::StoreChannel_OneStore );
				std::string marketString = valueDescriptor->name();

				QueryManager::InsertPaylog(
				account_guid ,
				platform_guid ,
				_server_res.order_id() , //const std::string & order_id ,
				product.price() ,
				product_id ,
				productString , // const std::string & product_data ,
				marketString ,
				platformString ,
				_server_res.purchase_time() , //const std::string & purchase_date ,
				requestString ,
				responseString );

			}
			assetLogInstance.SetShopProduct( product , productDetail );

		}
		break;
		case General::StoreChannel::StoreChannel_AppleAppStore:
		{
			if ( request.sig().empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_AppleSignatureMissing , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			const std::string& product_id = product.apple_product_id();

			// 영수증 검증 요청은 빌링 서버로 던진다.
			const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			Server::BuyShopAuthReq _server_req;
			_server_req.set_shop_product_type( request.market_item_kind() );
			_server_req.set_product_id( product_id );
			_server_req.set_developer_pay_load( request.dev_payload() );
			_server_req.set_receipt( request.recv_proof() );
			_server_req.set_signature( request.sig() );
			_server_req.set_market( General::StoreChannel::StoreChannel_AppleAppStore );
			_server_req.set_platform( General::AccessChannelType::AccessChannel_IOS );
			_server_req.set_is_sandbox( request.sandbox_flag() );

			std::basic_ostringstream<TCHAR> oss;
			oss << _T( "http://" ) << configReader->webserver.szDNS << _T( ":" )
				<< configReader->webserver.nPort << _T( "/" )
				<< configReader->webserver.szController << _T( "/" )
				<< configReader->webserver.szAction;
			std::basic_string<TCHAR> turl = oss.str();
			std::string url = StringUtil::ConvertToString( turl );

			std::string data;
			std::string billingError;

			std::future<Server::ServiceStatusCode> web_result = cProtoMsgStub::WebPostRequestAsync( url ,
					_server_req ,
					static_cast< UINT >( General::PacketID::Packet_StoreVerify ) ,
					0 ,
					0 ,
					data ,
					billingError ,
					nThreadIndex ,
					2 );

			web_result.wait();
			response.set_bill_fail( billingError );
			if ( Server::ServiceStatusCode::ServiceStatus_Success != web_result.get() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 데이터 복호화
			Server::BuyShopAuthRes _server_res;
			if ( _server_res.ParseFromArray( data.c_str() , static_cast< int >( data.size() ) ) == false ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			// 영수증 결제 확인. IOS 의 경우 market, transaction 으로 체크하도록 한다.
			// Reject incomplete verification results before recording or granting a purchase.
			if ( _server_res.transaction_i_d().empty() ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( General::StoreChannel::StoreChannel_AppleAppStore );
			std::string marketString = valueDescriptor->name();

			unsigned int payCheck = QueryManager::InsertPayCheck(
				account_guid ,
				platform_guid ,
				marketString ,
				_server_res.transaction_i_d() , // apple 의 경우에는 transactionid 를 orderid 대신에 기입
				product_id );

			// 지급된 영수증
			if ( ( unsigned int ) 1062 == payCheck ) {
				SendMessageAndLogWrite( General::ResultCode::Result_UsedReceiptPurchaseRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			if ( ( unsigned int ) 0 != payCheck ) {
				SendMessageAndLogWrite( General::ResultCode::Result_StorePurchaseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cAssetLog assetLogInstance( pClientSession , 30101 );

			// 상점 물품 지급
			bool updateMoney , updatePlayer , updateAvatar , updateMailBox;
			Server::ProductData _product_data = pClientSession->BuyShop( product , updateMoney , updatePlayer , updateAvatar , updateMailBox , productDetail );

			if ( updatePlayer ) {
				auto add_player = response.mutable_refresh_member();
				pClientSession->CopyPlayer( add_player );
			}

			if ( updateAvatar ) {
				pClientSession->CopyAvatars( response );
			}

			if ( updateMailBox ) {
				response.set_inbox_got( true );
			}

			// 구매 제한 금액 증가
			pClientSession->UpdateLostLimitPrice( price );
			// 로그 작업
			{
				assetLogInstance.SetProductOrderID( _server_res.transaction_i_d() );

				std::string requestString;
				google::protobuf::util::MessageToJsonString( request , &requestString );

				std::string responseString;
				google::protobuf::util::MessageToJsonString( response , &responseString );

				// 애플 영수증을 응답 로그에 포함 시킴
				// 클라에게는 전송하지 않음
				responseString += "Apple Receipt : " + _server_res.apple_receipt();

				google::protobuf::util::JsonPrintOptions options;
				options.always_print_primitive_fields = true; // Include fields with default values
				options.preserve_proto_field_names = true;    // Preserve field names from the proto file

				std::string productString;
				protoutil::cProtoUtil::ProtobufToJson( _product_data , productString );

				auto valueDescriptor = General::AccessChannelType_descriptor()->FindValueByNumber( _platform_type );
				std::string platformString = valueDescriptor->name();

				QueryManager::InsertPaylog(
				account_guid ,
				platform_guid ,
				_server_res.transaction_i_d() , // apple 의 경우에는 transactionid 를 orderid 대신에 기입
				product.price() ,
				product_id ,
				productString , // const std::string & product_data ,
				marketString ,
				platformString ,
				_server_res.purchase_time() , //const std::string & purchase_date ,
				requestString ,
				responseString );
			}

			assetLogInstance.SetShopProduct( product , productDetail );
		}
		break;
		}

		// 획득 보상 목록 상세

		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

		buy_shop_try.clear();
		google::protobuf::util::MessageToJsonString( request , &buy_shop_try );
		TraceA( "cProtoMsgStub::BuyShop Success" );
		TraceA( buy_shop_try );

		return;
	}
	break;
	}
}
void cProtoMsgStub::parseRewards( std::vector<std::pair<std::string , std::string>>& result , const std::string& rewards )
{
	std::istringstream rewardsStream( rewards );
	std::string reward;

	// 슬래시('/')로 구분된 각 보상 항목을 처리
	while ( std::getline( rewardsStream , reward , '/' ) ) {
		std::istringstream rewardStream( reward );
		std::string name;
		std::string amountStr;
		if ( std::getline( rewardStream , name , ':' ) && std::getline( rewardStream , amountStr ) ) {
			result.emplace_back( name , amountStr );
		}
	}
}

//이벤트 정보 로드
void cProtoMsgStub::UpdateLoginEvent( NetLib::cInterfaceIocpContext* pContext )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );
	std::vector<PmNet::DropDetail>* event_list = &pClientSession->login_event_list;
	*event_list = NetLib::cSingleton<cLoginEventManager>::GetInstance()->GetLoginEventList();

}
//유저 로그인이벤트정보 로드
void cProtoMsgStub::UpdateUserLoginEvent( NetLib::cInterfaceIocpContext* pContext )
{
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );
	//로그인 이벤트 정보 로드
	std::set<string>* receive_list = &pClientSession->receive_login_reward_list;
	receive_list->clear();
	std::future<BOOL> player_event_reward_result = QueryManager::GetLoginRewardByUserIDAsync( pClientSession->GetPlayerIdx() , *receive_list );
	player_event_reward_result.wait();

	if ( FALSE == player_event_reward_result.get() ) {
		//로그필요
		//SendMessageAndLogWrite( General::ResultCode::FindRecievedLoginEventFailed, pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
}

void cProtoMsgStub::VIPSaveUser( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ) {
	std::set<std::string>& list = NetLib::cSingleton<cDataLoader>::GetInstance()->GetVipList();
	PmNet::StoreMemberIndexRQ request;
	PmNet::StoreMemberIndexRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string hashedPassword = MADEPlatform::GenerateSha256( request.key() );
	if ( "477e2d13152129e72c4a47a5abed06ce422daff2ca0e99d33bc527477effee34" != hashedPassword )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	list.clear();
	for ( auto t_uid : request.uid_list() )
	{
		list.insert( t_uid );
		response.add_uid_list( t_uid );
	}
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}
void cProtoMsgStub::ChangeMailState( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ) {
	PmNet::SwapInboxStateRQ request;
	PmNet::SwapInboxStateRQ response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string hashedPassword = MADEPlatform::GenerateSha256( request.key() );
	if ( "477e2d13152129e72c4a47a5abed06ce422daff2ca0e99d33bc527477effee34" != hashedPassword )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	NetLib::cSingleton<cDataLoader>::GetInstance()->SetMailState( request.inbox_state() );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}
void cProtoMsgStub::ChangeDailyExpired( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ) {
	PmNet::SwapDailyStaleRQ request;
	PmNet::SwapDailyStaleRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::string hashedPassword = MADEPlatform::GenerateSha256( request.key() );
	if ( "477e2d13152129e72c4a47a5abed06ce422daff2ca0e99d33bc527477effee34" != hashedPassword )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_SecondKeyMismatch , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	NetLib::cSingleton<cDataLoader>::GetInstance()->SetDailyExpiredMonth( request.month() );

	QueryManager::UpdateSystemData( "DailyExpiredMonth" , std::to_string( request.month() ) );
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::VIPLoadUser( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ) {
	PmNet::PullMemberIndexRS res;
	std::set<std::string>& list = NetLib::cSingleton<cDataLoader>::GetInstance()->GetVipList();
	for ( auto t_uid : list )
	{
		res.add_uid_list( t_uid );
	}
	std::set<std::string>& log = NetLib::cSingleton<cDataLoader>::GetInstance()->GetVipLOGList();
	for ( auto t_uid : log )
	{
		res.add_trace_list( t_uid );
	}
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , res , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::CheckLoginReward( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ) {
	if ( !pContext )
		return;
	if ( CheckSession( pContext ) ) return;

	cClientSession* pClientSession = GetSession( pContext );
	PmNet::SigninBountyRQ req;
	PmNet::SigninBountyRS res;
	const uint64& player_idx = pClientSession->GetPlayerIdx();
	std::set<string>* receive_list = &pClientSession->receive_login_reward_list;
	std::vector<PmNet::DropDetail>* event_list = &pClientSession->login_event_list;


	std::string mail_indexs = "";
	for ( auto event : *event_list )
	{

		auto eventinfo = res.add_drop_info_list();
		eventinfo->CopyFrom( event );
		mail_indexs = "";
		if ( false == receive_list->contains( event.drop_name() ) ) {
			if ( !TimeUtils::isCurrentTimeWithin( event.start_ts() , event.end_ts() ) ) {
				continue;
			}

			std::vector<std::pair<std::string , string>> reward_list;
			parseRewards( reward_list , event.bounty() );

			for ( auto reward : reward_list ) {
				mail_indexs += std::format( "{}_" , pClientSession->SendReward( reward.first , reward.second , event.notice() ) );
			}

			if ( !mail_indexs.empty() ) {
				mail_indexs.pop_back(); // 마지막 콤마 제거
			}

			if ( !mail_indexs.empty() ) {
				auto result = QueryManager::InsertLoginRewardLog( player_idx , event.drop_name() , event.bounty() , mail_indexs );
				if ( FALSE == result )
				{
					//에러처리
				}
			}
			receive_list->insert( event.drop_name() );
			res.add_got_drop_now( event.drop_name() );
		}
	}

	if ( req.req_kind() & General::LoginRewardAction::LoginReward_Initialize )
	{
		for ( const auto& receive : *receive_list )
		{
			res.add_got_drop_list( receive );
		}
	}
	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , res , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::GetLostLimitOptions( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	PmNet::LossCapOptionsListRQ request;
	PmNet::LossCapOptionsListRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	auto* pLoader = NetLib::cSingleton<cDataLoader>::GetInstance();

	// loss_cap_amt options (BaselineConfig_LostLimitValue)
	const auto& cap_values = pLoader->GetListArrayValues( Server::BaselineConfigKey::BaselineConfig_LostLimitValue );
	for ( const uint64& v : cap_values )
		response.add_loss_cap_amt_options( v );

	// block_span_swap options (BaselineConfig_LostLimitTime, hours)
	const auto& time_values = pLoader->GetListArrayValues( Server::BaselineConfigKey::BaselineConfig_LostLimitTime );
	for ( const uint64& v : time_values )
		response.add_block_span_swap_options( static_cast< int32 >( v ) );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}
void cProtoMsgStub::ChangeLostLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::SwapLossCapRQ request;
	PmNet::SwapLossCapRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	const uint64& change_lost_limit = request.loss_cap_amt();
	const int32& change_time_limit = request.block_span_swap();

	General::ResultCode _error_code = pClientSession->ChangeLostLimit( change_lost_limit , change_time_limit );
	if ( _error_code != General::ResultCode::Result_Success ) {
		SendMessageAndLogWrite( _error_code , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto lost_limit = response.mutable_loss_cap();
	pClientSession->CopyLostLimit( lost_limit );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );

	int code = 10901;
	std::string loginTypeString = "";

	//std::string ipString = ::ConvertIP( pContext->GetIP() );

	std::string idpcode = "";

	//auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( request.store_kind() );
	//std::string marketString = valueDescriptor->name();

	//message LostLimit
	//{
	//	uint64 lost_limit = 1;							// 손실한도 설정 금액
	//	uint64 next_lost_limit = 2;						// 다음 손실한도 셋팅 금액 ( 0 이면 처리 안함 )
	//	int32 lost_limit_change_count = 3;				// 손실한도 변경 횟수 ( 매월 2회 변경 가능, 매월 1일 0시 기준 초기화 )
	//	int64 daily_lost_chip = 4;						// 오늘 총 손실 칩
	//	string refresh_time_of_lost_limit = 5;			// 손실한도 갱신 시간 ( 제한 기준값이 걸려 있는 상태에서는 0시에 초기화를 처리 하면 안된다. )
	//	uint64 buy_limit = 6;							// 구매 제한 금액
	//	uint64 monthly_buy_total = 7;					// 이번달 총 구매 금액
	//	string refresh_time_of_buy_limit = 8;			// 구매 제한 갱신 시간 ( 매월 1일 0시 기준 초기화 )
	//	uint64 account_idx = 9;							// account_idx ( user_account 테이블 )
	//	int32 set_time_limit = 10;						// 제한 시간 설정 ( 손실한도에 도달한 경우 설정한 시간 동안 게임 참가 불가 , default 6시간)
	//	int32 next_set_time_limit = 11;					// 다음 제한 시간 설정

	//	// 서버사용
	//	uint64 creation_time = 20;						// LostLimit 생성 시간 ( 서버간 동기화에 사용 )
	//}


	auto log_result = QueryManager::InsertAccountLog(
		code , // code
		pClientSession->GetPlatformGuid() , // platform_guid , // uid
		"" , // adminid
		"" , // adid
		"" , // asset
		"" , // cmd
		"" , // request.dev_meta() , // device
		"" , // gmsessid
		"" , // made_id.size() > 0 ? "Table" : "Google" , // idpcode 인증 당시에는 알 수 없다.
		pClientSession->GetIp() , // ipaddr
		"" , // request.os_ver() , // osver
		"" , // marketString , // pf
		"" , // svcuid
		"" , // accountid
		0 , // _player.wallet_chips() + _player.vault_chips() , // chip
		0 , // _player.wallet_chips() , // chip_g
		0 , // _player.vault_chips() , // chip_s
		0 , // _player.wallet_coins() + _player.vault_coins() , // coin
		0 , // _player.wallet_coins() , // coin_g
		0 , // _player.vault_coins() , // coin_s
		0 , // slotcoin
		0 , // _player.wallet_gems() + _player.paid_gems() , // gem
		0 , // _player.wallet_gems() , // gem_f
		0 , // _player.paid_gems() , // gem_p
		0 , // _player.kick_ticket_balance() , // kickoutticket
		0 , // _player.experience_points() , // exp
		0 , // friend_cnt
		pClientSession->GetJoinTime() , //TimeUtils::GetCurrentKSTDateTimeString() , // jointime
		loginTypeString , // logintype
		"" , // type
		"" , // _player.display_name() , // nickname
		"" , // deletetime
		"" , // registered
		change_time_limit , // block_hour
		lost_limit->loss_limit_changes() , // left_count
		change_lost_limit , // limit_type
		"" , // reqtm
		"" , // second_pw
		"" , // reason
		"" , // result
		"" , // hash
		pClientSession->GetGameVersion() , // ver
		"" ); // etc

	log_result.wait();

#ifdef _DEBUG
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( response , &serializedData );
	TraceA( serializedData );
#endif
}

void cProtoMsgStub::CheckLostLimitTime( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::VerifyLossCapSpanRQ request;
	PmNet::VerifyLossCapSpanRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	if ( false == pClientSession->CheckLostLimitTime() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_SelfLossLimitCooldownActive , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto lost_limit = response.mutable_loss_cap();
	pClientSession->CopyLostLimit( lost_limit );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::SafeMoneyDeposit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::VaultStashRQ request;
	PmNet::VaultStashRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cMoneyLogInstance logInstance( pClientSession , Server::AssetLedgerSource::AssetLedger_Vault );

	// 금고 이동
	cAssetLog assetLogInstance( pClientSession , 30901 );

	// 칩 입금
	if ( request.stash_stacks() )
	{
		// 입금 하려는 금액 확인
		if ( request.stash_stacks() > pClientSession->GetChip() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChipBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 금고 한도
		if ( pClientSession->GetMaxHoldingSafeChip() < pClientSession->GetSafeChip() + request.stash_stacks() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_SafeChipCapacityReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 무료 칩만 존재하는 경우
		uint64 cur_chip = pClientSession->GetChip();
		pClientSession->SetChip( General::PlayCategory::PlayCategory_None , cur_chip - request.stash_stacks() );

		// 유저의 안전 금고 칩에 남은 칩 합산
		uint64 cur_safe_chip = pClientSession->GetSafeChip();
		pClientSession->SetSafeChip( cur_safe_chip + request.stash_stacks() );

		assetLogInstance.SetSafeBoxLogType( Server::SafeBoxLogType::SafeBoxLogType_Deposit_Chip );
	}

	// 코인 입금
	if ( request.stash_tokens() )
	{
		// 입금 하려는 금액 확인
		uint64 current_coin = pClientSession->GetCoin();
		if ( request.stash_tokens() > current_coin ) {
			SendMessageAndLogWrite( General::ResultCode::Result_CoinBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		uint64 current_safe_coin = pClientSession->GetSafeCoin();
		pClientSession->SetCoin( General::PlayCategory::PlayCategory_None , current_coin - request.stash_tokens() );
		pClientSession->SetSafeCoin( current_safe_coin + request.stash_tokens() );

		assetLogInstance.SetSafeBoxLogType( Server::SafeBoxLogType::SafeBoxLogType_Deposit_Coin );
	}

	if ( request.stash_tokens() == 99 )
		pClientSession->SetEsterEgg( true );

	// 응답 처리
	response.set_stacks( pClientSession->GetChip() );
	response.set_tokens( pClientSession->GetCoin() );
	response.set_vault_stacks( pClientSession->GetSafeChip() );
	response.set_vault_tokens( pClientSession->GetSafeCoin() );

	// 플레이어 데이터 저장
	const uint64 player_idx = pClientSession->GetPlayerIdx();

	QueryManager::PlayerMoneyUpdate( player_idx , pClientSession->GetCoin() , pClientSession->GetChip() , pClientSession->GetRakeBack() , pClientSession->GetGem() , 0 , 0 , pClientSession->GetPaidGem() );

	QueryManager::PlayerSafeMoneyUpdate( pClientSession->GetPlayer() );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::SafeMoneyWithdraw( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::VaultTakeRQ request;
	PmNet::VaultTakeRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );
	cMoneyLogInstance logInstance( pClientSession , Server::AssetLedgerSource::AssetLedger_Vault );
	// 금고 이동
	cAssetLog assetLogInstance( pClientSession , 30901 );

	// 칩 출금
	if ( request.take_stacks() )
	{
		uint64 current_total_chip = pClientSession->GetSafeChip();
		if ( request.take_stacks() > current_total_chip ) {
			SendMessageAndLogWrite( General::ResultCode::Result_SafeChipBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 보유 한도
		if ( pClientSession->GetMaxHoldingChip() < pClientSession->GetChip() + request.take_stacks() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_ChipCapacityReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		// 안전 금고의 유료 칩만 출금
		uint64 cur_safe_paid_chip = pClientSession->GetSafeChip();
		pClientSession->SetSafeChip( cur_safe_paid_chip - request.take_stacks() );

		// 유저의 유료 칩 증가
		uint64 cur_paid_chip = pClientSession->GetChip();
		pClientSession->SetChip( General::PlayCategory::PlayCategory_None , cur_paid_chip + request.take_stacks() );

		assetLogInstance.SetSafeBoxLogType( Server::SafeBoxLogType::SafeBoxLogType_Withdraw_Chip );
	}

	// 코인 입금
	if ( request.take_tokens() )
	{
		// 출금 하려는 금액 확인
		uint64 current_safe_coin = pClientSession->GetSafeCoin();
		if ( request.take_tokens() > current_safe_coin ) {
			SendMessageAndLogWrite( General::ResultCode::Result_SafeCoinBalanceInsufficient , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		uint64 current_coin = pClientSession->GetCoin();
		pClientSession->SetSafeCoin( current_safe_coin - request.take_tokens() );
		pClientSession->SetCoin( General::PlayCategory::PlayCategory_None , current_coin + request.take_tokens() );

		assetLogInstance.SetSafeBoxLogType( Server::SafeBoxLogType::SafeBoxLogType_Withdraw_Coin );
	}

	// 응답 처리
	response.set_stacks( pClientSession->GetChip() );
	response.set_tokens( pClientSession->GetCoin() );
	response.set_vault_stacks( pClientSession->GetSafeChip() );
	response.set_vault_tokens( pClientSession->GetSafeCoin() );

	// 플레이어 데이터 저장
	const uint64 player_idx = pClientSession->GetPlayerIdx();

	QueryManager::PlayerMoneyUpdate( player_idx , pClientSession->GetCoin() , pClientSession->GetChip() , pClientSession->GetRakeBack() , pClientSession->GetGem() , 0 , 0 , pClientSession->GetPaidGem() );

	QueryManager::PlayerSafeMoneyUpdate( pClientSession->GetPlayer() );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::AdWatch( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::PromoViewRQ request;
	PmNet::PromoViewRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	int code = 70101;
	if ( false == request.start_flag() )
	{
		code = 70102;
		// 광고 업적 처리
		pClientSession->UpdateQuests( General::TaskTrigger::TaskTrigger_AdView );
	}

	std::string moneyTypeString = "";
	switch ( request.fund_kind() )
	{
	case General::AssetKind::AssetKind_Coin:
	{
		moneyTypeString = "RoyalTicket";
	}
	break;
	case General::AssetKind::AssetKind_Chip:
	{
		moneyTypeString = "AdReward";
	}
	break;
	}

	// 70101 광고 시청 시작 로그
	cAdsLog adsLogInstance( pClientSession , code , moneyTypeString );

	// 70102 광고 종료 로그
	// cAdsLog adsLogInstance( pClientSession , 70102 );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::GetPlayerInfo( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::FetchMemberDetailRQ request;
	PmNet::FetchMemberDetailRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	// 아무 응답 하지 않는다.
	if ( request.member_idx() == 0 )
		return;

	// 플레이어를 검색해서 응답해준다.
	NetLib::cSessionManager* pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::ExistsInstance();
	if ( pSessionManager == nullptr ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto session = pSessionManager->Get( request.member_idx() );
	if ( session == nullptr || session->GetSessionType() != Sessions::SESSION_CLIENT ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	cClientSession* pPlayerSession = static_cast< cClientSession* >( session );

	response.set_member_idx( request.member_idx() );
	response.set_member_stack( pPlayerSession->GetChip() );
	response.set_member_token( pPlayerSession->GetCoin() );
	response.set_hold_daily_stack( pPlayerSession->GetPlayerExt().holdem_daily_chips() );
	response.set_hold_daily_token( pPlayerSession->GetPlayerExt().holdem_daily_coins() );
	response.set_lbd_daily_stack( pPlayerSession->GetPlayerExt().lowbadugi_daily_chips() );
	response.set_lbd_daily_token( pPlayerSession->GetPlayerExt().lowbadugi_daily_coins() );

	// 플레이어 전적 데이터 카피
	auto records = response.mutable_ledger();
	pPlayerSession->CopyRecords( records );

	auto ingame_record = response.mutable_chamber_log();
	pPlayerSession->CopyIngameRecord( ingame_record );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::AddFriend( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::InsertMateRQ request;
	PmNet::InsertMateRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 친구 정보 검색
	const uint64& friend_player_idx = request.member_idx();

	std::string friend_platform_guid;
	auto result = QueryManager::GetPlatformGuidByPlayerIdxAsync( friend_player_idx , friend_platform_guid );
	result.wait();

	if ( FALSE == result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_PlayerLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 친구 목록에 Add 시켜준다.
	const int dbIndex = 1;
	result = QueryManager::InsertFriendAsync( pClientSession->GetPlayerIdx() , friend_platform_guid , friend_player_idx , dbIndex );
	result.wait();

	if ( FALSE == result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FriendAddFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto friend_info = NetLib::cSingleton<cFriendManager>::GetInstance()->GetFriendInfo( friend_player_idx );
	if ( friend_info != nullptr ) {

		// 프랜드 캐쉬가 있으면 캐쉬를 가져온다.
		auto add_friend = response.mutable_add_mate();
		add_friend->CopyFrom( *friend_info );
	}
	else {

		// DB 에서 읽어온다.
		General::ParticipantProfile _player;
		Server::ParticipantProfileInternal _playerExt;
		auto result_read_player = QueryManager::GetPlayerByPlayerIdxAsync( friend_player_idx , _player , _playerExt );
		result_read_player.wait();
		if ( FALSE == result_read_player.get() ) {
			SendMessageAndLogWrite( General::ResultCode::Result_PlayerLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
		}

		auto add_friend = response.mutable_add_mate();
		auto player = add_friend->mutable_member_info();
		player->CopyFrom( _player );

		//add_friend->CopyFrom( _player );
	}
	//친구 업적
	pClientSession->UpdateQuests( General::TaskTrigger::TaskTrigger_FriendRequest );

	// 50101 친구 신청 로그
	const General::ParticipantProfile player = pClientSession->GetPlayer();
	std::string friendRequestCount = QueryManager::GetFriendRequestCount( player.member_id() );

	cSocialLog socialLogInstance( pClientSession , 50101 );
	socialLogInstance.SetFriendId( friend_platform_guid );
	socialLogInstance.SetLogData( friendRequestCount );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::DeleteFriend( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::DropMateRQ request;
	PmNet::DropMateRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	const int delete_friend_count = pClientSession->GetDeleteFriendCount();

	if ( delete_friend_count >= 10 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_DailyFriendRemoveLimitReached , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	const uint64& friend_player_idx = request.member_idx();

	// 50102 친구 삭제 로그 ( 친구의 UID 가져오기 ) 시작
	std::string friend_platform_guid;
	auto result_guid = QueryManager::GetPlatformGuidByPlayerIdxAsync( friend_player_idx , friend_platform_guid );
	result_guid.wait();

	if ( FALSE == result_guid.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_PlayerLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	// 50102 친구 삭제 로그 ( 친구의 UID 가져오기 ) 끝

	// 삭제 제한 10명에 걸렸는지 확인한다.
	auto result = QueryManager::DeleteFriendsAsync( pClientSession->GetPlayerIdx() , friend_player_idx );
	result.wait();

	if ( FALSE == result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FriendRemoveFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 친구 삭제 카운트 저장
	pClientSession->SetDeleteFriendCount( delete_friend_count + 1 );

	result = pClientSession->SavePlayer();
	result.wait();

	response.set_drop_member_idx( friend_player_idx );
	response.set_mate_drop_cnt( pClientSession->GetDeleteFriendCount() );

	// 50102 친구 삭제 로그
	const General::ParticipantProfile player = pClientSession->GetPlayer();
	std::string friendCount = QueryManager::GetFriendCount( player.member_id() );

	cSocialLog socialLogInstance( pClientSession , 50102 );
	socialLogInstance.SetFriendId( friend_platform_guid );
	socialLogInstance.SetLogData( friendCount );

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

// 로비에 있는 유저의 정보를 가져온다.
void cProtoMsgStub::LobbyPlayerList( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::AtriumMemberIndexRQ request;
	PmNet::AtriumMemberIndexRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	const auto& lobby_players = NetLib::cSingleton<cFriendManager>::GetInstance()->GetLobbyPlayers( pClientSession->GetPlayerIdx() );
	for ( const auto player : lobby_players ) {

		auto add_player = response.add_atrium_members();
		if ( add_player == nullptr ) continue;

		add_player->CopyFrom( *player );
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::FriendList( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::MateIndexRQ request;
	PmNet::MateIndexRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::vector<uint64> friend_player_idx_list;
	auto result = QueryManager::GetFriendsAsync( pClientSession->GetPlayerIdx() , friend_player_idx_list );
	result.wait();

	if ( result.get() ) {

		// 친구 목록을 가져 옵니다.

		PmNet::Ledger _records;

		for ( const auto& friend_player_idx : friend_player_idx_list ) {

			auto add_friend = response.add_mate_members();
			if ( add_friend == nullptr ) continue;

			auto friend_info = NetLib::cSingleton<cFriendManager>::GetInstance()->GetFriendInfo( friend_player_idx );
			if ( friend_info != nullptr ) {

				PmNet::Ledger* friendRecords = friend_info->mutable_ledger();

				std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( friend_info->member_info().daily_reset_at() );
				std::time_t today_start_time = TimeUtils::TodayStartTimeTM();

				if ( cur_refresh_time <= today_start_time )
				{
					// 레코드 추가 될때 마다 추가 하기 + 이거 수정하면 서치 플레이어도
					friendRecords->clear_lowbadugi_today();
					friendRecords->clear_holdem_today();
					friendRecords->clear_baccarat_today();
					friendRecords->clear_blackjack_today();
					friendRecords->clear_slot_today();
					//friendRecords->clear_roulette_today();
				}

				// 프랜드 캐쉬가 있으면 캐쉬를 가져온다.
				add_friend->CopyFrom( *friend_info );
			}
			else {

				// DB 에서 읽어온다.
				General::ParticipantProfile _player;
				Server::ParticipantProfileInternal _playerExt;
				auto result_read_player = QueryManager::GetPlayerByPlayerIdxAsync( friend_player_idx , _player , _playerExt );
				result_read_player.wait();
				if ( FALSE == result_read_player.get() ) {
					//SendMessageAndLogWrite( General::ResultCode::Result_PlayerLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
					continue;
				}

				auto add_player = add_friend->mutable_member_info();
				add_player->CopyFrom( _player );

				PmNet::Ledger* responseRecords = add_friend->mutable_ledger();
				if ( responseRecords == nullptr ) continue;

				_records.Clear();
				int _reads = 0;
				result = std::async( [friend_player_idx , &_reads , &_records]() {
					return QueryManager::PlayerGetRecords( friend_player_idx , _reads , _records );
				} );
				result.wait();

				if ( true == result.get() )
				{
					std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( _player.daily_reset_at() );
					std::time_t today_start_time = TimeUtils::TodayStartTimeTM();

					if ( cur_refresh_time <= today_start_time )
					{
						// 레코드 추가 될때 마다 추가 하기 + 이거 수정하면 서치 플레이어도
						_records.clear_lowbadugi_today();
						_records.clear_holdem_today();
						_records.clear_baccarat_today();
						_records.clear_blackjack_today();
						_records.clear_slot_today();
						//_records.clear_roulette_today();

					}
				}

				responseRecords->CopyFrom( _records );
			}
		}
	}
	else {

		// 쿼리 실패
		SendMessageAndLogWrite( General::ResultCode::Result_FriendListLoadFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::SearchPlayer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	cClientSession* pClientSession = GetSession( pContext );

	PmNet::LookupMemberRQ request;
	PmNet::LookupMemberRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// 닉네임 검증 추가 (SQL 인젝션 방지)
	if ( !StringUtil::isValidNickname( request.alias_label() ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AliasFormatRejected , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	std::vector<General::ParticipantProfile> players;

	auto result = QueryManager::PlayerGetByNicknameAsync( request.alias_label() , players );
	result.wait();

	if ( FALSE == result.get() ) {
		SendMessageAndLogWrite( General::ResultCode::Result_FriendSearchFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	PmNet::Ledger _records;
	for ( const auto& player : players ) {

		auto add_player = response.add_looked_up_members();
		if ( add_player == nullptr ) continue;

		const uint64& _player_idx = player.member_id();

		auto friend_info = NetLib::cSingleton<cFriendManager>::GetInstance()->GetFriendInfo( _player_idx );
		if ( friend_info == nullptr ) {

			PmNet::MateDetail friendInfo;
			friendInfo.set_mate_state( General::ContactState::ContactState_Offline );
			auto add_friend = friendInfo.mutable_member_info();
			add_friend->CopyFrom( player );
			if ( add_friend == nullptr ) continue;

			_records.Clear();
			int _reads = 0;
			result = std::async( [_player_idx , &_reads , &_records]() {
				return QueryManager::PlayerGetRecords( _player_idx , _reads , _records );
			} );
			result.wait();

			if ( true == result.get() )
			{
				std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( player.daily_reset_at() );
				std::time_t today_start_time = TimeUtils::TodayStartTimeTM();

				if ( cur_refresh_time <= today_start_time )
				{
					// 레코드 추가 될때 마다 추가 하기 + 이거 수정하면 FriedList도
					_records.clear_lowbadugi_today();
					_records.clear_holdem_today();
					_records.clear_baccarat_today();
					_records.clear_blackjack_today();
					_records.clear_slot_today();
					//_records.clear_roulette_today();
				}
			}

			PmNet::Ledger* responseRecords = friendInfo.mutable_ledger();
			responseRecords->CopyFrom( _records );
			add_player->CopyFrom( friendInfo );
		}
		else {

			PmNet::Ledger* friendRecords = friend_info->mutable_ledger();

			std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( friend_info->member_info().daily_reset_at() );
			std::time_t today_start_time = TimeUtils::TodayStartTimeTM();

			if ( cur_refresh_time <= today_start_time )
			{
				// 레코드 추가 될때 마다 추가 하기 + 이거 수정하면 서치 플레이어도
				friendRecords->clear_lowbadugi_today();
				friendRecords->clear_holdem_today();
				friendRecords->clear_baccarat_today();
				friendRecords->clear_blackjack_today();
				friendRecords->clear_slot_today();
				//friendRecords->clear_roulette_today();
			}

			// 등록된 것이 있으면 있는 것을 보내준다.
			add_player->CopyFrom( *friend_info );
		}
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::CheatMoney( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) )
		return;

	if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage ) {
		return;
	}
	cClientSession* pClientSession = GetSession( pContext );

	PmNet::DebugFundsRQ request;
	PmNet::DebugFundsRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cGameRoom* pGameRoom = nullptr;
	const int roomNumber = pClientSession->GetJoinedRoomNumber();
	if ( roomNumber != 0 )
		pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );

	// 재화 갱신
	switch ( request.fund_kind() )
	{
	case General::AssetKind::AssetKind_Chip:
	{
		pClientSession->SetChip( General::PlayCategory::PlayCategory_None , request.fund_amount() );
	}
	break;
	case General::AssetKind::AssetKind_Coin:
	{
		pClientSession->SetCoin( General::PlayCategory::PlayCategory_None , request.fund_amount() );
	}
	break;
	case General::AssetKind::AssetKind_Gem:
	{
		pClientSession->SetGem( request.fund_amount() );
	}
	break;
	}

	int64 editMoney = pClientSession->GetCoin();

	// 모든 SavePlayer() 비동기 작업을 시작하고 결과를 추적
	std::future<BOOL> results = pClientSession->SavePlayer();
	results.wait(); // 비동기 완료 대기

	if ( results.get() ) {

		std::string errorString = "[ CheatMoney ] PlayerIdx [ " + std::to_string( pClientSession->GetPlayerIdx() ) + " ]";
		errorString = protoutil::cProtoUtil::ErrorCodeString( errorString.c_str() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

		// 방안에 진입해 있는 경움면 모든 플레이어의 데이터 전송
		if ( roomNumber == 0 ) {
			General::ParticipantProfile* pPlayer = response.add_members();
			pClientSession->CopyPlayer( pPlayer );
			GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
		}
		else {
			if ( pGameRoom != nullptr )
			{
				auto game = pGameRoom->GetGameInterface();
				for ( auto player : game->GetPlayersSessionList() ) {
					if ( player != nullptr ) {
						General::ParticipantProfile* pPlayer = response.add_members();
						player->CopyPlayer( pPlayer );
					}
				}

				// 방전체에 전송
				pGameRoom->RoomBroadCast( General::Packet_LabCreditUpdate , response );
			}
		}
	}
	else
	{
		std::string errorString = "[ CheatMoney Failed ] PlayerIdx [ " + std::to_string( pClientSession->GetPlayerIdx() ) + " ]";

		// 치트가 실패 하는 경우에는 재화가 수정을 기존과 동일하게 하는 경우 실패가 될 수 있다.
		// 플레이어의 현재 재화를 내려준다.
		General::ParticipantProfile* pPlayer = response.add_members();
		pClientSession->CopyPlayer( pPlayer );
		GetOwner()->SendBuffer( pContext , nThreadIndex , nCommand , response , General::ResultCode::Result_Success , errorString );
	}

}

void cProtoMsgStub::CheatCardChange( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
#ifdef _DEBUG

	if ( CheckSession( pContext ) )
		return;

	PmNet::DebugHandSwapRQ request;
	PmNet::DebugHandSwapRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}
	response.set_ctx_buf( request.ctx_buf() );

	cClientSession* pClientSession = GetSession( pContext );

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	auto gameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindRoom( roomNumber , nThreadIndex );

	IGame* gameInterface = gameRoom->GetGameInterface();

	General::PlayCategory gameType = gameInterface->GetGameType();

	switch ( gameType )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		cHoldem* pHoldem = static_cast< cHoldem* >( gameInterface );
		if ( pHoldem == nullptr ) {
			SendMessageAndLogWrite( General::ResultCode::Result_UnexpectedCondition , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
			return;
		}

		pHoldem->CheatCardChange( request );
	}
	break;
	}

#endif
}


// QA 서버 로그인 기능
// 세션 권한이 없으면 로그인 실패로 처리
void cProtoMsgStub::QALogin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::TesterSigninRQ request;
	PmNet::TesterSigninRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	std::string auth_code = request.auth_token();
	if ( auth_code.compare( "J5B6eUJYNdwvYQoc" ) == 0 ) {

		// 세션이 없으면 세션 부여

	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ConsoleAccess , response , General::ResultCode::Result_Success , "" );

	TraceA( "QALogin Success" );
}


// 이 치트는 홀덤, 로우바둑이 전역으로 관리가 되어야 한다.
void cProtoMsgStub::CardDeckCheat( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::StackDebugRQ request;
	PmNet::StackDebugRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	// 방에다가 셋팅하지 않고 전역으로 셋팅한다.

	switch ( request.match_kind() )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		if ( false == request.clear_debug() )
			NetLib::cSingleton<cQADeck>::GetInstance()->SetHoldemQA( request );
		else
			NetLib::cSingleton<cQADeck>::GetInstance()->ClearHoldemQA( request );

		const UINT& roomNumber = request.chamber_no();

		if ( roomNumber ) {
			cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( roomNumber );
			if ( pGameRoom == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			if ( pGameRoom->GetGameInterface()->GetGameType() != General::PlayCategory::PlayCategory_TexasHoldem ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			cHoldem* pHoldem = static_cast< cHoldem* >( pGameRoom->GetGameInterface() );
			if ( pHoldem == nullptr ) {
				SendMessageAndLogWrite( General::ResultCode::Result_RoomLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
			}

			if ( request.shared_cards().size() ) {

				pHoldem->m_qa_community_cards.clear();
				for ( const auto& card : request.shared_cards() ) {
					pHoldem->m_qa_community_cards.push_back( card );
				}
			}

			if ( request.clear_debug() )
				pHoldem->m_qa_community_cards.clear();
		}
	}
	break;
	}

	if ( false == request.clear_debug() )
	{
		auto player_coin_map = request.member_tokens_map();
		for ( auto player_coin_pair : player_coin_map )
		{
			uint64 playerIdx = player_coin_pair.first;
			uint64 playerMoney = player_coin_pair.second;

			auto iter = cClientSession::m_qa_player_coins.find( playerIdx );
			if ( iter != cClientSession::m_qa_player_coins.end() )
			{
				iter->second = playerMoney;
			}
			else
			{
				cClientSession::m_qa_player_coins.insert( std::pair<uint64_t , uint64_t>( playerIdx , playerMoney ) );
			}
		}

		auto player_chip_map = request.member_stacks_map();
		for ( auto player_chip_pair : player_chip_map )
		{
			uint64 playerIdx = player_chip_pair.first;
			uint64 playerMoney = player_chip_pair.second;

			auto iter = cClientSession::m_qa_player_chips.find( playerIdx );
			if ( iter != cClientSession::m_qa_player_chips.end() )
			{
				iter->second = playerMoney;
			}
			else
			{
				cClientSession::m_qa_player_chips.insert( std::pair<uint64_t , uint64_t>( playerIdx , playerMoney ) );
			}
		}

	}
	else
	{
		cClientSession::m_qa_player_coins.clear();
		cClientSession::m_qa_player_chips.clear();
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ConsoleDeckPreset , response , General::ResultCode::Result_Success , "" );

	// 요청 내역 로그 출력
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( request , &serializedData );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , serializedData.c_str() );
}

void cProtoMsgStub::UidSearchByNickname( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::MidLookupByAliasRQ request;
	PmNet::MidLookupByAliasRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	std::string _nick_name = request.alias_label();

	// Async 코드
	std::future<BOOL> result = std::async( [_nick_name , &response]() {
		return QueryManager::PlayerIdxSearchByNickname( _nick_name , response );
	} );

	result.wait(); // 비동기 완료 대기

	std::string errorString;
	if ( FALSE == result.get() ) {
		std::string errorString = "[ UidSearchByNickname Failed ]";
	}
	else {
		std::string errorString = "[ UidSearchByNickname Success ]";
	}
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );

	GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_ConsoleAliasLookup , response , General::ResultCode::Result_Success , "" );
}

void cProtoMsgStub::InitLostLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::ResetLossCapRQ request;
	PmNet::ResetLossCapRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	// 닉네임으로 검색
	std::future<string> result = QueryManager::FindAccountGuidByNicknameAsync( request.alias_label() );
	result.wait();

	// 검색에 실패함
	std::string _account_guid = result.get();
	if ( _account_guid.size() == 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// Account 정보 부터 가져 온다.
	std::future<General::LossLimitProfile> lostLimitResult = QueryManager::GetLostLimitAsync( _account_guid );
	lostLimitResult.wait();

	General::LossLimitProfile _lost_limit = lostLimitResult.get();
	if ( _lost_limit.account_id() == 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( _lost_limit.account_id() );
	if ( lostLimit != nullptr )
	{
		// 캐슁 되어 있던 데이터가 있는 경우
		// 완전 초기화
		lostLimit->set_loss_limit_amount( 100000000000 );
		lostLimit->set_pending_loss_limit( 0 );
		lostLimit->set_loss_limit_changes( 0 );
		lostLimit->set_daily_chip_loss( 0 );

		std::string next_limit_string = TimeUtils::TomorrowStartTimeString();
		lostLimit->set_loss_limit_reset_at( next_limit_string );

		lostLimit->set_play_block_hours( 6 );
		lostLimit->set_pending_play_block_hours( 0 );

		if ( FALSE == QueryManager::UpdateLostLimit( lostLimit ) )
		{

			// 쿼리 실패

		}

		auto add_lost_limit = response.mutable_loss_cap();
		add_lost_limit->CopyFrom( *lostLimit );
	}
	else
	{
		// 캐슁 되어 있던 데이터가 없는 경우
		// 완전 초기화
		_lost_limit.set_loss_limit_amount( 100000000000 );
		_lost_limit.set_pending_loss_limit( 0 );
		_lost_limit.set_loss_limit_changes( 0 );
		_lost_limit.set_daily_chip_loss( 0 );

		std::string next_limit_string = TimeUtils::TomorrowStartTimeString();
		_lost_limit.set_loss_limit_reset_at( next_limit_string );

		_lost_limit.set_play_block_hours( 6 );
		_lost_limit.set_pending_play_block_hours( 0 );

		auto add_lost_limit = response.mutable_loss_cap();
		add_lost_limit->CopyFrom( _lost_limit );

		if ( FALSE == QueryManager::UpdateLostLimit( add_lost_limit ) )
		{

			// 쿼리 실패

		}
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response , General::ResultCode::Result_Success , "" );

}

void cProtoMsgStub::InitBuyLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::ResetSpendCapRQ request;
	PmNet::ResetSpendCapRS response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response );
		return;
	}

	// 닉네임으로 검색
	std::future<string> result = QueryManager::FindAccountGuidByNicknameAsync( request.alias_label() );
	result.wait();

	// 검색에 실패함
	std::string _account_guid = result.get();
	if ( _account_guid.size() == 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_GuidLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	// Account 정보 부터 가져 온다.
	std::future<General::LossLimitProfile> lostLimitResult = QueryManager::GetLostLimitAsync( _account_guid );
	lostLimitResult.wait();

	General::LossLimitProfile _lost_limit = lostLimitResult.get();
	if ( _lost_limit.account_id() == 0 ) {
		SendMessageAndLogWrite( General::ResultCode::Result_AccountLookupFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}

	auto lostLimit = NetLib::cSingleton<cLostLimitManager>::GetInstance()->GetLostLimit( _lost_limit.account_id() );
	if ( lostLimit != nullptr )
	{
		// 구매 제한 완전 초기화
		lostLimit->set_purchase_limit_amount( 700000 );
		lostLimit->set_monthly_purchase_total( 0 );

		if ( FALSE == QueryManager::UpdateLostLimit( lostLimit ) )
		{

			// 쿼리 실패

		}

		auto add_lost_limit = response.mutable_loss_cap();
		add_lost_limit->CopyFrom( *lostLimit );
	}
	else
	{
		// 구매 제한 완전 초기화
		_lost_limit.set_purchase_limit_amount( 700000 );
		_lost_limit.set_monthly_purchase_total( 0 );

		auto add_lost_limit = response.mutable_loss_cap();
		add_lost_limit->CopyFrom( _lost_limit );

		if ( FALSE == QueryManager::UpdateLostLimit( add_lost_limit ) )
		{

			// 쿼리 실패

		}
	}

	GetOwner()->SendBuffer( pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response , General::ResultCode::Result_Success , "" );
}

Server::ServiceStatusCode cProtoMsgStub::WebPostRequest( const std::string& url ,
		google::protobuf::Message& _postMessage ,
		UINT nCommand ,
		int iServerID ,
		uint64 playerIdx ,
		std::string& data ,
		std::string& billingerror ,
		const uint32 nThreadIndex ,
		const int default_time_out_second )
{
	if ( url.length() <= 0 )
	{
		return Server::ServiceStatusCode::ServiceStatus_EndpointRejected;
	}

	// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetWebProtobufBuffer( nThreadIndex );
	if ( pGOOGLE_PROTOBUF_BUFFER == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"WebPostRequest Failed. GetWebProtobufBuffer returns nullptr nThreadIndex : %u" ,
			nThreadIndex );

		return Server::ServiceStatusCode::ServiceStatus_MessageEncodeFailed;
	}

	pGOOGLE_PROTOBUF_BUFFER->Clear();
	if ( !_postMessage.SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->DataBuffer ) ) )
	{
		return Server::ServiceStatusCode::ServiceStatus_MessageEncodeFailed;
	}

	Server::ServerHeader serverHeader;
	serverHeader.set_datasize( ( google::protobuf::uint32 ) _postMessage.ByteSizeLong() );
	serverHeader.set_data( reinterpret_cast< char* >( pGOOGLE_PROTOBUF_BUFFER->DataBuffer ) , static_cast< size_t >( serverHeader.datasize() ) );
	serverHeader.set_msgid( ( General::PacketID ) nCommand );
	serverHeader.set_errorcode( Server::ServiceStatusCode::ServiceStatus_Success );
	serverHeader.set_playeridx( ( google::protobuf::int64 ) playerIdx );
	serverHeader.set_serverid( ( google::protobuf::int32 ) iServerID );

	if ( !serverHeader.SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer ) ) )
	{
		return Server::ServiceStatusCode::ServiceStatus_PostBodyEncodeFailed;
	}

	std::string innerdata;

	ULONGLONG llRequestStart = ::GetTickCount64();

	if ( NetLib::restsdkHttp::RequestHttp( CSNet::WEBREQ_METHOD::POST , url , std::string( reinterpret_cast< char* >( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer ) , static_cast< size_t >( serverHeader.ByteSizeLong() ) ) , innerdata , default_time_out_second ) == FALSE )
	{
		return Server::ServiceStatusCode::ServiceStatus_RuntimeFault;
	}

	ULONGLONG llRequestEnd = ::GetTickCount64();

	ULONGLONG llSum = llRequestEnd - llRequestStart;

	if ( llSum > CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_WEB_TIME_OUT )
	{
		return Server::ServiceStatusCode::ServiceStatus_RequestTimedOut;
	}

	Server::ServerHeader _serverHeader;
	if ( !_serverHeader.ParseFromArray( innerdata.c_str() , ( int ) innerdata.size() ) )
	{
		return Server::ServiceStatusCode::ServiceStatus_WebPayloadParseFailed;
	}
	billingerror = _serverHeader.billingerror();
	data = _serverHeader.data();

	return _serverHeader.errorcode();
}

std::future<Server::ServiceStatusCode> cProtoMsgStub::WebPostRequestAsync( const std::string& url ,
		google::protobuf::Message& _postMessage ,
		UINT nCommand ,
		int iServerID ,
		uint64 playerIdx ,
		std::string& data ,
		std::string& billingerror ,
		const uint32 nThreadIndex ,
		const int default_time_out_second
)
{
	return std::async( [url , &_postMessage , nCommand , iServerID , playerIdx , &data , &billingerror , nThreadIndex , default_time_out_second]() {
		return WebPostRequest( url , _postMessage , nCommand , iServerID , playerIdx , data , billingerror , nThreadIndex , default_time_out_second );
	} );
}

void cProtoMsgStub::ClientErrorLog( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	PmNet::FailTraceRQ _request;
	if ( _request.ParseFromArray( pData , nLength ) == false )
	{
		return;
	}
	auto result = QueryManager::InsertErrorLog( _request.fail_trace() );
	if ( false == result.get() )
	{
		std::string t_error = std::format( "ClientErrorLog Error [ {} ]" , _request.fail_trace() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , t_error.c_str() );
	}
}

void cProtoMsgStub::GetRoomNumber( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( CheckSession( pContext ) ) return;
	if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage ) {
		return;
	}
	cClientSession* pClientSession = GetSession( pContext );

	General::RoomNumberQuery request;
	General::RoomNumberResult response;
	if ( !request.ParseFromArray( pData , nLength ) ) {
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	cClientSession* playerSession = static_cast< cClientSession* >( NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Get( request.member_id() ) );
	if ( playerSession == nullptr )
	{
		SendMessageAndLogWrite( General::ResultCode::Result_TransportParseFailed , pContext , nThreadIndex , static_cast< General::PacketID >( nCommand ) , response ); return;
	}
	else
		response.set_room_no( playerSession->GetJoinedRoomNumber() );
	response.set_member_id( request.member_id() );

	std::string errorString;
	pClientSession->SendRequest( General::PacketID::Packet_ConsoleSpaceNumber , response , General::ResultCode::Result_Success , errorString );

}
#pragma endregion
