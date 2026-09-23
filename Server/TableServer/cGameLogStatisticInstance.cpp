#include "cClientSession.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
//#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cGameLogStatisticInstance.h"

#include "Query.h"

#include <future>

cGameLogStatisticInstance::~cGameLogStatisticInstance()
{
	WriteGameLog();
}


void cGameLogStatisticInstance::PushLog( 
	uint64_t win_money ,
	uint64 lose_money ,
	General::BenefitTier member_ship_class ,
	uint64 dealer_cost )
{
	m_win_money += win_money;
	m_lose_money += lose_money;

	switch ( member_ship_class )
	{
	case General::BenefitTier::BenefitTier_None:
	case General::BenefitTier::BenefitTier_Basic:
		m_dealer_cost_normal += dealer_cost;
		break;
	case General::BenefitTier::BenefitTier_Standard:
		m_dealer_cost_regular += dealer_cost;
		break;
	case General::BenefitTier::BenefitTier_Premium:
		m_dealer_cost_top += dealer_cost;
		break;
	}

	++m_member_count;
}

void cGameLogStatisticInstance::PushMemberPlayerIndex( std::string member_player_index )
{
	if ( m_member_player_idx.empty() )
		m_member_player_idx = member_player_index;
	else
		m_member_player_idx = std::format( "{},{}" , m_member_player_idx , member_player_index );
}

void cGameLogStatisticInstance::WriteGameLog()
{
	if ( true == m_bIgnore )
		return;

	m_member_player_idx = std::format( "[{}]" , m_member_player_idx );

	// 게임 로그
	std::string kicked_platform_guid;
	auto result = QueryManager::GamelogTableStatisticInsert(
	( int ) m_game_type ,
	( int ) m_money_type ,
	m_channel_id ,
	m_member_count ,
	m_win_money ,
	m_lose_money ,
	m_dealer_cost_normal ,
	m_dealer_cost_regular ,
	m_dealer_cost_top ,
	m_member_player_idx );

	result.wait();

	if ( FALSE == result.get() ) {

		// 로그 저장 실패
	}
}