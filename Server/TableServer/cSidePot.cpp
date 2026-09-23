#include "cSidePot.h"

cSidePot::cSidePot( const uint64 beforePotSideMoney , const uint64 sideMoney )
	: m_beforePotBaseAmount( beforePotSideMoney ), m_baseAmount( sideMoney ), m_sideMoneyTotal( 0 )
{
}

// 플레이어의 베팅 금액이 사이드팟 금액보다 크다면 분배 권한을 갖는다.
void cSidePot::AddDistibuter( const uint64 playerIdx , const uint64 beforeSidePotBaseAmount , const uint64 betMoney )
{
	if ( betMoney >= m_baseAmount ) {
		m_distributers.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
		//m_sideMoneyTotal = m_distributers.size() * m_baseAmount;
		m_sideMoneyTotal = (m_baseAmount - beforeSidePotBaseAmount ) * m_distributers.size();
	}
}

void cSidePot::IncreaseSidePlayerCount( const uint64 playerIdx )
{
	auto iter = m_distributers.find( playerIdx );
	if ( iter == m_distributers.end() ) {
		m_distributers.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
	}
}

double cSidePot::GetPlayerMoney( const uint64 playerIdx )
{	
	// playerIdx 가 가져갈 권한이 있는가?
	auto dis = m_distributers.find( playerIdx );
	auto win = m_winners.find( playerIdx );
	if ( dis != m_distributers.end() && win != m_winners.end() ) {
		return (m_baseAmount - m_beforePotBaseAmount);
	}

	return 0;
}

// 리턴한돈에서 딜러비를 계산해서 빼줘야 한다.
double cSidePot::GetLoserMoney()
{
	uint64 loserMoney = m_sideMoneyTotal - ( ( m_baseAmount - m_beforePotBaseAmount ) * m_winners.size() );
	double taxMoney = loserMoney / ( double ) m_winners.size();
	return taxMoney;
}

bool cSidePot::IsDistributer( const uint64 playerIdx )
{
	auto iter = m_distributers.find( playerIdx );
	return iter != m_distributers.end();
}

// 사이드팟 기준 금액보다 큰 사람만 가져갈 권한을 갖는다.
bool cSidePot::AddWinner( const uint64 playerIdx , const uint64 betMoney )
{
	if ( betMoney >= m_baseAmount ) {
		m_winners.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
		return true;
	}
	return false;
}

void cSidePot::AddSidePlayer( const uint64 playerIdx )
{
	auto iter = m_sidePlayers.find( playerIdx );
	if ( iter == m_sidePlayers.end() ) {
		m_sidePlayers.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
	}
}

bool cSidePot::areSidePotOwners( const std::vector<uint64> playerIdxList )
{
	for ( const auto& playerIdx : playerIdxList ) {
		if ( m_sidePlayers.contains( playerIdx ) )
			return true;
	}
	return false;
}

bool cSidePot::isSidePotOwner( const uint64 playerIdx )
{
	if ( m_sidePlayers.contains( playerIdx ) )
		return true;

	return false;
}

void cSidePot::DistributedMoney( const uint64 distributedMoney )
{
	m_distributedMoneyTotal += distributedMoney;
}

bool cSidePot::isWinner( const uint64 playerIdx )
{
	auto iter = m_winners.find( playerIdx );
	return iter != m_winners.end();
}

/////////////////////////////////////////////////////
// cSidePotManager
/////////////////////////////////////////////////////

void cSidePotManager::Clear()
{
	for ( auto sidePot : m_sidePots ) {
		if ( sidePot.second != nullptr )
			delete sidePot.second;
	}
	m_sidePots.clear();
}

// SidePot 을 생성만 한다.
// SidePot 이 sideMoney 순서대로 생성되지는 않기 때문에 생성하고 재구성이 필요하다.
// sideMoney 는 플레이어가 현재까지 읽은 금액
bool cSidePotManager::CreateSidePot( const uint64 playerIdx , const uint64 sideMoney )
{
	// sideMoney 보다 적은 금액으로 설정된 sidePot 에는 권한이 설정 되어야 한다.
	auto iter = m_sidePots.find( sideMoney );
	if ( iter == m_sidePots.end() )
	{
		cSidePot* pSidePot = new cSidePot( 0 , sideMoney );
		m_sidePots.insert( std::pair<uint64 , cSidePot*>( sideMoney , pSidePot ) );
		pSidePot->AddSidePlayer( playerIdx );
	}
	// 사이드 팟이 기존에 존재한다.
	else
	{
		iter->second->AddSidePlayer( playerIdx );
	}

	// 사이드팟 재구성
	// sideMoney 보다 적은 사이드팟에도 전체 등록해준다.

	uint64 beforePotSideMoney = 0;
	for ( auto sidePotPair : m_sidePots ) {
		if ( sidePotPair.second == nullptr ) continue;

		if ( sidePotPair.second->m_baseAmount < sideMoney ) {
			sidePotPair.second->AddSidePlayer( playerIdx );
		}

		sidePotPair.second->m_beforePotBaseAmount = beforePotSideMoney;

		beforePotSideMoney = sidePotPair.second->m_baseAmount;
	}

	return true;
}

