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

#include "Query.h"

#include <iostream>
#include <format>
#include <future>

std::future<BOOL> cClientSession::RecordsGet( const uint64& playerIdx , int& rows , PmNet::Ledger& records )
{
	//const uint64& playerIdx = m_player.member_id();
	
	return std::async( std::launch::async , [playerIdx , &rows , &records]() {
		return QueryManager::PlayerGetRecords( playerIdx , rows , records);
	} );
}

// 오늘자 Daily Today 데이터를 갱신한다.
void cClientSession::UpdateRecordDailyRefresh()
{
	m_records.clear_lowbadugi_today();
	m_records.clear_holdem_today();
	m_records.clear_baccarat_today();
	m_records.clear_blackjack_today();
	m_records.clear_slot_today();
	//m_records.clear_pinball_today();
	//m_records.clear_roulette_today();
}



std::future<BOOL> cClientSession::UpdateHoldemRecords( bool bDailyInit )
{
	const uint64& playerIdx = m_player.member_id();

	const auto& today = m_records.holdem_today();
	const auto& total = m_records.holdem_total();

	int participate_count = today.participation_total();
	int participate_count_total = total.participation_total();
	int win_count = today.win_total();
	int win_count_total = total.win_total();
	int lose_count = today.loss_total();
	int lose_count_total = total.loss_total();
	int64 best_get_chip = today.best_chip_gain();
	int64 best_get_chip_total = total.best_chip_gain();
	int64 best_get_coin = today.best_coin_gain();
	int64 best_get_coin_total = total.best_coin_gain();
	int make_all_in_count_total = 0; // 로우바두기만 처리 아직 처리 안함
	int straight_wins = 0; // 블랙잭만 처리
	int64 slot_get_coin = 0; // 슬롯만 처리
	int64 today_chip = ( bDailyInit == true ) ? 0 : today.today_chip_gain();
	int64 today_coin = ( bDailyInit == true ) ? 0 : today.today_coin_gain();
	string curDateTime = TimeUtils::GetCurrentDateTime();

	string updateQuery = QueryManager::UpdateRecordsInsertQuery( playerIdx ,
								"GameType_Holdem" ,
								participate_count ,
								participate_count_total ,
								win_count ,
								win_count_total ,
								lose_count ,
								lose_count_total ,
								best_get_chip ,
								best_get_chip_total ,
								best_get_coin ,
								best_get_coin_total ,
								make_all_in_count_total ,
								straight_wins ,
								slot_get_coin ,
								today_chip ,
								today_coin ,
								curDateTime );

	return QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , updateQuery );
}






void cClientSession::UpdateHoldemToday( const bool& isWin , const General::AssetKind moneyType , const int64 getMoney )
{
	// 참여 전적 증가
	const General::PlayRecordSummary records = m_records.holdem_today();
	int participate_count = records.participation_total();
	int win_count = records.win_total();
	int lose_count = records.loss_total();
	int64 best_get_chip = records.best_chip_gain();
	int64 best_get_coin = records.best_coin_gain();
	int64 today_get_coin = records.today_coin_gain();
	int64 today_get_chip = records.today_chip_gain();

	auto today = m_records.mutable_holdem_today();

	today->set_participation_total( participate_count + 1 );

	if ( isWin )
		today->set_win_total( win_count + 1 );
	else
		today->set_loss_total( lose_count + 1 );

	switch ( moneyType )
	{
	case General::AssetKind::AssetKind_Chip:
	{
		today->set_today_chip_gain( today_get_chip + getMoney );
		if ( getMoney > best_get_chip )
			today->set_best_chip_gain( getMoney );
	}
	break;
	case General::AssetKind::AssetKind_Coin:
	{
		today->set_today_coin_gain( today_get_coin + getMoney );
		if ( getMoney > best_get_coin )
			today->set_best_coin_gain( getMoney );
	}
	break;
	}

}

void cClientSession::UpdateHoldemTotal( const bool& isWin , const General::AssetKind moneyType , const int64 getMoney )
{
	// 참여 전적 증가
	const General::PlayRecordSummary& records = m_records.holdem_total();
	int participate_count = records.participation_total();
	int win_count = records.win_total();
	int lose_count = records.loss_total();
	int64 best_get_chip = records.best_chip_gain();
	int64 best_get_coin = records.best_coin_gain();

	auto total = m_records.mutable_holdem_total();

	total->set_participation_total( participate_count + 1 );

	if ( isWin )
		total->set_win_total( win_count + 1 );
	else
		total->set_loss_total( lose_count + 1 );

	switch ( moneyType )
	{
	case General::AssetKind::AssetKind_Chip:
	{
		if ( getMoney > best_get_chip )
			total->set_best_chip_gain( getMoney );
	}
	break;
	case General::AssetKind::AssetKind_Coin:
	{
		if ( getMoney > best_get_coin )
			total->set_best_coin_gain( getMoney );
	}
	break;
	}

}

















// 현재 연승 기록보다 클 경우에만 갱신 시킨다.
//void cClientSession::UpdateBlackjackStraightWins( const int& straightWins )
//{
//	const General::PlayRecordSummary& records = m_records.blackjack_total();
//	if ( straightWins > records.blackjack_streak_best() ) {
//		auto total = m_records.mutable_blackjack_total();
//		total->set_blackjack_streak_best( straightWins );
//	}
//}

void cClientSession::UpdateJokboRecords( const General::PlayCategory gameType , const bool& isWin , const General::HandRank jokbo , const int blackjackCount )
{
	// m_updateGameType 에 따라 처리 구분
	switch ( gameType )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		auto jokbos = m_records.mutable_holdem_hand_ranks();
		auto iter = jokbos->find( jokbo );
		if ( iter != jokbos->end() ) {
			auto& record = iter->second;

			int curTotal = record.occurrence_total();
			int curWin = record.win_total();

			if ( isWin )
				record.set_win_total( curWin + 1 );

			record.set_occurrence_total( curTotal + 1 );
		}
	}
	break;
	default:
		return;
	}

	m_updateGameTypes.push_back( gameType );
	m_updateJokbos.push_back( jokbo );
}

// 만약에 내가 올인 이면 증가 시키지 않음
// TODO 로우바둑이만 인지 확인 필요


// 승리한 족보 1개만 갱신이 되어야 한다.
std::future<BOOL> cClientSession::UpdateJokboRecord()
{
	std::vector<std::future<BOOL>> results;

	const uint64& playerIdx = m_player.member_id();

	std::string updateQuery;

	for ( int n = 0; n < m_updateGameTypes.size(); ++n ) {

		auto& updateType = m_updateGameTypes[ n ];
		auto& updateJokbo = m_updateJokbos[ n ];

		// m_updateGameType 에 따라 처리 구분
		switch ( updateType )
		{
		case General::PlayCategory::PlayCategory_TexasHoldem:
		{
			auto jokbos = m_records.mutable_holdem_hand_ranks();

			auto iter = jokbos->find( updateJokbo );
			if ( iter != jokbos->end() ) {
				auto& jokbo = iter->second;

				updateQuery = QueryManager::UpdateRecordsJokboQuery( playerIdx ,
					static_cast< int >( updateType ) ,
					static_cast< int >( updateJokbo ) ,
					jokbo.occurrence_total() ,
					jokbo.win_total() );
			}
		}
		break;
		default:
			return std::async( std::launch::async , [updateQuery]() {
				return FALSE;
			} );
		}

		results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , updateQuery ) );
	}

	for ( auto& result : results )
		result.wait();

	m_updateGameTypes.clear();
	m_updateJokbos.clear();

	return std::async( std::launch::async , []() {
		return TRUE;
	} );
}