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
#include "TimeUtils.h"
#include "cMoneyLogInstance.h"

#include "Query.h"

#include <iostream>
#include <format>
#include <future>

void cClientSession::SetMissionAndAchieve( std::map<uint32 , General::TaskProgress>& daily_missions , std::map<uint32 , General::TaskProgress>& lounge_missions , std::map<uint32 , General::TaskProgress>& achievements )
{
	m_daily_missions = daily_missions;
	m_lounge_missions = lounge_missions;
	m_achievements = achievements;
}

void cClientSession::RequestMissionAndAchieve( const General::TaskCategory& achieve_type , PmNet::FetchGoalsRS& _response )
{
	if ( achieve_type == General::TaskCategory::TaskCategory_None || achieve_type == General::TaskCategory::TaskCategory_Mission ) {

		auto missions = _response.mutable_daily_goals();

		for ( auto& questPair : m_daily_missions )
		{
			auto adder = missions->Add();
			adder->CopyFrom( questPair.second );
		}
	}

	if ( achieve_type == General::TaskCategory::TaskCategory_None || achieve_type == General::TaskCategory::TaskCategory_LoungeMission ) {

		auto lounges = _response.mutable_atrium_goals();

		for ( auto& questPair : m_lounge_missions )
		{
			auto adder = lounges->Add();
			adder->CopyFrom( questPair.second );
		}
	}

	if ( achieve_type == General::TaskCategory::TaskCategory_None || achieve_type == General::TaskCategory::TaskCategory_Achievement ) {

		auto achievements = _response.mutable_trophies();

		for ( auto& questPair : m_achievements )
		{
			auto adder = achievements->Add();
			adder->CopyFrom( questPair.second );
		}
	}
}

void cClientSession::RequestMissionAndAchieve( const General::TaskCategory& achieve_type , google::protobuf::RepeatedPtrField<General::TaskProgress>* quests )
{
	if ( quests == nullptr )
		return;

	switch ( achieve_type )
	{
	case General::TaskCategory::TaskCategory_Mission:
	{
		for ( auto& questPair : m_daily_missions ) {

			auto& quest = questPair.second;

			auto adder = quests->Add();
			adder->CopyFrom(quest);
		}
	}
	break;
	case General::TaskCategory::TaskCategory_LoungeMission:
	{
		for ( auto& questPair : m_lounge_missions ) {

			auto& quest = questPair.second;

			auto adder = quests->Add();
			adder->CopyFrom( quest );
		}
	}
	break;
	case General::TaskCategory::TaskCategory_Achievement:
	{
		for ( auto& questPair : m_achievements ) {

			auto& quest = questPair.second;

			auto adder = quests->Add();
			adder->CopyFrom( quest );
		}
	}
	break;
	}
}

