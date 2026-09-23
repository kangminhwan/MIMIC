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
#include "cProtoUtil.h"

#include <iostream>
#include <format>

using protoutil::cProtoUtil;

std::string QueryManager::InsertRecordsJokboQuery( const uint64_t& player_idx ,
												  const std::string& game_type ,
												  int game_type_num ,
												  const std::string& jokbo ,
												  int jokbo_num ,
												  int total_count ,
												  int win_count ) {
	std::ostringstream oss;

	// INSERT INTO 쿼리 생성
	oss << "INSERT INTO records_jokbo ";
	oss << "(";
	oss << "`player_idx`, `game_type`, `game_type_num`, `jokbo`, `jokbo_num`, `total_count`, `win_count`";
	oss << ") ";
	oss << "VALUES ";
	oss << "(";
	oss << player_idx << ", ";
	oss << "'" << game_type << "', ";
	oss << game_type_num << ", ";
	oss << "'" << jokbo << "', ";
	oss << jokbo_num << ", ";
	oss << total_count << ", ";
	oss << win_count << ");";

	// 현재 날짜와 시간을 포맷에 맞게 추가
	/*std::time_t current_time = std::time( nullptr );
	char buffer[ 20 ];
	std::strftime( buffer , sizeof( buffer ) , "%Y-%m-%d %H:%M:%S" , std::localtime( &current_time ) );
	oss << "'" << buffer << "'";

	oss << ");";*/

	return oss.str();
}

std::string QueryManager::UpdateRecordsJokboQuery( const uint64_t& player_idx ,
												  int game_type_num ,
												  int jokbo_num ,
												  int total_count ,
												  int win_count )
{
	std::ostringstream oss;

	// UPDATE 쿼리 생성
	oss << "UPDATE records_jokbo ";
	oss << "SET ";
	oss << "`total_count` = " << total_count << ", ";
	oss << "`win_count` = " << win_count << " ";
	oss << "WHERE ";
	oss << "`player_idx` = " << player_idx << " AND ";
	oss << "`game_type_num` = " << game_type_num << " AND ";
	oss << "`jokbo_num` = " << jokbo_num << ";";

	return oss.str();
}

// 모든 데이터 다 만듭니다.
// 차후에는 필요한 데이터만 생성하는 방향으로 갑니다.
// 전체다 비동기로 생성하고 리턴한다.
BOOL QueryManager::CreateJokboRecords( const uint64& _player_idx )
{
	std::vector<std::future<BOOL>> results;

	// 로우바둑이
	General::PlayCategory _game_type = General::PlayCategory::PlayCategory_TexasHoldem;
	General::HandRank _jokbo = General::HandRank::HandRank_HoldemRoyalStraightFlush;// 로얄 스트레이트 플러쉬

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemStraightFlush;		// 스트레이트 플러쉬

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemFourOfKind;			// 포카드

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemFullHouse;			// 풀하우스

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemFlush;				// 플러쉬

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemStraight;			// 스트레이트

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemThreeOfKind;			// 트리플

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemTwoPair;			// 투페어

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemOnePair;			// 원페어

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_HoldemHighCard;			// 하이카

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );
	_jokbo = General::HandRank::HandRank_RecordFold;				// 족보 표기용 ( 홀덤 폴드 )

	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , InsertRecordsJokboQuery( _player_idx , cProtoUtil::GetEnumString( _game_type ) , static_cast< int >( _game_type ) , cProtoUtil::GetEnumString( _jokbo ) , static_cast< int >( _jokbo ) , 0 , 0 ) ) );

	// 바카라
	for ( auto& future : results ) {
		future.wait();
	}

	// 모든 결과를 검사하고 추가 작업을 수행
	for ( auto& future : results ) {
		if ( future.get() ) {
			// 작업이 성공한 경우에 수행할 동작
		}
		else
		{
			// 작업 실패한 경우의 로그
			int n = 0;
		}
	}

	return TRUE;
}

BOOL QueryManager::PlayerGetJokboRecords( const uint64& player_idx , int& reads , PmNet::Ledger& records )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format(
		"SELECT * FROM tpp.records_jokbo \n"
		"WHERE player_idx = '{}';" ,
		player_idx
	);

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

			int64 records_jokbo_idx = reader->GetLongLong( "records_jokbo_idx" );
			int64 player_idx = reader->GetLongLong( "player_idx" );
			std::string game_type = reader->GetString( "game_type" );
			int game_type_num = reader->GetLong( "game_type_num" );
			//std::string jokbo = reader->GetString( "jokbo" );
			int jokbo_num = reader->GetLong( "jokbo_num" );
			int total_count = reader->GetLong( "total_count" );
			int win_count = reader->GetLong( "win_count" );

			General::PlayCategory gameType = static_cast< General::PlayCategory >( game_type_num );
			General::HandRank jokbo = static_cast< General::HandRank >( jokbo_num );

			General::HandRankRecord recordJokbo;
			recordJokbo.set_hand_rank( jokbo );
			recordJokbo.set_occurrence_total( total_count );
			recordJokbo.set_win_total( win_count );

			switch ( gameType )
			{
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				records.mutable_holdem_hand_ranks()->insert( { jokbo_num , recordJokbo } );
			}
			break;
			}




			//reader->GetString( "player_idx" );
			//_avator.set_avatar_ref_id( reader->GetLong( "avatar_id" ) );
			//string _reg_dat = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "reg_date" ) );

			//avatars.push_back( _avator );
		}

		reads = result_set.size();

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
	}

	//if ( stmt != nullptr )
	//	mysql_stmt_close( stmt );

	return TRUE;
}