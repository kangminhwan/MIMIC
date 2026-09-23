#include "TableServerHeader.h"
#include "cHoldem.h"
#include "cCardDeck.h"
#include "cGameRoom.h"
#include "cClientSession.h"
#include "cProtoUtil.h"
#include "cLogicException.h"
#include "cHoldemKicker.h"
#include "cHoldemSameJokboCompare.h"

#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Common/cSingleton.h"

#include <algorithm>  
#include <iostream>
#include <format>

// Jokbo 계산 코드가 길어져 따로 Cpp 분리 하였다.
General::HandRank cHoldem::GetJokbo( google::protobuf::RepeatedField<General::PlayingCard> cards , std::vector<General::PlayingCard>& jokboCards )
{
	General::HandRank TableJokboType = General::HandRank::HandRank_HoldemBegin;
	int SpecialEff = -1;
	int HighCardNum = 0;
	int SecondCardNum = 0;

	bool bJokbo = false;
	jokboCards.clear();
	SpecialEff = -1;
	std::vector<General::PlayingCard> m_sortCards;

	for ( auto card : cards )
	{
		if ( card.suit_code() == General::CardSuit::CardSuit_None || card.rank_code() == General::CardRank::CardRank_None ) continue;
		m_sortCards.push_back( card );
	}

	std::sort( m_sortCards.begin() , m_sortCards.end(), SortAsc );

	// 같은 슈트에들어갈녀석~
	std::map<General::CardSuit , std::vector<General::PlayingCard>> _suits;
	std::vector<std::pair<General::CardRank , std::vector<General::PlayingCard>>> _pairs;

	for ( int i = 0; i < m_sortCards.size(); i++ )
	{
		auto _suit_iter = _suits.find( m_sortCards[ i ].suit_code() );
		if ( _suit_iter == _suits.end() ) {
			std::vector<General::PlayingCard> _temp;
			_suits.insert( std::pair<General::CardSuit , std::vector<General::PlayingCard>>( m_sortCards[ i ].suit_code() , _temp ) );
		}

		General::CardRank cardNumType = m_sortCards[ i ].rank_code();
		// _pairs 벡터에서 카드 숫자 타입에 해당하는 pair를 찾음
		auto _pair_iter = std::find_if( _pairs.begin() , _pairs.end() ,
			[cardNumType]( const std::pair<General::CardRank , std::vector<General::PlayingCard>>& element ) {
			return element.first == cardNumType;
		} );

		if ( _pair_iter == _pairs.end() ) {
			// 해당 숫자 타입이 없으면 새로 추가
			_pairs.push_back( std::make_pair( cardNumType , std::vector<General::PlayingCard>() ) );
			_pair_iter = _pairs.end() - 1; // 방금 추가된 요소를 가리키도록 iter 조정
		}


		_suits[ m_sortCards[ i ].suit_code() ].push_back( m_sortCards[ i ] );
		_pair_iter->second.push_back( m_sortCards[ i ] );
	}

	bool bFlush = false; // 플러시 여부
	std::vector<General::PlayingCard> _flushCards;
	General::CardSuit flushCardType = General::CardSuit::CardSuit_None;

	for ( int i = static_cast< int >( General::CardSuit::CardSuit_Heart ); i <= static_cast< int >( General::CardSuit::CardSuit_Spade ); i++ ) {
		auto _suit_iter = _suits.find( static_cast< General::CardSuit >( i ) );
		if ( _suit_iter != _suits.end() ) {
			if ( _suit_iter->second.size() >= 5 ) {
				bFlush = true;
				flushCardType = static_cast< General::CardSuit >( i );
				_flushCards = _suits[ static_cast< General::CardSuit >( i ) ];
			}
		}
	}
	
	bool bRoyalFlush = false;
	bool bStraightFlush = false;
	int _straightCount = 1;
	if ( bFlush ) {
		// 로티플 (A, 10, J, Q, K)
		// 스티플 (연속된 숫자~
		// 플러시 (무작위 숫자)

		// 하나의 슈트만이 올수있음
		std::vector<General::PlayingCard> royal;
		General::PlayingCard ace;
		ace.set_rank_code( General::CardRank::CardRank_Ace );
		royal.push_back( ace );
		General::PlayingCard ten;
		ten.set_rank_code( General::CardRank::CardRank_Ten );
		royal.push_back( ten );
		General::PlayingCard jack;
		jack.set_rank_code( General::CardRank::CardRank_Jack );
		royal.push_back( jack );
		General::PlayingCard queen;
		queen.set_rank_code( General::CardRank::CardRank_Queen );
		royal.push_back( queen );
		General::PlayingCard king;
		king.set_rank_code( General::CardRank::CardRank_King );
		royal.push_back( king );

		// _flushCards 는 작은수부터 위로 Sort 되어있음.
		for ( int i = 0; i < _flushCards.size(); i++ ) {
			if ( _flushCards[ i ].rank_code() == royal[ 0 ].rank_code() ) {
				auto _del_iter = royal.begin();
				royal.erase( _del_iter );
			}
		}

		bRoyalFlush = royal.size() == 0;

		if ( bRoyalFlush == false )
		{
			// 스트레이트면 플러시~
			// 로티플은 체크안해도됨
			// A 2 3 4 5 => 5스티플
			// 2 3 4 5 6 => 6스티플
			// 제일높은게 9 10 J Q K

			// 레드마인 #323
			for ( int i = _flushCards.size() - 1; i > 0; i-- )
			{
				// 모양이 같아서 같은 숫자는 나올수가없음
				if ( _flushCards[ i - 1 ].rank_code() + 1 == _flushCards[ i ].rank_code() )
				{
					_straightCount++;
					General::PlayingCard card;
					card.set_suit_code( flushCardType );
					card.set_rank_code( _flushCards[ i ].rank_code() );
					jokboCards.push_back( card );
					if ( _straightCount == 5 ) {
						bStraightFlush = true;
						break;
					}
				}
				else {
					_straightCount = 1;
					//_straightCount = 0;
					jokboCards.clear();
				}
			}

			// ADD 2024-01-02 스티플 아닌경우 m_JokboCards 클리어
			if ( bStraightFlush )
			{
				General::PlayingCard card;
				card.set_suit_code( flushCardType );
				int cardNumTypeNumber = (int)jokboCards[ jokboCards.size() - 1 ].rank_code() - 1;
				card.set_rank_code( static_cast< General::CardRank >( cardNumTypeNumber ) );
				jokboCards.push_back( card );
			}
			if ( bStraightFlush == false )
			{
				jokboCards.clear();
			}
		}
	}

	if ( bRoyalFlush )
	{
		// 나는 로티플!
		bJokbo = true;
		SpecialEff = 0;
		TableJokboType = General::HandRank::HandRank_HoldemRoyalStraightFlush;

		for ( int i = 0; i < m_sortCards.size(); i++ ) {
			if ( flushCardType == m_sortCards[ i ].suit_code() &&
				( m_sortCards[ i ].rank_code() == General::CardRank::CardRank_Ace
					|| m_sortCards[ i ].rank_code() == General::CardRank::CardRank_Ten
					|| m_sortCards[ i ].rank_code() == General::CardRank::CardRank_Jack
					|| m_sortCards[ i ].rank_code() == General::CardRank::CardRank_Queen
					|| m_sortCards[ i ].rank_code() == General::CardRank::CardRank_King ) ) {
				jokboCards.push_back( m_sortCards[ i ] );
			}
		}
		//SetJokboName();
		//HiLog.Log( ILog.Color.B , "나는 로티플 !!" );
		return TableJokboType;
	}
	else if ( bStraightFlush )
	{
		SpecialEff = 1;
		// 나는 스티플 !
		bJokbo = true;
		TableJokboType = General::HandRank::HandRank_HoldemStraightFlush;
		//SetJokboName();
		//HiLog.Log( ILog.Color.B , "나는 스티플 !!" );
		return TableJokboType;
	}

	// 풀 하우스, 포카드 체크 후 플러시 체크
	//var enumerator = _pairs.GetEnumerator();
	bool bTripleCard = false;
	bool bPairCard = false;
	bool bTwoPairCard = false;

	std::vector<General::CardRank> tripleCards;
	std::vector<General::CardRank> pairCards;

	for( auto iter = _pairs.begin(); iter != _pairs.end(); ++iter  )
	{
		if ( iter->second.size() == 4 )
		{
			// 포카드
			SpecialEff = 2;
			TableJokboType = General::HandRank::HandRank_HoldemFourOfKind;
			HighCardNum = ( int ) iter->second[ 0 ].rank_code();
			for ( int i = 0; i < m_sortCards.size(); i++ )
			{
				if ( HighCardNum == ( int ) m_sortCards[ i ].rank_code() )
					jokboCards.push_back( m_sortCards[ i ] );
			}
			//SetJokboName();
			return TableJokboType;
		}
		else if ( iter->second.size() == 3 )
		{
			// 트리플 ( 풀하우스 가능성도있음)
			bTripleCard = true;
			tripleCards.push_back( iter->second[ 0 ].rank_code() );
		}
		else if ( iter->second.size() == 2 )
		{
			// 페어
			bPairCard = true;
			pairCards.push_back( iter->second[ 0 ].rank_code() );
		}
	}

	bTwoPairCard = pairCards.size() >= 2;

	// 풀하우스
	// 풀하우스는 무조건 트리플은 한개이상 있어야함
	if ( ( bPairCard && bTripleCard ) || ( tripleCards.size() >= 2 ) )
	{
		if ( tripleCards.size() >= 2 )
		{
			// 트리플중 높은걸 트리플로 쓰고, 낮은걸 페어로 써야함
			// 트리플이 두개면 페어가 있을수가없음~!!
			HighCardNum = ( int ) tripleCards[ tripleCards.size() - 1 ];
			if ( tripleCards[ 0 ] == General::CardRank::CardRank_Ace )
				SecondCardNum = 1;
			else
				SecondCardNum = ( int ) tripleCards[ tripleCards.size() - 2 ];

			if ( SecondCardNum == 1 )
			{
				for ( int i = 0; i < m_sortCards.size(); i++ )
				{
					if ( m_sortCards[ i ].rank_code() == General::CardRank::CardRank_Ace )
						jokboCards.push_back( m_sortCards[ i ] );
				}
			}

			for ( int i = m_sortCards.size() - 1; i >= 0 && jokboCards.size() <= 4; i-- )
			{
				if ( ( int ) m_sortCards[ i ].rank_code() == SecondCardNum || ( int ) m_sortCards[ i ].rank_code() == HighCardNum )
					jokboCards.push_back( m_sortCards[ i ] );
			}
		}
		else
		{
			// 무조건 트리플, 페어 족보
			int tripleCardNum = ( int ) tripleCards[ 0 ];

			if ( pairCards[ 0 ] == General::CardRank::CardRank_Ace )
			{
				SecondCardNum = pairCards[ 0 ];
				HighCardNum = tripleCardNum;
			}
			
			else
			{
				int pairCardNum = ( int ) pairCards[ pairCards.size() - 1 ];

				SecondCardNum = pairCardNum;
				HighCardNum = tripleCardNum;
			}

			for ( int i = 0; i < m_sortCards.size(); i++ )
			{
				if ( ( int ) m_sortCards[ i ].rank_code() == HighCardNum || ( int ) m_sortCards[ i ].rank_code() == SecondCardNum )
					jokboCards.push_back( m_sortCards[ i ] );
			}
		}

		bJokbo = true;
		SpecialEff = 3;
		//HiLog.Log( ILog.Color.B , "나는 풀하우스!" );
		TableJokboType = General::HandRank::HandRank_HoldemFullHouse;
		//SetJokboName();
		return TableJokboType;
	}

	if ( bFlush )
	{
		// jokboCards 에 카드가 들어 있는 경우가 있어 클리어를 한번해준다.
		if ( jokboCards.size() > 0 )
			jokboCards.clear();

		// 나는 플러시~~
		bJokbo = true;
		SpecialEff = 4;
		TableJokboType = General::HandRank::HandRank_HoldemFlush;
		if ( _flushCards[ 0 ].rank_code() == General::CardRank::CardRank_Ace )
			HighCardNum = 1;
		else
			HighCardNum = ( int ) _flushCards[ _flushCards.size() - 1 ].rank_code();

		if ( HighCardNum == 1 )
		{
			jokboCards.push_back( _flushCards[ 0 ] );
		}

		// jokboCards 가 5장이 될때까지 뒤에서 부터 채운다.
		// Ace 인 경우에는 이미 jokboCards에 포함되어 있다.
		/*for ( auto iterReverse = _flushCards.rbegin(); iterReverse != _flushCards.rend(); ++iterReverse ) {
			jokboCards.push_back( *iterReverse );

			if ( jokboCards.size() >= 5 )
				break;
		}*/

		for ( int i = _flushCards.size() - 1; i >= 0 && jokboCards.size() <= 4; i-- )
		{
			jokboCards.push_back( _flushCards[ i ] );
		}

		//SetJokboName();
		return TableJokboType;
	}

	// 스트레이트 체크
	bool bAceKing = m_sortCards[ 0 ].rank_code() == General::CardRank::CardRank_Ace && m_sortCards[ m_sortCards.size() - 1 ].rank_code() == General::CardRank::CardRank_King;
	bStraightFlush = false;
	_straightCount = bAceKing ? 2 : 1;

	General::CardRank straightCardNum = bAceKing ? General::CardRank::CardRank_Ace : General::CardRank::CardRank_None;
	if ( bAceKing )
	{
		jokboCards.push_back( m_sortCards[ 0 ] );
	}

	for ( int i = m_sortCards.size() - 1; i > 0; i-- )
	{
		// 숫자가 제일 큰것부터 체크
		if ( m_sortCards[ i ].rank_code() == m_sortCards[ i - 1 ].rank_code() ) continue; // 숫자가 같으면 continue
		if ( m_sortCards[ i ].rank_code() - 1 == m_sortCards[ ( i - 1 ) ].rank_code() )
		{
			if ( straightCardNum == General::CardRank::CardRank_None )
				straightCardNum = m_sortCards[ i ].rank_code();

			// 24-03-18 스트레이트 체크 로직 변경
			jokboCards.push_back( m_sortCards[ i ] );
			if ( jokboCards.size() + 1 >= 5 )
			{
				jokboCards.push_back( m_sortCards[ i - 1 ] );
				bStraightFlush = true;
				break;
			}

			// m_JokboCards.Add( m_sortCards[ i ] );
			//if (straightCardNum == CardNumType.None)
			//    straightCardNum = m_sortCards[i].CardNumType;

			//_straightCount++;
			//if ( _straightCount >= 5 )
			//{
			//	// 스트레이트
			//	jokboCards.push_back( m_sortCards[ i - 1 ] );
			//	bStraightFlush = true;
			//	break;
			//}
		}
		else
		{
			jokboCards.clear();
			_straightCount = 1;
			straightCardNum = General::CardRank::CardRank_None;
		}
	}

	if ( bStraightFlush )
	{
		// 나는 스트레이트 !
		SpecialEff = 4;
		bJokbo = true;
		TableJokboType = General::HandRank::HandRank_HoldemStraight;
		HighCardNum = ( int ) straightCardNum;
		//SetJokboName();
		return TableJokboType;
	}
	else
	{
		jokboCards.clear();
	}

	if ( bTripleCard )
	{
		// 나는 트리플!
		bJokbo = true;
		TableJokboType = General::HandRank::HandRank_HoldemThreeOfKind;
		if ( tripleCards[ 0 ] == General::CardRank::CardRank_Ace )
			HighCardNum = 1;
		else
		{
			HighCardNum = ( int ) tripleCards[ tripleCards.size() - 1 ];
		}

		for ( int i = 0; i < m_sortCards.size(); i++ )
		{
			if ( ( int ) m_sortCards[ i ].rank_code() == HighCardNum )
			{
				jokboCards.push_back( m_sortCards[ i ] );
			}
		}

		//SetJokboName();
		return TableJokboType;
	}

	if ( bTwoPairCard )
	{
		// 나는 투페어
		bJokbo = true;
		TableJokboType = General::HandRank::HandRank_HoldemTwoPair;
		HighCardNum = ( int ) pairCards[ pairCards.size() - 1 ];

		if ( pairCards[ 0 ] == General::CardRank::CardRank_Ace )
		{
			SecondCardNum = 1;
		}
		else
		{
			SecondCardNum = ( int ) pairCards[ pairCards.size() - 2 ];
		}

		for ( int i = 0; i < m_sortCards.size(); i++ )
		{
			if ( ( int ) m_sortCards[ i ].rank_code() == HighCardNum || ( int ) m_sortCards[ i ].rank_code() == SecondCardNum )
			{
				jokboCards.push_back( m_sortCards[ i ] );
			}
		}

		//SetJokboName();
		return TableJokboType;
	}

	if ( bPairCard )
	{
		// 나는 원페어
		bJokbo = true;
		TableJokboType = General::HandRank::HandRank_HoldemOnePair;
		HighCardNum = ( int ) pairCards[ 0 ];

		for ( int i = 0; i < m_sortCards.size(); i++ )
		{
			if ( ( int ) m_sortCards[ i ].rank_code() == HighCardNum )
			{
				jokboCards.push_back( m_sortCards[ i ] );
			}
		}
		//SetJokboName();
		return TableJokboType;
	}

	if ( bJokbo == false )
	{
		// 난 아무것도없어.. 하이카드...
		TableJokboType = General::HandRank::HandRank_HoldemHighCard;

		if ( m_sortCards[ 0 ].rank_code() == General::CardRank::CardRank_Ace )
		{
			HighCardNum = ( int ) m_sortCards[ 0 ].rank_code();
			jokboCards.push_back( m_sortCards[ 0 ] );
		}
		else
		{
			HighCardNum = ( int ) m_sortCards[ m_sortCards.size() - 1 ].rank_code();
			jokboCards.push_back( m_sortCards[ m_sortCards.size() - 1 ]);
		}

		//SetJokboName();
		return TableJokboType;
	}

	// TODO 리턴값 변경 필요
	return General::HandRank::HandRank_HoldemHighCard;
}

