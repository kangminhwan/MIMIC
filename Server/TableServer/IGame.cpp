#include "TableServerHeader.h"
#include "IGame.h"
#include "cCardDeck.h"
#include "cGameRoom.h"
#include "cClientSession.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "cLogicException.h"
#include "cHoldemKicker.h"
#include "cHoldemSameJokboCompare.h"

#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Common/cSingleton.h"

#include <iostream>
#include <format>

#include <google/protobuf/util/json_util.h>


void IGame::LeftCardNoti( cClientSession* pClientSession )
{
	PmNet::TileRemainBulletinRS response;
	response.set_cards_left( m_deck.size() );

	if ( pClientSession == nullptr ) {
		m_pGameRoom->BroadCastToAllPlayer( General::PacketID::Packet_ShoeRemainNotice , response );
	}
	else {
		std::string errorMessage = "";
		m_pGameRoom->SendRequest( pClientSession , General::PacketID::Packet_ShoeRemainNotice , response , General::ResultCode::Result_Success , errorMessage );
	}
}

uint64 IGame::GetPlayerMoney( cClientSession* pClientSession )
{
	bool checkOnly = true;
	if ( General::AssetKind::AssetKind_Chip == m_pGameRoom->m_roomInfo.asset_kind() ) {
		return pClientSession->GetChip();
	}
	else if ( General::AssetKind::AssetKind_Coin == m_pGameRoom->m_roomInfo.asset_kind() ) {
		return pClientSession->GetCoin();
	}
	else
		return 0;
	return 0;
}

void IGame::SetMaxBetMoney( const uint64 maxBetMoney )
{
	if ( maxBetMoney != 0 )
		m_maxBetMoney = maxBetMoney;
	else
		m_maxBetMoney = UINT64_MAX;

	m_pGameRoom->m_roomInfo.set_wager_limit( m_maxBetMoney );
}

void IGame::ClearPlayerCache()
{
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr )
			continue;

		player->ClearPlayerBet();
	}
}

/* 홀덤 맥스 베팅
1. 채널이 설정한 맥스 베팅 값
2. 내가 가진 보유 재화 전체
3. 게임 진행중인 플레이어 중 두번째로 많은 재화를 가진 플레이어
*/
uint64 IGame::GetHoldemMaxBet( cClientSession* ownerPlayer )
{
	std::vector<uint64> maxMoneyOrder;

	//uint64 curMoney = GetPlayerMoney( player );
	//maxMoneyOrder.push_back( curMoney );

	// 내가 걸수 있는 최대 베팅 금액
	/*uint64 myMaxMoney = GetMaxBetMoney() - ownerPlayer->GetLostMoney();
	uint64 ownerCurrentMoney = GetPlayerMoney( ownerPlayer );
	if ( myMaxMoney > ownerCurrentMoney )
		maxMoneyOrder.push_back( ownerCurrentMoney );
	else
		maxMoneyOrder.push_back( myMaxMoney );*/


	// 베팅을 하는 사람이 최초에 들고 들어간 베팅 금액이 
	uint64 maxOwnerBetMoney = 0;

	// 내돈이 여태까지 베팅한 금액보다 경우
	if ( ( GetPlayerMoney( ownerPlayer ) + ownerPlayer->GetLostMoney() ) < GetMaxBetMoney() )
	{
		maxOwnerBetMoney = GetPlayerMoney( ownerPlayer );
	}
	else
	{
		maxOwnerBetMoney = GetMaxBetMoney() - ownerPlayer->GetLostMoney();
	}

	maxMoneyOrder.push_back( maxOwnerBetMoney );

	// 남은 칩을 순으로 sort 한뒤에
	// 2번째 유저의 남은 칩으로 계산 ( 만약에 두명이 동일하면 ??? )
	uint64 secondPlayerMaxMoney = 0;
	//std::vector<cClientSession*> activePlayers = GetActivePlayers( ownerPlayer->GetPlayerIdx() ); // 본인을 뺀경우
	std::vector<cClientSession*> activePlayers = GetMaxBetPlayers( ownerPlayer->GetLostMoney() ); // 본인을 포함시키는 경우
	if ( activePlayers.size() >= 2 ) {
		if ( m_pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Chip ) {
			std::vector<uint64> sortedMoney;
			for ( auto player : activePlayers ) {
				uint64 startMoney = player->GetChip() + player->GetLostMoney();
				if ( startMoney > GetMaxBetMoney() )
					startMoney = GetMaxBetMoney();

				// 아예 계산해서 넣자.
				sortedMoney.push_back( startMoney - ownerPlayer->GetLostMoney() );
			}

			std::sort( sortedMoney.begin() , sortedMoney.end() , std::greater<uint64>() );
			secondPlayerMaxMoney = sortedMoney[ 1 ];
		}
		else if ( m_pGameRoom->GetMoneyType() == General::AssetKind::AssetKind_Coin ) {
			std::vector<uint64> sortedMoney;
			for ( auto player : activePlayers ) {
				uint64 startMoney = player->GetCoin() + player->GetLostMoney();
				if ( startMoney > GetMaxBetMoney() )
					startMoney = GetMaxBetMoney();

				// 아예 계산해서 넣자.
				sortedMoney.push_back( startMoney - ownerPlayer->GetLostMoney() );
			}

			std::sort( sortedMoney.begin() , sortedMoney.end() , std::greater<uint64>() );
			secondPlayerMaxMoney = sortedMoney[ 1 ];
		}
	}
	else
	{
		secondPlayerMaxMoney = 0; // 베팅 할 수 있는 금액이 없다.
	}

	maxMoneyOrder.push_back( secondPlayerMaxMoney );

	// 세번째로 채널에 설정된 맥스 베팅
	//uint64 channelMaxBet = GetMaxBetMoney() - player->GetLostMoney();
	//maxMoneyOrder.push_back( channelMaxBet );
	maxMoneyOrder.push_back( GetMaxBetMoney() );

	std::sort( maxMoneyOrder.begin() , maxMoneyOrder.end() );

	// 가장 적은 놈을 리턴해 준다.
	return maxMoneyOrder[ 0 ];
}

