#include "../../Include/Netlib/Redis/cRedisManager.h"

#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Timer/cTimer.h"

NetLib::cRedisManager::cRedisManager()
{
	Init();

	// Redis Test Code
#ifdef _DEBUG
	/*__int64 account_idx = 3298;
	__int64 hero_idx = 6083;
	int threadArray = 0;
	GetAccountInfo(eRedisChannelUse::eAccountServer, threadArray, account_idx); // 현재는 같은 Redis를 쓰고 있어서 돌아 갑니다.
	GetHero(eRedisChannelUse::eDefault, threadArray, account_idx, hero_idx);  // 현재는 같은 Redis를 쓰고 있어서 돌아 갑니다. */
	//RankTest(eRedisChannelUse::eTestPvP);
#endif
}

NetLib::cRedisManager::~cRedisManager()
{
	Destroy();
}

void NetLib::cRedisManager::Init()
{
	m_expireSecond = 300;// default expire time을 5분으로 설정한다.

	TServerConfiguration* pTServerConfiguration = NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->GetConfiguration();
	if (pTServerConfiguration == nullptr)
		throw "cRedisManager::Init() Failed, ServerConfiguration is nullptr";

	if(_tcslen(pTServerConfiguration->szRedisDns) <= 0)
		throw "cRedisManager::Init() Failed, ServerConfiguration szRedisDns is nullptr";

	if (pTServerConfiguration->nRedisPort <= 0)
		throw "cRedisManager::Init() Failed, ServerConfiguration nRedisPort Check Please";

	cpp_redis::client::connect_callback_t _redis_connect_callback = std::bind(&NetLib::cRedisManager::RedisConnectCallBack, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	int redis_contents_count = static_cast<int>(eRedisContentType::eRank);
	for (int n = 0; n < redis_contents_count; ++n)
	{
		std::vector<cpp_redis::client*>* pRedis_clients = new std::vector<cpp_redis::client*>();
		
		TCHAR* pRedisDns = nullptr;
		UINT uPort = 0;

		switch (static_cast<eRedisContentType>(n+1))
		{
		case eRedisContentType::eCache:
			{
				pRedisDns = pTServerConfiguration->szRedisDns;
				uPort = pTServerConfiguration->nRedisPort;
			}
			break;
		default:
			continue;
	/*	case eRedisContentType::eSession:
			{
				pRedisDns = pTServerConfiguration->szSessionRedisDns;
				uPort = pTServerConfiguration->nSessionRedisPort;
			}
			break;
		case eRedisContentType::eRank:
			{
				pRedisDns = pTServerConfiguration->szRankRedisDns;
				uPort = pTServerConfiguration->nRedisPort;
			}
			break;*/
		}

		// CreateRedis Connections
		/*for (int n = 0; n < pTServerConfiguration->nWebThreadCnt; ++n)
		{
			cpp_redis::client* pClient = new cpp_redis::client();

			pClient->connect(ATL::CW2A(pRedisDns).m_psz, uPort, _redis_connect_callback, 0, -1, 5000);

			pRedis_clients->push_back(pClient);
		}*/

		// WebThread별로 커넥션 맺던 부분을 변경합니다.
		// 한놈만 Lock을 걸고 사용하도록 변경합니다.
		cpp_redis::client* pClient = new cpp_redis::client();
		pClient->connect(ATL::CW2A(pRedisDns).m_psz, uPort, _redis_connect_callback, 0, -1, 5000);
		pRedis_clients->push_back(pClient);

		m_redis_content_map.SetAt(n + 1, pRedis_clients);
		m_redis_content_locks.push_back(new NetLib::cCriticalSection());
	}
}

void NetLib::cRedisManager::RedisConnectCallBack(const std::string& host, std::size_t port, cpp_redis::client::connect_state status)
{
	switch (status)
	{
	case cpp_redis::client::connect_state::dropped:
	{
		std::cout << "client disconnected from " << host << ":" << port << std::endl;
	}
		break;
	case cpp_redis::client::connect_state::start:
		break;
	case cpp_redis::client::connect_state::sleeping:
		break;
	case cpp_redis::client::connect_state::ok:
		break;
	case cpp_redis::client::connect_state::failed:
		break;
	case cpp_redis::client::connect_state::lookup_failed:
		break;
	case cpp_redis::client::connect_state::stopped:
		break;
	default:
		break;
	}
}

void NetLib::cRedisManager::Destroy()
{
	POSITION oldPos = nullptr;
	POSITION pos = m_redis_content_map.GetStartPosition();
	while (pos != nullptr)
	{
		oldPos = pos;

		m_redis_content_map.GetNext(pos);

		ATL::CAtlMap<int, std::vector<cpp_redis::client*>*>::CPair* pPair = m_redis_content_map.GetAt(oldPos);
		if (pPair == nullptr || pPair->m_value == nullptr)
			continue;

		std::vector<cpp_redis::client*>* pRedis_clients = pPair->m_value;

		std::vector<cpp_redis::client*>::const_iterator con_iter = pRedis_clients->begin();
		std::vector<cpp_redis::client*>::const_iterator con_iter_end = pRedis_clients->end();
		for (; con_iter != con_iter_end; ++con_iter)
		{
			if ((*con_iter) != nullptr)
			{
				(*con_iter)->disconnect(true);
				delete (*con_iter);
			}
		}

		pRedis_clients->clear();

		delete pRedis_clients;
	}

	m_redis_content_map.RemoveAll();

	// Release CriticalSection
	std::vector<NetLib::cCriticalSection*>::iterator iterS = m_redis_content_locks.begin();
	for (; iterS != m_redis_content_locks.end(); ++iterS)
	{
		NetLib::cCriticalSection* pCriticalSection = *iterS;
		if (pCriticalSection == nullptr)
			continue;

		delete pCriticalSection;
		pCriticalSection = nullptr;
	}
}

cpp_redis::client* NetLib::cRedisManager::GetConnection(eRedisContentType contentType, const int threadArray)
{
	ATL::CAtlMap<int, std::vector<cpp_redis::client*>*>::CPair* pPair = m_redis_content_map.Lookup(static_cast<int>(contentType));
	if (pPair == nullptr || pPair->m_value == nullptr)
		return nullptr;

	/*std::vector<cpp_redis::client*>* pRedis_clients = pPair->m_value;

	if (threadArray < 0 || threadArray > pRedis_clients->size())
		return nullptr;

	cpp_redis::client* pClient = pRedis_clients->at(threadArray);
	if (pClient == nullptr)
		return nullptr;*/

	std::vector<cpp_redis::client*>* pRedis_clients = pPair->m_value;

	if (pRedis_clients->size() < 1)
		throw("NetLib::cRedisManager::GetConnection pRedis_clients->size() is smaller than 1, ask awesomepig server team");

	// if size() lager than 1, occur throw
	if(pRedis_clients->size() > 1)
		throw("NetLib::cRedisManager::GetConnection pRedis_clients->size() is lager than 1, ask awesomepig server team");

	// return first cpp_redis::client instance
	cpp_redis::client* pClient = pRedis_clients->at(0);
	if (pClient == nullptr)
		return nullptr;

	return pClient;
}

NetLib::cCriticalSection* NetLib::cRedisManager::GetLock(eRedisContentType contentType)
{
	// access criticalsection directly
	NetLib::cCriticalSection* pCriticalSection = m_redis_content_locks[static_cast<int>(contentType) - 1];

	return pCriticalSection == nullptr ? nullptr : pCriticalSection;
}