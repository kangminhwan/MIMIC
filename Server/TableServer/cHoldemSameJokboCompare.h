#pragma once
#include "TableServerHeader.h"

#include "IGame.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

class cHoldemSameJokboCompare
{
private:
	PmNet::MemberOutcome m_playerResult;
	uint64 m_jokboPoint;
	int m_fixedJokboSize;

private:
	cHoldemSameJokboCompare() {};

public:
	cHoldemSameJokboCompare( const PmNet::MemberOutcome& playerResult , const std::vector<General::PlayingCard>& sortedJokbo ) {
		m_jokboPoint = 0;
		m_playerResult = playerResult;
		CalcJokboPoint( sortedJokbo );
	}

	inline PmNet::MemberOutcome GetResult() { return m_playerResult; }
	inline uint64 GetPoint() { return m_jokboPoint; }

	static bool SortDesc( cHoldemSameJokboCompare& compare1 , cHoldemSameJokboCompare& compare2 )
	{
		if ( compare1.GetPoint() < compare2.GetPoint() )
			return false;

		if ( compare1.GetPoint() > compare2.GetPoint() )
			return true;

		return false;
	}

private:

	// 무조건 총 12자리의 uint64 로 만들어서 점수로 우열을 정한다.
	// 따라서 족보가 앞서는 카드는 무조건 점수가 더 커진다.
	void CalcJokboPoint( const std::vector<General::PlayingCard>& sortedJokbo ) {

		assert( sortedJokbo.size() <= 5 && "CalcJokboPoint Count Exception" );

		// 0 부터 4가 되겠지?
		int pushMore = 5 - sortedJokbo.size();

		std::string numberString;

		int jokboNum = GetJokbo( m_playerResult.hand_rank() );
		numberString += std::to_string( jokboNum );

		for ( int n = 0; n < pushMore; ++n ) {
			numberString += "00";
		}

		// 족보에 따라 점수를 처리하는 기준이 달라짐
		switch ( m_playerResult.hand_rank() )
		{
		case General::HandRank::HandRank_HoldemStraight:
		case General::HandRank::HandRank_HoldemStraightFlush:
		{
			// A 스트레이트 인경우에는 A를 14로 계산한다.
			if ( sortedJokbo[ 0 ].rank_code() == General::CardRank::CardRank_Ace &&
				sortedJokbo[ 1 ].rank_code() == General::CardRank::CardRank_King )
			{
				// 카운트는 최대 5개
				for ( auto& card : sortedJokbo ) {
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
			}
			// A 스트레이트가 아닌 경우에는 A를 1로 계산한다.
			else
			{
				// 스트레이트는 A 를 1로 계산한다.
				// 카운트는 최대 5개
				for ( auto& card : sortedJokbo ) {
					// 카드의 숫자를 두 자리로 추가
					int cardNum = static_cast< int >( card.rank_code() );
					if ( cardNum == 1 ) {
						numberString += "01";
					}
					else if ( cardNum >= 2 && cardNum <= 9 ) {
						numberString += "0" + std::to_string( cardNum );
					}
					else {
						numberString += std::to_string( cardNum );
					}
				}
			}
		}
		break;
		default:
		{
			// 카운트는 최대 5개
			for ( auto& card : sortedJokbo ) {
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
		}
		break;
		}		

		m_jokboPoint = std::stoull( numberString );
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

//class cHoldemSameJokboCompare
//{
//private:
//	PmNet::MemberOutcome m_playerResult;
//	uint64 m_jokboPoint;
//	int m_fixedJokboSize;
//
//private:
//	cHoldemSameJokboCompare() {};
//
//public:
//	cHoldemSameJokboCompare( const PmNet::MemberOutcome& playerResult, const std::vector<General::PlayingCard>& sortedJokbo, const int fixedJokboSize ) {
//		m_jokboPoint = 0;
//		m_playerResult = playerResult;
//		m_fixedJokboSize = fixedJokboSize;
//		CalcJokboPoint( sortedJokbo );
//	}
//
//	inline PmNet::MemberOutcome GetResult() { return m_playerResult; }
//	inline uint64 GetPoint() { return m_jokboPoint; }
//
//	static bool SortDesc( cHoldemSameJokboCompare& compare1 , cHoldemSameJokboCompare& compare2 )
//	{
//		if ( compare1.GetPoint() < compare2.GetPoint() )
//			return false;
//
//		if ( compare1.GetPoint() > compare2.GetPoint() )
//			return true;
//
//		return false;
//	}
//
//private:
//
//	// 무조건 총 12자리의 uint64 로 만들어서 점수로 우열을 정한다.
//	// 따라서 족보가 앞서는 카드는 무조건 점수가 더 커진다.
//	void CalcJokboPoint( const std::vector<General::PlayingCard>& sortedJokbo ) {
//		int jokboCardCount = sortedJokbo.size();
//
//		assert( jokboCardCount <= 5 && "CalcJokboPoint Count Exception" );
//
//		int pushMore = m_fixedJokboSize - sortedJokbo.size();
//
//		std::string numberString;
//
//		int jokboNum = GetJokbo( m_playerResult.hand_rank() );
//		numberString += std::to_string( jokboNum );
//
//		// 카운트는 최대 5개
//		for ( auto& card : sortedJokbo ) {
//			// 카드의 숫자를 두 자리로 추가
//			int cardNum = static_cast< int >( card.rank_code() );
//			if( cardNum == 1 ) {
//				numberString += "14";
//			}
//			else if ( cardNum >= 2 && cardNum <= 9 ) {
//				numberString += "0" + std::to_string( cardNum );
//			}
//			else {
//				numberString += std::to_string( cardNum );
//			}
//		}
//
//		if ( pushMore ) {
//			for ( int n = 0; n < pushMore; ++n ){
//				numberString += "00";
//			}
//		}
//
//		// 0부터 시작하면 0 없앰
//		if ( numberString.starts_with( "0" ) )
//			numberString.erase( numberString.begin() );
//
//		m_jokboPoint = std::stoull( numberString );
//	}
//
//	static int GetJokbo( const General::HandRank& jokbo )
//	{
//		int jokboNum = 0;
//		switch ( jokbo )
//		{
//		case General::HandRank::HandRank_HoldemRoyalStraightFlush:
//			jokboNum = 20;
//			break;
//		case General::HandRank::HandRank_HoldemStraightFlush:
//			jokboNum = 19;
//			break;
//		case General::HandRank::HandRank_HoldemFourOfKind:
//			jokboNum = 18;
//			break;
//		case General::HandRank::HandRank_HoldemFullHouse:
//			jokboNum = 17;
//			break;
//		case General::HandRank::HandRank_HoldemFlush:
//			jokboNum = 16;
//			break;
//		case General::HandRank::HandRank_HoldemStraight:
//			jokboNum = 15;
//			break;
//		case General::HandRank::HandRank_HoldemThreeOfKind:
//			jokboNum = 14;
//			break;
//		case General::HandRank::HandRank_HoldemTwoPair:
//			jokboNum = 13;
//			break;
//		case General::HandRank::HandRank_HoldemOnePair:
//			jokboNum = 12;
//			break;
//		case General::HandRank::HandRank_HoldemHighCard:
//			jokboNum = 11;
//			break;
//		}
//
//		return jokboNum;
//	}
//};