// ActivePlayer + Allin 유저중에 curBetMoney 보다 베팅을 많이한 플레이어
std::vector<cClientSession*> IGame::GetMaxBetPlayers( const uint64 curBetMoney )
{
	std::vector<cClientSession*> activePlayers;

	uint32 maxBetMoney = GetMaxBetMoney();
	int playerCount = 0;
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr )
			continue;

		if ( player->isDie() )
			continue;

		// 올인이고 베팅 금액이 더 작은 경우
		if ( player->isAllIn() && player->GetLostMoney() < curBetMoney )
			continue;

		activePlayers.push_back( player );
	}
	return activePlayers;
}

// 게임 머니 보유 한도에 도달한 플레이어들을 Kick 한다.
void IGame::KickMoneyHoldingLimitPlayers()
{
	// 방 나감 알림
	PmNet::ChamberLeaveRS _res;
	_res.set_chamber_no( m_pGameRoom->GetRoomNumber() );

	
	for ( auto player : m_playerSlots ) {
		if ( player == nullptr )
			continue;
		if ( player->HasMoneyLimit() ) {
			player->SendMoneyLimitPopopOnResult();
			LeaveSlot( player ,true);
			RemoveReservation( player );
			CancelWatcherReservation( player );
			player->RoomOutReset();
			_res.set_member_idx( player->GetPlayerIdx() );
			_res.set_observer_cnt( m_pGameRoom->GetWatcherCnt() );
			m_pGameRoom->BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
			m_pGameRoom->RemovePlayer( player->GetPlayerIdx() );
			m_pGameRoom->RemoveWatcher( player->GetPlayerIdx() );

			auto m_gameInterface = m_pGameRoom->GetGameInterface();
			
			player->SyncFriend_Online( true , player, General::ContactState::ContactState_Online , "" );
		}
	}
}

