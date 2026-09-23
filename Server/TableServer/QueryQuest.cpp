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

#include "cDataLoader.h"

#include <iostream>
#include <format>

using protoutil::cProtoUtil;

void QueryManager::CreateQuests( const uint64 playerIdx )
{
    std::vector<std::future<BOOL>> results;
    
    // Daily

    auto dailyMissions = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDailyMissons();

    for ( auto& questPair : dailyMissions ) {
    
        auto& quest = questPair.second;
        results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratInsertQuest( playerIdx , quest.task_id() , General::TaskCategory::TaskCategory_Mission , quest.task_trigger() , 0 , quest.target_progress() ) ) );

    }

    // Lounge

    auto loungeMissions = NetLib::cSingleton<cDataLoader>::GetInstance()->GetLoungeMissions();
    
    for ( auto& questPair : loungeMissions ) {

        auto& quest = questPair.second;
        results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratInsertQuest( playerIdx , quest.task_id() , General::TaskCategory::TaskCategory_LoungeMission , quest.task_trigger() , 0 , quest.target_progress() ) ) );

    }

    // Achievement

    auto achievements = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAchievements();

    for ( auto& questPair : achievements ) {

        auto& quest = questPair.second;
        results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratInsertQuest( playerIdx , quest.task_id() , General::TaskCategory::TaskCategory_Achievement , quest.task_trigger() , 0 , quest.target_progress() ) ) );

    }

    // Group Achievement
    auto groups = NetLib::cSingleton<cDataLoader>::GetInstance()->GetGroupAchievements();

    for ( auto& groupPair : groups ) {

        auto& quests = groupPair.second;
        for( auto& quest : quests )
            results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratInsertQuest( playerIdx , quest.task_id() , General::TaskCategory::TaskCategory_Achievement , quest.task_trigger() , 0 , quest.target_progress() ) ) );

    }

    for ( const auto& future : results )
        future.wait();

    for ( auto& future : results ) {
        if ( false == future.get() ) {
            // 쿼리 실패에 대한 처리
        }
    }

}
//void QueryManager::CreatePinballQuest( const uint64 playerIdx )
//{
//    std::vector<std::future<BOOL>> results;
//
//
//    auto loungeMissions = NetLib::cSingleton<cDataLoader>::GetInstance()->GetLoungeMissions();
//    auto& quest = loungeMissions[ 2433 ];
//    results.push_back( PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , GeneratInsertQuest( playerIdx , quest.task_id() , General::TaskCategory::TaskCategory_LoungeMission , quest.task_trigger() , 0 , quest.target_progress() ) ) );
//    for ( const auto& future : results )
//        future.wait();
//
//    for ( auto& future : results ) {
//        if ( false == future.get() ) {
//            // 쿼리 실패에 대한 처리
//        }
//    }
//
//}


// 퀘스트 정보를 인서트하는 함수
std::string QueryManager::GeneratInsertQuest( uint64 playerIdx , int questId , General::TaskCategory achieveType , General::TaskTrigger achieveEventType , uint64 currentCount , uint64 goalCount )
{
    std::string achieveTypeString = cProtoUtil::GetEnumString( achieveType );
    std::string achieveEventTypeString = cProtoUtil::GetEnumString( achieveEventType );

    // 인서트 쿼리 생성
    std::string query = "INSERT INTO quests (player_idx, quest_id, achieve_type, achieve_type_string, achieve_event_type, "
        "achieve_event_type_string, current_count, goal_count) VALUES ("
        "'" + std::to_string( playerIdx ) + "', "
        "'" + std::to_string( questId ) + "', "
        "'" + std::to_string( achieveType ) + "', "
        "'" + achieveTypeString + "', "
        "'" + std::to_string( achieveEventType ) + "', "
        "'" + achieveEventTypeString + "', "
        "'" + std::to_string( currentCount ) + "', "
        "'" + std::to_string( goalCount ) + "')";

    return query;
}

