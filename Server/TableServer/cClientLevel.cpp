#include "cClientSession.h"
#include "cVirtualSession.h"
#include "../Include/Netlib/UdpModule/cUDPSession.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Network/cPacketStack.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cGameRoom.h"
#include "cGameRoomManager.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "TimeUtils.h"

#include "Query.h"

#include <iostream>
#include <format>
#include <future>

void cClientSession::UpdateLevel( const uint64& getExp )
{
	if ( getExp == 0 )
		return;

	uint64 curExp = m_player.experience_points();
	int curLevel = m_player.member_level();

	uint64 newExp = curExp + getExp;

	const auto& levels = NetLib::cSingleton<cDataLoader>::GetInstance()->GetLevelList();

	int newLevel = 0;
	for ( auto& levelData : levels ) {

		if ( newExp <= levelData.exp() ) {
			newLevel = levelData.level();
			break;
		}
	}

	const uint64& player_idx = GetPlayerIdx();

	// 갱신 된 레벨에 대한 보상 처리
	if ( newLevel > curLevel )
	{
		std::vector<std::future<BOOL>> results;
		for ( int n = newLevel; n < newLevel + 1; ++n )
		{
			// 보상에 해당하는 리워드 칩을 우편으로 발송
			auto& level_data = levels[ n - 1 ];

			std::string expiry_date = TimeUtils::GetCurrentKSTDateTimeString(24 * 3);
			std::string query = QueryManager::GenerateMailBoxInsertQuery( player_idx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_LevelUpReward , level_data.reward_chip() , 0 , "" , 0 , expiry_date);
			results.push_back( QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query ) );

			if ( newLevel == levels.size() )
				break;
		}

		for ( auto& result : results )
			result.wait();

		for ( auto& result : results ) {
			if ( FALSE == result.get() ) {

			}
		}

		// Lobby 로 갔을때 메일 박스를 갱신해 주도록 하자

	}
	// 운영툴에 의해서 경험치를 수정하는 경우
	else
	{

	}

	m_player.set_member_level( newLevel );
	m_player.set_experience_points( newExp );

	// 플레이어 데이터 갱신

}