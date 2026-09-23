#pragma once
#include "TableServerHeader.h"
#include "TimeUtils.h"

#include <iostream>
#include <map>

// 손실한도 제한 캐쉬
// 어카운트 마다 1개씩만 생성가능
// 캐쉬 데이터는 삭제 하지 않는다.
// 게임서버가 여러대 인경우 동기화 패킷 개발 필요
class cLostLimitManager
{
protected:
	std::map<uint64 , General::LossLimitProfile*> lostLimitMap;

public:

	General::LossLimitProfile* GetLostLimit( const uint64& account_idx )
	{
		auto iter = lostLimitMap.find( account_idx );
		if ( iter != lostLimitMap.end() )
			return iter->second;
		 
		return nullptr;
	}

	General::LossLimitProfile* SetLostLimit( General::LossLimitProfile& lostLimit )
	{
		auto iter = lostLimitMap.find( lostLimit.account_id() );
		if ( iter != lostLimitMap.end() )
			return nullptr;

		const uint64& creationTime = TimeUtils::GetCurrentTimeInMilliseconds();
		lostLimit.set_created_at( creationTime );

		General::LossLimitProfile* lostLimitOrigin = new General::LossLimitProfile();
		lostLimitMap.insert( std::pair< uint64 , General::LossLimitProfile* >( lostLimit.account_id() , lostLimitOrigin ) );

		lostLimitOrigin->CopyFrom( lostLimit );

		return lostLimitOrigin;
	}

	void SyncLostLimit( const General::LossLimitProfile& lostLimit )
	{
		auto iter = lostLimitMap.find( lostLimit.account_id() );
		if ( iter != lostLimitMap.end() )
		{
			iter->second->CopyFrom( lostLimit );

		}
		else
		{
			General::LossLimitProfile* lostLimitOrigin = new General::LossLimitProfile();
			lostLimitMap.insert( std::pair< uint64 , General::LossLimitProfile* >( lostLimit.account_id() , lostLimitOrigin ) );

			lostLimitOrigin->CopyFrom( lostLimit );
		}
	}

	std::map<uint64 , General::LossLimitProfile*>* GetServerLostLimits() { return &lostLimitMap; }
};

