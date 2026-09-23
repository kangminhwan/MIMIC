#include "TableServerHeader.h"
#include "cHoldem.h"
#include "cCardDeck.h"
#include "cGameRoom.h"
#include "cClientSession.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "cLogicException.h"
#include "cHoldemKicker.h"
#include "cHoldemSameJokboCompare.h"
#include "cQADecks.h"
#include "Query.h"
#include "cGameLogInstance.h"
#include "cGameLogStatisticInstance.h"
#include "cMaintenanceManager.h"

#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Common/cSingleton.h"

#include <iostream>
#include <format>
#include <string.h>
#include <google/protobuf/util/json_util.h>
#include "AuthCodeGenerator.h"

using protoutil::cProtoUtil;

void cHoldem::InitGame()
{
	m_pGameRoom->SetMaxRoomPlayerCnt( m_maxPlayerCnt );

	// 플레이어 슬롯 초기화
	m_playerSlots.clear();
	for ( int n = 0; n < m_maxPlayerCnt; ++n ) {
		m_playerSlots.push_back( nullptr );
		m_slotReservation.push_back( nullptr );
	}

	Shuffle();
	MakeRoomStatusWait();
	DecideDealer();

	// 현재 배팅 슬롯, 베팅 오더 초기화
	m_curBetRound = E_BET_ROUND::NONE;
	m_lastPlayerBetTotal = 0;
	m_stepPlayerBet = 0;
	m_pGameRoom->m_roomInfo.set_active_bet_seat( 0 );
	CreateStockers();

	// 베팅 타입을 방 유형에 따라 결정이 필요하다.
	m_allowedBetTypes.RemoveAll();
	m_firstidx = 0;
	General::AssetKind moneyType = m_pGameRoom->m_roomInfo.asset_kind();
	General::BetPolicy betRuleType = m_pGameRoom->m_roomInfo.bet_policy();
	m_diedelay = 1000;
	if(moneyType == General::AssetKind::AssetKind_Coin )
		SetAutoStartCnt( 3 );
	else
		SetAutoStartCnt( m_maxPlayerCnt );

	/*switch ( betRuleType )
	{
	case General::BetPolicy::BetPolicy_None:
		SetAutoStartCnt( 2 );
		break;
	case General::BetPolicy::BetPolicy_HalfStake:
		SetAutoStartCnt( 2 );
		break;
	case General::BetPolicy::BetPolicy_FullStake:
		SetAutoStartCnt( 2 );
		break;
	case General::BetPolicy::BetPolicy_HoldemOpening:
		SetAutoStartCnt( 2 );
		break;
	case General::BetPolicy::BetPolicy_HoldemStandard:
		SetAutoStartCnt( 3 );
		break;
	case General::BetPolicy::BetPolicy_HoldemThreeBet:
		SetAutoStartCnt( 4 );
		break;
	case General::BetPolicy::BetPolicy_HoldemFourBet:
		SetAutoStartCnt( 5 );
		break;
	default:
		break;
	}*/


	// 딜러 초기화
	m_dealerIter = m_playerSlots.end();

	// 베팅순서 Iterator 초기화
	m_curBetPlayerIter = m_betSequence.end();

	// 승리 기록 삭제
	m_win_jokbos.clear();

	// 사이드 팟 기록 삭제
	cSidePotManager::Clear();

	// Kick 유저 목록 삭제
	ClearKickedPlayer();

	// 입장 금지 플레이어 초기화
	m_joinProhibited.clear();

	// Vote 시스템 초기화
	//InitVoteSystem();

	for ( auto buttonNum : IGame::m_channel.button_list() ) {
		auto button = static_cast< General::TableAction >( buttonNum );
		m_allowedBetTypes.SetAt( button , button );
	}
	// AllIn 은 공통이라 모두 추가해줌
	m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );

	switch ( moneyType )
	{
	/*case General::AssetKind::AssetKind_Chip:
	{
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_SeedOnly , General::TableAction::TableAction_SeedOnly );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_DoubleRaise , General::TableAction::TableAction_DoubleRaise );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );
	}
	break;*/
	case General::AssetKind::AssetKind_Coin:
	{
		switch ( betRuleType )
		{
		/*case General::BetPolicy::BetPolicy_FullStake:
		{
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_SeedOnly , General::TableAction::TableAction_SeedOnly );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_FullPot , General::TableAction::TableAction_FullPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );
		}
		break;
		case General::BetPolicy::BetPolicy_HalfStake:
		{
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_SeedOnly , General::TableAction::TableAction_SeedOnly );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );
		}
		break;*/
		case General::BetPolicy::BetPolicy_HoldemStandard: // 홀덤 일반룰
		{
			/*m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_FullPot , General::TableAction::TableAction_FullPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Maximum , General::TableAction::TableAction_Maximum );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );*/
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_SmallBlind , General::TableAction::TableAction_SmallBlind );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_BigBlind , General::TableAction::TableAction_BigBlind );
		}
		break;
		case General::BetPolicy::BetPolicy_HoldemThreeBet: // 홀덤 3 Bet 룰
		{
			/*m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_FullPot , General::TableAction::TableAction_FullPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Maximum , General::TableAction::TableAction_Maximum );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );*/
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_SmallBlind , General::TableAction::TableAction_SmallBlind );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_BigBlind , General::TableAction::TableAction_BigBlind );
			//m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiantBlind , General::TableAction::TableAction_GiantBlind );
		}
		break;
		case General::BetPolicy::BetPolicy_HoldemFourBet: // 홀덤 3 Bet 룰
		{
			/*m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_FullPot , General::TableAction::TableAction_FullPot );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_Maximum , General::TableAction::TableAction_Maximum );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );*/
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_SmallBlind , General::TableAction::TableAction_SmallBlind );
			m_allowedBetTypes.SetAt( General::TableAction::TableAction_BigBlind , General::TableAction::TableAction_BigBlind );
			//m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiantBlind , General::TableAction::TableAction_GiantBlind );
			//m_allowedBetTypes.SetAt( General::TableAction::TableAction_RoyalBlind , General::TableAction::TableAction_RoyalBlind );
		}
		break;
		}
	}
	break;
	}

	// 친구방이면 다시 재정의 합니다.
	if ( m_pGameRoom->m_roomInfo.access_mode() == General::RoomAccessMode::RoomAccess_FriendOnly ) {
		m_allowedBetTypes.RemoveAll();

		m_allowedBetTypes.SetAt( General::TableAction::TableAction_GiveUp , General::TableAction::TableAction_GiveUp );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_SeedOnly , General::TableAction::TableAction_SeedOnly );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_DoubleRaise , General::TableAction::TableAction_DoubleRaise );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_Check , General::TableAction::TableAction_Check );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_Call , General::TableAction::TableAction_Call );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_QuarterPot , General::TableAction::TableAction_QuarterPot );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_HalfPot , General::TableAction::TableAction_HalfPot );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_FullPot , General::TableAction::TableAction_FullPot );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_Maximum , General::TableAction::TableAction_Maximum );
		m_allowedBetTypes.SetAt( General::TableAction::TableAction_AllIn , General::TableAction::TableAction_AllIn );
	}

	// 딜러 수수료 처리
	//SetDealerFee( m_gameType , moneyType , m_pGameRoom->m_roomInfo.room_type() );
	SetDealerFee( m_channel.dealer_fee() );

	// 참여 큐 삭제
	m_participateQueue.clear();

	// 블라인드 베팅금액 초기화
	m_blindBet = 0;

	// 맥스 베팅 관련 초기화
	m_max_bet_player_exists = false;

	// GameUID
	m_gameUid = "";

	m_kick_out_count = 0; // 새로 추가

	game_record_string = "";

	//로그데이터 초기화
	for ( int i = 0; i < m_maxPlayerCnt; ++i )
	{
		p_id_int[ i ] = 0;
		p_id[ i ] = "";
		p_asset[ i ] = 0;
		p_play_cnt[ i ] = 0;
		p_change_coin[ i ] = 0;
		p_discard_coin[ i ] = 0;
		p_fee_coin[ i ] = 0;
		p_fee_info[ i ] = "";
		p_rakeback[ i ] = 0;
		p_coin[ i ] = 0;
		p_result[ i ] = "";

		win_id[ i ] = "";
		win_asset[ i ] = 0;
	}
}

void cHoldem::ResetGame()
{
	m_playerSlots.clear();
	m_deck.clear();
	m_stockDeck.clear();
	for ( auto player : m_betSequence )
	{
		if ( player == nullptr )
			continue;
		if ( player->is_copyed )
			delete player;
	}
	m_betSequence.clear();

	// 커뮤니티 카드 초기화
	m_communityCards.clear();

	m_lastPlayerBetTotal = 0;
	m_stepPlayerBet = 0;
	m_bCheatCardChange = false;

	// 참여 큐 삭제
	m_participateQueue.clear();

	// 블라인드 베팅금액 초기화
	m_blindBet = 0;

	// 맥스 베팅 관련 초기화
	m_max_bet_player_exists = false;
}

// Wait 상태에서 하는 일이 아무것도 없소이다.
// 인원이 Start 를 할 수 있는 상태가 되면 Start Wait 상태로 바꿔준다.
void cHoldem::Waiting()
{
	//if ( GetMemberCnt() >= GetMinStartPlayerCount() ) {

	//	// 풀방이면 AutoStart 가능하도록 변경
	//	if ( GetPlayingPlayerCount() == m_maxPlayerCnt ) {

	//		MakeRoomAutoStart( GetPlayingPlayerCount() );
	//	}
	//	else
	//	{
	//		MakeRoomStatusStartWait();
	//	}

	//	// SendStatusChange 파라미터는 before Status 입니다.
	//	SendStatusChange( Server::PlayPhase::PlayPhase_Waiting , "cHoldem::Waiting" );
	//}
}

// 방장이 시작 버튼을 눌렀을때
// 시드 금액이 부족한 유저가 있으면 플레이 시키지 않습니다.
General::ResultCode cHoldem::StartGame( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr )
		return General::ResultCode::Result_NullParameterFault;

	if ( m_gameStep != Server::PlayPhase::PlayPhase_Waiting && m_gameStep != Server::PlayPhase::PlayPhase_StartReady )
		return General::ResultCode::Result_PlayStepMismatch;

	// 현재 호스트만 시작을 누를수 있음
	if ( GetBossPlayerIdx() != pClientSession->GetPlayerIdx() )
		return General::ResultCode::Result_OwnerActionRequired;

	// 세션 정리
	std::set<cClientSession*> WrongSessions;
	int count = m_pGameRoom->ClearWrongSession( WrongSessions );
	if ( count > 0 )
	{
		std::ostringstream oss;
		oss << "ClearWorngSession2 : ";
		for ( int slot = 0; slot < m_playerSlots.size(); ++slot )
		{
			cClientSession* player = m_playerSlots[ slot ];
			if ( player == nullptr )
				continue;

			if ( WrongSessions.contains( player ) )
			{
				/*player->SessionLogout( 0 );
				player->DisConnectContext( 0 , 0 );
				player->SyncFriend_Online( true , player , General::ContactState::ContactState_Offline );*/
				oss << "Slots - (" << player->GetPlayerIdx() << "),";
			}

			if ( m_pGameRoom->GetRoomNumber() != player->GetJoinedRoomNumber() )
			{
				m_playerSlots[ slot ] = nullptr;
				PmNet::ChamberLeaveRS _res;
				_res.set_chamber_no( m_pGameRoom->GetRoomNumber() );
				_res.set_member_idx( player->GetPlayerIdx() );
				_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() );
				m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
			}
		}
		for ( int slot = 0; slot < m_slotReservation.size(); ++slot )
		{
			cClientSession* player = m_slotReservation[ slot ];
			if ( player == nullptr )
				continue;
			if ( WrongSessions.contains( player ) )
			{
				/*player->SessionLogout( 0 );
				player->DisConnectContext( 0 , 0 );
				player->SyncFriend_Online( true , player , General::ContactState::ContactState_Offline );*/
				oss << "slotReserv - (" << player->GetPlayerIdx() << "),";
			}

			if ( m_pGameRoom->GetRoomNumber() != player->GetJoinedRoomNumber() )
			{
				m_playerSlots[ slot ] = nullptr;
				PmNet::ChamberLeaveRS _res;
				_res.set_chamber_no( m_pGameRoom->GetRoomNumber() );
				_res.set_member_idx( player->GetPlayerIdx() );
				_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() );
				m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
			}
		}
		for ( int slot = 0; slot < m_participateQueue.size(); ++slot )
		{
			cClientSession* player = m_participateQueue[ slot ];
			if ( player == nullptr )
				continue;

			if ( WrongSessions.contains( player ) )
			{
				/*player->SessionLogout( 0 );
				player->DisConnectContext( 0 , 0 );
				player->SyncFriend_Online( true , player , General::ContactState::ContactState_Offline );*/
				oss << "participate - (" << player->GetPlayerIdx() << "),";
			}

			if ( m_pGameRoom->GetRoomNumber() != player->GetJoinedRoomNumber() )
			{
				m_playerSlots[ slot ] = nullptr;
				PmNet::ChamberLeaveRS _res;
				_res.set_chamber_no( m_pGameRoom->GetRoomNumber() );
				_res.set_member_idx( player->GetPlayerIdx() );
				_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() );
				m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
			}
		}
		QueryManager::InsertErrorLog( oss.str() );
	}

	if ( GetMinStartPlayerCount() == 0 )
		return General::ResultCode::Result_PlayStepMismatch;

	// 2명 이하 시작 못함, 베팅 룰에 따라 처리 조건이 달라짐
	// 일반 겜은 2명, 3벳 룰은 3명부터 가능
	if ( GetMemberCnt() < GetMinStartPlayerCount() )
		return General::ResultCode::Result_PlayerCountInsufficient;

	// 베팅 초기화
	cBettingChecker::Clear();
	m_betting.clear();

	// 사이드 팟 기록 삭제
	cSidePotManager::Clear();

	// 베팅 로그 초기화
	IGame::ClearBetLog();

	// 베팅 순서를 정렬한다.
	// Ante 및 초기 베팅 계산 관계로 순서부터 정렬해야함
	DecideBetSequence();
	if ( ( *m_curBetPlayerIter ) == nullptr )
	{
		DecideDealer();
		return General::ResultCode::Result_NullParameterFault;
	}
	// 누적 Lost 머니 초기화
	ClearStockedMoney();

	// Seed 금액이 부족하면 실패 처리
	if ( false == CheckSeed( m_channel.seed_money() ) )
		return General::ResultCode::Result_SeedBalanceInsufficient;

	// 방 플레이 횟수 누적
	uint32 played_game_count = m_pGameRoom->m_roomInfo.played_rounds();
	m_pGameRoom->m_roomInfo.set_played_rounds( played_game_count + 1 );

	// 시작 시간 찍기
	auto now = std::chrono::system_clock::now();
	auto epoch = now.time_since_epoch();
	timeStamp = std::chrono::duration_cast< std::chrono::seconds >( epoch ).count();

	// GameUID
	m_gameUid = NetLib::cSingleton<AuthCodeGenerator>::GetInstance()->generate( 8 );

	//로그데이터 초기화
	for ( int i = 0; i < m_maxPlayerCnt; ++i )
	{
		p_id_int[ i ] = 0;
		p_id[ i ] = "";
		p_asset[ i ] = 0;
		p_play_cnt[ i ] = 0;
		p_change_coin[ i ] = 0;
		p_discard_coin[ i ] = 0;
		p_fee_coin[ i ] = 0;
		p_fee_info[ i ] = "";
		p_rakeback[ i ] = 0;
		p_coin[ i ] = 0;
		p_result[ i ] = "";

		win_id[ i ] = "";
		win_asset[ i ] = 0;
	}

	int playerCount = -1;
	for ( auto player : m_playerSlots ) {
		playerCount++;

		if ( player == nullptr )
			continue;

		// 배열 경계 검사 추가
		if ( playerCount >= 9 ) {
			// 라이브 서버에서는 로그만 남기고 안전하게 처리
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Holdem player count exceeds maximum limit" );
			break; // 루프를 종료하여 안전하게 처리
		}

		p_asset[ playerCount ] = ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Chip ) ? player->GetChip() : player->GetCoin(); // Changed to GetCoin
	}

#pragma region SeedMoney 실지로 차감

	MinusPlayerSeed( m_channel.seed_money() );

#pragma region SeedMoney 실지로 차감

	// Ante 베팅 처리
	// 전체 플레이어 시드 머니 만큼 차감
	uint64 curPot = m_channel.seed_money() * GetMemberCnt();

	// 일반룰 SB 베팅 금액 처리 Small ( 시드 머니 )
	uint64 smallBet = m_channel.seed_money();
	uint64 beforeBet = smallBet;
	curPot += smallBet;

	// 일반룰 BB 베팅 금액 Big ( 시드머니 * 2 )
	uint64 bigBet = smallBet * 2; // BB 베팅
	beforeBet = bigBet - beforeBet;
	curPot += bigBet;

	// 3 BET Giant ( Big 처리 Pot 의 절반 )
	uint64 giantBet = 0;
	if ( m_pGameRoom->m_roomInfo.bet_policy() == General::BetPolicy::BetPolicy_HoldemThreeBet ||
		m_pGameRoom->m_roomInfo.bet_policy() == General::BetPolicy::BetPolicy_HoldemFourBet ) {
		giantBet = bigBet * 2; // GB 베팅
		beforeBet = giantBet - beforeBet;
		curPot += giantBet;
	}

	// 4 Bet ( Giant Pot 의 Half )
	uint64 royalBet = 0;
	if ( m_pGameRoom->m_roomInfo.bet_policy() == General::BetPolicy::BetPolicy_HoldemFourBet ) {
		royalBet = giantBet * 2; // RB 베팅
		beforeBet = royalBet - giantBet;
		curPot += royalBet;
	}

	PmNet::MatchWagerRS sb;
	PmNet::MatchWagerRS bb;
	PmNet::MatchWagerRS gb;
	PmNet::MatchWagerRS rb;
	uint64 smallPlusGiant = 0;
	//if ( m_pGameRoom->m_roomInfo.betting_rule_type() == General::BetPolicy::BetPolicy_HoldemStandard )
	{
		m_curBetRound = E_BET_ROUND::E_BET_ROUND_1;

		// SB 베팅
		uint64 smallBetPlayerIdx = ( *m_curBetPlayerIter )->GetPlayerIdx();
		std::vector<General::TableAction>* bettings = CreateStepPlayerBettings( *m_curBetPlayerIter , Server::PlayPhase::PlayPhase_FirstBet );
		bettings->push_back( General::TableAction::TableAction_SmallBlind );

		// BB 베팅
		m_curBetPlayerIter = GetNextBetPlayer();
		uint64 bigBetPlayerIdx = ( *m_curBetPlayerIter )->GetPlayerIdx();
		bettings = CreateStepPlayerBettings( *m_curBetPlayerIter , Server::PlayPhase::PlayPhase_FirstBet );
		bettings->push_back( General::TableAction::TableAction_BigBlind );

		/*
		// GB 베팅
		uint64 giantBetPlayerIdx = 0;
		if ( m_pGameRoom->m_roomInfo.bet_policy() == General::BetPolicy::BetPolicy_HoldemThreeBet ||
			m_pGameRoom->m_roomInfo.bet_policy() == General::BetPolicy::BetPolicy_HoldemFourBet ) {
			m_curBetPlayerIter = GetNextBetPlayer();
			if ( m_curBetPlayerIter == m_betSequence.end() ) {
				m_curBetPlayerIter = GetFirstBetPlayer();
				m_curBetRound = E_BET_ROUND::E_BET_ROUND_2;
			}
			giantBetPlayerIdx = ( *m_curBetPlayerIter )->GetPlayerIdx();

			bettings = CreateStepPlayerBettings( *m_curBetPlayerIter , Server::PlayPhase::PlayPhase_FirstBet );
			bettings->push_back( General::TableAction::TableAction_GiantBlind );
		}

		// RB 베팅
		uint64 royalBetPlayerIdx = 0;
		if ( m_pGameRoom->m_roomInfo.bet_policy() == General::BetPolicy::BetPolicy_HoldemFourBet ) {
			m_curBetPlayerIter = GetNextBetPlayer();
			if ( m_curBetPlayerIter == m_betSequence.end() ) {
				m_curBetPlayerIter = GetFirstBetPlayer();
				m_curBetRound = E_BET_ROUND::E_BET_ROUND_2;
			}
			royalBetPlayerIdx = ( *m_curBetPlayerIter )->GetPlayerIdx();

			bettings = CreateStepPlayerBettings( *m_curBetPlayerIter , Server::PlayPhase::PlayPhase_FirstBet );
			bettings->push_back( General::TableAction::TableAction_RoyalBlind );
		}
		*/

		// SB 와 GB 베팅 유저가 같을 수가 있다.
		//if ( smallBetPlayerIdx != giantBetPlayerIdx )
		{
			// Ante 베팅 체크
			for ( auto player : m_betSequence ) {
				if ( player == nullptr )
					continue;

				if ( player->GetPlayerIdx() == smallBetPlayerIdx )
					if ( false == CheckPlayerMoney( player , smallBet ) )
						return General::ResultCode::Result_SeedBalanceInsufficient;

				if ( player->GetPlayerIdx() == bigBetPlayerIdx )
					if ( false == CheckPlayerMoney( player , bigBet ) )
						return General::ResultCode::Result_SeedBalanceInsufficient;

				/*
				if ( player->GetPlayerIdx() == giantBetPlayerIdx )
					if ( false == CheckPlayerMoney( player , giantBet ) )
						return General::ResultCode::Result_SeedBalanceInsufficient;

				if ( player->GetPlayerIdx() == royalBetPlayerIdx )
					if ( false == CheckPlayerMoney( player , royalBet ) )
						return General::ResultCode::Result_SeedBalanceInsufficient;
				*/

				if ( player->GetPlayerIdx() != smallBetPlayerIdx &&
					player->GetPlayerIdx() != bigBetPlayerIdx 
					/* &&
					player->GetPlayerIdx() != giantBetPlayerIdx &&
					player->GetPlayerIdx() != royalBetPlayerIdx */ )   
				{
					if ( false == CheckPlayerMoney( player , m_channel.seed_money() ) )
						return General::ResultCode::Result_SeedBalanceInsufficient;
				}
			}
		}
		// 동일한 베팅을 할 수 없도록 기획이 변경됨
		//else
		//{
		//	smallPlusGiant = smallBet + giantBet;

		//	// Ante 베팅 체크
		//	for ( auto player : m_betSequence ) {
		//		if ( player == nullptr )
		//			continue;

		//		if ( player->GetPlayerIdx() == bigBetPlayerIdx )
		//			if ( false == CheckPlayerMoney( player , bigBet ) )
		//				return General::ResultCode::Result_SeedBalanceInsufficient;

		//		if ( player->GetPlayerIdx() == giantBetPlayerIdx )
		//			if ( false == CheckPlayerMoney( player , smallPlusGiant ) )
		//				return General::ResultCode::Result_SeedBalanceInsufficient;

		//		if ( player->GetPlayerIdx() != bigBetPlayerIdx && player->GetPlayerIdx() != giantBetPlayerIdx )
		//			if ( false == CheckPlayerMoney( player , m_channel.seed_money() ) )
		//				return General::ResultCode::Result_SeedBalanceInsufficient;
		//	}
		//}


#pragma region Ante 실지로 차감
		m_firstidx = 0;
		// sb, bb, gb, rb 순서로 차감이 이루어 져야 한다.
		roundbet = 0;
		if ( smallBet > 0 ) {
			m_blindBet = MinusSbBbGbRb( sb , smallBetPlayerIdx , smallBet , General::TableAction::TableAction_SmallBlind );
			m_betting[ smallBetPlayerIdx ] += smallBet;
			m_firstidx = smallBetPlayerIdx;
			maxbet = m_betting[ smallBetPlayerIdx ];
		}
		if ( bigBet > 0 ) {
			m_blindBet = MinusSbBbGbRb( bb , bigBetPlayerIdx , bigBet , General::TableAction::TableAction_BigBlind );
			m_betting[ bigBetPlayerIdx ] += bigBet;
			m_firstidx = bigBetPlayerIdx;
			maxbet = m_betting[ bigBetPlayerIdx ];
		}
		/*
		if ( giantBet > 0 ) {
			m_blindBet = MinusSbBbGbRb( gb , giantBetPlayerIdx , giantBet , General::TableAction::TableAction_GiantBlind );
			m_betting[ giantBetPlayerIdx ] += giantBet;
			m_firstidx = giantBetPlayerIdx;
			maxbet = m_betting[ giantBetPlayerIdx ];
		}
		if ( royalBet > 0 ) {
			m_blindBet = MinusSbBbGbRb( rb , royalBetPlayerIdx , royalBet , General::TableAction::TableAction_RoyalBlind );
			m_betting[ royalBetPlayerIdx ] += royalBet;
			m_firstidx = royalBetPlayerIdx;
			maxbet = m_betting[ royalBetPlayerIdx ];
		}
		*/
		//if ( smallBetPlayerIdx != giantBetPlayerIdx ) {
		//	MinusSbBbGb( sb , smallBetPlayerIdx , smallBet , General::TableAction::TableAction_SmallBlind );
		//	MinusSbBbGb( bb , bigBetPlayerIdx , bigBet , General::TableAction::TableAction_BigBlind );
		//	MinusSbBbGb( gb , giantBetPlayerIdx , giantBet , General::TableAction::TableAction_GiantBlind );
		//}
		//else
		//{
		//	//MinusSbBbGb( sb , smallBetPlayerIdx , smallBet , General::TableAction::TableAction_SmallBlind );
		//	MinusSbBbGb( bb , bigBetPlayerIdx , bigBet , General::TableAction::TableAction_BigBlind );
		//	MinusSbBbGb( gb , giantBetPlayerIdx , smallPlusGiant , General::TableAction::TableAction_GiantBlind );
		//}

		//for ( auto player : m_betSequence ) {
		//	if ( player != nullptr ) {
		//		// sb, bb 가 아닌 유저의 머니 차감
		//		if ( player->GetPlayerIdx() != smallBetPlayerIdx && player->GetPlayerIdx() != bigBetPlayerIdx && player->GetPlayerIdx() != giantBetPlayerIdx ) {
		//			MinusPlayerMoney( player , m_channel.seed_money() );
		//		}
		//	}
		//}

#pragma endregion Ante 실지로 차감

		// 방정보에 seed money set
		m_pGameRoom->m_roomInfo.set_seed_amount( m_channel.seed_money() );

		// pot, beforeBet 초기화
		m_pGameRoom->m_roomInfo.set_pot_amount( curPot );
		m_pGameRoom->m_roomInfo.set_previous_wager( beforeBet );

	}

	// General::RoomSnapshot 상태 변경
	m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_InPlay );
	std::string errorInfo = std::format( "Holdem START GAME Room : {}" ,m_pGameRoom->GetRoomNumber());
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );
	std::string errorInfo2 = std::format( "Room : {} \t playerIdx :" ,m_pGameRoom->GetRoomNumber());
	bool first = true;
	for (auto t_slot : m_playerSlots )
	{
		if ( t_slot == nullptr )
			continue;
		if( first )
			errorInfo2 += std::format( "{}",t_slot->GetPlayerIdx() );
		else
			errorInfo2 += std::format( ",{}",t_slot->GetPlayerIdx() );

		first = false;
	}
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo2.c_str() );

	std::string errorInfo3;
	m_pGameRoom->MakeUserLog( errorInfo3 );
	errorInfo3 += "\n";
	errorInfo3 += std::format( "room :{}  m_participateQueue : " , m_pGameRoom->GetRoomNumber() );
	for ( auto t : m_participateQueue )
	{
		if ( t == nullptr )
			continue;
		int t_playeridx = t->GetPlayerIdx();
		errorInfo3 += std::format( "{} ," , t_playeridx );
	}
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo3.c_str() );

	// TODO pooling 으로 변경하도록 한다.
	// 최대 인원 만큼 생성해 두면 될것 같다.
	// 현재는
	// side money pot 초기화
	for ( auto sidePot : m_sidePots ) {
		if ( sidePot != nullptr )
			delete sidePot;
	}

	// 카드 셔플
	Shuffle();

#ifdef _DEBUG
	// QA 용 덱 초기화
	m_qa_community_deck.clear();			// 딜러 덱
	m_qa_community_deck = m_qa_community_cards;

	// 플레이어들에게 셋팅된 카드를 덱으로 사용하도록 한다.
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr ) continue;

		const uint64& playerIdx = player->GetPlayerIdx();

		auto qa_iter = NetLib::cSingleton<cQADeck>::GetInstance()->m_holdem_qa_dealer_decks.find( playerIdx );
		if ( qa_iter != NetLib::cSingleton<cQADeck>::GetInstance()->m_holdem_qa_dealer_decks.end() ) {
			const auto& cards = qa_iter->second;
			player->m_holdem_qa_deck = cards;
		}
	}

#endif

	game_record_string = "";

	// 핸드 카드 분배
	HandCardDistribute( m_channel.seed_money() , sb , bb , gb , rb );

	playerCount = -1;
	for (auto player : m_playerSlots)
	{
		++playerCount;
		if ( player == nullptr )
	        continue;
		p_id_int[ playerCount ] = player->GetPlayerIdx();
		p_id[ playerCount ] = player->GetPlatformGuid();
		p_play_cnt[ playerCount ] = player->GetRoomPlayingCount();
	}

	cClientSession* pClientSession_boss = GetPlayerSession(GetBossPlayerIdx());

	if ( nullptr != pClientSession_boss && nullptr != ( *m_masterIter ) )
	{
		// 일반룰:N, 스트래들:S, 더블 스트래들:DS   (ex. MAX=6,OPTS=DS)
		std::ostringstream ossGameOpt;
		ossGameOpt << "MAX=" << std::to_string( m_pGameRoom->GetMaxRoomPlayerCnt() ) << ",OPTS=";

		if ( General::BetPolicy::BetPolicy_HoldemStandard == m_pGameRoom->m_roomInfo.bet_policy() )
			ossGameOpt << "N";
		else if ( General::BetPolicy::BetPolicy_HoldemThreeBet == m_pGameRoom->m_roomInfo.bet_policy() )
			ossGameOpt << "S";
		else if ( General::BetPolicy::BetPolicy_HoldemFourBet == m_pGameRoom->m_roomInfo.bet_policy() )
			ossGameOpt << "DS";

		int code = 20401;

		// InsertGameHoldemLog 로그 추가
		std::ostringstream ossPlayers;
		for ( int i = 0; i < m_maxPlayerCnt; ++i )
		{
			if ( p_id[ i ].empty() )
				continue;

			ossPlayers << p_id[ i ] << ",";
		}

		auto log_result_holdem_start = QueryManager::InsertGameHoldemLog(
			code ,                           // 코드
			m_gameUid ,                             // 게임 ID
			m_channel.id() , // 채널
			m_pGameRoom->m_roomInfo.played_rounds() + 1 , // 게임 횟수
			"" ,                             // 로그 버전
			ossGameOpt.str() ,                       // 게임 옵션
			pClientSession_boss->GetPlatformGuid() , // 보스
			( *m_masterIter )->GetPlatformGuid() , // 호스트
			p_id[ 0 ] , p_asset[ 0 ] , p_play_cnt[ 0 ] , p_change_coin[ 0 ] , p_discard_coin[ 0 ] , p_fee_coin[ 0 ] , p_fee_info[ 0 ] , p_rakeback[ 0 ] , p_coin[ 0 ] , p_result[ 0 ] ,
			p_id[ 1 ] , p_asset[ 1 ] , p_play_cnt[ 1 ] , p_change_coin[ 1 ] , p_discard_coin[ 1 ] , p_fee_coin[ 1 ] , p_fee_info[ 1 ] , p_rakeback[ 1 ] , p_coin[ 1 ] , p_result[ 1 ] ,
			p_id[ 2 ] , p_asset[ 2 ] , p_play_cnt[ 2 ] , p_change_coin[ 2 ] , p_discard_coin[ 2 ] , p_fee_coin[ 2 ] , p_fee_info[ 2 ] , p_rakeback[ 2 ] , p_coin[ 2 ] , p_result[ 2 ] ,
			p_id[ 3 ] , p_asset[ 3 ] , p_play_cnt[ 3 ] , p_change_coin[ 3 ] , p_discard_coin[ 3 ] , p_fee_coin[ 3 ] , p_fee_info[ 3 ] , p_rakeback[ 3 ] , p_coin[ 3 ] , p_result[ 3 ] ,
			p_id[ 4 ] , p_asset[ 4 ] , p_play_cnt[ 4 ] , p_change_coin[ 4 ] , p_discard_coin[ 4 ] , p_fee_coin[ 4 ] , p_fee_info[ 4 ] , p_rakeback[ 4 ] , p_coin[ 4 ] , p_result[ 4 ] ,
			p_id[ 5 ] , p_asset[ 5 ] , p_play_cnt[ 5 ] , p_change_coin[ 5 ] , p_discard_coin[ 5 ] , p_fee_coin[ 5 ] , p_fee_info[ 5 ] , p_rakeback[ 5 ] , p_coin[ 5 ] , p_result[ 5 ] ,
			p_id[ 6 ] , p_asset[ 6 ] , p_play_cnt[ 6 ] , p_change_coin[ 6 ] , p_discard_coin[ 6 ] , p_fee_coin[ 6 ] , p_fee_info[ 6 ] , p_rakeback[ 6 ] , p_coin[ 6 ] , p_result[ 6 ] ,
			p_id[ 7 ] , p_asset[ 7 ] , p_play_cnt[ 7 ] , p_change_coin[ 7 ] , p_discard_coin[ 7 ] , p_fee_coin[ 7 ] , p_fee_info[ 7 ] , p_rakeback[ 7 ] , p_coin[ 7 ] , p_result[ 7 ] ,
			p_id[ 8 ] , p_asset[ 8 ] , p_play_cnt[ 8 ] , p_change_coin[ 8 ] , p_discard_coin[ 8 ] , p_fee_coin[ 8 ] , p_fee_info[ 8 ] , p_rakeback[ 8 ] , p_coin[ 8 ] , p_result[ 8 ] ,
			// 추가 데이터
			ossPlayers.str() ,
			0 , // play_time
			"roomid_" + std::to_string( m_pGameRoom->GetRoomNumber() ) , // room_id
			// win data
			win_id[ 0 ] , win_asset[ 0 ] ,
			win_id[ 1 ] , win_asset[ 1 ] ,
			win_id[ 2 ] , win_asset[ 2 ] ,
			win_id[ 3 ] , win_asset[ 3 ] ,
			win_id[ 4 ] , win_asset[ 4 ] ,
			win_id[ 5 ] , win_asset[ 5 ] ,
			win_id[ 6 ] , win_asset[ 6 ] ,
			win_id[ 7 ] , win_asset[ 7 ] ,
			win_id[ 8 ] , win_asset[ 8 ] );

		log_result_holdem_start.wait();
	}

	return General::ResultCode::Result_Success;
}

uint64 cHoldem::MinusSbBbGbRb( PmNet::MatchWagerRS& response , const uint64 playerIdx , const uint64 betMoney , const General::TableAction bet )
{
	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		if ( player->GetPlayerIdx() == playerIdx ) {
			response.set_wager_member_idx( playerIdx );
			response.set_member_wager( bet );
			response.set_fund_kind( m_pGameRoom->m_roomInfo.asset_kind() );
			response.set_fund_before( GetPlayerMoney( player ) );
			MinusPlayerMoney( player , betMoney );
			response.set_fund_after( GetPlayerMoney( player ) );

			// 마지막에 베팅한 사람의 총 베팅 금액 설정
			m_stepPlayerBet = player->GetLostMoney();
			m_lastPlayerBetTotal = player->GetLostMoney();

			return m_lastPlayerBetTotal;
		}
	}
	return 0;
}

void cHoldem::Playing()
{

}

