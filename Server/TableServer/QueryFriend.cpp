#include "Query.h"

#include "./mysql/cMySQL.h"
#include "./mysql/cMySQLParserElement.h"
#include "./mysql/cMySQLReader.h"

#include "./mysql/cMySQLConnectionPooler.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Manager/ServerManager.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cProtoUtil.h"
#include "TimeUtils.h"
#include "StringUtil.h"

#include <iostream>
#include <format>

BOOL QueryManager::GetPlatformGuidByPlayerIdx( const uint64& friend_player_idx , std::string& platform_guid )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	/*MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;*/

	std::string executeQuery = std::format( "select platform_guid from players where player_idx = '{}';" , friend_player_idx );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			platform_guid = reader->GetString( "platform_guid" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
	}

	/*if ( stmt != nullptr )
		mysql_stmt_close( stmt );*/

	return TRUE;
}

std::future<BOOL> QueryManager::GetPlatformGuidByPlayerIdxAsync( const uint64& friend_player_idx , std::string& platform_guid )
{
	return std::async( std::launch::async , [&friend_player_idx , &platform_guid]() {
		return QueryManager::GetPlatformGuidByPlayerIdx( friend_player_idx , platform_guid );
	} );
}

BOOL QueryManager::GetFriends( const uint64& player_idx , std::vector<uint64>& friend_player_idx_list )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	/*MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;*/

	std::string executeQuery = std::format( "select * from friends where player_idx = {};" , player_idx );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			uint64 friend_player_idx = reader->GetLongLong( "friend_player_idx" );

			std::string friend_platform_guid = reader->GetString( "friend_platform_guid" );

			int db_index = reader->GetLong( "db_index" );

			// 갱신 시간 읽어오기
			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );

			friend_player_idx_list.push_back( friend_player_idx );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
	}

	/*if ( stmt != nullptr )
		mysql_stmt_close( stmt );*/

	return TRUE;
}

std::future<BOOL> QueryManager::GetFriendsAsync( const uint64& player_idx , std::vector<uint64>& friend_player_idx_list )
{
	return std::async( std::launch::async , [&player_idx , &friend_player_idx_list]() {
		return QueryManager::GetFriends( player_idx , friend_player_idx_list );
	} );
}

std::string QueryManager::GenerateInsertFriendQuery( uint64_t playerIdx , const std::string& friendPlatformGuid , uint64 friendPlayerIdx , int dbIndex ) {
	std::string query = "INSERT INTO friends (player_idx, friend_platform_guid, friend_player_idx, db_index) VALUES ("
		"" + std::to_string( playerIdx ) + ", "
		"'" + friendPlatformGuid + "', "
		"" + std::to_string( friendPlayerIdx ) + ", "
		"" + std::to_string( dbIndex ) + ");";
	return query;
}

std::future<BOOL> QueryManager::InsertFriendAsync( uint64_t playerIdx , const std::string& friendPlatformGuid , uint64 friendPlayerIdx , int dbIndex )
{
	std::string query = GenerateInsertFriendQuery( playerIdx , friendPlatformGuid , friendPlayerIdx , dbIndex );
	return QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
}

std::string QueryManager::GenerateUpdateFriendQuery( uint64_t playerIdx , const std::string& friendPlatformGuid , uint64 friendPlayerIdx , int dbIndex ) {
	std::string query = "UPDATE friends SET "
		"friend_player_idx = '" + std::to_string( friendPlayerIdx ) + "', "
		"db_index = '" + std::to_string( dbIndex ) + "' "
		"WHERE player_idx = '" + std::to_string( playerIdx ) + "' "
		"AND friend_platform_guid = '" + friendPlatformGuid + "';";

	return query;
}

std::string QueryManager::GenerateDeleteFriendQuery( uint64 playerIdx , uint64 friendPlayerIdx ) {
	std::string query = "DELETE FROM friends WHERE player_idx = '"
		+ std::to_string( playerIdx ) + "' "
		"AND friend_player_idx = '" + std::to_string( friendPlayerIdx ) + "';";
	return query;
}

std::future<BOOL> QueryManager::DeleteFriendsAsync( const uint64 player_idx , uint64 friendPlayerIdx )
{
	std::string query = GenerateDeleteFriendQuery( player_idx , friendPlayerIdx );
	return QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
}

