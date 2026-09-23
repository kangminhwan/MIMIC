#include "cClientSession.h"
#include "cVirtualSession.h"
#include "../Include/Netlib/UdpModule/cUDPSession.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Network/cPacketStack.h"
#include "../Include/Netlib/IOCP/cIocpConnector.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cGameRoom.h"
#include "cGameRoomManager.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "cMaintenanceManager.h"
#include "cConfigReader.h"

#include "Query.h"

#include <iostream>
#include <format>
#include <future>

void cClientSession::UpdateDailyLostLimit()
{
	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::UpdateDailyLostLimit m_pLostLimit is nullptr" );
		return;
	}

	// 손실한도 갱신 시간이 설정되어 있는 경우와 없는 경우로 구분
	std::string limit_string = m_pLostLimit->loss_limit_reset_at();
	std::time_t limit_time = TimeUtils::StringToTimeTM( limit_string );
	std::time_t today_start_time = TimeUtils::TodayStartTimeTM();

	// 초기화 시간이 지났으면 Daily 초기화 할 것들을 처리한다.
	// NOTE: caller (cClientSession.cpp daily refresh path) already gates this to once-per-day,
	//       so the legacy "limit_time > today_start_time" guard prevented pending updates from
	//       being applied for users who were never blocked. Force the else-branch path below.
	if ( false )
	{
		std::time_t current_time = std::time( nullptr );

		if ( current_time > limit_time )
		{
			// Daily 인 손실 한도 작업 갱신처리
			uint64 daily_lost_chip = m_pLostLimit->daily_chip_loss();
			m_pLostLimit->set_daily_chip_loss( 0 );

			if ( m_pLostLimit->pending_loss_limit() > 0 ) {

				uint64 cur_lost_limit = m_pLostLimit->loss_limit_amount();
				uint64 next_lost_limit = m_pLostLimit->pending_loss_limit();

				if ( cur_lost_limit != next_lost_limit ) {

					m_pLostLimit->set_loss_limit_amount( next_lost_limit );
					m_pLostLimit->set_pending_loss_limit( 0 );
				}
			}

			if ( m_pLostLimit->pending_play_block_hours() > 0 ) {

				uint64 cur_set_time_limit = m_pLostLimit->play_block_hours();
				uint64 next_set_time_limit = m_pLostLimit->pending_play_block_hours();

				if ( cur_set_time_limit != next_set_time_limit ) {

					m_pLostLimit->set_play_block_hours( next_set_time_limit );
					m_pLostLimit->set_pending_play_block_hours( 0 );
				}
			}

			// 다음 갱신 시간 셋팅
			std::string next_limit_string = TimeUtils::TomorrowStartTimeString();
			m_pLostLimit->set_loss_limit_reset_at( next_limit_string );
			if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) )
			{

				// 쿼리 실패

			}
			else
			{
				// 손실한도 상태 변경 동기화
				const uint64& account_idx = GetAccountIdx();

				Server::LostLimitUpdateReq _lost_limit_update;
				_lost_limit_update.set_update_lost_limit( true );
				auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
				add_lost_limit->CopyFrom( *m_pLostLimit );

				GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
				pGOOGLE_PROTO_BUFFER->Clear();
				if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::UpdateDailyLostLimit Send GMsg_LostLimitUpdate Failed." );
				}
				else {
					E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

					if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

						// 로비서버 전송 실패
						NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
					}
					
				}
			}
		}
		else
		{
			// 할 작업이 없음
		}
	}
	else
	{
		// Block window still active — leave pending values + chip loss intact.
		// CheckLostLimitTime() applies them on expiry; daily 00:00 must not bypass the block.
		std::time_t current_time = std::time( nullptr );
		if ( limit_time != 0 && current_time < limit_time ) {
			return;
		}

		// Daily 인 손실 한도 작업 갱신처리
		uint64 daily_lost_chip = m_pLostLimit->daily_chip_loss();
		m_pLostLimit->set_daily_chip_loss( 0 );

		if ( m_pLostLimit->pending_loss_limit() > 0 ) {

			uint64 cur_lost_limit = m_pLostLimit->loss_limit_amount();
			uint64 next_lost_limit = m_pLostLimit->pending_loss_limit();

			if ( cur_lost_limit != next_lost_limit ) {

				m_pLostLimit->set_loss_limit_amount( next_lost_limit );
			}
			m_pLostLimit->set_pending_loss_limit( 0 );
		}

		if ( m_pLostLimit->pending_play_block_hours() > 0 ) {

			uint64 cur_set_time_limit = m_pLostLimit->play_block_hours();
			uint64 next_set_time_limit = m_pLostLimit->pending_play_block_hours();

			if ( cur_set_time_limit != next_set_time_limit ) {

				m_pLostLimit->set_play_block_hours( next_set_time_limit );
			}
			m_pLostLimit->set_pending_play_block_hours( 0 );
		}

		// reset_at left as-is: an expired past timestamp behaves identically to "" for the
		// guard above (limit_time != 0 && current < limit_time is false either way).
		// Clearing it would make CheckLostLimitTime treat the user as "just-expired" again
		// on the next call (limit_time == 0 → current > limit_time is trivially true),
		// which currently triggers an erroneous one-free-pass when daily_chip_loss > set_chip.

		if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) )
		{

			// 쿼리 실패

		}
		else
		{
			// 손실한도 상태 변경 동기화
			const uint64& account_idx = GetAccountIdx();

			Server::LostLimitUpdateReq _lost_limit_update;
			_lost_limit_update.set_update_lost_limit( true );
			auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
			add_lost_limit->CopyFrom( *m_pLostLimit );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::UpdateDailyLostLimit Send GMsg_LostLimitUpdate Failed." );
			}
			else {
				E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
					E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

				if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

					// 로비서버 전송 실패
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
				}
				
			}
		}
	}
}