// 게임방의 스텝 처리
void cHoldem::Process()
{
	// 시간 종료되었으면 다음스텝으로 넘기고 종료
	if ( isTimeOver() ) {
		SetNextPlay();
		return;
	}

	m_pGameRoom->CheckWatcherCount();
	CheckParticipateQueue();
	if ( m_gameStep != Server::PlayPhase::PlayPhase_Waiting && m_gameStep != Server::PlayPhase::PlayPhase_StartReady )
	{
		CheckDieUser();
		MakeParticipateOnplay();
	}
	else
	{
		m_pGameRoom->ClearOutUser();
	}

	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_Waiting:
	{
		Waiting();
	}
	break;
	case Server::PlayPhase::PlayPhase_StartReady:
	{
		// Start_Wait 에서는 방장이 시작하거나
		// 풀방인 상태에서 2초가 흐른 경우에만 방이 자동으로 시작한다.
		if ( isAutoStartTimeOver() ) {
			if( GetAutoStartCnt() <= GetMemberCnt() )
				StartGame( ( *m_dealerIter ) );
		}
		else
			m_autoStartTick -= GetTickCount64();
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemPreFlop:
	{
	}
	break;
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
	{
		BetProcess();
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
	{
		//CardChangeProcess();
	}
	break;
	case Server::PlayPhase::PlayPhase_Judgement:
	{
		// 결과창은 할게 없어 보임, 대기 시간 종료시에 끝
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemShowdown:
	{
		// ShowDown 시간에는 할게 없음
	}
	break;
	}
}

void cHoldem::CheckParticipateQueue()
{
	if ( participatelock )
		return;
	participatelock = true;
	const int participatersCount = m_participateQueue.size();
	for ( auto iter = m_participateQueue.begin(); iter != m_participateQueue.end(); ) {
		auto participateRequester = *iter;
		if ( participateRequester == nullptr ) {
			iter = m_participateQueue.erase( iter );
			std::string log = std::format(
				"CheckParticipateQueue NULL  Room: {}" ,
				  m_pGameRoom->GetRoomNumber()
			);

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
			continue;
		}
		if ( participateRequester->GetJoinedRoomNumber() != m_pGameRoom->GetRoomNumber() )
		{
			iter = m_participateQueue.erase( iter );
			std::string log = std::format(
				"CheckParticipateQueue ROOMNUM Room: {}" ,
				  m_pGameRoom->GetRoomNumber()
			);

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
			continue;
		}
		if ( false == m_pGameRoom->isWatcher( participateRequester->GetPlayerIdx() ) ) {
			iter = m_participateQueue.erase( iter );
			std::string log = std::format(
				"CheckParticipateQueue WATCHER  Room: {}" ,
				  m_pGameRoom->GetRoomNumber()
			);

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
			continue;
		}
		iter++;
	}
	participatelock = false;
}

bool cHoldem::isTimeOver()
{
	uint64 remain_ms = m_expireTick - ::GetTickCount64();

	// Common::LowBaduki m_gameStep;
	// 테스트, 남은 시간을 클라이언트 들에게 뿌려줌
	PmNet::PhaseRemainRS _res;
	_res.set_phase_remain_ms( remain_ms );
	//m_pGameRoom->RoomBroadCast(Common::GMsg_PlayRemainSec, _res); // 남아 있는 시간 계속 안보내도록 일단 주석처리

	return ::GetTickCount64() >= m_expireTick;
}

// Next Step 으로 진행시킵니다.
void cHoldem::SetNextPlay()
{
	Server::PlayPhase before = m_gameStep;
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_Waiting: // 입장대기, 대기 인원이 두명이 이상 부터는 방장이 시작이 활성화 된다.
	{
		// 풀방이 아니면 시작상태에서 대기
		if ( GetMemberCnt() < GetAutoStartCnt() )
			MakeRoomStatusStartWait();
		else
		{
			// 자동시작 시간만 설정하고 StartWait 으로 이동
			m_gameStep = Server::PlayPhase::PlayPhase_StartReady;
			m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );
			m_autoStartTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;	// TODO 대기시간 LowBadukiWait 1.5초로 설정함 데이터 만들어 달라고 요청은 했음

			// General::RoomSnapshot 상태 변경
			m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_Waiting );
		}
	}
	break;
	case Server::PlayPhase::PlayPhase_StartReady: // 시작 대기
	{
		//m_gameStep = Server::PlayPhase::PlayPhase_InitialDeal;
		//m_expireTick = ::GetTickCount64() + Server::GameDef::LowBaduki_Base_Distribution_Animation; // 최초 카드 분배 연출 애니메이션 대기 시간 ( 클라에서 연출 하는 동안 다음 진행이 안되도록 대기하는 시간 )

		// 자동 시작합니다.
		//StartGame( ( *m_dealerIter ) );
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemPreFlop:
	{
		//m_curBetRound = E_BET_ROUND::E_BET_ROUND_1; // 베팅 라운드 초기화
		// sb, bb, gb 처리후에 베팅한 금액이 유지가 되어야 한다.
		//m_lastPlayerBetTotal = 0;
		//m_stepPlayerBet = 0;
		//m_pGameRoom->m_roomInfo.set_beforebet( 0 );
		ClearPlayerCache(); // General::ParticipantProfile 의 betting, betting_money 초기화
		m_raiseCount.clear();
		m_gameStep = Server::PlayPhase::PlayPhase_FirstBet;
		m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );
		DecideHoldemBet1();
		SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiBetWait );
	}
	break;
	case Server::PlayPhase::PlayPhase_FirstBet:
	{
		m_gameStep = Server::PlayPhase::PlayPhase_HoldemFlop;
		m_raiseCount.clear();
		m_stepPlayerBet = 0;
		DecideFirstBet();
		int cardCount = GetCommunityCard();
		uint64 waitMs = cardCount * 200;
		m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait; // 1.5 초 정도??
		//SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiBetWait );
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	{
		m_curBetRound = E_BET_ROUND::E_BET_ROUND_1; // 베팅 라운드 초기화
		m_raiseCount.clear();
		m_pGameRoom->m_roomInfo.set_previous_wager( 0 );
		ClearPlayerCache(); // General::ParticipantProfile 의 betting, betting_money 초기화
		m_gameStep = Server::PlayPhase::PlayPhase_SecondBet;
		m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );
		DecideFirstBet();
		SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiBetWait );
	}
	break;
	case Server::PlayPhase::PlayPhase_SecondBet:
	{
		m_gameStep = Server::PlayPhase::PlayPhase_HoldemTurn;
		m_raiseCount.clear();
		m_stepPlayerBet = 0;
		DecideFirstBet();
		int cardCount = GetCommunityCard();
		uint64 waitMs = cardCount * 200;
		m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait; // 1.5 초 정도??
		//SetTurnExpireAndSendPlayerTurn( waitMs );
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	{
		m_curBetRound = E_BET_ROUND::E_BET_ROUND_1; // 베팅 라운드 초기화
		m_raiseCount.clear();
		m_pGameRoom->m_roomInfo.set_previous_wager( 0 );
		ClearPlayerCache(); // General::ParticipantProfile 의 betting, betting_money 초기화
		m_gameStep = Server::PlayPhase::PlayPhase_ThirdBet;
		m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );
		DecideFirstBet();
		SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiBetWait );
	}
	break;
	case Server::PlayPhase::PlayPhase_ThirdBet:
	{
		m_gameStep = Server::PlayPhase::PlayPhase_HoldemRiver;
		m_stepPlayerBet = 0;
		DecideFirstBet();
		int cardCount = GetCommunityCard();
		uint64 waitMs = cardCount * 200;
		m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait; // 1.5 초 정도??
		//SetTurnExpireAndSendPlayerTurn( waitMs );
	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemRiver:
	{
		m_curBetRound = E_BET_ROUND::E_BET_ROUND_1; // 베팅 라운드 초기화
		m_raiseCount.clear();
		m_pGameRoom->m_roomInfo.set_previous_wager( 0 );
		ClearPlayerCache(); // General::ParticipantProfile 의 betting, betting_money 초기화
		m_gameStep = Server::PlayPhase::PlayPhase_FinalBet;
		m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );
		DecideFirstBet();
		SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiBetWait );
	}
	break;
	case Server::PlayPhase::PlayPhase_FinalBet:
	{
		m_gameStep = Server::PlayPhase::PlayPhase_Judgement;
		m_stepPlayerBet = 0;

		// 결과창 연출시간 인원수에 따라 달라짐
		int preFlopWait = CalculatePreFlopAnimation();

		// TODO 홀덤 결과창 10초로 설정해둠, 차후에 변경 필요
		preFlopWait = 10000;
		m_expireTick = ::GetTickCount64() + preFlopWait;

		// 결과 처리 호출
		if ( m_communityCards.size() )
			CalcResult();
		else {
			// 커뮤니티 카드가 없는 경우
			// 플레이어들이 전체 다 Die
			CalcResultNoCommunityCard();
		}
	}
	break;
	case Server::PlayPhase::PlayPhase_Judgement:
	{
		// 결과창 보기 종료시에 2인이상이 방에 있으면 바로 시작 버튼을 누를수 있는 상태로 만들어 줘야 한다.
		//if ( m_pGameRoom->GetJoinedCnt() >= 2 ) {
		participatelock = true;
		if ( GetMemberCnt() >= GetMinStartPlayerCount() ) {

			m_gameStep = Server::PlayPhase::PlayPhase_Waiting;
			m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;
			m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_Waiting );
		}
		else
		{
			// Wait 상태로 만듬
			MakeRoomStatusWait();
		}
		// Result 종료 이기 때문에 방 종료 처리
		EndGame();

		// 결과 처리에 따른 유저 데이터 저장
		m_communityCards.clear();

		// 래빗헌팅 / 패 공개 상태도 핸드 단위로 초기화
		m_rabbitHuntCards.clear();
		m_rabbitHuntDone = false;
		m_shownHands.clear();

		SaveUserData(true);

		m_pGameRoom->ClearOutUser();

		//점검애들 내보내기
		m_pGameRoom->CheckMaintance();

		ProcessKickOut();

		// 나가기 예약된 유저 내보내기
		m_pGameRoom->KickRoomOutReservedOrAllinOrLostLimitPlayer();

		// 시드 머니 부족 유저 내보내기
		m_pGameRoom->RoomOutNotEnoughSeedPlayer();

		KickMoneyHoldingLimitPlayers();

		OnWatcherReservation();



		// 예약자 플레이 참가 처리
		OnSlotReservation();

		// 참여 큐에 있던 플레이어 참가 처리
		MakeParticipate();



		// 딜러 변경
		DecideDealer();

		DecideMaster();

		// 플레이어들의 룸 플레잉 카운트 증가 시킴
		IncreaseRoomPlayingCount();

		// 베팅 로그 파일 출력
		WriteBetLog();
		// 이전 데이터 남는 문제 수정 처리
		{
			// Wait 상태에서 진입한 플레이어들이 이전 플레이 데이터를 볼 수 있어 삭제한다.
			ClearStockedMoney();

			// 베팅 초기화
			cBettingChecker::Clear();

			// 클라이언트의 Player 베팅 캐쉬 삭제
			ClearPlayerCache();
		}
		participatelock = false;

	}
	break;
	case Server::PlayPhase::PlayPhase_HoldemShowdown:
	{
		m_gameStep = Server::PlayPhase::PlayPhase_Judgement;
		m_stepPlayerBet = 0;

		// 결과창 연출시간 인원수에 따라 달라짐
		int preFlopWait = CalculatePreFlopAnimation();

		// TODO 홀덤 결과창 10초로 설정해둠, 차후에 변경 필요
		preFlopWait = 10000;
		m_expireTick = ::GetTickCount64() + preFlopWait;

		// 결과 처리 호출
		CalcResult();
	}
	break;
	}

	SendStatusChange( before , "cHoldem::SetNextPlay" );
}

// 이방에서 3번 이상 플레이한 유저의 경우 투표 발의권이 생긴다.
// 권한이 생긴것을 본인에게 통보해준다.
void cHoldem::IncreaseRoomPlayingCount()
{
	for ( auto player : m_playerSlots )
	{
		if ( player != nullptr )
		{
			player->IncreaseRoomPlayingCount();

			// 3회가 되었을때 한번 알려 준다.
			/*if ( player->GetRoomPlayingCount() == 3 ) {
				PmNet::ChamberPollEligibleRS _res;
				std::string errorString;
				m_pGameRoom->SendRequest( player , Common::GMsg_RoomRightToVote , _res , General::ResultCode::Result_Success , errorString );
			}*/
		}
	}
}

void cHoldem::SendMasterChange( int64 playerindx , bool b_out )
{
	// m_masterIter가 m_playerSlots.end()인 경우 함수 종료
	if ( m_masterIter == m_playerSlots.end() )
		return;

	auto player = ( *m_masterIter );
	if ( player != nullptr ) {
		uint64 masterPlayerIdx = player->GetPlayerIdx();

		int masterSearchSlot = std::distance( m_playerSlots.begin() , m_masterIter ) + 1;

		// 보스 변경 전송
		PmNet::MatchCaptainSwapRS _res;
		_res.set_captain_seat( masterSearchSlot );
		_res.set_captain_idx( masterPlayerIdx );
		if( b_out )
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_TableOwnerNotice , _res , playerindx );
		else
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_TableOwnerNotice , _res );
		m_pGameRoom->m_roomInfo.set_host_seat( masterSearchSlot );

		m_kick_out_count = 0; // 새로 추가
	}
}

void cHoldem::SendMasterChange()
{
	// m_masterIter가 m_playerSlots.end()인 경우 함수 종료
	if ( m_masterIter == m_playerSlots.end() )
		return;

	auto player = ( *m_masterIter );
	if ( player != nullptr ) {
		uint64 masterPlayerIdx = player->GetPlayerIdx();

		int masterSearchSlot = std::distance( m_playerSlots.begin() , m_masterIter ) + 1;

		// 보스 변경 전송
		PmNet::MatchCaptainSwapRS _res;
		_res.set_captain_seat( masterSearchSlot );
		_res.set_captain_idx( masterPlayerIdx );
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_TableOwnerNotice , _res );

		m_pGameRoom->m_roomInfo.set_host_seat( masterSearchSlot );
	}
}

// 핸드 카드 재 분배 시간을 계산한다.
// ms 으로 계산
// TODO 분배 시간 데이터화
int cHoldem::CalculatePreFlopAnimation()
{
	static int baseWait = 1400;		// 핸드 카드 재 분배 기본 대기 시간
	static int waitPerPlayer = 150;	// 핸드 카드 플레이어별 대기 시간
	static int blindWait = 500;		// 핸드 카드 GB, RB 등의 연출이 끝나고 연출 종료까지 대기 시간
	int preFlopWait = baseWait + blindWait + ( GetPlayingPlayerCount() * waitPerPlayer );	// 핸드 카드 재분배 대기 시간 공식
	return preFlopWait;
}

// 홀덤 연출 시간
// TODO 분배 시간 데이터화
int cHoldem::CalculateShowDownAnimation( int& leftCommunityCards )
{
	int communityWaitMs = 0;
	switch ( leftCommunityCards )
	{
	case 5:
		communityWaitMs = m_show_down_community_delay_ms_per * 3;
		break;
	case 2:
		communityWaitMs = m_show_down_community_delay_ms_per * 2;
	case 1:
		communityWaitMs = m_show_down_community_delay_ms_per * 1;
	default:
		communityWaitMs = m_show_down_community_delay_ms_per;
	}

	static int baseWait = 2000; // showdown image 이펙트
	int showDownWait = baseWait + ( GetPlayingPlayerCount() * m_show_down_delay_ms_per_player ) + communityWaitMs;
	return showDownWait;
}

// 핸드 카드 분배, PreFlop
// 2장씩 나눈다.
void cHoldem::HandCardDistribute( const uint64& seedMoney , PmNet::MatchWagerRS& sb , PmNet::MatchWagerRS& bb , PmNet::MatchWagerRS& gb , PmNet::MatchWagerRS& rb )
{
	PmNet::SeedDealoutRS _res;
	PmNet::SeedDealoutRS _res_watcher;
	PmNet::SeedDealoutRS _res_cheat;
	std::string errorMessage;

	if ( m_stockDeck.size() > 0 )
		m_stockDeck.clear();

	_res.set_seed_funds( seedMoney );
	_res_cheat.set_seed_funds( seedMoney );
	_res_watcher.set_seed_funds( seedMoney );

	// Blind Bet 전송
	PmNet::MatchWagerRS* pPlayBetRes = nullptr;
	if ( sb.wager_member_idx() != 0 ) {
		pPlayBetRes = _res.add_blind_wagers();
		pPlayBetRes->CopyFrom( sb );

		_res_watcher.add_blind_wagers()->CopyFrom( sb );
	}
	if ( bb.wager_member_idx() != 0 ) {
		pPlayBetRes = _res.add_blind_wagers();
		pPlayBetRes->CopyFrom( bb );

		_res_watcher.add_blind_wagers()->CopyFrom( bb );
	}
	if ( gb.wager_member_idx() != 0 ) {
		pPlayBetRes = _res.add_blind_wagers();
		pPlayBetRes->CopyFrom( gb );

		_res_watcher.add_blind_wagers()->CopyFrom( gb );
	}
	if ( rb.wager_member_idx() != 0 ) {
		pPlayBetRes = _res.add_blind_wagers();
		pPlayBetRes->CopyFrom( rb );

		_res_watcher.add_blind_wagers()->CopyFrom( rb );
	}



	// 플레이어들에게 카드를 2장씩 나눠준다.
	for ( auto player : m_betSequence ) {

		if ( player == nullptr )
			continue;

		player->m_cards.Clear();

		General::CardSet cards;
		General::CardSet watcher_cards;

		for ( int n = 0; n < 2; ++n ) {
			std::vector<General::PlayingCard>::iterator iterBegin = m_deck.begin();
			General::PlayingCard newCard = *iterBegin;

#ifdef _DEBUG
			// QA 덱 셋팅된 유저가 딜러가 되었을때만
			auto qa_iter = player->m_holdem_qa_deck.begin();
			if ( qa_iter != player->m_holdem_qa_deck.end() )
			{
				newCard = *qa_iter;
				player->m_holdem_qa_deck.erase( qa_iter );
			}
#endif
			player->m_cards.Add( newCard );

			// 전송용 데이터
			cards.add_playing_cards()->CopyFrom( newCard );
			watcher_cards.add_playing_cards()->CopyFrom( General::PlayingCard::default_instance() );

			m_deck.erase( iterBegin );
		}
		_res.mutable_member_cards()->insert({ player->GetPlayerIdx() , cards } );
		_res_watcher.mutable_member_cards()->insert({ player->GetPlayerIdx() , watcher_cards } );
	}
	// 관전자들에게 플레이어들의 카드 정보 전송
	m_pGameRoom->WatcherBroadCast( General::Packet_InitialDeal , _res_watcher );
	m_pGameRoom->RoomBroadCastCUser( General::Packet_InitialDeal , _res );
	//보내는 리스폰 새로생성
	for ( auto splayer : m_betSequence ) {
		if ( splayer == nullptr )
			continue;
		//받을 유저인덱스
		uint64 _playeridx = splayer->GetPlayerIdx();
		if ( m_pGameRoom->isCheat( _playeridx ) )
			continue;
		//보낼 레스 복사
		PmNet::SeedDealoutRS res = _res;
		//디폴트카드생성
		General::CardSet cards;
		cards.add_playing_cards()->CopyFrom( General::PlayingCard::default_instance() );
		cards.add_playing_cards()->CopyFrom( General::PlayingCard::default_instance() );

		//디폴트카드넣어주기
		for ( auto player : m_betSequence ) {
			if ( player == nullptr )
				continue;
			if (  player->GetPlayerIdx() == _playeridx )
				continue;
			auto map = res.mutable_member_cards();
			(*map)[ player->GetPlayerIdx() ] = cards;
			//res.mutable_member_cards()->erase( player->GetPlayerIdx() );
			//res.mutable_member_cards()->insert( { player->GetPlayerIdx() , cards } );
		}

		m_pGameRoom->SendRequest( splayer , General::Packet_InitialDeal , res , General::ResultCode::Result_Success , errorMessage );
	}
	{
		std::vector<General::PlayingCard> t_communityCards;
		PmNet::SharedTileRS _res;

		for ( int n = 0; n < 5; ++n ) {
			std::vector<General::PlayingCard>::iterator iter = m_deck.begin();
			General::PlayingCard& newCard = *(iter + n);
			t_communityCards.push_back( newCard ); // 플레이어 카드 추가

			auto new_community_card = _res.add_fresh_shared_cards();
			new_community_card->CopyFrom( newCard );

		}

		for ( const auto& community_card : t_communityCards ) {
			auto before_community_card = _res.add_shared_cards();
			before_community_card->CopyFrom( community_card );
		}
		_res.set_intrusion_flag( true );
		m_pGameRoom->BroadCastToCheatPlayer( General::Packet_ConsoleBoardPreset , _res );
	}
	int preFlopWait = CalculatePreFlopAnimation();
	m_expireTick = ::GetTickCount64() + preFlopWait;

	m_gameStep = Server::PlayPhase::PlayPhase_HoldemPreFlop;
	std::string log = std::format( "Room Status: {}, Room Number: {}" , "Server::PlayPhase::PlayPhase_HoldemPreFlop" , m_pGameRoom->GetRoomNumber() );
	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
}

// Server::PlayPhase::PlayPhase_Waiting 상태도 플레이어의 In, Out 이 발생 할 수 있어 방상태는 General::RoomState::RoomState_Waiting 이다.
void cHoldem::MakeRoomStatusWait()
{
	// 1일 동안 대기 탑시다.
	m_gameStep = Server::PlayPhase::PlayPhase_Waiting;
	m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );
	m_turnExpireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );			// 덩 달아서 초기화 해둡니다.
	m_turnExpireTickMinimum = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );	// 덩 달아서 초기화 해둡니다.
	m_autoStartTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );			// AutoStart 불가능 상태로 재 설정

	// General::RoomSnapshot 상태 변경
	m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_Waiting );

	m_bCheatCardChange = false;
}

// Server::PlayPhase::PlayPhase_StartReady 상태도 플레이어의 In, Out 이 발생 할 수 있어 방상태는 General::RoomState::RoomState_Waiting 이다.
void cHoldem::MakeRoomStatusStartWait()
{
	m_gameStep = Server::PlayPhase::PlayPhase_StartReady;

	m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) ); // 대기 시간 무한

	// General::RoomSnapshot 상태 변경
	m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_Waiting );
}

/// <summary>
/// Start_Wait 상태가 2초후에 끝나도록 자동으로 설정한다.
/// Start_Wait 과 상관 없이 m_autoStartTick 이 끝나면 방장이 시작을 누른것과 동일한 상태가 된다.
/// </summary>
void cHoldem::MakeRoomAutoStart( const int& memberCount )
{
	if ( IsAutoStartable( memberCount ) ) {

		m_gameStep = Server::PlayPhase::PlayPhase_StartReady;
		m_expireTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) );					// 대기 시간 무한
		m_autoStartTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;	// TODO 대기시간 LowBadukiWait 1.5초로 설정함 데이터 만들어 달라고 요청은 했음

		// General::RoomSnapshot 상태 변경
		m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_Waiting );
	}
}

void cHoldem::MakeRoomStatusResult()
{
	m_expireTick = ::GetTickCount64(); // Bet_4 상태에서 바로 종료 시키기 위함
	m_gameStep = Server::PlayPhase::PlayPhase_FinalBet;
}

// 딜러 결정
// 결과 처리 후에 호출이 되어야 한다.
// Boss 정보 변경
// Boss 슬롯 로직은 1 ~ 5 로 처리한다.
// 보스가 방에서 나가는 경우에는 forceChange 가 true
void cHoldem::DecideDealer( bool forceChange )
{
	// 방생성시에 최초는 1번으로 자동 결정
	if ( m_pGameRoom->m_roomInfo.leader_seat() == 0 ) {
		m_dealerIter = m_playerSlots.begin();
		int index = std::distance( m_playerSlots.begin() , m_dealerIter );
		m_pGameRoom->m_roomInfo.set_leader_seat( index + 1 );
	}

	// 플레이 중인 인원이 1명 뿐이면 검색 못함
	/*if ( GetMemberCnt() < 2 )
		return;*/

	// Boss 가 시작하자 마자 Die 를 한경우가 아니라면 보스를 교체한다.
	//if ( false == forceChange ) {
	//	if ( false == HasBossAnyBetting() )
	//		//if ( ( *m_bossIter )->GetLostChip() <= cHoldem::CalcSeed( m_pGameRoom->m_roomInfo.room_type() , m_pGameRoom->m_roomInfo.seed_chip_type() ) )
	//		return;
	//}

	if ( GetMemberCnt() == 0 )
		return;


	if ( GetMemberCnt() == 1 ) {

		for ( auto searchIter = m_playerSlots.begin(); searchIter != m_playerSlots.end(); ++searchIter ) {

			auto player = *searchIter;
			if ( player == nullptr ) continue;

			m_dealerIter = searchIter;

			int bossSearchSlot = std::distance( m_playerSlots.begin() , m_dealerIter ) + 1;
			int beforeBossSlot = m_pGameRoom->m_roomInfo.leader_seat();

			// 기존 보스와 이번에 결정된 보스 슬롯을 비교
			if ( beforeBossSlot != bossSearchSlot ) {
				m_pGameRoom->m_roomInfo.set_leader_seat( bossSearchSlot );
				SendBossChange( bossSearchSlot );
			}

			return;
		}

	}

	bool findSuccess = false;

	// m_bossIter 부터 end 까지
	for ( auto searchIter = m_dealerIter; searchIter != m_playerSlots.end(); ++searchIter )
	{
		if ( searchIter == m_dealerIter )
			continue;

		auto player = *searchIter;
		if ( player == nullptr )
			continue;

		m_dealerIter = searchIter;
		findSuccess = true;
		break;
	}

	// m_bossIter 을 검색하지 못했을 경우 m_bossIter 까지 재 검색
	// begin 부터 m_bossIter 까지
	if ( false == findSuccess ) {
		for ( auto iter = m_playerSlots.begin(); iter != m_playerSlots.end(); ++iter )
		{
			cClientSession* player = *iter;
			if ( player == nullptr )
				continue;

			if ( iter == m_dealerIter )
				break;

			m_dealerIter = iter;
			break;
		}
	}

	int bossSearchSlot = std::distance( m_playerSlots.begin() , m_dealerIter ) + 1;
	int beforeBossSlot = m_pGameRoom->m_roomInfo.leader_seat();

	// 기존 보스와 이번에 결정된 보스 슬롯을 비교
	if ( beforeBossSlot != bossSearchSlot ) {
		m_pGameRoom->m_roomInfo.set_leader_seat( bossSearchSlot );
		SendBossChange( bossSearchSlot );
	}
}

void cHoldem::DecideDealer( int64 outuserindx , bool forceChange )
{
	// 방생성시에 최초는 1번으로 자동 결정
	if ( m_pGameRoom->m_roomInfo.leader_seat() == 0 ) {
		m_dealerIter = m_playerSlots.begin();
		int index = std::distance( m_playerSlots.begin() , m_dealerIter );
		m_pGameRoom->m_roomInfo.set_leader_seat( index + 1 );
	}

	// 플레이 중인 인원이 1명 뿐이면 검색 못함
	if ( GetMemberCnt() < 2 )
		return;

	// Boss 가 시작하자 마자 Die 를 한경우가 아니라면 보스를 교체한다.
	//if ( false == forceChange ) {
	//	if ( false == HasBossAnyBetting() )
	//		//if ( ( *m_bossIter )->GetLostChip() <= cHoldem::CalcSeed( m_pGameRoom->m_roomInfo.room_type() , m_pGameRoom->m_roomInfo.seed_chip_type() ) )
	//		return;
	//}

	bool findSuccess = false;
	auto searchIter = m_dealerIter;

	// m_bossIter 부터 end 까지
	for ( ; searchIter != m_playerSlots.end(); ++searchIter )
	{
		if ( searchIter == m_dealerIter )
			continue;

		auto player = *searchIter;
		if ( player == nullptr )
			continue;

		m_dealerIter = searchIter;
		findSuccess = true;
		break;
	}

	// m_bossIter 을 검색하지 못했을 경우 m_bossIter 까지 재 검색
	// begin 부터 m_bossIter 까지
	if ( false == findSuccess ) {
		for ( auto iter = m_playerSlots.begin(); iter != m_playerSlots.end(); ++iter )
		{
			cClientSession* player = *iter;
			if ( player == nullptr )
				continue;

			if ( iter == m_dealerIter )
				break;

			m_dealerIter = iter;
			break;
		}
	}

	int bossSearchSlot = std::distance( m_playerSlots.begin() , m_dealerIter ) + 1;
	int beforeBossSlot = m_pGameRoom->m_roomInfo.leader_seat();

	// 기존 보스와 이번에 결정된 보스 슬롯을 비교
	if ( beforeBossSlot != bossSearchSlot ) {
		m_pGameRoom->m_roomInfo.set_leader_seat( bossSearchSlot );
		SendBossChange( bossSearchSlot , outuserindx );
	}
}

void cHoldem::DecideMaster()
{
	if ( *m_masterIter == nullptr) {
		NextMaster();
		SendMasterChange();
	}
}

// 보스가 변경되었을때 통보를 해준다.
void cHoldem::SendBossChange( const int newBossSlot )
{
	uint64 bossPlayerIdx = m_playerSlots[ newBossSlot - 1 ]->GetPlayerIdx();

	// 보스 변경 전송
	PmNet::MatchLeadSwapRS _res;
	_res.set_lead_seat( newBossSlot );
	_res.set_lead_idx( bossPlayerIdx );
	m_pGameRoom->BroadCastToAllPlayer( General::Packet_LeadSeatNotice , _res );

	m_pGameRoom->m_roomInfo.set_leader_seat( newBossSlot );
}

void cHoldem::SendBossChange( const int newBossSlot , const int64 outuserindx )
{
	uint64 bossPlayerIdx = m_playerSlots[ newBossSlot - 1 ]->GetPlayerIdx();

	// 보스 변경 전송
	PmNet::MatchLeadSwapRS _res;
	_res.set_lead_seat( newBossSlot );
	_res.set_lead_idx( bossPlayerIdx );
	m_pGameRoom->BroadCastToAllPlayer( General::Packet_LeadSeatNotice , _res , outuserindx );

	m_pGameRoom->m_roomInfo.set_leader_seat( newBossSlot );
}

void cHoldem::SendStatusChange( const Server::PlayPhase beforeStatus , std::string callFunc , int64 except_playerIdx )
{
	uint64 playerIdx = GetBossPlayerIdx();

	// play status 바뀌었음을 통보
	if ( beforeStatus != m_gameStep ) {
		PmNet::MatchStateSwapRS _res;
		_res.set_match_kind( General::PlayCategory::PlayCategory_TexasHoldem );
		_res.set_match_phase( m_gameStep );
		_res.set_lead_idx( playerIdx );
		m_pGameRoom->m_roomInfo.set_play_phase( m_gameStep );
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_RoundStateNotice , _res , except_playerIdx );

		std::string beforeStep = protoutil::cProtoUtil::GetEnumString( beforeStatus );
		std::string curStep = protoutil::cProtoUtil::GetEnumString( m_gameStep );
		std::string log = std::format( "{} : curStep {} nextStep {}, Room Number: {}" ,
			callFunc.c_str() , beforeStep.c_str() , curStep.c_str() , m_pGameRoom->GetRoomNumber() );
		//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
	}
}

int cHoldem::GetPlayingPlayerCount()
{
	int playerCount = 0;
	for ( auto player : m_playerSlots ) {
		if ( player != nullptr && false == player->isDie() ) {
			++playerCount;
		}
	}
	return playerCount;
}

int cHoldem::GetActivePlayerCount()
{
	int playerCount = 0;
	for ( auto player : m_playerSlots ) {
		if ( player != nullptr && false == player->isDie() && false == player->isAllIn() &&!player->is_copyed ) {
			++playerCount;
		}
	}
	return playerCount;
}

// MaxBet 베팅을 이미 한 사람도 ActivePlayer로 생각하지 않는다.
std::vector<cClientSession*> cHoldem::GetActivePlayers()
{
	std::vector<cClientSession*> activePlayers;

	uint32 maxBetMoney = GetMaxBetMoney();
	int playerCount = 0;
	for ( auto player : m_playerSlots ) {
		if ( player != nullptr &&
			false == player->isDie() &&
			false == player->isAllIn() && !player->is_copyed)
		{
			activePlayers.push_back( player );
		}
	}
	return activePlayers;
}

std::vector<cClientSession*> cHoldem::GetActivePlayers( const uint64 exceptPlayerIdx )
{
	std::vector<cClientSession*> activePlayers;

	uint32 maxBetMoney = GetMaxBetMoney();
	int playerCount = 0;
	for ( auto player : m_playerSlots ) {
		if ( player != nullptr &&
			player->GetPlayerIdx() != exceptPlayerIdx &&
			false == player->isDie() &&
			false == player->isAllIn() )
		{
			activePlayers.push_back( player );
		}
	}
	return activePlayers;
}

// ActivePlayer + Allin 유저중에 curBetMoney 보다 베팅을 많이한 플레이어
//std::vector<cClientSession*> cHoldem::GetMaxBetPlayers( const uint64 curBetMoney )
//{
//	std::vector<cClientSession*> activePlayers;
//
//	uint32 maxBetMoney = GetMaxBetMoney();
//	int playerCount = 0;
//	for ( auto player : m_playerSlots ) {
//		if ( player == nullptr )
//			continue;
//
//		if ( player->isDie() )
//			continue;
//
//		// 올인이고 베팅 금액이 더 작은 경우
//		if ( player->isAllIn() && player->GetLostMoney() < curBetMoney )
//			continue;
//
//		activePlayers.push_back( player );
//	}
//	return activePlayers;
//}

// 현재 스텝에서 하프 베팅을 할수 있는 유저 체크
int cHoldem::GetPossibleToHalfBetPlayerCount()
{
	// 자신의 현재 Money + 현재까지 잃은 칩 -> 마지막 유저의 베팅금액
	// 위의 조건 보다 금액이 커야함
	uint64 curPot = m_pGameRoom->m_roomInfo.pot_amount();
	uint64 beforeBet = m_pGameRoom->m_roomInfo.previous_wager();
	uint64 halfBetMoney = ( ( curPot + beforeBet ) / 2 ) + beforeBet;

	int playerCount = 0;
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr )
			continue;

		if ( player->isDie() )
			continue;

		if ( player->isAllIn() )
			continue;

		uint64 playerMoney = GetPlayerMoney( player );
		if ( playerMoney < halfBetMoney )
			continue;

		++playerCount;
	}
	return playerCount;
}

// 2명 이하 시작 못함, 베팅 룰에 따라 처리 조건이 달라짐
	// 일반 겜은 2명, 3벳 룰은 3명부터 가능
int cHoldem::GetMinStartPlayerCount()
{
	switch ( m_pGameRoom->m_roomInfo.bet_policy() )
	{
	case General::BetPolicy::BetPolicy_HoldemStandard:
		return 2;
	case General::BetPolicy::BetPolicy_HoldemThreeBet:
		return 3;
	case General::BetPolicy::BetPolicy_HoldemFourBet:
		return 4;
	default:
		return 0;
	}
}

void cHoldem::GetCurrentStatus( PmNet::MatchStateSwapRS& _res )
{
	uint64 remain_msec = m_expireTick - ::GetTickCount64();

	_res.set_match_kind( GetGameType() );
	_res.set_match_phase( m_gameStep );
	_res.set_phase_remain_ms( remain_msec );
}

void cHoldem::EndGame()
{
	m_pGameRoom->m_roomInfo.set_pot_amount( 0 );
	for ( auto& player : m_playerSlots ) {
		if ( player == nullptr )
			continue;
		player->SetAllIn( false );
		player->SetSide( false );
		player->SetDie( false );
	}
}

void cHoldem::OnIntruding( cClientSession* pClientSession , bool ignoreCommunity , bool relogin)
{
	PmNet::SharedTileRS _res;
	_res.set_intrusion_flag( true );
	for ( const auto& community_card : m_communityCards ) {
		auto before_community_card = _res.add_shared_cards();
		before_community_card->CopyFrom( community_card );
	}

	std::string errorMessage;

	// 난입과 다르게, 게임 참여를 통해 들어오는 경우에는 커뮤니티 카드를 보내지 않는다.
	// 초기화 시점때문에 커뮤니티 카드가 보이게 된다.
	//if ( ignoreCommunity )
	pClientSession->SendRequest( General::Packet_BoardCardReveal , _res , General::ResultCode::Result_Success , errorMessage );

	// 방의 스텝 별로 전송할 내역을 세분화 한다
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
	{
		OnIntrudingSendTurnExpireAndSendPlayerTurn();
	}
	break;
	case Server::PlayPhase::PlayPhase_Judgement:
	{
		if ( relogin )
		{
			_result_res.set_quick_reveal( true );
			pClientSession->SendRequest( General::Packet_RoundOutcome , _result_res , General::ResultCode::Result_Success , errorMessage );
		}
	}
	break;
	}

	// 방의 상태와 남은 시간은 알려준다.
	PmNet::MatchStateSwapRS statusResponse;
	GetCurrentStatus( statusResponse );
	pClientSession->SendRequest( General::Packet_RoundStateNotice , statusResponse , General::ResultCode::Result_Success , errorMessage );
}


// Bet_1 1라운드 마지막 유저의 베팅이 끝나고 나고 지속적으로 호출
// Bet_2, Bet_3, Bet_4 1라운드 마지막 유저의 베팅이 끝나고 나고 지속적으로 호출
// 모든 유저의 베팅 금액이 동일한지 확인해서 같으면 true 리턴
//bool cHoldem::CheckAllPlayingPlayerSameBet()
//{
//	std::vector<cClientSession*> activeUsers;
//
//	for ( auto iter = m_playerSlots.begin(); iter != m_playerSlots.end(); ++iter )
//	{
//		auto player = *iter;
//		if ( player == nullptr || player->isDie() || player->isAllIn() )
//			continue;
//
//		activeUsers.push_back( player );
//	}
//
//	// 사이즈가 0이면 안되는데??
//	if ( activeUsers.size() == 0 )
//		throw std::exception( "cHoldem::CheckAllPlayingPlayerSameBet activeUsers size 0" );
//
//	uint64 compareBase = activeUsers[ 0 ]->GetLostMoney();
//	for ( auto player : activeUsers ) {
//		if ( player->GetLostMoney() != compareBase )
//			return false;
//	}
//	return true;
//}

// 블라인드 베팅을 빼고
// 베팅을 최소한 1번씩은 했는데 , 금액이 동일한 경우에 종료
// 홀덤의 BET_1 일때만 이조건으로 체크한다.
bool cHoldem::CheckAllPlayingPlayerSameBetWithBetOnce()
{
	// 블라인드 베팅 제외하고 최소한 1회 베팅을 전부 했는지 체크 한다.
	std::vector<General::TableAction> bettings;

	for ( auto player : m_playerSlots ) {
		if ( player == nullptr || player->isDie() )
			continue;

		bettings.clear();
		player->GetStepBettingExceptBindBet( Server::PlayPhase::PlayPhase_FirstBet , bettings );

		if ( bettings.size() < 1 )
			return false;
	}


	return CheckAllPlayingPlayerSameBet();
}