int cHoldem::SortAsc( General::PlayingCard& card1 , General::PlayingCard& card2 )
{
	if ( card1.rank_code() > card2.rank_code() )
		return false;

	if ( card1.rank_code() < card2.rank_code() )
		return true;

	return false;
}

int cHoldem::SortDesc( General::PlayingCard& card1 , General::PlayingCard& card2 )
{
	if ( card1.rank_code() < card2.rank_code() )
		return false;

	if ( card1.rank_code() > card2.rank_code() )
		return true;

	return false;
}

// 내림 차순
// PlayerResult 안에 kicker 카드까지 셋팅 되어서 비교가 되어야 한다.
bool cHoldem::ComparePlayerResult( PmNet::MemberOutcome& player1 , PmNet::MemberOutcome& player2 )
{
	// 족보 부터 비교
	if ( player1.hand_rank() < player2.hand_rank() )
		return true;

	if ( player1.hand_rank() > player2.hand_rank() )
		return false;

	// 족보가 동일한 경우 포인트 비교
	/*std::vector<General::PlayingCard> sortedJokbo1 = SortJokboCards( player1.cards() );
	std::vector<General::PlayingCard> sortedJokbo2 = SortJokboCards( player2.cards() );
	cHoldemSameJokboCompare compare1 = cHoldemSameJokboCompare( player1 , sortedJokbo1 );
	cHoldemSameJokboCompare compare2 = cHoldemSameJokboCompare( player2 , sortedJokbo2 );
	player1.set_hand_rank_score( compare1.GetPoint() );
	player2.set_hand_rank_score( compare2.GetPoint() );*/

	// 족보 카드에 대한 포인트를 비교한다.
	if ( player1.hand_rank_score() < player2.hand_rank_score() )
		return false;

	if ( player1.hand_rank_score() > player2.hand_rank_score() )
		return true;

	/*cHoldemKicker kicker1 = cHoldemKicker( player1 );
	cHoldemKicker kicker2 = cHoldemKicker( player2 );
	player1.set_kicker_score( kicker1.GetPoint() );
	player2.set_kicker_score( kicker2.GetPoint() );*/

	// 포인트 까지 동일하면 키커 비교 한다.
	if ( player1.kicker_score() < player2.kicker_score() )
		return false;

	if ( player1.kicker_score() > player2.kicker_score() )
		return true;

	return false;
}