// 퀘스트 보상 지급
BOOL cClientSession::GetQuestReward( const General::TaskCategory& achieve_type , const int& quest_id , uint64& get_coin , uint64& get_chip )
{
	std::vector<std::future<BOOL>> results;

	get_coin = get_chip = 0;

	const auto& rewards = NetLib::cSingleton<cDataLoader>::GetInstance()->GetQuestRewards();
	auto iterReward = rewards.find( quest_id );
	if ( iterReward == rewards.end() )
		return FALSE;

	// AssetLedgerSource 타입을 결정한다.
	Server::AssetLedgerSource money_event_type = achieve_type == General::TaskCategory::TaskCategory_Achievement ? Server::AssetLedgerSource::AssetLedger_Achievement : Server::AssetLedgerSource::AssetLedger_Mission;
	cMoneyLogInstance logInstance( this , money_event_type );

	const uint64& playerIdx = GetPlayerIdx();

	const uint64& rewardMoney = iterReward->second;

	const auto& quests = NetLib::cSingleton<cDataLoader>::GetInstance()->GetQuests();
	auto iter = quests.find( quest_id );
	if ( iter != quests.end() ) {

		const auto& quest = iter->second;

		switch ( achieve_type )
		{
		case General::TaskCategory::TaskCategory_Mission:
		{
			auto iter_daily = m_daily_missions.find( quest_id );
			if ( iter_daily == m_daily_missions.end() )
				return FALSE;

			auto& _quest = iter_daily->second;
			if ( _quest.grant_claimed() || _quest.current_progress() != _quest.target_progress() )
				return FALSE;

			if ( _quest.grant_kind() == General::GrantItemKind::GrantItem_FreeCoin ) {

				const uint64& currentMoney = GetCoin();
				SetCoin( General::PlayCategory::PlayCategory_None , rewardMoney + currentMoney );
				get_coin = rewardMoney;
			}
			else {
				const uint64& currentMoney = GetChip();
				SetChip( General::PlayCategory::PlayCategory_None , rewardMoney + currentMoney );
				get_chip = rewardMoney;
			}

			results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateCompleteQuest( playerIdx , quest.task_id() , TRUE ) ) );
			results.push_back( SavePlayer() );
			_quest.set_grant_claimed( true );

			for ( auto& result : results )
				result.wait();

			return TRUE;
		}
		break;
		case General::TaskCategory::TaskCategory_LoungeMission:
		{
			auto iter_lounge = m_lounge_missions.find( quest_id );
			if ( iter_lounge == m_lounge_missions.end() )
				return FALSE;

			auto& _quest = iter_lounge->second;
			if ( _quest.grant_claimed() || _quest.current_progress() != _quest.target_progress() )
				return FALSE;

			if ( _quest.grant_kind() == General::GrantItemKind::GrantItem_FreeCoin ) {

				const uint64& currentMoney = GetCoin();
				SetCoin( General::PlayCategory::PlayCategory_None , rewardMoney + currentMoney );
				get_coin = rewardMoney;
			}
			else {
				const uint64& currentMoney = GetChip();
				SetChip( General::PlayCategory::PlayCategory_None , rewardMoney + currentMoney );
				get_chip = rewardMoney;
			}

			results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateCompleteQuest( playerIdx , quest.task_id() , TRUE ) ) );
			results.push_back( SavePlayer() );
			_quest.set_grant_claimed( true );

			for ( auto& result : results )
				result.wait();

			return TRUE;
		}
		break;
		case General::TaskCategory::TaskCategory_Achievement:
		{
			auto iter_achievement = m_achievements.find( quest_id );
			if ( iter_achievement == m_achievements.end() )
				return FALSE;

			auto& _quest = iter_achievement->second;
			if ( _quest.grant_claimed() || _quest.current_progress() < _quest.target_progress() )
				return FALSE;

			if ( _quest.grant_kind() == General::GrantItemKind::GrantItem_FreeCoin ) {

				const uint64& currentMoney = GetCoin();
				SetCoin( General::PlayCategory::PlayCategory_None , rewardMoney + currentMoney );
				get_coin = rewardMoney;
			}
			else {
				const uint64& currentMoney = GetChip();
				SetChip( General::PlayCategory::PlayCategory_None , rewardMoney + currentMoney );
				get_chip = rewardMoney;
			}

			results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateCompleteQuest( playerIdx , quest.task_id() , TRUE ) ) );
			results.push_back( SavePlayer() );
			_quest.set_grant_claimed( true );

			for ( auto& result : results )
				result.wait();

			return TRUE;
		}
		break;
		}


	}

}

// TaskTrigger 에 해당하는 퀘스트 갱신
// 연속 퀘스트의 순서대로 진행이 되어야 한다.
std::vector<uint32> cClientSession::UpdateQuests( const General::TaskTrigger& achieve_event_type , const uint64& increaseCount)
{
	std::vector<uint32> quest_ids;

	std::vector<std::future<BOOL>> results;

	const uint64 playerIdx = GetPlayerIdx();

	for ( auto& questPair : m_daily_missions ) {
		auto& quest = questPair.second;

		if ( quest.task_trigger() == achieve_event_type ) {

			if ( UpdateQuest( quest , increaseCount ) ) {
				quest_ids.push_back( quest.task_id() );
				results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateUpdateQuest( playerIdx , quest.task_id() , quest.current_progress() ) ) );
			}
		}
	}

	for ( auto& questPair : m_lounge_missions ) {
		auto& quest = questPair.second;

		if ( quest.task_trigger() == achieve_event_type ) {

			if ( UpdateQuest( quest , increaseCount ) ) {
				quest_ids.push_back( quest.task_id() );
				results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateUpdateQuest( playerIdx , quest.task_id() , quest.current_progress() ) ) );
			}
		}
	}

	for ( auto& questPair : m_achievements ) {
		auto& quest = questPair.second;

		if ( quest.task_trigger() == achieve_event_type ) {

			if ( UpdateQuest( quest , increaseCount ) ) {
				quest_ids.push_back( quest.task_id() );
				results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateUpdateQuest( playerIdx , quest.task_id() , quest.current_progress() ) ) );
			}
		}
	}

	for ( auto& result : results ) {

		result.wait();

		if ( FALSE == result.get() ) {

			// 실패 처리


		}
	}

	return quest_ids;
}