// Bet_1 1라운드 마지막 유저의 베팅이 끝나고 나고 지속적으로 호출
// Bet_2, Bet_3, Bet_4 1라운드 마지막 유저의 베팅이 끝나고 나고 지속적으로 호출
// 모든 유저의 베팅 금액이 동일한지 확인해서 같으면 true 리턴
// 조건 추가 : 모든 유저가 All 인 유저보다는 베팅한 금액이 크면서 베팅 금액이 동일한 경우
bool cHoldem::CheckAllPlayingPlayerSameBet()
{
	std::vector<cClientSession*> activeUsers;

	for ( auto player : m_playerSlots ) {
		if ( player == nullptr || player->isDie() )
			continue;

		activeUsers.push_back( player );
	}

	// 사이즈가 0이면 안되는데??
	//if ( activeUsers.size() == 0 )
	//	throw std::exception("cLowBaduki::CheckAllPlayingPlayerSameBet activeUsers size 0");
	if ( activeUsers.size() == 0 )
		return true;

	if ( IsSameVirtualBet( activeUsers ) )
		return true;

	//// Size 로직이 발생했는지 검사
	//uint64 sideUserBetMoney = 0;
	//auto sides = cSidePotManager::GetAllSides();
	//for ( auto side : sides ) {
	//	if ( side->m_sidePotBase > sideUserBetMoney )
	//		sideUserBetMoney = side->m_sidePotBase;
	//}

	//// 조건 추가 : 모든 유저가 All 인 유저보다는 베팅한 금액이 크면서 베팅 금액이 동일한 경우
	//// 사이드 유저보다 모두 금액이 동일하거나 이상인가?
	//for ( auto player : activeUsers ) {
	//	if ( player->GetLostMoney() < sideUserBetMoney )
	//		return false;
	//}

	// 모든 유저의 베팅 금액이 동일한가?
	uint64 compareBase = activeUsers[ 0 ]->GetLostMoney();
	for ( auto player : activeUsers ) {
		if ( player->GetLostMoney() != compareBase )
			return false;
	}
	return true;
}

//  || player ->GetLostMoney() != GetMaxBetMoney()
bool cHoldem::IsSameVirtualBet( std::vector<cClientSession*>& activeUsers )
{
	for ( auto player : activeUsers ) {

		// Max 베팅을 한 유저는 건너뛴다.
		if ( player->GetLostMoney() == GetMaxBetMoney() )
			continue;

		if ( player->GetVirtualLostMoney() != m_lastPlayerBetTotal )
			return false;
	}
	return true;
}

// 최근 5게임을 분석해서 승리한 판수를 파악한다.
// 기권은 어떻게 구분할거냐?????? General::HandRank 이걸로 해결이 안되는데??
google::protobuf::RepeatedField<General::WinningHandHistory> cHoldem::GetRecentlyPlayedGames()
{
	google::protobuf::RepeatedField<General::WinningHandHistory> returnJokbos;
	//int history_base = m_win_jokbos.size() >= 5 ? 5 : m_win_jokbos.size(); 5, 4, 3, 2, 1 이런식으로 내려줄때
	int history_base = m_win_jokbos.size(); // 100, 99, 98 플레이한 순서대로
	int outSize = 0;
	for ( auto iter = m_win_jokbos.rbegin(); iter != m_win_jokbos.rend(); ++iter )
	{
		if ( returnJokbos.size() >= NetLib::cSingleton<cDataLoader>::GetInstance()->RecentlyHistoryCount )
			break;

		General::WinningHandHistory& jokbo = *iter;
		General::WinningHandHistory clone = jokbo;
		clone.set_play_order( history_base - outSize );
		returnJokbos.Add( clone );

		++outSize;
	}
	return returnJokbos;
}

// 방장이 kick out 한 플레이어들을 내보낸다.
void cHoldem::ProcessKickOut()
{
	// 방 나감 알림
	PmNet::ChamberLeaveRS _res;
	_res.set_chamber_no( m_pGameRoom->GetRoomNumber() );

	PmNet::ExpelMemberRS _kick_res;
	std::string errorMessage;

	for ( auto& playerPair : m_kickedPlayers ) {

		const uint64& kickPlayerIdx = playerPair.second;
		cClientSession* pClientSession = GetPlayerALLSession( kickPlayerIdx );

		cClientSession* pClientSession_boss = GetPlayerSession( GetBossPlayerIdx() ); //새로 추가

		if ( pClientSession == nullptr ) {
			//예약자일경우
			/*pClientSession = GetReservaionSession( kickPlayerIdx );
			if( pClientSession == nullptr )*/
				continue;
		}

		if ( pClientSession->HasMoneyLimit() ) {
			pClientSession->SendMoneyLimitPopopOnResult();
		}

		_kick_res.set_expel_member_idx( kickPlayerIdx );
		m_pGameRoom->SendRequest( pClientSession , General::Packet_UserRemove , _kick_res , General::ResultCode::Result_Success , errorMessage );

		_res.set_member_idx( kickPlayerIdx );
		_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() );
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );

		LeaveSlot( pClientSession , true );
		RemoveReservation( pClientSession );
		CancelWatcherReservation( pClientSession );

		m_kick_out_count++;

		//20801 작성

		int code = 20801;
		auto master = ( *m_masterIter );
		if ( nullptr != master )
		{
			auto log_result = QueryManager::InsertGameKickoutLog(
				code ,                             // 코드
				m_gameUid ,           // 게임 ID ( 못찾음 )
				m_channel.id() ,           // 채널
				m_pGameRoom->m_roomInfo.played_rounds() ,                         // 게임 횟수
				"" ,           // 로그 버전
				master->GetPlatformGuid() ,              // 호스트
				master->GetKickOutTicketCount() ,                   // 퇴장 티켓 수
				m_kick_out_count ,                    // 퇴장 횟수
				master->GetPlatformGuid() + " | " + pClientSession->GetPlatformGuid() ,              // SKEY
				pClientSession->GetPlatformGuid() ,            // 타겟
				"roomid_" + std::to_string( m_pGameRoom->GetRoomNumber() )         // 방 ID
			);

			log_result.wait();
		}

		pClientSession->RoomOutReset();
		m_pGameRoom->RemovePlayer( kickPlayerIdx );
		m_pGameRoom->RemoveWatcher( kickPlayerIdx );
	}

	m_kickedPlayers.clear();
}

void cHoldem::ProcessKickOutDieUser()
{
	// 방 나감 알림
	PmNet::ChamberLeaveRS _res;
	_res.set_chamber_no( m_pGameRoom->GetRoomNumber() );

	PmNet::ExpelMemberRS _kick_res;
	std::string errorMessage;

	for ( auto& playerPair : m_kickedPlayers ) {

		const uint64& kickPlayerIdx = playerPair.second;
		cClientSession* pClientSession = GetPlayerALLSession( kickPlayerIdx );
		if ( pClientSession == nullptr ) continue;

		if ( pClientSession->HasMoneyLimit() ) {
			pClientSession->SendMoneyLimitPopopOnResult();
		}

		_kick_res.set_expel_member_idx( kickPlayerIdx );
		m_pGameRoom->SendRequest( pClientSession , General::Packet_UserRemove , _kick_res , General::ResultCode::Result_Success , errorMessage );

		_res.set_member_idx( kickPlayerIdx );
		_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() );
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );

		LeaveSlot( pClientSession , true );
		RemoveReservation( pClientSession );
		CancelWatcherReservation( pClientSession );

		pClientSession->RoomOutReset();

		m_pGameRoom->RemovePlayer( kickPlayerIdx );
		m_pGameRoom->RemoveWatcher( kickPlayerIdx );
	}

	m_kickedPlayers.clear();
}

bool cHoldem::IsParticipationPlayer( const uint64 playerIdx )
{
	for ( auto iter = m_participateQueue.begin(); iter != m_participateQueue.end(); ++iter ) {

		if ( *iter == nullptr ) continue;
		if ( ( *iter )->GetPlayerIdx() == playerIdx ) {
			return true;
		}
	}
	return false;
}

void cHoldem::ClearBet( cClientSession* pClientSession , Server::PlayPhase step )
{
	if ( nullptr == pClientSession->GetBettings()->Lookup( step ) ) {
		pClientSession->GetBettings()->SetAt( step , new std::vector<General::TableAction>() );
	}
	else {
		pClientSession->GetBettings()->Lookup( step )->m_value->clear();
	}
}

// 최초로 베팅할 유저를 결정한다.
// dealer 다음에 Die 유저와 nullptr 을 제외한 플레이어를 선택한다.
std::vector<cClientSession*>::iterator cHoldem::DecideHoldemFirstBetPlayer()
{
#ifdef _DEBUG
	auto dealer = ( *m_dealerIter );
	std::string dealerNick = dealer->GetNickName();
#endif

	std::vector<cClientSession*>::iterator searchIter;

	// 처음부터 찾습니다.
	if ( m_dealerIter == m_playerSlots.end() ) {
		for ( std::vector<cClientSession*>::iterator iter = m_playerSlots.begin(); iter != m_playerSlots.end(); ++iter )
		{
			if ( *iter == nullptr || ( *iter )->isDie() )
				continue;

			searchIter = iter;
		}
	}
	else {
		bool findSuccess = false;

		// m_bossIter 부터 end 까지
		for ( searchIter = m_dealerIter; searchIter != m_playerSlots.end(); ++searchIter ) {
			if ( searchIter == m_dealerIter )
				continue;

			if ( *searchIter == nullptr )
				continue;

			//m_dealerIter = searchIter;
			findSuccess = true;
			break;
		}

		// m_bossIter 을 검색하지 못했을 경우 m_bossIter 까지 재 검색
		// begin 부터 m_bossIter 까지
		if ( false == findSuccess ) {
			for ( searchIter = m_playerSlots.begin(); searchIter != m_playerSlots.end(); ++searchIter ) {
				if ( *searchIter == nullptr )
					continue;

				if ( searchIter == m_dealerIter )
					break;

				break;
			}
		}
	}

#ifdef _DEBUG
	auto firstBetPlayer = ( *searchIter );
	std::string firstBetNick = firstBetPlayer->GetNickName();
#endif

	return searchIter;
}

// 홀덤에서 sb, bb, gb 후에 베팅 순서를 처리한다.
// m_curBetPlayerIter 순서를 변경하지 않는다.
// 3 bet 룰부터는 베팅 라운드가 변경 될 수 있어서 처리가 필요하다.
void cHoldem::DecideHoldemBet1()
{
	// 다음 사람부터 베팅을 진행시킨다.
	// 베팅 라운드가 첫번째 라운드가 아닐 수 도 있습니다.
	m_curBetPlayerIter = GetNextBetPlayer();
	if ( m_curBetPlayerIter == m_betSequence.end() ) {
		int round = static_cast< int >( m_curBetRound );
		round = round + 1;
		m_curBetRound = static_cast< E_BET_ROUND >( round );
		m_curBetPlayerIter = m_betSequence.begin();
	}

	// Iterator 안전성 검사 추가
	if ( m_curBetPlayerIter == m_betSequence.end() || m_betSequence.empty() ) {
		// 라이브 서버에서는 로그만 남기고 안전하게 처리
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Holdem betting sequence is empty" );
		return; // 함수를 안전하게 종료
	}

	int m_curBetSlot = ( *m_curBetPlayerIter )->GetBettingSlot();
	m_pGameRoom->m_roomInfo.set_active_bet_seat( m_curBetSlot );
}

// m_betSequence 베팅순서 결정을 위해 사용해서는 안된다.
// 베팅을 할때마다 순서를 결정해야 할것 같다. Die 유저 때문에
void cHoldem::DecideFirstBet()
{
	m_curBetPlayerIter = GetFirstBetPlayer();
	if ( (*m_curBetPlayerIter) == nullptr )
	{
		m_pGameRoom->m_roomInfo.set_active_bet_seat( 0 );
		return;
	}
	int m_curBetSlot = ( *m_curBetPlayerIter )->GetBettingSlot();
	m_pGameRoom->m_roomInfo.set_active_bet_seat( m_curBetSlot );
}

// 홀덤의 베팅 순서는 dealer 이후 부터 처리 한다.
// m_dealerIter 다음 사람부터 m_dealerIter 까지 ( 딜러가 마지막에 베팅 함 )
void cHoldem::DecideBetSequence()
{
	for ( auto player : m_betSequence )
	{
		if ( player == nullptr )
			continue;
		if ( player->is_copyed )
			delete player;
	}
	m_betSequence.clear();

	auto firstBetIter = DecideHoldemFirstBetPlayer();

	for ( auto iter = firstBetIter; iter != m_playerSlots.end(); ++iter ) {
		cClientSession* player = *iter;
		m_betSequence.push_back( player );
	}

	// begin 부터 firstBetIter 까지
	for ( auto iter = m_playerSlots.begin(); iter != m_playerSlots.end(); ++iter ) {
		if ( iter == firstBetIter )
			break;
		cClientSession* player = *iter;
		m_betSequence.push_back( player );
	}

	// 무의미 해짐
	//assert( m_betSequence.size() == m_pGameRoom->GetMaxRoomPlayerCnt() , "m_playerSlots size error" );

#ifdef _DEBUG

	int betSequence = 0;
	for ( auto player : m_betSequence ) {
		++betSequence;
		if ( player == nullptr )
			continue;

		uint64 moneyValue = GetPlayerMoney( player );
		std::string logString = std::format( " bet Sequence {}, NickName {} PlayerIdx {} Money {}" , betSequence , player->GetNickName().c_str() , player->GetPlayerIdx() , moneyValue );
		TraceA( logString );
		InsertBetLog( logString );
	}

#endif

	// ClientSession 에 자신의 베팅 어레이 순서 저장
	for ( int n = 0; n < m_betSequence.size(); ++n ) {
		if ( m_betSequence[ n ] != nullptr ) {
			m_betSequence[ n ]->SetBettingSlot( n + 1 );
			m_betSequence[ n ]->SetDie( false );
			m_betSequence[ n ]->SetAllIn( false );
			m_betSequence[ n ]->SetSide( false );

			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_FirstBet );
			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_SecondBet );
			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_ThirdBet );
			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_FinalBet );

			// 카드 분배는
			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_HoldemFlop );
			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_HoldemTurn );
			ClearBet( m_betSequence[ n ] , Server::PlayPhase::PlayPhase_HoldemRiver );
		}
	}

	// 베팅을 할때마다 순서를 결정해야 할것 같다. Die 유저 때문에
	DecideFirstBet();
}

General::ResultCode cHoldem::UserBet( cClientSession* pClientSession , const PmNet::MatchWagerRQ& userBet )
{
	// 게임 스텝 확인
	if ( m_gameStep != Server::PlayPhase::PlayPhase_FirstBet
		&& m_gameStep != Server::PlayPhase::PlayPhase_SecondBet
		&& m_gameStep != Server::PlayPhase::PlayPhase_ThirdBet
		&& m_gameStep != Server::PlayPhase::PlayPhase_FinalBet )
		return General::ResultCode::Result_PlayStepMismatch;

	const General::TableAction& betting = userBet.wager_kind();

	// 현재 방에서 가능한 베팅인지 확인
	ATL::CAtlMap<General::TableAction , General::TableAction>::CPair* pPair = m_allowedBetTypes.Lookup( betting );
	if ( pPair == nullptr )
		return General::ResultCode::Result_BetOptionRejected;

	// 유저의 순서가 맞는지 확인
	int m_curBetSlot = m_pGameRoom->m_roomInfo.active_bet_seat();
	if ( m_curBetSlot == 0 )
		return General::ResultCode::Result_NullParameterFault;
	// 방어코드
	cClientSession* curBet = m_betSequence[ m_curBetSlot - 1 ];
	if ( nullptr == curBet )
		return General::ResultCode::Result_TurnOwnershipMismatch;

	uint64 turnWho = curBet->GetPlayerIdx();
	if ( turnWho != pClientSession->GetPlayerIdx() )
		return General::ResultCode::Result_TurnOwnershipMismatch;

	uint64 beforeStockMoney = pClientSession->GetLostMoney();
	uint64 beforeMoney = GetPlayerMoney( pClientSession );

	BOOL isAllinBeforeBet = pClientSession->isAllIn();
	//BOOL isSideBeforeBet = pClientSession->isSide();

	BOOL allin , side; // AllIn, Side 발생 상황 전송
	General::ResultCode betResult = CaclBet( pClientSession , betting , allin , side ); // 베팅 처리
	if ( betResult != General::ResultCode::Result_Success )
		return betResult;


	uint64 afterStockMoney = pClientSession->GetLostMoney();

	uint64 curLostMoney = 0;
	if ( afterStockMoney >= beforeStockMoney )
		curLostMoney = afterStockMoney - beforeStockMoney;

	uint64 afterMoney = GetPlayerMoney( pClientSession );

	// 베팅이 성공했다면, 베팅 금액과 베팅 내역 Player Cache 에 저장
	pClientSession->SetPlayerBet( betting , curLostMoney );

	// 베팅 정보 전송
	PmNet::MatchWagerRS _res;
	_res.set_wager_member_idx( pClientSession->GetPlayerIdx() );
	_res.set_member_wager( betting );
	_res.set_fund_kind( m_pGameRoom->m_roomInfo.asset_kind() );
	_res.set_fund_before( beforeMoney );
	_res.set_fund_after( afterMoney );

	// 베팅 전에 이미 올인 상황이면 그대로 내려줌
	if ( isAllinBeforeBet )
		_res.set_all_in_flag( isAllinBeforeBet );
	else
		_res.set_all_in_flag( allin );

	// 사이드가 발생이 되는 상황일때만 보냄
	/*if ( FALSE == isSideBeforeBet && TRUE == side )
		_res.set_flank_flag( side );*/
	if ( side )
		_res.set_flank_flag( side );

	pClientSession->SetSide( side );

	m_pGameRoom->BroadCastToAllPlayer( General::Packet_StakeSubmit , _res );

	// 사이드 발생 플레이어 알림
	//SendSide();
	CheckSide();

	GoToDirectResult();


	return betResult;
}

void cHoldem::CheckSide()
{
	for ( auto player : m_playerSlots )
	{
		if ( player == nullptr )
			continue;
		if ( player->isSide() )
			continue;

		if ( player->isAllIn() && maxbet > player->GetLostMoney() )
		{
			player->SetSide();
			PmNet::FlankWagerRS _res;
			_res.set_member_idx( player->GetPlayerIdx() );
			_res.set_flank_flag( true );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_SideStakeNotice , _res );
		}


	}
}

// AllIn 유저들을 체크해서
// 현재 베팅 금액이 유저가 베팅한 금액보다 커지는 순간 Side를 발송한다.
//void cHoldem::SendSide()
//{
//	PmNet::NotifySidePlayersRes _res;
//
//	for ( auto player : m_playerSlots ) {
//		if ( player == nullptr ) continue;
//
//		// 각 플레이어들의 베팅시마다 체크해서
//		// Side 플레이어들을 클라이언트 에게 알려준다.
//		if ( player->isAllIn() && false == player->isSendSide() ) {
//
//			if ( player->GetLostMoney() < m_lastPlayerBetTotal ) {
//
//				_res.add_flank_member_idx_list( player->GetPlayerIdx() );
//
//				player->SetSendSide();
//			}
//
//		}
//	}
//
//	if ( _res.flank_member_idx_list().size() ) {
//		m_pGameRoom->BroadCastToAllPlayer( Common::GMsg_NotifySidePlayers , _res );
//	}
//}

void cHoldem::BetProcess()
{
	if ( isTurnTimeOver() ) {

		// 베팅을 하지 않은 경우에만 다이 처리
		if ( false == CheckBet() )
			DieCurPlayer();
		//CheckDieUser();

		/*else
		{
			cClientSession* pClientSession = ( *m_curBetPlayerIter );
			pClientSession->InitNoActionCounter();
		}*/

		if ( GoToDirectResult() )
			return;
	}



	if ( CheckBet() ) {
		if ( false == NextBet() ) {
			m_expireTick = ::GetTickCount64();	// NextBet 실패의 경우는 다음 스텝으로 진행
		}
		//CheckDieUser();

		/*cClientSession* pClientSession = ( *m_curBetPlayerIter );
		pClientSession->InitNoActionCounter();*/

		// ShowDown Check
		// Wait Until ShowDown Animation Finish

		if ( isShowDownTime() ) {
			int leftCommunityCards = 0;
			DoShowDown( leftCommunityCards );
			m_expireTick = ::GetTickCount64() + CalculateShowDownAnimation( leftCommunityCards );
			m_gameStep = Server::PlayPhase::PlayPhase_HoldemShowdown;
		}
	}
}

// 맥스 올인으로 인해 베팅할 금액이 없는 경우
bool cHoldem::isShowDownTime()
{
	// 커뮤니티 카드가 4장이하
	if ( m_communityCards.size() > 4 )
		return false;

	// activeUsers 가 채널에 설정된 MaxBet 을 이미 사용했으면 ShowDown
	std::vector<cClientSession*> activePlayers = GetActivePlayers();

	// Active Player 는 1명 이상이면 ShowDown 상황이 아님
	//if ( activePlayers.size() > 1 )
	//	return false;

	// 채널에 설정된 맥스베팅 금액을 모두 넘어선 경우
	if ( AllPlayerBetMax( activePlayers ) )
		return true;

	// Active Player 가 1명이고, 해당 유저의 베팅 금액이 현재까지의 베팅 금액과 동일하면 ShowDown
	if ( activePlayers.size() == 1 ) {
		auto player = activePlayers[ 0 ];
		if ( player->GetLostMoney() == m_lastPlayerBetTotal )
			return true;
	}

	return NoOneCanBet( activePlayers );

	// 하프 베팅을 할수 있는 유저가 없다.
	//return GetPossibleToHalfBetPlayerCount() == 0;
}

// 남은 커뮤니티 카드 분배
void cHoldem::DoShowDown( int& leftCommunityCards )
{
	leftCommunityCards = 5 - m_communityCards.size();

	PmNet::RevealRS _res;

	for ( int n = 0; n < leftCommunityCards; ++n ) {
		std::vector<General::PlayingCard>::iterator iter = m_deck.begin();
		General::PlayingCard& newCard = *iter;

#ifdef _DEBUG
		// QA 커뮤니티 카드 셋팅 된경우 변경
		auto qa_iter = m_qa_community_deck.begin();
		if ( qa_iter != m_qa_community_deck.end() ) {
			newCard = *qa_iter;
			m_qa_community_deck.erase( qa_iter );
		}
#endif

		m_communityCards.push_back( newCard );

		auto new_community_card = _res.add_fresh_shared_cards();
		new_community_card->CopyFrom( newCard );

		m_deck.erase( iter ); // 덱에서 삭제
	}

	for ( const auto& community_card : m_communityCards ) {
		auto before_community_card = _res.add_shared_cards();
		before_community_card->CopyFrom( community_card );
	}

	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		// 베팅 금액이 최종 베팅 금액 보다 적고
		// Die 한 유저가 아니면
		// Side 발송해준다.
		if ( m_lastPlayerBetTotal > player->GetLostMoney() && player->isDie() == false ) {
			_res.add_flank_member_idx_list( player->GetPlayerIdx() );
		}

		General::CardSet player_cards;
		for ( const auto& card : player->GetCardsClone() ) {
			auto add_card = player_cards.add_playing_cards();
			add_card->CopyFrom( card );
		}

		_res.mutable_member_hand_tiles()->insert( { player->GetPlayerIdx(), player_cards } );
	}

	m_pGameRoom->BroadCastToAllPlayer( General::Packet_FinalReveal , _res );
}

// 래빗헌팅: 플랍(3장)/턴(4장)이 열린 상태에서 폴드로 판이 일찍 끝난 경우에 한해,
// 결과창에서 요청 시 남은 덱에서 "계속 진행했다면" 나왔을 나머지 카드(들)를 보여준다.
// 눈요기용 기능으로, 실제 배당/승패에는 영향을 주지 않는다 (m_deck 을 소모하지 않음).
// 프리플랍 올폴드(커뮤니티 카드 0장)는 대상에서 제외한다.
General::ResultCode cHoldem::RabbitHunt( cClientSession* pClientSession , std::vector<General::PlayingCard>& outCards )
{
	if ( pClientSession == nullptr )
		return General::ResultCode::Result_NullParameterFault;

	if ( m_gameStep != Server::PlayPhase::PlayPhase_Judgement )
		return General::ResultCode::Result_PlayStepMismatch;

	// 이번 핸드에 참가했던 유저(폴드 포함)만 요청 가능. 막 착석했거나 참가 안 한 관전자는 대상 아님.
	const uint64 playerIdx = pClientSession->GetPlayerIdx();
	bool participated = false;
	for ( auto player : m_betSequence ) {
		if ( player != nullptr && player->GetPlayerIdx() == playerIdx ) {
			participated = true;
			break;
		}
	}
	if ( false == participated )
		return General::ResultCode::Result_ActionRejected;

	// 프리플랍 올폴드(0장)는 대상 아님. 이미 5장이 다 깔린 경우(리버 진행/쇼다운 완료)도 헌팅할 카드가 없다.
	if ( m_communityCards.empty() || m_communityCards.size() >= 5 )
		return General::ResultCode::Result_ActionRejected;

	if ( false == m_rabbitHuntDone )
	{
		m_rabbitHuntCards.clear();
		const int leftCount = 5 - static_cast< int >( m_communityCards.size() );
		for ( int n = 0; n < leftCount && n < static_cast< int >( m_deck.size() ); ++n ) {
			m_rabbitHuntCards.push_back( m_deck[ n ] );
		}
		m_rabbitHuntDone = true;
	}

	outCards = m_rabbitHuntCards;
	return General::ResultCode::Result_Success;
}

// 내 패 보여주기: 폴드한 유저는 언제든, 폴드하지 않은 유저는 쇼다운 없이(상대 전원 폴드로) 이겼을 때만
// 결과창에서 자발적으로 자기 패의 왼쪽/오른쪽/전체 중 하나를 골라 공개한다.
General::ResultCode cHoldem::VoluntaryShowHand( cClientSession* pClientSession , const General::HandRevealScope& scope , std::vector<General::PlayingCard>& outCards )
{
	if ( pClientSession == nullptr )
		return General::ResultCode::Result_NullParameterFault;

	if ( m_gameStep != Server::PlayPhase::PlayPhase_Judgement )
		return General::ResultCode::Result_PlayStepMismatch;

	if ( scope == General::HandRevealScope::HandRevealScope_None )
		return General::ResultCode::Result_ActionRejected;

	const uint64 playerIdx = pClientSession->GetPlayerIdx();

	// 요청자를 이번 핸드 참여자 목록에서 찾고, 동시에 생존자(폴드 안 한 유저) 수를 센다.
	cClientSession* requester = nullptr;
	int aliveCount = 0;
	for ( auto player : m_betSequence ) {
		if ( player == nullptr ) continue;
		if ( player->GetPlayerIdx() == playerIdx )
			requester = player;
		if ( false == player->isDie() )
			++aliveCount;
	}

	if ( requester == nullptr )
		return General::ResultCode::Result_ActionRejected;

	// 폴드한 유저는 언제든 공개 가능. 폴드 안 한 유저는 본인이 유일 생존자(쇼다운 없는 승리)일 때만 가능.
	if ( false == requester->isDie() && aliveCount != 1 )
		return General::ResultCode::Result_ActionRejected;

	if ( m_shownHands.contains( playerIdx ) )
		return General::ResultCode::Result_ActionRejected;

	m_shownHands.insert( playerIdx );

	google::protobuf::RepeatedField<General::PlayingCard> cards = requester->GetCardsClone();

	outCards.clear();
	switch ( scope )
	{
	case General::HandRevealScope::HandRevealScope_Left:
		if ( cards.size() > 0 ) outCards.push_back( cards[ 0 ] );
		break;
	case General::HandRevealScope::HandRevealScope_Right:
		if ( cards.size() > 1 ) outCards.push_back( cards[ 1 ] );
		break;
	case General::HandRevealScope::HandRevealScope_All:
	default:
		for ( const auto& card : cards ) {
			outCards.push_back( card );
		}
		break;
	}

	return General::ResultCode::Result_Success;
}

// 다음 베팅으로 진행한다.
// m_curBetPlayerIter 다음 유저로 진행
// false 인 경우에는 다음 베팅이 없는 경우임으로 NextStep 으로 진행
bool cHoldem::NextBet()
{
	// 베팅할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
		break;
	case Server::PlayPhase::PlayPhase_FinalBet:
		break;
	default:
		ThrowGameStepException( "NextBet" , m_gameStep );
	}

	std::vector<cClientSession*>::iterator iterNext = GetNextBetPlayer();

	// 마지막 유저까지 라운드 베팅이 끝났다.
	if ( iterNext == m_betSequence.end() )
	{
		// 마지막 유저의 베팅이 끝나고 베팅 총량이 동일하면 종료 시킨다.
		if ( CheckAllPlayingPlayerSameBet() )
		{
			if ( isShowDownTime() ) {
				int leftCommunityCards = 0;
				DoShowDown( leftCommunityCards );
				m_expireTick = ::GetTickCount64() + CalculateShowDownAnimation( leftCommunityCards );
				m_gameStep = Server::PlayPhase::PlayPhase_HoldemShowdown;
				return true;
			}else
			return false;
		}
		// 다음 라운드 고고, 실패면 다음 라운드가 존재하지 않는 경우
		if ( false == GoToNextRound() )
			return false;
		else {
			// 베팅 싸이클이 바뀌면 플레이어 베팅 캐쉬 초기화
			ClearPlayerCache();
		}

		// 다시 첫번째 플레이어부터 진행
		m_curBetPlayerIter = GetFirstBetPlayer();
	}
	else {
		m_curBetPlayerIter = iterNext; // 다음 플레이어로 진행

		// 홀덤은 첫번째 스텝에서는 베팅을 3라운드까지 할 수 있다.
		switch ( m_gameStep )
		{
		case Server::PlayPhase::PlayPhase_FirstBet:
		{
			// TODO E_BET_ROUND_2 에서도 체크 하게 해버리면, 마지막 유저가 Call 을 못하는 현상이 발생된다.
			// 마지막 유저까지는 베팅이 되도록 변경해야 한다.
			// 2, 3 라운드에서 베팅총량이 같으면 종료 시킨다.
			if ( m_curBetRound == E_BET_ROUND::E_BET_ROUND_2 || m_curBetRound == E_BET_ROUND::E_BET_ROUND_3 || m_curBetRound == E_BET_ROUND::E_BET_ROUND_4 ) {
				if ( CheckAllPlayingPlayerSameBetWithBetOnce() )
					return false;
			}
		}
		break;
		case Server::PlayPhase::PlayPhase_SecondBet:
		case Server::PlayPhase::PlayPhase_ThirdBet:
		{
			if ( m_curBetRound != E_BET_ROUND::E_BET_ROUND_1 ) {
				if ( CheckAllPlayingPlayerSameBet() )
					return false;
			}
		}
		break;
		case Server::PlayPhase::PlayPhase_FinalBet:
		{
			if ( GetActivePlayerCount() == 1 )
			{
				if ( CheckMaxMoney() == ( *m_curBetPlayerIter )->GetLostMoney() )
				{
					MakeRoomStatusResult();
					return true;
				}

			}
			// 2, 3, 4 라운드에서 베팅총량이 같으면 종료 시킨다.
			if ( m_curBetRound != E_BET_ROUND::E_BET_ROUND_1 ) {
				if ( CheckAllPlayingPlayerSameBet() )
					return false;
			}
			//// 3, 4 라운드에서 베팅총량이 같으면 종료 시킨다.
			//if ( m_curBetRound != E_BET_ROUND::E_BET_ROUND_1 && m_curBetRound != E_BET_ROUND::E_BET_ROUND_2 ) {
			//	if ( CheckAllPlayingPlayerSameBet() )
			//		return false;
			//}
		}
		break;
		}
	}

	int m_curBetSlot = ( *m_curBetPlayerIter )->GetBettingSlot();
	m_pGameRoom->m_roomInfo.set_active_bet_seat( m_curBetSlot );
	if( ( *m_curBetPlayerIter )->isAllIn() )
		SetTurnExpireAndSendPlayerTurn(0);
	else
		SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiBetWait );
	return true;
}

uint64 cHoldem::CheckMaxMoney()
{
	uint64 maxlostmoney = 0;
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr || player->isDie() )
			continue;
		maxlostmoney = maxlostmoney > player->GetLostMoney() ? maxlostmoney : player->GetLostMoney();
	}
	return maxlostmoney;
}


bool cHoldem::CheckDieUser()
{
	bool b_out = false;
	for ( auto player : m_playerSlots )
	{
		if ( player == nullptr )
			continue;
		if ( !player->isDie() )
			continue;

		if ( player->GetDelayTick() > ::GetTickCount64() )
			continue;

		auto kickIter = m_kickedPlayers.find( player->GetPlayerIdx() );
		if ( kickIter != m_kickedPlayers.end() )
		{
			PmNet::ExpelMemberRS _kick_res;
			_kick_res.set_expel_member_idx( player->GetPlayerIdx() );
			std::string errorMessage;
			m_pGameRoom->SendRequest( player , General::Packet_UserRemove , _kick_res , General::ResultCode::Result_Success , errorMessage );
			cClientSession* t_session = new cClientSession( *player );
			auto gameInstance = m_pGameRoom->GetGameInterface();
			gameInstance->SwapWithDummy( player , t_session );
			m_pGameRoom->RoomOut( player->GetPlayerIdx() , player );
			m_kickedPlayers.erase( kickIter );
			b_out = true;
			continue;
		}

		if ( player->GetReserved() )
		{
			cClientSession* t_session = new cClientSession( *player );
			auto gameInstance = m_pGameRoom->GetGameInterface();
			gameInstance->SwapWithDummy( player , t_session );
			m_pGameRoom->RoomOut( player->GetPlayerIdx() , player );
			b_out = true;
			continue;
		}
		auto iter = m_watcherQueue.find( player->GetPlayerIdx() );
		if ( iter != m_watcherQueue.end() )
		{
			cClientSession* t_session = new cClientSession( *player );
			auto gameInstance = m_pGameRoom->GetGameInterface();
			gameInstance->SwapWithDummy( player , t_session );
			// 플레이어에서 삭제
			m_pGameRoom->RemovePlayer( player->GetPlayerIdx() );

			// 관전자 등록
			m_pGameRoom->RegisterWatcher( player );

			PmNet::ToObserverShiftRS response;
			response.set_chamber_no( m_pGameRoom->GetRoomNumber() );
			response.set_member_idx( player->GetPlayerIdx() );

			m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_WatcherModeEnter , response );
			// 관전자 카운트 알림
			/*PmNet::InformObserverCntRS _watcher_res;
			int reservedCount = GetReservationPlayerPlayerCount();
			_watcher_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() - reservedCount );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

			LeaveSlot( player );

			// 삭제
			m_watcherQueue.erase( iter );
			b_out = true;
		}
	}
	return b_out;
}

// 참여 유저 중에 seedMoney 를 가지고 있지 못하면 시작하지 않습니다.
bool cHoldem::CheckSeed( const uint64 seedMoney )
{
	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		uint64 moneyValue = 0;
		switch ( m_pGameRoom->m_roomInfo.asset_kind() )
		{
		case General::AssetKind::AssetKind_Chip:
			moneyValue = player->GetChip();
			break;
		case General::AssetKind::AssetKind_Coin:
			moneyValue = player->GetCoin();
			break;
		default:
			return false; // 아직 결정되지 않은 재화 타입은 처리 못하도록 막아 둔다.
		}

		if ( moneyValue < seedMoney )
			return false;
	}
	return true;
}

// 유저들의 시드 머니 금액을 차감한다.
bool cHoldem::MinusPlayerSeed( const uint64& seedMoney )
{
	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		if ( false == MinusPlayerMoney( player , seedMoney ) )return false;
		else m_betting[ player->GetPlayerIdx() ] = seedMoney;
	}
	return true;
}

//uint64 cHoldem::GetPlayerMoney( cClientSession* pClientSession )
//{
//	bool checkOnly = true;
//	if ( General::AssetKind::AssetKind_Chip == m_pGameRoom->m_roomInfo.money_type() ) {
//		return pClientSession->GetChip();
//	}
//	else if ( General::AssetKind::AssetKind_Coin == m_pGameRoom->m_roomInfo.money_type() ) {
//		return pClientSession->GetCoin();
//	}
//	else
//		return 0;
//	return 0;
//}

void cHoldem::SetPlayerMoney( cClientSession* pClientSession , uint64 moneyAmount )
{
	bool checkOnly = true;
	if ( General::AssetKind::AssetKind_Chip == m_pGameRoom->m_roomInfo.asset_kind() ) {
		return pClientSession->SetChip( General::PlayCategory::PlayCategory_TexasHoldem , moneyAmount );
	}
	else if ( General::AssetKind::AssetKind_Coin == m_pGameRoom->m_roomInfo.asset_kind() ) {
		return pClientSession->SetCoin( General::PlayCategory::PlayCategory_TexasHoldem , moneyAmount );
	}
}

// 재화가 충분한지 확인
bool cHoldem::CheckPlayerMoney( cClientSession* pClientSession , uint64 moneyAmount )
{
	bool checkOnly = true;
	if ( General::AssetKind::AssetKind_Chip == m_pGameRoom->m_roomInfo.asset_kind() ) {
		if ( false == pClientSession->MinusChip( moneyAmount , checkOnly ) )
			return false;
	}
	else if ( General::AssetKind::AssetKind_Coin == m_pGameRoom->m_roomInfo.asset_kind() ) {
		if ( false == pClientSession->MinusCoin( moneyAmount , checkOnly ) )
			return false;
	}
	else
		return false;
	return true;
}

// 1명의 칩 또는 코인 차감
bool cHoldem::MinusPlayerMoney( cClientSession* pClientSession , uint64 moneyAmount )
{
	if ( moneyAmount <= 0 )
		return false;

	if ( General::AssetKind::AssetKind_Chip == m_pGameRoom->m_roomInfo.asset_kind() ) {
		bool checkOnly;
		if ( false == pClientSession->MinusChip( moneyAmount , checkOnly = false ) )
			return false;
	}
	else if ( General::AssetKind::AssetKind_Coin == m_pGameRoom->m_roomInfo.asset_kind() ) {
		bool checkOnly;
		if ( false == pClientSession->MinusCoin( moneyAmount , checkOnly = false ) )
			return false;
	}
	else
		return false;
	return true;
}

// 모든 플레이 인원의 칩 또는 코인 차감
// DIE 한 유저의 차감 안하는 코드 로직 추가 필요
// 실질적으로 DB에 저장하는 부분은 게임 결과창에서 처리한다.
// 보유금액이 모자란 경우에는 실패로 처리한다.
bool cHoldem::MinusPlayersMoney( uint64 moneyAmount , bool checkOnly )
{
	for ( auto player : m_playerSlots )
	{
		if ( player == nullptr )
			continue;

		if ( General::AssetKind::AssetKind_Chip == m_pGameRoom->m_roomInfo.asset_kind() ) {
			if ( false == player->MinusChip( moneyAmount , checkOnly ) )
				return false;
		}
		else if ( General::AssetKind::AssetKind_Coin == m_pGameRoom->m_roomInfo.asset_kind() ) {
			if ( false == player->MinusCoin( moneyAmount , checkOnly ) )
				return false;
		}
		else
		{

		}
	}

	return true;
}

void cHoldem::ClearStockedMoney()
{
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr )
			continue;

		player->InitLostMoney();
		player->InitVirtualLostMoney();
	}
}

std::vector<cClientSession*>::iterator cHoldem::GetFirstBetPlayer()
{
	for ( std::vector<cClientSession*>::iterator iter = m_betSequence.begin(); iter != m_betSequence.end(); ++iter )
	{
		if ( *iter == nullptr )
			continue;

		// 이번판에 죽었으면 무시
		if ( ( *iter )->isDie() )
			continue;
		/*if ( ( *iter )->isAllIn() )
			continue;*/

		if ( (*iter)->is_copyed )
			continue;

		if ( m_pGameRoom->isWatcher( ( *iter )->GetPlayerIdx() ) )
			continue;

		return iter;
	}
	return m_betSequence.end();
}

