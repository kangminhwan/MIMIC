#include "cClientSession.h"
#include "cVirtualSession.h"
#include "../Include/Netlib/UdpModule/cUDPSession.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Network/cPacketStack.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cGameRoom.h"
#include "cGameRoomManager.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "cAssetLog.h"
#include "cMessageLog.h"

#include "Query.h"
#include "cMoneyLogInstance.h"

#include <iostream>
#include <format>
#include <future>

void cClientSession::SetMailBoxOnLogin( std::vector<PmNet::InboxDetail>& mail_list )
{
	m_mailBox.clear();

	for ( auto& mail : mail_list ) {

		m_mailBox.insert( std::pair< uint64, PmNet::InboxDetail >( mail.inbox_idx() , mail ));

	}
}

General::ResultCode cClientSession::RequestMailBox( const int& pageSize , const int& page , PmNet::InboxIndexRS& _response )
{
	if ( pageSize < 1 )
		return General::ResultCode::Result_PageSizeRequired;

	if ( page < 1 )
		return General::ResultCode::Result_PageIndexRequired;

	const int totalSize = m_mailBox.size();

	_response.set_total_cnt( totalSize );

	// pageSize 에 맞게 페이지 자른다.
	int totalPage = 0;
	if ( totalSize % pageSize == 0 ) {
		totalPage = totalSize / pageSize;
	}
	else {
		totalPage = totalSize / pageSize;
		totalPage += 1;
	}

	int pagingSize = pageSize > 0 ? pageSize : 10; // default size 10 으로 셋팅
	int startIndex = pagingSize * ( page - 1 );
	int endIndex = startIndex + pagingSize;

	std::vector<PmNet::InboxDetail> mailVector;
	for ( const auto& pair : m_mailBox ) {
		mailVector.push_back( pair.second );
	}

	//google::protobuf::RepeatedField<PmNet::InboxIndexRS> proto_list;
	auto mail_list = _response.mutable_inbox_list();
	if ( mail_list == nullptr )
		return General::ResultCode::Result_UnexpectedCondition;

	for ( int n = startIndex; n < mailVector.size(); ++n ) {
		if ( n > endIndex )
			break;

		const auto& mail_info = mailVector[ n ];

		auto add_mail_info = mail_list->Add();
		if ( add_mail_info == nullptr ) continue;

		add_mail_info->CopyFrom( mail_info );
	}


	return General::ResultCode::Result_Success;
}

