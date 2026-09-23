#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"



// 소멸자 로그처리를 위한 호출을 한다.
class cGameLogInstance
{
private:
	General::PlayCategory m_game_type;
	General::AssetKind m_money_type;
	General::BetPolicy m_betting_rule_type;
	std::string m_channel_id;

	std::vector<std::string> m_loser_platform_guids;	// 패배 유저 플랫폼 아이디
	std::vector<std::string> m_loser_ips;				// 패배 유저 아이피
	std::vector<uint64> m_loser_held_amounts;			// 패배 유저 소지 금액
	std::vector<int64> m_loser_lose_amounts;			// 패배 유저 손실 금액
	std::vector<uint64> m_loser_remain_amounts;			// 패배 유저 남은 금액
	std::vector<uint64> m_loser_rake_amounts;			// 패배 유저 적립 금액
	std::vector<General::CardSet> m_loser_cards;			// 패배 유저 카드 정보
	std::vector<General::HandRank> m_lose_jokbo;			// 패배 유저 족보

	std::vector<std::string> m_winner_platform_guids;	// 승리 유저 플랫폼 아이디
	std::vector<std::string> m_winner_ips;				// 승리 유저 아이피
	std::vector<uint64> m_winner_held_amounts;			// 승리 유저 소지 금액
	std::vector<int64> m_winner_get_amounts;			// 승리 유저 획득 금액
	std::vector<uint64> m_winner_remain_amounts;		// 승리 유저 남은 금액
	std::vector<uint64> m_winner_rake_amounts;			// 승리 유저 적립 금액
	std::vector<General::CardSet> m_winner_cards;			// 승리 유저 카드 정보
	std::vector<General::HandRank> m_winner_jokbo;			// 승리 유저 족보

	General::CardSet m_community_cards;					// 홀덤 커뮤니티 카드

	// 바카라용

	// 블랙잭용

	
public:

	// PushLoser, PushWinner 는 바두기, 홀덤 공통
	void PushLoser(
		const std::string& platform_guid ,
		const std::string& ip ,
		uint64_t held_amount ,
		int64 lose_amount ,
		uint64_t remain_amount ,
		uint64_t rake_amount ,
		General::HandRank jokbo ,
		const google::protobuf::RepeatedPtrField<General::PlayingCard>& cardss );

	void PushWinner(
		const std::string& platform_guid ,
		const std::string& ip ,
		uint64_t held_amount ,
		int64 get_amount ,
		uint64_t remain_amount ,
		uint64_t rake_amount ,
		General::HandRank jokbo ,
		const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards );

	// PushCommunityCard 는 홀덤에서만 사용
	void PushCommunityCard( std::vector<General::PlayingCard>& communityCard );

	void WriteGameLog();

private:
	cGameLogInstance() {
		m_game_type = General::PlayCategory::PlayCategory_None;
		m_money_type = General::AssetKind::AssetKind_None;
		m_betting_rule_type = General::BetPolicy::BetPolicy_None;
	}

public:
	cGameLogInstance( 
		const General::PlayCategory game_type ,
		const General::AssetKind money_type ,
		const General::BetPolicy betting_rule_type ,
		const std::string channel_id ) {
		m_game_type = game_type;
		m_money_type = money_type;
		m_betting_rule_type = betting_rule_type;
		m_channel_id = channel_id;
	}
	~cGameLogInstance();
};