// 마지막 까지 한번 쭈욱 돌린다.
// reverse iterator 로 처리해봤는데 문제 생겨서, 순서대로 처리
std::vector<cClientSession*>::iterator cHoldem::GetLastBetPlayer()
{
	std::vector<cClientSession*>::iterator lastPlayerIter = m_betSequence.begin();
	for ( std::vector<cClientSession*>::iterator iter = m_betSequence.begin(); iter != m_betSequence.end(); ++iter )
	{
		if ( *iter == nullptr )
			continue;

		// 이번판에 죽었으면 무시
		if ( ( *iter )->isDie() )
			continue;

		if ( m_pGameRoom->isWatcher( ( *iter )->GetPlayerIdx() ) )
			continue;

		lastPlayerIter = iter;
	}
	return lastPlayerIter;
}

std::vector<cClientSession*>::iterator cHoldem::GetNextBetPlayer()
{
	std::vector<cClientSession*>::iterator nextPlayerIter;

	for ( nextPlayerIter = m_curBetPlayerIter; nextPlayerIter != m_betSequence.end(); ++nextPlayerIter ) {
		if ( nextPlayerIter == m_curBetPlayerIter )
			continue;

		if ( *nextPlayerIter == nullptr )
			continue;

		if ( ( *nextPlayerIter )->is_copyed )
			continue;

		if ( m_pGameRoom->isWatcher( ( *nextPlayerIter )->GetPlayerIdx() ) )
			continue;

		//if ( ( *nextPlayerIter )->isAllIn() )
		//{
		//	BOOL allin , side; // AllIn, Side 발생 상황 전송
		//	General::ResultCode betResult = CaclBet( ( *nextPlayerIter ) , General::TableAction::TableAction_Call , allin , side ); // 베팅 처리
		//	continue;
		//}
		if ( false == ( *nextPlayerIter )->isDie() ) {
			break;
		}
	}

	return nextPlayerIter;
}

bool cHoldem::GoToNextRound()
{
	int round = m_curBetRound;
	++round;
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
	{
		if ( m_curBetRound == E_BET_ROUND::E_BET_ROUND_4 )
			return false;
	}
	break;
	default:
		ThrowGameStepException( "cHoldem::GoToNextRound" , m_gameStep );
	}

	m_curBetRound = static_cast< E_BET_ROUND >( round );
	return true;
}

// 플레이 하는 유저가 1명인 경우
// 결과 처리로 직행한다.
bool cHoldem::GoToDirectResult()
{
	if ( GetPlayingPlayerCount() == 1 ) {
		MakeRoomStatusResult();
		return true;
	}
	return false;
}

// 마지막 유저의 베팅이 끝났는데, 모든 유저가 Die 이거나, Allin 인 경우 베팅을 종료시킨다.
bool cHoldem::NoNextRoundWhenOneUserLeftForBet()
{
	int activeUsers = 0;
	for ( auto player : m_betSequence ) {
		if ( player == nullptr || player->isDie() || player->isAllIn() )
			continue;
		++activeUsers;
	}
	return activeUsers > 1;
}

/* 최대 베팅 금액 결정 로직
1. 로우바둑이( 칩 )
- 2인의 경우 재화가 적은 플레이어의 재화를 따름

2. 로우바둑이( 코인 )
- 로우바둑이( 칩 )과 동일한 베팅 방식 사용

3. 로우바둑이( 친구대전 - 칩 )
- 2인 : 맥스는 재화가 적은플레이어를 기준으로 맥스 베팅 금액 설정
- 3인 : 맥스버튼은 베팅 플레이어가 베팅가능한 최대 금액 베팅
* 3인이상→2인이 된 경우 2인의 규칙을 따름

4. 로우바둑이( 친구대전 - 코인 )
- 전체 : 플레이어가 베팅가능한 최대 금액 베팅
*/
/*
Active 플레이어 중에 2번째로 큰 금액을 맥스 베팅 금액으로 결정한다.
*/
uint64 cHoldem::GetMaxBet( uint64& curBet )
{
	// 활성화 유저가 2명이 안되면 그대로 리턴
	if ( GetActivePlayerCount() < 2 )
		return curBet;

	// 남은 칩을 순으로 sort 한뒤에
	// 2번째 유저의 남은 칩으로 계산 ( 만약에 두명이 동일하면 ??? )
	std::vector<cClientSession*> activePlayers;
	for ( auto player : m_betSequence ) {
		if ( player == nullptr || player->isDie() || player->isAllIn() )
			continue;
		activePlayers.push_back( player );
	}

	uint64 secondPlayerMaxBet = 0;
	if ( m_pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Chip ) {
		std::sort( activePlayers.begin() , activePlayers.end() , &cHoldem::CompareMaxBetChip );
		secondPlayerMaxBet = activePlayers[ 1 ]->GetChip();
	}
	else if ( m_pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Coin ) {
		std::sort( activePlayers.begin() , activePlayers.end() , &cHoldem::CompareMaxBetCoin );
		secondPlayerMaxBet = activePlayers[ 1 ]->GetCoin();
	}

	// 정리된 순서 중에 2번째 금액을 MaxBet 으로 설정한다.
	// curBet 이 MaxBet 보다 작으면 curBet 을 리턴해준다.
	return ( curBet <= secondPlayerMaxBet ? curBet : secondPlayerMaxBet );
}

/* 홀덤 맥스 베팅
1. 채널이 설정한 맥스 베팅 값
2. 내가 가진 보유 재화 전체
3. 게임 진행중인 플레이어 중 두번째로 많은 재화를 가진 플레이어
*/
//uint64 cHoldem::GetHoldemMaxBet( cClientSession* ownerPlayer )
//{
//	std::vector<uint64> maxMoneyOrder;
//
//	//uint64 curMoney = GetPlayerMoney( player );
//	//maxMoneyOrder.push_back( curMoney );
//
//	// 내가 걸수 있는 최대 베팅 금액
//	maxMoneyOrder.push_back( GetMaxBetMoney() - ownerPlayer->GetLostMoney() );
//
//	// 남은 칩을 순으로 sort 한뒤에
//	// 2번째 유저의 남은 칩으로 계산 ( 만약에 두명이 동일하면 ??? )
//	uint64 secondPlayerMaxMoney = 0;
//	//std::vector<cClientSession*> activePlayers = GetActivePlayers( ownerPlayer->GetPlayerIdx() ); // 본인을 뺀경우
//	std::vector<cClientSession*> activePlayers = GetMaxBetPlayers( ownerPlayer->GetLostMoney() ); // 본인을 포함시키는 경우
//	if ( activePlayers.size() >= 2 ) {
//		if ( m_pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Chip ) {
//			std::sort( activePlayers.begin() , activePlayers.end() , &cHoldem::CompareMaxBetChip );
//			secondPlayerMaxMoney = activePlayers[ 1 ]->GetChip();
//		}
//		else if ( m_pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Coin ) {
//			std::vector<uint64> sortedMoney;
//			for ( auto player : activePlayers ) {
//				uint64 startMoney = player->GetCoin() + player->GetLostMoney();
//				if ( startMoney > GetMaxBetMoney() )
//					startMoney = GetMaxBetMoney();
//
//				// 아예 계산해서 넣자.
//				sortedMoney.push_back( startMoney - ownerPlayer->GetLostMoney() );
//			}
//
//			//std::sort( sortedMoney.begin() , sortedMoney.end());
//			std::sort( sortedMoney.begin() , sortedMoney.end() , std::greater<uint64>() );
//			secondPlayerMaxMoney = sortedMoney[ 1 ];
//		}
//	}
//	else
//	{
//		secondPlayerMaxMoney = 0; // 베팅 할 수 있는 금액이 없다.
//	}
//
//	maxMoneyOrder.push_back( secondPlayerMaxMoney );
//
//	// 세번째로 채널에 설정된 맥스 베팅
//	//uint64 channelMaxBet = GetMaxBetMoney() - player->GetLostMoney();
//	//maxMoneyOrder.push_back( channelMaxBet );
//	maxMoneyOrder.push_back( GetMaxBetMoney() );
//
//	std::sort( maxMoneyOrder.begin() , maxMoneyOrder.end() );
//
//	// 가장 적은 놈을 리턴해 준다.
//	return maxMoneyOrder[ 0 ];
//}

bool cHoldem::CompareMaxBetChip( cClientSession* player1 , cClientSession* player2 )
{
	return player1->GetChip() > player2->GetChip();
}

bool cHoldem::CompareMaxBetCoin( cClientSession* player1 , cClientSession* player2 )
{
	return player1->GetCoin() > player2->GetCoin();
}

void cHoldem::CreateStockers()
{
	// clear 만 하면서 계속 재활용을 하도록 한다.

	BETTING_ROUND_VEC* bet_1 = new BETTING_ROUND_VEC();
	BETTING_ROUND_VEC* bet_2 = new BETTING_ROUND_VEC();
	BETTING_ROUND_VEC* bet_3 = new BETTING_ROUND_VEC();
	BETTING_ROUND_VEC* bet_4 = new BETTING_ROUND_VEC();


	// 홀덤은 첫 베팅 부터 라운드 3까지 진행

	std::vector<General::TableAction> bet_seq;
	bet_1->insert( std::pair<int , std::vector<General::TableAction>>( 1 , bet_seq ) );
	bet_1->insert( std::pair<int , std::vector<General::TableAction>>( 2 , bet_seq ) );
	bet_1->insert( std::pair<int , std::vector<General::TableAction>>( 3 , bet_seq ) );
	m_bettingStocks.insert( std::pair<Server::PlayPhase , BETTING_ROUND_VEC*>( Server::PlayPhase::PlayPhase_FirstBet , bet_1 ) );

	bet_2->insert( std::pair<int , std::vector<General::TableAction>>( 1 , bet_seq ) );
	bet_2->insert( std::pair<int , std::vector<General::TableAction>>( 2 , bet_seq ) );
	bet_2->insert( std::pair<int , std::vector<General::TableAction>>( 3 , bet_seq ) );
	m_bettingStocks.insert( std::pair<Server::PlayPhase , BETTING_ROUND_VEC*>( Server::PlayPhase::PlayPhase_SecondBet , bet_2 ) );

	bet_3->insert( std::pair<int , std::vector<General::TableAction>>( 1 , bet_seq ) );
	bet_3->insert( std::pair<int , std::vector<General::TableAction>>( 2 , bet_seq ) );
	bet_3->insert( std::pair<int , std::vector<General::TableAction>>( 3 , bet_seq ) );
	m_bettingStocks.insert( std::pair<Server::PlayPhase , BETTING_ROUND_VEC*>( Server::PlayPhase::PlayPhase_ThirdBet , bet_3 ) );

	bet_4->insert( std::pair<int , std::vector<General::TableAction>>( 1 , bet_seq ) );
	bet_4->insert( std::pair<int , std::vector<General::TableAction>>( 2 , bet_seq ) );
	bet_4->insert( std::pair<int , std::vector<General::TableAction>>( 3 , bet_seq ) );
	m_bettingStocks.insert( std::pair<Server::PlayPhase , BETTING_ROUND_VEC*>( Server::PlayPhase::PlayPhase_FinalBet , bet_4 ) );
}

bool cHoldem::AutoCall()
{
	PmNet::MatchWagerRQ userBet;
	userBet.set_wager_kind( General::TableAction::TableAction_Call );

	UserBet( *m_curBetPlayerIter , userBet );

	//if ( cLowBadukiBettingChecker::IsStepFirstBet( m_gameStep ) )
	//{
	//	// 최초 베팅이면 Check 가능
	//	UserBet( *m_curBetPlayerIter , General::TableAction::TableAction_Check );
	//}
	//else
	//{
	//	UserBet( *m_curBetPlayerIter , General::TableAction::TableAction_Call );
	//}
	return true;
}

void cHoldem::SetTurnExpireAndSendPlayerTurn( uint64 waitMs )
{
	m_turnExpireTick = ::GetTickCount64() + waitMs;
	m_turnExpireTickMinimum = waitMs == 0 ? 0  : ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiTurnMinimumWait;
	if ( ( *m_curBetPlayerIter ) == nullptr )
		return;
	//m_turnExpireTickMinimum = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiTurnMinimumWait; // Turn 을 넘기기 위한 미니멈 시간 셋팅 ( 현재 1 초 )
	uint64 turnWho = 0;
	if( m_curBetPlayerIter != m_betSequence.end())
		turnWho = ( *m_curBetPlayerIter )->GetPlayerIdx();


	if ( false == ( *m_curBetPlayerIter )->isAllIn() && ( *m_curBetPlayerIter )->is_copyed == false ) {

		// Bet 할 플레이어 통보
		PmNet::PhaseTurnRS _res;
		_res.set_member_idx( turnWho );
		_res.set_remain_ms( waitMs );	// 현재 7초로 셋팅 20240318
		_res.set_match_phase( m_gameStep );		// 로우 바둑이 플레이 상태, Bet_1, Bet_2, Bet_3, Bet_4, Breakfast, Lunch,  Dinner
		_res.set_max_wager( maxbet );
		_res.set_member_wager( m_betting[ turnWho ] );
		auto betList = GetAvailableActionsForPlayer( *m_curBetPlayerIter );

		for ( auto t_betList : betList ) {
			_res.add_wager_options( t_betList );
		}
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_TurnNotice , _res );
	}

	std::string curStep = protoutil::cProtoUtil::GetEnumString( m_gameStep );
	std::string errorString = std::format( "[ PmNet::PhaseTurnRS {} ] Send RoomNumber [ {} ], PlayerIdx [ {} ]" , curStep.c_str() , m_pGameRoom->GetRoomNumber() , turnWho );
	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
	TraceA( errorString );
}

int cHoldem::GetMaxRaiseCountForCurrentRound()
{
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_HoldemPreFlop:
	case Server::PlayPhase::PlayPhase_FirstBet:
		return 2;
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
	case Server::PlayPhase::PlayPhase_FinalBet:
		return 3;
	default:
		return 0;
	}
}

bool cHoldem::CanPlayerRaise( cClientSession* player )
{
	if ( !player )
		return false;

	uint64 playerIdx = player->GetPlayerIdx();
	if ( m_raiseCount.find( playerIdx ) == m_raiseCount.end() )
		return true;
	int count = m_raiseCount[ playerIdx ];
	int maxCount = GetMaxRaiseCountForCurrentRound();

	return count < maxCount;
}

std::vector<General::TableAction> cHoldem::GetAvailableActionsForPlayer( cClientSession* player )
{
	std::vector<General::TableAction> actions;
	if ( !player ) return actions;

	const bool isChip = ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Chip );
	const uint64 channelMaxBet = m_maxBetMoney;   // 채널 맥스뱃

	const uint64 playerIdx = player->GetPlayerIdx();
	const uint64 playerMoney = GetPlayerMoney( player );
	const uint64 playerBet = m_betting[ playerIdx ];
	const uint64 toCall = ( maxbet > playerBet ) ? ( maxbet - playerBet ) : 0;

	const uint64 pot = m_pGameRoom->m_roomInfo.pot_amount();

	// 금액 계산(규칙서: C는 콜금액, P는 현재 팟)
	const uint64 quarterAmt = ( pot + toCall ) / 4;
	const uint64 halfAmt = ( pot + toCall ) / 2;
	const uint64 fullAmt = ( pot + toCall );

	// 맥스베팅 도달 여부 확인
	const bool hasReachedMaxBet = ( playerBet + toCall >= channelMaxBet );

	// 나머지 플레이어들의 상태 확인 (다이/맥스/올인 여부)
	auto checkOtherPlayersCanBet = [&]() -> bool {
		for ( const auto& [idx , _bet] : m_betting ) {
			if ( idx == playerIdx ) continue; // 자신 제외

			cClientSession* otherPlayer = FindPlayerByIdx( idx );
			if ( !otherPlayer ) continue;

			// 다이한 플레이어는 제외
			if ( otherPlayer->isDie() ) continue;

			// 올인한 플레이어는 제외
			if ( otherPlayer->isAllIn() ) continue;
			if ( 0 == GetPlayerMoney( otherPlayer ) ) continue;
			// 맥스베팅에 도달한 플레이어는 제외
			uint64 otherPlayerBet = m_betting[ idx ];
			if ( otherPlayerBet >= channelMaxBet ) continue;

			// 베팅 가능한 플레이어가 하나라도 있으면 true 반환
			return true;
		}
		return false; // 베팅 가능한 상대가 없음
	};

	const bool hasActiveBettingOpponents = checkOtherPlayersCanBet();

	// 다른 플레이어들이 콜할 수 있는 최대 금액 계산
	uint64 maxCallableByOthers = 0;
	for ( const auto& [idx, _bet] : m_betting ) {
		if ( idx == playerIdx ) continue; // 자신 제외

		cClientSession* otherPlayer = FindPlayerByIdx( idx );
		if ( !otherPlayer ) continue;

		// 다이한 플레이어는 제외
		if ( otherPlayer->isDie() ) continue;

		// 올인한 플레이어는 제외
		if ( otherPlayer->isAllIn() ) continue;

		uint64 otherPlayerBet = m_betting[idx];
		uint64 otherPlayerMoney = GetPlayerMoney( otherPlayer );

		// 맥스베팅에 도달한 플레이어는 제외
		if ( otherPlayerBet >= channelMaxBet ) continue;

		// 이 플레이어가 최대로 콜할 수 있는 금액 계산
		uint64 maxCallableAmount = std::min( otherPlayerMoney, channelMaxBet - otherPlayerBet );
		uint64 maxPossibleBet = otherPlayerBet + maxCallableAmount;

		maxCallableByOthers = std::max( maxCallableByOthers, maxPossibleBet );
	}

	// 현재 maxbet에서 다른 플레이어들이 콜할 수 있는 최대 레이즈 금액
	uint64 maxRaiseCallable = ( maxCallableByOthers > maxbet ) ? ( maxCallableByOthers - maxbet ) : 0;

	// 체크/콜 (맥스베팅 한도 확인)
	if ( toCall == 0  ) {
		actions.push_back( General::TableAction::TableAction_Check );
	}
	else if ( playerMoney > toCall && playerBet + toCall <= channelMaxBet ) {
		actions.push_back( General::TableAction::TableAction_Call );
	}else if( playerMoney  <= toCall )
		actions.push_back( General::TableAction::TableAction_AllIn );
	// 이미 맥스베팅에 도달했다면 추가 베팅 액션 불가
	if ( hasReachedMaxBet ) return actions;

	// 나머지 플레이어들이 모두 다이/맥스/올인 상태라면 추가 베팅 불가
	if ( !hasActiveBettingOpponents ) return actions;

	if ( player->isAllIn() ) return actions;

	// 레이즈 가능?
	if ( !CanPlayerRaise( player ) ) return actions;

	// --- 공통: 쿼터/하프/풀 (맥스베팅 한도 확인 + 콜 가능성 확인) ---
	bool shouldAddMax = false;

	if ( playerMoney >= toCall + quarterAmt && playerBet + toCall + quarterAmt <= channelMaxBet ) {
		// 다른 플레이어들이 쿼터 금액을 콜할 수 있는지 확인
		if ( quarterAmt <= maxRaiseCallable ) {
			actions.push_back( General::TableAction::TableAction_QuarterPot );
		}
		else {
			shouldAddMax = true;
		}
	}

	if ( playerMoney >= toCall + halfAmt && playerBet + toCall + halfAmt <= channelMaxBet ) {
		// 다른 플레이어들이 하프 금액을 콜할 수 있는지 확인
		if ( halfAmt <= maxRaiseCallable ) {
			actions.push_back( General::TableAction::TableAction_HalfPot );
		}
		else {
			shouldAddMax = true;
		}
	}

	if ( playerMoney >= toCall + fullAmt && playerBet + toCall + fullAmt <= channelMaxBet ) {
		// 다른 플레이어들이 풀 금액을 콜할 수 있는지 확인
		if ( fullAmt <= maxRaiseCallable ) {
			actions.push_back( General::TableAction::TableAction_FullPot );
		}
		else {
			shouldAddMax = true;
		}
	}

	// === 맥스/올인 로직 ===
	// shouldAddMax 플래그가 설정되었거나 원래 조건에 따라 Max/Allin 추가
	if ( shouldAddMax && playerBet + playerMoney > channelMaxBet ) {
		actions.push_back( General::TableAction::TableAction_Maximum );
	}
	else {
		// 올인 금액을 다른 플레이어들이 콜할 수 있는지 확인
		if ( playerMoney <= maxRaiseCallable + toCall || playerMoney <= toCall ) {
			actions.push_back( General::TableAction::TableAction_AllIn );
		}
		else {
			actions.push_back( General::TableAction::TableAction_Maximum );
		}
	}

	return actions;
}

void cHoldem::OnIntrudingSendTurnExpireAndSendPlayerTurn()
{
	uint64 remainTickCount = m_turnExpireTick - ::GetTickCount64();
	if ( ( *m_curBetPlayerIter ) == nullptr )
		return;
	uint64 turnWho = ( *m_curBetPlayerIter )->GetPlayerIdx();

	// Bet 할 플레이어 통보
	PmNet::PhaseTurnRS _res;
	_res.set_member_idx( turnWho );
	_res.set_remain_ms( remainTickCount );
	_res.set_match_phase( m_gameStep );
	_res.set_max_wager( maxbet );
	_res.set_member_wager( m_betting[ turnWho ] );

	auto betList = GetAvailableActionsForPlayer( *m_curBetPlayerIter );

	for ( auto t_betList : betList ) {
		_res.add_wager_options( t_betList );
	}

	m_pGameRoom->BroadCastToAllPlayer( General::Packet_TurnNotice , _res );
}

// 현재 베팅을 해야할 플레이어가 베팅을 했는지 확인한다.
bool cHoldem::CheckBet()
{
	if ( false == isMinumumTurnTimeOver() )
		return false;
	// 베팅할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
		break;
	default:
		ThrowGameStepException( "CheckBet" , m_gameStep );
	}
	if ( ( *m_curBetPlayerIter ) == nullptr )
		return true;
	// 현재 베팅할 유저
	cClientSession* pClientSession = ( *m_curBetPlayerIter );

	if ( pClientSession->is_copyed )
		return true;
	if ( pClientSession->isDie() ) // Die 유저면 끝
		return true;

	// 더 이상 베팅을 할 유저가 없으면 true 리턴
	if ( GetActivePlayerCount() == 0 )
		return true;

	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>* pBettinMap = pClientSession->GetBettings(); // nullptr 이 될 수 없다.
	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = pBettinMap->Lookup( m_gameStep );
	if ( pPair == nullptr )
		return false;

	std::vector<General::TableAction>* bettings = pPair->m_value;
	if ( bettings == nullptr ) // 아직 베팅 안함
		return false;

	// 스텝별로 따로 처리할것이 있는가?
	// 현재로써는 없어 보임
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
		break;
	}

	// 이번 라운드에 베팅을 했는가 확인, 1 ~ 3 회 이므로
	if ( bettings->size() < m_curBetRound )
	{
		// AllIn 인 유저면 Auto Call 작동
		if ( ( *m_curBetPlayerIter )->isAllIn() ) {
			AutoCall();
			pClientSession->InitNoActionCounter();
			return true;
		}
		return false;
	}

	pClientSession->InitNoActionCounter();
	// 현재 순서 array
	return true;
}

// 보스가 Die 이외의 베팅을 가지고 있으면 true 를 리턴한다.
// true 를 리턴 하면 보스가 교체가 된다.
bool cHoldem::HasBossAnyBetting()
{
	if ( m_dealerIter == m_playerSlots.end() )
		throw std::runtime_error( "cHoldem::HasBossAnyBetting m_dealerIter iterator is at the end" );

	cClientSession* pClientSession = ( *m_dealerIter );
	if ( pClientSession == nullptr )
		return true;

	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>* pBettinMap = pClientSession->GetBettings();
	if ( pBettinMap == nullptr )
		throw std::runtime_error( "cHoldem::HasBossAnyBetting pClientSession->GetBettings() Betting map is null" );

	std::vector<Server::PlayPhase> bets;
	bets.push_back( Server::PlayPhase::PlayPhase_FirstBet );
	bets.push_back( Server::PlayPhase::PlayPhase_SecondBet );
	bets.push_back( Server::PlayPhase::PlayPhase_ThirdBet );
	bets.push_back( Server::PlayPhase::PlayPhase_FinalBet );

	for ( auto& bet : bets ) {
		ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = pBettinMap->Lookup( bet );
		if ( pPair == nullptr )
			continue;

		std::vector<General::TableAction>* bettings = pPair->m_value;
		if ( bettings == nullptr )
			continue;

		for ( auto iter = bettings->begin(); iter != bettings->end(); ++iter ) {
			General::TableAction betting = *iter;
			if ( betting != General::TableAction::TableAction_GiveUp )
				return true;
		}
	}

	return false;
}

// 입력받은 플레이어 들이 Max Bet 만큼 이미 베팅을 했는지 여부 판별
bool cHoldem::AllPlayerBetMax( std::vector<cClientSession*>& activePlayers )
{
	// activeUsers 가 채널에 설정된 MaxBet 을 이미 사용했으면 ShowDown
	for ( auto player : activePlayers ) {
		if ( player == nullptr )
			continue;
		uint64 lostMoney = player->GetLostMoney();
		uint64 maxBetMoney = GetMaxBetMoney();

		if ( lostMoney < maxBetMoney )
			return false;
	}
	return true;
}

// 모든 플레이어가 Call 조차도 할 수 없는 상황
bool cHoldem::NoOneCanBet( std::vector<cClientSession*>& activePlayers )
{
	// 전체 다이 + 전체 올인 액티브 0 가능
	if ( activePlayers.size() == 0 )
		return true;
	//throw std::exception( "cHoldem::NoOneCanBet activeUsers size 0" );

// Call 비용을 계산한다. 현재 베팅 m_lastPlayerBetTotal
	for ( auto player : activePlayers ) {
		if ( player == nullptr )
			continue;
		uint64 curMoney = GetPlayerMoney( player );
		if ( curMoney == 0 )
			continue;

		if ( player->GetLostMoney() >= GetMaxBetMoney() )
			continue;

		return false;
		/*uint64 callCost = m_lastPlayerBetTotal - player->GetLostMoney();

		if ( curMoney >= callCost )
			return false;*/
	}
	return true;
}

// 현재 베팅 순서의 유저의 TimeOver 에 의한 Die 베팅 처리
// 베팅을 했으면 Die 처리 하지 않는다.
void cHoldem::DieCurPlayer()
{
	// 베팅할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
		break;
	default:
		break;//ThrowGameStepException( "DieCurPlayer" , m_gameStep );
	}
	if ( ( *m_curBetPlayerIter ) == nullptr )
		return;

	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = ( *m_curBetPlayerIter )->GetBettings()->Lookup( m_gameStep );
	if ( pPair == nullptr ) {
		std::vector<General::TableAction>* bettings = new std::vector<General::TableAction>();
		bettings->push_back( General::TableAction::TableAction_GiveUp );
		( *m_curBetPlayerIter )->GetBettings()->SetAt( m_gameStep , bettings );
	}
	else {
		pPair->m_value->push_back( General::TableAction::TableAction_GiveUp );
	}
	cClientSession* pClientSession_boss = GetPlayerSession(GetBossPlayerIdx()); //새로 추가
	General::AssetKind moneyType = m_pGameRoom->m_roomInfo.asset_kind();

	// 베팅 정보 전송
	PmNet::MatchWagerRS _res;
	_res.set_wager_member_idx( ( *m_curBetPlayerIter )->GetPlayerIdx() );
	_res.set_member_wager( General::TableAction::TableAction_GiveUp );
	_res.set_fund_kind( moneyType ); // Money 타입을 구분해야 하는데, 현재는 방타입으로 구분
	_res.set_fund_before( GetPlayerMoney( *m_curBetPlayerIter ) );
	_res.set_fund_after( GetPlayerMoney( *m_curBetPlayerIter ) );
	m_pGameRoom->BroadCastToAllPlayer( General::Packet_StakeSubmit , _res );

	// Die 처리
	( *m_curBetPlayerIter )->SetDie( true );
	( *m_curBetPlayerIter )->SetDelayTick( ::GetTickCount64() + m_diedelay );

	// 자동으로 나가기 예약
	if ( ( *m_curBetPlayerIter )->GetNoActionCounter() == 0 )
	{
		( *m_curBetPlayerIter )->IncreaseNoActionCounter();
	}
	else {
		if ( false == ( *m_curBetPlayerIter )->GetReserved() ) {

			( *m_curBetPlayerIter )->SetRoomOutReserve();

			if ( nullptr != *m_masterIter )
			{
				int code = 20404;
				auto log_holdem_exit = QueryManager::InsertGameExitLog(
					code ,
					m_channel.id() ,
					m_gameUid ,
					m_pGameRoom->m_roomInfo.played_rounds() + 1 ,
					"" ,
					( *m_masterIter )->GetPlatformGuid() ,
					p_id[ 0 ] , p_asset[ 0 ] , 0 ,
					p_id[ 1 ] , p_asset[ 1 ] , 0 ,
					p_id[ 2 ] , p_asset[ 2 ] , 0 ,
					p_id[ 3 ] , p_asset[ 3 ] , 0 ,
					p_id[ 4 ] , p_asset[ 4 ] , 0 ,
					p_id[ 5 ] , p_asset[ 5 ] ,
					p_id[ 6 ] , p_asset[ 6 ] ,
					p_id[ 7 ] , p_asset[ 7 ] ,
					p_id[ 8 ] , p_asset[ 8 ] ,
					( *m_curBetPlayerIter )->GetPlatformGuid() ,
					"roomid_" + std::to_string( m_pGameRoom->GetRoomNumber() ) , // room_id
					"TimeOver" ,
					""
				);

				log_holdem_exit.wait();
			}

			PmNet::ChamberLeaveHoldRS _outRes;
			_outRes.set_chamber_no( m_pGameRoom->GetRoomNumber() );
			_outRes.set_hold_ok( true );
			_outRes.set_member_idx( ( *m_curBetPlayerIter )->GetPlayerIdx() );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_LeaveReserve , _outRes );
		}
	}
}

std::vector<General::TableAction>* cHoldem::GetPlayerBettings( cClientSession* pClientSession )
{
	std::vector<General::TableAction>* bettings = nullptr;

	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>* pBettinMap = pClientSession->GetBettings(); // nullptr 이 될 수 없다.
	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = pBettinMap->Lookup( m_gameStep );

	// 검증이 된 상태에서 SetAt 하는 것이 맞긴 하지만 코드 간략화를 위해 이곳에서
	if ( pPair == nullptr ) {
		bettings = new std::vector<General::TableAction>();
		pBettinMap->SetAt( m_gameStep , bettings );
	}
	else {
		bettings = pPair->m_value;
	}
	return bettings;
}

std::vector<General::TableAction>* cHoldem::CreateStepPlayerBettings( cClientSession* pClientSession , Server::PlayPhase gameStep )
{
	std::vector<General::TableAction>* bettings = nullptr;
	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>* pBettinMap = pClientSession->GetBettings(); // nullptr 이 될 수 없다.
	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = pBettinMap->Lookup( gameStep );

	// 검증이 된 상태에서 SetAt 하는 것이 맞긴 하지만 코드 간략화를 위해 이곳에서
	if ( pPair == nullptr ) {
		bettings = new std::vector<General::TableAction>();
		pBettinMap->SetAt( gameStep , bettings );
	}
	else {
		bettings = pPair->m_value;
	}
	return bettings;
}

General::ResultCode cHoldem::MoreBetCheck( std::vector<General::TableAction>* bettings )
{
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
	{
		switch ( m_curBetRound )
		{
		case E_BET_ROUND::E_BET_ROUND_1:
		case E_BET_ROUND::E_BET_ROUND_2:
		case E_BET_ROUND::E_BET_ROUND_3:
		case E_BET_ROUND::E_BET_ROUND_4:
		{
			if ( bettings->size() > static_cast< int >( m_curBetRound ) - 1 )
				return General::ResultCode::Result_BetWindowClosed;
		}
		break;
		default:
			return General::ResultCode::Result_BetOptionRejected;
		}
	}
	break;
	default:
		return General::ResultCode::Result_PlayStepMismatch;
	}
	return General::ResultCode::Result_Success;
}

bool cHoldem::HasBetting( cClientSession* pClientSession )
{
	std::vector<General::TableAction>* bettings = GetPlayerBettings( pClientSession );
	if ( bettings == nullptr )
		return false;

	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_FirstBet:
	case Server::PlayPhase::PlayPhase_SecondBet:
	case Server::PlayPhase::PlayPhase_ThirdBet:
	case Server::PlayPhase::PlayPhase_FinalBet:
	{
		switch ( m_curBetRound )
		{
		case E_BET_ROUND::E_BET_ROUND_1:
		case E_BET_ROUND::E_BET_ROUND_2:
		case E_BET_ROUND::E_BET_ROUND_3:
		case E_BET_ROUND::E_BET_ROUND_4:
		{
			if ( bettings->size() > static_cast< int >( m_curBetRound ) - 1 )
				return true;
		}
		break;
		}
	}
	break;
	}
	return false;
}

// Call 을 한번이라도 했으면
// 이후 부터는 Call을 하지 못한다.
bool cHoldem::TryingCallTwice( std::vector<General::TableAction>* bettings , General::TableAction curBet )
{
	bool findCall = false;
	for ( auto iter = bettings->begin(); iter != bettings->end(); ++iter ) {
		auto& bet = *iter;
		if ( bet == General::TableAction::TableAction_Call ) {
			findCall = true;
			break;
		}
	}

	if ( findCall == true ) {
		if ( curBet == General::TableAction::TableAction_Call )
			return true;
	}

	return false;
}

bool cHoldem::isBoss( const uint64 playerIdx )
{
	if ( m_dealerIter == m_playerSlots.end() )
		return false;

	if ( *m_dealerIter == nullptr )
		return false;

	return ( *m_dealerIter )->GetPlayerIdx() == playerIdx;
}

