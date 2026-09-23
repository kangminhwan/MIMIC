#include "cMysqladhocQuery.h"
#include <mysql.h>

#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"

#include "cMySQL.h"
#include "cMySQLReader.h"
#include "cMySQLParserElement.h"
#include "../MySQL/cMySQLConnectionPooler.h"
#include "../MySQL/DefProcedure.h"

#define batle_infos_query_select_string	"select account_idx, hero_idx, "\
										"daily_resign_penalty_end_utc_time_sec, battle_ticket_initial_use_time, cur_winning_streak_count, "\
										"rank_point, rank_template_id, daily_resign_count, battle_ticket "\
										"from hero_battle "\
										"use index(PRIMARY) "\
										"where "

#define battle_infos_query_where_string "account_idx=%I64d and hero_idx=%I64d and battle_type=%d"

adhoc_query_resource_helper::adhoc_query_resource_helper(adhoc_query_resource* pResource)
{
	_pResource = pResource;
}

adhoc_query_resource_helper::~adhoc_query_resource_helper()
{
	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader(_pResource->vec_mysql_reader);

	_pResource->vec_battle_info.clear();
	_pResource->vec_mysql_reader.clear();
}

cMysqladhocQuery::cMysqladhocQuery() :
	m_array_szAdhocQuery(nullptr),
	m_array_battleinfo(nullptr),
	m_st_array_count(0),
	m_st_alloc_count(0)
{

}

cMysqladhocQuery::~cMysqladhocQuery()
{
	if (m_array_szAdhocQuery != nullptr)
	{
		for (int n = 0; n < m_st_array_count; ++n)
		{
			delete[] m_array_szAdhocQuery[n];
		}

		delete[] m_array_szAdhocQuery;
	}

	if (m_array_battleinfo != nullptr)
	{
		delete[] m_array_battleinfo;
	}
}

void cMysqladhocQuery::Init(_In_ const UINT uithreadcount, _In_opt_ const size_t st_alloc_count)
{
	m_st_array_count = uithreadcount;

	m_st_alloc_count = st_alloc_count;

	m_array_battleinfo = new ATL::CAtlMap<int, adhoc_query_resource>[m_st_array_count];

	m_array_szAdhocQuery = new char*[m_st_array_count];

	for (size_t n = 0; n < m_st_array_count; ++n)
	{
		m_array_szAdhocQuery[n] = new char[m_st_alloc_count];
		array_adhoc_query_clear(static_cast<UINT>(n));
	}
}

void cMysqladhocQuery::array_battleinfo_clear(_In_ const UINT uithreadindex)
{
	if (uithreadindex >= m_st_array_count)
	{
		return;
	}

	POSITION pos = m_array_battleinfo[uithreadindex].GetStartPosition();
	while (pos)
	{
		adhoc_query_resource& _resource = m_array_battleinfo[uithreadindex].GetValueAt(pos);
		_resource.vec_battle_info.clear();

		m_array_battleinfo[uithreadindex].GetNext(pos);
	}
}

void cMysqladhocQuery::array_adhoc_query_clear(_In_ const UINT uithreadindex)
{
	if (uithreadindex >= m_st_array_count)
	{
		return;
	}

	memset(m_array_szAdhocQuery[uithreadindex], 0x00, m_st_alloc_count);
}