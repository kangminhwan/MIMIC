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

BOOL QueryManager::PlayerGetRecords( const uint64& player_idx , int& reads , PmNet::Ledger& records )
{
	//printf( "PlayerGetRecords\n" );

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	/*MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;*/

	//std::string query = "CALL AvatarsGet(?)";

	//// 파람바인더에 파라미터를 추가해준다.
	//cParamBinder parambinder( 1 );
	//parambinder.BindParam( 1 , MYSQL_TYPE_LONGLONG , player_idx );

	//NetLib::cVector<cMySQLReader*>* result_set = new NetLib::cVector<cMySQLReader*>();

	//BOOL bResult = cMySQL::ExcuteProcedure( MySQLConnection , parambinder , query , *result_set );
	//if ( bResult == FALSE )
	//	return FALSE;

	//if ( result_set->size() == 0 )
	//	return FALSE;

	std::string executeQuery = std::format(
		"SELECT player_idx, game_type, game_type_string, participate_count, participate_count_total, \n"
		"       win_count, win_count_total, lose_count, lose_count_total, \n"
		"       best_get_chip, best_get_chip_total, best_get_coin, best_get_coin_total, \n"
		"       make_all_in_count_total, straight_wins, \n"
		"       slot_get_coin, today_chip, today_coin, reg_date \n"
		"FROM records \n"
		"WHERE player_idx = '{}';" ,
		player_idx
	);

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );

		reads = result_set.size();

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			int participate_count = reader->GetLong( "participate_count" );
			int participate_count_total = reader->GetLong( "participate_count_total" );
			int win_count = reader->GetLong( "win_count" );
			int win_count_total = reader->GetLong( "win_count_total" );
			int lose_count = reader->GetLong( "lose_count" );
			int lose_count_total = reader->GetLong( "lose_count_total" );
			int64 best_get_chip = reader->GetLongLong( "best_get_chip" );
			int64 best_get_chip_total = reader->GetLongLong( "best_get_chip_total" );
			int64 best_get_coin = reader->GetLongLong( "best_get_coin" );
			int64 best_get_coin_total = reader->GetLongLong( "best_get_coin_total" );
			int make_all_in_count_total = reader->GetLong( "make_all_in_count_total" );
			int straight_wins = reader->GetLong( "straight_wins" );
			//int best_straight_wins = reader->GetLong( "best_straight_wins" );
			int64 slot_get_coin = reader->GetLongLong( "slot_get_coin" );
			int64 today_chip = reader->GetLongLong( "today_chip" );
			int64 today_coin = reader->GetLongLong( "today_coin" );
			//int64 today_get_coin = reader->GetLongLong( "today_get_coin" );
			//int64 today_get_chip = reader->GetLongLong( "today_get_chip" );

			std::string game_type_string = reader->GetString( "game_type_string" );

			General::PlayCategory _gameType = static_cast< General::PlayCategory >( reader->GetLong( "game_type" ) );
			switch ( _gameType )
			{
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				auto today = records.mutable_holdem_today();
				today->set_participation_total( participate_count );
				today->set_win_total( win_count );
				today->set_loss_total( lose_count );
				today->set_best_chip_gain( best_get_chip );
				today->set_best_coin_gain( best_get_coin );
				today->set_today_chip_gain( today_chip );
				today->set_today_coin_gain( today_coin );

				auto total = records.mutable_holdem_total();
				total->set_participation_total( participate_count_total );
				total->set_win_total( win_count_total );
				total->set_loss_total( lose_count_total );
				total->set_best_chip_gain( best_get_chip_total );
				total->set_best_coin_gain( best_get_coin_total );
			}
			break;
			}

			//reader->GetString( "player_idx" );
			//_avator.set_avatar_ref_id( reader->GetLong( "avatar_id" ) );
			//string _reg_dat = TimeUtils::MYSQLTimeToString( reader->GetDateTime( "reg_date" ) );

			//avatars.push_back( _avator );
		}

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