General::ResultCode cClientSession::OpenMail( google::protobuf::RepeatedField<uint64> mail_idx_list , PmNet::InboxReadRS& _response )
{
	cMoneyLogInstance logInstance( this , Server::AssetLedgerSource::AssetLedger_Inbox );

	uint64 get_coin = 0;
	uint64 get_chip = 0;
	uint64 get_gem = 0;
	uint64 get_paid_gem = 0;
	uint64 get_kickoutTicket = 0;

	std::vector<std::future<BOOL>> results;
	std::vector<uint64> delete_list;
	std::vector<uint64> fail_list;

	for ( auto mail_idx : mail_idx_list ) {

		auto iter = m_mailBox.find( mail_idx );
		if ( iter == m_mailBox.end() ) continue;

		auto& mail_info = iter->second;

		cAssetLog assetLogInstance( this , 30105 );
		assetLogInstance.SetMailInfo( mail_idx,  mail_info );
		
		switch ( mail_info.bounty_kind() )
		{
		case General::GrantItemKind::GrantItem_FreeCoin:
		{
			get_coin += mail_info.cnt();

			// 맥스 홀딩 코인 재화 제한 실패 처리
			if ( GetCoin() + GetSafeCoin() + get_coin > GetMaxHoldingCoin() ) {
				get_coin -= mail_info.cnt();
				_response.add_fail_idx_list( mail_idx );
				continue;
			}

			// 승인 안된 메일 수령할경우 실패처리
			if ( mail_info.inbox_state() == (int)General::InboxState::InboxState_Requested || mail_info.inbox_state() == ( int ) General::InboxState::InboxState_Rejected ) {
				get_coin -= mail_info.cnt();
				_response.add_fail_idx_list( mail_idx );
				continue;
			}
			
			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			std::string daily_coin_limit_mail_count_str = "";
			if ( mail_type == "7" ) {
								int daily_coin_limit_mail_count = this->GetPlayerExt().coin_limit_mail_count();
				daily_coin_limit_mail_count_str = std::to_string( daily_coin_limit_mail_count );
			}

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40101 );
			messageLogInstance.SetData( reg_date , daily_coin_limit_mail_count_str , count );
			messageLogInstance.SetMsgid( std::to_string(mail_idx) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetReward( "Coin : " + std::to_string( mail_info.cnt() ) );
			messageLogInstance.SetEtc( "accept" );
			delete_list.push_back( mail_info.inbox_idx() );
		}
		break;
		case General::GrantItemKind::GrantItem_FreeChip:
		{
			get_chip += mail_info.cnt();

			// 맥스 홀딩 칩 재화 제한 실패 처리
			if ( GetChip() + get_chip > GetMaxHoldingChip() ) {
				get_chip -= mail_info.cnt();
				_response.add_fail_idx_list( mail_idx );
				continue;
			}


			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			std::string daily_coin_limit_mail_count_str = "";
			if ( mail_type == "7" ) {
								int daily_coin_limit_mail_count = this->GetPlayerExt().coin_limit_mail_count();
				daily_coin_limit_mail_count_str = std::to_string( daily_coin_limit_mail_count );
			}

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40101 );
			messageLogInstance.SetData( reg_date , daily_coin_limit_mail_count_str , count );
			messageLogInstance.SetMsgid( std::to_string( mail_idx ) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetReward( "Chip : " + std::to_string( mail_info.cnt() ) );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetEtc( "accept" );

			delete_list.push_back( mail_info.inbox_idx() );
		}
		break;
		case General::GrantItemKind::GrantItem_Avatar:
		{
			const int& avatar_id = mail_info.item_no();
			int period = mail_info.span() == 0 ? 7 : mail_info.span();
			std::string extime;
			// 보유 중인 아바타 인지 확인
			if ( hasAvatar( avatar_id ) ) {

				// 기간을 추가 해준다.
				auto iter = m_avatars.find( avatar_id );
				auto& avatar = iter->second;

				std::string expiry_string = avatar.expires_at();
				std::time_t expiry_time = std::time( nullptr );
				std::time_t now = std::time( nullptr );

				// 스트링이 비어 있는 경우의 예외 처리
				if ( !expiry_string.empty() )
					expiry_time = TimeUtils::StringToTimeTM( expiry_string );

				// 과거 처리
				if ( expiry_time < now ) {
					expiry_time = now;
				}

				// 기간 연장
				std::time_t new_expiry_time = TimeUtils::AddDays( expiry_time , period );
				std::string _new = TimeUtils::TMToString( new_expiry_time );
				extime = _new;
				avatar.set_expires_at( _new );

				results.push_back( QueryManager::PlayerUpdateAvatarAsnc( GetPlayerIdx() , avatar_id , _new ) );
			}
			else
			{
				// 구매 처리
				results.push_back( PlayerSetAvatar( avatar_id ) );
			}

			auto avatas_list = _response.add_skin_list();
			avatas_list->CopyFrom( GetAvatar( avatar_id ) );

			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			std::string daily_coin_limit_mail_count_str = "";
			if ( mail_type == "7" ) {
								int daily_coin_limit_mail_count = this->GetPlayerExt().coin_limit_mail_count();
				daily_coin_limit_mail_count_str = std::to_string( daily_coin_limit_mail_count );
			}

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40101 );
			messageLogInstance.SetData( reg_date , daily_coin_limit_mail_count_str , count );
			messageLogInstance.SetMsgid( std::to_string( mail_idx ) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetReward( "Avatar : " + std::to_string( avatar_id ) + "*" + extime );
			messageLogInstance.SetEtc( "accept" );

			delete_list.push_back( mail_info.inbox_idx() );

			_response.add_inbox_idx_list( mail_info.inbox_idx() );
		}
		break;
		case General::GrantItemKind::GrantItem_InventoryItem:
		break;
		case General::GrantItemKind::GrantItem_KickTicket:
		{
			int64 ticketcount = mail_info.cnt();

			AddKickOutTicket( ticketcount );

			get_kickoutTicket += ticketcount;

			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			std::string daily_coin_limit_mail_count_str = "";
			if ( mail_type == "7" ) {
								int daily_coin_limit_mail_count = this->GetPlayerExt().coin_limit_mail_count();
				daily_coin_limit_mail_count_str = std::to_string( daily_coin_limit_mail_count );
			}

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40101 );
			messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , "" , "" );
			messageLogInstance.SetMsgid( std::to_string( mail_idx ) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetReward( "KickoutTicket : " + std::to_string( ticketcount ) );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetEtc( "accept" );

			delete_list.push_back( mail_info.inbox_idx() );
		}
		break;
		case General::GrantItemKind::GrantItem_FreeGem:
		{
			get_gem += mail_info.cnt();

			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40101 );
			messageLogInstance.SetData( reg_date , "" , count );
			messageLogInstance.SetMsgid( std::to_string( mail_idx ) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetReward( "Gem : " + std::to_string( mail_info.cnt() ) );
			messageLogInstance.SetEtc( "accept" );
			delete_list.push_back( mail_info.inbox_idx() );
		}
			break;
		case General::GrantItemKind::GrantItem_PaidGem:
		{
			get_paid_gem += mail_info.cnt();

			auto messageData = QueryManager::GetMessageData( mail_idx );
			std::string reward_type_string = std::get<0>( messageData );
			std::string mail_type = std::get<1>( messageData );
			std::string mail_type_string = std::get<2>( messageData );
			std::string count = std::get<3>( messageData );
			std::string reg_date = std::get<4>( messageData );

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40101 );
			messageLogInstance.SetData( reg_date , "" , count );
			messageLogInstance.SetMsgid( std::to_string( mail_idx ) );
			messageLogInstance.SetMsgtype( mail_type );
			messageLogInstance.SetSender( mail_type_string );
			messageLogInstance.SetReward( "Paid_Gem : " + std::to_string( mail_info.cnt() ) );
			messageLogInstance.SetEtc( "accept" );
			delete_list.push_back( mail_info.inbox_idx() );
		}
			break;
		default:
		{
			fail_list.push_back( mail_idx );
		}
		break;
		}
	}

	// 메일을 삭제한다.
	if ( FALSE == QueryManager::DeleteMailBox( delete_list ) )
		return General::ResultCode::Result_MailRemoveFailed;

	for ( auto& mail_idx : delete_list ) {
		m_mailBox.erase( mail_idx );
		_response.add_inbox_idx_list( mail_idx );
	}

	// 재화 처리
	if ( get_coin > 0 ) {

		uint64 curCoin = GetCoin() + get_coin;
		SetCoin( General::PlayCategory::PlayCategory_None , curCoin );

		_response.set_tokens( get_coin );
	}

	if ( get_chip > 0 ) {

		uint64 curChip = GetChip() + get_chip;
		SetChip( General::PlayCategory::PlayCategory_None , curChip );

		_response.set_stacks( get_chip );
		UpdateQuests( General::TaskTrigger::TaskTrigger_ChipReach );
	}

	if ( get_kickoutTicket > 0 )
	{
		_response.add_item_nos( get_kickoutTicket );
	}

	if ( get_gem > 0 )
	{
		uint64 curGem = GetGem() + get_gem;
		SetGem( curGem );
		_response.set_gems( get_gem );
	}

	if ( get_paid_gem > 0 )
	{
		uint64 curPaid_Gem = GetPaidGem() + get_paid_gem;
		SetPaidGem( curPaid_Gem );
		_response.set_paid_gems( get_paid_gem );
	}

	// 재화 업데이트
	results.push_back( SavePlayer() );

	for( auto& result : results )
		result.wait();

	return General::ResultCode::Result_Success;
}