BOOL QueryManager::GetPlayerByPlayerIdx( const uint64& player_idx , General::ParticipantProfile& _player , Server::ParticipantProfileInternal& _playerExt )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	std::string query = "CALL PlayerGetByPlayerIdx(?)";

	// 파람바인더에 파라미터를 추가해준다.
	cParamBinder parambinder( 1 );
	parambinder.BindParam( 1 , MYSQL_TYPE_LONGLONG , player_idx );

	NetLib::cVector<cMySQLReader*> result_set;

	BOOL bResult = cMySQL::ExcuteProcedure( MySQLConnection , parambinder , query , result_set );
	if ( bResult == FALSE )
		return FALSE;

	if ( result_set.size() == 0 )
		return FALSE;

	// 데이터 읽기
	NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
	NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
	for ( ; iter != iter_end; ++iter )
	{
		cMySQLReader* reader = ( *iter );
		if ( reader == nullptr )
			continue;

		_player.set_member_level( reader->GetLong( "level" ) );
		_player.set_experience_points( reader->GetLongLong( "exp" ) );
		//_player.set_loss_limit(reader->GetLongLong("lost_limit"));
		//_player.set_refresh_time_of_loss_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_loss_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		//_player.set_buy_limit(reader->GetLongLong("buy_limit"));
		//_player.set_refresh_time_of_buy_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_buy_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		_player.set_wallet_chips( reader->GetLongLong( "chips" ) );
		_player.set_wallet_coins( reader->GetLongLong( "coin" ) );
		_playerExt.set_remaining_reel_coin( reader->GetLongLong( "remain_slot_coin" ) );
		_player.set_wallet_gems( reader->GetLongLong( "gem" ) );
		//uint64 test = reader->GetLongLong( "gem" );
		_player.set_kick_ticket_balance( reader->GetLong( "kick_ticket_count" ) );
		_player.set_equipped_avatar_id( reader->GetLong( "avatar_id" ) );
		_player.set_membership_enabled( ( bool ) reader->GetTinyInt( "membership_activated" ) );
		_player.set_membership_expires_at( TimeUtils::MYSQLTimeToString( reader->GetDateTime( "membership_expiry_time" ) ) );// DateTime 형식을 읽을때 문제가 발생됨
		_player.set_chip_refill_uses( reader->GetLong( "chips_refill_count" ) );
		_player.set_coin_refill_uses( reader->GetLong( "coin_refill_count" ) );
		//_player.set_sub_passwd_activated((bool)reader->GetTinyInt("sub_passwd_activated"));
		//_player.set_sub_passwd(reader->GetString("sub_passwd"));
		_player.set_member_id( reader->GetLongLong( "player_idx" ) );
		_player.set_display_name( reader->GetString( "nickname" ) );
		_player.set_vault_chips( reader->GetLongLong( "safe_chips" ) );
		//_player.set_safe_paid_chips( reader->GetLongLong( "safe_paid_chips" ) );
		_player.set_vault_coins( reader->GetLongLong( "safe_coin" ) );
		_playerExt.set_blackjack_win_streak( reader->GetLong( "blackjack_straight_wins" ) );
		_player.set_rakeback_balance( reader->GetLongLong( "rakeback" ) );

		//_player.set_paid_chips( reader->GetLongLong( "paid_chips" ) );
		//_player.set_paid_coin( reader->GetLongLong( "paid_coin" ) );
		_player.set_paid_gems( reader->GetLongLong( "paid_gem" ) );

		// 멤버쉽 클라스 읽어오기.
		int member_ship_number = reader->GetLong( "member_ship_class" );
		//string membership_string = reader->GetString("member_ship_class_string");
		const General::BenefitTier member_ship_class = static_cast< General::BenefitTier >( member_ship_number );
		_player.set_membership_tier( member_ship_class );

		// 갱신 시간 읽어오기
		MYSQL_TIME daily = reader->GetDateTime( "daily_refresh_time" );
		//std::tm daily_time = TimeUtils::MysqlTimeToTM( daily );
		string daily_string = TimeUtils::MYSQLTimeToString( daily );
		_player.set_daily_reset_at( daily_string );

		MYSQL_TIME weekly = reader->GetDateTime( "weekly_refresh_time" );
		//std::tm weekly_time = TimeUtils::MysqlTimeToTM( weekly );
		string weekly_string = TimeUtils::MYSQLTimeToString( weekly );
		_playerExt.set_weekly_reset_at( weekly_string );

		MYSQL_TIME monthly = reader->GetDateTime( "monthly_refresh_time" );
		//std::tm monthly_time = TimeUtils::MysqlTimeToTM( monthly );
		string monthly_string = TimeUtils::MYSQLTimeToString( monthly );
		_playerExt.set_monthly_reset_at( monthly_string );

		MYSQL_TIME free_coin = reader->GetDateTime( "refresh_time_coin_free_charge" );
		//std::tm free_coin_time = TimeUtils::MysqlTimeToTM( free_coin );
		string free_coin_string = TimeUtils::MYSQLTimeToString( free_coin );
		_player.set_coin_free_charge_reset_at( free_coin_string );

		MYSQL_TIME free_chip = reader->GetDateTime( "refresh_time_chip_free_charge" );
		//std::tm free_chip_time = TimeUtils::MysqlTimeToTM( free_chip );
		string free_chip_string = TimeUtils::MYSQLTimeToString( free_chip );
		_player.set_chip_free_charge_reset_at( free_chip_string );

		_player.set_nickname_changed_once( ( bool ) reader->GetTinyInt( "is_changed_nickname" ) );

		MYSQL_TIME nick_expiry = reader->GetDateTime( "nickname_change_prohibite_expiry_time" );
		string nick_expiry_string = TimeUtils::MYSQLTimeToString( nick_expiry );
		_player.set_nickname_change_locked_until( nick_expiry_string );

		int32 attendance_days = reader->GetLong( "attendance_days" );
		_player.set_attendance_streak_days( attendance_days );
		//std::string test = reader->GetString( "nickname" );
		//test = reader->GetString( "nickname" );

		_playerExt.set_lowbadugi_daily_chips( reader->GetLongLong( "lowbaduki_today_chip" ) );
		_playerExt.set_lowbadugi_daily_coins( reader->GetLongLong( "lowbaduki_today_coin" ) );
		_playerExt.set_holdem_daily_chips( reader->GetLongLong( "holdem_today_chip" ) );
		_playerExt.set_holdem_daily_coins( reader->GetLongLong( "holdem_today_coin" ) );
	}

	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	mysql_stmt_close( stmt );

	return TRUE;
}

