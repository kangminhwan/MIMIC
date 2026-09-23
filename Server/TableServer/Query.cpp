#include "Query.h"

#include "./mysql/cMySQL.h"
#include "./mysql/cMySQLParserElement.h"
#include "./mysql/cMySQLReader.h"

#include "./mysql/cMySQLConnectionPooler.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Manager/ServerManager.h"
#include "../Include/Netlib/Queue/cLogQueue.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "TimeUtils.h"
#include "cProtoUtil.h"
#include "StringUtil.h"

#include <mysql.h>
#include <mysqld_error.h>
#include <json/json.h>

#include <iostream>
#include <format>
#include <string.h>
#include <tuple>

using protoutil::cProtoUtil;

QueryManager::QueryManager()
{

}

//BOOL QueryManager::PlayerExecuteQuery( std::string executeQuery )
//{
//	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
//	MYSQL* MySQLConnection = con_ins.GetConnection();
//	if ( MySQLConnection == nullptr )
//		return FALSE;
//
//	try {
//
//		int query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
//		if ( query_stat != 0 ) {
//
//			fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
//			return FALSE;
//		}
//	}
//	catch ( const runtime_error& e ) {
//
//		printf( e.what() );
//	}
//
//	return TRUE;
//}

// 업데이트 하고 성공한 row 카운트가 있을 경우에만 TRUE 리턴
BOOL QueryManager::UpdateExecuteQuery( const E_DB_TYPE& db_type , std::string executeQuery )
{
	cConnInstance con_ins( db_type );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	try {
		int query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
		if ( query_stat != 0 ) {
			fprintf( stderr , "Mysql query error: %s\n" , mysql_error( MySQLConnection ) );
			return FALSE;
		}

		uint64 affected_rows = mysql_affected_rows( MySQLConnection );
		if ( affected_rows == ( uint64 ) -1 ) {
			fprintf( stderr , "Mysql affected rows error: %s\n" , mysql_error( MySQLConnection ) );
			return FALSE;
		}

		if ( affected_rows > 0 ) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
	catch ( const std::runtime_error& e ) {
		printf( "%s\n" , e.what() );
	}

	return FALSE;
}

std::future<BOOL> QueryManager::UpdateExecuteQueryAsync( const E_DB_TYPE& db_type , std::string executeQuery )
{
	return std::async( std::launch::async , [db_type , executeQuery]() {
		return QueryManager::UpdateExecuteQuery( db_type , executeQuery );
	} );
}

BOOL QueryManager::PlayerExecuteQuery( const E_DB_TYPE& db_type , std::string executeQuery )
{
	cConnInstance con_ins( db_type );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;
	if ( std::count( executeQuery.begin() , executeQuery.end() , ';' ) >= 2 )
		return FALSE;
	int query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {

		//fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		std::cerr << executeQuery << std::endl;
		std::cerr << executeQuery.c_str() << std::endl;
		unsigned int error_code = mysql_errno( MySQLConnection );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "QueryManager::PlayerExecuteQuery Query Error [ %s ]" , mysql_error( MySQLConnection ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "ExecuteQuery [ %s ]" , executeQuery.c_str() );

		// 연결이 끊어진 경우 커넥션 재 설정
		// CR_SERVER_LOST 2013, 2006
		//if ( error_code == CR_SERVER_LOST || error_code == CR_SERVER_GONE_ERROR ) {

		//	// 재연결 시도
		//	if ( mysql_ping( MySQLConnection ) == 0 ) {

		//		std::cerr << "Reconnected successfully." << std::endl;

		//		// 재연결 후 쿼리 재시도
		//		if ( mysql_query( MySQLConnection , executeQuery.c_str() ) == 0 ) {
		//			return FALSE;
		//		}
		//	}

		//}

		return FALSE;
	}
	else
	{
		MYSQL_RES* res = mysql_store_result( MySQLConnection );
		if ( res != nullptr ) {
			mysql_free_result( res ); // 결과 버림
		}
	}
	return TRUE;
}

std::future<unsigned int> QueryManager::PlayerExecuteQueryWithErrorCodeAsync( const E_DB_TYPE& db_type , std::string executeQuery )
{
	return std::async( std::launch::async , [db_type , executeQuery]() {
		return QueryManager::PlayerExecuteQueryWithErrorCode( db_type , executeQuery );
	} );
}

// 0 이 아니면 오류가 난것이다.
unsigned int QueryManager::PlayerExecuteQueryWithErrorCode( const E_DB_TYPE& db_type , std::string executeQuery )
{
	cConnInstance con_ins( db_type );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	int query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {

		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		unsigned int error_code = mysql_errno( MySQLConnection );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "QueryManager::PlayerExecuteQuery Query Error [ %s ]" , mysql_error( MySQLConnection ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "ExecuteQuery [ %s ]" , executeQuery.c_str() );

		return error_code;
	}

	return 0;
}

BOOL QueryManager::PlayerExecuteQueryWithAffectedRows( const E_DB_TYPE& db_type , std::string executeQuery )
{
	cConnInstance con_ins( db_type );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	int query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {

		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		unsigned int error_code = mysql_errno( MySQLConnection );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "QueryManager::PlayerExecuteQueryWithAffectedRows Query Error [ %s ]" , mysql_error( MySQLConnection ) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "ExecuteQuery [ %s ]" , executeQuery.c_str() );
		return FALSE;
	}

	// 쿼리가 성공했는지 확인
	my_ulonglong affected_rows = mysql_affected_rows( MySQLConnection );
	if ( affected_rows == ( my_ulonglong ) -1 ) {
		fprintf( stderr , "Mysql affected rows error: %s\n" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	if ( affected_rows > 0 ) {
		//std::cout << "Query executed successfully. Affected rows: " << affected_rows << std::endl;
		return TRUE;
	}
	else {
		//std::cout << "Query executed successfully, but no rows were affected." << std::endl;
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::PlayerExecuteQueryWithAffectedRowsAsync( const E_DB_TYPE& db_type , std::string executeQuery )
{
	return std::async( std::launch::async , [db_type , executeQuery]() {
		return QueryManager::PlayerExecuteQueryWithAffectedRows( db_type , executeQuery );
	} );
}

//std::future<BOOL> QueryManager::PlayerExecuteQueryAsync( std::string executeQuery )
//{
//	return std::async( std::launch::async , [executeQuery]() {
//		return QueryManager::PlayerExecuteQuery( executeQuery );
//	} );
//}

std::future<BOOL> QueryManager::PlayerExecuteQueryAsync( const E_DB_TYPE& db_type , std::string executeQuery )
{
	return std::async( std::launch::async , [db_type , executeQuery]() {

		try
		{
			return QueryManager::PlayerExecuteQuery( db_type , executeQuery );
		}
		catch ( const std::exception& e )
		{
			std::cerr << executeQuery << std::endl;
			std::cerr << "QueryManager::PlayerExecuteQueryAsync Exception occurred in async query execution: " << e.what() << " | Query: " << executeQuery << std::endl;

			std::ostringstream oss;
			oss << "QueryManager::PlayerExecuteQueryAsync Exception occurred in async query execution: " << e.what() << "\n";
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , oss.str().c_str() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Query [ %s ]" , executeQuery.c_str() );

			//throw;
			return FALSE;
		}
		catch ( ... )
		{
			std::cerr << "QueryManager::PlayerExecuteQueryAsync Unknown exception occurred in async query execution | Query: " << executeQuery << std::endl;

			std::ostringstream oss;
			oss << "QueryManager::PlayerExecuteQueryAsync Unknown exception occurred in async query execution: \n";
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , oss.str().c_str() );
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Query [ %s ]" , executeQuery.c_str() );

			//throw;
			return FALSE;
		}
	} );
}

BOOL QueryManager::PlayerExecuteQuery_ID( const E_DB_TYPE& db_type , std::string executeQuery , uint64& inserted_id )
{
	cConnInstance con_ins( db_type );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	try
	{
		int query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
		if ( query_stat != 0 )
		{
			fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
			//mysql_stmt_close( stmt );
			return FALSE;
		}
		else
		{
			inserted_id = mysql_insert_id( MySQLConnection );
		}
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
	}

	return TRUE;
}

std::future<BOOL> QueryManager::PlayerExecuteQueryAsync_ID( const E_DB_TYPE& db_type , std::string executeQuery , uint64& inserted_id )
{
	return std::async( std::launch::async , [db_type , executeQuery , &inserted_id]() {
		return QueryManager::PlayerExecuteQuery_ID( db_type , executeQuery , inserted_id );
	} );
}

BOOL QueryManager::DeleteAccountAsync( const std::string& _platform_guid )
{
	std::string accountQuery = std::format( "DELETE FROM account.user_platform where platform_guid  = '{}';" , _platform_guid );
	std::string playerQuery = std::format( "DELETE FROM tpp.players where platform_guid  = '{}';" , _platform_guid );
	std::string friendsQuery = std::format( "DELETE FROM tpp.friends where friend_platform_guid  = '{}';" , _platform_guid );
	std::vector<std::future<BOOL>> results;
	results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , accountQuery ) );
	results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , playerQuery ) );
	results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , friendsQuery ) );
	for ( auto& result : results )
		result.wait();

	return TRUE;
	
}

std::future<BOOL> QueryManager::CreateAccountAsync( string _name_auth_code , string _name_auth_type , string _account_guid , int _db_index , uint64 _lost_limit , int _lost_limit_change_count , string _refresh_loss_limit , uint64 _buy_limit , string _refresh_buy_limit , uint64_t& _user_account_idx )
{
	return std::async( std::launch::async , [_name_auth_code , _name_auth_type , _account_guid , _db_index , _lost_limit , _lost_limit_change_count , _refresh_loss_limit , _buy_limit , _refresh_buy_limit , &_user_account_idx]() {
		return QueryManager::CreateAccount( _name_auth_code , _name_auth_type , _account_guid , _db_index , _lost_limit , _lost_limit_change_count , _refresh_loss_limit , _buy_limit , _refresh_buy_limit , _user_account_idx );
	} );
}

BOOL QueryManager::CreateAccount( string _name_auth_code , string _name_auth_type , string _account_guid , int _db_index , uint64 _lost_limit , int _lost_limit_change_count , string _refresh_loss_limit , uint64 _buy_limit , string _refresh_buy_limit , uint64_t& _user_account_idx )
{
	BOOL returnValue = FALSE;
	string m_errorString;

	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_ACCOUNT);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
	{
		// add error code
		return returnValue;
	}

	MYSQL_STMT* stmt = nullptr;
	int        status;

	// initialize and prepare CALL statement with parameter placeholders
	stmt = mysql_stmt_init(MySQLConnection);
	if (stmt == nullptr)
	{
		printf("Could not initialize statement\n");
		return returnValue;
	}

	std::string query = "CALL CreateUserAccount(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	status = mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()));
	stmt_error(stmt, status);
	if (status)
		return returnValue;

	//uint64 _loss_limit , int _loss_limit_change_count , string _refresh_loss_limit , uint64 _buy_limit , string _refresh_buy_limit , uint64_t& _user_account_idx

	// push procedure params
	cParamBinder parambinder(10);
	parambinder.BindParam(1, MYSQL_TYPE_STRING, _name_auth_code);
	parambinder.BindParam(2, MYSQL_TYPE_STRING, _name_auth_type);
	parambinder.BindParam(3, MYSQL_TYPE_STRING, _account_guid);
	parambinder.BindParam(4, MYSQL_TYPE_LONG, _db_index);
	parambinder.BindParam(5, MYSQL_TYPE_LONGLONG, _lost_limit );
	parambinder.BindParam(6, MYSQL_TYPE_LONG, _lost_limit_change_count );
	parambinder.BindParam(7, MYSQL_TYPE_STRING , _refresh_loss_limit );
	parambinder.BindParam(8, MYSQL_TYPE_LONGLONG , _buy_limit );
	parambinder.BindParam(9, MYSQL_TYPE_STRING , _refresh_buy_limit );
	parambinder.BindParam(10, MYSQL_TYPE_LONGLONG, _user_account_idx);

	cParamBinder outputBinder(1);
	outputBinder.BindParam(1, MYSQL_TYPE_LONGLONG, _user_account_idx);

	try
	{
		// bind parameters
		// 출력 변수를 문에 바인딩
		status = mysql_stmt_bind_param(stmt, parambinder.GetBinder());
		if (status)
			throw runtime_error(m_errorString.c_str());

		status = mysql_stmt_execute(stmt);
		stmt_error_string(stmt, status, m_errorString);
		if (status)
			throw runtime_error(m_errorString.c_str());

		// OUT 결과를 리턴받을 값이 1개면 1개만 bind 시킨다.
		status = mysql_stmt_bind_result(stmt, outputBinder.GetBinder());
		stmt_error_string(stmt, status, m_errorString);
		if (status)
			throw runtime_error(m_errorString.c_str());

		// 프로시져의 OUT 결과를 가져옴
		status = mysql_stmt_fetch(stmt);
		stmt_error_string(stmt, status, m_errorString);
		if (status)
			throw runtime_error(m_errorString.c_str());

		returnValue = TRUE;
	}
	catch(const runtime_error& e)
	{
		printf(e.what());
	}

	if (stmt != nullptr)
		mysql_stmt_close(stmt);

	return returnValue;
}