// 플레이어가 베팅한 금액중에
// 최종 베팅금액보다 적은 사람이 있는 경우 사이드 팟을 생성시켜 준다.
bool cSidePotManager::CreateHiddenSidePot( const uint64 playerIdx , const uint64 playerBetMoney , const uint64 lastBetMoney )
{
	if ( playerBetMoney < lastBetMoney ) {

		auto iter = m_sidePots.find( playerBetMoney );
		if ( iter == m_sidePots.end() ) {
			if ( CreateSidePot( playerIdx , playerBetMoney ) )
				return true;
		}
	}
	return false;
}

void cSidePotManager::PushSidePotOnResult( const uint64 playerIdx , const uint64 sideMoney )
{
	uint64 beforeSidePotBaseAmount = 0;
	for ( auto& sidePair : m_sidePots ) {
		if ( sidePair.second == nullptr ) continue;

		// sidepot 의 금액보다 크다면 분배 권한을 갖도록 등록된다.
		sidePair.second->AddDistibuter( playerIdx , beforeSidePotBaseAmount , sideMoney );

		// 사이드팟 기준금액을 해당 사이드팟의 기준금액으로 갱신
		beforeSidePotBaseAmount = sidePair.first;
	}
}

bool cSidePotManager::WinnersHasSide( std::vector<uint64> winner_player_idx_list )
{
	/*std::vector<cSidePot*> winners_side;
	for ( auto sidePot : m_sidePots )
	{
		uint64 playerIdx = sidePot->m_sidePotPlayerIdx;
		if ( std::find( winner_player_idx_list.begin() , winner_player_idx_list.end() , playerIdx ) != winner_player_idx_list.end() )
			return true;
	}*/
	return false;
}

cSidePot* cSidePotManager::GetSide( uint64 playerIdx )
{
	/*for ( auto sidePot : m_sidePots )
	{
		if ( playerIdx == sidePot.second->m_sidePotPlayerIdx )
			return sidePot.second;
	}*/
	return nullptr;
}

std::map<uint64 , cSidePot*> cSidePotManager::GetSides( std::vector<uint64> playerIdxList )
{
	std::map<uint64 , cSidePot*> playerSidePots;

	for ( auto sidePotPair : m_sidePots ) {
		
		auto sidePot = sidePotPair.second;
		if ( sidePot == nullptr ) continue;

		if ( sidePot->areSidePotOwners( playerIdxList ) ) {
			playerSidePots.insert(std::pair<uint64 , cSidePot*>( sidePotPair.first, sidePotPair.second ));
		}
	}
	return playerSidePots;
}

bool cSidePotManager::HasSidePotOwner( const uint64 playerIdx )
{
	for ( auto sidePotPair : m_sidePots ) {

		auto sidePot = sidePotPair.second;
		if ( sidePot == nullptr ) continue;

		if ( sidePot->isSidePotOwner( playerIdx ) ) {
			return true;
		}
	}
	return false;
}

void cSidePotManager::DeleteSidePots( const std::vector<uint64>& sideKeys )
{
	for( auto key : sideKeys )
	{
		auto iter = m_sidePots.find(key);
		if ( iter != m_sidePots.end() ) {
			delete iter->second;
			m_sidePots.erase( iter );
		}
	}
}


uint64 cSidePotManager::CurCompletedSidePotMoney()
{
	uint64 sideMoney = 0;

	for ( auto sidePotPair : m_sidePots ) {

		auto sidePot = sidePotPair.second;
		if ( sidePot == nullptr ) continue;

		if ( sidePot->isCompleted() ) {

			if ( sideMoney < sidePot->m_baseAmount )
				sideMoney = sidePot->m_baseAmount;
		}
	}
	return sideMoney;
}