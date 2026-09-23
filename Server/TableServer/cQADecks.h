#pragma once

#include "TableServerHeader.h"

class cQADeck
{
public:
	bool m_holdem_qa_dealer_cheat; // 활성화 되어 있으면 QA의 카드 목록대로 처리한다. 초기화 처리 전까지는 이쪽을 타도록 한다.
	std::map<uint64 , std::vector<General::PlayingCard>> m_holdem_qa_dealer_decks;
	std::vector<General::PlayingCard> m_holdem_qa_community_cards;

	bool m_baduki_qa_dealer_cheat;
	std::map<uint64 , std::vector<General::PlayingCard>> m_baduki_qa_dealer_decks;

	void SetHoldemQA( const PmNet::StackDebugRQ& qaRequest )
	{
		m_holdem_qa_dealer_cheat = true;

		if ( qaRequest.shared_cards().size() ) {

			m_holdem_qa_community_cards.clear();
			for ( const auto& card : qaRequest.shared_cards() ) {
				m_holdem_qa_community_cards.push_back( card );
			}
		}


		// 1
		std::vector<General::PlayingCard> qa_cards;
		for ( const auto& card : qaRequest.cards_1() ) {
			qa_cards.push_back( card );
		}

		auto iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_1() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_1() , qa_cards ) );
		}

		// 2
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_2() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_2() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_2() , qa_cards ) );
		}

		// 3
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_3() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_3() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_3() , qa_cards ) );
		}

		// 4
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_4() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_4() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_4() , qa_cards ) );
		}

		// 5
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_5() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_5() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_5() , qa_cards ) );
		}

		// 6
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_6() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_6() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_6() , qa_cards ) );
		}

		// 7
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_7() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_7() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_7() , qa_cards ) );
		}

		// 8
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_8() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_8() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_8() , qa_cards ) );
		}

		// 9
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_9() ) {
			qa_cards.push_back( card );
		}

		iter = m_holdem_qa_dealer_decks.find( qaRequest.member_idx_9() );
		if ( iter != m_holdem_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_holdem_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_9() , qa_cards ) );
		}

		
	}

	void ClearHoldemQA( const PmNet::StackDebugRQ& qaRequest )
	{
		m_holdem_qa_dealer_cheat = false;

		for ( auto& pair : m_holdem_qa_dealer_decks ) {
			pair.second.clear();
		}

		m_holdem_qa_dealer_decks.clear();

		m_holdem_qa_community_cards.clear();
	}

	void SetBadukiQA( const PmNet::StackDebugRQ& qaRequest )
	{
		m_baduki_qa_dealer_cheat = true;

		std::vector<General::PlayingCard> qa_cards;
		for ( const auto& card : qaRequest.cards_1() ) {
			qa_cards.push_back( card );
		}

		// 1
		auto iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_1() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_1() , qa_cards ) );
		}

		// 2
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_2() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_2() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_2() , qa_cards ) );
		}

		// 3
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_3() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_3() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_3() , qa_cards ) );
		}

		// 4
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_4() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_4() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_4() , qa_cards ) );
		}

		// 5
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_5() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_5() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_5() , qa_cards ) );
		}

		// 6
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_6() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_6() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_6() , qa_cards ) );
		}

		// 7
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_7() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_7() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_7() , qa_cards ) );
		}

		// 8
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_8() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_8() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_8() , qa_cards ) );
		}

		// 9
		qa_cards.clear();
		for ( const auto& card : qaRequest.cards_9() ) {
			qa_cards.push_back( card );
		}

		iter = m_baduki_qa_dealer_decks.find( qaRequest.member_idx_9() );
		if ( iter != m_baduki_qa_dealer_decks.end() )
		{
			auto& cards = iter->second;
			cards.clear();
			cards = qa_cards;
		}
		else
		{
			m_baduki_qa_dealer_decks.insert( std::pair<uint64 , std::vector<General::PlayingCard>>( qaRequest.member_idx_9() , qa_cards ) );
		}
	}

	void ClearBadukiQA( const PmNet::StackDebugRQ& qaRequest )
	{
		m_baduki_qa_dealer_cheat = false;

		for ( auto& pair : m_baduki_qa_dealer_decks ) {
			pair.second.clear();
		}

		m_baduki_qa_dealer_decks.clear();
	}
};