// 구매 한도 갱신 시간에 따른 처리
void cClientSession::UpdateMonthlyLostLimit()
{
	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::UpdateMonthlyLostLimit m_pLostLimit is nullptr" );
		return;
	}

	// 구매 한도 갱신 시간
	std::string limit_string = m_pLostLimit->purchase_limit_reset_at();
	std::time_t limit_time = TimeUtils::StringToTimeTM( limit_string );

	// 이번달의 시작 시간
	std::string this_month_start_stirng = TimeUtils::ThisMonthStartTimeString();
	std::time_t this_month_start_time = TimeUtils::StringToTimeTM( this_month_start_stirng );

	// 손실 제한 갱신 시간과
	// 이번달 시작 시간이 겹쳐 같은 경우에도 갱신하도록 수정
	if ( this_month_start_time >= limit_time )
	{
		// 지난 달 누적 구매 초기화
		uint64 monthly_buy_total = m_pLostLimit->monthly_purchase_total();
		m_pLostLimit->set_monthly_purchase_total( 0 );
		m_pLostLimit->set_loss_limit_changes( 0 );

		// 다음 갱신 시간 셋팅
		std::string next_limit_string = TimeUtils::NextMonthStartTimeString();
		m_pLostLimit->set_purchase_limit_reset_at( next_limit_string );

		if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) )
		{

			// 쿼리 실패

		}
		else
		{
			// 손실한도 상태 변경 동기화
			const uint64& account_idx = GetAccountIdx();

			Server::LostLimitUpdateReq _lost_limit_update;
			_lost_limit_update.set_update_lost_limit( true );
			auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
			add_lost_limit->CopyFrom( *m_pLostLimit );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::UpdateMonthlyLostLimit Send GMsg_LostLimitUpdate Failed." );
			}
			else {
				E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
					E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

				if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

					// 로비서버 전송 실패
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
				}
				
			}
		}
	}
}

// 손실한도 변경 예약
General::ResultCode cClientSession::ChangeLostLimit( const uint64& new_lost_limit , const int32& new_time_limit )
{
	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::ChangeLostLimit m_pLostLimit is nullptr" );
		return General::ResultCode::Result_LossLimitUpdateFailed;
	}

	// 변경 하려는 부분이 없음
	//if ( new_lost_limit == 0 && new_time_limit == 0 )
	//	return General::ResultCode::Result_LossLimitUpdateFailed;

	// 손실한도가 변경이 예약 되어 있음
	//if ( m_pLostLimit->pending_loss_limit() != 0 )
	//	return General::ResultCode::Result_LossLimitChangeAlreadyReserved;

	//// 손실한도 제한 시간 변경이 예약 되어 있음
	//if ( m_pLostLimit->pending_play_block_hours() != 0 )
	//	return General::ResultCode::Result_PlayTimeLimitChangeReserved;

	// 월간 손실 한도 변경 횟수 초과
	if ( m_pLostLimit->loss_limit_changes() >= 2 )
		return General::ResultCode::Result_LossLimitChangeQuotaReached;

	m_pLostLimit->set_pending_loss_limit( new_lost_limit );
	m_pLostLimit->set_pending_play_block_hours( new_time_limit );

	int cur_lost_limit_change_count = m_pLostLimit->loss_limit_changes();
	m_pLostLimit->set_loss_limit_changes( ++cur_lost_limit_change_count );

	if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) ) {
		// 쿼리 실패
		return General::ResultCode::Result_LossLimitUpdateFailed;
	}
	else
	{
		// 손실한도 상태 변경 동기화
		const uint64& account_idx = GetAccountIdx();

		Server::LostLimitUpdateReq _lost_limit_update;
		_lost_limit_update.set_update_lost_limit( true );
		auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
		add_lost_limit->CopyFrom( *m_pLostLimit );

		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::ChangeLostLimit Send GMsg_LostLimitUpdate Failed." );
		}
		else {
			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

				// 로비서버 전송 실패
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
			}
			
		}
	}
	
	return General::ResultCode::Result_Success;
}