std::future<BOOL> QueryManager::GetPlayerByPlayerIdxAsync( const uint64& player_idx , General::ParticipantProfile& _player , Server::ParticipantProfileInternal& _playerExt )
{
	return std::async( std::launch::async , [&player_idx , &_player , &_playerExt]() {
		return QueryManager::GetPlayerByPlayerIdx( player_idx , _player , _playerExt );
	} );
}

BOOL QueryManager::PlayerGetByNickname( const std::string& nickname , std::vector<General::ParticipantProfile>& players )
{
	// 입력 검증 추가 (SQL 인젝션 방지)
	if ( !StringUtil::isValidNickname( nickname ) ) {
		return FALSE;
	}
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	std::string query = "SELECT * FROM players WHERE nickname = '" + nickname + "';";

	NetLib::cVector<cMySQLReader*> result_set;

	BOOL bResult = cMySQL::ExcuteQuery( MySQLConnection , query , result_set );
	if ( bResult == FALSE )
		return FALSE;

	if ( result_set.size() == 0 )
		return FALSE;

	General::ParticipantProfile _player;

	// 데이터 읽기
	NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
	NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
	for ( ; iter != iter_end; ++iter )
	{
		cMySQLReader* reader = ( *iter );
		if ( reader == nullptr )
			continue;

		_player.set_member_level( reader->GetLong( "level" ) );
		_player.set_experience_points( reader->GetLongLong( "exp" ) );
		//_player.set_loss_limit(reader->GetLongLong("lost_limit"));
		//_player.set_refresh_time_of_loss_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_loss_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		//_player.set_buy_limit(reader->GetLongLong("buy_limit"));
		//_player.set_refresh_time_of_buy_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_buy_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		_player.set_wallet_chips( reader->GetLongLong( "chips" ) );
		_player.set_wallet_coins( reader->GetLongLong( "coin" ) );
		_player.set_wallet_gems( reader->GetLongLong( "gem" ) );
		//uint64 test = reader->GetLongLong( "gem" );
		_player.set_kick_ticket_balance( reader->GetLong( "kick_ticket_count" ) );
		_player.set_equipped_avatar_id( reader->GetLong( "avatar_id" ) );
		_player.set_membership_enabled( ( bool ) reader->GetTinyInt( "membership_activated" ) );
		_player.set_membership_expires_at( TimeUtils::MYSQLTimeToString( reader->GetDateTime( "membership_expiry_time" ) ) );// DateTime 형식을 읽을때 문제가 발생됨
		_player.set_chip_refill_uses( reader->GetLong( "chips_refill_count" ) );
		_player.set_coin_refill_uses( reader->GetLong( "coin_refill_count" ) );
		//_player.set_sub_passwd_activated((bool)reader->GetTinyInt("sub_passwd_activated"));
		//_player.set_sub_passwd(reader->GetString("sub_passwd"));
		_player.set_member_id( reader->GetLongLong( "player_idx" ) );
		_player.set_display_name( reader->GetString( "nickname" ) );
		_player.set_vault_chips( reader->GetLongLong( "safe_chips" ) );
		//_player.set_safe_paid_chips( reader->GetLongLong( "safe_paid_chips" ) );
		_player.set_vault_coins( reader->GetLongLong( "safe_coin" ) );
		_player.set_rakeback_balance( reader->GetLongLong( "rakeback" ) );

		//_player.set_paid_chips( reader->GetLongLong( "paid_chips" ) );
		//_player.set_paid_coin( reader->GetLongLong( "paid_coin" ) );
		_player.set_paid_gems( reader->GetLongLong( "paid_gem" ) );

		// 멤버쉽 클라스 읽어오기.
		int member_ship_number = reader->GetLong( "member_ship_class" );
		//string membership_string = reader->GetString("member_ship_class_string");
		const General::BenefitTier member_ship_class = static_cast< General::BenefitTier >( member_ship_number );
		_player.set_membership_tier( member_ship_class );

		// 갱신 시간 읽어오기
		MYSQL_TIME daily = reader->GetDateTime( "daily_refresh_time" );
		//std::tm daily_time = TimeUtils::MysqlTimeToTM( daily );
		string daily_string = TimeUtils::MYSQLTimeToString( daily );
		_player.set_daily_reset_at( daily_string );

		MYSQL_TIME weekly = reader->GetDateTime( "weekly_refresh_time" );
		//std::tm weekly_time = TimeUtils::MysqlTimeToTM( weekly );
		string weekly_string = TimeUtils::MYSQLTimeToString( weekly );

		MYSQL_TIME monthly = reader->GetDateTime( "monthly_refresh_time" );
		//std::tm monthly_time = TimeUtils::MysqlTimeToTM( monthly );
		string monthly_string = TimeUtils::MYSQLTimeToString( monthly );

		MYSQL_TIME free_coin = reader->GetDateTime( "refresh_time_coin_free_charge" );
		//std::tm free_coin_time = TimeUtils::MysqlTimeToTM( free_coin );
		string free_coin_string = TimeUtils::MYSQLTimeToString( free_coin );
		_player.set_coin_free_charge_reset_at( free_coin_string );

		MYSQL_TIME free_chip = reader->GetDateTime( "refresh_time_chip_free_charge" );
		//std::tm free_chip_time = TimeUtils::MysqlTimeToTM( free_chip );
		string free_chip_string = TimeUtils::MYSQLTimeToString( free_chip );
		_player.set_chip_free_charge_reset_at( free_chip_string );

		_player.set_nickname_changed_once( ( bool ) reader->GetTinyInt( "is_changed_nickname" ) );

		MYSQL_TIME nick_expiry = reader->GetDateTime( "nickname_change_prohibite_expiry_time" );
		string nick_expiry_string = TimeUtils::MYSQLTimeToString( nick_expiry );
		_player.set_nickname_change_locked_until( nick_expiry_string );

		int32 attendance_days = reader->GetLong( "attendance_days" );
		_player.set_attendance_streak_days( attendance_days );
		//std::string test = reader->GetString( "nickname" );
		//test = reader->GetString( "nickname" );


		players.push_back( _player );
	}

	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	mysql_stmt_close( stmt );

	return TRUE;
}

std::future<BOOL> QueryManager::PlayerGetByNicknameAsync( const std::string& nickname , std::vector<General::ParticipantProfile>& players )
{
	return std::async( std::launch::async , [&nickname , &players]() {
		return QueryManager::PlayerGetByNickname( nickname , players );
	} );
}