// 오름 차순
// 족보만 비교 한다.
bool cHoldem::ComparePlayerResultByJokbo( PmNet::MemberOutcome& player1 , PmNet::MemberOutcome& player2 )
{
	// 족보 부터 비교
	if ( player1.hand_rank() < player2.hand_rank() )
		return true;

	if ( player1.hand_rank() > player2.hand_rank() )
		return false;

	return false;
}

// 족보 비교하고, 카드까지 비교한다.
bool cHoldem::ComparePlayerResultDetail( PmNet::MemberOutcome& player1 , PmNet::MemberOutcome& player2 )
{
	// 족보 부터 비교
	if ( player1.hand_rank() < player2.hand_rank() )
		return true;

	if ( player1.hand_rank() > player2.hand_rank() )
		return false;

	// 로티플이면 비교 의미 없음
	// catch로 잡아서 처리필요
	if ( player1.hand_rank() == General::HandRank::HandRank_HoldemRoyalStraightFlush )
		throw cLogicException("Draw");

	auto& cards1 = player1.cards();
	auto& cards2 = player2.cards();

	auto& card1 = cards1.at( 0 );
	auto& card2 = cards2.at( 0 );

	// ACE 는 여기서 비교 하고 끝낸다.
	if ( card1.rank_code() == General::CardRank::CardRank_Ace ) {
		// card1 이 ace 이고, card2 가 ace 가 아니면, card1 이 작은 경우 false 리턴
		if ( card1.rank_code() < card2.rank_code() )
			return false;
	}

	if ( card2.rank_code() == General::CardRank::CardRank_Ace ) {
		// card2 이 ace 이고, card1 가 ace 가 아니면, card2 가 작은 경우 true 리턴
		if ( card1.rank_code() > card2.rank_code() )
			return true;
	}

	// 승부에 사용 된 card 로 판단해야 한다.
	int cardSequence = 0;
	return CompareCardPerSequence( cards1 , cards2 , cardSequence );
}