string QueryManager::AccountGetByAuthCode(string _name_auth_code)
{
	string account_guid;

	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_ACCOUNT);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr) { /*add error code*/ return account_guid; }

	std::string executeQuery = std::format("select * from user_account where name_auth_code = '{}';", _name_auth_code);

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery(MySQLConnection , executeQuery , result_set);

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			uint64 account_idx = reader->GetLongLong( "account_idx" );
			//string name_auth_code = reader->GetString( "name_auth_code" );
			//string name_auth_type = reader->GetString( "name_auth_type" );
			account_guid = reader->GetString( "account_guid" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch(const runtime_error& e)
	{
		//printf(e.what());
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	//if (stmt != nullptr)
	//	mysql_stmt_close(stmt);

	return account_guid;
}

std::future<General::LossLimitProfile> QueryManager::GetLostLimitAsync( const std::string& account_guid )
{
	return std::async( std::launch::async , [&account_guid]() {
		return QueryManager::GetLostLimit( account_guid );
	} );
}

General::LossLimitProfile QueryManager::GetLostLimit( const std::string& account_guid )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) { /*add error code*/ return General::LossLimitProfile::default_instance(); }

	General::LossLimitProfile _lost_limit;

	std::string executeQuery = std::format( "select * from user_account where account_guid = '{}';" , account_guid );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );

		/*
		message LostLimit
		{
			uint64 lost_limit = 1;							// 손실한도 설정 금액
			int32 loss_limit_change_count = 2;				// 손실한도 변경 횟수 ( 매월 2회 변경 가능, 매월 1일 0시 기준 초기화 )
			string refresh_time_of_loss_limit = 3;			// 손실한도 갱신 시간
			uint64 buy_limit = 4;							// 구매 제한 금액
			string refresh_time_of_buy_limit = 5;			// 구매 제한 갱신 시간
		}
		*/

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			_lost_limit.Clear();

			uint64 account_idx = reader->GetLongLong( "account_idx" );
			_lost_limit.set_account_id( account_idx );
			//string name_auth_code = reader->GetString( "name_auth_code" );
			//string name_auth_type = reader->GetString( "name_auth_type" );
			//string account_guid = reader->GetString( "account_guid" );

			uint64 lost_limit = reader->GetLongLong( "lost_limit" );
			_lost_limit.set_loss_limit_amount( lost_limit );

			uint64 next_lost_limit = reader->GetLongLong( "next_lost_limit" );
			_lost_limit.set_pending_loss_limit( next_lost_limit );

			int lost_limit_change_count = reader->GetLong( "lost_limit_change_count" );
			_lost_limit.set_loss_limit_changes( lost_limit_change_count );

			int64 daily_lost_chip = reader->GetLongLong( "daily_lost_chip" );
			_lost_limit.set_daily_chip_loss( daily_lost_chip );

			MYSQL_TIME mysql_datetime = reader->GetDateTime( "refresh_time_of_loss_limit" );
			string refresh_time_of_lost_limit = TimeUtils::MYSQLTimeToString( mysql_datetime );
			_lost_limit.set_loss_limit_reset_at( refresh_time_of_lost_limit );

			uint64 buy_limit = reader->GetLongLong( "buy_limit" );
			_lost_limit.set_purchase_limit_amount( buy_limit );

			uint64 monthly_buy_total = reader->GetLongLong( "monthly_buy_total" );
			_lost_limit.set_monthly_purchase_total( monthly_buy_total );

			mysql_datetime = reader->GetDateTime( "refresh_time_of_buy_limit" );
			string refresh_time_of_buy_limit = TimeUtils::MYSQLTimeToString( mysql_datetime );
			_lost_limit.set_purchase_limit_reset_at( refresh_time_of_buy_limit );

			int32 set_time_limit = reader->GetLong("set_time_limit");
			_lost_limit.set_play_block_hours( set_time_limit );

			int32 next_set_time_limit = reader->GetLong( "next_set_time_limit" );
			_lost_limit.set_pending_play_block_hours( next_set_time_limit );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf(e.what());
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return _lost_limit;
}

BOOL QueryManager::CreatePlatform(string _account_guid, string _platform_guid, string _platform_authcode, BYTE _platform_code, string _MADE_id , string _MADE_password , BYTE _push_token_os, string _push_token, int _db_index, string _sub_password , uint64_t& _platform_idx)
{
	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_ACCOUNT);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
		return FALSE;

	// initialize and prepare CALL statement with parameter placeholders
	MYSQL_STMT* stmt = mysql_stmt_init(MySQLConnection);
	if (stmt == nullptr)
		return FALSE;

	std::string query = "CALL CreateUserPlatform(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	int status = mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()));
	stmt_error(stmt, status);
	if (status)
		return FALSE;

	string error_string;

	// 파람바인더에 파라미터를 추가해준다.
	cParamBinder parambinder(11);
	parambinder.BindParam(1, MYSQL_TYPE_STRING, _account_guid);
	parambinder.BindParam(2, MYSQL_TYPE_STRING, _platform_guid);
	parambinder.BindParam(3, MYSQL_TYPE_STRING, _platform_authcode );
	parambinder.BindParam(4, MYSQL_TYPE_TINY, _platform_code);
	parambinder.BindParam(5, MYSQL_TYPE_STRING, _MADE_id);
	parambinder.BindParam(6, MYSQL_TYPE_STRING, _MADE_password );
	parambinder.BindParam(7, MYSQL_TYPE_TINY, _push_token_os);
	parambinder.BindParam(8, MYSQL_TYPE_STRING, _push_token);
	parambinder.BindParam(9, MYSQL_TYPE_LONG, _db_index);
	parambinder.BindParam(10, MYSQL_TYPE_STRING , _sub_password);
	parambinder.BindParam(11 , MYSQL_TYPE_LONGLONG , _platform_idx );

	cParamBinder outputBinder(1);
	outputBinder.BindParam(1, MYSQL_TYPE_LONGLONG, _platform_idx);

	// bind parameters
	status = mysql_stmt_bind_param(stmt, parambinder.GetBinder());
	stmt_error_string(stmt, status, error_string);
	if (status)
		return FALSE;

	status = mysql_stmt_execute(stmt);
	stmt_error_string(stmt, status, error_string);
	if (status)
		return FALSE;

	// OUT 결과를 리턴받을 값이 1개면 1개만 bind 시킨다.
	status = mysql_stmt_bind_result(stmt, outputBinder.GetBinder());
	stmt_error_string(stmt, status, error_string);
	if (status)
		return FALSE;

	// 프로시져의 OUT 결과를 가져옴
	status = mysql_stmt_fetch(stmt);
	stmt_error_string(stmt, status, error_string);
	if (status)
		return FALSE;

	mysql_stmt_close(stmt);
	return TRUE;
}

std::string QueryManager::GenerateCreatePlatform( string _account_guid , string _platform_guid , string _platform_authcode , BYTE _platform_code , string _MADE_id , string _MADE_password , BYTE _push_token_os , string _push_token , int _db_index , string _sub_password , uint64_t& _platform_idx )
{
	std::ostringstream oss;
	oss << "INSERT INTO user_platform (account_guid, platform_guid, platform_authcode, platform_code, tpp_id, tpp_password, push_token_os, push_token, db_index, sub_password , reg_date) VALUES ('"
		<< _account_guid << "', '"
		<< _platform_guid << "', '"
		<< _platform_authcode << "', "
		<< _platform_code << ", '"
		<< _MADE_id << "', '"
		<< _MADE_password << "', "
		<< _push_token_os << ", '"
		<< _push_token << "', "
		<< _db_index << ", '"
		<< _sub_password << "' "
		<< "now() );";

	return oss.str();
}

std::future<BOOL> QueryManager::FindMADEPlatformAsync( const std::string& _MADE_id , std::string& _account_guid , std::string& _platform_guid , BYTE& _platform_code , std::string& _MADE_password , int& _passwd_retry_count ,
		std::string& _sub_password , int& _sub_passwd_retry_count , BOOL& _game_play_agree , BOOL& _personal_info_agree , BOOL& _advertise_push_agree , BOOL& _night_advertise_push_agree , std::string& expected_withdrawal_date,
		int& _cancle_withdrawal_first_join )
{
	return std::async( std::launch::async , [_MADE_id , &_account_guid , &_platform_guid , &_platform_code , &_MADE_password , &_passwd_retry_count, &_sub_password , &_sub_passwd_retry_count , &_game_play_agree , &_personal_info_agree , &_advertise_push_agree , &_night_advertise_push_agree , &expected_withdrawal_date, &_cancle_withdrawal_first_join]() {
		return QueryManager::FindMADEPlatform( _MADE_id , _account_guid , _platform_guid , _platform_code , _MADE_password , _passwd_retry_count , _sub_password , _sub_passwd_retry_count , _game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , expected_withdrawal_date, _cancle_withdrawal_first_join );
	} );
}

BOOL QueryManager::FindMADEPlatform( const std::string& _MADE_id , 
	std::string& _account_guid , 
	std::string& _platform_guid , 
	BYTE& _platform_code ,
	std::string& _MADE_password , 
	int& _passwd_retry_count , 
	std::string& _sub_password , 
	int& _sub_passwd_retry_count ,
	BOOL& _game_play_agree ,
	BOOL& _personal_info_agree ,
	BOOL& _advertise_push_agree ,
	BOOL& _night_advertise_push_agree ,
	std::string& _expected_withdrawal_date,
	int& _cancle_withdrawal_first_join )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) return FALSE;

	std::string executeQuery = std::format( "select * from user_platform where tpp_id = '{}';" , _MADE_id );

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

			_account_guid = reader->GetString( "account_guid" );
			_platform_guid = reader->GetString( "platform_guid" );
			_MADE_password = reader->GetString( "tpp_password" );
			_passwd_retry_count = reader->GetLong( "passwd_retry_count" );
			_sub_password = reader->GetString( "sub_password" );
			_sub_passwd_retry_count = reader->GetLong( "sub_passwd_retry_count" );
			_game_play_agree = reader->GetTinyInt( "game_play_agree" );
			_personal_info_agree = reader->GetTinyInt( "personal_info_agree" );
			_advertise_push_agree = reader->GetTinyInt( "advertise_push_agree" );
			_night_advertise_push_agree = reader->GetTinyInt( "night_advertise_push_agree" );
			_platform_code = reader->GetTinyInt( "platform_code" );

			// 1970-01-01 00:00:00 인 경우에는 DateTime 셋팅이 되지 않은 것으로 간주한다.
			MYSQL_TIME mysql_datetime = reader->GetDateTime( "expected_withdrawal_date" );
			_expected_withdrawal_date = TimeUtils::MYSQLTimeToString( mysql_datetime );
			if ( _expected_withdrawal_date._Equal( "1970-01-01 00:00:00" ) ) {
				_expected_withdrawal_date = "";
			}

			_cancle_withdrawal_first_join = reader->GetTinyInt( "cancle_withdrawal_first_join" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf(e.what());
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

BOOL QueryManager::UpdateMADEPasswordRetryCount( std::string _account_guid , std::string _platform_guid , int _passwd_retry_count )
{
	std::string executeQuery = std::format( "update user_platform set passwd_retry_count = {} where account_guid = '{}' and platform_guid = '{}';" , _passwd_retry_count , _account_guid , _platform_guid );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::ResetMADEPassword( std::string _account_guid , std::string _platform_guid , std::string _MADE_password )
{
	std::string executeQuery = std::format( "update user_platform set tpp_password = '{}' , passwd_retry_count = 0 where account_guid = '{}' and platform_guid = '{}';" , _MADE_password , _account_guid , _platform_guid );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::UpdateSubPassword( const std::string& _account_guid , const std::string& _platform_guid , const std::string& _hashed_sub_password )
{
	std::string executeQuery = std::format( "update user_platform set sub_password_update_time = NOW() , sub_password = '{}', sub_passwd_retry_count = 0 where account_guid = '{}' and platform_guid = '{}';" , _hashed_sub_password , _account_guid , _platform_guid );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::UpdateSubPasswordRetryCount( std::string _account_guid , std::string _platform_guid , int _passwd_retry_count )
{
	std::string executeQuery = std::format( "update user_platform set sub_passwd_retry_count = {} where account_guid = '{}' and platform_guid = '{}';" , _passwd_retry_count , _account_guid , _platform_guid );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::RemoveSubPasswordRetryCount( std::string _platform_guid )
{
	std::string executeQuery = std::format( "update user_platform set sub_password = '' where platform_guid = '{}';" , _platform_guid );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

std::future<BOOL> QueryManager::FindPlatformAsync( const std::string _account_guid , const std::string _platform_guid , std::string& _platform_authcode , std::string& _sub_password , int& _sub_passwd_retry_count ,
	BOOL& _game_play_agree ,
	BOOL& _personal_info_agree ,
	BOOL& _advertise_push_agree ,
	BOOL& _night_advertise_push_agree ,
	std::string& _expected_withdrawal_date,
	int& _cancle_withdrawal_first_join )
{
	return std::async( std::launch::async , [ _account_guid , _platform_guid , &_platform_authcode , &_sub_password , &_sub_passwd_retry_count , &_game_play_agree, &_personal_info_agree, &_advertise_push_agree, &_night_advertise_push_agree, &_expected_withdrawal_date, &_cancle_withdrawal_first_join]() {
		return QueryManager::FindPlatform( _account_guid , _platform_guid , _platform_authcode , _sub_password ,_sub_passwd_retry_count , _game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _expected_withdrawal_date , _cancle_withdrawal_first_join );
	} );
}

BOOL QueryManager::FindPlatform( const std::string _account_guid , const std::string _platform_guid , std::string& _platform_authcode , std::string& _sub_password , int& _sub_passwd_retry_count ,
	BOOL& _game_play_agree ,
	BOOL& _personal_info_agree ,
	BOOL& _advertise_push_agree ,
	BOOL& _night_advertise_push_agree , 
	std::string& _expected_withdrawal_date,
	int& _cancle_withdrawal_first_join )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) return FALSE;

	std::string executeQuery = std::format( "select * from user_platform where account_guid = '{}' and platform_guid = '{}';" , _account_guid , _platform_guid );

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

			_platform_authcode = reader->GetString( "platform_authcode" );
			_sub_password = reader->GetString( "sub_password" );
			_sub_passwd_retry_count = reader->GetLong( "sub_passwd_retry_count" );
			_game_play_agree = reader->GetTinyInt( "game_play_agree" );
			_personal_info_agree = reader->GetTinyInt( "personal_info_agree" );
			_advertise_push_agree = reader->GetTinyInt( "advertise_push_agree" );
			_night_advertise_push_agree = reader->GetTinyInt( "night_advertise_push_agree" );

			// 1970-01-01 00:00:00 인 경우에는 DateTime 셋팅이 되지 않은 것으로 간주한다.
			MYSQL_TIME mysql_datetime = reader->GetDateTime( "expected_withdrawal_date" );
			_expected_withdrawal_date = TimeUtils::MYSQLTimeToString( mysql_datetime );
			if ( _expected_withdrawal_date._Equal( "1970-01-01 00:00:00" ) ) {
				_expected_withdrawal_date = "";
			}

			_cancle_withdrawal_first_join = reader->GetTinyInt( "cancle_withdrawal_first_join" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf(e.what());
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

std::future<BOOL> QueryManager::FindPlatformByCidAndPlatformGuidAsync( const std::string _cid , const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count )
{
	return std::async( std::launch::async , [_cid , _platform_guid , &_sub_password , &_sub_passwd_retry_count]() {
		return QueryManager::FindPlatformByCidAndPlatformGuid( _cid , _platform_guid , _sub_password , _sub_passwd_retry_count );
	} );
}


BOOL QueryManager::FindPlatformByCidAndPlatformGuid( const std::string _cid , const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) return FALSE;

	std::string executeQuery = std::format( "select * from user_platform where platform_authcode = '{}' and platform_guid = '{}';" , _cid , _platform_guid );

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

			_sub_password = reader->GetString( "sub_password" );
			_sub_passwd_retry_count = reader->GetLong( "sub_passwd_retry_count" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf(e.what());
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

std::future<BOOL> QueryManager::FindSubPasswordByPlatformGuidAsync( const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count , std::string& _sub_password_update_time )
{
	return std::async( std::launch::async , [ _platform_guid , &_sub_password , &_sub_passwd_retry_count , &_sub_password_update_time]() {
		return QueryManager::FindSubPasswordByPlatformGuid( _platform_guid , _sub_password , _sub_passwd_retry_count , _sub_password_update_time );
	} );
}

BOOL QueryManager::FindSubPasswordByPlatformGuid( const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count , std::string& _sub_password_update_time )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) return FALSE;

	std::string executeQuery = std::format( "select * from user_platform where platform_guid = '{}';" , _platform_guid );

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

			_sub_password = reader->GetString( "sub_password" );
			_sub_passwd_retry_count = reader->GetLong( "sub_passwd_retry_count" );

			if ( reader->isDateTime( "sub_password_update_time" ) )
				_sub_password_update_time = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "sub_password_update_time" ) );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf(e.what());
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

std::future<BOOL> QueryManager::UpdateTermsAgree( const std::string& _MADE_id , const BOOL _game_play_agree , const BOOL _personal_info_agree , const  BOOL _advertise_push_agree , const BOOL _night_advertise_push_agree )
{
	std::string executeQuery = std::format( "update user_platform set game_play_agree = {}, game_play_agree = {}, advertise_push_agree = {}, night_advertise_push_agree = {} where tpp_id = '{}';" 
		, _game_play_agree , _personal_info_agree , _advertise_push_agree  , _night_advertise_push_agree  , _MADE_id );
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
}

std::future<BOOL> QueryManager::UpdateTermsAgreeByPlatformGuid( const std::string& _platform_guid , const BOOL _game_play_agree , const BOOL _personal_info_agree , const  BOOL _advertise_push_agree , const BOOL _night_advertise_push_agree )
{
	std::string executeQuery = std::format( "update user_platform set game_play_agree = {}, game_play_agree = {}, advertise_push_agree = {}, night_advertise_push_agree = {} where platform_guid = '{}';"
		, _game_play_agree , _personal_info_agree , _advertise_push_agree , _night_advertise_push_agree , _platform_guid );
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
}
std::future<BOOL> QueryManager::UpdatePushAgreeByPlatformGuid( const std::string& _platform_guid , const  BOOL _advertise_push_agree , const BOOL _night_advertise_push_agree )
{
	std::string executeQuery = std::format( "update user_platform set  advertise_push_agree = {}, night_advertise_push_agree = {} where platform_guid = '{}';"
		, _advertise_push_agree , _night_advertise_push_agree , _platform_guid );
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
}
std::future<BOOL> QueryManager::UpdatePushToken( const std::string& _platform_guid , const  BYTE& _push_token_os , const string _push_token )
{
	std::string executeQuery = std::format( "update user_platform set  push_token_os = {}, push_token = '{}' where platform_guid = '{}';"
		, _push_token_os , _push_token , _platform_guid );
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
}

BOOL QueryManager::PlatformGet( string _account_guid , string _platform_authcode , string& _platform_guid , BYTE& _platform_code , string& _MADE_id , BYTE& _push_token_os , string& _push_token , int& _db_index , uint64_t& _platform_idx )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_STMT* stmt = nullptr;
	int        status;

	// initialize and prepare CALL statement with parameter placeholders
	//stmt = mysql_stmt_init( MySQLConnection );
	//if ( stmt == nullptr ) {
	//	printf( "Could not initialize statement\n" );
	//	return FALSE;
	//}

	//std::string query = "CALL PlatformGet(?,?)";
	//status = mysql_stmt_prepare( stmt , query.c_str() , static_cast< unsigned long >( query.length() ) );
	//stmt_error( stmt , status );
	//if ( status )
	//	return FALSE;

	// push procedure params
	/*cParamBinder parambinder( 2 );
	parambinder.BindParam( 1 , MYSQL_TYPE_STRING , _account_guid );
	parambinder.BindParam( 2 , MYSQL_TYPE_STRING , _platform_authcode );*/

	std::string executeQuery = std::format( "select * from user_platform where account_guid = '{}' and platform_authcode = '{}';" 
		, _account_guid, _platform_authcode );

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

			_platform_idx = reader->GetLongLong( "platform_idx" );
			//string account_guid = reader->GetString( "account_guid" );
			_platform_guid = reader->GetString( "platform_guid" );
			//string _platform_authcode = reader->GetString( "platform_authcode" );
			_platform_code = reader->GetTinyInt( "platform_code" );
			_MADE_id = reader->GetString( "tpp_id" );
			_push_token_os = reader->GetTinyInt( "push_token_os" );
			_push_token = reader->GetString( "push_token" );
			_db_index = reader->GetLong( "db_index" );
			string _reg_dat = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "reg_date" ) );
			//MYSQL_TIME reg_dat = reader->GetDateTime( "reg_date" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
	}

	if ( stmt != nullptr )
		mysql_stmt_close( stmt );

	return TRUE;
}

