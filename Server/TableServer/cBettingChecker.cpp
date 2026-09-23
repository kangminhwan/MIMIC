#include "cBettingChecker.h"

void cBettingChecker::CreateStockers()
{
	// clear 만 하면서 계속 재활용을 하도록 한다.

	BETTING_ROUND_VEC* bet_1 = new BETTING_ROUND_VEC();
	BETTING_ROUND_VEC* bet_2 = new BETTING_ROUND_VEC();
	BETTING_ROUND_VEC* bet_3 = new BETTING_ROUND_VEC();
	BETTING_ROUND_VEC* bet_4 = new BETTING_ROUND_VEC();

	std::vector<General::TableAction> bet_seq;
	bet_1->insert( std::pair<int, std::vector<General::TableAction>>( 1, bet_seq ) );
	bet_1->insert( std::pair<int, std::vector<General::TableAction>>( 2, bet_seq ) );
	m_bettingStocks.insert( std::pair<Server::PlayPhase , BETTING_ROUND_VEC*>(Server::PlayPhase::PlayPhase_FirstBet, bet_1) );

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

void cBettingChecker::Delete()
{
	for ( auto betStocks : m_bettingStocks ) {

		BETTING_ROUND_VEC* betStock = betStocks.second; // std::map<int , std::vector<General::TableAction>>

		for ( auto iter = betStock->begin(); iter != betStock->end(); ++iter ) {
			int seq = ( *iter ).first;
			( *iter ).second.clear();	// std::vector<General::TableAction>
		}
		delete betStock;
	}
	m_bettingStocks.clear();
}

// 베팅 기록을 초기화 한다.
void cBettingChecker::Clear()
{
	for ( auto betStocks : m_bettingStocks ) {

		BETTING_ROUND_VEC* betStock = betStocks.second; // std::map<int , std::vector<General::TableAction>>

		for ( auto iter = betStock->begin(); iter != betStock->end(); ++iter ) {
			( *iter ).second.clear(); // std::vector<General::TableAction>
		}
	}
}

// false 체크 할건지 판단 필요
bool cBettingChecker::Bet( Server::PlayPhase bettingStep , const int betRound , General::TableAction betting )
{
	std::map<Server::PlayPhase , BETTING_ROUND_VEC*>::iterator it = m_bettingStocks.find( bettingStep );
	if ( it == m_bettingStocks.end() )
		return false;
	
	BETTING_ROUND_VEC* founded = it->second;
	//std::map<int , std::vector<General::TableAction>>::iterator betRoundIter = founded->find( betRound );
	auto betRoundIter = founded->find( betRound );
	if ( betRoundIter == founded->end() )
		return false;
	
	betRoundIter->second.push_back( betting );
	return true;
}

// 이번 스텝에 삥이 이미 있는지 여부
bool cBettingChecker::StepBbingCheck( Server::PlayPhase bettingStep )
{
	std::map<Server::PlayPhase , BETTING_ROUND_VEC*>::iterator it = m_bettingStocks.find( bettingStep );
	if ( it == m_bettingStocks.end() )
		return false;
		//throw std::exception("cLowBadukiBettingChecker::StepBbingCheck Exception"); // 이문제는 생기면 안됨. m_bettingStocks 클리어 하는 로직이 있는 것으로 보임. betRound 의 vector clear만 해야한다.

	BETTING_ROUND_VEC* founded = it->second;

	for ( auto iterRound = founded->begin(); iterRound != founded->end(); ++iterRound ) {
		for ( auto iterVec : ( *iterRound ).second ) {
			if ( iterVec == General::TableAction::TableAction_SeedOnly )
				return true;
		}
	}
	return false;
}

// STEP 사이즈 합산해서 0 일때만 최초로 판정한다.
bool cBettingChecker::IsStepFirstBet( Server::PlayPhase bettingStep )
{
	std::map<Server::PlayPhase , BETTING_ROUND_VEC*>::iterator it = m_bettingStocks.find( bettingStep );
	if ( it == m_bettingStocks.end() )
	{
		throw std::runtime_error( "cLowBadukiBettingChecker::IsStepFirstBet Exception" );
	}

	BETTING_ROUND_VEC* founded = it->second;

	int stepBetTotal = 0;
	for ( auto iterRound = founded->begin(); iterRound != founded->end(); ++iterRound )
		stepBetTotal += ( *iterRound ).second.size();

	return stepBetTotal == 0;
}

// PenultimateBet -> 마지막 베팅 전의 베팅 ( 단 Die, Call 이 아닌 )
General::TableAction cBettingChecker::GetPenultimateBet( Server::PlayPhase bettingStep , const int betRound , bool& gotoNextRound )
{
	gotoNextRound = true;

	std::map<Server::PlayPhase , BETTING_ROUND_VEC*>::iterator it = m_bettingStocks.find( bettingStep );
	if ( it == m_bettingStocks.end() )
		throw std::runtime_error( "cLowBadukiBettingChecker::StepBbingCheck Exception 1" ); // 이문제는 생기면 안됨. m_bettingStocks 클리어 하는 로직이 있는 것으로 보임. betRound 의 vector clear만 해야한다.

	BETTING_ROUND_VEC* founded = it->second;

	auto round = founded->find( betRound );
	if ( round == founded ->end() )
		throw std::runtime_error( "cLowBadukiBettingChecker::StepBbingCheck Exception 2, Round Not Founded" ); // 이문제는 생기면 안됨. m_bettingStocks 클리어 하는 로직이 있는 것으로 보임. betRound 의 vector clear만 해야한다.

	std::vector<General::TableAction>& bettings = round->second;

	for ( auto iter = bettings.rbegin(); iter != bettings.rend(); ++iter ) {
		if ( ( *iter ) == General::TableAction::TableAction_GiveUp || ( *iter ) == General::TableAction::TableAction_Call || iter == bettings.rbegin() )
			continue;
		return ( *iter );
	}

	// 여기 까지 오면 베팅한 유저가 없는 겁니다??
	//throw std::exception( "cLowBadukiBettingChecker::StepBbingCheck Exception 3, All Die Or No Betting" ); // 이문제는 생기면 안됨. m_bettingStocks 클리어 하는 로직이 있는 것으로 보임. betRound 의 vector clear만 해야한다.
	gotoNextRound = false;
}

// 이번라운드에 Betting 이 없으면 베팅이 없는 것으로 간주한다.
bool cBettingChecker::RoundHasBetting( Server::PlayPhase bettingStep , const int betRound )
{
	std::map<Server::PlayPhase , BETTING_ROUND_VEC*>::iterator it = m_bettingStocks.find( bettingStep );
	if ( it == m_bettingStocks.end() )
		throw std::runtime_error( "cLowBadukiBettingChecker::StepBbingCheck Exception 1" ); // 이문제는 생기면 안됨. m_bettingStocks 클리어 하는 로직이 있는 것으로 보임. betRound 의 vector clear만 해야한다.

	BETTING_ROUND_VEC* founded = it->second;

	auto round = founded->find( betRound );
	if ( round == founded->end() )
		throw std::runtime_error( "cLowBadukiBettingChecker::StepBbingCheck Exception 2, Round Not Founded" ); // 이문제는 생기면 안됨. m_bettingStocks 클리어 하는 로직이 있는 것으로 보임. betRound 의 vector clear만 해야한다.

	std::vector<General::TableAction>& bettings = round->second;

	for ( auto iter = bettings.begin(); iter != bettings.end(); ++iter ) {
		if ( ( *iter ) == General::TableAction::TableAction_GiveUp || ( *iter ) == General::TableAction::TableAction_Call )
			continue;
		return true;
	}
	return false;
}