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

// 출석 일수를 갱신한다.
// Lobby Update 에서만 처리하도록 한다.
BOOL cClientSession::UpdateAttendance()
{
	const uint64& playerIdx = GetPlayerIdx();
	int32 curDays = m_player.attendance_streak_days();

	const int32 totalDays = NetLib::cSingleton<cDataLoader>::GetInstance()->TotalAttendanceDays();

	if ( curDays == totalDays )
	{
		// 30일 차에 출석 보상이 지급 완료된 상황이다.
		// 다시 1일차 부터 지급한다.
		return AttendanceDay1Reward();
	}
	else
	{
		const Server::ShopProduct& reward = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAttendanceReward( curDays + 1 );
		m_player.set_attendance_streak_days( curDays + 1 );

		// 우편으로 칩 지금 보상 획득 기간 3일
		string reward_limit_date = TimeUtils::GetCurrentKSTDateTimeString(72);
		string query;
		if ( reward.paid_chips() > 0 ) {
			query = QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_Attendance , reward.paid_chips() , 0 , "" , 0 , reward_limit_date );
		}
		else if ( reward.avatar_id() > 0 ) {
			query = QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_Avatar , General::InboxReason::InboxReason_Attendance , 1 , reward.avatar_id() , "" , reward.avatar_add_days() , reward_limit_date );
		}
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
		result.wait();
		
		if ( FALSE == result.get() )
		{
			// 메일 박스 보상 실패
			return FALSE;
		}
		else
		{
			if ( FALSE == QueryManager::UpdateAttendance( playerIdx , m_player.attendance_streak_days() ) ) {

				// 메일 박스 보상은 나갔지만, 출석 업데이트 실패

			}
		}
	}
	return TRUE;
}

BOOL cClientSession::AttendanceDay1Reward()
{
	if ( FALSE == GiveAttendanceDay1Reward() )
	{
		// 메일 박스 보상 실패
		return FALSE;
	}
	else
	{
		const uint64& playerIdx = GetPlayerIdx();

		if ( FALSE == QueryManager::UpdateAttendance( playerIdx , m_player.attendance_streak_days() ) ) {

			// 메일 박스 보상은 나갔지만, 출석 업데이트 실패

		}
	}
	return TRUE;
}

BOOL cClientSession::GiveAttendanceDay1Reward()
{
	m_player.set_attendance_streak_days( 1 );

	const uint64& playerIdx = GetPlayerIdx();

	const Server::ShopProduct& reward = NetLib::cSingleton<cDataLoader>::GetInstance()->GetAttendanceReward( 1 );

	// 우편으로 칩 지금 보상 획득 기간 3일
	string reward_limit_date = TimeUtils::GetCurrentKSTDateTimeString( 72 );
	string query;
	if ( reward.paid_chips() > 0 ) {
		query = QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_Attendance , reward.paid_chips() , 0 , "" , 0 , reward_limit_date );
	}
	else if ( reward.avatar_id() > 0 ) {
		query = QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_Avatar , General::InboxReason::InboxReason_Attendance , 1 , reward.avatar_id() , "" , reward.avatar_add_days() , reward_limit_date );
	}
	std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
	result.wait();
	return result.get();
}