std::vector<uint32> cClientSession::UpdateExceptLoungeQuests( const General::TaskTrigger& achieve_event_type , const uint64& increaseCount )
{
	std::vector<uint32> quest_ids;

	std::vector<std::future<BOOL>> results;

	const uint64 playerIdx = GetPlayerIdx();

	for ( auto& questPair : m_daily_missions ) {
		auto& quest = questPair.second;

		if ( quest.task_trigger() == achieve_event_type ) {

			if ( UpdateQuest( quest , increaseCount ) ) {
				quest_ids.push_back( quest.task_id() );
				results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateUpdateQuest( playerIdx , quest.task_id() , quest.current_progress() ) ) );
			}
		}
	}

	for ( auto& questPair : m_achievements ) {
		auto& quest = questPair.second;

		if ( quest.task_trigger() == achieve_event_type ) {

			if ( UpdateQuest( quest , increaseCount ) ) {
				quest_ids.push_back( quest.task_id() );
				results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateUpdateQuest( playerIdx , quest.task_id() , quest.current_progress() ) ) );
			}
		}
	}

	for ( auto& result : results ) {

		result.wait();

		if ( FALSE == result.get() ) {

			// 실패 처리


		}
	}

	return quest_ids;
}

void cClientSession::UpdateExceptLoungeQuests( const General::TaskTrigger& achieve_event_type , std::map<uint64 , int64>& all_in_map )
{
	int all_in_count = all_in_map.size();
	/*auto iter = all_in_map.find( m_player.member_id() );
	if ( iter == all_in_map.end() )
		return;*/

	UpdateExceptLoungeQuests( achieve_event_type , all_in_count );
}

std::vector<uint32> cClientSession::UpdateLoungeQuests( const General::TaskTrigger& achieve_event_type , const uint64& increaseCount )
{
	std::vector<uint32> quest_ids;

	std::vector<std::future<BOOL>> results;

	const uint64 playerIdx = GetPlayerIdx();
	for ( auto& questPair : m_lounge_missions ) {
		auto& quest = questPair.second;

		if ( quest.task_trigger() == achieve_event_type ) {

			if ( UpdateQuest( quest , increaseCount ) ) {
				quest_ids.push_back( quest.task_id() );
				results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateUpdateQuest( playerIdx , quest.task_id() , quest.current_progress() ) ) );
			}
		}
	}

	for ( auto& result : results ) {

		result.wait();

		if ( FALSE == result.get() ) {

			// 실패 처리


		}
	}

	return quest_ids;
}

