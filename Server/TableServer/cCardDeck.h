#pragma once
#include "TableServerHeader.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <random> // std::random_device, std::mt19937

using std::default_random_engine;

// Deck 은 딱 52장만 만들어 둔다.
class cCardDeck
{
private:
	std::vector<General::PlayingCard> cardDeck;
	std::mt19937 gen;
	bool firstFlag = false;			//초기화플래그

public:
	cCardDeck() 
	{
		if ( false == firstFlag ) //어짜피한번이겠지만...
		{
			std::random_device rd;
			gen.seed( rd() );
			for ( int i = 0; i < 1000; ++i ) {
				gen();
			}
			firstFlag = true;
		}
		// 카드무늬, 카드 갯수
		int suits = (int)General::CardSuit::CardSuit_Spade;
		int numbers = ( int ) General::CardRank::CardRank_King;

		for (int suit = 1; suit <= suits; ++suit) {
			for (int number = 1; number <= numbers; ++number) {
				General::PlayingCard* pCard = new General::PlayingCard();
				pCard->set_suit_code(static_cast<General::CardSuit>(suit));
				pCard->set_rank_code(static_cast<General::CardRank>(number));
				cardDeck.push_back(*pCard);
			}
		}
	}


	std::mt19937& Getgen()
	{
		return gen;
	}

	inline std::vector<General::PlayingCard> GetDeck() { return cardDeck; }
	inline int DeckSize() { return cardDeck.size(); }

	std::vector<General::PlayingCard> Shuffle()
	{
		std::vector<General::PlayingCard> clone = GetClone();

		//std::mt19937 gen(rd());

		// 벡터를 무작위로 섞음
		std::shuffle(clone.begin(), clone.end(), gen);
		return clone;
	}

	//std::vector<General::PlayingCard> Shuffle(std::vector<General::PlayingCard>& cards)
	//{
	//	std::vector<General::PlayingCard> cloned;
	//	for ( const General::PlayingCard& card : cards ) {
	//		cloned.push_back( card );
	//	}

	//	//std::mt19937 gen( rd() );

	//	// 벡터를 무작위로 섞음
	//	std::shuffle( cloned.begin() , cloned.end() , gen );
	//	return cloned;
	//}
	std::vector<General::PlayingCard> Shuffle( std::vector<General::PlayingCard>& cards )
	{
		std::shuffle( cards.begin() , cards.end() , gen );
		return cards;
	}


private:
	int rand(int start, int end)
	{
		//std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis( start , end );
		return dis(gen);
	}

	std::vector<General::PlayingCard> GetClone() {
		std::vector<General::PlayingCard> cloned;
		for ( const General::PlayingCard& card : cardDeck ) {
			cloned.push_back( card );
		}
		return cloned;
	}
};