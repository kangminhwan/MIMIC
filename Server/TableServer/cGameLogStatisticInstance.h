#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"

// 소멸자 로그처리를 위한 호출을 한다.
class cGameLogStatisticInstance
{
private:
	General::PlayCategory m_game_type;
	General::AssetKind m_money_type;
	General::BetPolicy m_betting_rule_type;
	std::string m_channel_id;

	uint64 m_win_money;									// 승리한 유저가 획득한 금액
	uint64 m_lose_money;								// 패배한 유저가 잃은 금액

	uint64 m_dealer_cost_normal;
	uint64 m_dealer_cost_regular;
	uint64 m_dealer_cost_top;

	int m_member_count;

	std::string m_member_player_idx;
	bool m_bIgnore;
public:

	void PushLog(
		uint64_t win_money ,
		uint64 lose_money ,
		General::BenefitTier member_ship_class ,
		uint64 dealer_cost );

	void PushMemberPlayerIndex( std::string member_player_index );

	void IgnoreGameLog( bool bIgnore ) { m_bIgnore = bIgnore; }

	void WriteGameLog();

private:
	cGameLogStatisticInstance() {
		m_game_type = General::PlayCategory::PlayCategory_None;
		m_money_type = General::AssetKind::AssetKind_None;
		m_betting_rule_type = General::BetPolicy::BetPolicy_None;
		m_channel_id = "";
		m_win_money = 0;									// 승리한 유저가 획득한 금액m_win_money = 0;
		m_lose_money = 0;								// 패배한 유저가 잃은 금액
		m_dealer_cost_normal = 0;
		m_dealer_cost_regular = 0;
		m_dealer_cost_top = 0;
		m_member_count = 0;
		m_member_player_idx = "";
		m_bIgnore = false;
	}

public:
	cGameLogStatisticInstance(
		const General::PlayCategory game_type ,
		const General::AssetKind money_type ,
		const General::BetPolicy betting_rule_type ,
		const std::string channel_id ) {
		m_game_type = game_type;
		m_money_type = money_type;
		m_betting_rule_type = betting_rule_type;
		m_channel_id = channel_id;
		m_win_money = 0;									// 승리한 유저가 획득한 금액m_win_money = 0;
		m_lose_money = 0;								// 패배한 유저가 잃은 금액
		m_dealer_cost_normal = 0;
		m_dealer_cost_regular = 0;
		m_dealer_cost_top = 0;
		m_member_count = 0;
		m_member_player_idx = "";
		m_bIgnore = false;
	}
	~cGameLogStatisticInstance();
};