std::future<BOOL> QueryManager::FindAccountByPlatformAuthCodeAsync( const std::string& _platform_authcode , std::string& _account_guid , string& _platform_guid )
{
	return std::async( std::launch::async , [_platform_authcode, &_account_guid, &_platform_guid]() {
		return QueryManager::FindAccountByPlatformAuthCode( _platform_authcode , _account_guid , _platform_guid );
	} );
}

BOOL QueryManager::FindAccountByPlatformAuthCode( const std::string& _platform_authcode , std::string& _account_guid , string& _platform_guid )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select * from user_platform where platform_authcode = '{}';",  _platform_authcode );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
			return FALSE;

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			//_platform_idx = reader->GetLongLong( "platform_idx" );
			_account_guid = reader->GetString( "account_guid" );
			_platform_guid = reader->GetString( "platform_guid" );
			//string _platform_authcode = reader->GetString( "platform_authcode" );
			//_platform_code = reader->GetTinyInt( "platform_code" );
			//_nickname = reader->GetString( "nickname" );
			//_push_token_os = reader->GetTinyInt( "push_token_os" );
			//_push_token = reader->GetString( "push_token" );
			//_db_index = reader->GetLong( "db_index" );
			//string _reg_dat = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "reg_date" ) );
			//MYSQL_TIME reg_dat = reader->GetDateTime( "reg_date" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf( e.what() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

std::future<string> QueryManager::FindCiExpiryTimeAsync( const std::string& _account_guid )
{
	return std::async( std::launch::async , [ &_account_guid ]() {
		return QueryManager::FindCiExpiryTime( _account_guid );
	} );
}

std::string QueryManager::FindCiExpiryTime( const std::string& _account_guid )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return std::string();

	std::string _ci_expiry_time;

	std::string executeQuery = std::format( "select ci_expiry_time from user_account where account_guid = '{}';" , _account_guid );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
			return FALSE;

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			_ci_expiry_time = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "ci_expiry_time" ) );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf( e.what() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return _ci_expiry_time;
}

std::future<string> QueryManager::FindExpiryTimeByCiAsync( const std::string& ci )
{
	return std::async( std::launch::async , [ &ci ]() {
		return QueryManager::FindExpiryTimeByCi( ci );
	} );
}

// ci 로 만료 시간 검색
std::string QueryManager::FindExpiryTimeByCi( const std::string& ci )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return std::string();

	std::string _ci_expiry_time;

	std::string executeQuery = std::format( "select ci_expiry_time from user_account where name_auth_code = '{}';" , ci );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
			return FALSE;

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			_ci_expiry_time = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "ci_expiry_time" ) );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf( e.what() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return _ci_expiry_time;
}

BOOL QueryManager::UpdateNiceAuthExpiryTime( const std::string& ci , const std::string& expiry_time )
{
	std::string executeQuery = std::format( "update user_account set ci_expiry_time = '{}' where name_auth_code = '{}';" , expiry_time , ci);
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::UpdateDailyRefreshTime( const std::string& refresh_time , const std::string& platform_guid )
{
	std::string executeQuery = std::format( "update players set daily_refresh_time = '{}' where platform_guid = '{}';" , refresh_time , platform_guid );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::UpdateSystemData( const std::string& data_type , const std::string& data)
{
	std::string executeQuery = std::format( "update system_data set data = '{}' where data_type = '{}';" , data , data_type );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::InsertPlayer(
	string _account_guid,
	string _platform_guid,
	string _nickname,
	int _level,
	uint64 _exp,
	uint64 _chips,
	uint64 _coin,
	uint64 _gem ,
	int _kick_ticket_count,
	int _avatar_id,
	bool _membership_activated,
	string _membership_expiry_time,
	int _chips_refill_count,
	int _coin_refill_count,
	uint64 _remain_slot_coin ,
	string _daily_refresh_time,
	string _monthly_refresh_time,
	uint64& _player_idx)
{
	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_SHARD);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
		return FALSE;

	// initialize and prepare CALL statement with parameter placeholders
	MYSQL_STMT* stmt = mysql_stmt_init(MySQLConnection);
	if (stmt == nullptr)
		return FALSE;

	std::string query = "CALL InsertPlayer(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	int status = mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()));
	stmt_error(stmt, status);
	if (status)
		return FALSE;

	// 파람바인더에 파라미터를 추가해준다.
	cParamBinder parambinder(18);
	parambinder.BindParam(1, MYSQL_TYPE_STRING, _account_guid);
	parambinder.BindParam(2, MYSQL_TYPE_STRING, _platform_guid);
	parambinder.BindParam(3, MYSQL_TYPE_STRING, _nickname);
	parambinder.BindParam(4, MYSQL_TYPE_LONG, _level);
	parambinder.BindParam(5, MYSQL_TYPE_LONGLONG, _exp);
	parambinder.BindParam(6, MYSQL_TYPE_LONGLONG, _chips);
	parambinder.BindParam(7, MYSQL_TYPE_LONGLONG, _coin);
	parambinder.BindParam(8, MYSQL_TYPE_LONGLONG, _gem);
	parambinder.BindParam(9, MYSQL_TYPE_LONG, _kick_ticket_count);
	parambinder.BindParam(10, MYSQL_TYPE_LONG, _avatar_id);
	parambinder.BindParam(11, MYSQL_TYPE_TINY, _membership_activated);
	parambinder.BindParam(12, MYSQL_TYPE_STRING, _membership_expiry_time);
	parambinder.BindParam(13, MYSQL_TYPE_LONG, _chips_refill_count);
	parambinder.BindParam(14, MYSQL_TYPE_LONG, _coin_refill_count);
	parambinder.BindParam(15, MYSQL_TYPE_LONG, _remain_slot_coin);
	parambinder.BindParam(16, MYSQL_TYPE_STRING, _daily_refresh_time);
	parambinder.BindParam(17, MYSQL_TYPE_STRING, _monthly_refresh_time);
	parambinder.BindParam(18, MYSQL_TYPE_LONG, _player_idx);

	cParamBinder outputBinder(1);
	outputBinder.BindParam(1, MYSQL_TYPE_LONGLONG, _player_idx);

	// bind parameters
	status = mysql_stmt_bind_param(stmt, parambinder.GetBinder());
	stmt_error(stmt, status);
	if (status)
		return FALSE;

	BOOL return_value = TRUE;
	std::string error_string = "";
	status = mysql_stmt_execute(stmt);
	stmt_error_string(stmt, status, error_string);
	if (status)
		return_value = FALSE;

	status = mysql_stmt_bind_result(stmt, outputBinder.GetBinder());
	stmt_error_string(stmt, status, error_string);
	if (status)
		return FALSE;

	// 프로시져의 OUT 결과를 가져옴
	status = mysql_stmt_fetch(stmt);
	stmt_error_string(stmt, status, error_string);
	if (status)
		return FALSE;

	mysql_stmt_close(stmt);
	return return_value;
}


std::future<BOOL> QueryManager::PlayerUpdateByQuery( const General::ParticipantProfile& player , const Server::ParticipantProfileInternal& playerExt )
{
	std::stringstream ss;
	ss << "UPDATE players SET "
		<< "level = " << player.member_level() << ", "
		<< "exp = " << player.experience_points() << ", "
		<< "chips = " << player.wallet_chips() << ", "
		<< "coin = " << player.wallet_coins() << ", "
		<< "remain_slot_coin = " << playerExt.remaining_reel_coin() << ", "
		<< "gem = " << player.wallet_gems() << ", "
		<< "kick_ticket_count = " << player.kick_ticket_balance() << ", "
		<< "avatar_id = " << player.equipped_avatar_id() << ", "
		<< "membership_activated = " << player.membership_enabled() << ", "
		<< "membership_expiry_time = '" << player.membership_expires_at() << "', "
		<< "chips_refill_count = " << player.chip_refill_uses() << ", "
		<< "coin_refill_count = " << player.coin_refill_uses() << ", "
		<< "safe_chips = " << player.vault_chips() << ", "
		<< "safe_coin = " << player.vault_coins() << ", "
		<< "blackjack_straight_wins = " << playerExt.blackjack_win_streak() << ", "
		<< "daily_refresh_time = '" << player.daily_reset_at() << "', "
		<< "weekly_refresh_time = '" << playerExt.weekly_reset_at() << "', "
		<< "monthly_refresh_time = '" << playerExt.monthly_reset_at() << "', "
		<< "refresh_time_coin_free_charge = '" << player.coin_free_charge_reset_at() << "', "
		<< "refresh_time_chip_free_charge = '" << player.chip_free_charge_reset_at() << "', "
		<< "member_ship_class = " << player.membership_tier() << ", "
		//<< "member_ship_class_string = '" << player.member_ship_class_string() << "', "
		<< "rakeback = " << player.rakeback_balance() << ", "
		<< "paid_gem = " << player.paid_gems() << ", "
		<< "is_changed_nickname = " << player.nickname_changed_once() << ", "
		<< "nickname_change_prohibite_expiry_time = '" << player.nickname_change_locked_until() << "', "
		<< "attendance_days = " << player.attendance_streak_days() << ", "
		<< "lowbaduki_today_chip = " << playerExt.lowbadugi_daily_chips() << ", "
		<< "lowbaduki_today_coin = " << playerExt.lowbadugi_daily_coins() << ", "
		<< "holdem_today_chip = " << playerExt.holdem_daily_chips() << ", "
		<< "holdem_today_coin = " << playerExt.holdem_daily_coins() << ", "
		<< "delete_friend_count = " << playerExt.friend_deletes_today() << ", "
		<< "daily_coin_limit_mail_count = " << playerExt.coin_limit_mail_count() << ", "
		<< "ranking_reward_received_date = '" << playerExt.ranking_reward_claimed_date() << "' "
		//<< "reg_date = '" << player.reg_date() << "' "
		<< "WHERE player_idx = " << player.member_id() << ";";

	std::string executeQuery = ss.str();
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , executeQuery );
}

//std::future<BOOL> QueryManager::PlayerUpdateAsync( const General::ParticipantProfile& player )
//{
//	return std::async( std::launch::async , [&player]() {
//		return QueryManager::PlayerUpdate( player );
//	} );
//}
//
//BOOL QueryManager::PlayerUpdate( const General::ParticipantProfile& player )
//{
//	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
//	MYSQL* MySQLConnection = con_ins.GetConnection();
//	if ( MySQLConnection == nullptr )
//		return FALSE;
//
//	// initialize and prepare CALL statement with parameter placeholders
//	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
//	if ( stmt == nullptr )
//		return FALSE;
//
//	std::string query = "CALL PlayerUpdate(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
//	int status = mysql_stmt_prepare( stmt , query.c_str() , static_cast< unsigned long >( query.length() ) );
//	stmt_error( stmt , status );
//	if ( status )
//		return FALSE;
//
//	// 파람바인더에 파라미터를 추가해준다.
//	int paramSeq = 0;
//	cParamBinder parambinder( 20 );
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , player.display_name() ); // 닉네임은 여기에서 갱신하지 않음
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.member_level() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.experience_points() );
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.loss_limit() );
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , player.refresh_time_of_loss_limit() ); // MYSQL_TYPE_DATETIME DateTime 형식이 없어서 string 으로 집어 넣습니다.
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.buy_limit() );
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , player.refresh_time_of_buy_limit() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.wallet_chips() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.wallet_coins() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.wallet_gems() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.kick_ticket_balance() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.equipped_avatar_id() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.membership_tier() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_TINY , player.membership_enabled() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , player.membership_expires_at() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.chip_refill_uses() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.coin_refill_uses() );
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_TINY , player.sub_passwd_activated() );
//	//parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , player.aux_key() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.remaining_reel_coin() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_TINY , player.nickname_changed_once() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.lowbadugi_daily_chips() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.lowbadugi_daily_coins() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.holdem_daily_chips() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , player.holdem_daily_coins() );
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , player.nickname_change_locked_until() );
//	
//	bool boolTest = player.nickname_changed_once();
//	std::string test = player.nickname_change_locked_until();
//	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , player.member_id() );
//
//	// bind parameters
//	status = mysql_stmt_bind_param( stmt , parambinder.GetBinder() );
//	stmt_error( stmt , status );
//	if ( status )
//		return FALSE;
//
//	BOOL return_value = TRUE;
//	std::string error_string = "";
//	status = mysql_stmt_execute( stmt );
//	stmt_error_string( stmt , status , error_string );
//	if ( status )
//		return_value = FALSE;
//
//	// 영향을 끼친 로우가 없으면 실패로 간주한다.
//	uint64 affected_rows = mysql_affected_rows( MySQLConnection );
//	if ( affected_rows < 1 ) {
//		stmt_error_string( stmt , status , error_string );
//		return_value = FALSE;
//	}
//	
//	mysql_stmt_close( stmt );
//	return return_value;
//}