General::ResultCode cHoldem::CaclBet( cClientSession* pClientSession , General::TableAction curBetting , BOOL& allin , BOOL& side )
{
	allin = side = FALSE;

	if ( pClientSession->isDie() )
		return General::ResultCode::Result_FoldedPlayerActionRejected;

	std::vector<General::TableAction>* bettings = GetPlayerBettings( pClientSession );

	if ( General::ResultCode::Result_Success != MoreBetCheck( bettings ) )
		return MoreBetCheck( bettings );

	// 홀덤은 Call 계속 할 수 있다.
	//if ( TryingCallTwice( bettings , curBetting ) )
	//	return General::ResultCode::Result_ActionRejected;

	uint64 playerIdx = pClientSession->GetPlayerIdx();
	uint64 curPot = m_pGameRoom->m_roomInfo.pot_amount();
	uint64 beforeBet = m_pGameRoom->m_roomInfo.previous_wager();
	uint64 betMoney = 0;

	if ( General::PlayCategory::PlayCategory_TexasHoldem == m_pGameRoom->m_roomInfo.play_category() )
	{
		switch ( curBetting )
		{
		case General::TableAction::TableAction_GiveUp:
		{
			// DIE 처리 필요, 플레이 종료
			pClientSession->SetDie( true );
			pClientSession->SetDelayTick( ::GetTickCount64() + m_diedelay );
		}
		break;
		// 베팅한 금액이 없을때만 가능, 카드 교환 1회당 1번만 가능
		case General::TableAction::TableAction_SeedOnly:
		{
			if ( m_stepPlayerBet != 0 )
				return General::ResultCode::Result_ActionRejected;

			if ( cBettingChecker::StepBbingCheck( m_gameStep ) )
				return General::ResultCode::Result_BbingAlreadyUsed;

			betMoney = m_pGameRoom->m_roomInfo.seed_amount();
		}
		break;
		case General::TableAction::TableAction_DoubleRaise: // 앞 사람이 베팅한 금액의 2배를 베팅함 (CALL*2)
		{
			if ( pClientSession->isAllIn() )
				return General::ResultCode::Result_ActionRejected;

			// 앞전에 베팅 금액이 없으면 실패 처리
			if ( beforeBet <= 0 )
				return General::ResultCode::Result_ActionRejected;

			beforeBet = m_lastPlayerBetTotal - pClientSession->GetLostMoney();
			betMoney = beforeBet * 2;
			uint64 maxBetMoney = GetHoldemMaxBet( pClientSession );

			// 앞 사람의 베팅을 받고 싶어도 못 받는 경우 처리
			if ( maxBetMoney < betMoney )
				betMoney = maxBetMoney;
		}
		break;
		case General::TableAction::TableAction_Check: // 베팅하지 않고, 다음 플레이어 베팅 순으로 넘김(0)
		{
			// 레드마인 개선 #254 프리플랍 시점에 베팅한 플레이어가 없는 경우 버튼 출력 개선요청의 건
			//조건1.( 대상 ) 현재 프리플랍 시점에서 "BB" or "GB" or "RB" 베팅 플레이어
			//조건2.( 베팅 ) "BB" or "GB" or "RB" 베팅 플레이어의 턴 발생 전까지 베팅한 플레이어가 없는 경우
			if ( Server::PlayPhase::PlayPhase_FirstBet == m_gameStep ) {

				// Check 가 허용 되는 경우
				//if ( m_blindBet == m_betting[playerIdx ] ) {
				if ( maxbet == m_betting[playerIdx ] ) {

				}
				else {
					if ( beforeBet > 0 )
						return General::ResultCode::Result_ActionRejected;
				}
			}
			else
			{
				// 최초베팅일 경우에만 가능
				// 앞사람의 베팅 금액이 없을때만 가능. ( 이걸로 결정 )
				if ( beforeBet > 0 )
					return General::ResultCode::Result_ActionRejected;
			}

		}
		break;
		case General::TableAction::TableAction_Call: // 앞사람의 베팅 금액과 동일한 금액 베팅( CALL )
		{
			// 이번 판에 베팅한 금액이 있을때만 Call 을 받는다.
			if ( m_stepPlayerBet != 0 )
				beforeBet = m_lastPlayerBetTotal - ( pClientSession->isAllIn() == false ? pClientSession->GetLostMoney() : pClientSession->GetVirtualLostMoney() );

			betMoney = beforeBet;
			//uint64 maxBetMoney = GetHoldemMaxBet( pClientSession );

			// 앞 사람의 베팅을 받고 싶어도 못 받는 경우 처리
			//if ( maxBetMoney < betMoney )
			//	betMoney = maxBetMoney;
			if ( pClientSession->isAllIn() && betMoney > 0 ) {
				pClientSession->PlusVirtualLostMoney( betMoney );
			}
			if ( pClientSession->isAllIn()&& maxbet > pClientSession->GetLostMoney() )
			{
				side = TRUE;
			}

			// Allin 인데 콜을 받았다. 가상 베팅 금액을 증가시켜 준다.
			//if ( pClientSession->isAllIn() && betMoney > 0 ) {

			//	pClientSession->PlusVirtualLostMoney( betMoney );

			//	if ( false == pClientSession->isSendSide() ) {

			//		// SIDE 가 발생하였다.
			//		side = TRUE;
			//		pClientSession->SetSendSide();
			//		//cSidePotManager::CreateSidePot( pClientSession->GetPlayerIdx() , pClientSession->GetLostMoney() );
			//	}
			//}
			// 베팅금액이 더 이상 없으면 베팅을 넘긴다.
			if ( betMoney <= 0 ) {

				bettings->push_back( curBetting );
				cBettingChecker::Bet( m_gameStep , m_curBetRound , curBetting );

#ifdef _DEBUG
				// BET 로그 추가
				std::string nick = pClientSession->GetNickName();
				uint64 beforeMoney = GetPlayerMoney( pClientSession ) + betMoney;
				uint64 curMoney = GetPlayerMoney( pClientSession );

				General::AssetKind moneyType = m_pGameRoom->m_roomInfo.asset_kind();
				std::string moneyTypeString = protoutil::cProtoUtil::GetEnumString( moneyType );
				std::string curStep = protoutil::cProtoUtil::GetEnumString( m_gameStep );
				std::string betString = protoutil::cProtoUtil::GetEnumString( curBetting );
				// Auto Call 용 로그 출력
				betString = "Auto Call";
				std::string log = std::format( "Nick: {}, Betting: {}, Room Status: {}, Room Number: {}, MoneyType: {} [before {}, current {}]" ,
					nick.c_str() , betString.c_str() , curStep.c_str() , m_pGameRoom->GetRoomNumber() , moneyTypeString , beforeMoney , curMoney );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );

				// Step, Round, Nick, 소모 칩량, Pot
				log = std::format( "Room Status: {}, Round: {}, Nick: {}, UserIdx: {}, UsedMoney: {}, Betting: {}, BetAmount: {}, Pot: {}" ,
					curStep.c_str() , ( int ) m_curBetRound , nick.c_str() , pClientSession->GetPlayerIdx() , pClientSession->GetLostMoney() , betString.c_str() , betMoney , m_pGameRoom->m_roomInfo.pot_amount() );
				TraceA( log.c_str() );
				InsertBetLog( log );
#endif
				return General::ResultCode::Result_Success;
			}

		}
		break;
		case General::TableAction::TableAction_QuarterPot: // 전체 판돈의 25%를 베팅 (POT+CALL)*1/4+CALL
		{
			if ( pClientSession->isAllIn() )
				return General::ResultCode::Result_ActionRejected;

			if ( m_lastPlayerBetTotal != 0 )
				beforeBet = m_lastPlayerBetTotal - pClientSession->GetLostMoney();

			betMoney = ( ( curPot + beforeBet ) / 4 ) + beforeBet;
			uint64 maxBetMoney = GetHoldemMaxBet( pClientSession );

			// 앞 사람의 베팅을 받고 싶어도 못 받는 경우 처리
			if ( maxBetMoney < betMoney )
				betMoney = maxBetMoney;
		}
		break;
		case General::TableAction::TableAction_HalfPot: // 전체 판돈의 50%를 베팅 (POT+CALL)*1/2+CALL
		{
			if ( pClientSession->isAllIn() )
				return General::ResultCode::Result_ActionRejected;

			if ( m_lastPlayerBetTotal != 0 )
				beforeBet = m_lastPlayerBetTotal - pClientSession->GetLostMoney();

			betMoney = ( ( curPot + beforeBet ) / 2 ) + beforeBet;
			uint64 maxBetMoney = GetHoldemMaxBet( pClientSession );

			// 앞 사람의 베팅을 받고 싶어도 못 받는 경우 처리
			if ( maxBetMoney < betMoney )
				betMoney = maxBetMoney;
		}
		break;
		case General::TableAction::TableAction_FullPot: // 전체 판돈의 100%를 베팅 (POT+CALL) +CALL
		{
			if ( pClientSession->isAllIn() )
				return General::ResultCode::Result_ActionRejected;

			if ( m_lastPlayerBetTotal != 0 )
				beforeBet = m_lastPlayerBetTotal - pClientSession->GetLostMoney();

			betMoney = ( curPot + beforeBet ) + beforeBet;
			uint64 maxBetMoney = GetHoldemMaxBet( pClientSession );

			// 앞 사람의 베팅을 받고 싶어도 못 받는 경우 처리
			if ( maxBetMoney < betMoney )
				betMoney = maxBetMoney;
		}
		break;
		case General::TableAction::TableAction_Maximum: // 플레이어의 베팅 한도 내 최대 베팅, 채널별 베팅한도 최대 금액, 돈이 모자라면 ALLIN
		{
			if ( pClientSession->isAllIn() )
				return General::ResultCode::Result_ActionRejected;

			if ( m_lastPlayerBetTotal != 0 )
				beforeBet = m_lastPlayerBetTotal - pClientSession->GetLostMoney();

			// Max bet 로직
			betMoney = GetHoldemMaxBet( pClientSession );

			// 현재 베팅된 금액을 따라 가야 한다.
			// 현재 맥스베팅 로직상 플레이어가 가진 돈 이상으로 나올 수 없다.
			const uint64 possiblePlayerBet = betMoney + pClientSession->GetLostMoney();

			// 맥스 베팅이 플레이어가 베팅 할수 있는 최대 금액 보다 큰 경우, 가상으로 베팅을 처리한다.
			if ( m_lastPlayerBetTotal > possiblePlayerBet )
				betMoney = m_lastPlayerBetTotal - pClientSession->GetLostMoney();
		}
		break;
		case General::TableAction::TableAction_AllIn:
		{
			if ( pClientSession->isAllIn() )
				return General::ResultCode::Result_ActionRejected;

			if ( m_lastPlayerBetTotal != 0 )
				beforeBet = m_lastPlayerBetTotal - pClientSession->GetLostMoney();

			// Max bet 로직
			betMoney = GetHoldemMaxBet( pClientSession ); // 97412041
			if ( betMoney == 0 ) // 0 이 나오는 상황이면 플레이어들중에 2번째로 작은 금액을 결정할수 없는 상황이다.
				betMoney = beforeBet;

			// 현재 베팅된 금액을 따라 가야 한다.
			const uint64 possiblePlayerBet = betMoney + pClientSession->GetLostMoney();

			// 맥스 베팅이 플레이어가 베팅 할수 있는 최대 금액 보다 큰 경우, 가상으로 베팅을 처리한다.
			// 현재 맥스베팅 로직상 플레이어가 가진 돈 이상으로 나올 수 없다.
			if ( m_lastPlayerBetTotal > possiblePlayerBet )
				betMoney = m_lastPlayerBetTotal - pClientSession->GetLostMoney();
		}
		break;
		}

		// Die 가 아닌 경우에만 처리
		if ( curBetting != General::TableAction::TableAction_GiveUp && curBetting != General::TableAction::TableAction_Check )
		{
			if ( MinusPlayerMoney( pClientSession , betMoney ) ) {
				roundbet += betMoney;
				m_pGameRoom->m_roomInfo.set_pot_amount( curPot + betMoney );
				m_pGameRoom->m_roomInfo.set_previous_wager( betMoney );
				m_lastPlayerBetTotal = pClientSession->GetLostMoney(); // 이번 베팅에 건 칩을 저장 시켜 준다.

				if ( curBetting != General::TableAction::TableAction_Check && betMoney != 0 )
					m_stepPlayerBet = pClientSession->GetLostMoney(); // Check가 아닐때만 갱신한다.

				// 남은 Money 가 없으면 올인이다.
				uint64 remainMoney = GetPlayerMoney( pClientSession );
				if ( remainMoney == 0 ) {
					pClientSession->SetAllIn();
					allin = TRUE;
					//cSidePotManager::CreateSidePot( pClientSession->GetPlayerIdx() , pClientSession->GetLostMoney() );
				}
				//else
				//{
				//	// AllIn이 발생하지 않았을 경우에만 갱신시켜 준다.
				//	m_lastPlayerBetTotal = pClientSession->GetLostChip(); // 이번 베팅에 건 칩을 저장 시켜 준다.
				//}
			}
			else
			{
				// 올인 유저의 금액이 갱신 안되도록 수정해봄
				if ( false == pClientSession->isAllIn() )
				{
					// SIDE MONEY가 발생되었다.
					// 플레이어의 Money 한도만큼만 처리한다.
					uint64 remainMoney = GetPlayerMoney( pClientSession );
					uint64 virtualMoney = betMoney; // 가상으로 받은 금액
					betMoney = remainMoney;
					MinusPlayerMoney( pClientSession , betMoney );

					uint64 curLostMoney = pClientSession->GetLostMoney();	// 현재까지 플레이어가 베팅한 금액

					m_pGameRoom->m_roomInfo.set_pot_amount( curPot + betMoney );// POT 은 플레이어의 남은 금액만 추가 시켜 준다.

					if ( curLostMoney > m_lastPlayerBetTotal ) {
						m_pGameRoom->m_roomInfo.set_previous_wager( betMoney );	// 이전 베팅 금액은 플레이어가 베팅한 금액으로 처리
						m_lastPlayerBetTotal = curLostMoney;				// 베팅을 한 금액으로 갱신한다. ( 돈이 모자라지만

						if ( curBetting != General::TableAction::TableAction_Check && betMoney != 0 )
							m_stepPlayerBet = pClientSession->GetLostMoney();	// Check가 아닐때만 갱신한다.
					}

					// 가상으로 받은 금액은 갱신해 준다.
					pClientSession->PlusVirtualLostMoney( virtualMoney - remainMoney );

					// SIDE 상태로 만들고, SIDE POT을 만든다.
					pClientSession->SetAllIn();
					pClientSession->SetSide();
					allin = TRUE;

					if ( curLostMoney < m_lastPlayerBetTotal ) {
						if ( false == pClientSession->isSendSide() ) {

							// SIDE 가 발생하였다.
							side = TRUE;
							pClientSession->SetSendSide();
							//cSidePotManager::CreateSidePot( pClientSession->GetPlayerIdx() , pClientSession->GetLostMoney() );
						}
					}
				}
				else
				{
					// 이미 올인인 상황
					// m_lastPlayerBetTotal 을 변경하지 않는다. 자신은 베팅을 못함으로
					betMoney = 0;
				}
			}
		}
		m_raiseCount[ playerIdx ]++;
		m_betting[ playerIdx ] += betMoney;
		if ( maxbet < m_betting[ playerIdx ] )
			maxbet = m_betting[ playerIdx ];
		bettings->push_back( curBetting );
		cBettingChecker::Bet( m_gameStep , m_curBetRound , curBetting );

#ifdef _DEBUG
		// BET 로그 추가
		std::string nick = pClientSession->GetNickName();
		uint64 beforeMoney = GetPlayerMoney( pClientSession ) + betMoney;
		uint64 curMoney = GetPlayerMoney( pClientSession );

		General::AssetKind moneyType = m_pGameRoom->m_roomInfo.asset_kind();
		std::string moneyTypeString = protoutil::cProtoUtil::GetEnumString( moneyType );
		std::string curStep = protoutil::cProtoUtil::GetEnumString( m_gameStep );
		std::string betString = protoutil::cProtoUtil::GetEnumString( curBetting );
		std::string log = std::format( "Nick: {}, Betting: {}, Room Status: {}, Room Number: {}, MoneyType: {} [before {}, current {}]" ,
			nick.c_str() , betString.c_str() , curStep.c_str() , m_pGameRoom->GetRoomNumber() , moneyTypeString , beforeMoney , curMoney );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );

		// Step, Round, Nick, 소모 칩량, Pot
		log = std::format( "Room Status: {}, Round: {}, Nick: {}, UserIdx: {}, UsedChip: {}, Betting: {}, BetAmount: {}, Pot: {}" ,
			curStep.c_str() , ( int ) m_curBetRound , nick.c_str() , pClientSession->GetPlayerIdx() , pClientSession->GetLostMoney() , betString.c_str() , betMoney , m_pGameRoom->m_roomInfo.pot_amount() );
		TraceA( log.c_str() );
		InsertBetLog( log );
#endif

		return General::ResultCode::Result_Success;
	}

	return General::ResultCode::Result_ActionRejected;
}

void cHoldem::ReChargeDeck()
{
	// m_deck의 카드가 완전히 소진된 경우
	// 저장해 두었던 StockDeck으로 m_deck을 다시 구성합니다. Shuffle 도 해야 합니다.
	if ( m_deck.empty() ) {
		m_deck = m_stockDeck;
		m_stockDeck.clear();
		m_deck = NetLib::cSingleton<cCardDeck>::GetInstance()->Shuffle( m_deck );
		DebugCardCheckVec( m_deck );
	}
}

// Card Management
bool cHoldem::NextCardChange()
{
	// 베팅할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
		break;
	default:
		ThrowGameStepException( "NextCardChange" , m_gameStep );
	}

	// 다음 카드 교체를 진행할 유저가 있는가??
	std::vector<cClientSession*>::iterator iterNext = GetNextBetPlayer();

	// 다음 플레이어가 없다.
	if ( iterNext == m_betSequence.end() )
		return false;
	else
		m_curBetPlayerIter = iterNext; // 다음 플레이어로 진행

	if( (*m_curBetPlayerIter)->isAllIn() )
		SetTurnExpireAndSendPlayerTurn( 0 );
	else
		SetTurnExpireAndSendPlayerTurn( NetLib::cSingleton<cDataLoader>::GetInstance()->CardChangeWait );

	return true;
}

bool cHoldem::CheckCardChange()
{
	if ( false == isMinumumTurnTimeOver() )
		return false;

	// 베팅교체 할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
		break;
	default:
		ThrowGameStepException( "CheckCardChange" , m_gameStep );
	}

	// 현재 교채할 유저
	cClientSession* pClientSession = ( *m_curBetPlayerIter );
	if ( pClientSession == nullptr )
		return false;

	if ( pClientSession->isDie() ) {
		// Die 유저면 끝 이긴 하지만, 베팅할 유저로 선택된 것은 문제이기 때문에 throw 처리
		std::string curStep = protoutil::cProtoUtil::GetEnumString( m_gameStep );
		std::string errorInfo = std::format( "GameStep {} CheckCardChange call violation" , curStep.c_str() );
		// 라이브 서버에서는 로그만 남기고 안전하게 처리
		 NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );
		return false; // 에러 코드로 안전하게 반환
	}

	// 정황상 Exception 이 발생 될 수 있으나, Exception 이 발생되는 상황이면 로직에 문제가 발생되는 경우이기 때문에 이런 형태를 취함
	// TODO: ( *m_curBetPlayerIter )->GetBettings() 가 비어 있어서 터지는 현상 발견
	if ( ( *m_curBetPlayerIter )->GetBettings()->Lookup( m_gameStep )->m_value->size() > 0 )
		return true;

	return false;
}

// Die 를 Pass 로 처리한다.
void cHoldem::PassCurPlayer()
{
	// 베팅할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
		break;
	default:
		ThrowGameStepException( "PassCurPlayer" , m_gameStep );
	}

	if ( ( *m_curBetPlayerIter ) == nullptr )
		return;

	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = ( *m_curBetPlayerIter )->GetBettings()->Lookup( m_gameStep );
	if ( pPair == nullptr ) {
		std::vector<General::TableAction>* bettings = new std::vector<General::TableAction>();
		bettings->push_back( General::TableAction::TableAction_GiveUp );
		( *m_curBetPlayerIter )->GetBettings()->SetAt( m_gameStep , bettings );
	}
	else {
		pPair->m_value->push_back( General::TableAction::TableAction_GiveUp );
	}

	auto player = ( *m_curBetPlayerIter );
	if ( player == nullptr )
		return;
}

bool cHoldem::UserRemoveCard( cClientSession* pClientSession , const General::PlayingCard& remove_card )
{
	google::protobuf::RepeatedField<General::PlayingCard>::iterator iter;
	google::protobuf::RepeatedField<General::PlayingCard>* player_cards = pClientSession->GetCards();

	for ( iter = player_cards->begin(); iter != player_cards->end(); ++iter )
	{
		General::PlayingCard card = *iter;
		if ( card.suit_code() == remove_card.suit_code() && card.rank_code() == remove_card.rank_code() ) {
			player_cards->erase( iter );
			m_stockDeck.push_back( card ); // 재발급 받는 카드를 저장해 두어야 합니다.
			return true;
		}
	}
	return false;
}

// 서버상에서는 Bet1, 2, 3, 4 상태에서 상태가 변경되는 순간에 호출된다.
int cHoldem::GetCommunityCard()
{
	// 베팅할때 외에는 이 함수는 호출될 이유가 없음
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_HoldemFlop:
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
		break;
	default:
		ThrowGameStepException( "GetCommunityCard" , m_gameStep );
	}

	int cardCount = 0;
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_HoldemFlop:
		cardCount = 3;
		break;
	case Server::PlayPhase::PlayPhase_HoldemTurn:
	case Server::PlayPhase::PlayPhase_HoldemRiver:
		cardCount = 1;
		break;
	}

	PmNet::SharedTileRS _res;

	for ( int n = 0; n < cardCount; ++n ) {
		std::vector<General::PlayingCard>::iterator iter = m_deck.begin();
		General::PlayingCard& newCard = *iter;

#ifdef _DEBUG
		// QA 커뮤니티 카드 셋팅 된경우 변경
		auto qa_iter = m_qa_community_deck.begin();
		if ( qa_iter != m_qa_community_deck.end() ) {
			newCard = *qa_iter;
			m_qa_community_deck.erase( qa_iter );
		}
#endif
		m_communityCards.push_back( newCard ); // 플레이어 카드 추가

		auto new_community_card = _res.add_fresh_shared_cards();
		new_community_card->CopyFrom( newCard );

		m_deck.erase( iter ); // 덱에서 삭제
	}

	for ( const auto& community_card : m_communityCards ) {
		auto before_community_card = _res.add_shared_cards();
		before_community_card->CopyFrom( community_card );
	}

	_res.set_match_phase( m_gameStep );
	m_pGameRoom->BroadCastToAllPlayer( General::Packet_BoardCardReveal , _res ,0,false);

	// 교환한 카드 갯수를 리턴해 준다.
	return cardCount;
}

void cHoldem::Shuffle()
{
	// 카드 셔플
	m_deck.clear();
	m_deck = NetLib::cSingleton<cCardDeck>::GetInstance()->Shuffle();
	DebugCardCheckVec( m_deck );

	// 저장카드 초기화
	m_stockDeck.clear();

	// 커뮤니티 카드 초기화
	m_communityCards.clear();
}

// 방에 진입과, 나가기는 방이 대기중일 때만 가능한다.
General::ResultCode cHoldem::EnterSlot( cClientSession* pClientSession , const int slotNumber )
{
	if ( pClientSession == nullptr )
		return General::ResultCode::Result_NullParameterFault;
	//assert( pClientSession != nullptr , "cHoldem EnterSlot Failed" );

	// 강제 퇴장 기록 확인
	if ( isJoinProhibited( pClientSession->GetPlayerIdx() ) )
		return General::ResultCode::Result_RoomEntryKickBlocked;

	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->isban( pClientSession->GetPlayerIdx() ) )
	{
		return General::ResultCode::Result_ActionRejected;
	}
	// TODO 재화 타입에 맞는 재화량 확인이 필요
	// 바두기 룸에 입장하기 위한 재화가 충분한지 확인
	General::RoomAccessMode roomType = m_pGameRoom->m_roomInfo.access_mode();
	uint64 seedMoney = m_pGameRoom->m_roomInfo.seed_amount();
	if ( seedMoney > pClientSession->GetMoney( m_pGameRoom->m_roomInfo.asset_kind() ))
		return General::ResultCode::Result_BalanceInsufficient;

	int memberCount = GetMemberCnt();

	// Wait 상태이고 멤버가 2명일 때만 처리 합니다.
	int startMemberCount = GetMinStartPlayerCount();

	if ( slotNumber == 0 ) {

		// 플레이어 카운트 해서 2명이상이 되면 1.5초 텀을 주고 시작 할 수있도록 해준다.
		for ( int slot = 0; slot < m_maxPlayerCnt; ++slot )
		{
			if ( m_playerSlots[ slot ] == nullptr && m_slotReservation[ slot ] == nullptr )
			{
				m_playerSlots[ slot ] = pClientSession;
				++memberCount;

				if ( m_gameStep == Server::PlayPhase::PlayPhase_Waiting && memberCount == startMemberCount ) {
					// 방장이 시작 버튼을 활성화 해주기 까지 LowBaduki_Wait 시간만큼 기다립니다.
					// 시간이 다 되면 Server::PlayPhase::PlayPhase_StartReady 상태가 될 것이다.
					m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;
				}

				// Wait 또는 StartWait 상태에서 5명이 되면 2초 후에 자동시작이 되도록 설정한다.
				MakeRoomAutoStart( memberCount );

				// 방이동 카운트 초기화
				pClientSession->InitMoveSlotCount();

				return General::ResultCode::Result_Success;
			}
		}
	}
	else
	{

		// 슬롯을 정해서 들어왔다.
		if ( m_playerSlots[ slotNumber - 1 ] != nullptr )
			return General::ResultCode::Result_SeatTargetOccupied;

		m_playerSlots[ slotNumber - 1 ] = pClientSession;
		++memberCount;

		if ( m_gameStep == Server::PlayPhase::PlayPhase_Waiting && memberCount == startMemberCount ) {
			// 방장이 시작 버튼을 활성화 해주기 까지 LowBaduki_Wait 시간만큼 기다립니다.
			// 시간이 다 되면 Server::PlayPhase::PlayPhase_StartReady 상태가 될 것이다.
			m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;
		}

		// Wait 또는 StartWait 상태에서 5명이 되면 2초 후에 자동시작이 되도록 설정한다.
		MakeRoomAutoStart( memberCount );

		return General::ResultCode::Result_Success;
	}

	return General::ResultCode::Result_RoomEntryFailed; // 일반적인 로우 바둑이 방 조인 실패
}

bool cHoldem::IsAutoStartable( const int& memberCount )
{
	return ( m_gameStep == Server::PlayPhase::PlayPhase_Waiting || m_gameStep == Server::PlayPhase::PlayPhase_StartReady ) && (memberCount == m_pGameRoom->GetMaxRoomPlayerCnt()|| memberCount >= GetAutoStartCnt() );
}

void cHoldem::CalcResult()
{
	participatelock = true;
	// 시작 시간 찍기
	auto now = std::chrono::system_clock::now();
	auto epoch = now.time_since_epoch();
	timeStamp = std::chrono::duration_cast< std::chrono::seconds >( epoch ).count() - timeStamp;

	std::vector<PmNet::MemberOutcome> playersResults;
	std::deque<PmNet::MemberOutcome> losers;
	std::vector<PmNet::MemberOutcome> copiedResult;
	std::vector<std::pair< uint64 , uint64>> player_lostMoeny;
	// 플레이어의 액션이 처리 되지 않은 홀덤의 경우 생성되지 않은 사이드 팟을 생성한다.
	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;
		player_lostMoeny.push_back( std::make_pair( player->GetPlayerIdx() , player->GetLostMoney() ) );
	}
	std::sort( player_lostMoeny.begin() , player_lostMoeny.end() ,
			 []( const std::pair<uint64 , uint64>& a , const std::pair<uint64 , uint64>& b ) {
		return a.second > b.second;
	} );

	for(auto temp : player_lostMoeny )
		cSidePotManager::CreateSidePot( temp.first , temp.second);

	// 플레이 유저들의 카드 정렬kicker.GetPoint()
	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		// 사이드팟이 존재 한다면 사이드 금액을 셋팅한다.
		cSidePotManager::PushSidePotOnResult( player->GetPlayerIdx() , player->GetLostMoney() );

		// 딜러비를 물지 않는 금액을 셋팅하고 시작
		uint64 lostMoney = player->GetLostMoney();
		player->SetFreeDealerFeeMoney( lostMoney );

		PmNet::MemberOutcome playersResult;

		//나간놈 더미
		if ( player->is_copyed||m_pGameRoom->isWatcher( ( player )->GetPlayerIdx() ) )
		{
			playersResult.set_alias_label( player->GetPlayer().display_name() );
			playersResult.set_member_idx( player->GetPlayerIdx() );
			playersResult.set_hand_rank( General::HandRank::HandRank_None );
			General::ParticipantProfile* pPlayer = playersResult.mutable_member_info();
			player->CopyPlayer( pPlayer );
			playersResult.set_fund_before( GetPlayerMoney( player ) + player->GetLostMoney() );
			playersResult.set_svr_fund_after( GetPlayerMoney( player ) );
			playersResult.set_fund_after( GetPlayerMoney( player ) );
			losers.push_back( playersResult );
			copiedResult.push_back( playersResult );
			continue;
		}

		google::protobuf::RepeatedField<General::PlayingCard> cards = player->GetCardsClone();
		/*if ( cards.size() == 0 )
		{
			playersResult.set_alias_label( player->GetPlayer().display_name() );
			playersResult.set_member_idx( player->GetPlayerIdx() );
			playersResult.set_hand_rank( General::HandRank::HandRank_None );
			General::ParticipantProfile* pPlayer = playersResult.mutable_member_info();
			player->CopyPlayer( pPlayer );
			playersResult.set_fund_before( GetPlayerMoney( player ) + player->GetLostMoney() );
			playersResult.set_svr_fund_after( GetPlayerMoney( player ) );
			playersResult.set_fund_after( GetPlayerMoney( player ) );

			losers.push_back( playersResult );
			copiedResult.push_back( playersResult );
			std::string errorInfo = std::format( "Error:CardZero room:{}, player:{}" , m_pGameRoom->GetRoomNumber() , player->GetPlayerIdx());
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );

			continue;
		}*/
		// 핸드 카드 복사
		for ( const auto& card : cards ) {
			auto hand_card = playersResult.add_hand_tiles();
			hand_card->CopyFrom( card );
		}

		// 커뮤니티 카드 복사
		for ( const auto& card : m_communityCards ) {
			cards.Add( card );
		}

#ifdef _DEBUG

		General::PlayingCard card1;
		General::PlayingCard card2;
		General::PlayingCard card3;
		General::PlayingCard card4;
		General::PlayingCard card5;

		if ( m_communityCards.size() == 5 )
		{
			card1 = m_communityCards.at( 0 );
			card2 = m_communityCards.at( 1 );
			card3 = m_communityCards.at( 2 );
			card4 = m_communityCards.at( 3 );
			card5 = m_communityCards.at( 4 );
		}

