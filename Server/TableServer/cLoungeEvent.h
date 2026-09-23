#pragma once
#include "TableServerHeader.h"

#include "TimeUtils.h"

class cLoungeEvent
{
private:
	General::LoungeBenefitEvent m_lounge_event;
	std::time_t m_start_time;
	std::time_t m_end_time;

public:
	bool HasLoungeEvent()
	{
		std::time_t now = std::time( nullptr );
		if ( m_start_time <= now <= m_end_time )
			return true;

		return false;
	}

	void CopyLoungeEvent( General::LoungeBenefitEvent* lounge_event )
	{
		if ( lounge_event == nullptr ) return;

		if ( HasLoungeEvent() ) {

			lounge_event->CopyFrom( m_lounge_event );
		}
	}

	uint64 GetMaxHoldingCoin() {
		if ( HasLoungeEvent() ) return m_lounge_event.coin_balance_limit();
		return 0;
	}

	double GetDealerFee() {
		if ( HasLoungeEvent() ) return m_lounge_event.dealer_fee_discount_rate() / (double)100;
		return 1.0;
	}

	uint64 GetRakeBackRate() {
		if ( HasLoungeEvent() ) return m_lounge_event.rakeback_ratio();
		return 0;
	}

public:
	cLoungeEvent() {
		m_lounge_event.Clear();

		// 기본 값 셋팅
		/*m_lounge_event.set_coin_balance_limit( 150000000 );
		m_lounge_event.set_dealer_fee_discount_rate( 50 );
		m_lounge_event.set_rakeback_ratio( 65 );
		m_lounge_event.set_starts_at( "2024-06-05 00:00:00");
		m_lounge_event.set_ends_at( "2024-12-31 23:59:59" );*/

		SetLoungeEvent( 150000000 , 50 , 65 , "2024-01-01 00:00:00" , "2029-12-31 23:59:59" );
	}

	// 함수를 호출함으로써 이벤트 제어가 가능하다.
	void SetLoungeEvent(
		const uint64 coin_balance_limit ,
		int32 dealer_fee_discount_rate ,
		int32 rakeback_ratio ,
		std::string starts_at ,
		std::string ends_at ) {

		m_lounge_event.set_coin_balance_limit( coin_balance_limit );
		m_lounge_event.set_dealer_fee_discount_rate( dealer_fee_discount_rate );
		m_lounge_event.set_rakeback_ratio( rakeback_ratio );
		m_lounge_event.set_starts_at( starts_at );
		m_lounge_event.set_ends_at( ends_at );

		m_start_time = TimeUtils::StringToTimeTM( starts_at );
		m_end_time = TimeUtils::StringToTimeTM( ends_at );
	}

/*
message LoungeEvent
{
	uint64 coin_balance_limit = 1;						// 보유 코인 + 금고 코인
	int32 dealer_fee_discount_rate = 2;					// 딜러비 할인율 ( 50 인 경우 50% )
	int32 rakeback_ratio = 3;						// 적립통장 적립율 ( 65 인 경우 0.65 )
	string starts_at = 4;							// 시작 날짜 ( YYYY-MM-DD HH:mm:ss )
	string ends_at = 5;							// 종료 날짜 ( YYYY-MM-DD HH:mm:ss )
	int32 coin_recovery_count = 6;					// 보유코인 한도 초과시 초과 코인 복구 횟수
}
*/
};