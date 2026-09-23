#include "Query.h"

#include "./mysql/cMySQL.h"
#include "./mysql/cMySQLParserElement.h"
#include "./mysql/cMySQLReader.h"

#include "./mysql/cMySQLConnectionPooler.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Manager/ServerManager.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "TimeUtils.h"

#include <iostream>
#include <format>

BOOL QueryManager::InsertAvatar( const uint64& _player_idx , const int& _avatar_id , const std::string& expiry_date , uint64& _avatar_idx )
{
	//printf("InsertAvatar\n");

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	// initialize and prepare CALL statement with parameter placeholders
	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	std::string query = "CALL InsertAvatar(?, ?, ?, ?)";
	int status = mysql_stmt_prepare( stmt , query.c_str() , static_cast< unsigned long >( query.length() ) );
	stmt_error( stmt , status );
	if ( status )
		return FALSE;

	// 파람바인더에 파라미터를 추가해준다.
	cParamBinder parambinder( 4 );
	parambinder.BindParam( 1 , MYSQL_TYPE_LONGLONG , _player_idx );
	parambinder.BindParam( 2 , MYSQL_TYPE_LONG , _avatar_id );
	parambinder.BindParam( 3 , MYSQL_TYPE_STRING , expiry_date );
	parambinder.BindParam( 4 , MYSQL_TYPE_LONG , _avatar_idx );

	cParamBinder outputBinder( 1 );
	outputBinder.BindParam( 1 , MYSQL_TYPE_LONGLONG , _avatar_idx );

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

	status = mysql_stmt_bind_result( stmt , outputBinder.GetBinder() );
	stmt_error_string( stmt , status , error_string );
	if ( status )
		return FALSE;

	// 프로시져의 OUT 결과를 가져옴
	status = mysql_stmt_fetch( stmt );
	stmt_error_string( stmt , status , error_string );
	if ( status )
		return FALSE;

	mysql_stmt_close( stmt );
	return return_value;
}

std::future<BOOL> QueryManager::InsertAvatarAsync(const uint64& _player_idx , const int& _avatar_id , const std::string& expiry_date , uint64& _avatar_idx )
{
	return std::async( std::launch::async , [_player_idx , _avatar_id , expiry_date  , &_avatar_idx]() {
		return QueryManager::InsertAvatar( _player_idx , _avatar_id , expiry_date , _avatar_idx );
	} );
}

BOOL QueryManager::AvatarsGet( const uint64& player_idx , std::map<int , General::AvatarProfile>& avatars )
{
	//printf( "AvatarsGet\n" );

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	//std::string query = "CALL AvatarsGet(?)";

	// 파람바인더에 파라미터를 추가해준다.
	/*cParamBinder parambinder( 1 );
	parambinder.BindParam( 1 , MYSQL_TYPE_LONGLONG , player_idx );

	NetLib::cVector<cMySQLReader*>* result_set = new NetLib::cVector<cMySQLReader*>();

	BOOL bResult = cMySQL::ExcuteProcedure( MySQLConnection , parambinder , query , *result_set );
	if ( bResult == FALSE )
		return FALSE;

	if ( result_set->size() == 0 )
		return FALSE;*/

	General::AvatarProfile _avator;

	// 데이터 읽기
	/*NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set->begin();
	NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set->end();
	for ( ; iter != iter_end; ++iter )
	{
		cMySQLReader* reader = ( *iter );
		if ( reader == nullptr )
			continue;

		_avator.Clear();

		_avator.set_avatar_ref_id( reader->GetLong( "avatar_id" ) );

		avatars.push_back(_avator);
	}*/

	std::string executeQuery = std::format( "select * from avatars where player_idx = '{}';" , player_idx );

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

			_avator.Clear();

			//reader->GetLongLong( "avatar_idx" );
			//reader->GetString( "player_idx" );
			_avator.set_avatar_ref_id( reader->GetLong( "avatar_id" ) );
			//string _reg_dat = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "reg_date" ) );

			// 갱신 시간 읽어오기
			MYSQL_TIME expiry_date = reader->GetDateTime( "expiry_date" );
			string expiry_date_string = TimeUtils::MYSQLTimeToString( expiry_date );
			_avator.set_expires_at( expiry_date_string );

			avatars.insert( std::pair<int, General::AvatarProfile>( _avator.avatar_ref_id(), _avator ) );
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

BOOL QueryManager::PlayerSetAvatar( const uint64& _player_idx , const int& _avatar_id )
{
	//printf( "PlayerSetAvatar\n" );

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	// initialize and prepare CALL statement with parameter placeholders
	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	std::string query = "CALL PlayerSetAvatar(?, ?)";
	int status = mysql_stmt_prepare( stmt , query.c_str() , static_cast< unsigned long >( query.length() ) );
	stmt_error( stmt , status );
	if ( status )
		return FALSE;

	// 파람바인더에 파라미터를 추가해준다.
	int paramSeq = 0;
	cParamBinder parambinder( 2 );
	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONG , _avatar_id );
	parambinder.BindParam( ++paramSeq , MYSQL_TYPE_LONGLONG , _player_idx );

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

std::string QueryManager::GeneratePlayerUpdateAvatar( const uint64& _player_idx , const int& _avatar_id , const std::string _expiry_date )
{
	std::ostringstream oss;

	// UPDATE 쿼리 생성
	oss << "UPDATE avatars ";
	oss << "SET ";
	oss << "`expiry_date` = '" << _expiry_date << "' ";
	oss << "WHERE ";
	oss << "`player_idx` = " << _player_idx << " AND ";
	oss << "`avatar_id` = " << _avatar_id << ";";

	return oss.str();
}

std::future<BOOL> QueryManager::PlayerUpdateAvatarAsnc( const uint64& _player_idx , const int& _avatar_id , const std::string _expiry_date )
{
	return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratePlayerUpdateAvatar( _player_idx , _avatar_id , _expiry_date ) );
}

BOOL QueryManager::PlayerUpdateAvatar( const uint64& _player_idx , const int& _avatar_id , const std::string _expiry_date )
{
	std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratePlayerUpdateAvatar( _player_idx , _avatar_id , _expiry_date ) );
	result.wait();
	return result.get();
}