#endif

		std::vector<General::PlayingCard> jokboCards;
		General::HandRank jokbo = GetJokbo( cards , jokboCards );
		playersResult.set_alias_label( player->GetNickName() );
		playersResult.set_member_idx( player->GetPlayerIdx() );
		//uint64 playerMoney = GetPlayerMoney( player );
		//uint64 lostMoney = player->GetLostMoney();
		playersResult.set_fund_before( GetPlayerMoney( player ) + player->GetLostMoney() ); // 현재칩 + 사용된 칩 누적
		playersResult.set_svr_fund_after( GetPlayerMoney( player ) );								// 미리 처리 해둔다.
		playersResult.set_hand_rank( jokbo );

		// 족보 처리를 위해 사용된 카드
		for ( auto card : jokboCards ) {
			General::PlayingCard* pCard = playersResult.add_cards();
			pCard->CopyFrom( card );
		}

		// 족보 카드에 대한 점수 생성
		std::vector<General::PlayingCard> sortedJokboCards = SortJokboCards( playersResult.cards() , jokbo );
		cHoldemSameJokboCompare compareJokbo = cHoldemSameJokboCompare( playersResult , sortedJokboCards );
		playersResult.set_hand_rank_score( compareJokbo.GetPoint() );

		// 키커 카드 미리 처리한다.
		std::vector<General::PlayingCard> community_hand_tiles;
		community_hand_tiles = m_communityCards; // 커뮤니티 카드 복사
		for ( const auto& card : playersResult.hand_tiles() )
			community_hand_tiles.push_back( card );

		auto kickerCards = KickerCards( community_hand_tiles , playersResult.cards() );
		for ( auto kickerCard : kickerCards ) {
			auto kicker = playersResult.add_kicker_tiles();
			kicker->CopyFrom( kickerCard );
		}

		cHoldemKicker compareKicker = cHoldemKicker( playersResult );
		playersResult.set_kicker_score( compareKicker.GetPoint() );

		General::ParticipantProfile* pPlayer = playersResult.mutable_member_info();
		player->CopyPlayer( pPlayer );

		// Die 한 사람은 패배자로 무조건 간다.
		if ( player->isDie() ) {
			losers.push_back( playersResult );
		}
		else
			playersResults.push_back( playersResult );

		std::string jokboString;
		const google::protobuf::EnumDescriptor* discriptor = General::HandRank_descriptor();
		const google::protobuf::EnumValueDescriptor* valueDescriptor = discriptor->FindValueByNumber( jokbo );
		if ( valueDescriptor ) {
			jokboString = valueDescriptor->name();
		}

		/*std::string errorInfo = std::format( "Holdem CalcResult Player nick {}, Jokbo  {}" , player->GetNickName() , jokboString );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );*/
	}

	// jokbo, point, kicker point 까지 한번에 비교하는 함수로 처리하자.
	std::sort( playersResults.begin() , playersResults.end() , cHoldem::ComparePlayerResult );

	std::vector<uint64> winners;

	int diePlayerRank = 0;
	bool bAlone = false;
	// 1명 뿐이 없는 경우
	if ( playersResults.size() == 1 )
	{
		bAlone = true;
		playersResults[ 0 ].set_rank_pos( 1 );

		for ( auto& playerResult : losers ) {
			playerResult.set_rank_pos( 2 );
			playersResults.push_back( playerResult );
		}
	}
	else
	{
		int rank = 1;
		for ( int n = 0; n < playersResults.size(); ++n )
		{
			playersResults[ n ].set_rank_pos( rank );

			// 족보가 동일하고, 족보 포인트도 동일하다.
			if ( ( playersResults[ n ].hand_rank() == playersResults[ n + 1 ].hand_rank() ) && ( playersResults[ n ].hand_rank_score() == playersResults[ n + 1 ].hand_rank_score() ) )
			{
				// 키커 비교
				if ( playersResults[ n ].kicker_score() != playersResults[ n + 1 ].kicker_score() )
				{
					++rank;
				}
			}
			else
			{
				++rank;
			}

			// 우열을 정하지 못해. 비교한 두 명의 랭크는 같다.
			if ( n == playersResults.size() - 2 ) {
				playersResults[ n + 1 ].set_rank_pos( rank );
				break;
			}
		}

		++rank;
		if( diePlayerRank == 0 )
			diePlayerRank = rank;
		for ( auto& playerResult : losers ) {
			playerResult.set_rank_pos( rank );
			playersResults.push_back( playerResult );
		}
	}

	// rank_map 에 플레이어의 등수에 따라 정렬해서 집어 넣어 놓는다.
	std::map<int , std::vector<PmNet::MemberOutcome>> rank_map;
	{
		std::vector<PmNet::MemberOutcome> temp;

		for ( const auto& playerResult : playersResults )
		{
			auto iter = rank_map.find( playerResult.rank_pos() );
			if ( iter == rank_map.end() )
			{
				temp.clear();
				temp.push_back( playerResult );
				rank_map.insert( std::pair<int , std::vector<PmNet::MemberOutcome>>( playerResult.rank_pos() , temp ) );
			}
			else
			{
				iter->second.push_back( playerResult );
			}
		}
	}

	std::map<uint64 , PmNet::MemberOutcome> playersResultMap;

	// 등수가 같은 유저들끼리 잘라서 Loop 를 돌린다.
	uint64 potMoney = m_pGameRoom->m_roomInfo.pot_amount();
	m_pGameRoom->PushPotMoneyValue( potMoney );
	m_pGameRoom->CalcPotMoneyStatistic();

	// 동일 등수를 묶어서 처리 한다.
	// potMoney 가 0 이 되면 종료한다.
	for ( int rank = 1; rank <= rank_map.size(); ++rank ) {

		auto iter = rank_map.find( rank );
		if ( iter == rank_map.end() )
		{
			int a = 0;
			break;
		}
		auto& rankResults = iter->second;

		// Die 플레이어들순서가 되면 더 이상 처리 하지 않는다.
		if ( diePlayerRank == rank )
			break;

		CalcMoneyNew( rank , potMoney , rankResults , playersResultMap );
		if ( potMoney <= 1 )
			break;
	}

	// 남은 팟 금액은 1등에게 분배한다.
	if ( potMoney > 1 ) {
		std::ostringstream oss;
		oss << "potRemain : " + std::to_string( potMoney );
		oss << "roomNum : " + std::to_string( m_pGameRoom->GetRoomNumber() );
		for ( auto t_winner : winners )
		{
			oss << "winner: " + std::to_string( t_winner );
		}

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , oss.str().c_str());

		//auto iter = rank_map.find( 1 );
		//auto& rankResults = iter->second;
		//if ( rankResults.size() == 0 )
		//{
		//
		//}
		//else
		//{
		//	double getMoney = potMoney / rankResults.size();

		//	for ( auto& rankResult : rankResults ) {
		//
		//		auto iterPair = playersResultMap.find( rankResult.player_idx() );
		//		if ( iterPair != playersResultMap.end() )
		//		{
		//			auto& playerResult = iterPair->second;
		//			const uint64& playerIdx = playerResult.member_idx();
		//			cClientSession* pClientSession = GetPlayerSession( playerIdx );
		//			if ( pClientSession == nullptr )
		//				continue;

		//			double curAfterMoney = playerResult.svr_fund_after();
		//			double curDealerFee = playerResult.svr_rake_cut();

		//			double dealerFee = getMoney * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
		//			//dealerFee = ceil( dealerFee );

		//			playerResult.set_svr_fund_after( curAfterMoney + getMoney - dealerFee );
		//			playerResult.set_svr_rake_cut( curDealerFee + dealerFee );
		//		}
		//	}
		//}
	}

	// AllIn 카운트 증가 처리
	std::map<uint64 , int64> all_in_map;

	// 게임 로그 저장
	cGameLogInstance game_log( General::PlayCategory::PlayCategory_TexasHoldem , m_pGameRoom->m_roomInfo.asset_kind() , m_pGameRoom->m_roomInfo.bet_policy() , GetChannelId() );
	cGameLogStatisticInstance game_statistic_log( General::PlayCategory::PlayCategory_TexasHoldem , m_pGameRoom->m_roomInfo.asset_kind() , m_pGameRoom->m_roomInfo.bet_policy() , GetChannelId() );

	// 결과 전송
	_result_res.Clear();
	_result_res.set_match_kind( m_pGameRoom->m_roomInfo.play_category() );
	_result_res.set_fund_kind( m_pGameRoom->m_roomInfo.asset_kind() );

	for ( int n = 0; n < playersResults.size(); ++n ) {
		auto& playerResult = playersResults[ n ];

		cClientSession* pClientSession = GetPlayerSession( playerResult.member_idx() );
		if ( pClientSession == nullptr ) continue;

		PmNet::MemberOutcome* pPlayerResult = nullptr;

		if ( playerResult.rank_pos() == 1 ) {
			pPlayerResult = _result_res.add_top_set();
		}
		else {
			pPlayerResult = _result_res.add_bottom_set();
		}

		// playersResultMap 에서 결과를 가져오자
		auto iter = playersResultMap.find( playerResult.member_idx() );
		if ( iter != playersResultMap.end() ) {
			playerResult = iter->second;
		}
		//TraceA( std::format("{} : {}" , playerResult.alias_label() , floor( playerResult.svr_fund_after() ) ));
		playerResult.set_fund_after( floor( playerResult.svr_fund_after() ) );

		playerResult.set_rake_cut( ceil( playerResult.svr_rake_cut() ) );
		if ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind_Coin ) {
			if ( playerResult.svr_rake_cut() > 500000000 )
			{
				//playerResult.set_rake_cut( 0 );
				std::string errorInfo = std::format( "Holdem Money BUG Room : {} , m_gameUid : {}" , m_pGameRoom->GetRoomNumber() , m_gameUid );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );
				QueryManager::InsertErrorLog( errorInfo );

			}
		}
		////경버그 방어코드 주석처리
		//if ( playerResult.svr_fund_after() > 5000000000 )
		//	playerResult.set_svr_fund_after( playerResult.fund_before() );

		//if ( playerResult.fund_after() > 5000000000 )
		//{
		//	//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , .c_str() );
		//	playerResult.set_fund_after( playerResult.fund_before() );
		//}
		const uint64& playerIdx = playerResult.member_idx();
		for ( int i = 0; i < m_maxPlayerCnt; ++i )
		{
			if ( p_id_int[ i ] == playerIdx )
			{
				auto afterMoney = ( playerResult.fund_after() );
				if ( afterMoney > pClientSession->GetMaxHoldingCoin() )
					p_discard_coin[ i ] = afterMoney - pClientSession->GetMaxHoldingCoin();

				break;
			}
		}

		SetPlayerMoney( pClientSession , playerResult.fund_after() );
		pPlayerResult->CopyFrom( playerResult );

		// 손실한도 처리
		pClientSession->UpdateLostLimitChip( m_pGameRoom->m_roomInfo.asset_kind() , playerResult.fund_before() , playerResult.fund_after() );

		// 올인 베팅을 했더라도 입장 가능한 최소 머니 이상 소지하게되면 강퇴처리가 안되어야 합니다.
		if ( pClientSession->isAllIn() ) {
			if ( GetPlayerMoney( pClientSession ) >= m_channel.money_kick() )
			{
				pClientSession->SetAllIn( false );
			}
			else
			{
				all_in_map.insert( std::pair<int64 , int64>( playerResult.member_idx() , playerResult.member_idx() ) );
			}
		}

		// 승리 족보를 기록으로 남긴다.
		if ( playerResult.rank_pos() == 1 && m_channel.money_type() == General::AssetKind_Chip ) {

			General::WinningHandHistory winJokbo;
			winJokbo.set_give_up_win( GetPlayingPlayerCount() == 1 );
			winJokbo.set_hand_rank( playerResult.hand_rank() );
			m_win_jokbos.push_back( winJokbo );

			// 미션, 업적처리
			pClientSession->UpdateQuests( General::TaskTrigger::TaskTrigger_HoldemWin );
		}

		if ( playerResult.hand_rank_score() > 160000000000 && m_channel.money_type() == General::AssetKind_Chip )
		{
			pClientSession->UpdateQuests( General::TaskTrigger::TaskTrigger_HoldemFlush );
		}
		/*std::string t_jstring = std::format( "JokboPoint : {} " , playerResult.hand_rank_score() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , t_jstring.c_str() );*/

		// 전적 처리
		bool isWin = playerResult.rank_pos() == 1;
		int64 getMoney = 0;
		if ( playerResult.fund_after() >= playerResult.fund_before() ) {
			getMoney = static_cast< int64 >( playerResult.fund_after() - playerResult.fund_before() );
		}
		else {
			getMoney = -static_cast< int64 >( playerResult.fund_before() - playerResult.fund_after() );
		}
		pClientSession->UpdateHoldemToday( isWin , m_pGameRoom->m_roomInfo.asset_kind() , getMoney );
		pClientSession->UpdateHoldemTotal( isWin , m_pGameRoom->m_roomInfo.asset_kind() , getMoney );

		// 머니 로그
		std::string target_identifier; // 로우바둑이, 홀덤으로 재화 소진 또는 획득일 경우 대상 UID 기록\r\n* 상점에서의 획득 또는 소진일 경우 구매한 상품 ID 기록\r\n* 미션, 업적이벤트, 업적, 우편함 일경우 해당 log code  기록 (event_type과 동일)\r\n(소진일 경우 승자 UID 1개 출력/획득 일 경우 패배 UID 최대 8개 기록)',
		auto result = QueryManager::MoneyLogInsert(
		( int ) Server::AssetLedgerSource::AssetLedger_Game ,
		( int ) m_pGameRoom->m_roomInfo.asset_kind() ,
		pClientSession->GetAccountGuid() ,
		pClientSession->GetPlatformGuid() ,
		pClientSession->GetNickName() ,
		pClientSession->GetIp() ,
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney : 0 ,
		( int ) General::PlayCategory::PlayCategory_TexasHoldem ,
		( int ) General::SlotCatalog::PM_SLOT_None ,
		target_identifier );

		result.wait();

		if ( FALSE == result.get() ) {

			// 로그 저장 실패
		}

		// 인게임 레코드 처리
		pClientSession->IngameRecord( General::PlayCategory::PlayCategory_TexasHoldem , m_pGameRoom->m_roomInfo.asset_kind() , isWin , playerResult.fund_before() , playerResult.fund_after() );

		if ( pClientSession->isDie() )
			pClientSession->UpdateJokboRecords( General::PlayCategory::PlayCategory_TexasHoldem , isWin , General::HandRank::HandRank_RecordFold );
		else if ( false == bAlone )
			pClientSession->UpdateJokboRecords( General::PlayCategory::PlayCategory_TexasHoldem , isWin , playerResult.hand_rank() );

		// 칩방이고, 획득머니가 있는 경우에 업적 갱신
		if ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Chip && playerResult.fund_after() > playerResult.fund_before() ) {
			pClientSession->UpdateQuests( General::TaskTrigger::TaskTrigger_GameWinChip , playerResult.fund_after() - playerResult.fund_before() );
		}

		if ( m_channel.channel_content_type() == Server::ChannelPlayMode::ChannelPlayMode_Standard ) {
			// 칩방에서 승리한  경우에는 경험치를 증가 시킨다.
			if ( playerResult.rank_pos() == 1 && m_pGameRoom->m_roomInfo.asset_kind() & General::AssetKind::AssetKind_Chip ) {
				pClientSession->UpdateLevel( 1 );
			}
		}
			// 적립 통장 적립 처리
		// 전체 수익에 대해서 적립을 한다.
		uint64 rakeback_money = 0;
		if ( playerResult.fund_after() > playerResult.fund_before() ) {

			if ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Coin ) {
				rakeback_money = pClientSession->AddRakeBack( General::PlayCategory::PlayCategory_TexasHoldem , playerResult.fund_after() - playerResult.fund_before() );
				pPlayerResult->mutable_member_info()->set_rakeback_balance( pClientSession->GetRakeBack() );
			}
			// pClientSession->AddRakeBack( General::PlayCategory::PlayCategory_TexasHoldem , playerResult.rake_cut() );
		}

		game_log.PushCommunityCard( m_communityCards );

		if ( isWin )
		{
			for ( int i = 0; i < m_playerSlots.size(); ++i )
			{
				if ( playerResult.member_idx() == p_id_int[ i ] )
				{
					win_id[ i ] = pClientSession->GetPlatformGuid();
					win_asset[ i ] = playerResult.fund_after();
					break;
				}
			}

			game_log.PushWinner(
				pClientSession->GetPlatformGuid() ,
				pClientSession->GetIp() ,
				playerResult.fund_before() ,
				getMoney ,
				playerResult.fund_after() ,
				rakeback_money ,
				playerResult.hand_rank() ,
				playerResult.cards() );
		}
		else {

			game_log.PushLoser(
				pClientSession->GetPlatformGuid() ,
				pClientSession->GetIp() ,
				playerResult.fund_before() ,
				getMoney ,
				playerResult.fund_after() ,
				rakeback_money ,
				playerResult.hand_rank() ,
				playerResult.cards() );
		}
		// 핸드카드와 승자 족보 정보를 p0_result에 넣음
        std::string resultString = CardsToString( playerResult.hand_tiles() ) + " | " + "[" + CardsToString( m_communityCards ) + "]";

		for ( int i = 0; i < m_playerSlots.size(); ++i )
		{
			if ( playerResult.member_idx() == p_id_int[ i ] )
			{
				p_result[ i ] = resultString;

				cClientSession* pClientSession_rslt = GetPlayerSession( playerResult.member_idx() );

				p_rakeback[ i ] = rakeback_money;
				p_change_coin[ i ] = playerResult.fund_after() - playerResult.fund_before();
				p_fee_coin[ i ] = playerResult.rake_cut();

				switch ( pClientSession_rslt->GetMemberShipClass() )
				{
				case General::BenefitTier::BenefitTier_Premium:
					p_fee_info[ i ] = "Premium";
					break;
				case General::BenefitTier::BenefitTier_Standard:
					p_fee_info[ i ] = "Standard";
					break;
				default:
					p_fee_info[ i ] = "None";
					break;
				}

				p_coin[ i ] = pClientSession_rslt->GetCoin();

				p_asset[ i ] = 0; // x
				p_play_cnt[ i ] = 0; // x
				break;
			}
		}

		// 게임 통계용 로그 저장
		game_statistic_log.PushLog(
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney * -1 : 0 ,
		pClientSession->GetMemberShipClass() ,
		playerResult.rake_cut() );

		game_statistic_log.PushMemberPlayerIndex( std::to_string( playerResult.member_idx() ) );
	}

	// 승리자들에게 올인시킨 상대방이 있는지 체크하여 퀘스트 업데이트
	if ( !all_in_map.empty() ) {
		for ( int n = 0; n < playersResults.size(); ++n ) {
			auto& playerResult = playersResults[ n ];

			// 승리자인 경우에만 체크
			if ( playerResult.rank_pos() == 1 ) {
				cClientSession* pWinnerSession = GetPlayerSession( playerResult.member_idx() );
				if ( pWinnerSession != nullptr ) {
					// 승리자가 아닌 다른 올인 플레이어가 있는지 체크
					bool hasAllinEnemy = false;
					for ( const auto& allinPlayer : all_in_map ) {
						if ( allinPlayer.first != playerResult.member_idx() ) {
							hasAllinEnemy = true;
							break;
						}
					}

					if ( hasAllinEnemy ) {
						pWinnerSession->UpdateQuests( General::TaskTrigger::TaskTrigger_HoldemEnemyAllIn );
					}
				}
			}
		}
	}

	// 나간놈 로그
 	for ( int i = 0; i < copiedResult.size(); ++i )
	{
		auto playerResult = copiedResult[ i ];

		int idx = 0;
		for ( idx = 0; idx < m_playerSlots.size(); ++idx )
		{
			if ( copiedResult[ i ].member_idx() == p_id_int[ idx ] )
			{
				p_discard_coin[ idx ] = 0;
				p_change_coin[ idx ] = playerResult.fund_after() - playerResult.fund_before();
				p_fee_coin[ idx ] = playerResult.rake_cut();
				p_coin[ idx ] = playerResult.fund_after();

				p_asset[ idx ] = 0; // x
				p_play_cnt[ idx ] = 0; // x
				break;
			}
		}
	}

	int code = 20403;

	// 일반룰:N, 스트래들:S, 더블 스트래들:DS   (ex. MAX=6,OPTS=DS)
	std::ostringstream ossGameOpt;
	ossGameOpt << "MAX=" << std::to_string( m_pGameRoom->GetMaxRoomPlayerCnt() ) << ",OPTS=";

	if ( General::BetPolicy::BetPolicy_HoldemStandard == m_pGameRoom->m_roomInfo.bet_policy() )
		ossGameOpt << "N";
	else if ( General::BetPolicy::BetPolicy_HoldemThreeBet == m_pGameRoom->m_roomInfo.bet_policy() )
		ossGameOpt << "S";
	else if ( General::BetPolicy::BetPolicy_HoldemFourBet == m_pGameRoom->m_roomInfo.bet_policy() )
		ossGameOpt << "DS";

	// InsertGameHoldemLog 로그 추가
	std::ostringstream ossPlayers;
	for ( int i = 0; i < m_maxPlayerCnt; ++i )
	{
		if ( p_id[ i ].empty() )
			continue;

		ossPlayers << p_id[ i ] << ",";
	}

	auto log_result_holdem_end = QueryManager::InsertGameHoldemLog(
		code ,                           // 코드
		m_gameUid ,                             // 게임 ID
		m_channel.id() , // 채널
		m_pGameRoom->m_roomInfo.played_rounds() + 1 , // 게임 횟수
		"" ,                             // 로그 버전
		ossGameOpt.str() ,                       // 게임 옵션
		"" , // 보스
		"" , // 호스트
		p_id[ 0 ] , p_asset[ 0 ] , p_play_cnt[ 0 ] , p_change_coin[ 0 ] , p_discard_coin[ 0 ] , p_fee_coin[ 0 ] , p_fee_info[ 0 ] , p_rakeback[ 0 ] , p_coin[ 0 ] , p_result[ 0 ] ,
		p_id[ 1 ] , p_asset[ 1 ] , p_play_cnt[ 1 ] , p_change_coin[ 1 ] , p_discard_coin[ 1 ] , p_fee_coin[ 1 ] , p_fee_info[ 1 ] , p_rakeback[ 1 ] , p_coin[ 1 ] , p_result[ 1 ] ,
		p_id[ 2 ] , p_asset[ 2 ] , p_play_cnt[ 2 ] , p_change_coin[ 2 ] , p_discard_coin[ 2 ] , p_fee_coin[ 2 ] , p_fee_info[ 2 ] , p_rakeback[ 2 ] , p_coin[ 2 ] , p_result[ 2 ] ,
		p_id[ 3 ] , p_asset[ 3 ] , p_play_cnt[ 3 ] , p_change_coin[ 3 ] , p_discard_coin[ 3 ] , p_fee_coin[ 3 ] , p_fee_info[ 3 ] , p_rakeback[ 3 ] , p_coin[ 3 ] , p_result[ 3 ] ,
		p_id[ 4 ] , p_asset[ 4 ] , p_play_cnt[ 4 ] , p_change_coin[ 4 ] , p_discard_coin[ 4 ] , p_fee_coin[ 4 ] , p_fee_info[ 4 ] , p_rakeback[ 4 ] , p_coin[ 4 ] , p_result[ 4 ] ,
		p_id[ 5 ] , p_asset[ 5 ] , p_play_cnt[ 5 ] , p_change_coin[ 5 ] , p_discard_coin[ 5 ] , p_fee_coin[ 5 ] , p_fee_info[ 5 ] , p_rakeback[ 5 ] , p_coin[ 5 ] , p_result[ 5 ] ,
		p_id[ 6 ] , p_asset[ 6 ] , p_play_cnt[ 6 ] , p_change_coin[ 6 ] , p_discard_coin[ 6 ] , p_fee_coin[ 6 ] , p_fee_info[ 6 ] , p_rakeback[ 6 ] , p_coin[ 6 ] , p_result[ 6 ] ,
		p_id[ 7 ] , p_asset[ 7 ] , p_play_cnt[ 7 ] , p_change_coin[ 7 ] , p_discard_coin[ 7 ] , p_fee_coin[ 7 ] , p_fee_info[ 7 ] , p_rakeback[ 7 ] , p_coin[ 7 ] , p_result[ 7 ] ,
		p_id[ 8 ] , p_asset[ 8 ] , p_play_cnt[ 8 ] , p_change_coin[ 8 ] , p_discard_coin[ 8 ] , p_fee_coin[ 8 ] , p_fee_info[ 8 ] , p_rakeback[ 8 ] , p_coin[ 8 ] , p_result[ 8 ] ,
		// 추가 데이터
		ossPlayers.str() ,
		timeStamp , // play_time
		"roomid_" + std::to_string( m_pGameRoom->GetRoomNumber() ) , // room_id
		// win data
		win_id[ 0 ] , win_asset[ 0 ] ,
		win_id[ 1 ] , win_asset[ 1 ] ,
		win_id[ 2 ] , win_asset[ 2 ] ,
		win_id[ 3 ] , win_asset[ 3 ] ,
		win_id[ 4 ] , win_asset[ 4 ] ,
		win_id[ 5 ] , win_asset[ 5 ] ,
		win_id[ 6 ] , win_asset[ 6 ] ,
		win_id[ 7 ] , win_asset[ 7 ] ,
		win_id[ 8 ] , win_asset[ 8 ] );

	log_result_holdem_end.wait();

	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		// 누적 All IN 카운트를 계속 증가 시킨다.
		player->UpdateQuest( General::TaskTrigger::TaskTrigger_EnemyAllIn , all_in_map );

		// 미션, 업적처리
		if ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Chip )
			player->UpdateExceptLoungeQuests( General::TaskTrigger::TaskTrigger_HoldemPlay );
		else
			player->UpdateLoungeQuests( General::TaskTrigger::TaskTrigger_HoldemPlay );

		player->UpdateQuests( General::TaskTrigger::TaskTrigger_ChipReach );
	}

	// 최근 기록을 response 에 복사
	auto histories = GetRecentlyPlayedGames();
	for ( auto& history : histories )
	{
		General::WinningHandHistory* pJokbo = _result_res.add_win_log();
		pJokbo->CopyFrom( history );
	}

	m_pGameRoom->BroadCastToAllPlayer( General::Packet_RoundOutcome , _result_res );

	// 전송 메시지 직렬화
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( _result_res , &serializedData );
	TraceA( serializedData );
	InsertBetLog( serializedData );
	game_record_string = serializedData;
}

// 커뮤니티 카드를 나누기도 전에 플레이어들이 Die 를 해버려서 승부가 결정난 경우
void cHoldem::CalcResultNoCommunityCard()
{
	// 시작 시간 찍기
	auto now = std::chrono::system_clock::now();
	auto epoch = now.time_since_epoch();
	timeStamp = std::chrono::duration_cast< std::chrono::seconds >( epoch ).count() - timeStamp;

	// 게임 로그 저장
	cGameLogInstance game_log( General::PlayCategory::PlayCategory_TexasHoldem , m_pGameRoom->m_roomInfo.asset_kind() , m_pGameRoom->m_roomInfo.bet_policy() , GetChannelId() );
	cGameLogStatisticInstance game_statistic_log( General::PlayCategory::PlayCategory_TexasHoldem , m_pGameRoom->m_roomInfo.asset_kind() , m_pGameRoom->m_roomInfo.bet_policy() , GetChannelId() );

	// 결과 전송
	//PmNet::MatchOutcomeRS _res;
	_result_res.Clear();
	_result_res.set_match_kind( m_pGameRoom->m_roomInfo.play_category() );
	_result_res.set_fund_kind( m_pGameRoom->m_roomInfo.asset_kind() );

	uint64 potMoney = m_pGameRoom->m_roomInfo.pot_amount();
	m_pGameRoom->PushPotMoneyValue( potMoney );
	m_pGameRoom->CalcPotMoneyStatistic();

	// 플레이 유저들의 카드 정렬kicker.GetPoint()
	for ( auto player : m_betSequence ) {
		if ( player == nullptr )
			continue;

		if ( nullptr == GetPlayerSession( player->GetPlayerIdx() ) )
			continue;

		PmNet::MemberOutcome playerResult;

		google::protobuf::RepeatedField<General::PlayingCard> cards = player->GetCardsClone();

		// 핸드 카드 복사
		for ( const auto& card : cards ) {
			auto hand_card = playerResult.add_hand_tiles();
			hand_card->CopyFrom( card );
		}

		playerResult.set_alias_label( player->GetNickName() );
		playerResult.set_member_idx( player->GetPlayerIdx() );
		playerResult.set_fund_before( GetPlayerMoney( player ) + player->GetLostMoney() ); // 현재칩 + 사용된 칩 누적

		General::ParticipantProfile* pPlayer = playerResult.mutable_member_info();
		player->CopyPlayer( pPlayer );
		PmNet::MemberOutcome* pPlayerResult;
		// Die 한 사람은 패배자로 무조건 간다.
		if ( player->isDie() )
		{
			playerResult.set_fund_after( GetPlayerMoney( player ) );

			// 손실한도 처리
			player->UpdateLostLimitChip( m_pGameRoom->m_roomInfo.asset_kind() , playerResult.fund_before() , playerResult.fund_after() );

			pPlayerResult = _result_res.add_bottom_set();
			pPlayerResult->CopyFrom( playerResult );

			/*std::string errorInfo = std::format( "Holdem CalcResult Player nick {} loser by die" , player->GetNickName() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );*/
		}
		else
		{
			// 올인 유저 인경우중간에 승부가 나 된을 획득 했음으로 올인을 풀어준다.
			if ( player->isAllIn() )
				player->SetAllIn( false );

			uint64 curMoney = GetPlayerMoney( player );
			uint64 getMoney = potMoney - player->GetLostMoney();
			uint64 dealerFee = getMoney * player->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );

			playerResult.set_fund_after( curMoney + player->GetLostMoney() + getMoney - dealerFee );
			playerResult.set_rake_cut( dealerFee );

			pPlayerResult = _result_res.add_top_set();
			pPlayerResult->CopyFrom( playerResult );

			const uint64& playerIdx = playerResult.member_idx();
			for ( int i = 0; i < m_maxPlayerCnt; ++i )
			{
				if ( p_id_int[ i ] == playerIdx )
				{
					auto afterMoney = ( curMoney + player->GetLostMoney() + getMoney - dealerFee );
					if ( afterMoney > player->GetMaxHoldingCoin() )
						p_discard_coin[ i ] = afterMoney - player->GetMaxHoldingCoin();

					break;
				}
			}

			SetPlayerMoney( player , curMoney + player->GetLostMoney() + getMoney - dealerFee );

			// 손실한도 처리
			player->UpdateLostLimitChip( m_pGameRoom->m_roomInfo.asset_kind() , playerResult.fund_before() , playerResult.fund_after() );

			General::WinningHandHistory winJokbo;
			winJokbo.set_give_up_win( GetPlayingPlayerCount() == 1 );
			//winJokbo.set_hand_rank( iter->second.hand_rank() );
			m_win_jokbos.push_back( winJokbo );

			/*std::string errorInfo = std::format( "Holdem CalcResult Player nick {} winner by all die" , player->GetNickName() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorInfo.c_str() );*/
		}

		// 전적 처리
		bool isWin = player->isDie() == false;
		int64 getMoney = 0;
		if ( playerResult.fund_after() >= playerResult.fund_before() ) {
			getMoney = static_cast< int64 >( playerResult.fund_after() - playerResult.fund_before() );
		}
		else {
			getMoney = -static_cast< int64 >( playerResult.fund_before() - playerResult.fund_after() );
		}
		player->UpdateHoldemToday( isWin , m_pGameRoom->m_roomInfo.asset_kind() , getMoney );
		player->UpdateHoldemTotal( isWin , m_pGameRoom->m_roomInfo.asset_kind() , getMoney );

		// 머니 로그
		std::string target_identifier; // 로우바둑이, 홀덤으로 재화 소진 또는 획득일 경우 대상 UID 기록\r\n* 상점에서의 획득 또는 소진일 경우 구매한 상품 ID 기록\r\n* 미션, 업적이벤트, 업적, 우편함 일경우 해당 log code  기록 (event_type과 동일)\r\n(소진일 경우 승자 UID 1개 출력/획득 일 경우 패배 UID 최대 8개 기록)',
		auto result = QueryManager::MoneyLogInsert(
		( int ) Server::AssetLedgerSource::AssetLedger_Game ,
		( int ) m_pGameRoom->m_roomInfo.asset_kind() ,
		player->GetAccountGuid() ,
		player->GetPlatformGuid() ,
		player->GetNickName() ,
		player->GetIp() ,
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney : 0 ,
		( int ) General::PlayCategory::PlayCategory_TexasHoldem ,
		( int ) General::SlotCatalog::PM_SLOT_None ,
		target_identifier );

		result.wait();

		if ( FALSE == result.get() ) {

			// 로그 저장 실패
		}

		// 인게임 레코드 처리
		player->IngameRecord( General::PlayCategory::PlayCategory_TexasHoldem , m_pGameRoom->m_roomInfo.asset_kind() , isWin , playerResult.fund_before() , playerResult.fund_after() );

		if ( player->isDie() )
			player->UpdateJokboRecords( General::PlayCategory::PlayCategory_TexasHoldem , isWin , General::HandRank::HandRank_RecordFold );
		/*else
			player->UpdateJokboRecords( General::PlayCategory::PlayCategory_TexasHoldem , isWin , playerResult.hand_rank() );*/

			// 미션, 업적처리
		if( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Chip )
			player->UpdateExceptLoungeQuests( General::TaskTrigger::TaskTrigger_HoldemPlay );
		else
			player->UpdateLoungeQuests( General::TaskTrigger::TaskTrigger_HoldemPlay );

		player->UpdateQuests( General::TaskTrigger::TaskTrigger_ChipReach );

		// 미션, 업적처리
		if ( isWin )
		{
			player->UpdateQuests( General::TaskTrigger::TaskTrigger_HoldemWin );
			if ( m_channel.channel_content_type() == Server::ChannelPlayMode::ChannelPlayMode_Standard ) {
				// 칩방에서 승리한  경우에는 경험치를 증가 시킨다.
				if ( m_pGameRoom->m_roomInfo.asset_kind() & General::AssetKind::AssetKind_Chip ) {
					player->UpdateLevel( 1 );
				}
			}

		}
		// 칩방이고, 획득머니가 있는 경우에 업적 갱신
		if ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Chip && playerResult.fund_after() > playerResult.fund_before() ) {
			player->UpdateQuests( General::TaskTrigger::TaskTrigger_GameWinChip , playerResult.fund_after() - playerResult.fund_before() );
		}


		// 적립 통장 적립 처리
		// 전체 수익에 대해서 적립을 한다.
		uint64 rakeback_money = 0;
		if ( playerResult.rake_cut() > 0 ) {

			if ( m_pGameRoom->m_roomInfo.asset_kind() == General::AssetKind::AssetKind_Coin ) {
				rakeback_money = player->AddRakeBack( General::PlayCategory::PlayCategory_TexasHoldem , playerResult.fund_after() - playerResult.fund_before() );
				pPlayerResult->mutable_member_info()->set_rakeback_balance( player->GetRakeBack() );
			}
		}

		//game_log.PushCommunityCard( m_communityCards );

		if ( isWin )
		{

			for ( int i = 0; i < m_playerSlots.size(); ++i )
			{
				if ( playerResult.member_idx() == p_id_int[ i ] )
				{
					win_id[ i ] = player->GetPlatformGuid();
					win_asset[ i ] = playerResult.fund_after();
					break;
				}
			}

			game_log.PushWinner(
				player->GetPlatformGuid() ,
				player->GetIp() ,
				playerResult.fund_before() ,
				getMoney ,
				playerResult.fund_after() ,
				rakeback_money ,
				playerResult.hand_rank() ,
				playerResult.cards() );
		}
		else {

			game_log.PushLoser(
				player->GetPlatformGuid() ,
				player->GetIp() ,
				playerResult.fund_before() ,
				getMoney ,
				playerResult.fund_after() ,
				rakeback_money ,
				playerResult.hand_rank() ,
				playerResult.cards() );
		}

				// 핸드카드와 승자 족보 정보를 p0_result에 넣음
		std::string cards_string = CardsToString(playerResult.hand_tiles());

        std::string resultString = cards_string;

		for ( int i = 0; i < m_playerSlots.size(); ++i )
		{
			if ( playerResult.member_idx() == p_id_int[ i ] )
			{
				p_result[ i ] = resultString;

				cClientSession* pClientSession_rslt = GetPlayerSession( playerResult.member_idx() );
				if ( nullptr == pClientSession_rslt )
					continue;

				p_rakeback[ i ] = rakeback_money;
				p_change_coin[ i ] = playerResult.fund_after() - playerResult.fund_before();
				p_fee_coin[ i ] = playerResult.rake_cut();

				switch ( pClientSession_rslt->GetMemberShipClass() )
				{
				case General::BenefitTier::BenefitTier_Premium:
					p_fee_info[ i ] = "Premium";
					break;
				case General::BenefitTier::BenefitTier_Standard:
					p_fee_info[ i ] = "Standard";
					break;
				default:
					p_fee_info[ i ] = "None";
					break;
				}

				p_coin[ i ] = pClientSession_rslt->GetCoin();

				p_asset[ i ] = 0; // x
				p_play_cnt[ i ] = 0; // x
				break;
			}
		}

		cClientSession* pClientSession_rslt = GetPlayerSession( playerResult.member_idx() );
		if ( nullptr == pClientSession_rslt )
			continue;

		// 게임 통계용 로그 저장
		game_statistic_log.PushLog(
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney * -1 : 0 ,
		player->GetMemberShipClass() ,
		playerResult.rake_cut() );

		game_statistic_log.PushMemberPlayerIndex( std::to_string( playerResult.member_idx() ) );
	}

	// 최근 기록을 response 에 복사
	auto histories = GetRecentlyPlayedGames();
	for ( auto& history : histories )
	{
		General::WinningHandHistory* pJokbo = _result_res.add_win_log();
		pJokbo->CopyFrom( history );
	}

	int code = 20403;

	// 일반룰:N, 스트래들:S, 더블 스트래들:DS   (ex. MAX=6,OPTS=DS)
	std::ostringstream ossGameOpt;
	ossGameOpt << "MAX=" << std::to_string( m_pGameRoom->GetMaxRoomPlayerCnt() ) << ",OPTS=";

	if ( General::BetPolicy::BetPolicy_HoldemStandard == m_pGameRoom->m_roomInfo.bet_policy() )
		ossGameOpt << "N";
	else if ( General::BetPolicy::BetPolicy_HoldemThreeBet == m_pGameRoom->m_roomInfo.bet_policy() )
		ossGameOpt << "S";
	else if ( General::BetPolicy::BetPolicy_HoldemFourBet == m_pGameRoom->m_roomInfo.bet_policy() )
		ossGameOpt << "DS";

	// InsertGameHoldemLog 로그 추가
	std::ostringstream ossPlayers;
	for ( int i = 0; i < m_maxPlayerCnt; ++i )
	{
		if ( p_id[ i ].empty() )
			continue;

		ossPlayers << p_id[ i ] << ",";
	}

	auto log_result_holdem_end = QueryManager::InsertGameHoldemLog(
		code ,                           // 코드
		m_gameUid ,                             // 게임 ID
		m_channel.id() , // 채널
		m_pGameRoom->m_roomInfo.played_rounds() + 1 , // 게임 횟수
		"" ,                             // 로그 버전
		ossGameOpt.str() ,                       // 게임 옵션
		"" , // 보스
		"" , // 호스트
		p_id[ 0 ] , p_asset[ 0 ] , p_play_cnt[ 0 ] , p_change_coin[ 0 ] , p_discard_coin[ 0 ] , p_fee_coin[ 0 ] , p_fee_info[ 0 ] , p_rakeback[ 0 ] , p_coin[ 0 ] , p_result[ 0 ] ,
		p_id[ 1 ] , p_asset[ 1 ] , p_play_cnt[ 1 ] , p_change_coin[ 1 ] , p_discard_coin[ 1 ] , p_fee_coin[ 1 ] , p_fee_info[ 1 ] , p_rakeback[ 1 ] , p_coin[ 1 ] , p_result[ 1 ] ,
		p_id[ 2 ] , p_asset[ 2 ] , p_play_cnt[ 2 ] , p_change_coin[ 2 ] , p_discard_coin[ 2 ] , p_fee_coin[ 2 ] , p_fee_info[ 2 ] , p_rakeback[ 2 ] , p_coin[ 2 ] , p_result[ 2 ] ,
		p_id[ 3 ] , p_asset[ 3 ] , p_play_cnt[ 3 ] , p_change_coin[ 3 ] , p_discard_coin[ 3 ] , p_fee_coin[ 3 ] , p_fee_info[ 3 ] , p_rakeback[ 3 ] , p_coin[ 3 ] , p_result[ 3 ] ,
		p_id[ 4 ] , p_asset[ 4 ] , p_play_cnt[ 4 ] , p_change_coin[ 4 ] , p_discard_coin[ 4 ] , p_fee_coin[ 4 ] , p_fee_info[ 4 ] , p_rakeback[ 4 ] , p_coin[ 4 ] , p_result[ 4 ] ,
		p_id[ 5 ] , p_asset[ 5 ] , p_play_cnt[ 5 ] , p_change_coin[ 5 ] , p_discard_coin[ 5 ] , p_fee_coin[ 5 ] , p_fee_info[ 5 ] , p_rakeback[ 5 ] , p_coin[ 5 ] , p_result[ 5 ] ,
		p_id[ 6 ] , p_asset[ 6 ] , p_play_cnt[ 6 ] , p_change_coin[ 6 ] , p_discard_coin[ 6 ] , p_fee_coin[ 6 ] , p_fee_info[ 6 ] , p_rakeback[ 6 ] , p_coin[ 6 ] , p_result[ 6 ] ,
		p_id[ 7 ] , p_asset[ 7 ] , p_play_cnt[ 7 ] , p_change_coin[ 7 ] , p_discard_coin[ 7 ] , p_fee_coin[ 7 ] , p_fee_info[ 7 ] , p_rakeback[ 7 ] , p_coin[ 7 ] , p_result[ 7 ] ,
		p_id[ 8 ] , p_asset[ 8 ] , p_play_cnt[ 8 ] , p_change_coin[ 8 ] , p_discard_coin[ 8 ] , p_fee_coin[ 8 ] , p_fee_info[ 8 ] , p_rakeback[ 8 ] , p_coin[ 8 ] , p_result[ 8 ] ,
		// 추가 데이터
		ossPlayers.str() ,
		timeStamp , // play_time
		"roomid_" + std::to_string( m_pGameRoom->GetRoomNumber() ) , // room_id
		// win data
		win_id[ 0 ] , win_asset[ 0 ] ,
		win_id[ 1 ] , win_asset[ 1 ] ,
		win_id[ 2 ] , win_asset[ 2 ] ,
		win_id[ 3 ] , win_asset[ 3 ] ,
		win_id[ 4 ] , win_asset[ 4 ] ,
		win_id[ 5 ] , win_asset[ 5 ] ,
		win_id[ 6 ] , win_asset[ 6 ] ,
		win_id[ 7 ] , win_asset[ 7 ] ,
		win_id[ 8 ] , win_asset[ 8 ] );

	log_result_holdem_end.wait();

	m_pGameRoom->BroadCastToAllPlayer( General::Packet_RoundOutcome , _result_res );

	// 전송 메시지 직렬화
	std::string serializedData;
	google::protobuf::util::MessageToJsonString( _result_res , &serializedData );
	TraceA( serializedData );
	InsertBetLog( serializedData );
	game_record_string = serializedData;
}