BOOL QueryManager::PlayerMoneyUpdate( const uint64& _player_idx , const uint64& _coin , const uint64& _chip , const uint64& _rakeback , const uint64& _gem , const uint64& _paid_chips , const uint64& _paid_coin , const uint64& _paid_gem )
{
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratePlayerMoneyUpdate( _player_idx , _coin , _chip , _rakeback , _gem , _paid_chips , _paid_coin , _paid_gem ) );
	result.wait();
	return result.get();
}

std::string QueryManager::GeneratePlayerMoneyUpdate( const uint64& _player_idx , const uint64& _coin , const uint64& _chip , const uint64& _rakeback , const uint64& _gem , const uint64& _paid_chips , const uint64& _paid_coin , const uint64& _paid_gem )
{
	// 업데이트 쿼리 생성
	std::string query = "UPDATE players SET coin = " + std::to_string( _coin ) + ", " +
		"chips = " + std::to_string( _chip ) + ", "
		"gem = " + std::to_string( _gem ) + ", "
		//"paid_chips = " + std::to_string( _paid_chips ) + ", "
		//"paid_coin = " + std::to_string( _paid_coin ) + ", "
		"paid_gem = " + std::to_string( _paid_gem ) + ", "
		"rakeback = " + std::to_string( _rakeback ) + " "
		"WHERE player_idx = " + std::to_string( _player_idx );

	return query;
}

BOOL QueryManager::PlayerSafeMoneyUpdate( const General::ParticipantProfile& player )
{
	// 업데이트 쿼리 생성
	std::string query = "UPDATE players SET safe_chips = " + std::to_string( player.vault_chips() ) + ", " +
		//"safe_paid_chips = " + std::to_string( player.safe_paid_chips() ) + ", "
		"safe_coin = " + std::to_string( player.vault_coins() ) + " "
		"WHERE player_idx = " + std::to_string( player.member_id() );
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
	result.wait();
	return result.get();
}

BOOL QueryManager::PlayerSetSubPasswd( const uint64& _player_idx , const bool& _sub_passwd_activated , const std::string& _sub_passwd )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	// initialize and prepare CALL statement with parameter placeholders
	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	std::string query = "CALL PlayerSetSubPasswd(?, ?, ?)";
	int status = mysql_stmt_prepare( stmt , query.c_str() , static_cast< unsigned long >( query.length() ) );
	stmt_error( stmt , status );
	if ( status )
		return FALSE;

	// 파람바인더에 파라미터를 추가해준다.
	int paramSeq = 0;
	cParamBinder parambinder( 3 );
	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , _player_idx );
	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_TINY , _sub_passwd_activated );
	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_STRING , _sub_passwd );

	// bind parameters
	status = mysql_stmt_bind_param( stmt , parambinder.GetBinder() );
	stmt_error( stmt , status );
	if ( status )
		return FALSE;

	BOOL return_value = TRUE;
	std::string error_string = "";
	status = mysql_stmt_execute( stmt );
	stmt_error_string( stmt , status , error_string );
	if ( status )
		return_value = FALSE;

	// 영향을 끼친 로우가 없으면 실패로 간주한다.
	uint64 affected_rows = mysql_affected_rows( MySQLConnection );
	if ( affected_rows < 1 ) {
		stmt_error_string( stmt , status , error_string );
		return_value = FALSE;
	}

	mysql_stmt_close( stmt );
	return return_value;
}

BOOL QueryManager::GetPlayer(string _account_guid, string _platform_guid, General::ParticipantProfile& _player, Server::ParticipantProfileInternal& _playerExt)
{
	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
		return FALSE;

	MYSQL_STMT* stmt = mysql_stmt_init(MySQLConnection);
	if (stmt == nullptr)
		return FALSE;

	std::string query = "CALL PlayerGet(?, ?)";

	// 파람바인더에 파라미터를 추가해준다.
	cParamBinder parambinder(2);
	parambinder.BindParam(1, MYSQL_TYPE_STRING, _account_guid);
	parambinder.BindParam(2, MYSQL_TYPE_STRING, _platform_guid);

	NetLib::cVector<cMySQLReader*> result_set;

	BOOL bResult = cMySQL::ExcuteProcedure(MySQLConnection, parambinder, query, result_set);
	if (bResult == FALSE)
		return FALSE;

	if ( result_set.size() == 0 )
		return FALSE;

	// 데이터 읽기
	NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
	NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
	for (; iter != iter_end; ++iter)
	{
		cMySQLReader* reader = (*iter);
		if (reader == nullptr)
			continue;

		_player.set_member_level(reader->GetLong("level"));
		_player.set_experience_points(reader->GetLongLong("exp"));
		//_player.set_loss_limit(reader->GetLongLong("lost_limit"));
		//_player.set_refresh_time_of_loss_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_loss_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		//_player.set_buy_limit(reader->GetLongLong("buy_limit"));
		//_player.set_refresh_time_of_buy_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_buy_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		_player.set_wallet_chips(reader->GetLongLong("chips"));
		_player.set_wallet_coins(reader->GetLongLong("coin"));
		_playerExt.set_remaining_reel_coin( reader->GetLongLong( "remain_slot_coin" ) );
		_player.set_wallet_gems(reader->GetLongLong("gem"));
		//uint64 test = reader->GetLongLong( "gem" );
		_player.set_kick_ticket_balance(reader->GetLong("kick_ticket_count"));
		_player.set_equipped_avatar_id(reader->GetLong("avatar_id"));
		_player.set_membership_enabled((bool)reader->GetTinyInt("membership_activated"));
		_player.set_membership_expires_at(TimeUtils::MYSQLTimeToString(reader->GetDateTime("membership_expiry_time")));// DateTime 형식을 읽을때 문제가 발생됨
		_player.set_chip_refill_uses(reader->GetLong("chips_refill_count"));
		_player.set_coin_refill_uses(reader->GetLong("coin_refill_count"));
		//_player.set_sub_passwd_activated((bool)reader->GetTinyInt("sub_passwd_activated"));
		//_player.set_sub_passwd(reader->GetString("sub_passwd"));
		_player.set_member_id(reader->GetLongLong("player_idx"));
		_player.set_display_name( reader->GetString("nickname"));
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

		int32 attendance_days = reader->GetLong("attendance_days");
		_player.set_attendance_streak_days( attendance_days );
		//std::string test = reader->GetString( "nickname" );
		//test = reader->GetString( "nickname" );

		_playerExt.set_lowbadugi_daily_chips( reader->GetLongLong( "lowbaduki_today_chip" ) );
		_playerExt.set_lowbadugi_daily_coins( reader->GetLongLong( "lowbaduki_today_coin" ) );
		_playerExt.set_holdem_daily_chips( reader->GetLongLong( "holdem_today_chip" ) );
		_playerExt.set_holdem_daily_coins( reader->GetLongLong( "holdem_today_coin" ) );
		int32 delete_friend_count = reader->GetLong( "delete_friend_count" );
		_playerExt.set_friend_deletes_today( delete_friend_count );
		int32 daily_coin_limit_mail_count = reader->GetLong( "daily_coin_limit_mail_count" );
		_playerExt.set_coin_limit_mail_count( daily_coin_limit_mail_count );

		if ( reader->isDateTime( "ranking_reward_received_date" ) )
		{
			MYSQL_TIME ranking_reward_received_date = reader->GetDateTime( "ranking_reward_received_date" );
			string ranking_reward_received_date_string = TimeUtils::MYSQLTimeToString( ranking_reward_received_date , false );
			_playerExt.set_ranking_reward_claimed_date( ranking_reward_received_date_string );
		}
	}

	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );

	return TRUE;
}


BOOL QueryManager::GetPlayerCoin( const uint64& _player_idx , General::ParticipantProfile& _player , Server::ParticipantProfileInternal& _playerExt )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	std::string query = std::format( "SELECT * FROM players WHERE player_idx = '{}';" , _player_idx );


	// 파람바인더에 파라미터를 추가해준다.
	cParamBinder parambinder( 1 );
	parambinder.BindParam( 1 , MYSQL_TYPE_LONG , _player_idx );

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

		//_player.set_member_level( reader->GetLong( "level" ) );
		//_player.set_experience_points( reader->GetLongLong( "exp" ) );
		//_player.set_loss_limit(reader->GetLongLong("lost_limit"));
		//_player.set_refresh_time_of_loss_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_loss_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		//_player.set_buy_limit(reader->GetLongLong("buy_limit"));
		//_player.set_refresh_time_of_buy_limit(TimeUtils::MYSQLTimeToString(reader->GetDateTime("refresh_time_of_buy_limit")));// DateTime 형식을 읽을때 문제가 발생됨
		//_player.set_wallet_chips( reader->GetLongLong( "chips" ) );
		_player.set_wallet_coins( reader->GetLongLong( "coin" ) );
		_playerExt.set_remaining_reel_coin( reader->GetLongLong( "remain_slot_coin" ) );
		//_player.set_wallet_gems( reader->GetLongLong( "gem" ) );
		//uint64 test = reader->GetLongLong( "gem" );
		//_player.set_kick_ticket_balance( reader->GetLong( "kick_ticket_count" ) );
		//_player.set_equipped_avatar_id( reader->GetLong( "avatar_id" ) );
		//_player.set_membership_enabled( ( bool ) reader->GetTinyInt( "membership_activated" ) );
		//_player.set_membership_expires_at( TimeUtils::MYSQLTimeToString( reader->GetDateTime( "membership_expiry_time" ) ) );// DateTime 형식을 읽을때 문제가 발생됨
		//_player.set_chip_refill_uses( reader->GetLong( "chips_refill_count" ) );
		//_player.set_coin_refill_uses( reader->GetLong( "coin_refill_count" ) );
		//_player.set_sub_passwd_activated((bool)reader->GetTinyInt("sub_passwd_activated"));
		//_player.set_sub_passwd(reader->GetString("sub_passwd"));
		//_player.set_member_id( reader->GetLongLong( "player_idx" ) );
		//_player.set_display_name( reader->GetString( "nickname" ) );
		//_player.set_vault_chips( reader->GetLongLong( "safe_chips" ) );
		//_player.set_safe_paid_chips( reader->GetLongLong( "safe_paid_chips" ) );
		//_player.set_vault_coins( reader->GetLongLong( "safe_coin" ) );
		//_player.set_blackjack_win_streak( reader->GetLong( "blackjack_straight_wins" ) );
		_player.set_rakeback_balance( reader->GetLongLong( "rakeback" ) );

		//_player.set_paid_chips( reader->GetLongLong( "paid_chips" ) );
		//_player.set_paid_coin( reader->GetLongLong( "paid_coin" ) );
		//_player.set_paid_gems( reader->GetLongLong( "paid_gem" ) );

		// 멤버쉽 클라스 읽어오기.
		//int member_ship_number = reader->GetLong( "member_ship_class" );
		//string membership_string = reader->GetString("member_ship_class_string");
		//const General::BenefitTier member_ship_class = static_cast< General::BenefitTier >( member_ship_number );
		//_player.set_membership_tier( member_ship_class );

		// 갱신 시간 읽어오기
		//MYSQL_TIME daily = reader->GetDateTime( "daily_refresh_time" );
		////std::tm daily_time = TimeUtils::MysqlTimeToTM( daily );
		//string daily_string = TimeUtils::MYSQLTimeToString( daily );
		//_player.set_daily_reset_at( daily_string );
		//
		//MYSQL_TIME weekly = reader->GetDateTime( "weekly_refresh_time" );
		////std::tm weekly_time = TimeUtils::MysqlTimeToTM( weekly );
		//string weekly_string = TimeUtils::MYSQLTimeToString( weekly );
		//_player.set_weekly_reset_at( weekly_string );
		//
		//MYSQL_TIME monthly = reader->GetDateTime( "monthly_refresh_time" );
		////std::tm monthly_time = TimeUtils::MysqlTimeToTM( monthly );
		//string monthly_string = TimeUtils::MYSQLTimeToString( monthly );
		//_player.set_monthly_reset_at( monthly_string );
		//
		//MYSQL_TIME free_coin = reader->GetDateTime( "refresh_time_coin_free_charge" );
		////std::tm free_coin_time = TimeUtils::MysqlTimeToTM( free_coin );
		//string free_coin_string = TimeUtils::MYSQLTimeToString( free_coin );
		//_player.set_coin_free_charge_reset_at( free_coin_string );
		//
		//MYSQL_TIME free_chip = reader->GetDateTime( "refresh_time_chip_free_charge" );
		////std::tm free_chip_time = TimeUtils::MysqlTimeToTM( free_chip );
		//string free_chip_string = TimeUtils::MYSQLTimeToString( free_chip );
		//_player.set_chip_free_charge_reset_at( free_chip_string );
		//
		//_player.set_nickname_changed_once( ( bool ) reader->GetTinyInt( "is_changed_nickname" ) );
		//
		//MYSQL_TIME nick_expiry = reader->GetDateTime( "nickname_change_prohibite_expiry_time" );
		//string nick_expiry_string = TimeUtils::MYSQLTimeToString( nick_expiry );
		//_player.set_nickname_change_locked_until( nick_expiry_string );
		//
		//int32 attendance_days = reader->GetLong( "attendance_days" );
		//_player.set_attendance_streak_days( attendance_days );
		////std::string test = reader->GetString( "nickname" );
		////test = reader->GetString( "nickname" );
		//
		//_player.set_lowbadugi_daily_chips( reader->GetLongLong( "lowbaduki_today_chip" ) );
		//_player.set_lowbadugi_daily_coins( reader->GetLongLong( "lowbaduki_today_coin" ) );
		//_player.set_holdem_daily_chips( reader->GetLongLong( "holdem_today_chip" ) );
		//_player.set_holdem_daily_coins( reader->GetLongLong( "holdem_today_coin" ) );
		//int32 delete_friend_count = reader->GetLong( "delete_friend_count" );
		//_player.set_friend_deletes_today( delete_friend_count );
		int32 daily_coin_limit_mail_count = reader->GetLong( "daily_coin_limit_mail_count" );
		_playerExt.set_coin_limit_mail_count( daily_coin_limit_mail_count );
	}

	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	mysql_stmt_close( stmt );

	return TRUE;
}

