#pragma once
#include "TableServerHeader.h"

#include "..\..\ProtocolBuffer\cpp\Server.pb.h"

#include <algorithm>
#include <random> // std::random_device, std::mt19937

class stRate
{
public:
	uint64 moneyValue;
	int startNum;
	int endNum;
	int Count;

	uint64 GetMoneyValue() { ++Count; return moneyValue; }

	stRate() { Count = 0; }
};

class cDecider
{

protected:
	std::vector<stRate> m_rate_list;
	int m_totalRateCount;
	std::uniform_int_distribution<> m_dis;
	std::mt19937 gen;

public:
	void Push( const Server::LotteryData& lotteryData ) {

		stRate _rate;
		_rate.moneyValue = lotteryData.money_value();
		_rate.startNum = m_totalRateCount + 1;
		m_totalRateCount += lotteryData.rate();
		_rate.endNum = m_totalRateCount;

		m_rate_list.push_back( _rate );
	}

	// m_totalRateCount 에 대한 랜덤으로 수정한다.
	uint64 DecideValue() {

		m_dis = std::uniform_int_distribution<>( 1 , m_totalRateCount );
		auto rateCount = m_dis( gen );

		for ( auto& _rate : m_rate_list ) {
			if ( _rate.startNum <= rateCount && rateCount <= _rate.endNum )
				return _rate.moneyValue;
		}
		return 0;
	}

	cDecider()
	{
		m_totalRateCount = 0;
		gen.seed( std::random_device{}( ) );
	}
};

class cFreeCharger
{
protected:
	cDecider m_coinDecider;
	cDecider m_chipDecider;

public:
	void PushRate( const Server::LotteryData& lotteryData ) {

		if ( lotteryData.money_type() == General::AssetKind::AssetKind_Coin )
			m_coinDecider.Push( lotteryData );
		else if ( lotteryData.money_type() == General::AssetKind::AssetKind_Chip )
			m_chipDecider.Push( lotteryData );
	}

	uint64 GetFreeMoney( const General::AssetKind& money_type ) {

		if ( money_type == General::AssetKind::AssetKind_Coin )
			return m_coinDecider.DecideValue();
		else if ( money_type == General::AssetKind::AssetKind_Chip )
			return m_chipDecider.DecideValue();
	}
};

