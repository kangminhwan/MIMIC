#include "cClientSession.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
//#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cGameLogInstance.h"

#include "Query.h"

#include <future>

cGameLogInstance::~cGameLogInstance()
{
	WriteGameLog();
}

std::string CardTypeToString( General::CardSuit type ) {
	switch ( type ) {
	case General::CardSuit::CardSuit_Heart: return "H";
	case General::CardSuit::CardSuit_Diamond: return "D";
	case General::CardSuit::CardSuit_Club: return "C";
	case General::CardSuit::CardSuit_Spade: return "S";
	default: return "";
	}
}

std::string CardNumTypeToString( General::CardRank num ) {
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

std::string CardsToString( const General::CardSet& cards )
{
	std::ostringstream oss;
	for ( const auto& card : cards.playing_cards() ) {
		oss << CardTypeToString( card.suit_code() ) << CardNumTypeToString( card.rank_code() ) << ",";
	}
	std::string result = oss.str();
	if ( !result.empty() ) {
		result.pop_back();
	}
	return result;
}

std::string ConvertCardsToString( const std::vector<General::PlayingCard>& cards ) {
	std::ostringstream oss;
	for ( const auto& card : cards ) {
		oss << CardTypeToString( card.suit_code() ) << CardNumTypeToString( card.rank_code() ) << ",";
	}
	return oss.str();
}

std::string VectorToString( const std::vector<std::string>& vec )
{
	std::ostringstream oss;
	for ( const auto& str : vec ) {
		oss << str << ",";
	}
	std::string result = oss.str();
	if ( !result.empty() ) {
		result.pop_back();
	}
	return result;
}



// Function to convert the inner map to a string


void cGameLogInstance::PushLoser(
	const std::string& platform_guid ,
	const std::string& ip ,
	uint64_t held_amount ,
	int64 lose_amount ,
	uint64_t remain_amount ,
	uint64_t rake_amount ,
	General::HandRank jokbo ,
	const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards )
{
	m_loser_platform_guids.push_back( platform_guid );
	m_loser_ips.push_back( ip );
	m_loser_held_amounts.push_back( held_amount );
	m_loser_lose_amounts.push_back( lose_amount );
	m_loser_remain_amounts.push_back( remain_amount );
	m_loser_rake_amounts.push_back( rake_amount );
	m_lose_jokbo.push_back( jokbo );

	General::CardSet _cards;
	for ( const auto& card : cards ) {
		auto add_card = _cards.add_playing_cards();
		add_card->CopyFrom( card );
	}
	m_loser_cards.push_back( _cards );
}

void cGameLogInstance::PushWinner(
	const std::string& platform_guid ,
	const std::string& ip ,
	uint64_t held_amount ,
	int64 get_amount ,
	uint64_t remain_amount ,
	uint64_t rake_amount ,
	General::HandRank jokbo ,
	const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards )
{
	m_winner_platform_guids.push_back( platform_guid );
	m_winner_ips.push_back( ip );
	m_winner_held_amounts.push_back( held_amount );
	m_winner_get_amounts.push_back( get_amount );
	m_winner_remain_amounts.push_back( remain_amount );
	m_winner_rake_amounts.push_back( rake_amount );
	m_winner_jokbo.push_back( jokbo );

	General::CardSet _cards;
	for ( const auto& card : cards ) {
		auto add_card = _cards.add_playing_cards();
		add_card->CopyFrom( card );
	}
	m_winner_cards.push_back( _cards );
}





void cGameLogInstance::PushCommunityCard( std::vector<General::PlayingCard>& communityCard )
{
	m_community_cards.Clear();

	General::CardSet _cards;
	for ( const auto& card : communityCard ) {
		auto add_card = m_community_cards.add_playing_cards();
		add_card->CopyFrom( card );
	}
}









void cGameLogInstance::WriteGameLog()
{
	switch ( m_game_type )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// 패배자들의 카드 정보
		std::string loser_card_info;
		for ( const auto& loserCards : m_loser_cards ) {
			loser_card_info += "[";
			loser_card_info += CardsToString( loserCards );
			loser_card_info += "]";
		}

		// 홀덤 커뮤니티 카드 정보
		if ( m_community_cards.playing_cards().size() ) {
			loser_card_info += " | ";
			loser_card_info += "[";
			loser_card_info += CardsToString( m_community_cards );
			loser_card_info += "]";
		}

		// 로그 안씀
		/*
		// 패배 유저 기준으로 로그를 기록한다.
		for ( int n = 0; n < m_loser_platform_guids.size(); ++n ) {

			// 게임 로그
			std::string kicked_platform_guid;
			auto result = QueryManager::GamelogTableInsert(
			( int ) m_game_type ,
			( int ) m_money_type ,
			m_channel_id ,
			"201" , // 게임 결과 코드\r\n승리 = 101\r\n패배 = 201\r\n무승부 = 303'
			( int ) m_betting_rule_type ,
			m_loser_platform_guids[ n ] ,
			m_loser_ips[ n ] ,
			m_loser_held_amounts[ n ] ,
			m_loser_lose_amounts[ n ] ,
			m_loser_remain_amounts[ n ] ,
			m_loser_rake_amounts[ n ] ,
			CardsToString( m_loser_cards[ n ] ) + " | " + CardsToString( m_winner_cards[ 0 ] ) ,
			VectorToString( m_winner_platform_guids ) ,
			VectorToString( m_winner_ips ) ,
			m_winner_held_amounts[ 0 ] ,
			m_winner_get_amounts[ 0 ] ,
			m_winner_remain_amounts[ 0 ] ,
			m_winner_rake_amounts[ 0 ] ,
			CardsToString( m_winner_cards[ 0 ] ) + " | " + loser_card_info ,
			kicked_platform_guid );

			result.wait();

			if ( FALSE == result.get() ) {

				// 로그 저장 실패
			}
		}
		*/
	}
	break;
	}

	


}