// 카드의 순서대로 비교한다.
// 동일한 경우에는 하나씩 앞자리 카드로 이동하면서 비교한다.
// 재귀 호출을 통해 처리한다.
// cardSequence 0, 1, 2, 3 으로 비교
bool cHoldem::CompareCardPerSequence( const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards1 , const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards2 , int& cardSequence )
{
	General::PlayingCard lastCard_1 = cards1.at( cardSequence );
	General::PlayingCard lastCard_2 = cards2.at( cardSequence );

	if ( lastCard_1.rank_code() > lastCard_2.rank_code() )
		return false;

	if ( lastCard_1.rank_code() < lastCard_2.rank_code() )
		return true;

	// 마지막 어레이 까지 비교해서 더이상 비교 할 수가 없다.
	if ( cardSequence > cards1.size() - 1 )
		throw cLogicException( "Draw" );

	return CompareCardPerSequence( cards1 , cards2 , ++cardSequence );
}

std::string cHoldem::GetTypeString( General::CardSuit cardType )
{
	std::string result;
	switch ( cardType )
	{
	case General::CardSuit::CardSuit_Heart:
		result = "하트 ♥";
		break;
	case General::CardSuit::CardSuit_Diamond:
		result = "다이아 ◆";
		break;
	case General::CardSuit::CardSuit_Club:
		result = "클로버 ♣";
		break;
	case General::CardSuit::CardSuit_Spade:
		result = "스페이드 ♠";
		break;
	default:
		result = "무니에러";
		break;
	}
	return result;
}

std::string cHoldem::GetNumberString( General::CardRank cardNumType )
{
	std::string result;
	switch ( cardNumType )
	{
	case General::CardRank::CardRank_Ace:
		result = "1";
		break;
	case General::CardRank::CardRank_Two:
		result = "2";
		break;
	case General::CardRank::CardRank_Three:
		result = "3";
		break;
	case General::CardRank::CardRank_Four:
		result = "4";
		break;
	case General::CardRank::CardRank_Five:
		result = "5";
		break;
	case General::CardRank::CardRank_Six:
		result = "6";
		break;
	case General::CardRank::CardRank_Seven:
		result = "7";
		break;
	case General::CardRank::CardRank_Eight:
		result = "8";
		break;
	case General::CardRank::CardRank_Nine:
		result = "9";
		break;
	case General::CardRank::CardRank_Ten:
		result = "10";
		break;
	case General::CardRank::CardRank_Jack:
		result = "11";
		break;
	case General::CardRank::CardRank_Queen:
		result = "12";
		break;
	case General::CardRank::CardRank_King:
		result = "13";
		break;
	default:
		result = "번호에러";
		break;
	}
	return result;
}