BOOL cClientSession::UpdateQuest( General::TaskProgress& _quest , const uint64& increaseCount )
{
	switch ( _quest.task_trigger() )
	{
	case General::TaskTrigger::TaskTrigger_LowBadugiPlay:		// 로우바둑이 플레이 횟수
	case General::TaskTrigger::TaskTrigger_LowBadugiEnemyAllIn:	// 로우바둑이 상대방 올인 시킨 횟수
	case General::TaskTrigger::TaskTrigger_LowBadugiSevenMade:	// 로우바둑이 7 Made 이상 족보 횟수
	case General::TaskTrigger::TaskTrigger_LowBadugiWin:	// 로우바둑이 승리 카운트
	case General::TaskTrigger::TaskTrigger_HoldemPlay:			// 홀덤 플레이 횟수
	case General::TaskTrigger::TaskTrigger_HoldemWin:		// 홀덤 승리 횟수
	case General::TaskTrigger::TaskTrigger_HoldemFlush:	// 홀덤 족보 플러쉬이상 횟수
	case General::TaskTrigger::TaskTrigger_HoldemEnemyAllIn:	// 홀덤 족보 플러쉬이상 횟수
	case General::TaskTrigger::TaskTrigger_BaccaratPlay:			// 바카라 플레이 횟수
	case General::TaskTrigger::TaskTrigger_BaccaratWin:		// 바카라 승리 횟수
	case General::TaskTrigger::TaskTrigger_BlackjackPlay:		// 블랙잭 플레이 횟수
	case General::TaskTrigger::TaskTrigger_BlackjackWin:	// 블랙잭 승리 횟수
	case General::TaskTrigger::TaskTrigger_SlotSpin:		// 슬롯 스핀 횟수 ( 5 코인 이상 베팅한 경우 )
	case General::TaskTrigger::TaskTrigger_AliasChange:		// 닉네임 변경 횟수
	case General::TaskTrigger::TaskTrigger_EnemyAllIn:			// 상대방 올인 시킨 횟수
	case General::TaskTrigger::TaskTrigger_AdView:			// 광고 시청하기
	case General::TaskTrigger::TaskTrigger_FriendRequest:			// 친구 신청하기 횟수
	{
		const uint64 curCount = _quest.current_progress();
		if ( curCount < _quest.target_progress() ) {
			_quest.set_current_progress( curCount + increaseCount );
			return TRUE;
		}
	}
	break;
	case General::TaskTrigger::TaskTrigger_ChipReach:			// 칩 보유하기 (수량)
	{
		const uint64 curCount = _quest.current_progress();

		// 완료 상태 인가?
		if ( curCount < _quest.target_progress() ) {

			// 보유 칩이 Goal 보다 큰지 확인
			uint64 currentChip = GetChip();
			if ( currentChip >= _quest.target_progress() ) {
				_quest.set_current_progress( _quest.target_progress() );
				return TRUE;
			}
		}
	}
	break;
	case General::TaskTrigger::TaskTrigger_GameWinChip:		// 게임 승리로 칩 획득하기 (수량)
	{
		const uint64 curCount = _quest.current_progress();

		// 완료 상태 인가?
		if ( curCount < _quest.target_progress() ) {

			// 보유 칩이 Goal 보다 큰지 확인
			if ( increaseCount >= _quest.target_progress() ) {
				_quest.set_current_progress( _quest.target_progress() );
				return TRUE;
			}
		}
	}
	break;
	}

	return FALSE;
}

void cClientSession::UpdateQuest( General::TaskTrigger achieveEventType , std::map<uint64 , int64>& all_in_map )
{
	int all_in_count = all_in_map.size();
	auto iter = all_in_map.find( m_player.member_id() );
	if ( iter == all_in_map.end() )
		return;

	UpdateQuests( achieveEventType , all_in_count );
}

// DailyMissions , LoungeMissions 초기화
void cClientSession::UpdateQuestsDailyRefresh()
{
	std::vector<std::future<BOOL>> results;

	const uint64 playerIdx = GetPlayerIdx();

	for ( auto& questPair : m_daily_missions ) {

		auto& quest = questPair.second;
		quest.set_current_progress( 0 );
		quest.set_grant_claimed( false );

		results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateResetQuest( playerIdx , quest.task_id() ) ) );
	}

	for ( auto& questPair : m_lounge_missions ) {

		auto& quest = questPair.second;
		quest.set_current_progress( 0 );
		quest.set_grant_claimed( false );

		results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateResetQuest( playerIdx , quest.task_id() ) ) );
	}

	for ( auto& result : results ) {

		result.wait();

		if ( FALSE == result.get() ) {

			// 실패 처리


		}
	}
}

//bool cClientSession::IsOver7Made( General::HandRank& jokbo )
//{
//	switch ( jokbo )
//	{
//	case General::HandRank::HandRank_LowGolf:
//	case General::HandRank::HandRank_LowSecond = 2 ,
//	case General::HandRank::HandRank_LowThird = 3 ,
//	case General::HandRank::HandRank_LowFiveMade = 4 ,
//	case General::HandRank::HandRank_LowSixMade = 5 ,
//	case General::HandRank::HandRank_LowSevenMade = 6 ,
//	case General::HandRank::HandRank_LowEightMade = 7 ,
//	case General::HandRank::HandRank_LowNineMade = 8 ,
//	case General::HandRank::HandRank_LowTenMade = 9 ,
//	case General::HandRank::HandRank_LowJackMade = 10 ,
//	case General::HandRank::HandRank_LowQueenMade = 11 ,
//	case General::HandRank::HandRank_LowKingMade = 12 ,
//	case General::HandRank::HandRank_LowBase = 13 ,
//	case General::HandRank::HandRank_LowTwoBase = 14 ,
//	case General::HandRank::HandRank_LowTop = 15 ,
//	}
//	return false;
//}