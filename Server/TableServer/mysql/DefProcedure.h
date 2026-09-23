#pragma once

struct stMysqlConnectionInfo
{
	stMysqlConnectionInfo()
	{
		Clear();
	}

	~stMysqlConnectionInfo()
	{
		Clear();
	}

	void Clear()
	{
		db_idx = 0;
		db_group = 0;
		db_type.clear();
		db_name.clear();
		private_ip.clear();
		private_port = 0;
		auth_id.clear();
		auth_pw.clear();
	}

	int db_idx;
	int db_group;
	std::string db_type;
	std::string db_name;
	std::string private_ip;
	int private_port;
	std::string auth_id;
	std::string auth_pw;
};

enum E_DB_TYPE
{
	E_DB_TYPE_NONE,
	E_DB_TYPE_ACCOUNT,
	E_DB_TYPE_SHARD,
	E_DB_TYPE_LOG,
};

/*
	MySQL 프로지져들의 리턴 field의 정의 입니다.
	cMySQLParserElement.h 에 정의된 vector<cMySQLParserBase*> 의 어레이 값에 대응되는 항목입니다.
	불편하시면 바꾸셔도 됩니다.
	프로시져와 받을 데이터 카운트가 정확한지에 대한 검증 부분입니다.
*/
enum E_FIELD_BattleInfo_DB
{
	battle_type,
	battle_ticket,
	battle_ticket_initial_use_time,
	rank_point,
	rank_template_id,
	rank_grade,
	rank_tier,
	league_play_count,
	league_win_count,
	league_lose_count,
	daily_resign_count,
	daily_play_count,
	cur_winning_streak_count,
	received_count_in_daily_reward,
	league_get_it_reward,
	daily_resign_penalty_end_utc_time_sec,
	daily_reward_grade,
	POP_UP,
	daily_reset_start_utc_time_sec,
	league_reward_template_id,
	purchase_ticket_count,
	purchase_ticket_count_reset_start_utc_time_sec,
	E_BattleInfo_DB_count, // E_BattleInfo_DB 의 리턴 카운트 숫자 입니다.
};

enum E_FIELD_DB_CONFIG
{
	db_idx, db_group, db_type, db_name, private_ip, private_port, public_ip, public_port, auth_id, auth_pw, status,
	E_FIELD_DB_CONFIG_count, // E_BattleInfo_DB 의 리턴 카운트 숫자 입니다.
};