#pragma once
#include "TableServerHeader.h"
#include "TimeUtils.h"

#include <iostream>
#include <map>

#include <google/protobuf/util/json_util.h>

#include "Query.h"
#include <future>
#include <codecvt>

// 매니저에 등록이 되는 경우
// 플레이어가 로그인 하는 경우에는 무조건 Set 처리
// 로비서버간 또는 슬롯서버에서는 update 만 진행
class cFriendManager
{
protected:
	std::map<uint64 , PmNet::MateDetail*> friendMap;
private:
	~cFriendManager()
	{
		for ( auto& pair : friendMap ) 
			delete pair.second;  // Delete the objects owned by the map.

		friendMap.clear();  // Clear the map after releasing its entries.
	}

public:

	PmNet::MateDetail* GetFriendInfo( const uint64& player_idx )
	{
		auto iter = friendMap.find( player_idx );
		if ( iter != friendMap.end() )
			return iter->second;

		return nullptr;
	}

	void SetFriendInfo( PmNet::MateDetail& friendInfo )
	{
		const uint64& playerIdx = friendInfo.member_info().member_id();

		PmNet::MateDetail* pFriendInfo = nullptr;
		
		auto iter = friendMap.find( playerIdx );
		if ( iter != friendMap.end() ) {

			pFriendInfo = iter->second;
			/*if ( pFriendInfo != nullptr ) {
				delete pFriendInfo;
				pFriendInfo = nullptr;
			}

			pFriendInfo = new PmNet::MateDetail();*/
			if ( pFriendInfo != nullptr )
				pFriendInfo->CopyFrom( friendInfo );
		}
		else
		{
			pFriendInfo = new PmNet::MateDetail();
			pFriendInfo->CopyFrom( friendInfo );
			friendMap.insert( std::pair< uint64 , PmNet::MateDetail* >( playerIdx , pFriendInfo ) );
		}
	}

	// ONLINE 상태이고
	// playing_channel_id 이 없고, slot_game_type 도 없으면
	// 로비 플레이어 이다.
	// 최대 6명 까지 카피한다.
	// 랜덤은 나중에 한다.
	std::vector<PmNet::MateDetail*> GetLobbyPlayers(const uint64 playerIdx) const {

		std::vector<uint64> friend_player_idx_list;
		auto result = QueryManager::GetFriendsAsync( playerIdx , friend_player_idx_list );
		result.wait();

		if ( result.get() ) {
		}

		std::vector<PmNet::MateDetail*> lobby_players;
		for ( auto friendInfoPair : friendMap ) {

			PmNet::MateDetail* pFriendInfo = friendInfoPair.second;
			if ( pFriendInfo == nullptr ) continue;
			if ( pFriendInfo->mate_state() != General::ContactState::ContactState_Online ) continue;
			if ( !pFriendInfo->active_ch_token().empty() ) continue;
			if ( pFriendInfo->reel_kind() != General::SlotCatalog::PM_SLOT_None ) continue;
			if ( pFriendInfo->member_info().member_id() == playerIdx ) continue;

			std::string nickname = pFriendInfo->member_info().display_name();
			int count = 0;
			for ( size_t i = 0; i < nickname.length(); ++i ) 
			{
				if ( ( nickname[ i ] & 0xC0 ) != 0x80 ) 
					++count;
			}
			if ( count > 8 ) continue;
			
			if ( std::find( friend_player_idx_list.begin() , friend_player_idx_list.end() , pFriendInfo->member_info().member_id() ) != friend_player_idx_list.end() ) continue;

			lobby_players.push_back( pFriendInfo );
			if ( lobby_players.size() >= 6 )
				break;
		}
		return lobby_players;
	}