// 금일 획득, 또는 손실한 칩에 대해 계속 누적을 한다.
void cClientSession::UpdateLostLimitChip( const General::AssetKind& moneyType , const uint64& before_chip , const uint64& after_chip )
{
	if ( moneyType != General::AssetKind::AssetKind_Chip )
		return;

	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::UpdateLostLimitChip m_pLostLimit is nullptr" );
		return;
	}

	if ( before_chip == after_chip )
		return;

	const int64 current_lost_chip = m_pLostLimit->daily_chip_loss();
	int64 add_lost_chip = 0;
	bool b_limit_out = false;
	if ( after_chip < before_chip ) {

		// 손실 금액 증가
		const uint64 lost_chip = before_chip - after_chip;
		const int64 new_lost_chip = current_lost_chip + lost_chip;

		// 양수값 입니다.
		add_lost_chip = lost_chip;

		m_pLostLimit->set_daily_chip_loss( new_lost_chip );
	}
	else {
		// 손실 금액 감소
		const uint64 get_chip = after_chip - before_chip;
		const int64 new_lost_chip = current_lost_chip - get_chip;

		// 음수값 입니다.
		add_lost_chip = -1 * ( int64 )get_chip;

		m_pLostLimit->set_daily_chip_loss( new_lost_chip );
	}

	int64 set_chip = m_pLostLimit->loss_limit_amount();
	if ( m_pLostLimit->daily_chip_loss() > set_chip )
	{
		const int32 time_limit = m_pLostLimit->play_block_hours();
		std::string time_limit_string = TimeUtils::GetCurrentKSTDateTimeString( time_limit );
		m_pLostLimit->set_loss_limit_reset_at( time_limit_string );
		int dailylossblockcode = 10401;
		std::string datas;
		datas = "DailyLostChip : " + std::to_string( m_pLostLimit->daily_chip_loss() );

		std::future<BOOL> daily_loss_block_log_result = QueryManager::InsertDailyLossBlock(
				dailylossblockcode , // code
				m_platform_guid , // UID , // uid
				datas , // DATAS
				"" , // GMID
				std::to_string( set_chip ) ); // TOTAL

		daily_loss_block_log_result.wait();
		NetLib::cSingleton<cMaintenanceManager>::GetInstance()->PushbackLostLimit( GetAccountGuid() );
		b_limit_out = true;
	}

	if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) )
	{

		// 쿼리 실패

	}
	else
	{
		// 칩 손실한도 동기화
		const uint64& account_idx = GetAccountIdx();

		Server::LostLimitUpdateReq _lost_limit_update;
		_lost_limit_update.set_account_idx( account_idx );
		_lost_limit_update.set_add_lost_chip( add_lost_chip );
		_lost_limit_update.set_update_lost_chip( true ); 
		if ( b_limit_out )
		{
			_lost_limit_update.set_refresh_time_of_lost_limit( m_pLostLimit->loss_limit_reset_at() );
			_lost_limit_update.set_limit_out( true );
			_lost_limit_update.set_account_guid( GetAccountGuid() );
		}


		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::UpdateLostLimitChip Send GMsg_LostLimitUpdate Failed." );
		}
		else {
			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

				// 로비서버 전송 실패
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
			}

			
		}

		
	}
}



bool cClientSession::CheckLostLimitPrice( const int32& price )
{
	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::UpdateLostLimitPrice m_pLostLimit is nullptr" );
		return true;
	}

	const int64 current_buy = m_pLostLimit->monthly_purchase_total();
	if ( current_buy + price > m_pLostLimit->purchase_limit_amount() )
		return true;
	else
		return false;
}
void cClientSession::UpdateLostLimitPrice( const int32& price )
{
	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::UpdateLostLimitPrice m_pLostLimit is nullptr" );
		return;
	}

	const int64 current_buy = m_pLostLimit->monthly_purchase_total();

	m_pLostLimit->set_monthly_purchase_total( current_buy + price );

	if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) ) {

		// 쿼리 실패
		return;
	}
	else
	{
		// 구매 금액 손실한도 동기화
		const uint64& account_idx = GetAccountIdx();

		Server::LostLimitUpdateReq _lost_limit_update;
		_lost_limit_update.set_account_idx( account_idx );
		_lost_limit_update.set_add_price( price );
		_lost_limit_update.set_update_price( true );

		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::UpdateLostLimitChip Send GMsg_LostLimitUpdate Failed." );
		}
		else {
			E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
				E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

			if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

				// 로비서버 전송 실패
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
			}
			
		}
	}
}