// 핸드카드 + 커뮤니티카드에서
// 족보에 사용된 카드를 제외한 카드 들을 키커라 칭한다.
// 키커 카드 중에 강한 카드 순으로 정렬한다. A K Q J ~~ 2
std::vector<General::PlayingCard> cHoldem::KickerCards( std::vector<General::PlayingCard>& cards , const google::protobuf::RepeatedPtrField<General::PlayingCard>& jokboCards )
{
	std::vector<General::PlayingCard> numbers;
	std::vector<General::PlayingCard> aces;

	// protobuf 메시지를 비교하는 람다 함수
	auto protobufEqual = []( const General::PlayingCard& msg1 , const General::PlayingCard& msg2 ) {
		std::string str1 , str2;
		msg1.SerializeToString( &str1 );
		msg2.SerializeToString( &str2 );
		return str1 == str2;
	};

	// 족보에 포함되지 않은 저장한다.
	for ( auto& card : cards ) {
		//auto iter = std::find( jokboCards.begin() , jokboCards.end() , card );
		auto iter = std::find_if( jokboCards.begin() , jokboCards.end() ,
			[&card , &protobufEqual]( const General::PlayingCard& otherCard ) {
			return protobufEqual( card , otherCard );
		} );
		if ( iter == jokboCards.end() ) {
			if ( card.rank_code() != General::CardRank::CardRank_Ace )
				numbers.push_back( card );
			else
				aces.push_back( card );
		}
	}

	// 숫자 카드 desc 정렬
	std::sort( numbers.begin() , numbers.end() , SortDesc );

	std::vector<General::PlayingCard> kickers;
	std::vector<General::PlayingCard> tempKickers;
	tempKickers = aces;
	for ( auto& card : numbers )
		tempKickers.push_back( card );

	int kickerCount = 5 - jokboCards.size();
	for ( int n = 0; n < kickerCount; ++n ) {

		// tempKickers 가 모자란 경우 터지지 않도록 수정
		if ( n < tempKickers.size() )
			kickers.push_back( tempKickers[ n ] );
	}

	return kickers;
}

std::vector<General::PlayingCard> cHoldem::SortJokboCards( const google::protobuf::RepeatedPtrField<General::PlayingCard>& jokboCards , General::HandRank jokbo)
{
	std::vector<General::PlayingCard> numbers;
	std::vector<General::PlayingCard> aces;
	std::map<General::CardRank , int> fullhouse;

	// 족보에 포함되지 않은 저장한다.
	for ( auto& card : jokboCards ) {
		fullhouse[ card.rank_code() ]++;
		if ( card.rank_code() != General::CardRank::CardRank_Ace )
			numbers.push_back( card );
		else
			aces.push_back( card );
	}

	if ( jokbo == General::HandRank::HandRank_HoldemFullHouse )
	{
		std::vector<General::PlayingCard> sortedFullJokbo;
		//트리플
		for ( auto& card : jokboCards )
		{
			if ( fullhouse[ card.rank_code() ] == 3 )
				sortedFullJokbo.push_back( card );
		}
		for ( auto& card : jokboCards )
		{
			if ( fullhouse[ card.rank_code() ] == 2 )
				sortedFullJokbo.push_back( card );
		}

		return sortedFullJokbo;
	}

	// 숫자 카드 desc 정렬
	std::sort( numbers.begin() , numbers.end() , SortDesc );


	std::vector<General::PlayingCard> sortedJokbo;
	sortedJokbo = aces;
	for ( auto& card : numbers )
		sortedJokbo.push_back( card );







	return sortedJokbo;
}

/*
1. 결과처리 -> 유저들 결과 정렬해서 들어갈것 PlayerResults 에는 순서대로
2. PlayerResult 에 등수 추가 필요 ( 동일 등수의 처리 문제 )
3. 1등이 여려명인 경우 Side가 발생한 사람이 Array 상 위에 위치 시키도록 정렬
4. Side 로직은 PlayerResults 를 순서대로 처리만 한다.
5. Die 한 유저도 포함 PlayerResults 에 포함시키고, Die 인 경우에는 돈을 나누는 부분만 생략한다.
*/
//void cHoldem::CalcMoney( const std::vector<uint64>& winners , std::deque<PmNet::MemberOutcome>& sortedLosers , std::vector<uint64>& diePlayersPots , std::vector<PmNet::MemberOutcome>& playersResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap )
////void cHoldem::CalcMoney( const std::vector<uint64>& winners , std::vector<uint64>& diePlayersPots , std::vector<PmNet::MemberOutcome>& playersResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap )
//{
//	uint64 potMoney = m_pGameRoom->m_roomInfo.pot();
//
//	// 승리자가 1명
//	if ( winners.size() == 1 )
//	{
//		// playersResults 의 0번이 1등이 아닐수도 있어서 로직을 변경한다
//		std::vector<PmNet::MemberOutcome> tempResults = playersResults;
//
//		// 1등부터 다시 정렬한다.
//		playersResults.clear();
//		uint64 winnerPlayerIdx = winners[ 0 ];
//
//		std::vector<cSidePot*> sidePots = cSidePotManager::GetAllSides();
//
//		// winnerPlayerIdx 를 0번으로 해서 재 정렬
//		{
//			for ( auto& playersResult : tempResults ) {
//				if ( winnerPlayerIdx == playersResult.member_idx() )
//					playersResults.push_back( playersResult );
//			}
//
//			for ( auto& playersResult : tempResults ) {
//				if ( winnerPlayerIdx != playersResult.member_idx() )
//					playersResults.push_back( playersResult );
//			}
//		}
//
//		uint64 beforeSidePotBase = 0;
//		for ( int n = 0; n < playersResults.size(); ++n ) {
//			auto& playerResult = playersResults[ n ];
//			const uint64 playerIdx = playerResult.member_idx();
//			cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//			cSidePot* sidePot = cSidePotManager::GetSide( playerIdx );
//			if ( sidePot == nullptr )
//			{
//				// TODO 남은 사람들중에 동일한 포인트가 존재 한다면?? 나누어야 하나?
//
//				// Side 팟 금액 처리
//				uint64 getMoney = 0;
//
//				// 딜러 수수료 부과 계산할 금액
//				uint64 taxMoney = 0;
//
//				// beforeSidePotBase 보다 베팅을 더한 금액을 가져와야 한다.
//				// 자기 자신의 돈이 아닌 다른 사람들의 금액에 대해서 딜러 수수료를 계산한다.
//				for ( int start = 0; start < playersResults.size(); ++start ) {
//					auto player = GetPlayerIdx( playersResults[ start ].player_idx() );
//					if ( player->GetLostMoney() >= beforeSidePotBase )
//					{
//						uint64 plusMoney = player->GetLostMoney() - beforeSidePotBase;
//
//						// 자신의 돈외에는 딜러 수수료 부과
//						if ( playerIdx != playersResults[ start ].player_idx() )
//							taxMoney += plusMoney;
//
//						getMoney += plusMoney;
//					}
//				}
//
//				uint64 dealerFee = taxMoney * m_dealerFeeRate;
//
//				playerResult.set_fund_after( GetPlayerMoney( pClientSession ) + getMoney - dealerFee );
//				playerResult.set_rake_cut( dealerFee );
//
//				playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
//				break;
//			}
//			else
//			{
//				// Side 팟 금액 처리
//				uint64 getMoney = 0;
//
//				// 딜러 수수료 부과 계산할 금액
//				uint64 taxMoney = 0;
//
//				// beforeSidePotBase 보다 베팅을 더한 금액을 가져와야 한다.
//				// 자기 자신의 돈이 아닌 다른 사람들의 금액에 대해서 딜러 수수료를 계산한다.
//				for ( int start = 0; start < playersResults.size(); ++start ) {
//					auto player = GetPlayerIdx( playersResults[ start ].player_idx() );
//					if ( player->GetLostMoney() >= beforeSidePotBase )
//					{
//						uint64 plusMoney = 0;
//						if ( player->GetLostMoney() >= sidePot->m_sidePotBase )
//							plusMoney = sidePot->m_sidePotBase - beforeSidePotBase;
//						else
//							plusMoney = player->GetLostMoney() - beforeSidePotBase;
//
//						getMoney += plusMoney;
//
//						// 자신의 돈외에는 딜러 수수료 부과
//						if ( playerIdx != playersResults[ start ].player_idx() )
//							taxMoney += plusMoney;
//					}
//				}
//
//				// Die 플레이어의 돈을 나눈다.
//				/*for ( auto& diePot : diePlayersPots ) {
//					if ( diePot >= getMoney )
//					{
//						getMoney += getMoney;
//						diePot -= getMoney;
//					}
//					else
//					{
//						if ( diePot > 0 ) {
//							getMoney += diePot;
//							diePot = 0;
//						}
//					}
//				}*/
//
//				beforeSidePotBase = sidePot->m_sidePotBase;
//				potMoney -= getMoney;
//
//				// 수수료를 부과한다.
//				//uint64 nonTaxMoney = getMoney - taxMoney;
//				uint64 dealerFee = taxMoney * m_dealerFeeRate;
//
//				playerResult.set_fund_after( GetPlayerMoney( pClientSession ) + getMoney - dealerFee );
//				playerResult.set_rake_cut( dealerFee );
//
//				// 결과 처리를 위해 저장해둔다.
//				playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
//
//				// potMoney 가 더이상 없다면 종료한다.
//				if ( potMoney <= 0 )
//					break;
//			}
//		}
//	}
//	// 승리자가 여려명인데, playersResults 사이즈와 동일한 경우 공동우승
//	// 나가리 판
//	else if ( winners.size() == playersResults.size() )
//	{
//		// 본인이 손실한 금액을 그대로 다시 셋팅해주고 끝낸다.
//		for ( auto& playerResult : playersResults ) {
//			const uint64 playerIdx = playerResult.member_idx();
//			playerResult.set_fund_after( playerResult.fund_before() );
//			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
//		}
//	}
//	// 승리자가 여러명
//	else
//	{
//		// playersResults 에 패배자를 포함시킨다.
//		for ( auto& playerResult : sortedLosers )
//			playersResults.push_back( playerResult );
//
//		std::vector<uint64> diePlayerPots;
//
//		std::vector<cSidePot*> winnerSides = cSidePotManager::GetSides( winners );
//
//		// winnerSides 사이드가 있는 경우
//		if ( winnerSides.size() != 0 )
//		{
//			std::vector<PmNet::MemberOutcome> newPlayersResults;
//			std::map<uint64 , uint64> newPlayersResultMap;
//
//			// newPlayersResults 의 정렬
//			// Side 있는 승리자, Side 없는 승리자, 그외 나머지 순으로 정렬
//			{
//				// 승리자 사이드 목록에 있는 승리자
//				for ( auto sidePot : winnerSides ) {
//					for ( auto& playerResult : playersResults ) {
//						if ( sidePot->m_sidePotPlayerIdx == playerResult.member_idx() ) {
//							newPlayersResults.push_back( playerResult );
//							newPlayersResultMap.insert(std::pair<uint64, uint64>( playerResult.member_idx() , playerResult.member_idx() ));
//						}
//
//					}
//				}
//
//				// 승리자 사이드 목록에 없는 승리자
//				for ( auto& playerIdx : winners ) {
//					auto sidePot = cSidePotManager::GetSide( playerIdx );
//					if ( sidePot == nullptr ) {
//						for ( auto& playerResult : playersResults ) {
//							if ( playerIdx == playerResult.member_idx() ) {
//								newPlayersResults.push_back( playerResult );
//								newPlayersResultMap.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
//							}
//						}
//					}
//				}
//
//				// newPlayersResults 목록에 포함 안된 플레이어
//				for ( auto& playerResult : playersResults ) {
//					auto iterFind = newPlayersResultMap.find( playerResult.member_idx() );
//					if ( iterFind == newPlayersResultMap.end() ) {
//						const uint64 playerIdx = playerResult.member_idx();
//						cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//						playerResult.set_fund_after( GetPlayerMoney( pClientSession ) );
//						newPlayersResults.push_back( playerResult );
//					}
//				}
//
//				// Die Player 들의 팟을 계산한다. winners size 뒤에 플레이어들이 Die 유저들임
//				// TODO : 아직 못 끝냈다. lostMoney 를 플레이어 숫자에 따라 나누고 분배해주어야 한다.
//				// Side 팟에다가 사이드에 해당하는 나눌돈을 셋팅해주고, 나눌 사람을 카운팅해 둔다.
//				// 미리 Pot 에서 패배자의 돈을 뺀상태에서 처리해본다.
//				std::vector<uint64> diePlayerPots;
//				uint64 beforeSidePotBase = 0;
//				for ( int m = 0; m < winnerSides.size(); ++m )
//				{
//					cSidePot* sidePot = winnerSides[ m ];
//
//					// 패배자들의 나눌돈 계산
//					for ( int n = winners.size(); n < newPlayersResults.size(); ++n )
//					{
//						const uint64 playerIdx = newPlayersResults[ n ].player_idx();
//						cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//						if ( beforeSidePotBase < pClientSession->GetLostMoney() && pClientSession->GetLostMoney() <= sidePot->m_sidePotBase )
//						{
//							uint64 lostMoney = pClientSession->GetLostMoney() - beforeSidePotBase;
//
//							// lostMoney 을 cSidePot 에 저장해둔다.
//							// Pot 에서 lostMoney 를 미리 빼둔다.
//							sidePot->PushMoney( lostMoney );
//							potMoney -= lostMoney;
//
//							diePlayerPots.push_back( lostMoney );
//						}
//					}
//
//					// 나눌돈이 있을경우 나눠줄 사람 카운트
//					for ( int n = 0; n < winners.size(); ++n )
//					{
//						const uint64 playerIdx = newPlayersResults[ n ].player_idx();
//						cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//						if ( pClientSession->GetLostMoney() >= sidePot->m_sidePotBase )
//						{
//							sidePot->IncreaseSidePlayer();
//						}
//					}
//
//					beforeSidePotBase = sidePot->m_sidePotBase;
//				}
//
//				beforeSidePotBase = 0;
//
//				// side 팟 금액을 승자들에게 모두 주어야한다.
//				// 0 ~ 4, 1 ~ 4, 2 ~ 4, 3 ~ 4
//				std::map<uint64, uint64> remainPlayers;
//				for ( int n = 0; n < winners.size(); ++n ) {
//
//					cSidePot* sidePot = cSidePotManager::GetSide( winners[ n ]);
//
//					if ( sidePot == nullptr )
//					{
//						remainPlayers.insert( std::pair<uint64 , uint64>( winners[ n ] , winners[ n ] ) );
//						continue;
//
//						// TODO : 아직 못 끝냈다. 이긴사람중에 사이드 있는 유저 처리하고 두명이상이 남은 경우의 분배처리
//						auto& playerResult = newPlayersResults[ n ];
//
//						const uint64 playerIdx = playerResult.member_idx();
//						cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//						uint64 beforeMoney = playerResult.fund_before();
//						uint64 testMoney = playerResult.fund_after();
//						uint64 afterMoney = GetPlayerMoney( pClientSession ) + potMoney;
//
//						uint64 dealerFee = 0;
//						if ( afterMoney > beforeMoney ) {
//							dealerFee = ( afterMoney - beforeMoney ) * m_dealerFeeRate;
//						}
//
//						// 사이드 팟에 있던 진사람 돈 분배
//						double loserMoney = 0;
//						for ( auto loserSides : winnerSides ) {
//							loserMoney += loserSides->GetSideMoneyPer();
//						}
//						double loserMoneyDealerFee = loserMoney * m_dealerFeeRate;
//						loserMoney = loserMoney - loserMoneyDealerFee;
//
//						// TODO 남은 사람들중에 동일한 포인트가 존재 한다면?? 나누어야 하나?
//						playerResult.set_fund_after( GetPlayerMoney( pClientSession ) + loserMoney + potMoney - dealerFee );
//						playerResult.set_rake_cut( dealerFee + loserMoneyDealerFee );
//						playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
//						break;
//					}
//					else
//					{
//						// winners 가 4면
//						// n == 0 일때는 newPlayersResults 0 부터 3  까지
//						// n == 1 일때는 newPlayersResults 1 부터 3  까지
//						// n == 2 일때는 newPlayersResults 2 부터 3  까지
//						// n == 3 일때는 newPlayersResults 3 처리
//						//for ( int m = n; m < winners.size() - 1; ++m ) {
//						for ( int m = 0; m < winners.size(); ++m ) {
//							auto& playerResult = newPlayersResults[ m ];
//
//							const uint64 playerIdx = playerResult.member_idx();
//							cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//							// 내가 잃은 돈이 sidePot->m_sidePotBase 보다 큰 경우만 지급
//							if ( pClientSession->GetLostMoney() >= sidePot->m_sidePotBase )
//							{
//								uint64 curAfterMoney = playerResult.fund_after();
//
//								uint64 plusMoney = sidePot->m_sidePotBase - beforeSidePotBase;
//								potMoney -= plusMoney; // pot 금액 처리
//
//								uint64 curDealerFee = playerResult.rake_cut();
//								double dealerFee = sidePot->GetSideMoneyPer() * m_dealerFeeRate;
//								double loserMoney = sidePot->GetSideMoneyPer() - dealerFee; // 딜러비용 공제하고 더해준다.
//								playerResult.set_rake_cut( curDealerFee + dealerFee );
//
//								curAfterMoney = curAfterMoney + loserMoney + ( sidePot->m_sidePotBase - beforeSidePotBase );
//								playerResult.set_fund_after( curAfterMoney );
//							}
//						}
//
//						beforeSidePotBase = sidePot->m_sidePotBase;
//
//						// potMoney 가 더 이상 없으면 종료
//						if ( potMoney <= 0 )
//							break;
//					}
//				}
//
//				// 팟머니가 남았다.
//				// 남은 팟머니를 remainWinners 들에게 분배하고 끝낸다.
//				// 사이드 없는 승리자들에게 분배하고 끝낸다.
//				// 남은 사이드 팟 머니에 대해 딜러 Fee는??
//				if ( potMoney > 0 ) {
//					uint64 plusMoney = potMoney / remainPlayers.size();
//
//					for ( int n = 0; n < newPlayersResults.size(); ++n ) {
//						auto& playerResult = newPlayersResults[ n ];
//
//						const uint64 playerIdx = playerResult.member_idx();
//
//						auto iterFind = remainPlayers.find( playerIdx );
//						if ( iterFind != remainPlayers.end() ) {
//							cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//							uint64 afterMoney = playerResult.fund_after();
//							playerResult.set_fund_after( afterMoney + plusMoney );
//						}
//
//
//					}
//				}
//
//				// 결과 처리
//				for ( int n = 0; n < winners.size(); ++n ) {
//					auto& playerResult = newPlayersResults[ n ];
//					playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerResult.member_idx() , playerResult ) );
//				}
//			}
//		}
//		// 승리자가 사이드가 없다. N빵 하고 끝
//		else
//		{
//			// 승리한 사람들이 N 빵
//			uint64 getMoney = potMoney / winners.size();
//
//			for ( int n = 0; n < playersResults.size(); ++n ) {
//				auto& playerResult = playersResults[ n ];
//				const uint64& playerIdx = playerResult.member_idx();
//				cClientSession* pClientSession = GetPlayerIdx( playerIdx );
//
//				// winners.size() - 1 까지는 승자, 나머지 패자
//				if ( n <= winners.size() - 1 ) {
//					uint64 dealerFee = getMoney - pClientSession->GetLostMoney();
//					dealerFee = dealerFee * m_dealerFeeRate;
//					playerResult.set_fund_after( GetPlayerMoney( pClientSession ) + getMoney - dealerFee );
//					playerResult.set_rake_cut( dealerFee );
//				}
//
//				playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
//			}
//		}
//	}
//}

// ASC 정렬
// bool 값은 0 부터, isAllIn 인경우 1 이기 때문에, 정렬은 역순으로
bool cHoldem::CompareResultServer( stResultServer& result1 , stResultServer& result2 )
{
	if ( result1.session->isAllIn() > result2.session->isAllIn() )
		return true;

	if ( result1.session->isAllIn() < result2.session->isAllIn() )
		return false;

	if ( result1.point < result2.point )
		return true;

	return false;
}


// 유저 데이터 저장
void cHoldem::SaveUserData(bool bResult)
{
	/*for ( auto player : m_playerSlots ) {
		if ( player == nullptr )
			continue;

		player->SavePlayer();
	}*/

	// 모든 SavePlayer() 비동기 작업을 시작하고 결과를 추적할 벡터
	std::vector<std::future<BOOL>> results;
	for ( auto& player : m_playerSlots ) {
		if ( player == nullptr )
			continue;

		results.push_back( player->SavePlayer() );
		results.push_back( player->UpdateHoldemRecords() );
		results.push_back( player->UpdateJokboRecord() );
	}

	// 2024.07.23 게임레코드 추가
	// 게임레코드 저장
	if ( true == bResult )
	{
		// SKEY
		std::ostringstream jsonStream;
		jsonStream << "{\"skey\":[";
		bool firstPlayer = true;
		for ( int i = 0; i < m_maxPlayerCnt; ++i )
		{
			if ( p_id[ i ].empty() )
				continue;

			if ( !firstPlayer ) {
				jsonStream << ",";
			}
			else {
				firstPlayer = false;
			}

			jsonStream << "\"" << p_id[ i ] << "\"";
		}

		jsonStream << "]}";

		auto log_result = QueryManager::InsertGameRecordHoldemRecord(
		20402 , // code
		( int ) General::PlayCategory::PlayCategory_TexasHoldem , // game_type
		m_gameUid , // uip
		std::to_string( m_pGameRoom->GetRoomNumber() ) , // 게임방서버id
		m_pGameRoom->m_roomInfo.played_rounds() + 1 ,
		"" , // logver 공백으로 넣기
		game_record_string , // gamerecord
		jsonStream.str() ); // skey 참여한플레이어id
		log_result.wait();
		if ( FALSE == log_result.get() ) {
			// 로그 저장 실패

		}
	}

	// 모든 비동기 작업이 완료될 때까지 대기
	for ( auto& future : results ) {
		future.wait();
	}

	// 모든 결과를 검사하고 추가 작업을 수행
	for ( auto& future : results ) {
		if ( future.get() ) {
			// 작업이 성공한 경우에 수행할 동작
		}
		else
		{
			// 작업 실패한 경우의 로그
			int n = 0;
		}
	}

	for ( auto& player : m_playerSlots )
	{
		if ( player == nullptr )
			continue;

		player->SyncFriend_Online( player , General::ContactState::ContactState_Hidden , GetChannelId() );
	}
}

// 방에 진입과, 나가기는 방이 대기중일 때만 가능한다.
bool cHoldem::LeaveSlot( cClientSession* pClientSession , bool b_out )
{
	// 보스 및 마스터의 경우는 플레이 중일때는 갱신하지 않도록 수정한다.
	if ( m_gameStep == Server::PlayPhase::PlayPhase_Waiting || m_gameStep == Server::PlayPhase::PlayPhase_StartReady ) {

		// 보스가 나가는 것인지 확인
		if ( isBoss( pClientSession->GetPlayerIdx() ) ) {
			bool forceChange = true;
			if( b_out )
				DecideDealer( pClientSession->GetPlayerIdx() , forceChange );
			else
				DecideDealer( forceChange );
		}
	}

	// 마스터가 나가는 것인지 확인
	if ( isMaster( pClientSession ) ) {
		m_joinProhibited.clear();
		NextMaster();
		SendMasterChange( pClientSession->GetPlayerIdx() , b_out );
	}
	pClientSession->GetPlayerExtRef().set_kickout_cooldown_until( TimeUtils::GetCurrentDateTime() );
	for ( int slot = 0; slot < m_maxPlayerCnt; ++slot ) {
		auto player = m_playerSlots[ slot ];
		if ( player == nullptr )
			continue;


		// 주소값 비교
		if ( player->GetPlayerIdx() != pClientSession->GetPlayerIdx() )
			continue;
		// 같은 유저이니 지워 준다.
		m_playerSlots[ slot ] = nullptr;
		player->SetAllIn( false );
		player->SetDie( false );
		player->SetSide( false );
		// 방인원이 시작 할 수 있는 인원 보다 작을때만 Wait 으로 돌아간다.

		if ( m_gameStep == Server::PlayPhase::PlayPhase_Waiting || m_gameStep == Server::PlayPhase::PlayPhase_StartReady )
		{
			if ( GetMemberCnt() < GetMinStartPlayerCount() ) {
				Server::PlayPhase before = m_gameStep;
				MakeRoomStatusWait();
				SendStatusChange( before , "cHoldem::LeaveSlot" , pClientSession->GetPlayerIdx() );
			}
			/*if ( b_out )
				MakeParticipate( pClientSession->GetPlayerIdx() );
			else
				MakeParticipate();*/
		}


		m_autoStartTick = ::GetTickCount64() + static_cast< ULONGLONG >( ( 24 * 3600 * 1000 ) ); // 1명이라도 나가 버리면 AutoStart 불가능 상태로 재 설정
		return true;
	}
	return false;
}

// 더미랑 스왑
bool cHoldem::SwapWithDummy( cClientSession* pClientSession , cClientSession* pDummyClientSession )
{
	SaveUserData();
	if ( pDummyClientSession == nullptr )//진입
	{
		for ( int n = 0; n < m_betSequence.size(); ++n ) {
			auto player = m_betSequence[ n ];
			if ( player == nullptr )
				continue;
			if ( player->GetPlayerIdx() == pClientSession->GetPlayerIdx() )
			{
				return true;
			}
		}

		//for ( int slot = 0; slot < m_maxPlayerCnt; ++slot ) {
		//	auto player = m_playerSlots[ slot ];
		//	if ( player == nullptr )
		//		continue;

		//	if ( player->GetPlayerIdx() != pClientSession->GetPlayerIdx() )
		//		continue;

		//	{
		//		// 같은 유저이니 지워 준다.
		//		m_playerSlots[ slot ] = nullptr;
		//		m_playerSlots[ slot ] = pClientSession;
		//		return true;
		//	}
		//}
	}
	else//나가기
	{
		for ( int i = 0; i < m_playerSlots.size(); ++i )
		{
			if ( nullptr == m_playerSlots[ i ] )
				continue;

			if ( pClientSession->GetPlayerIdx() == m_playerSlots[ i ]->GetPlayerIdx() )
			{
				pDummyClientSession->SetBeforeSlotIndex( i );
				break;
			}
		}

		for ( int n = 0; n < m_betSequence.size(); ++n ) {
			auto player = m_betSequence[ n ];
			if ( player == nullptr )
				continue;
			if ( player->GetPlayerIdx() == pClientSession->GetPlayerIdx() )
			{
				m_betSequence[ n ] = pDummyClientSession;
				return true;
			}
		}
	}
	return false;
}

// 보스가 슬롯을 변경하는 경우에는 누가 보스를 잡게 할것인지 기획필요
// 마스터의 경우에는 마스터의 슬롯을 변경해주면 됨
General::ResultCode cHoldem::MoveSlot( cClientSession* pClientSession , const int slotNumber )
{
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_Waiting:
	case Server::PlayPhase::PlayPhase_StartReady:
		break;
	default:
		return General::ResultCode::Result_PlayStepMismatch;
	}

	// 방에서 나가기로 예약한 상태라면 슬롯 이동 할 수 없다.
	if ( pClientSession->GetReserved() )
		return General::ResultCode::Result_ActionRejected; // 같은 슬롯으로 이동하려고 하였다.

	if ( false == pClientSession->IsRemainMoveSlotCount() )
		return General::ResultCode::Result_SeatMoveLimitReached; // 더 이상 슬롯을 이동 할 수 없다.

	const uint64& playerIdx = pClientSession->GetPlayerIdx();

	// m_playerSlots 에서 몇번째 슬롯인지 알아온다.
	int searchSlot = 0;
	for ( int n = 0; n < m_playerSlots.size(); ++n ) {
		auto player = m_playerSlots[ n ];
		if ( player == nullptr ) continue;
		if ( player->GetPlayerIdx() == playerIdx ) {
			searchSlot = n + 1;
			break;
		}
	}

	// searchSlot 번호는 1번부터 시작한다.
	if ( searchSlot == 0 )
		return General::ResultCode::Result_EmptySeatLookupFailed;

	if ( m_playerSlots[ slotNumber - 1 ] != nullptr )
		return General::ResultCode::Result_SeatTargetOccupied; // 비어 있지 않은 슬롯으로 이동하려고 하였다.

	if ( slotNumber == searchSlot )
		return General::ResultCode::Result_ActionRejected; // 같은 슬롯으로 이동하려고 하였다.

	// 마스터가 슬롯을 변경하는 경우
	bool isMasterMove = false;
	if ( isMaster( pClientSession ) ) {
		m_masterIter = m_playerSlots.begin() + ( slotNumber - 1 );
		isMasterMove = true;
	}

	// 현재 슬롯 nullptr 로 변경
	m_playerSlots[ searchSlot - 1 ] = nullptr;
	m_playerSlots[ slotNumber - 1 ] = pClientSession;

	// 슬롯 변경 알림

	// 마스터 변경 알림
	if ( isMasterMove )
		SendMasterChange();

	pClientSession->MinusMoveSlotCount();

	return General::ResultCode::Result_Success;
}

//// 예약자를 플레이어로 변경한다.
//// 예약자가 있을때 RoomJoinRes 를 전체에게 내려준다.
//// 예약자에 대해서는 예약 당시에 입장조건을 체크하고, 여기에서는 이동만 처리한다.
//void cHoldem::OnMoveSlot()
//{
//	for ( int slot = 0; slot < HOLDEM_SLOT_COUNT; ++slot ) {
//
//		if ( m_slotReservation[ slot ] == nullptr )
//			continue;
//
//		auto reservationPlayer = m_slotReservation[ slot ];
//		const uint64& playerIdx = reservationPlayer->GetPlayerIdx();
//
//		if ( m_playerSlots[ slot ] != nullptr ) {
//			std::string errorInfo = std::format( "cHoldem::OnMoveSlot() m_playerSlots not nullptr slot number {} Exception #1" , slot + 1 );
//			throw std::runtime_error( errorInfo );
//		}
//
//		if ( General::ResultCode::Result_Success == m_pGameRoom->RoomJoin( reservationPlayer , slot + 1 ) ) {
//
//			// 예약 정보 삭제
//			m_slotReservation[ slot ] = nullptr;
//			//m_playerSlots[ slot ] = reservationPlayer;
//
//			// Watcher 에 등록되어 있으면 삭제한다.
//			m_pGameRoom->RemoveWatcher( playerIdx );
//
//		}
//		else
//		{
//			std::string errorInfo = std::format( "cHoldem::OnMoveSlot() m_playerSlots not nullptr slot number {} Exception #2" , slot + 1 );
//			throw std::runtime_error( errorInfo );
//		}
//	}
//
//	PmNet::ChamberEnterRS response;
//	m_pGameRoom->GetRoomJoinResponseDetail( response );
//
//	// 방전체에 통보
//	m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );
//}

// 점유한 슬롯이 있는지 확인
bool cHoldem::HasSlot( cClientSession* pClientSession )
{
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr ) continue;
		if ( player == pClientSession || player->GetPlayerIdx() == pClientSession->GetPlayerIdx() )
			return true;
	}
	return false;
}

// 예약자를 플레이어로 변경한다.
// 예약자가 있을때 RoomJoinRes 를 전체에게 내려준다.
// 예약자에 대해서는 예약 당시에 입장조건을 체크하고, 여기에서는 이동만 처리한다.
void cHoldem::OnSlotReservation()
{
	int participatersCount = 0;

	for ( int slot = 0; slot < m_maxPlayerCnt; ++slot ) {

		if ( m_slotReservation[ slot ] == nullptr )
			continue;

		auto reservationPlayer = m_slotReservation[ slot ];
		const uint64& playerIdx = reservationPlayer->GetPlayerIdx();

		if ( m_playerSlots[ slot ] != nullptr ) {
			continue;
			/*std::string errorInfo = std::format( "cHoldem::OnSlotReservation() m_playerSlots not nullptr slot number {} Exception #1" , slot + 1 );
			throw std::runtime_error( errorInfo );*/
		}

		General::ResultCode _errorCode = m_pGameRoom->RoomJoin( reservationPlayer , slot + 1 );
		if ( _errorCode == General::ResultCode::Result_Success ) {

			// 게임 데이터 초기화
			reservationPlayer->GameReset();

			// 예약 정보 삭제
			m_slotReservation[ slot ] = nullptr;

			m_pGameRoom->RemoveWatcher( playerIdx );



			++participatersCount;
		}
		else
		{
			// 예약 정보 삭제
			m_slotReservation[ slot ] = nullptr;
			PmNet::ChamberLeaveRS _res;
			_res.set_member_idx( playerIdx );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
			continue;
			/*std::string errorInfo = std::format( "cHoldem::OnSlotReservation() m_playerSlots not nullptr slot number {} Exception #2" , slot + 1 );
			throw std::runtime_error( errorInfo );*/
		}
	}

	// 대기자가 있던 경우에믄 보낸다.
	if ( participatersCount > 0 ) {

		m_pGameRoom->m_roomInfo.set_pot_amount( 0 );

		PmNet::ChamberEnterRS response;
		m_pGameRoom->GetGameRoomDetail( response );

		// 방전체에 통보
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );
	}
}