//General::ParticipantProfile QueryManager::ParsePlayer(cMySQLReader& reader)
//{
//	General::ParticipantProfile _player;
//	_player.set_member_level(reader.GetLong("level"));
//	_player.set_experience_points(reader.GetLongLong("exp"));
//	//_player.set_loss_limit(reader.GetLongLong("lost_limit"));
//	//_player.set_refresh_time_of_loss_limit(reader.GetString("refresh_time_of_loss_limit"));
//	//_player.set_buy_limit(reader.GetLongLong("buy_limit"));
//	//_player.set_refresh_time_of_buy_limit(reader.GetString("refresh_time_of_buy_limit"));
//	_player.set_wallet_chips(reader.GetLongLong("chips"));
//	_player.set_wallet_coins(reader.GetLongLong("coin"));
//	_player.set_remaining_reel_coin( reader.GetLongLong( "remain_slot_coin" ) );
//	_player.set_wallet_gems(reader.GetLongLong("gem"));
//	_player.set_kick_ticket_balance(reader.GetLong("kick_ticket_count"));
//	_player.set_equipped_avatar_id(reader.GetLong("avatar_id"));
//	_player.set_membership_enabled((bool)reader.GetTinyInt("membership_activated"));
//	_player.set_membership_expires_at(reader.GetString("membership_expiry_time"));
//	_player.set_chip_refill_uses(reader.GetLong("chips_refill_count"));
//	_player.set_coin_refill_uses(reader.GetLong("coin_refill_count"));
//	//_player.set_sub_passwd_activated((bool)reader.GetTinyInt("sub_passwd_activated"));
//	//_player.set_sub_passwd(reader.GetString("sub_passwd"));
//	_player.set_member_id(reader.GetLongLong("player_idx"));
//
//	//_player.set_paid_chips( reader.GetLongLong( "paid_chips" ) );
//	//_player.set_safe_paid_chips( reader.GetLongLong( "safe_paid_chips" ) );
//	//_player.set_paid_coin( reader.GetLongLong( "paid_coin" ) );
//	_player.set_paid_gems( reader.GetLongLong( "paid_gem" ) );
//
//	// 갱신 시간 읽어오기
//	MYSQL_TIME daily = reader.GetDateTime( "daily_refresh_time" );
//	//std::tm daily_time = TimeUtils::MysqlTimeToTM( daily );
//	string daily_string = TimeUtils::MYSQLTimeToString( daily );
//	_player.set_daily_reset_at( daily_string );
//
//	MYSQL_TIME weekly = reader.GetDateTime( "weekly_refresh_time" );
//	//std::tm weekly_time = TimeUtils::MysqlTimeToTM( weekly );
//	string weekly_string = TimeUtils::MYSQLTimeToString( weekly );
//	_player.set_weekly_reset_at( weekly_string );
//
//	MYSQL_TIME monthly = reader.GetDateTime( "monthly_refresh_time" );
//	//std::tm monthly_time = TimeUtils::MysqlTimeToTM( monthly );
//	string monthly_string = TimeUtils::MYSQLTimeToString( monthly );
//	_player.set_monthly_reset_at( monthly_string );
//
//	_player.set_nickname_changed_once( ( bool ) reader.GetTinyInt( "is_changed_nickname" ) );
//
//	MYSQL_TIME nick_expiry = reader.GetDateTime( "nickname_change_prohibite_expiry_time" );
//	string nick_expiry_string = TimeUtils::MYSQLTimeToString( nick_expiry );
//	_player.set_nickname_change_locked_until( nick_expiry_string );
//
//	int32 attendance_days = reader.GetLong( "attendance_days" );
//	_player.set_attendance_streak_days( attendance_days );
//
//	return _player;
//}


BOOL QueryManager::PlayerIdxSearchByNickname( string _nack_name , PmNet::MidLookupByAliasRS& response )
{
	BOOL returnValue = FALSE;

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
	{
		// add error code
		return returnValue;
	}

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	MY_CHARSET_INFO charset;
	mysql_get_character_set_info( MySQLConnection , &charset );

	//한글사용을위해추가.
	mysql_query( MySQLConnection , "set session character_set_connection=euckr;" );
	mysql_query( MySQLConnection , "set session character_set_results=euckr;" );
	mysql_query( MySQLConnection , "set session character_set_client=euckr;" );


	// Select 쿼리문
	std::string query = "SELECT player_idx, nickname FROM players WHERE nickname LIKE '" + _nack_name + "%';";
	query_stat = mysql_query( MySQLConnection , query.c_str() );
	if ( query_stat != 0 )
	{
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return 1;
	}

	// 결과출력
	PmNet::TesterMemberDetail user_info;

	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL )
	{
		//printf( "%2s %2s %s\n" , sql_row[ 0 ] , sql_row[ 1 ]);
		printf( "%s %s\n" , sql_row[ 0 ] , sql_row[ 1 ] );

		user_info.Clear();
		user_info.set_member_idx( _atoi64(sql_row[ 0 ]) );
		user_info.set_alias_label( sql_row[ 1 ] );

		auto add_user_info = response.add_tester_infos();
		add_user_info->CopyFrom( user_info );
	}
	mysql_free_result( sql_result );

	return returnValue;
}

std::string QueryManager::GenerateDailyRefreshTimeUpdateQuery( const uint64& _player_idx , const std::string& _daily_refresh_time )
{
	// 업데이트 쿼리 생성
	std::string query = "UPDATE players SET daily_refresh_time = '" + _daily_refresh_time + "' "
		//"weekly_refresh_time = '" + std::string( currentTimeStr ) + "', "
		//"monthly_refresh_time = '" + std::string( currentTimeStr ) + "' "
		"WHERE player_idx = " + std::to_string( _player_idx );

	return query;
}

BOOL QueryManager::UpdateNickName( const uint64& _player_idx , const std::string& _nick_name )
{
	// 닉네임 길이 검증 (UTF-8 바이트 길이 최대 64바이트)
	if ( _nick_name.empty() || _nick_name.length() > 64 ) {
		return FALSE;
	}
	
	// UTF-8 문자 수 검증 (최대 20자)
	std::wstring w_nick = StringUtil::Utf8ToWide( _nick_name );
	if ( w_nick.length() > 20 ) {
		return FALSE;
	}
	
	// 기존 StringUtil 함수를 활용한 특수문자 검증
	// 알파뉴메릭 문자만 허용하는 경우 (영문, 숫자만)
	std::string alphanumeric_only = StringUtil::RemoveSpecialCharactersAndSpaces( _nick_name );
	
	// 한글이 포함된 경우를 위한 추가 검증
	// 허용된 문자: 영문, 숫자, 한글, 언더스코어, 하이픈, 공백
	bool has_invalid_chars = false;
	for ( size_t i = 0; i < _nick_name.length(); ) {
		unsigned char c = static_cast<unsigned char>( _nick_name[i] );
		
		// ASCII 범위 문자 검증
		if ( c < 128 ) {
			// 영문 대소문자, 숫자, 언더스코어, 하이픈, 공백만 허용
			if ( !( ( c >= 'A' && c <= 'Z' ) || 
					( c >= 'a' && c <= 'z' ) || 
					( c >= '0' && c <= '9' ) || 
					( c == '_' ) || 
					( c == '-' ) ||
					( c == ' ' ) ) ) {
				has_invalid_chars = true;
				break;
			}
			i++;
		} 
		// UTF-8 멀티바이트 문자 (한글 등)
		else if ( ( c & 0xE0 ) == 0xC0 ) {
			// 2바이트 UTF-8
			if ( i + 1 >= _nick_name.length() ) {
				has_invalid_chars = true;
				break;
			}
			i += 2;
		}
		else if ( ( c & 0xF0 ) == 0xE0 ) {
			// 3바이트 UTF-8 (한글)
			if ( i + 2 >= _nick_name.length() ) {
				has_invalid_chars = true;
				break;
			}
			i += 3;
		}
		else if ( ( c & 0xF8 ) == 0xF0 ) {
			// 4바이트 UTF-8
			if ( i + 3 >= _nick_name.length() ) {
				has_invalid_chars = true;
				break;
			}
			i += 4;
		}
		else {
			// 잘못된 UTF-8 바이트
			has_invalid_chars = true;
			break;
		}
	}
	
	if ( has_invalid_chars ) {
		return FALSE;
	}
	
	// 이제 안전한 닉네임이므로 그대로 사용
	std::string safe_nick_name = _nick_name;

	std::string query = "UPDATE players SET nickname = '" + safe_nick_name + "' "
		"WHERE player_idx = " + std::to_string( _player_idx );

	std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
	result.wait();
	return result.get();
}

std::future<BOOL> QueryManager::SelectNickNameAsync( const std::string& _nick_name , std::vector<std::string>& _find_nicks )
{
	return std::async( std::launch::async , [&_nick_name , &_find_nicks]() {
		return QueryManager::SelectNickName( _nick_name , _find_nicks );
	} );
}

BOOL QueryManager::SelectNickName( const std::string& _nick_name , std::vector<std::string>& _find_nicks )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	/*MY_CHARSET_INFO charset;
	mysql_get_character_set_info( MySQLConnection , &charset );*/

	//한글사용을위해추가.
	/*mysql_query( MySQLConnection , "set session character_set_connection=euckr;" );
	mysql_query( MySQLConnection , "set session character_set_results=euckr;" );
	mysql_query( MySQLConnection , "set session character_set_client=euckr;" );*/


	// Select 쿼리문
	std::string executeQuery = "select nickname from players where nickname = '" + _nick_name + "';";
	//std::string executeQuery = std::format( "select * from players where nickname = '{}';" , _nick_name );
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {
		//printf( "%2s %2s %s\n" , sql_row[ 0 ] , sql_row[ 1 ] , sql_row[ 2 ] );
		_find_nicks.push_back( sql_row[ 0 ] );
	}
	mysql_free_result( sql_result );

	return TRUE;
}

std::future<string> QueryManager::FindAccountGuidByNicknameAsync( const std::string& _nick_name )
{
	return std::async( std::launch::async , [ &_nick_name ]() {
		return QueryManager::FindAccountGuidByNickname( _nick_name );
	} );
}

std::string QueryManager::FindAccountGuidByNickname( const std::string& _nick_name )
{
	std::string _account_guid;

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// Select 쿼리문
	std::string executeQuery = "select account_guid from players where nickname = '" + _nick_name + "';";
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {
		_account_guid = sql_row[ 0 ];
	}
	mysql_free_result( sql_result );

	return _account_guid;
}

std::future<BOOL> QueryManager::SelectMADEByCidAsync( const std::string& _cid , std::vector<std::string>& _MADE_ids )
{
	return std::async( std::launch::async , [&_cid , &_MADE_ids]() {
		return QueryManager::SelectMADEByCid( _cid , _MADE_ids );
	} );
}

BOOL QueryManager::SelectMADEByCid( const std::string& _cid , std::vector<std::string>& _MADE_ids )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// Select 쿼리문
	std::string executeQuery = "select A.platform_code , A.tpp_id from user_platform as A join user_account as B ON A.account_guid = B.account_guid where B.name_auth_code = '" + _cid + "' order by A.reg_date asc;";
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {
		//++_find_count;
		int platform_code = atoi( sql_row[ 0 ] );
		General::AccessChannelType _platform_type = static_cast< General::AccessChannelType >( platform_code );
		if(_platform_type == General::AccessChannelType::AccessChannel_LocalAccount )
			_MADE_ids.push_back( sql_row[ 1 ] );
	}
	mysql_free_result( sql_result );

	return TRUE;
}

std::future<BOOL> QueryManager::DeletedByCidAsync( const std::string& _cid , std::vector<std::string>& _platform_guids )
{
	return std::async( std::launch::async , [&_cid , &_platform_guids]() {
		return QueryManager::DeletedByCid( _cid , _platform_guids );
	} );
}

