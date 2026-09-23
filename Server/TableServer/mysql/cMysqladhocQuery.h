#pragma once
#include "../TableServerHeader.h"

namespace adhocquery
{
	//	결과 처리 하기 위한 데이터
	struct BattleInfo
	{
		__int64 account_idx;							//	계정 인덱스
		__int64 hero_idx;								//	영웅 인덱스
		double daily_resign_penalty_end_utc_time_sec;	//	탈주 패널티 시간 끝나는 UTC 기준 시간
		double battle_ticket_initial_use_time;			//	배틀 티켓 초기 사용시간
		int cur_winning_streak_count;					//	현재 연승 카운트
		int rank_point;									//	랭크 포인트
		int rank_template_id;							//	랭크 템플릿 아이디
		int daily_resign_count;							//	일일 탈주 카운트
		int battle_ticket;								//	배틀 티켓 수
		int battle_type;								//	배틀 타입

		bool is_battle_info_db;							//	true 면 DB에서 가져온겁니다.

		BattleInfo()
		{
			memset(this, 0x00, sizeof(BattleInfo));
		}
	};
};

struct adhoc_query_resource
{
	NetLib::cVector<adhocquery::BattleInfo*> vec_battle_info;
	NetLib::cVector<class cMySQLReader*> vec_mysql_reader;
};

struct adhoc_query_resource_helper
{
private:
	adhoc_query_resource* _pResource;

public:
	adhoc_query_resource* get_resource() const { return _pResource; }

public:
	adhoc_query_resource_helper(adhoc_query_resource* pResource);
	~adhoc_query_resource_helper();
};

class cMysqladhocQuery
{
private:
	//	web thread count
	size_t m_st_array_count;

	//	m_array_szAdhocQuery 를 할당 할때 사용합니다.
	size_t m_st_alloc_count;

	//	web thread count 만큼 할당 합니다.
	char** m_array_szAdhocQuery;

	//	web thread count 만큼 할당 합니다.
	ATL::CAtlMap<int, adhoc_query_resource>* m_array_battleinfo;

private:
	void array_battleinfo_clear(_In_ const UINT uithreadindex);
	void array_adhoc_query_clear(_In_ const UINT uithreadindex);

public:
	void Init(_In_ const UINT uithreadcount, _In_opt_ const size_t st_alloc_count = 1024);

public:
	cMysqladhocQuery();
	~cMysqladhocQuery();
};