// 플레이 종료시에 체크해서 손실한도가 넘었으면 플레이를 금지한다.
bool cClientSession::IsOverLostLimit( bool setLostTime )
{
	if ( m_pLostLimit == nullptr ) {
		//assert( false , "cClientSession::IsOverLostLimit m_pLostLimit is nullptr" );
		return false;
	}
	/*if ( E_SERVER_STAGE::LIVE!= NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
		return false;*/
	int64 set_chip = m_pLostLimit->loss_limit_amount();

	if ( m_pLostLimit->daily_chip_loss() > set_chip )
	{
		CheckLostLimitTime();

		if ( m_pLostLimit->daily_chip_loss() <= set_chip )
			return false;

		if ( setLostTime ) {

			// 설정한 시간 만큼 제재를 가야야 합니다.
			const int32 time_limit = m_pLostLimit->play_block_hours();

			// 현재 시간에 제한 시간을 더한 시간 계산
			std::string time_limit_string = TimeUtils::GetCurrentKSTDateTimeString( time_limit );
			m_pLostLimit->set_loss_limit_reset_at( time_limit_string );

			if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) ) {
				// 쿼리 실패

			}
			else
			{
				// 구매 금액 손실한도 동기화
				const uint64& account_idx = GetAccountIdx();

				Server::LostLimitUpdateReq _lost_limit_update;
				_lost_limit_update.set_update_lost_limit( true );
				auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
				add_lost_limit->CopyFrom( *m_pLostLimit );

				GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
				pGOOGLE_PROTO_BUFFER->Clear();
				if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::IsOverLostLimit Send GMsg_LostLimitUpdate Failed." );
				}
				else {
					E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

					if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

						// 로비서버 전송 실패
						NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , std::format( "LostLimit broadcast LOBBY_SERVER no target. account_idx={}", account_idx ).c_str() );
					}
					
				}
			}
		}
		
		return true;
	}

	return false;
}

// 자가 제한 시간이 풀렸는지 확인하고
// 시간이 넘었으면 초기화 해준다.
bool cClientSession::CheckLostLimitTime()
{
	// 손실한도 갱신 시간이 설정되어 있는 경우와 없는 경우로 구분
	std::string limit_string = m_pLostLimit->loss_limit_reset_at();
	std::time_t limit_time = TimeUtils::StringToTimeTM( limit_string );

	std::time_t current_time = std::time( nullptr );

	if ( current_time > limit_time )
	{
		// Daily 인 손실 한도 작업 갱신처리
		uint64 daily_lost_chip = m_pLostLimit->daily_chip_loss();
		m_pLostLimit->set_daily_chip_loss( 0 );

		if ( m_pLostLimit->pending_loss_limit() > 0 ) {

			uint64 cur_lost_limit = m_pLostLimit->loss_limit_amount();
			uint64 next_lost_limit = m_pLostLimit->pending_loss_limit();

			if ( cur_lost_limit != next_lost_limit ) {

				m_pLostLimit->set_loss_limit_amount( next_lost_limit );
				m_pLostLimit->set_pending_loss_limit( 0 );
			}
		}

		if ( m_pLostLimit->pending_play_block_hours() > 0 ) {

			uint64 cur_set_time_limit = m_pLostLimit->play_block_hours();
			uint64 next_set_time_limit = m_pLostLimit->pending_play_block_hours();

			if ( cur_set_time_limit != next_set_time_limit ) {

				m_pLostLimit->set_play_block_hours( next_set_time_limit );
				m_pLostLimit->set_pending_play_block_hours( 0 );
			}
		}

		// 다음 갱신 시간 셋팅
		std::string next_limit_string = TimeUtils::TomorrowStartTimeString();
		m_pLostLimit->set_loss_limit_reset_at( next_limit_string );

		if ( FALSE == QueryManager::UpdateLostLimit( m_pLostLimit ) )
		{

			// 쿼리 실패
			return false;
		}
		else
		{
			// 손실한도 상태 변경 동기화
			const uint64& account_idx = GetAccountIdx();

			Server::LostLimitUpdateReq _lost_limit_update;
			_lost_limit_update.set_update_lost_limit( true );
			auto add_lost_limit = _lost_limit_update.mutable_lost_limits();
			add_lost_limit->CopyFrom( *m_pLostLimit );

			GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
			pGOOGLE_PROTO_BUFFER->Clear();
			if ( _lost_limit_update.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::CheckLostLimitTime Send GMsg_LostLimitUpdate Failed." );
				return false;
			}
			else {
				E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
					E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeLossCapUpdate , pGOOGLE_PROTO_BUFFER->SerializeBuffer , _lost_limit_update.ByteSizeLong() );

				if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

					// 로비서버 전송 실패
					return false;
				}
				
			}
			return true;
		}
	}

	return false;
}