void IGame::CalcMoneyNew( const int& rank , uint64& potMoney, std::vector<PmNet::MemberOutcome>& playerResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap )
{
	// 예외처리 나가리 판
	// 승리자가 여려명인데, 전체 플레이한 인원과 동일한 경우 공동우승
	if ( rank == 1 && ( playerResults.size() == GetMemberCnt() && 1 != GetMemberCnt() ) ) {

		// 본인이 손실한 금액을 그대로 다시 셋팅해주고 끝낸다.
		for ( auto& playerResult : playerResults ) {
			const uint64 playerIdx = playerResult.member_idx();
			playerResult.set_svr_fund_after( playerResult.fund_before() );
			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
		}
		potMoney = 0;
		return;
	}

	std::vector<uint64> playerIdxList;
	for ( const auto& playerResult : playerResults ) {
		playerIdxList.push_back( playerResult.member_idx() );
	}

	// playersResults 플레이어들이 소속된 사이드팟 목록을 가져온다.
	// 돈의 소유권이 있는지와 승리한 플에이어 셋팅
	// 둘다 성립이 되어야하고, 결국 winner count 만큼 나누어준다.
	std::vector<uint64> sideKeys;

	// 사이드 목록을 뽑아올때 자신의 최종 배팅 금액 기준으로 뽑는다.

	//auto sidePots = cSidePotManager::GetSides( playerIdxList );
	auto sidePots = cSidePotManager::GetSides();
	for ( auto& sidePair : sidePots )
	{
		const uint64& side_money = sidePair.first;
		auto side = sidePair.second;

		// 현재 사이드팟에 플레이어가 승리한 플레이어들이 가져갈 권한이 있는지 확인한다.
		for ( auto& playerResult : playerResults ) {

			const uint64& playerIdx = playerResult.member_idx();
			cClientSession* pClientSession = GetPlayerSession( playerIdx );
			if ( pClientSession == nullptr )
				continue;
			if ( side->IsDistributer( playerResult.member_idx() ) ) {
				side->AddWinner( playerResult.member_idx() , pClientSession->GetLostMoney() );
			}
		}

	}
	
	// 사이드 팟이 없을때
	if ( sidePots.size() == 0 )
	{
		// 단순하게 금액을 N 빵하고 끝냄
		double getMoney = potMoney / playerResults.size();

		for ( int n = 0; n < playerResults.size(); ++n ) {
			auto& playerResult = playerResults[ n ];
			const uint64& playerIdx = playerResult.member_idx();
			cClientSession* pClientSession = GetPlayerSession( playerIdx );
			if ( pClientSession == nullptr )
				continue;

			double dealerFee = 0;
			if ( pClientSession->GetLostMoney() < getMoney ) {
				dealerFee = ( getMoney - pClientSession->GetLostMoney() ) * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
				dealerFee = ceil( dealerFee );
			}
			playerResult.set_svr_fund_after( GetPlayerMoney( pClientSession ) + getMoney - dealerFee );
			playerResult.set_svr_rake_cut( dealerFee );
			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
		}
		potMoney = 0;
		return;
	}
	else
	{
		// 사이드가 없는 플레이어가 있는지 확인
		std::vector<uint64> nonSidePlayers;
		for ( auto& playerResult : playerResults )
		{
			if ( false == cSidePotManager::HasSidePotOwner( playerResult.member_idx() ) )
			{
				nonSidePlayers.push_back( playerResult.member_idx() );
			}
		}

		// 돈 계산은 플레이어 기준으로  처리한다.
		for ( auto& playerResult : playerResults )
		{
			double getPlayerMoneyTotal = 0;
			double getLoserMoneyTotal = 0;

			for ( auto& sidePair : sidePots )
			{
				const uint64& playerIdx = playerResult.member_idx();
				auto sidePot = sidePair.second;

				if ( sidePot->isWinner( playerIdx ) == false )
					continue;

				if ( sidePot->isCompleted() )
					continue;

				// Tax 머니가 얼마인지는 여기에서 계산
				double getLoserMoney = sidePot->GetLoserMoney();
				getLoserMoneyTotal += getLoserMoney;

				// 딜러비 없는돈
				double getPlayerMoney = sidePot->GetPlayerMoney( playerResult.member_idx() );
				getPlayerMoneyTotal += getPlayerMoney;

				// 플레이어에게 준돈 누적
				sidePot->DistributedMoney( (getLoserMoney) + getPlayerMoney );

				// 처리한 사이드팟을 삭제하기 위해 저장해둔다.
				sideKeys.push_back( sidePair.first );
			}
			/*
			* 
			*/
			// 팟금액에서 플레이어에게 나눠준 돈은 뺀다.
			if ( potMoney < getPlayerMoneyTotal + getLoserMoneyTotal )
				potMoney = 0;
			else
				potMoney -= getPlayerMoneyTotal + getLoserMoneyTotal;
			/*if ( potMoney > 100000000000 )
				potMoney = 0;*/
			const uint64& playerIdx = playerResult.member_idx();
			cClientSession* pClientSession = GetPlayerSession( playerIdx );
			if ( pClientSession == nullptr )
				continue;
			// 딜러비
			double dealerFee = getLoserMoneyTotal * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
			//dealerFee = ceil( dealerFee );

			// 플레이어 줄돈
			playerResult.set_svr_fund_after( GetPlayerMoney( pClientSession ) + getPlayerMoneyTotal + getLoserMoneyTotal - dealerFee );
			playerResult.set_svr_rake_cut( dealerFee );

			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerResult.member_idx() , playerResult ) );
		}

		// 처리한 사이드는 삭제한다.
		cSidePotManager::DeleteSidePots( sideKeys );
		
		// 처리한 사이드팟 금액을 빼준다.
		//potMoney -= sidePotMoney;

		// 팟 머니가 남아 있고, 사이드 팟이 없는 승리자가 있으면 돈을 나누고 끝낸다.
		//if ( potMoney > 0 && nonSidePlayers.size() )
		//{
		//	// nonSidePlayers 들에게 돈을 배분해야 한다.
		//	double getMoney = potMoney / nonSidePlayers.size();

		//	for ( auto& playerIdx : nonSidePlayers ) {

		//		auto iterPair = playersResultMap.find( playerIdx );
		//		if ( iterPair != playersResultMap.end() )
		//		{
		//			auto& playerResult = iterPair->second;
		//			const uint64& playerIdx = playerResult.member_idx();
		//			cClientSession* pClientSession = GetPlayerSession( playerIdx );
		//			if ( pClientSession == nullptr )
		//				continue;
		//			double nonTaxMoney = pClientSession->GetLostMoney() - cSidePotManager::CurCompletedSidePotMoney();

		//			double curAfterMoney = playerResult.svr_fund_after();
		//			double curDealerFee = playerResult.svr_rake_cut();

		//			// 코드 수정
		//			// 딜러비 처리를 위해 획득머니에서 딜러비를 제와하지 않는 금액을 계산할때 nonTaxMoney 가 덬 커지는 상황이면 딜러비 0 으로 예외처리
		//			double dealerFee = 0;
		//			if ( getMoney > nonTaxMoney )
		//				dealerFee = (getMoney - nonTaxMoney) * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
		//			//dealerFee = ceil( dealerFee );

		//			playerResult.set_svr_fund_after( curAfterMoney + getMoney - dealerFee );
		//			playerResult.set_svr_rake_cut( curDealerFee + dealerFee );
		//		}
		//	}

		//	potMoney = 0;
		//}
	}
}


