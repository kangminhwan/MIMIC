// 로그인시 퀘스트 로딩 및 누락된 미션 생성 함수
#include "cClientSession.h"
#include "cDataLoader.h"
#include "Query.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include <future>

// 로그인시 퀘스트 관련 초기화 및 누락 미션 생성
void cClientSession::LoadQuestsOnLogin()
{
    try {
        // cDataLoader에서 마스터 미션 목록 가져오기
        auto& dataLoader = *NetLib::cSingleton<cDataLoader>::GetInstance();
        auto masterDailyMissions = dataLoader.GetDailyMissons();
        auto masterLoungeMissions = dataLoader.GetLoungeMissions();
        auto masterAchievements = dataLoader.GetAchievements();

        // 플레이어의 현재 퀘스트 상태와 비교하여 누락된 미션 생성
        CreateMissingMissions( masterDailyMissions , m_daily_missions , General::TaskCategory::TaskCategory_Mission );
        CreateMissingMissions( masterLoungeMissions , m_lounge_missions , General::TaskCategory::TaskCategory_LoungeMission );
        CreateMissingMissions( masterAchievements , m_achievements , General::TaskCategory::TaskCategory_Achievement );

        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "LoadQuestsOnLogin completed for player %llu" , GetPlayerIdx() );
    }
    catch ( const std::exception& e ) {
        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "LoadQuestsOnLogin error for player %llu: %s" , GetPlayerIdx() , e.what() );
    }
}

// 누락된 미션 생성 함수
void cClientSession::CreateMissingMissions(
    const std::map<int32 , General::TaskProgress>& masterMissions ,
    std::map<uint32 , General::TaskProgress>& playerMissions ,
    const General::TaskCategory& missionType )
{
    std::vector<std::future<BOOL>> insertResults;
    const uint64 playerIdx = GetPlayerIdx();
    int createdCount = 0;

    try {
        // 마스터 미션과 플레이어 미션 비교
        for ( const auto& masterPair : masterMissions ) {
            const int32 questId = masterPair.first;
            const General::TaskProgress& masterQuest = masterPair.second;

            // 플레이어가 해당 퀘스트를 가지고 있는지 확인
            auto playerIter = playerMissions.find( static_cast<uint32>( questId ) );

            // 누락된 퀘스트가 있으면 생성
            if ( playerIter == playerMissions.end() ) {
                General::TaskProgress newQuest;
                newQuest.CopyFrom( masterQuest );
                newQuest.set_current_progress( 0 );
                newQuest.set_grant_claimed( false );

                // 데이터베이스에 새 퀘스트 삽입
                insertResults.emplace_back( QueryManager::PlayerExecuteQueryAsync(
                    E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GeneratInsertQuest( playerIdx , newQuest.task_id() , missionType , newQuest.task_trigger() , 0 , newQuest.target_progress() ) ) );

                // 플레이어 메모리에 퀘스트 추가
                uint32 mapKey = static_cast< uint32 >( questId );
                playerMissions[ mapKey ] = newQuest;

                createdCount++;
            }
        }

        // 모든 삽입 작업 완료 대기
        for ( auto& result : insertResults ) {
            result.wait();
            if ( FALSE == result.get() ) {
                NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Failed to insert mission type %d for player %llu" , (int)missionType , playerIdx );
            }
        }

        if ( createdCount > 0 ) {
            NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "Created %d missing missions (type %d) for player %llu" , createdCount , (int)missionType , playerIdx );
        }
    }
    catch ( const std::exception& e ) {
        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "CreateMissingMissions error for type %d, player %llu: %s" , (int)missionType , playerIdx , e.what() );
    }
}