#pragma once
#include "TableServerHeader.h"

#include "IGame.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

class cHoldemKicker
{
private:
	PmNet::MemberOutcome m_playerResult;
	uint64 m_kickerPoint;

private:
	cHoldemKicker() {};

public:
	cHoldemKicker( const PmNet::MemberOutcome& playerResult ) {
		m_kickerPoint = 0;
		m_playerResult = playerResult;
		CalcKickerPoint();
	}

	inline PmNet::MemberOutcome GetResult() { return m_playerResult; }
	inline uint64 GetPoint() { return m_kickerPoint; }

	static bool SortDesc( cHoldemKicker& kicker1 , cHoldemKicker& kicker2 )
	{
		if ( kicker1.GetPoint() < kicker2.GetPoint() )
			return false;

		if ( kicker1.GetPoint() > kicker2.GetPoint() )
			return true;

		return false;
	}

private:
	void CalcKickerPoint() {

		assert( m_playerResult.kicker_tiles().size() <= 5 && "CalcKickerPoint Count Exception" );

		int pushMore = 5 - m_playerResult.kicker_tiles().size();

		std::string numberString;

		int jokboNum = GetJokbo( m_playerResult.hand_rank() );
		numberString += std::to_string( jokboNum );

		// 카운트는 최대 5개
		for ( auto& card : m_playerResult.kicker_tiles() )
		{
			// 카드의 숫자를 두 자리로 추가
			int cardNum = static_cast< int >( card.rank_code() );
			if ( cardNum == 1 ) {
				numberString += "14";
			}
			else if ( cardNum >= 2 && cardNum <= 9 ) {
				numberString += "0" + std::to_string( cardNum );
			}
			else {
				numberString += std::to_string( cardNum );
			}
		}

		for ( int n = 0; n < pushMore; ++n ) {
			numberString += "00";
		}

		m_kickerPoint = std::stoull( numberString );
	}

	static int GetJokbo( const General::HandRank& jokbo )
	{
		int jokboNum = 0;
		switch ( jokbo )
		{
		case General::HandRank::HandRank_HoldemRoyalStraightFlush:
			jokboNum = 20;
			break;
		case General::HandRank::HandRank_HoldemStraightFlush:
			jokboNum = 19;
			break;
		case General::HandRank::HandRank_HoldemFourOfKind:
			jokboNum = 18;
			break;
		case General::HandRank::HandRank_HoldemFullHouse:
			jokboNum = 17;
			break;
		case General::HandRank::HandRank_HoldemFlush:
			jokboNum = 16;
			break;
		case General::HandRank::HandRank_HoldemStraight:
			jokboNum = 15;
			break;
		case General::HandRank::HandRank_HoldemThreeOfKind:
			jokboNum = 14;
			break;
		case General::HandRank::HandRank_HoldemTwoPair:
			jokboNum = 13;
			break;
		case General::HandRank::HandRank_HoldemOnePair:
			jokboNum = 12;
			break;
		case General::HandRank::HandRank_HoldemHighCard:
			jokboNum = 11;
			break;
		}

		return jokboNum;
	}

};