BOOL QueryManager::DeletedByCid( const std::string& _cid , std::vector<std::string>& _platform_guids )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;
	std::string deleteQuery =
		"SELECT A.platform_guid "
		"FROM account.user_platform AS A "
		"JOIN account.user_account AS B "
		"ON A.account_guid = B.account_guid "
		"WHERE B.name_auth_code = '" + _cid + "' "
		"AND A.expected_withdrawal_date < NOW();";
	query_stat = mysql_query( MySQLConnection , deleteQuery.c_str() );

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {
		_platform_guids.push_back( sql_row[ 0 ] );
	}
	mysql_free_result( sql_result );
	return TRUE;
}


std::future<BOOL> QueryManager::SelectByCidAsync( const std::string& _cid , std::vector<std::string>& _MADE_ids )
{
	return std::async( std::launch::async , [&_cid , &_MADE_ids]() {
		return QueryManager::SelectByCid( _cid , _MADE_ids );
	} );
}

BOOL QueryManager::SelectByCid( const std::string& _cid , std::vector<std::string>& _MADE_ids )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;
	std::string deleteQuery =
		"DELETE A, P  FROM account.user_platform AS A JOIN account.user_account AS B ON A.account_guid = B.account_guid LEFT JOIN tpp.players AS P ON A.platform_guid = P.platform_guid WHERE B.name_auth_code = '" + _cid + "' AND A.expected_withdrawal_date < NOW();"; 
	query_stat = mysql_query( MySQLConnection , deleteQuery.c_str() );

	// Select 쿼리문
	std::string executeQuery = "select A.platform_code , A.tpp_id from user_platform as A join user_account as B ON A.account_guid = B.account_guid where B.name_auth_code = '" + _cid + "' order by A.reg_date asc;";
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {
		//++_find_count;
		int platform_code = atoi( sql_row[ 0 ] );
		General::AccessChannelType _platform_type = static_cast< General::AccessChannelType >( platform_code );
		_MADE_ids.push_back( sql_row[ 1 ] );
	}
	mysql_free_result( sql_result );

	return TRUE;
}

std::future<BOOL> QueryManager::SelectMADEIdAsync( const std::string& _MADE_id , std::vector<std::string>& _find_ids )
{
	return std::async( std::launch::async , [&_MADE_id , &_find_ids]() {
		return QueryManager::SelectMADEId( _MADE_id , _find_ids );
	} );
}

BOOL QueryManager::SelectMADEId( const std::string& _MADE_id , std::vector<std::string>& _find_ids )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// Select 쿼리문
	std::string executeQuery = "select tpp_id from user_platform where tpp_id = '" + _MADE_id + "';";
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );
	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {
		_find_ids.push_back( sql_row[ 0 ] );
	}
	mysql_free_result( sql_result );

	return TRUE;
}

std::future<BOOL> QueryManager::SelectAccountSanctionAsync( const std::string& account_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason )
{
	return std::async( std::launch::async , [ &account_guid, &sanction_period_start, &sanction_period_end, &sanction_reason ]() {
		return QueryManager::SelectAccountSanction( account_guid , sanction_period_start , sanction_period_end , sanction_reason );
	} );
}

BOOL QueryManager::SelectAccountSanction( const std::string& account_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// Select 쿼리문
	std::string executeQuery = "select account_idx , sanction_period_start , sanction_period_end , sanction_reason from user_account where account_guid = '" + account_guid + "';";
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );

	if ( sql_result->row_count == 0 ) {
		mysql_free_result( sql_result );
		return FALSE;
	}

	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {

		// VALUE
		char* endPtr;
		uint64 account_idx = std::strtoull( sql_row[ 0 ] , &endPtr , 10 );

		if ( sql_row[ 1 ] )
			sanction_period_start = sql_row[ 1 ];

		if ( sql_row[ 2 ] )
			sanction_period_end = sql_row[ 2 ];

		if ( sql_row[ 3 ] )
			sanction_reason = sql_row[ 3 ];

	}
	mysql_free_result( sql_result );

	return TRUE;
}

std::future<BOOL> QueryManager::SelectPlatformSanctionAsync( const std::string& platform_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason )
{
	return std::async( std::launch::async , [&platform_guid , &sanction_period_start , &sanction_period_end , &sanction_reason]() {
		return QueryManager::SelectPlatformSanction( platform_guid , sanction_period_start , sanction_period_end , sanction_reason );
	} );
}

BOOL QueryManager::SelectPlatformSanction( const std::string& platform_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// Select 쿼리문
	std::string executeQuery = "select platform_idx , sanction_period_start , sanction_period_end , sanction_reason from user_platform where platform_guid = '" + platform_guid + "';";
	query_stat = mysql_query( MySQLConnection , executeQuery.c_str() );
	if ( query_stat != 0 ) {
		fprintf( stderr , "Mysql query error : %s" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	// 결과출력
	sql_result = mysql_store_result( MySQLConnection );

	if ( sql_result->row_count == 0 ) {
		mysql_free_result( sql_result );
		return FALSE;
	}

	while ( ( sql_row = mysql_fetch_row( sql_result ) ) != NULL ) {

		// VALUE
		char* endPtr;
		uint64 platform_idx = std::strtoull( sql_row[ 0 ] , &endPtr , 10 );

		if ( sql_row[ 1 ] )
			sanction_period_start = sql_row[ 1 ];

		if ( sql_row[ 2 ] )
			sanction_period_end = sql_row[ 2 ];

		if ( sql_row[ 3 ] )
			sanction_reason = sql_row[ 3 ];

	}
	mysql_free_result( sql_result );

	return TRUE;
}

BOOL QueryManager::PlatformWithdrawalGame( const std::string account_guid , const std::string platform_guid , const std::string expected_withdrawal_date )
{
	std::string executeQuery;

	if ( expected_withdrawal_date.empty() ) {
		executeQuery = std::format( "update user_platform set expected_withdrawal_date = null where account_guid = '{}' and platform_guid = '{}';" , account_guid , platform_guid );
	}
	else {
		executeQuery = std::format( "update user_platform set expected_withdrawal_date = '{}' where account_guid = '{}' and platform_guid = '{}';" ,
		expected_withdrawal_date , account_guid , platform_guid );
	}

	std::future<BOOL> result = UpdateExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

BOOL QueryManager::PlatformCancleWithdrawalFirstJoin( const std::string account_guid , const std::string platform_guid , const int cancle_withdrawal_first_join )
{
	std::string executeQuery;

	if ( cancle_withdrawal_first_join == 1 ) {
		executeQuery = std::format( "update user_platform set cancle_withdrawal_first_join = 1 where account_guid = '{}' and platform_guid = '{}';" , account_guid , platform_guid );
	}
	else {
		executeQuery = std::format( "update user_platform set cancle_withdrawal_first_join = 0 where account_guid = '{}' and platform_guid = '{}';" , account_guid , platform_guid );
	}

	std::future<BOOL> result = UpdateExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , executeQuery );
	result.wait();
	return result.get();
}

// 트랜잭션 테스트 코드 추가
// 트랜잭션 실패시에 에러 코드 리턴 추가
BOOL QueryManager::TransactionTest(string _account_guid, string _platform_guid, BYTE _platform_code, string _nickname, BYTE _push_token_os, string _push_token, int _db_index, uint64_t& _platform_idx)
{
	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_ACCOUNT);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
		return FALSE;

	try
	{
		// initialize and prepare CALL statement with parameter placeholders
		MYSQL_STMT* stmt = mysql_stmt_init(MySQLConnection);
		if (stmt == nullptr)
		{
			printf("Could not initialize statement\n");
			return FALSE;
		}

		std::string query = "CALL CreateUserPlatform(?, ?, ?, ?, ?, ?, ?, ?)";
		int status = mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()));
		stmt_error(stmt, status);
		if (status)
			return FALSE;

		// 파람바인더에 파라미터를 추가해준다.
		cParamBinder parambinder(8);
		parambinder.BindParam(1, MYSQL_TYPE_STRING, _account_guid);
		parambinder.BindParam(2, MYSQL_TYPE_STRING, _platform_guid);
		parambinder.BindParam(3, MYSQL_TYPE_TINY, _platform_code);
		parambinder.BindParam(4, MYSQL_TYPE_STRING, _nickname);
		parambinder.BindParam(5, MYSQL_TYPE_TINY, _push_token_os);
		parambinder.BindParam(6, MYSQL_TYPE_STRING, _push_token);
		parambinder.BindParam(7, MYSQL_TYPE_LONG, _db_index);
		parambinder.BindParam(8, MYSQL_TYPE_LONGLONG, _platform_idx);

		// bind parameters
		status = mysql_stmt_bind_param(stmt, parambinder.GetBinder());
		stmt_error(stmt, status);
		if (status)
			return FALSE;

		// 트랜잭션 시작
		string error_string = "";
		status = mysql_query(MySQLConnection, "START TRANSACTION");
		stmt_error(stmt, status);
		if (status)
			return FALSE;

		status = mysql_stmt_execute(stmt);
		stmt_error_string(stmt, status, error_string);
		if (status)
		{
			mysql_query(MySQLConnection, "ROLLBACK");  // 실패 시 롤백
			return FALSE;
		}
		else
		{
			mysql_query(MySQLConnection, "COMMIT");  // 성공 시 커밋
		}

		mysql_stmt_close(stmt);

		return TRUE;
	}
	catch (...)
	{
		mysql_query(MySQLConnection, "ROLLBACK");  // 예외 발생 시 롤백
		return FALSE;
	}
}

std::string QueryManager::GenerateFreeChargeUpdateQuery( const uint64& _player_idx , const General::AssetKind& _monty_type , const std::string& _refresh_time )
{
	// 업데이트 쿼리 생성
	std::string query;

	if ( _monty_type == General::AssetKind::AssetKind_Chip ) {

		query = "UPDATE players SET refresh_time_chip_free_charge = '" + _refresh_time + "' "
			"WHERE player_idx = " + std::to_string( _player_idx );
	}
	else if ( _monty_type == General::AssetKind::AssetKind_Coin ) {

		query = "UPDATE players SET refresh_time_coin_free_charge = '" + _refresh_time + "' "
			"WHERE player_idx = " + std::to_string( _player_idx );
	}

	return query;
}

std::future<BOOL> QueryManager::GetPlayerGuidsAsync( const uint64& _player_idx , std::string& _account_guid , std::string& _platform_guid )
{
	return std::async( std::launch::async , [_player_idx , &_account_guid , &_platform_guid]() {
		return QueryManager::GetPlayerGuids( _player_idx , _account_guid , _platform_guid );
	} );
}

BOOL QueryManager::GetPlayerGuids( const uint64& _player_idx , std::string& _account_guid , std::string& _platform_guid )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select account_guid, platform_guid from players where player_idx = {};" , _player_idx );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
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

			_account_guid = reader->GetString( "account_guid" );
			_platform_guid = reader->GetString( "platform_guid" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf( e.what() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetPlayerPlatformAsync( const std::string& _account_guid , const std::string& _platform_guid , int& platform_code )
{
	return std::async( std::launch::async , [_account_guid , _platform_guid , &platform_code]() {
		return QueryManager::GetPlayerPlatform( _account_guid , _platform_guid , platform_code );
	} );
}

BOOL QueryManager::GetPlayerPlatform( const std::string& _account_guid , const std::string& _platform_guid , int& platform_code )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	platform_code = 0;

	std::string executeQuery = std::format( "select platform_code from user_platform where account_guid = '{}' and platform_guid = '{}';" , _account_guid , _platform_guid );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
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

			platform_code = reader->GetTinyInt( "platform_code" );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf( e.what() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetPlayerJoinTimeAsync( const std::string& _account_guid , const std::string& _platform_guid , string& _join_time )
{
	return std::async( std::launch::async , [_account_guid , _platform_guid , &_join_time]() {
		return QueryManager::GetPlayerJoinTime( _account_guid , _platform_guid , _join_time );
	} );
}

BOOL QueryManager::GetPlayerJoinTime( const std::string& _account_guid , const std::string& _platform_guid , string& _join_time )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	_join_time = "";

	std::string executeQuery = std::format( "select reg_date from user_platform where account_guid = '{}' and platform_guid = '{}';" , _account_guid , _platform_guid );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
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

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			_join_time = TimeUtils::MYSQLTimeToString( reg_date );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		//printf( e.what() );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

BOOL QueryManager::UpdateLostLimit( General::LossLimitProfile* lost_limit )
{
	std::string query = GenerateUpdateLostLimit( lost_limit );
	std::future<BOOL> result;
	result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , query );
	result.wait();
	return result.get();
}

std::string QueryManager::GenerateUpdateLostLimit( General::LossLimitProfile* lost_limit )
{
	std::string query = "UPDATE user_account SET lost_limit = " + std::to_string( lost_limit->loss_limit_amount() ) + ", " +
		"next_lost_limit = " + std::to_string( lost_limit->pending_loss_limit() ) + ", " +
		"lost_limit_change_count = " + std::to_string( lost_limit->loss_limit_changes() ) + ", " +
		"daily_lost_chip = " + std::to_string( lost_limit->daily_chip_loss() ) + ", " +
		"refresh_time_of_loss_limit = '" + lost_limit->loss_limit_reset_at() + "', " +
		// "buy_limit = " + std::to_string( lost_limit->purchase_limit_amount() ) + ", " + // buy_limit 은 갱신할 일이 없음
		"monthly_buy_total = " + std::to_string( lost_limit->monthly_purchase_total() ) + ", " +
		"refresh_time_of_buy_limit = '" + lost_limit->purchase_limit_reset_at() + "', " +
		"set_time_limit = " + std::to_string( lost_limit->play_block_hours() ) + ", " +
		"next_set_time_limit = " + std::to_string( lost_limit->pending_play_block_hours() ) + " " +
		"WHERE account_idx = " + std::to_string( lost_limit->account_id() ) + ";";
	return query;
}

BOOL QueryManager::UpdateAttendance( const uint64& _player_idx , const int& _days )
{
	std::string query = GenerateUpdateAttendance( _player_idx , _days );

	std::future<BOOL> result;
	result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
	result.wait();
	return result.get();
}

std::string QueryManager::GenerateUpdateAttendance( const uint64& _player_idx , const int& _days )
{
	std::string query = "UPDATE players SET attendance_days = " + std::to_string( _days ) + " " +
		"WHERE player_idx = " + std::to_string( _player_idx ) + ";";
	return query;
}

BOOL QueryManager::GetLoginRewardByUserID( const int& user_id , std::set<string>& receive_list)
{

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_LOG);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) { return FALSE; }

	std::string executeQuery = std::format( "SELECT event_name FROM login_reward_logs WHERE user_id = '{}';" , user_id );

	try {
		NetLib::cVector<cMySQLReader*> result_set;
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) )
			return FALSE;

		if ( result_set.size() == 0 )
			return TRUE;
		// 데이터 읽기
		for ( const auto& reader : result_set ) {
			if ( reader == nullptr )
				continue;

			std::string event_id = reader->GetString( "event_name" );
			receive_list.insert( event_id );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
	}

	return TRUE;
}

