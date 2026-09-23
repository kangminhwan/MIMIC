#include "cRedisController.h"

#include <cpp_redis>

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Timer/cTimer.h"

#include "../Include/Netlib/Redis/cRedisManager.h"

#include "cDataLoader.h"
#include "TimeUtils.h"
#include <google/protobuf/util/json_util.h>

cRedisController::cRedisController()
{
}


cRedisController::~cRedisController()
{
}

eRedisChannelUse cRedisController::GetRedisBattleChannelUse()
{

	return eRedisChannelUse::eCacheServer;
}

//void cRedisController::RankTest(eRedisChannelUse channel)
//{
//	cpp_redis::client* pClient = NetLib::cSingleton<NetLib::cRedisManager>::GetInstance()->GetConnection(eRedisContentType::eCache, 0);
//	if (pClient == nullptr)
//	{
//		// add error code
//		return;
//	}
//
//	// Get Lock
//	NetLib::cCriticalSection* pCriticalSection = NetLib::cSingleton<NetLib::cRedisManager>::GetInstance()->GetLock(eRedisContentType::eCache);
//	if (pCriticalSection == nullptr)
//		return;
//
//	NetLib::cCSLock csLock(pCriticalSection);
//
//	// redis db select
//	pClient->select(static_cast<int>(channel));
//
//	pClient->flushdb();
//	pClient->sync_commit();
//
//	std::vector<std::string> playerList;
//	for (int i = 0; i < 1000; ++i)
//	{
//		char format_string[128];
//		sprintf_s(format_string, "hero_%d", i + 1);
//		playerList.push_back(format_string);
//	}
//
//	std::string key = NetLib::RedisKey::Player(1);
//
//	std::multimap<std::string, std::string> score_member;
//
//	std::vector<std::string> option;
//	for (int n = 0; n < playerList.size(); ++n)
//	{
//		std::string hero_name = playerList[n];
//
//		int point = (rand() % 1000) + 1;
//		char format_string[128];
//		sprintf_s(format_string, "%d", point);
//		/*option.clear();
//		option.push_back(format_string);*/
//
//		score_member.clear();
//		score_member.insert(std::pair<std::string, std::string>(format_string, hero_name));
//
//		pClient->zadd(key, option, score_member);
//	}
//
//	// zadd commit
//	pClient->sync_commit();
//
//	auto getZRange = pClient->zrange(key, 0, -1, true);
//	pClient->sync_commit();
//	cpp_redis::reply replyZRange = getZRange.get();
//
//	auto getZrevrange = pClient->zrevrange(key, 0, -1, true);
//	pClient->sync_commit();
//	cpp_redis::reply replyZrevrange = getZrevrange.get();
//
//}


void cRedisController::GetAuthData( const std::string& requestNo ,std::string& out )
{
	cpp_redis::client* pClient = NetLib::cSingleton<NetLib::cRedisManager>::GetInstance()->GetConnection( eRedisContentType::eCache , 0 );
	if ( pClient == nullptr ) {

		std::cerr << "Failed to get Redis connection." << std::endl;
		return;
	}

	// Redis DB select
	pClient->select( static_cast< int >( eRedisChannelUse::eAuth ) );

	std::string redis_key = requestNo;
	std::promise<void> get_promise;
	pClient->get( redis_key , [&out , &get_promise]( cpp_redis::reply& reply )
	{
		if ( reply.ok() )
		{
			if ( reply.is_string() )
			{
				out = reply.as_string(); // 인증 데이터 저장
				std::cout << "Received NICE auth data: " << out << std::endl;
			}
			else
			{
				std::cout << "Received OK NICE auth data No" << std::endl;
			}
		}
		else
		{
			std::cout << "Failed to get NICE auth data from Redis: " << reply.as_string() << std::endl;
		}

		get_promise.set_value();
	} );
	pClient->sync_commit();
	get_promise.get_future().wait();
}

void cRedisController::DeleteAuthData( const std::string& requestNo )
{
	cpp_redis::client* pClient = NetLib::cSingleton<NetLib::cRedisManager>::GetInstance()->GetConnection( eRedisContentType::eCache , 0 );
	if ( pClient == nullptr ) {

		std::cerr << "Failed to get Redis connection." << std::endl;
		return;
	}

	// Redis DB select
	pClient->select( static_cast< int >( eRedisChannelUse::eAuth ) );

	std::string redis_key = requestNo;
	std::promise<void> del_promise;
	bool isDeleted = false;
	pClient->del( { redis_key } , [&del_promise , &isDeleted]( cpp_redis::reply& reply )
	{
		if ( reply.ok() && reply.is_integer() && reply.as_integer() > 0 )
		{
			std::cout << "[GameServer] NICE 인증 데이터 삭제 완료: " << reply.as_integer() << " 개 삭제됨." << std::endl;
			isDeleted = true;
		}
		else
		{
			std::cout << "[GameServer] NICE 인증 데이터 삭제 실패! 해당 키가 없음." << std::endl;
		}

		del_promise.set_value();
	} );

	pClient->sync_commit();
	del_promise.get_future().wait();

}