// Default Value로 인서트 쿼리를 작성합니다.
// 한방 쿼리로 작성
std::future<BOOL> QueryManager::CreateRecordsAsync( const uint64& _player_idx )
{
	std::vector<std::future<BOOL>> results;


	std::string insertQuery = GenerateRecordsInsertQuery( _player_idx , General::PlayCategory_TexasHoldem , "GameType_Holdem" , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 );
	results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , insertQuery ) );




	//insertQuery = GenerateRecordsInsertQuery( _player_idx , 14 , "GameType_Pinball" , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 );
	//results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , insertQuery ) );

	//insertQuery = GenerateRecordsInsertQuery( _player_idx , 13 , "GameType_Roulette" , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 );
	//results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , insertQuery ) );


	for ( auto& result : results )
		result.wait();

	for ( auto& result : results ) {

		try {
			if ( FALSE == result.get() ) {
				return std::async( std::launch::deferred , []() { return FALSE; } );
			}
		}
		catch ( const std::exception& e ) {
			std::cerr << "QueryManager::CreateRecordsAsync Exception occurred while getting result: " << e.what() << std::endl;
			return std::async( std::launch::deferred , []() { return FALSE; } );
		}
		catch ( ... ) {
			std::cerr << "QueryManager::CreateRecordsAsync Unknown exception occurred while getting result." << std::endl;
			return std::async( std::launch::deferred , []() { return FALSE; } );
		}
	}
	return std::async( std::launch::deferred , []() { return TRUE; } );
}


//

std::string QueryManager::GenerateRecordsInsertQuery( const const uint64& player_idx ,
								const int& game_type ,
								const std::string& game_type_string ,
								int participate_count ,
								int participate_count_total ,
								int win_count ,
								int win_count_total ,
								int lose_count ,
								int lose_count_total ,
								int best_get_chip ,
								int best_get_chip_total ,
								int best_get_coin ,
								int best_get_coin_total ,
								int make_all_in_count_total ,
								int straight_wins ,
								int slot_get_coin ,
								int today_chip ,
								int today_coin)
{
	std::ostringstream oss;

	// INSERT INTO 쿼리 생성
	oss << "INSERT INTO records ";
	oss << "(";
	oss << "`player_idx`, `game_type` , `game_type_string`, `participate_count`, `participate_count_total`, ";
	oss << "`win_count`, `win_count_total`, `lose_count`, `lose_count_total`, ";
	oss << "`best_get_chip`, `best_get_chip_total`, `best_get_coin`, `best_get_coin_total`, ";
	oss << "`make_all_in_count_total`, `straight_wins`, `slot_get_coin`, `today_chip`, `today_coin` ) ";

	// VALUES 부분 생성
	oss << "VALUES ";
	oss << "(";
	oss << "" << player_idx << "," << game_type << ", '" << game_type_string << "', ";
	oss << participate_count << ", " << participate_count_total << ", ";
	oss << win_count << ", " << win_count_total << ", ";
	oss << lose_count << ", " << lose_count_total << ", ";
	oss << best_get_chip << ", " << best_get_chip_total << ", ";
	oss << best_get_coin << ", " << best_get_coin_total << ", ";
	oss << make_all_in_count_total << ", " << straight_wins << ", ";
	oss << slot_get_coin << ", " << today_chip << ", " << today_coin << ");";

	return oss.str();
}

std::string QueryManager::UpdateRecordsInsertQuery( const uint64& player_idx ,
								const std::string& game_type_string ,
								int participate_count ,
								int participate_count_total ,
								int win_count ,
								int win_count_total ,
								int lose_count ,
								int lose_count_total ,
								int64 best_get_chip ,
								int64 best_get_chip_total ,
								int64 best_get_coin ,
								int64 best_get_coin_total ,
								int make_all_in_count_total ,
								int straight_wins ,
								int64 slot_get_coin ,
								int64 today_chip ,
								int64 today_coin ,
								const std::string& dateTime )
{
	std::ostringstream oss;

	// UPDATE 쿼리 생성
	oss << "UPDATE records SET ";
	oss << "participate_count = " << participate_count << ", ";
	oss << "participate_count_total = " << participate_count_total << ", ";
	oss << "win_count = " << win_count << ", ";
	oss << "win_count_total = " << win_count_total << ", ";
	oss << "lose_count = " << lose_count << ", ";
	oss << "lose_count_total = " << lose_count_total << ", ";
	oss << "best_get_chip = " << best_get_chip << ", ";
	oss << "best_get_chip_total = " << best_get_chip_total << ", ";
	oss << "best_get_coin = " << best_get_coin << ", ";
	oss << "best_get_coin_total = " << best_get_coin_total << ", ";
	oss << "make_all_in_count_total = " << make_all_in_count_total << ", ";
	oss << "straight_wins = " << straight_wins << ", ";
	oss << "slot_get_coin = " << slot_get_coin << ", ";
	oss << "today_chip = " << today_chip << ", ";
	oss << "today_coin = " << today_coin << ", ";
	oss << "update_date = '" << dateTime << "' ";
	oss << "WHERE player_idx = " << player_idx << " ";
	oss << "AND game_type_string = '" << game_type_string << "';";

	return oss.str();
}