	// 서버간 동기화시에만 사용된다.
	// 내 로컬부터 갱신하고, 다른 서버로 쏴버린다.
	// Disconnect 시에도 처리해 주어야 한다.
	void SyncFriendInfoStatus( const Server::SyncFriendInfoStatus& syncFriendInfoStatus )
	{
		auto iter = friendMap.find( syncFriendInfoStatus.player_idx() );
		if ( iter == friendMap.end() )
		{
			PmNet::MateDetail friendInfo;
			friendInfo.set_active_ch_token( syncFriendInfoStatus.playing_channel_id() );
			friendInfo.set_reel_kind( syncFriendInfoStatus.slot_game_type() );

			if ( General::ContactState::ContactState_Hidden != status )
				friendInfo.set_mate_state( syncFriendInfoStatus.friend_status() );

			friendInfo.mutable_member_info()->set_wallet_chips( syncFriendInfoStatus.player_chip() );
			friendInfo.mutable_member_info()->set_wallet_coins( syncFriendInfoStatus.player_coin() );
			friendInfo.mutable_member_info()->set_vault_chips( syncFriendInfoStatus.player_safe_chip() );
			friendInfo.mutable_member_info()->set_vault_coins( syncFriendInfoStatus.player_safe_coin() );

			General::ParticipantProfile* responsePlayer = friendInfo.mutable_member_info();
			if ( syncFriendInfoStatus.has_player_data() )
				responsePlayer->CopyFrom( syncFriendInfoStatus.player_data() );

			// Update the server ID only when a nonzero ID is supplied.
			if ( syncFriendInfoStatus.server_id() != 0 )
				friendInfo.set_node_id( syncFriendInfoStatus.server_id() );

			PmNet::Ledger* responseRecords = friendInfo.mutable_ledger();

			if ( responseRecords != nullptr )
			{
				PmNet::Ledger _records;
				_records.Clear();
				int _reads = 0;
				uint64_t playerIdx = syncFriendInfoStatus.player_idx();

				auto result = std::async( [playerIdx , &_reads , &_records]() {
					return QueryManager::PlayerGetRecords( playerIdx , _reads , _records );
				} );

				result.wait();

				if ( result.get() )
					responseRecords->CopyFrom( _records );
			}

			SetFriendInfo( friendInfo );
			return;
		}
		PmNet::MateDetail* friendInfo = iter->second;
		if ( friendInfo == nullptr )
			return;

		// 열수
		friendInfo->set_active_ch_token( syncFriendInfoStatus.playing_channel_id() );
		friendInfo->set_reel_kind( syncFriendInfoStatus.slot_game_type() );

		if ( syncFriendInfoStatus.friend_status() != General::ContactState::ContactState_Hidden )
			friendInfo->set_mate_state( syncFriendInfoStatus.friend_status() );

		friendInfo->mutable_member_info()->set_wallet_chips( syncFriendInfoStatus.player_chip() );
		friendInfo->mutable_member_info()->set_wallet_coins( syncFriendInfoStatus.player_coin() );
		friendInfo->mutable_member_info()->set_vault_chips( syncFriendInfoStatus.player_safe_chip() );
		friendInfo->mutable_member_info()->set_vault_coins( syncFriendInfoStatus.player_safe_coin() );

		General::ParticipantProfile* responsePlayer = friendInfo->mutable_member_info();
		if ( syncFriendInfoStatus.has_player_data() )
			responsePlayer->CopyFrom( syncFriendInfoStatus.player_data() );

		// Update the server ID only when a nonzero ID is supplied.
		if ( syncFriendInfoStatus.server_id() != 0 )
			friendInfo->set_node_id( syncFriendInfoStatus.server_id() );

		PmNet::Ledger* responseRecords = friendInfo->mutable_ledger();
		if ( responseRecords != nullptr )
		{
			PmNet::Ledger _records;
			_records.Clear();
			int _reads = 0;
			uint64_t playerIdx = syncFriendInfoStatus.player_idx();

			auto result = std::async( [playerIdx , &_reads , &_records]() {
				return QueryManager::PlayerGetRecords( playerIdx , _reads , _records );
			} );

			result.wait();

			if ( result.get() )
				responseRecords->CopyFrom( _records );
		}

		// Offline status updates should supply a zero server ID.

#ifdef _DEBUG

		//PmNet::MateDetail friendLog = *friendInfo;
		//
		////std::string serializedData;
		////google::protobuf::util::MessageToJsonString( friendLog , &serializedData );
		//TraceA( std::format( "player_idx {} Status {}" , syncFriendInfoStatus.player_idx() , (int)syncFriendInfoStatus.friend_status() ) );

#endif
	}

	// 서버간 동기화
	// 받는 즉시 갱신해준다.
	void SyncFriendInfo( const PmNet::MateDetail& friendInfo )
	{
		const uint64& playerIdx = friendInfo.member_info().member_id();

		auto iter = friendMap.find( playerIdx );
		if ( iter == friendMap.end() ) {

			PmNet::MateDetail* pFriendInfo = new PmNet::MateDetail();
			if ( pFriendInfo == nullptr )
				return;

			pFriendInfo->CopyFrom( friendInfo );
			friendMap.insert( std::pair< uint64 , PmNet::MateDetail* >( playerIdx , pFriendInfo ) );
		}
		else {

			PmNet::MateDetail* pFriendInfo = iter->second;
			if ( pFriendInfo == nullptr )
				return;

			pFriendInfo->CopyFrom( friendInfo );
		}
		
	}

	void GetChannelUserCount( std::vector<std::string>& channelIds , std::map<std::string , int>& channelUserCount )
	{
		int cnt = 0;
		for ( auto& pair : friendMap )
		{
			auto pFriendInfo = pair.second;

			if ( pFriendInfo->mate_state() == General::ContactState::ContactState_Offline )
				continue;

			if ( pFriendInfo->mate_state() == General::ContactState::ContactState_Online )
			{
				channelUserCount[ "Online" ]++;
				continue;
			}

			// 현재 하고 있는 슬롯의 종류
			General::SlotCatalog slotGameType = pFriendInfo->reel_kind();

			if ( slotGameType !=General::SlotCatalog::PM_SLOT_None )
			{
				std::string name = General::SlotCatalog_Name( slotGameType );
				channelUserCount[ name ]++;
			}

			for ( auto& channel : channelIds )
			{
				if ( pFriendInfo->active_ch_token().find( channel ) != std::string::npos )
				{
					channelUserCount[ channel ]++;
					break;
				}
			}
		}
	}

	std::map<uint64 , PmNet::MateDetail*>* GetFriendMap() { return &friendMap; }
};