BOOL QueryManager::PlayerGetQuests( const uint64& player_idx , int& reads ,
        std::map<uint32 , General::TaskProgress>& missions ,
        std::map<uint32 , General::TaskProgress>& lounges ,
        std::map<uint32 , General::TaskProgress>& achieves )
{
	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	std::string executeQuery = std::format("SELECT * FROM quests WHERE player_idx = '{}';" , player_idx );

    General::TaskProgress _quest;

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

            _quest.Clear();

			uint64 quest_idx = reader->GetLongLong( "quest_idx" );
			uint64 player_idx = reader->GetLongLong( "player_idx" );
			int32 quest_id = reader->GetLong( "quest_id" );
			int32 achieve_type = reader->GetLong( "achieve_type" );
			General::TaskCategory achieveType = static_cast< General::TaskCategory >( achieve_type );
			int32 achieve_event_type = reader->GetLong( "achieve_event_type" );
			General::TaskTrigger achieveEventType = static_cast< General::TaskTrigger >( achieve_event_type );
            uint64 current_count = reader->GetLongLong( "current_count" );
            uint64 goal_count = reader->GetLongLong( "goal_count" );
            BOOL reward_complete = reader->GetTinyInt( "reward_complete" );
            MYSQL_TIME reg_date = reader->GetDateTime("reg_date");

            _quest.set_task_instance_id( quest_idx );
            _quest.set_task_id( quest_id );
            //_quest.set_task_category( achieve_type );
            _quest.set_task_trigger( achieveEventType );
            _quest.set_current_progress( current_count );
            _quest.set_target_progress( goal_count );
            _quest.set_grant_claimed( reward_complete );

            switch ( achieveType )
            {
            case General::TaskCategory::TaskCategory_Mission:
            {
                auto quests = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDailyMissons();
                auto iter = quests.find( quest_id );
                if ( iter != quests.end() )
                    _quest.set_grant_kind ( iter->second.grant_kind() );

                missions.insert(std::pair<uint32 , General::TaskProgress>( quest_id , _quest ));
            }
            break;
            case General::TaskCategory::TaskCategory_LoungeMission:
            {
                auto quests = NetLib::cSingleton<cDataLoader>::GetInstance()->GetLoungeMissions();
                auto iter = quests.find( quest_id );
                if ( iter != quests.end() )
                    _quest.set_grant_kind( iter->second.grant_kind() );

                lounges.insert( std::pair<uint32 , General::TaskProgress>( quest_id , _quest ) );
            }
            break;
            case General::TaskCategory::TaskCategory_Achievement:
            {
                auto quests = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAchievements();
                auto iter = quests.find( quest_id );
                if ( iter != quests.end() )
                    _quest.set_grant_kind( iter->second.grant_kind() );

                achieves.insert( std::pair<uint32 , General::TaskProgress>( quest_id , _quest ) );
            }
            break;
            }
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

std::string QueryManager::GenerateUpdateQuest( uint64_t playerIdx , int questId , uint64 currentCount )
{
    // 업데이트 쿼리 생성
    std::string query = "UPDATE quests SET current_count = '" + std::to_string( currentCount ) + "' "
        "WHERE player_idx = '" + std::to_string( playerIdx ) + "' "
        "AND quest_id = '" + std::to_string( questId ) + "'";

    return query;
}

std::string QueryManager::GenerateCompleteQuest( uint64_t playerIdx , int questId , BOOL reward_complete )
{
    // 업데이트 쿼리 생성
    std::string query = "UPDATE quests SET reward_complete = " + std::to_string( reward_complete ? 1 : 0 ) + " "
        "WHERE player_idx = '" + std::to_string( playerIdx ) + "' "
        "AND quest_id = '" + std::to_string( questId ) + "'";

    return query;
}

std::string QueryManager::GenerateResetQuest( uint64_t playerIdx , int questId )
{
    // 업데이트 쿼리 생성
    std::string query = "UPDATE quests SET reward_complete = 0 , current_count = 0 WHERE player_idx = '" + std::to_string( playerIdx ) + "' "
        "AND quest_id = '" + std::to_string( questId ) + "'";

    return query;
}

std::string QueryManager::GetFriendRequestCount( const uint64& player_idx )
{
	std::string friendRequestCount;

	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_SHARD);
    MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
    {
        std::cerr << "Failed to get MySQL connection" << std::endl;
        return friendRequestCount;
    }

	// Select 쿼리문
	std::string executeQuery = std::format(
        "SELECT COUNT(*) FROM quests WHERE player_idx = '{}' AND achieve_event_type = 17;", 
        player_idx
    );

    if (mysql_query(MySQLConnection, executeQuery.c_str()))
    {
        std::cerr << "MySQL query error: " << mysql_error(MySQLConnection) << std::endl;
        return friendRequestCount;
    }

    MYSQL_RES* result = mysql_store_result(MySQLConnection);
    if (result == nullptr)
    {
        std::cerr << "MySQL store result error: " << mysql_error(MySQLConnection) << std::endl;
        return friendRequestCount;
    }

    MYSQL_ROW row;
    if ((row = mysql_fetch_row(result)))
    {
        friendRequestCount = row[0]; // 첫 번째 컬럼의 값 (friendRequestCount)
    }

    mysql_free_result(result);
    return friendRequestCount;
}

std::string QueryManager::GetFriendCount( const uint64& player_idx )
{
	std::string friendCount = "0";

	cConnInstance con_ins(E_DB_TYPE::E_DB_TYPE_SHARD);
    MYSQL* MySQLConnection = con_ins.GetConnection();
	if (MySQLConnection == nullptr)
    {
        std::cerr << "Failed to get MySQL connection" << std::endl;
        return friendCount;
    }

	// Select 쿼리문
	std::string executeQuery = std::format(
        "SELECT COUNT(*) FROM friends WHERE player_idx = '{}';", 
        player_idx
    );

    if (mysql_query(MySQLConnection, executeQuery.c_str()))
    {
        std::cerr << "MySQL query error: " << mysql_error(MySQLConnection) << std::endl;
        return friendCount;
    }

    MYSQL_RES* result = mysql_store_result(MySQLConnection);
    if (result == nullptr)
    {
        std::cerr << "MySQL store result error: " << mysql_error(MySQLConnection) << std::endl;
        return friendCount;
    }

    MYSQL_ROW row;
    if ((row = mysql_fetch_row(result)))
    {
        friendCount = row[0]; // 첫 번째 컬럼의 값 (friendCount)
    }

    mysql_free_result(result);
    return friendCount;
}