// slotNumber 가 있는 경우에는 slotNumber 에 예약
General::ResultCode cHoldem::SlotReservation( cClientSession* pClientSession , const int slotNumber )
{
	if ( pClientSession == nullptr )
		return General::ResultCode::Result_UnexpectedCondition; // throw 할지 이렇게 처리할지 고민

	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->isban( pClientSession->GetPlayerIdx() ) )
	{
		return General::ResultCode::Result_ActionRejected;
	}

	if ( HasSlotReservation( pClientSession ) )
		return General::ResultCode::Result_PlayReservationAlreadyExists;

	/*if( GetReservationPlayerPlayerCount() >= MAXRESERVECOUNT )
		return General::ResultCode::Result_SeatReservationCapacityReached;*/

	General::RoomAccessMode roomType = m_pGameRoom->m_roomInfo.access_mode();
	m_pGameRoom->m_roomInfo.asset_kind();
	uint64 seedMoney = m_pGameRoom->m_roomInfo.seed_amount();
	if ( seedMoney > GetPlayerMoney( pClientSession ) )
		return General::ResultCode::Result_BalanceInsufficient;

	// slotNumber 가 없는 경우에는 앞에서 부터 채운다.
	if ( slotNumber == 0 )
	{
		// 예약되지 않은 슬롯 검색
		int searchSlotNumber = -1;
		for ( int n = 0; n < m_pGameRoom->GetMaxRoomPlayerCnt(); ++n )
		{
			if ( m_playerSlots[ n ] != nullptr ) continue;
			if ( m_slotReservation[ n ] != nullptr ) continue;

			//if ( m_playerSlots[ n ]->GetPlayerIdx() == pClientSession->GetPlayerIdx() ) return General::ResultCode::Result_PlayerSeatAlreadyJoined;
			//if ( m_slotReservation[ n ]->GetPlayerIdx() == pClientSession->GetPlayerIdx() ) return General::ResultCode::Result_PlayReservationAlreadyExists;

			searchSlotNumber = n + 1;
			break;
		}

		if ( searchSlotNumber == -1 )
			return General::ResultCode::Result_EmptySeatLookupFailed;

		// 예약 처리
		m_slotReservation[ searchSlotNumber - 1 ] = pClientSession;
	}
	else
	{
		// slotNumber 범위 체크
		if ( slotNumber < 1 || slotNumber > m_maxPlayerCnt )
			return General::ResultCode::Result_EmptySeatLookupFailed;

		if ( m_playerSlots[ slotNumber - 1 ] != nullptr )
			return General::ResultCode::Result_SeatTargetOccupied;

		if ( m_slotReservation[ slotNumber - 1 ] != nullptr )
			return General::ResultCode::Result_SeatTargetOccupied;

		m_slotReservation[ slotNumber - 1 ] = pClientSession;
	}

	// 관전자 카운트 알림
	/*PmNet::InformObserverCntRS _watcher_res;
	int reservedCount = GetReservationPlayerPlayerCount();
	_watcher_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() - reservedCount );
	m_pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

	// 예약자 목록을 내려준다.

	m_pGameRoom->GetWatcherCount();

	return General::ResultCode::Result_Success;
}

bool cHoldem::HasSlotReservation( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr ) return false;

	for ( auto player : m_slotReservation ) {
		if ( player == nullptr ) continue;
		if ( player->GetPlayerIdx() == pClientSession->GetPlayerIdx() )
			return true;
	}
	return false;
}

bool cHoldem::RemoveReservation( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr ) return false;

	for ( int n = 0; n < m_maxPlayerCnt; ++n ) {
		cClientSession* player = m_slotReservation[ n ];
		if ( player == nullptr )
			continue;

		// 예약 취소
		if ( player->GetPlayerIdx() == pClientSession->GetPlayerIdx() ) {
			m_slotReservation[ n ] = nullptr;
			return true;
		}
	}
	return false;
}

bool cHoldem::RemoveParticipationQueue( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr ) return false;

	for ( auto iter = m_participateQueue.begin(); iter != m_participateQueue.end(); ++iter ) {

		if ( *iter == nullptr ) continue;
		if ( ( *iter )->GetPlayerIdx() == pClientSession->GetPlayerIdx() ) {
		//if ( ( *iter )== pClientSession ) {
			m_participateQueue.erase( iter );
			return true;
		}
	}
	return false;
}

// 관전자 상태
// 풀방이라서 큐에 등록 되어 있던 상태
void cHoldem::MakeParticipate(int64 except_playerIdx )
{
	participatelock = true;
	const int participatersCount = m_participateQueue.size();

	for ( auto iter = m_participateQueue.begin(); iter != m_participateQueue.end(); ) 
	{
		auto participateRequester = *iter;
		if ( participateRequester == nullptr ) 
		{
			iter = m_participateQueue.erase( iter );
			continue;
		}

		const uint64_t playerIdx = participateRequester->GetPlayerIdx();

		if ( participateRequester->GetJoinedRoomNumber() != m_pGameRoom->GetRoomNumber() )
		{
			for ( auto iter = m_participateQueue.begin(); iter != m_participateQueue.end();) 
			{
				if ( ( *iter )->GetPlayerIdx() == playerIdx )
				{
					iter = m_participateQueue.erase( iter );
				}
				else
				{
					++iter;
				}
			}

			iter = m_participateQueue.begin();

			continue;
		}

		// 게임 데이터 초기화
		participateRequester->GameReset();

		if ( General::ResultCode::Result_Success == m_pGameRoom->RoomJoin( participateRequester ) ) 
		{
			std::string log = std::format(
				"MakeParticipate playeridx: {} Room: {}" ,
				playerIdx , m_pGameRoom->GetRoomNumber()
			);

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
			iter = m_participateQueue.erase( iter );  // erase 후 새로운 반복자 반환
			m_pGameRoom->RemoveWatcher( playerIdx );   // Watcher에 등록되어 있으면 삭제
		}
		else {

			++iter;  // 삭제하지 않는 경우에만 반복자 증가
		}
	}

	// 대기자가 있던 경우에믄 보낸다.
	if ( participatersCount > 0 ) {

		m_pGameRoom->m_roomInfo.set_pot_amount( 0 );
		if ( GetMemberCnt() >= GetMinStartPlayerCount() ) {

			m_gameStep = Server::PlayPhase::PlayPhase_Waiting;
			m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;
			m_pGameRoom->m_roomInfo.set_room_state( General::RoomState::RoomState_Waiting );
		}

		PmNet::ChamberEnterRS response;
		m_pGameRoom->GetGameRoomDetail( response );

		// 방전체에 통보
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response ,except_playerIdx);
	}
	participatelock = false;
}
void cHoldem::MakeParticipateOnplay( int64 except_playerIdx )
{
	if ( participatelock )
		return;
	const int participatersCount = m_participateQueue.size();
	auto gameInstance = m_pGameRoom->GetGameInterface();

	int reservedCount = gameInstance->GetReservationPlayerPlayerCount();
	int playerCount = gameInstance->GetMemberCnt();
	int maxPlayerCount = m_pGameRoom->GetMaxRoomPlayerCnt();
	for ( auto iter = m_participateQueue.begin(); iter != m_participateQueue.end(); ) {
		if ( reservedCount + playerCount >= maxPlayerCount )
			return;
		auto participateRequester = *iter;

		if ( participateRequester == nullptr ) {
			iter = m_participateQueue.erase( iter ); // nullptr 요소는 바로 삭제
			continue;
		}

		// 다른방에있는데 여기 참가신청 할라해서 삭제해버림
		if( participateRequester->GetJoinedRoomNumber() != m_pGameRoom->GetRoomNumber() ) {
			iter = m_participateQueue.erase( iter );
			continue;
		}

		const uint64_t playerIdx = participateRequester->GetPlayerIdx();

		// 게임 데이터 초기화 (필요하다면 주석 해제)
		participateRequester->GameReset();

		if ( General::ResultCode::Result_Success == gameInstance->SlotReservation( participateRequester ) ) {
			iter = m_participateQueue.erase( iter );  // 성공적으로 처리된 요청 삭제

			// 대기자가 있던 경우 방에 통보
			PmNet::ChamberEnterRS response;
			m_pGameRoom->GetGameRoomDetail( response , true );

			// 방 전체에 통보
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response );

			// 로그 기록
			std::string log = std::format(
				"MakeParticipateOnplay playeridx: {} Room: {}" ,
				playerIdx , m_pGameRoom->GetRoomNumber()
			);

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
		}
		else {
			++iter; // 실패한 경우 다음 요소로 진행
		}
	}


	//// 대기자가 있던 경우에믄 보낸다.
	//if ( participatersCount > 0 ) {

	//	m_pGameRoom->m_roomInfo.set_pot( 0 );
	//	if ( GetMemberCnt() >= GetMinStartPlayerCount() ) {

	//		m_gameStep = Server::PlayPhase::PlayPhase_Waiting;
	//		m_expireTick = ::GetTickCount64() + NetLib::cSingleton<cDataLoader>::GetInstance()->LowBadukiWait;
	//		m_pGameRoom->m_roomInfo.set_room_status( General::RoomState::RoomState_Waiting );
	//	}

	//	PmNet::ChamberEnterRS response;
	//	m_pGameRoom->GetGameRoomDetail( response );

	//	// 방전체에 통보
	//	m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceEnter , response , except_playerIdx );
	//}
}

// 플레이중인 유저가 관전 모드로 갈때만 사용한다.
// Wait, StartWait 일때만 전환 가능
General::ResultCode cHoldem::TransferWatcher( cClientSession* pClientSession )
{
	switch ( m_gameStep )
	{
	case Server::PlayPhase::PlayPhase_Waiting:
	case Server::PlayPhase::PlayPhase_StartReady:
	{
		// 즉시 관전자로 변경
		if ( false == LeaveSlot( pClientSession ) )
			return General::ResultCode::Result_ActionRejected;

		// 자리 이동 예약된 것이 있다면 삭제
		if ( RemoveReservation( pClientSession ) ) {

			PmNet::DropMemberOnSeatRS remove_response;
			remove_response.set_member_idx( pClientSession->GetPlayerIdx() );
			m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_SlotSeatClear , remove_response );
		}

		m_pGameRoom->RemovePlayer( pClientSession->GetPlayerIdx() );
		m_pGameRoom->RegisterWatcher( pClientSession );

		PmNet::ToObserverShiftRS response;
		response.set_chamber_no( m_pGameRoom->GetRoomNumber() );
		response.set_member_idx( pClientSession->GetPlayerIdx() );

		m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_WatcherModeEnter , response );
		// 관전자 카운트 알림
		/*PmNet::InformObserverCntRS _watcher_res;
		int reservedCount = GetReservationPlayerPlayerCount();
		_watcher_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() - reservedCount );
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/
		return General::ResultCode::Result_Success;
	}
	break;
	default:
	{
		// 자리 이동 예약된 것이 있다면 삭제
		if ( RemoveReservation( pClientSession ) ) {

			PmNet::DropMemberOnSeatRS remove_response;
			remove_response.set_member_idx( pClientSession->GetPlayerIdx() );
			m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_SlotSeatClear , remove_response );

			PmNet::ToObserverShiftRS response;
			response.set_chamber_no( m_pGameRoom->GetRoomNumber() );
			response.set_member_idx( pClientSession->GetPlayerIdx() );

			m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_WatcherModeEnter , response );
			// 관전자 카운트 알림
			/*PmNet::InformObserverCntRS _watcher_res;
			int reservedCount = GetReservationPlayerPlayerCount();
			_watcher_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() - reservedCount );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/
			return General::ResultCode::Result_Success;
		}
		else {

			// 관전 예약
			const auto& errorCode = WatcherReservation( pClientSession );
			if ( errorCode != General::ResultCode::Result_Success )
				return errorCode;

			PmNet::ToObserverShiftRS response;
			response.set_chamber_no( m_pGameRoom->GetRoomNumber() );
			response.set_member_idx( pClientSession->GetPlayerIdx() );
			response.set_view_hold_ok( true );

			m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_WatcherModeEnter , response );
			// 관전자 카운트 알림
			/*PmNet::InformObserverCntRS _watcher_res;
			int reservedCount = GetReservationPlayerPlayerCount();
			_watcher_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() - reservedCount );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/
			return General::ResultCode::Result_Success;
		}
	}
	}

	return General::ResultCode::Result_PlayStepMismatch;
}

void cHoldem::OnWatcherReservation()
{
	for ( auto iter = m_watcherQueue.begin(); iter != m_watcherQueue.end(); )
	{
		const uint64& playerIdx = iter->first;
		auto player = iter->second;

		if ( player == nullptr )
		{
			++iter;
			continue;
		}

		// 플레이어에서 삭제
		m_pGameRoom->RemovePlayer( player->GetPlayerIdx() );

		// 관전자 등록
		m_pGameRoom->RegisterWatcher( player );

		PmNet::ToObserverShiftRS response;
		response.set_chamber_no( m_pGameRoom->GetRoomNumber() );
		response.set_member_idx( player->GetPlayerIdx() );

		m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_WatcherModeEnter , response );
		// 관전자 카운트 알림
		/*PmNet::InformObserverCntRS _watcher_res;
		int reservedCount = GetReservationPlayerPlayerCount();
		_watcher_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() - reservedCount );
		m_pGameRoom->BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

		LeaveSlot( player );

		// 삭제
		iter = m_watcherQueue.erase( iter );
	}
}

General::ResultCode cHoldem::WatcherReservation( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr ) return General::ResultCode::Result_WatchReservationFailed;

	uint64 playerIdx = pClientSession->GetPlayerIdx();

	auto iter = m_watcherQueue.find( playerIdx );
	if ( iter != m_watcherQueue.end() )
		return General::ResultCode::Result_WatchReservationAlreadyExists; // 이미 예약

	m_watcherQueue.insert( std::pair<uint64 , cClientSession*>( playerIdx , pClientSession ) );

	return General::ResultCode::Result_Success;
}

General::ResultCode cHoldem::CancelWatcherReservation( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr ) return General::ResultCode::Result_WatchReservationCancelFailed;

	uint64 playerIdx = pClientSession->GetPlayerIdx();

	auto iter = m_watcherQueue.find( playerIdx );
	if ( iter == m_watcherQueue.end() )
		return General::ResultCode::Result_ActionRejected;

	m_watcherQueue.erase( iter );

	return General::ResultCode::Result_Success;
}

cClientSession* cHoldem::GetPlayerSession( uint64 playerIdx )
{
	if ( m_playerSlots.empty() )
		return nullptr;

	for ( int slot = 0; slot < m_maxPlayerCnt; ++slot ) {
		if ( m_playerSlots[ slot ] != nullptr )
		{
			if ( m_playerSlots[ slot ]->GetPlayerIdx() == playerIdx )
				return m_playerSlots[ slot ];
		}
		/*if ( m_slotReservation[ slot ] != nullptr )
		{
			if ( m_slotReservation[ slot ]->GetPlayerIdx() == playerIdx )
				return m_slotReservation[ slot ];
		}*/
	}
	return nullptr;
}

cClientSession* cHoldem::GetPlayerALLSession( uint64 playerIdx )
{
	if ( m_playerSlots.empty() )
		return nullptr;

	for ( int slot = 0; slot < m_maxPlayerCnt; ++slot ) {
		if ( m_playerSlots[ slot ] != nullptr )
		{
			if ( m_playerSlots[ slot ]->GetPlayerIdx() == playerIdx )
				return m_playerSlots[ slot ];
		}

		if ( m_slotReservation[ slot ] != nullptr )
		{
			if ( m_slotReservation[ slot ]->GetPlayerIdx() == playerIdx )
				return m_slotReservation[ slot ];
		}
	}
	return nullptr;
}

uint64 cHoldem::GetBossPlayerIdx()
{
	if ( m_dealerIter == m_playerSlots.end() )
		return 0;
		//throw std::exception( "m_bossIter exception" );

	auto player = *m_dealerIter;
	if ( player == nullptr ) // 일단 죽지 않도록 수정
		return 0;

	return ( *m_dealerIter )->GetPlayerIdx();;
}

// 최초 딜러 결정할때 무조건 호출할것.
void cHoldem::SetBossFirst()
{
	m_dealerIter = m_playerSlots.begin();
}

void cHoldem::SetMaster()
{
	IGame::SetMaster();
	m_pGameRoom->m_roomInfo.set_host_seat( GetMasterSlot() );
}

uint64 cHoldem::GetMasterPlayerIdx()
{
	if ( *m_masterIter != nullptr )
		return ( *m_masterIter )->GetPlayerIdx();
	return 0;
}

std::vector<cClientSession*> cHoldem::GetPlayersSessionList()
{
	return m_playerSlots;
}

#pragma region GET, SET Method



#pragma endregion GET, SET Method



// Enum 값을 문자열로 변환하는 static 함수
std::string cHoldem::EnumToString( int enumValue , const google::protobuf::EnumDescriptor* descriptor )
{
	const google::protobuf::EnumValueDescriptor* valueDescriptor = descriptor->FindValueByNumber( enumValue );
	if ( valueDescriptor ) {
		return valueDescriptor->name();
	}
	else {
		std::string errorInfo = std::format( "EnumToString enumValue {} Exception" , enumValue );
		throw std::runtime_error( errorInfo );
	}
}

// 예외를 던지는 static 함수
void cHoldem::ThrowGameStepException( std::string caller , Server::PlayPhase gameStep )
{
	const google::protobuf::EnumDescriptor* enumDescriptor = Server::PlayPhase_descriptor();
	std::string curStep = EnumToString( gameStep , enumDescriptor );
	std::string errorInfo = std::format( "GameStep {} {} call violation" , curStep , caller.c_str() );
	throw std::runtime_error( errorInfo );
}

// 동일한 카드가 존재하는지 검사한다.
void cHoldem::DebugCardCheck( google::protobuf::RepeatedField<General::PlayingCard>& cards )
{
#ifdef _DEBUG
	// suit_code 별로 합산
	std::map<int , int> cardMap;

	for ( const General::PlayingCard& card : cards ) {
		int typeChecker = card.suit_code();
		typeChecker = typeChecker * 1000;
		typeChecker += card.rank_code();

		cardMap[ typeChecker ] += 1;
	}

	// 이경우는 동일한 카드가 존재하는 경우이다.
	if ( cardMap.size() != cards.size() ) {
		int n = 0;
	}
#endif
}

void cHoldem::DebugCardCheckVec( std::vector<General::PlayingCard>& cards )
{
#ifdef _DEBUG
	// suit_code 별로 합산
	std::map<int , int> cardMap;

	for ( const General::PlayingCard& card : cards ) {
		int typeChecker = card.suit_code();
		typeChecker = typeChecker * 100;
		typeChecker += card.rank_code();

		cardMap[ typeChecker ] += 1;
	}

	// 이경우는 동일한 카드가 존재하는 경우이다.
	if ( cardMap.size() != cards.size() ) {
		int n = 0;
	}
#endif
}

// 핸드카드 + 커뮤니티 카드까지 총 7장
void cHoldem::DebugGetRoyalStraightFlush( google::protobuf::RepeatedField<General::PlayingCard>& cards )
{
#ifdef _DEBUG
	cards.Clear();

	General::PlayingCard card;
	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Ten );
	cards.Add( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Jack );
	cards.Add( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Queen );
	cards.Add( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_King );
	cards.Add( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Ace );
	cards.Add( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Ace );
	cards.Add( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Three );
	cards.Add( card );
#endif
}

void cHoldem::DebugMakeCommunityCardRoyalStraightFlush( std::vector<General::PlayingCard>& cards )
{
#ifdef _DEBUG
	cards.clear();

	General::PlayingCard card;
	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Ten );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Jack );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Queen );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_King );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Ace );
	cards.push_back( card );
#endif
}

// 2명 까지 키커 사용을 하는 모드로 강제로 만든다.
void cHoldem::DebugMakeKickerSituation( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
{
#ifdef _DEBUG
	cards.clear();

	General::PlayingCard card;
	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_King );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Three );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Three );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Jack );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Jack );
	cards.push_back( card );

	int seq = 1;

	for ( auto player : players ) {
		if ( player == nullptr )
			continue;

		auto cards = player->GetCards();

		switch ( seq )
		{
		case 1:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_Ace );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_Two );
			cards->Add( card );
			++seq;
		}
		break;
		case 2:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Two );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Spade );
			card.set_rank_code( General::CardRank::CardRank_Two );
			cards->Add( card );
			++seq;
		}
		break;
		case 3:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Ace );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Queen );
			cards->Add( card );
		}
		break;
		}
	}
#endif
}

void cHoldem::DebugMakeTwoPairSituation( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
{
#ifdef _DEBUG
	cards.clear();

	General::PlayingCard card;
	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Jack );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Nine );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Two );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Ten );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Five );
	cards.push_back( card );

	int seq = 1;

	for ( auto player : players ) {
		if ( player == nullptr )
			continue;

		auto cards = player->GetCards();

		switch ( seq )
		{
		case 1:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Jack );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Nine );
			cards->Add( card );
			++seq;
		}
		break;
		case 2:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Spade );
			card.set_rank_code( General::CardRank::CardRank_Two );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Ten );
			cards->Add( card );
			++seq;
		}
		break;
		}
	}
#endif
}

void cHoldem::DebugMakeTwoPairSameKickerSituation( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
{
#ifdef _DEBUG
	cards.clear();

	General::PlayingCard card;
	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Ace );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_King );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Two );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Spade );
	card.set_rank_code( General::CardRank::CardRank_Ten );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Five );
	cards.push_back( card );

	int seq = 1;

	for ( auto player : players ) {
		if ( player == nullptr )
			continue;

		auto cards = player->GetCards();

		switch ( seq )
		{
		case 1:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_Ace );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
			card.set_rank_code( General::CardRank::CardRank_King );
			cards->Add( card );
			++seq;
		}
		break;
		case 2:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_Ace );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_King );
			cards->Add( card );
			++seq;
		}
		break;
		}
	}
#endif
}


void cHoldem::DebugTest( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
{
#ifdef _DEBUG
	cards.clear();

	General::PlayingCard card;
	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Ace );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Club );
	card.set_rank_code( General::CardRank::CardRank_Five );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Six );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
	card.set_rank_code( General::CardRank::CardRank_Six );
	cards.push_back( card );

	card.set_suit_code( General::CardSuit::CardSuit_Heart );
	card.set_rank_code( General::CardRank::CardRank_Four );
	cards.push_back( card );

	int seq = 1;

	for ( auto player : players ) {
		if ( player == nullptr )
			continue;

		auto cards = player->GetCards();

		switch ( seq )
		{
		case 1:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Club );
			card.set_rank_code( General::CardRank::CardRank_Seven );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_Eight );
			cards->Add( card );
			++seq;
		}
		break;
		case 2:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_Three );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Spade );
			card.set_rank_code( General::CardRank::CardRank_Two );
			cards->Add( card );
			++seq;
		}
		break;
		case 3:
		{
			cards->Clear();

			card.set_suit_code( General::CardSuit::CardSuit_Club );
			card.set_rank_code( General::CardRank::CardRank_Nine );
			cards->Add( card );

			card.set_suit_code( General::CardSuit::CardSuit_Spade );
			card.set_rank_code( General::CardRank::CardRank_Queen );
			cards->Add( card );
			++seq;
		}
		break;
		}
	}
#endif
}

// 결함 #52 게임 결과 트리플 족보가 적용되지 않는 현
//void cHoldem::DebugTest( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
//{
//#ifdef _DEBUG
//	cards.clear();
//
//	General::PlayingCard card;
//	card.set_suit_code( General::CardSuit::CardSuit_Club );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_Ten );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Spade );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_Eight );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_King );
//	cards.push_back( card );
//
//	int seq = 1;
//
//	for ( auto player : players ) {
//		if ( player == nullptr )
//			continue;
//
//		auto cards = player->GetCards();
//
//		switch ( seq )
//		{
//		case 1:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Five );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_Four );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 2:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Jack );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Five );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 3:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Jack );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Eight );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		}
//	}
//#endif
//}

// 결함 #49 [Dev] 스플릿 윈 발생 시 상금 분배가 다른 현상
//void cHoldem::DebugTest( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
//{
//#ifdef _DEBUG
//	cards.clear();
//
//	General::PlayingCard card;
//	card.set_suit_code( General::CardSuit::CardSuit_Spade );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//	card.set_rank_code( General::CardRank::CardRank_Three );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_Six );
//	cards.push_back( card );
//
//	int seq = 1;
//
//	for ( auto player : players ) {
//		if ( player == nullptr )
//			continue;
//
//		auto cards = player->GetCards();
//
//		switch ( seq )
//		{
//		case 1:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Club );
//			card.set_rank_code( General::CardRank::CardRank_Four );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_Three );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 2:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Three );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Nine );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 3:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Two );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Ten );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 4:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Eight );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Club );
//			card.set_rank_code( General::CardRank::CardRank_Two );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		}
//	}
//#endif
//}

// 결함 #43 [Dev] 홀덤 3벳 룰 플레이 결과의 최종 획득/손실금액 오적용 현상
//void cHoldem::DebugTest( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
//{
//#ifdef _DEBUG
//	cards.clear();
//
//	General::PlayingCard card;
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_King );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Spade );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//	card.set_rank_code( General::CardRank::CardRank_Three );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//	card.set_rank_code( General::CardRank::CardRank_Five );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Club );
//	card.set_rank_code( General::CardRank::CardRank_Eight );
//	cards.push_back( card );
//
//	int seq = 1;
//
//	for ( auto player : players ) {
//		if ( player == nullptr )
//			continue;
//
//		auto cards = player->GetCards();
//
//		switch ( seq )
//		{
//		case 1:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_Ace );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_Ten );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 2:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_Eight );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Six );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 3:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_Ten );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Seven );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 4:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Three );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Spade );
//			card.set_rank_code( General::CardRank::CardRank_King );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		}
//	}
//#endif
//}

//void cHoldem::DebugTest( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players )
//{
//#ifdef _DEBUG
//	cards.clear();
//
//	General::PlayingCard card;
//	card.set_suit_code( General::CardSuit::CardSuit_Spade );
//	card.set_rank_code( General::CardRank::CardRank_Two );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Spade );
//	card.set_rank_code( General::CardRank::CardRank_Ten );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_Three );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Spade );
//	card.set_rank_code( General::CardRank::CardRank_Queen );
//	cards.push_back( card );
//
//	card.set_suit_code( General::CardSuit::CardSuit_Heart );
//	card.set_rank_code( General::CardRank::CardRank_Queen );
//	cards.push_back( card );
//
//	int seq = 1;
//
//	for ( auto player : players ) {
//		if ( player == nullptr )
//			continue;
//
//		auto cards = player->GetCards();
//
//		switch ( seq )
//		{
//		case 1:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_King );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Club );
//			card.set_rank_code( General::CardRank::CardRank_Seven );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 2:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Heart );
//			card.set_rank_code( General::CardRank::CardRank_Seven );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Diamond );
//			card.set_rank_code( General::CardRank::CardRank_Two );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		case 3:
//		{
//			cards->Clear();
//
//			card.set_suit_code( General::CardSuit::CardSuit_Club );
//			card.set_rank_code( General::CardRank::CardRank_Nine );
//			cards->Add( card );
//
//			card.set_suit_code( General::CardSuit::CardSuit_Club );
//			card.set_rank_code( General::CardRank::CardRank_Two );
//			cards->Add( card );
//			++seq;
//		}
//		break;
//		}
//	}
//#endif
//}

void cHoldem::CheatCardChange( PmNet::DebugHandSwapRQ& request )
{
	m_bCheatCardChange = true;
	m_cheatCardChange = request;
}

void cHoldem::WriteBetLog()
{
	for ( auto& log : m_betLogs ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
	}
}

// 카드 번호만 비교 함수
// 오름 차순
bool cHoldem::CompareCardByNumOnlyAsc( General::PlayingCard& card1 , General::PlayingCard& card2 )
{
	if ( card1.rank_code() < card2.rank_code() )
		return true;
	return false;
}

// Vector 두번째 카드만 비교해서 오른쪽으로
bool cHoldem::CompareCardBySecondCardNumOnlyAsc( std::vector<General::PlayingCard>& cards1 , std::vector<General::PlayingCard>& cards2 )
{
	if ( cards1[ 1 ].rank_code() < cards2[ 1 ].rank_code() )
		return true;
	return false;
}


// 카드 비교 함수
// 오름 차순
bool cHoldem::CompareCardByNumAsc( General::PlayingCard& card1 , General::PlayingCard& card2 )
{
	if ( card1.rank_code() < card2.rank_code() ) {
		return true;
	}
	else if ( card1.rank_code() > card2.rank_code() ) {
		return false;
	}
	return CompareCardByTypeAsc( card1 , card2 );
}

// 카드 타입 비교 함수
// 오름 차순
bool cHoldem::CompareCardByTypeAsc( const General::PlayingCard& card1 , const General::PlayingCard& card2 )
{
	// CardType를 기준으로 먼저 비교
	if ( card1.suit_code() < card2.suit_code() ) {
		return true;
	}
	else if ( card1.suit_code() > card2.suit_code() ) {
		return false;
	}
	return false;  // 같을 때는 false 반환 (strict weak ordering)
}

std::vector<General::PlayingCard> cHoldem::CompareGroupCombinationAndGetMinimum( std::map<General::CardSuit , std::vector<General::PlayingCard>> cardTypeMap )
{
	std::vector<General::PlayingCard> returnCards;
	std::vector<std::vector<General::PlayingCard>> mapList;
	//std::map<General::CardSuit , std::vector<General::PlayingCard>> CompareCardMap;
	std::vector<std::vector<General::PlayingCard>> combinationList;

	int count_of_over_2_card_in_one_group = 0; // 한개의 그룹에 카드가 2개 이상인 경우
	for ( auto& pair : cardTypeMap ) {

		if ( pair.second.size() != 1 ) {
			mapList.push_back( pair.second );
			++count_of_over_2_card_in_one_group;
		}
		else
			returnCards.push_back( pair.second[ 0 ] );
	}

	// 한그룹에 카드가 2개 이상인 그룹이 2개 이상인경우에만 컴비네이션 조합을 만든다.
	if ( mapList.size() > 1 )
	{
		for ( auto& cardA : mapList[ 0 ] )
		{
			for ( int n = 1; n < mapList.size(); ++n )
			{
				for ( auto& cardB : mapList[ n ] )
				{
					if ( cardA.suit_code() == cardB.suit_code() )
						continue;

					if ( cardA.rank_code() == cardB.rank_code() )
						continue;

					std::vector<General::PlayingCard> combination;
					combination.push_back( cardA );
					combination.push_back( cardB );
					combinationList.push_back( combination );

					// 카드 출력
					General::CardSuit cardTypeA = cardA.suit_code();
					std::string cardTypeStringA = protoutil::cProtoUtil::GetEnumString( cardTypeA );
					General::CardRank cardNumTypeA = cardA.rank_code();
					std::string cardNumTypeStringA = protoutil::cProtoUtil::GetEnumString( cardNumTypeA );

					General::CardSuit cardTypeB = cardB.suit_code();
					std::string cardTypeStringB = protoutil::cProtoUtil::GetEnumString( cardTypeB );
					General::CardRank cardNumTypeB = cardB.rank_code();
					std::string cardNumTypeStringB = protoutil::cProtoUtil::GetEnumString( cardNumTypeB );

					std::string cardInfo = std::format( "Card Combo CardA {} {} CardB {} {}" , cardTypeStringA , cardNumTypeStringA , cardTypeStringB , cardNumTypeStringB );
					TraceA( cardInfo );
				}
			}
		}

		// 넘버가 적은 카드 부터 오름 차순으로 정렬한다.
		for ( auto& combo : combinationList ) {
			std::sort( combo.begin() , combo.end() , CompareCardByNumOnlyAsc );
		}

		// 첫번째 숫자만 비교해서 가장 작은 숫자 검색
		General::CardRank compareNumType = General::CardRank::CardRank_King;

		for ( auto& combo : combinationList ) {
			if ( combo[ 0 ].rank_code() < compareNumType )
				compareNumType = combo[ 0 ].rank_code();
		}

		// compareNumType 빼고는 삭제
		std::vector<std::vector<General::PlayingCard>> sortedCombination;
		while ( combinationList.size() )
		{
			auto iter = combinationList.begin();
			std::vector<General::PlayingCard>& cards = *iter;
			if ( cards[ 0 ].rank_code() == compareNumType ) {
				sortedCombination.push_back( cards );
			}
			combinationList.erase( iter );
		}

		// 두번째 카드 비교
		std::sort( sortedCombination.begin() , sortedCombination.end() , CompareCardBySecondCardNumOnlyAsc );

		// 첫번째 놈이 최적의 카드라 생각한다.
		for ( auto& card : sortedCombination[ 0 ] ) {
			returnCards.push_back( card );
		}

		// 번호 정렬 필요
		std::sort( returnCards.begin() , returnCards.end() , CompareCardByNumOnlyAsc );

	}
	else
	{
		returnCards.clear();

		for ( auto& pair : cardTypeMap )
			returnCards.push_back( pair.second[ 0 ] );

		std::sort( returnCards.begin() , returnCards.end() , CompareCardByNumOnlyAsc );
	}

	return returnCards;
}

int cHoldem::SumTypeCount( google::protobuf::RepeatedField<General::PlayingCard>& cards )
{
	// suit_code 별로 합산
	std::map<General::CardSuit , int> cardMap;

	for ( const General::PlayingCard& card : cards ) {
		cardMap[ card.suit_code() ] += 1; // 해당 suit_code의 카운트를 증가시킴
	}
	return cardMap.size();
}

// 1. 중복된 무늬중에 가장 작은 숫자 하나 남겨 두고 제거
// 2. 중복된 숫자하나만 남겨 두고 제거
std::vector<General::PlayingCard> cHoldem::RemoveDuplicatedCard( google::protobuf::RepeatedField<General::PlayingCard>& cards )
{
	General::PlayingCard& card1 = cards.at( 0 );
	General::PlayingCard& card2 = cards.at( 1 );
	General::PlayingCard& card3 = cards.at( 2 );
	General::PlayingCard& card4 = cards.at( 3 );

	// 무늬 Groupping
	std::map<General::CardSuit , std::vector<General::PlayingCard>> cardTypeMap;
	for ( const General::PlayingCard& card : cards ) {
		auto iter = cardTypeMap.find( card.suit_code() );
		if ( iter != cardTypeMap.end() ) {
			iter->second.push_back( card ); // 기존에 존재 push 처리
		}
		else {
			std::vector<General::PlayingCard> cardVec;
			cardVec.push_back( card );
			cardTypeMap[ card.suit_code() ] = cardVec;
		}
	}

	// 2개 이상의 카드를 가진 그룹이 2개 이상이면
	std::vector<General::PlayingCard> vecCards = CompareGroupCombinationAndGetMinimum( cardTypeMap );

	//std::vector<General::PlayingCard> vecCards;
	//for ( auto& cards : cardTypeMap ) {
	//	if ( cards.second.size() > 1 ) {
	//		std::sort( cards.second.begin() , cards.second.end() , CompareCardByNumOnlyAsc ); // 중복무늬 발생 무늬별로 오름 차순으로 sort
	//	}
	//	vecCards.push_back( cards.second[ 0 ] ); // 한개만
	//}

	// 중복 숫자 제거
	std::map<General::CardRank , std::vector<General::PlayingCard>> cardNumMap;
	for ( const General::PlayingCard& card : vecCards ) {
		auto iter = cardNumMap.find( card.rank_code() );
		if ( iter != cardNumMap.end() ) {
			iter->second.push_back( card ); // 기존에 존재 push 처리
		}
		else {
			std::vector<General::PlayingCard> cardVec;
			cardVec.push_back( card );
			cardNumMap[ card.rank_code() ] = cardVec;
		}
	}

	// 중복 되던 말던, 숫자별로 1개씩
	std::vector<General::PlayingCard> removedCards;
	for ( auto& cards : cardNumMap ) {
		removedCards.push_back( cards.second[ 0 ] ); // 한개만
	}

#ifdef _DEBUG

	// 카드 출력
	TraceA( "-----------------------------------------------------------------------------------" );
	TraceA( "cHoldem::RemoveDuplicatedCard" );
	for ( auto card : removedCards )
	{
		General::CardSuit cardType = card.suit_code();
		std::string cardTypeString = protoutil::cProtoUtil::GetEnumString( cardType );
		General::CardRank cardNumType = card.rank_code();
		std::string cardNumTypeString = protoutil::cProtoUtil::GetEnumString( cardNumType );
		std::string cardInfo = std::format( "CardType {} CardNumType {}" , cardTypeString , cardNumTypeString );
		TraceA( cardInfo );
	}
	TraceA( "-----------------------------------------------------------------------------------" );

#endif

	return removedCards;
}

//void cHoldem::SetMaxBetMoney( const uint64 maxBetMoney )
//{
//	if ( maxBetMoney != 0 )
//		m_maxBetMoney = maxBetMoney;
//	else
//		m_maxBetMoney = UINT64_MAX;
//
//	m_pGameRoom->m_roomInfo.set_max_bet_money( m_maxBetMoney );
//}

std::vector<General::PlayingCard> cHoldem::RemoveDuplicatedCard( const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards )
{
	// 무늬 Groupping
	std::map<General::CardSuit , std::vector<General::PlayingCard>> cardTypeMap;
	for ( const General::PlayingCard& card : cards ) {
		auto iter = cardTypeMap.find( card.suit_code() );
		if ( iter != cardTypeMap.end() ) {
			iter->second.push_back( card ); // 기존에 존재 push 처리
		}
		else {
			std::vector<General::PlayingCard> cardVec;
			cardVec.push_back( card );
			cardTypeMap[ card.suit_code() ] = cardVec;
		}
	}

	// 2개 이상의 카드를 가진 그룹이 2개 이상이면
	std::vector<General::PlayingCard> vecCards = CompareGroupCombinationAndGetMinimum( cardTypeMap );

	//std::vector<General::PlayingCard> vecCards;
	//for ( auto& cards : cardTypeMap ) {
	//	if ( cards.second.size() > 1 ) {
	//		std::sort( cards.second.begin() , cards.second.end() , CompareCardByNumOnlyAsc ); // 중복무늬 발생 무늬별로 오름 차순으로 sort
	//	}
	//	vecCards.push_back( cards.second[ 0 ] ); // 한개만
	//}

	// 중복 숫자 제거
	std::map<General::CardRank , std::vector<General::PlayingCard>> cardNumMap;
	for ( const General::PlayingCard& card : vecCards ) {
		auto iter = cardNumMap.find( card.rank_code() );
		if ( iter != cardNumMap.end() ) {
			iter->second.push_back( card ); // 기존에 존재 push 처리
		}
		else {
			std::vector<General::PlayingCard> cardVec;
			cardVec.push_back( card );
			cardNumMap[ card.rank_code() ] = cardVec;
		}
	}

	// 중복 되던 말던, 숫자별로 1개씩
	std::vector<General::PlayingCard> removedCards;
	for ( auto& cards : cardNumMap ) {
		removedCards.push_back( cards.second[ 0 ] ); // 한개만
	}

#ifdef _DEBUG

	// 카드 출력
	TraceA( "-----------------------------------------------------------------------------------" );
	TraceA( "cHoldem::RemoveDuplicatedCard" );
	for ( auto card : removedCards )
	{
		General::CardSuit cardType = card.suit_code();
		std::string cardTypeString = protoutil::cProtoUtil::GetEnumString( cardType );
		General::CardRank cardNumType = card.rank_code();
		std::string cardNumTypeString = protoutil::cProtoUtil::GetEnumString( cardNumType );
		std::string cardInfo = std::format( "CardType {} CardNumType {}" , cardTypeString , cardNumTypeString );
		TraceA( cardInfo );
	}
	TraceA( "-----------------------------------------------------------------------------------" );

#endif

	return removedCards;
}

/* 이 함수는 일단 어디에 쓸지는 모르지만 만들어둠
중복된 숫자의 종류 갯수
( 1, 2, 2, 3 ) 인경우 1
( 2, 2, 2, 3 ) 인경우 1
( 2, 2, 3, 3 ) 인경우 2
*/
int cHoldem::SameNumCount( google::protobuf::RepeatedField<General::PlayingCard>& cards )
{
	// suit_code 별로 합산
	std::map<General::CardRank , int> cardMap;

	for ( const General::PlayingCard& card : cards ) {
		cardMap[ card.rank_code() ] += 1; // 해당 rank_code 의 카운트를 증가시킴
	}

	switch ( cardMap.size() )
	{
	case 3:
		return 1;
	case 2:
		return 2;
	case 1:
		return 4;
	}
	return 0;
}

std::string cHoldem::CardTypeToString( General::CardSuit type ) {
	switch ( type ) {
	case General::CardSuit::CardSuit_Heart: return "H";
	case General::CardSuit::CardSuit_Diamond: return "D";
	case General::CardSuit::CardSuit_Club: return "C";
	case General::CardSuit::CardSuit_Spade: return "S";
	default: return "";
	}
}

std::string cHoldem::CardNumTypeToString( General::CardRank num ) {
	switch ( num ) {
	case General::CardRank::CardRank_Ace: return "A";
	case General::CardRank::CardRank_Two: return "2";
	case General::CardRank::CardRank_Three: return "3";
	case General::CardRank::CardRank_Four: return "4";
	case General::CardRank::CardRank_Five: return "5";
	case General::CardRank::CardRank_Six: return "6";
	case General::CardRank::CardRank_Seven: return "7";
	case General::CardRank::CardRank_Eight: return "8";
	case General::CardRank::CardRank_Nine: return "9";
	case General::CardRank::CardRank_Ten: return "10";
	case General::CardRank::CardRank_Jack: return "J";
	case General::CardRank::CardRank_Queen: return "Q";
	case General::CardRank::CardRank_King: return "K";
	default: return "";
	}
}

std::string cHoldem::CardsToString( const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards )
{
	std::ostringstream oss;
	for ( const General::PlayingCard& card : cards ) {
		oss << cHoldem::CardTypeToString( card.suit_code() ) << cHoldem::CardNumTypeToString( card.rank_code() ) << ",";
	}
	std::string result = oss.str();
	if ( !result.empty() ) {
		result.pop_back();
	}
	return result;
}

std::string cHoldem::CardsToString( const std::vector<General::PlayingCard>& cards )
{
	std::ostringstream oss;
	for ( const General::PlayingCard& card : cards ) {
		oss << cHoldem::CardTypeToString( card.suit_code() ) << cHoldem::CardNumTypeToString( card.rank_code() ) << ",";
	}
	std::string result = oss.str();
	if ( !result.empty() ) {
		result.pop_back();
	}
	return result;
}

bool cHoldem::CheckSameCI( const std::string& ci )
{
	for ( auto t_player : m_playerSlots )
	{
		if ( t_player != nullptr && t_player->GetAccountGuid() == ci )
			return true;
	}
	return false;

}