void IGame::CalcMoneyLowBa( const int& rank , uint64& potMoney , std::vector<PmNet::MemberOutcome>& playerResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap )
{
	// 예외처리 나가리 판
	// 승리자가 여려명인데, 전체 플레이한 인원과 동일한 경우 공동우승
	if ( rank == 1 && ( playerResults.size() == GetMemberCnt() )&& 1 != GetMemberCnt ()) {
		// 본인이 손실한 금액을 그대로 다시 셋팅해주고 끝낸다.
		for ( auto& playerResult : playerResults ) {
			const uint64 playerIdx = playerResult.member_idx();
			playerResult.set_svr_fund_after( playerResult.fund_before() );
			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
		}
		potMoney = 0;
		return;
	}

	std::vector<uint64> playerIdxList;
	for ( const auto& playerResult : playerResults ) {
		playerIdxList.push_back( playerResult.member_idx() );
	}

	// playersResults 플레이어들이 소속된 사이드팟 목록을 가져온다.
	// 돈의 소유권이 있는지와 승리한 플에이어 셋팅
	// 둘다 성립이 되어야하고, 결국 winner count 만큼 나누어준다.
	std::vector<uint64> sideKeys;

	// 사이드 목록을 뽑아올때 자신의 최종 배팅 금액 기준으로 뽑는다.

	//auto sidePots = cSidePotManager::GetSides( playerIdxList );
	auto sidePots = cSidePotManager::GetSides();
	for ( auto& sidePair : sidePots )
	{
		const uint64& side_money = sidePair.first;
		auto side = sidePair.second;

		// 현재 사이드팟에 플레이어가 승리한 플레이어들이 가져갈 권한이 있는지 확인한다.
		for ( auto& playerResult : playerResults ) {

			const uint64& playerIdx = playerResult.member_idx();
			cClientSession* pClientSession = GetPlayerSession( playerIdx );
			if ( pClientSession == nullptr )
				continue;
			if ( side->IsDistributer( playerResult.member_idx() ) ) {
				side->AddWinner( playerResult.member_idx() , pClientSession->GetLostMoney() );
			}
		}

		// 처리한 사이드팟을 삭제하기 위해 저장해둔다.
		sideKeys.push_back( side_money );
	}

	// 사이드 팟이 없을때
	if ( sidePots.size() == 0 )
	{
		// 단순하게 금액을 N 빵하고 끝냄
		double getMoney = potMoney / playerResults.size();

		for ( int n = 0; n < playerResults.size(); ++n ) {
			auto& playerResult = playerResults[ n ];
			const uint64& playerIdx = playerResult.member_idx();
			cClientSession* pClientSession = GetPlayerSession( playerIdx );
			if ( pClientSession == nullptr )
				continue;

			double dealerFee = 0;
			if ( pClientSession->GetLostMoney() < getMoney ) {
				dealerFee = ( getMoney - pClientSession->GetLostMoney() ) * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
				dealerFee = ceil( dealerFee );
			}
			playerResult.set_svr_fund_after( GetPlayerMoney( pClientSession ) + getMoney - dealerFee );
			playerResult.set_svr_rake_cut( dealerFee );
			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
		}
		potMoney = 0;
		return;
	}
	else
	{

		// 사이드가 없는 플레이어가 있는지 확인
		std::vector<uint64> nonSidePlayers;
		for ( auto& playerResult : playerResults )
		{
			if ( false == cSidePotManager::HasSidePotOwner( playerResult.member_idx() ) )
			{
				nonSidePlayers.push_back( playerResult.member_idx() );
			}
		}

		// 돈 계산은 플레이어 기준으로  처리한다.
		for ( auto& playerResult : playerResults )
		{
			double getPlayerMoneyTotal = 0;
			double getLoserMoneyTotal = 0;

			for ( auto& sidePair : sidePots )
			{
				const uint64& playerIdx = playerResult.member_idx();
				auto sidePot = sidePair.second;

				if ( sidePot->isWinner( playerIdx ) == false )
					continue;

				if ( sidePot->isCompleted() )
					continue;

				// Tax 머니가 얼마인지는 여기에서 계산
				double getLoserMoney = sidePot->GetLoserMoney();
				getLoserMoneyTotal += getLoserMoney;

				// 딜러비 없는돈
				double getPlayerMoney = sidePot->GetPlayerMoney( playerResult.member_idx() );
				getPlayerMoneyTotal += getPlayerMoney;

				// 플레이어에게 준돈 누적
				sidePot->DistributedMoney( ( getLoserMoney ) +getPlayerMoney );
			}
			/*
			*
			*/
			// 팟금액에서 플레이어에게 나눠준 돈은 뺀다.
			if ( potMoney < getPlayerMoneyTotal + getLoserMoneyTotal )
				potMoney += 1;
			potMoney -= getPlayerMoneyTotal + getLoserMoneyTotal;
			/*if ( potMoney > 100000000000 )
				potMoney = 0;*/
			const uint64& playerIdx = playerResult.member_idx();
			cClientSession* pClientSession = GetPlayerSession( playerIdx );
			if ( pClientSession == nullptr )
				continue;
			// 딜러비
			double dealerFee = getLoserMoneyTotal * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
			//dealerFee = ceil( dealerFee );

			// 플레이어 줄돈
			playerResult.set_svr_fund_after( GetPlayerMoney( pClientSession ) + getPlayerMoneyTotal + getLoserMoneyTotal - dealerFee );
			playerResult.set_svr_rake_cut( dealerFee );

			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerResult.member_idx() , playerResult ) );
		}

		// 처리한 사이드는 삭제한다.
		//cSidePotManager::DeleteSidePots( sideKeys );

		// 처리한 사이드팟 금액을 빼준다.
		//potMoney -= sidePotMoney;

		// 팟 머니가 남아 있고, 사이드 팟이 없는 승리자가 있으면 돈을 나누고 끝낸다.
		if ( potMoney > 0 && nonSidePlayers.size() )
		{
			// nonSidePlayers 들에게 돈을 배분해야 한다.
			double getMoney = potMoney / nonSidePlayers.size();

			for ( auto& playerIdx : nonSidePlayers ) {

				auto iterPair = playersResultMap.find( playerIdx );
				if ( iterPair != playersResultMap.end() )
				{
					auto& playerResult = iterPair->second;
					const uint64& playerIdx = playerResult.member_idx();
					cClientSession* pClientSession = GetPlayerSession( playerIdx );
					if ( pClientSession == nullptr )
						continue;
					double nonTaxMoney = pClientSession->GetLostMoney() - cSidePotManager::CurCompletedSidePotMoney();

					double curAfterMoney = playerResult.svr_fund_after();
					double curDealerFee = playerResult.svr_rake_cut();
					// 코드 수정
					// 딜러비 처리를 위해 획득머니에서 딜러비를 제와하지 않는 금액을 계산할때 nonTaxMoney 가 덬 커지는 상황이면 딜러비 0 으로 예외처리
					double dealerFee = 0;
					if ( getMoney > nonTaxMoney )
						dealerFee = (getMoney - nonTaxMoney) * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
					//dealerFee = ceil( dealerFee );

					playerResult.set_svr_fund_after( curAfterMoney + getMoney - dealerFee );
					playerResult.set_svr_rake_cut( curDealerFee + dealerFee );
				}
			}

			potMoney = 0;
		}
	}
}