BOOL QueryManager::GetLoginRewardList( std::vector<PmNet::DropDetail>& event_list ) {
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT ); // ??
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) {
		return FALSE;
	}

	// 현재 시간 가져오기
	time_t now = time( nullptr );
	struct tm time_info;
	localtime_s( &time_info , &now );
	char current_time[ 20 ];
	sprintf_s( current_time , sizeof( current_time ) , "%04d-%02d-%02d %02d:%02d" ,
			  1900 + time_info.tm_year , 1 + time_info.tm_mon , time_info.tm_mday , time_info.tm_hour , time_info.tm_min );

	std::string executeQuery = "SELECT * FROM login_rewards WHERE end_time >= '" + std::string( current_time ) + "'";

	PmNet::DropDetail eventinfo;
	NetLib::cVector<cMySQLReader*> result_set;

	try {
		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) ) {
			return FALSE;
		}

		if ( result_set.size() == 0 ) {
			return TRUE;
		}

		// 데이터 읽기
		for ( const auto& reader : result_set ) {
			if ( reader == nullptr ) {
				continue;
			}
			eventinfo.Clear();

			eventinfo.set_drop_name( reader->GetString( "event_name" ) );
			MYSQL_TIME start_time = reader->GetDateTime( "start_time" );
			MYSQL_TIME end_time = reader->GetDateTime( "end_time" );
			eventinfo.set_start_ts( cMySQL::MySQLTimeToString( start_time ) );
			eventinfo.set_end_ts( cMySQL::MySQLTimeToString( end_time ) );

			eventinfo.set_bounty( reader->GetString( "reward" ) );
			// Debug: Try original message first
			std::string rawMessage = reader->GetString( "message" );
			
			eventinfo.set_notice( rawMessage );
			event_list.push_back( eventinfo );
		}

		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const std::runtime_error& e ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
		return FALSE;
	}
	catch ( ... ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Unknown exception occurred" );
		return FALSE;
	}

	// 리소스 반환
	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );

	return TRUE;
}

std::future<BOOL> QueryManager::GetLoginRewardListAsync(std::vector<PmNet::DropDetail>& event_list )
{
	return std::async( std::launch::async , [&event_list]() {
		return QueryManager::GetLoginRewardList( event_list );
	} );
}

std::future<BOOL> QueryManager::GetLoginRewardByUserIDAsync( const int& user_id , std::set<string>& receive_list )
{
	return std::async( std::launch::async , [user_id ,  &receive_list]() {
		return QueryManager::GetLoginRewardByUserID( user_id , receive_list );
	} );
}

BOOL QueryManager::InsertLoginRewardLog( const uint64& user_id , const std::string& event_name , const std::string& reward , std::string& mail_index ) {
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_LOG );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) {
		return FALSE;
	}

	// 현재 시간 가져오기
	time_t now = time( nullptr );
	struct tm time_info;
	localtime_s( &time_info , &now );
	char reward_time[ 20 ];
	sprintf_s( reward_time , sizeof( reward_time ) , "%04d-%02d-%02d %02d:%02d:%02d" ,
			  1900 + time_info.tm_year , 1 + time_info.tm_mon , time_info.tm_mday ,
			  time_info.tm_hour , time_info.tm_min , time_info.tm_sec );

	// SQL 쿼리 작성
	std::string query = "INSERT INTO login_reward_logs (user_id, event_name, reward_time, reward, mail_index) VALUES (" +
		std::to_string( user_id ) + ", '" + event_name + "', '" + reward_time + "', '" + reward + "', '" + mail_index + "')";

	// 쿼리 실행
	if ( mysql_query( MySQLConnection , query.c_str() ) ) {
		fprintf( stderr , "InsertLoginRewardLog failed: %s\n" , mysql_error( MySQLConnection ) );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetSlotRewardEventAsync( std::vector<PmNet::ReelBountyDrop>& event_list )
{
	return std::async( std::launch::async , [&event_list]() {
		return QueryManager::GetSlotRewardEventList( event_list );
	} );
}

BOOL QueryManager::GetSlotRewardEventList( std::vector<PmNet::ReelBountyDrop>& event_list ) {
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) {
		return FALSE;
	}

	std::string executeQuery = "SELECT * FROM slot_event WHERE end_time >= NOW() AND start_time<= NOW()";

	PmNet::ReelBountyDrop eventinfo;
	NetLib::cVector<cMySQLReader*> result_set;

	try {

		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) ) {
			return FALSE;
		}

		event_list.clear();

		if ( result_set.size() == 0 ) {
			return TRUE;
		}
		// 데이터 읽기
		for ( const auto& reader : result_set ) {

			if ( reader == nullptr ) {
				continue;
			}
			eventinfo.Clear();

			eventinfo.set_drop_code( reader->GetString( "event_code" ) );
			MYSQL_TIME start_time = reader->GetDateTime( "start_time" );
			MYSQL_TIME end_time = reader->GetDateTime( "end_time" );
			eventinfo.set_start_ts( cMySQL::MySQLTimeToString( start_time ) );
			eventinfo.set_end_ts( cMySQL::MySQLTimeToString( end_time ) );
			//eventinfo.set_drop_tab_kind( (General::SlotEventTab )( reader->GetLong( "event_type" ) ));
			std::string t_reward = StringUtil::JsonToString( reader->GetJson( "reward" ) );
			eventinfo.set_bounty( t_reward );
			event_list.push_back( eventinfo );
		}

		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const std::runtime_error& e ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
		return FALSE;
	}
	catch ( ... ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Unknown exception occurred" );
		return FALSE;
	}

	// 리소스 반환
	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );

	return TRUE;
}
std::future<BOOL> QueryManager::GetSlotLastRewardEventAsync( std::vector<PmNet::ReelBountyDrop>& event_list )
{
	return std::async( std::launch::async , [&event_list]() {
		return QueryManager::GetSlotLastRewardEventList( event_list );
	} );
}

BOOL QueryManager::GetSlotLastRewardEventList( std::vector<PmNet::ReelBountyDrop>& event_list ) {
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr ) {
		return FALSE;
	}

	std::string executeQuery = "SELECT * FROM slot_event WHERE end_time < NOW() ORDER BY end_time DESC LIMIT 1";

	PmNet::ReelBountyDrop eventinfo;
	NetLib::cVector<cMySQLReader*> result_set;

	try {

		if ( FALSE == cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set ) ) {
			return FALSE;
		}

		event_list.clear();

		if ( result_set.size() == 0 ) {
			return TRUE;
		}
		// 데이터 읽기
		for ( const auto& reader : result_set ) {

			if ( reader == nullptr ) {
				continue;
			}
			eventinfo.Clear();

			eventinfo.set_drop_code( reader->GetString( "event_code" ) );
			MYSQL_TIME start_time = reader->GetDateTime( "start_time" );
			MYSQL_TIME end_time = reader->GetDateTime( "end_time" );
			eventinfo.set_start_ts( cMySQL::MySQLTimeToString( start_time ) );
			eventinfo.set_end_ts( cMySQL::MySQLTimeToString( end_time ) );
			//eventinfo.set_drop_tab_kind( ( General::SlotEventTab ) ( reader->GetLong( "event_type" ) ) );
			json j_reward = reader->GetJson( "reward" );
			std::string t_reward = StringUtil::JsonToString( j_reward );
			eventinfo.set_bounty( t_reward );
			event_list.push_back( eventinfo );
		}

		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const std::runtime_error& e ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
		return FALSE;
	}
	catch ( ... ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Unknown exception occurred" );
		return FALSE;
	}

	// 리소스 반환
	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );

	return TRUE;
}

std::future<BOOL> QueryManager::GetCheatListAsync( std::set<UINT64>& CList )
{
	return std::async( std::launch::async , [&CList]() {
		return QueryManager::GetCheatList( CList );
	} );
}

BOOL QueryManager::GetCheatList( std::set<UINT64>& CList )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "SELECT * FROM account.whitelist;" );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
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
			CList.insert( reader->GetLongLong( "idwhitelist" ) );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetBanListAsync( std::set<UINT64>& BList )
{
	return std::async( std::launch::async , [&BList]() {
		return QueryManager::GetBanList( BList );
	} );
}

BOOL QueryManager::GetBanList( std::set<UINT64>& BList )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "SELECT * FROM account.ban_list;" );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
		if ( result_set.size() == 0 )
			return FALSE;

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		BList.clear();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;
			BList.insert( reader->GetLongLong( "idban_list" ) );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetMaintenanceMessageAsync( Server::MaintenanceMessage& message )
{
	return std::async( std::launch::async , [&message]() {
		return QueryManager::GetMaintenanceMessage( message );
	} );
}

BOOL QueryManager::GetMaintenanceMessage( Server::MaintenanceMessage& message )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select * from maintenance where notice_type = 3 and start_date <= now() AND now() <= end_date order by maintenance_idx asc limit 1;");

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
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

			message.set_maintenance_idx( reader->GetLongLong( "maintenance_idx" ) );
			message.set_title( reader->GetString( "title" ) );
			message.set_message( reader->GetString( "message" ) );
			message.set_version_min( reader->GetString( "version_min" ) );
			message.set_version_max( reader->GetString( "version_max" ) );
			message.set_play_store( reader->GetTinyInt( "play_store" ) );
			message.set_app_store( reader->GetTinyInt( "app_store" ) );
			message.set_one_store( reader->GetTinyInt( "one_store" ) );
			message.set_pc( reader->GetTinyInt( "pc" ) );
			message.set_web_url( reader->GetString( "web_url" ) );
			message.set_img_file_name( reader->GetString( "img_file_name" ) );
			message.set_white_list( reader->GetString( "white_list" ) );

			MYSQL_TIME start_date = reader->GetDateTime( "start_date" );
			std::string start_date_string = TimeUtils::MYSQLTimeToString( start_date );
			message.set_start_date( start_date_string );

			MYSQL_TIME end_date = reader->GetDateTime( "end_date" );
			std::string end_date_string = TimeUtils::MYSQLTimeToString( end_date );
			message.set_end_date( end_date_string );

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			std::string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );
			message.set_reg_date( reg_date_string );
			
			/*message.set_noticetype( reader->GetTinyInt( "notice_type" ) );
			message.set_noticetype( reader->GetTinyInt( "exposure_type" ) );
			message.set_noticetype( reader->GetTinyInt( "exposure_option " ) );*/
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetMaintenanceSystemMessageAsync( Server::MaintenanceMessage& message )
{
	return std::async( std::launch::async , [&message]() {
		return QueryManager::GetMaintenanceSystemMessage( message );
	} );
}

BOOL QueryManager::GetMaintenanceForcedMessage( Server::MaintenanceMessage& message )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select * from maintenance where notice_type = 4 and start_date <= now() AND now() <= end_date order by maintenance_idx asc limit 1;" );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
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

			message.set_maintenance_idx( reader->GetLongLong( "maintenance_idx" ) );
			message.set_title( reader->GetString( "title" ) );
			message.set_message( reader->GetString( "message" ) );
			message.set_version_min( reader->GetString( "version_min" ) );
			message.set_version_max( reader->GetString( "version_max" ) );
			message.set_play_store( reader->GetTinyInt( "play_store" ) );
			message.set_app_store( reader->GetTinyInt( "app_store" ) );
			message.set_one_store( reader->GetTinyInt( "one_store" ) );
			message.set_pc( reader->GetTinyInt( "pc" ) );
			message.set_web_url( reader->GetString( "web_url" ) );
			message.set_img_file_name( reader->GetString( "img_file_name" ) );
			message.set_white_list( reader->GetString( "white_list" ) );

			MYSQL_TIME start_date = reader->GetDateTime( "start_date" );
			std::string start_date_string = TimeUtils::MYSQLTimeToString( start_date );
			message.set_start_date( start_date_string );

			MYSQL_TIME end_date = reader->GetDateTime( "end_date" );
			std::string end_date_string = TimeUtils::MYSQLTimeToString( end_date );
			message.set_end_date( end_date_string );

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			std::string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );
			message.set_reg_date( reg_date_string );

			/*message.set_noticetype( reader->GetTinyInt( "notice_type" ) );
			message.set_noticetype( reader->GetTinyInt( "exposure_type" ) );
			message.set_noticetype( reader->GetTinyInt( "exposure_option " ) );*/
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetMaintenanceForcedMessageAsync( Server::MaintenanceMessage& message )
{
	return std::async( std::launch::async , [&message]() {
		return QueryManager::GetMaintenanceForcedMessage( message );
	} );
}

BOOL QueryManager::GetMaintenanceSystemMessage( Server::MaintenanceMessage& message )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select * from maintenance where notice_type = 2 and start_date <= now() AND now() <= end_date order by maintenance_idx asc limit 1;" );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
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

			message.set_maintenance_idx( reader->GetLongLong( "maintenance_idx" ) );
			message.set_title( reader->GetString( "title" ) );
			message.set_message( reader->GetString( "message" ) );
			message.set_version_min( reader->GetString( "version_min" ) );
			message.set_version_max( reader->GetString( "version_max" ) );
			message.set_play_store( reader->GetTinyInt( "play_store" ) );
			message.set_app_store( reader->GetTinyInt( "app_store" ) );
			message.set_one_store( reader->GetTinyInt( "one_store" ) );
			message.set_pc( reader->GetTinyInt( "pc" ) );
			message.set_web_url( reader->GetString( "web_url" ) );
			message.set_img_file_name( reader->GetString( "img_file_name" ) );
			message.set_white_list( reader->GetString( "white_list" ) );

			MYSQL_TIME start_date = reader->GetDateTime( "start_date" );
			std::string start_date_string = TimeUtils::MYSQLTimeToString( start_date );
			message.set_start_date( start_date_string );

			MYSQL_TIME end_date = reader->GetDateTime( "end_date" );
			std::string end_date_string = TimeUtils::MYSQLTimeToString( end_date );
			message.set_end_date( end_date_string );

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			std::string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );
			message.set_reg_date( reg_date_string );

			/*message.set_noticetype( reader->GetTinyInt( "notice_type" ) );
			message.set_noticetype( reader->GetTinyInt( "exposure_type" ) );
			message.set_noticetype( reader->GetTinyInt( "exposure_option " ) );*/
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetMaintenanceImageMessageAsync( Server::MaintenanceMessage& message )
{
	return std::async( std::launch::async , [&message]() {
		return QueryManager::GetMaintenanceImageMessage( message );
	} );
}

