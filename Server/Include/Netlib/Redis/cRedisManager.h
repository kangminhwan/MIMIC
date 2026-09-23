#pragma once
#include "../Common/Netlib.h"

#include <vector>
#include <string>
#include <cpp_redis>

#include <locale>
#include <codecvt>

#ifdef _DEBUG
#pragma comment(lib, "cpp_redisd.lib")
#pragma comment(lib, "tacopied.lib")
#else
#pragma comment(lib, "cpp_redis.lib")
#pragma comment(lib, "tacopie.lib")
#endif


enum class eRedisChannelUse
{
	eDefault = 0, // cpp_redis 는 디폴트 채널이 0입니다. StackExchange 는 -1 입니다. 이부분 주의 바랍니다.
	eCacheServer = 1,
	eAuth = 2,
	ePinball = 3,
	eSlotCredits = 3,
};

// eRedisContentType은 array로 처리 합니다. 이빨 안빠지게 해주세요.
enum class eRedisContentType
{
	eCache = 1,
	eSession = 2,
	eRank = 3,
};

BEGIN_NETLIB

class RedisKey
{
public:

	static std::string Player(uint64 playerIdx , int _slot_game_type)
	{
		char format_string[128];
		sprintf_s(format_string, "slotgametype:%d_player:%I64d", _slot_game_type , playerIdx );

		return format_string;
	}

	static std::string PlayerPinball(uint64 playerIdx,int _pinball_type)
	{
		char format_string[128];
		sprintf_s(format_string, "pinballtype:%d_player:%I64d", _pinball_type, playerIdx);

		return format_string;
	}

	static std::string PlayerSelectSpin(uint64 playerIdx, int _slot_game_type)
	{
		char format_string[128];
		sprintf_s(format_string, "selectspin_slotgametype:%d_player:%I64d", _slot_game_type, playerIdx);

		return format_string;
	}

	static std::string PlayerPickGame(uint64 playerIdx, int _slot_game_type)
	{
		char format_string[128];
		sprintf_s(format_string, "pickgame:slotgametype-%d:player-%I64d", _slot_game_type, playerIdx);

		return format_string;
	}

	static std::string SlotDailyBestRanking(int slotGameType, std::string state, std::string day)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_daily_best_rank:%s:%d_%s", day.c_str(), slotGameType, state.c_str());

		return format_string;
	}

	static std::string SlotDailyBestRankingGoat(int slotGameType)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_daily_best_rank:goat:%d", slotGameType);

		return format_string;
	}

	static std::string SlotWeeklyBestRanking(int slotGameType, std::string state, std::string day)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_weekly_best_rank:%s:%d_%s", day.c_str(), slotGameType, state.c_str());

		return format_string;
	}

	static std::string SlotWeeklyBestRankingGoat(int slotGameType)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_weekly_best_rank:goat:%d", slotGameType);

		return format_string;
	}

	static std::string SlotWeeklyWinRanking(int slotGameType, std::string state, std::string day)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_weekly_win_rank:%s:%d_%s", day.c_str() , slotGameType, state.c_str());

		return format_string;
	}

	static std::string SlotWeeklyWinRankingGoat(int slotGameType)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_weekly_win_rank:goat:%d", slotGameType);

		return format_string;
	}

	static std::string SlotElectronicDisplayBorad()
	{
		return "slot_electronic_display_board";
	}

	static std::string SlotCredits(uint64 playerIdx, int slot_game_type, uint64 bet_money)
	{
		char format_string[128];
		sprintf_s(format_string, "slot_credits:slot_game_type-%d:player-%I64d:bet_money-%I64d", slot_game_type, playerIdx, bet_money);

		return format_string;
	}

#pragma region SERVER

	static std::string AccountServer(__int64 account_idx)
	{
		char format_string[128];
		sprintf_s(format_string, "account_server:%I64d", account_idx);

		return format_string;
	}

	static std::string ServerUserCount(uint64 server_key)
	{
		char format_string[128];
		sprintf_s(format_string, "server_user_count:%I64u", server_key);

		return format_string;
	}

	static std::string GetUserCountString(int user_count)
	{
		char format_string[128];
		_itoa_s(user_count, format_string, sizeof(format_string), 10);
		
		return format_string;
	}

#pragma endregion
};

/*
	eRedisContentType 에 따른 커넥션 분리는 아직 하고 있지 않음
*/
class cRedisManager
{
//private:
//	friend class cRedisController;

public:
	cRedisManager();
	~cRedisManager();

	int m_expireSecond;// = 300;// default expire time을 5분으로 설정한다.

	//Server::ServiceStatusCode Parsing(google::protobuf::Message& message, cpp_redis::reply& reply);

protected:
	//std::vector<cpp_redis::client*> m_redis_clients;
	ATL::CAtlMap<int, std::vector<cpp_redis::client*>*> m_redis_content_map;
	std::vector<NetLib::cCriticalSection*> m_redis_content_locks;

private:
	void Init();
	void Destroy();

	void RedisConnectCallBack(const std::string& host, std::size_t port, cpp_redis::client::connect_state status);

public:
	cpp_redis::client* GetConnection(eRedisContentType contentType, const int threadArray);
	NetLib::cCriticalSection* GetLock(eRedisContentType contentType);
	int GetSessionExpireSec() { return m_expireSecond; }
};

END_NETLIB