void IGame::CalcMoneyFriends( const int& rank , uint64& potMoney , std::vector<PmNet::MemberOutcome>& playerResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap )
{
	// 예외처리 나가리 판
	// 승리자가 여려명인데, 전체 플레이한 인원과 동일한 경우 공동우승
	if ( rank == 1 && ( playerResults.size() == GetMemberCnt() && 1 != GetMemberCnt() ) ) {

		// 본인이 손실한 금액을 그대로 다시 셋팅해주고 끝낸다.
		for ( auto& playerResult : playerResults ) {
			const uint64 playerIdx = playerResult.member_idx();
			playerResult.set_svr_fund_after( playerResult.fund_before() );
			playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
		}
		potMoney = 0;
		return;
	}
	//배팅금 돌려받기
	for ( auto& playerResult : playerResults ) {
		const uint64 playerIdx = playerResult.member_idx();
		potMoney -= (playerResult.fund_before() - playerResult.svr_fund_after());
		playerResult.set_svr_fund_after( playerResult.fund_before() );
	}
	// 단순하게 금액을 N 빵하고 끝냄
	double getMoney = potMoney / playerResults.size();

	for ( int n = 0; n < playerResults.size(); ++n ) {
		auto& playerResult = playerResults[ n ];
		const uint64& playerIdx = playerResult.member_idx();
		cClientSession* pClientSession = GetPlayerSession( playerIdx );
		if ( pClientSession == nullptr )
			continue;

		double dealerFee = 0;
			
		dealerFee = ( getMoney ) * pClientSession->GetDealerFeeRate( m_channel.channel_content_type() , m_dealerFeeRate );
		dealerFee = ceil( dealerFee );
		
		playerResult.set_svr_fund_after( getMoney - dealerFee + playerResult.svr_fund_after() );
		playerResult.set_svr_rake_cut( dealerFee );
		playersResultMap.insert( std::pair<uint64 , PmNet::MemberOutcome>( playerIdx , playerResult ) );
	}
	potMoney = 0;
	return;
}

cClientSession* IGame::GetReservaionSession( const uint64& playeridx )
{
	for ( int slot = 0; slot < m_slotReservation.size(); ++slot )
	{
		if ( m_slotReservation[ slot ] != nullptr )
		{
			auto pc = m_slotReservation[ slot ];
			pc->GetPlayerIdx();
			if ( pc->GetPlayerIdx() == playeridx )
			{
				return m_slotReservation[ slot ];
			}
		}
	}
	return nullptr;
}

cClientSession* IGame::FindPlayerByIdx( uint64 playerIdx )
{
	// std::scoped_lock lk(m_roomMutex);  // 멀티스레드 동시 접근이면 잠금

	auto match = [&]( cClientSession* s ) -> bool {
		return ( s != nullptr ) && ( s->GetPlayerIdx() == playerIdx );
	};

	// 1) 플레이어 슬롯
	for ( cClientSession* s : m_playerSlots )
		if ( match( s ) ) return s;

	return nullptr;
}