BOOL QueryManager::GetMaintenanceImageMessage( Server::MaintenanceMessage& message )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select * from maintenance where notice_type = 5 and img_file_name is not null and img_file_name != '' and start_date <= now() AND now() <= end_date order by maintenance_idx asc limit 1;" );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
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

			message.set_maintenance_idx( reader->GetLongLong( "maintenance_idx" ) );
			message.set_title( reader->GetString( "title" ) );
			message.set_message( reader->GetString( "message" ) );
			message.set_version_min( reader->GetString( "version_min" ) );
			message.set_version_max( reader->GetString( "version_max" ) );
			message.set_play_store( reader->GetTinyInt( "play_store" ) );
			message.set_app_store( reader->GetTinyInt( "app_store" ) );
			message.set_one_store( reader->GetTinyInt( "one_store" ) );
			message.set_pc( reader->GetTinyInt( "pc" ) );
			message.set_web_url( reader->GetString( "web_url" ) );
			message.set_img_file_name( reader->GetString( "img_file_name" ) );
			message.set_white_list( reader->GetString( "white_list" ) );

			MYSQL_TIME start_date = reader->GetDateTime( "start_date" );
			std::string start_date_string = TimeUtils::MYSQLTimeToString( start_date );
			message.set_start_date( start_date_string );

			MYSQL_TIME end_date = reader->GetDateTime( "end_date" );
			std::string end_date_string = TimeUtils::MYSQLTimeToString( end_date );
			message.set_end_date( end_date_string );

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			std::string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );
			message.set_reg_date( reg_date_string );

			break;
		}

		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetMaintenanceImageMessagesAsync( std::vector<Server::MaintenanceMessage>& messages )
{
	return std::async( std::launch::async , [&]() {
		return QueryManager::GetMaintenanceImageMessages( messages );
	} );
}

BOOL QueryManager::GetMaintenanceImageMessages( std::vector<Server::MaintenanceMessage>& messages )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format( "select * from maintenance where notice_type = 5 and img_file_name is not null and img_file_name != '' and start_date <= now() AND now() <= end_date order by maintenance_idx asc;" );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
		if ( result_set.size() == 0 )
			return FALSE;

		// 모든 이미지 메시지 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			Server::MaintenanceMessage message;
			message.set_maintenance_idx( reader->GetLongLong( "maintenance_idx" ) );
			message.set_title( reader->GetString( "title" ) );
			message.set_message( reader->GetString( "message" ) );
			message.set_version_min( reader->GetString( "version_min" ) );
			message.set_version_max( reader->GetString( "version_max" ) );
			message.set_play_store( reader->GetTinyInt( "play_store" ) );
			message.set_app_store( reader->GetTinyInt( "app_store" ) );
			message.set_one_store( reader->GetTinyInt( "one_store" ) );
			message.set_pc( reader->GetTinyInt( "pc" ) );
			message.set_web_url( reader->GetString( "web_url" ) );
			message.set_img_file_name( reader->GetString( "img_file_name" ) );
			message.set_white_list( reader->GetString( "white_list" ) );

			MYSQL_TIME start_date = reader->GetDateTime( "start_date" );
			std::string start_date_string = TimeUtils::MYSQLTimeToString( start_date );
			message.set_start_date( start_date_string );

			MYSQL_TIME end_date = reader->GetDateTime( "end_date" );
			std::string end_date_string = TimeUtils::MYSQLTimeToString( end_date );
			message.set_end_date( end_date_string );

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			std::string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );
			message.set_reg_date( reg_date_string );

			messages.push_back( message );
		}

		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

BOOL QueryManager::GetNoticeMessageInfo( std::vector<PmNet::BulletinDetail>& notices )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = "SELECT * FROM maintenance WHERE notice_type = '1' AND start_date <= NOW() AND end_date > NOW();";

	PmNet::BulletinDetail notice;

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
			notice.set_bulletin_idx( reader->GetLongLong( "maintenance_idx" ) );
			notice.set_headline( reader->GetString( "title" ) );
			notice.set_notice( reader->GetString( "message" ) );
			notice.set_min_ver( reader->GetString( "version_min" ) );
			notice.set_max_ver( reader->GetString( "version_max" ) );
			notice.set_ps_avail( reader->GetTinyInt( "play_store" ) );
			notice.set_as_avail( reader->GetTinyInt( "app_store" ) );
			notice.set_os_avail( reader->GetTinyInt( "one_store" ) );
			notice.set_pc_avail( reader->GetTinyInt( "pc" ) );
			notice.set_link_url( reader->GetString( "web_url" ) );
			notice.set_img_url( reader->GetString( "img_file_name" ) );

			MYSQL_TIME start_date = reader->GetDateTime( "start_date" );
			std::string start_date_string = TimeUtils::MYSQLTimeToString( start_date );
			notice.set_start_ts( start_date_string );

			MYSQL_TIME end_date = reader->GetDateTime( "end_date" );
			std::string end_date_string = TimeUtils::MYSQLTimeToString( end_date );
			notice.set_end_ts( end_date_string );
				
			General::AnnouncementKind _notice_type = static_cast< General::AnnouncementKind >( reader->GetTinyInt( "notice_type" ) );
			notice.set_bulletin_kind( _notice_type );
			General::DisplaySurface _exposure_type = static_cast< General::DisplaySurface >( reader->GetTinyInt( "exposure_type" ) );
			notice.set_expose_kind( _exposure_type  );
			General::DisplayPolicy _exposure_option = static_cast< General::DisplayPolicy >( reader->GetTinyInt( "exposure_option" ) );
			notice.set_expose_opt( _exposure_option );

			notices.push_back( notice );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const std::runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}


std::future<BOOL> QueryManager::GetSetVersionsAsync( std::vector<Server::Version>& versions )
{
	return std::async( std::launch::async , [&versions]() {
		return QueryManager::GetSetVersions( versions );
	} );
}

BOOL QueryManager::GetSetVersions( std::vector<Server::Version>& versions )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = "SELECT * FROM set_versions;";

	Server::Version version;

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

			version.set_market( reader->GetTinyInt( "market" ) );
			version.set_title( reader->GetString( "title" ) );
			version.set_message( reader->GetString( "message" ) );
			version.set_version_min( reader->GetString( "version_min" ) );
			version.set_version_latest( reader->GetString( "version_lastest" ) );
			version.set_version_review( reader->GetString( "version_review" ) );

			MYSQL_TIME reg_date = reader->GetDateTime( "reg_date" );
			std::string reg_date_string = TimeUtils::MYSQLTimeToString( reg_date );
			version.set_reg_date( reg_date_string );

			versions.push_back( version );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const std::runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}


std::tuple<std::string, std::string, std::string, std::string, std::string> QueryManager::GetMessageData(const uint64& mail_idx)
{
	std::string reward_type_string;
	std::string mail_type;
	std::string mail_type_string;
	std::string count;
	std::string reg_date;

	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_SHARD);
    MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
    {
        std::cerr << "Failed to get MySQL connection" << std::endl;
		return std::make_tuple(reward_type_string, mail_type, mail_type_string, count, reg_date);
    }

	// Select 쿼리문
	std::string executeQuery = std::format(
        "SELECT reward_type_string, mail_type, mail_type_string, count, reg_date FROM mailbox WHERE mail_idx = '{}';", 
        mail_idx
    );

    if (mysql_query(MySQLConnection, executeQuery.c_str()))
    {
        std::cerr << "MySQL query error: " << mysql_error(MySQLConnection) << std::endl;
    	return std::make_tuple(reward_type_string, mail_type, mail_type_string, count, reg_date);
    }

    MYSQL_RES* result = mysql_store_result(MySQLConnection);
    if (result == nullptr)
    {
        std::cerr << "MySQL store result error: " << mysql_error(MySQLConnection) << std::endl;
    	return std::make_tuple(reward_type_string, mail_type, mail_type_string, count, reg_date);
    }

    MYSQL_ROW row;
	if ((row = mysql_fetch_row(result))) 
	{
        reward_type_string = row[0] ? row[0] : ""; 	// 1 번째 컬럼의 값 (reward_type_string)
        mail_type = row[1] ? row[1] : ""; 			// 2 번째 컬럼의 값 (mail_type)
        mail_type_string = row[2] ? row[2] : ""; 	// 3 번째 컬럼의 값 (mail_type_string)
        count = row[3] ? row[3] : ""; 				// 4 번째 컬럼의 값 (count)
        reg_date = row[4] ? row[4] : ""; 			// 5 번째 컬럼의 값 (reg_date)
    }

    mysql_free_result(result);
    return std::make_tuple(reward_type_string, mail_type, mail_type_string, count, reg_date);
}


std::vector<uint64> QueryManager::GetExpiredMessageList(const uint64& player_idx)
{
    std::vector<uint64> expired_mail_list;

    cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_SHARD);
    MYSQL* MySQLConnection = con_ins.GetConnection();
    if (MySQLConnection == nullptr)
    {
        return expired_mail_list;
    }

    std::string executeQuery = std::format(
        "SELECT mail_idx FROM mailbox WHERE player_idx = '{}' AND expire_date < NOW();",
        player_idx
    );

    try
    {
        NetLib::cVector<cMySQLReader*> result_set;
        cMySQL::ExcuteQuery(MySQLConnection, executeQuery, result_set);

        for (cMySQLReader* reader : result_set)
        {
            if (reader == nullptr)
                continue;

            uint64 mail_idx = reader->GetLongLong("mail_idx");
            expired_mail_list.push_back(mail_idx);
        }
        NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader(result_set);
    }
	catch ( const runtime_error& e )
    {
        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , e.what() );
    }

    return expired_mail_list;
}

::string QueryManager::GetLastMailBoxIndexQuery(
		const uint64_t& player_idx )
{
	string index = "0";
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
	{
		std::cerr << "Failed to get MySQL connection" << std::endl;
		return index;
	}

	// Select 쿼리문
	std::string executeQuery = std::format(
		"SELECT mail_idx FROM mailbox WHERE player_idx = {} ORDER BY mail_idx DESC LIMIT 1;" ,
		player_idx );

	if ( mysql_query( MySQLConnection , executeQuery.c_str() ) )
	{
		std::cerr << "MySQL query error: " << mysql_error( MySQLConnection ) << std::endl;
		return index;
	}

	MYSQL_RES* result = mysql_store_result( MySQLConnection );
	if ( result == nullptr )
	{
		std::cerr << "MySQL store result error: " << mysql_error( MySQLConnection ) << std::endl;
		return index;
	}

	MYSQL_ROW row;
	if ( ( row = mysql_fetch_row( result ) ) )
	{
		index = row[ 0 ]; // 첫 번째 컬럼의 값 (reg_date)
	}

	mysql_free_result( result );
	return index;
}

std::future<BOOL> QueryManager::InsertErrorLog(
		const std::string& _error )
{
	std::ostringstream query;
	query << "INSERT INTO client_error_log (client_error_log) VALUES ("
		<< "'" << _error << "')";

	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertAbusingLog(
		const std::string& _error )
{
	std::ostringstream query;
	query << "INSERT INTO abusing_log (IP) VALUES ("
		<< "'" << _error << "')";

	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::PingSQL()
{
	std::ostringstream query;
	query << "SELECT 1";
	PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_ACCOUNT , query.str() );
	PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query.str() );
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

BOOL QueryManager::GetPlayerIP(const uint64 player_index, std::string& IP)
{
	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_LOG);
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
		return FALSE;

	std::string executeQuery = "SELECT * FROM log.ip_log Where player_idx = " + std::to_string(player_index) + ";";
	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery(MySQLConnection, executeQuery, result_set);
		if (result_set.size() == 0)
			return FALSE;

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for (; iter != iter_end; ++iter)
		{
			cMySQLReader* reader = (*iter);
			if (reader == nullptr)
				continue;

			IP =  reader->GetString("IP");
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader(result_set);
	}
	catch (const runtime_error& e)
	{
		printf(e.what());
		return FALSE;
	}
	catch (const std::exception& e)
	{
		printf(e.what());
		return FALSE;
	}

	return TRUE;
}

std::future<BOOL> QueryManager::GetPlayerIPAsync(const uint64 player_index, std::string& IP)
{
	return std::async(std::launch::async, [player_index  ,&IP]() {
		return QueryManager::GetPlayerIP(player_index,IP);
		});
}

std::future<BOOL> QueryManager::InsertPlayerIPLog(const uint64 player_index, std::string& IP)
{
	std::ostringstream query;
	query << "INSERT INTO log.ip_log (player_idx , IP) VALUES ("
		<< "'" << player_index << "','" << IP<<"')";

	return PlayerExecuteQueryAsync(E_DB_TYPE::E_DB_TYPE_LOG, query.str());
}

std::future<BOOL> QueryManager::GetSystemDataAsync( std::map<std::string , std::string>& data )
{
	return std::async( std::launch::async , [&data]() {
		return QueryManager::GetSystemData( data );
	} );
}

BOOL QueryManager::GetSystemData( std::map<std::string , std::string>& data )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_ACCOUNT );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = "SELECT * FROM account.system_data;";
	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );
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


			std::string data_type = reader->GetString( "data_type" );
			std::string data_value = reader->GetString( "data_value" );

			data[ data_type ] = data_value;
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
		return FALSE;
	}
	catch ( const std::exception& e )
	{
		printf( e.what() );
		return FALSE;
	}

	return TRUE;
}

//std::future<BOOL> QueryManager::InsertSlotEventLog(
//		const int event_type ,
//		const std::string& start_date ,
//		const std::string& end_date ,
//		const int rank ,
//		const uint64 player_index,
//		const uint64 score )
//{
//	std::ostringstream query;
//	query << "INSERT INTO slot_event_log (EVENT_TYPE , EVENT_START_TIME , EVENT_END_TIME , RANK , PLAYER_INDEX , SCORE ) VALUES ("
//		<< event_type << ", "
//		<< "'" << start_date << "', "
//		<< "'" << end_date << "', "
//		<< rank << ", "
//		<< player_index << ", "
//		<< score << "')";
//
//	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
//}
//
//std::future<BOOL> QueryManager::InsertSlotEventLog(
//		std::ostringstream& query )
//{
//	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
//}