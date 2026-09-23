#include "cClientSession.h"
#include "cVirtualSession.h"
#include "../Include/Netlib/UdpModule/cUDPSession.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/IOCP/cIocpConnector.h"

#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Network/cPacketStack.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include <sstream>

#include "cGameRoom.h"
#include "cGameRoomManager.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "TimeUtils.h"
#include "cFreeCharger.h"
#include "cMoneyLogInstance.h"

#include "Query.h"

#include <future>
#include "cMessageLog.h"
#include "cConfigReader.h"
#include "cFriendManager.h"
#include "cRedisController.h"

std::map<uint64_t , uint64_t> cClientSession::m_qa_player_coins;
std::map<uint64_t , uint64_t> cClientSession::m_qa_player_chips;

cClientSession::cClientSession()
{
	m_delayTick = 0;
	//m_cheatRoulette = 999;
	m_virtualClientIdx = 0;
	b_updateLost = false;
}

cClientSession::cClientSession( const cClientSession& other ) {
	// 멤버 변수를 복사합니다.
	this->b_updateLost = other.b_updateLost;
	this->m_player = other.m_player;
	this->is_copyed = true;
	this->m_die = true;
	this->m_cards = other.m_cards;
	this->m_stockLostMoney = other.m_stockLostMoney;
	this->m_freeDealerFeeMoney = other.m_freeDealerFeeMoney;
	this->m_beforeSlotIndex = other.m_beforeSlotIndex;
	this->m_roomOutReason = other.m_roomOutReason;
	//this->m_bettings = other.m_bettings;
	this->m_bettings.RemoveAll(); // 기존 데이터를 모두 제거
	POSITION pos = other.m_bettings.GetStartPosition();
	while ( pos ) {
		Server::PlayPhase key;
		std::vector<General::TableAction>* src = nullptr;
		other.m_bettings.GetNextAssoc( pos , key , src );
		if ( src ) {
			auto* cloned = new std::vector<General::TableAction>( *src ); // 내용 복사
			this->m_bettings.SetAt( key , cloned );
		}
	}
}

cClientSession::~cClientSession()
{
}

cClientSession* cClientSession::CreateSession()
{
	return new cClientSession();
}

cVirtualSession* cClientSession::CreateVirtualSession()
{
	// PlayerIdx 가 없는데, 인덱스를 생성하려고 하면 이셉션 발생시킨다.
	if ( GetPlayerIdx() == 0 ) {
		return nullptr;
	}

	// virtual cliend playeridx 인덱스 부여
	uint64 virtualClientIdx = ++m_virtualClientIdx;
	cVirtualSession* pVirtualSession = new cVirtualSession( this , virtualClientIdx );
	pVirtualSession->SetSessionType( Sessions::SESSION_DUMMY );
	virtualClients.push_back( pVirtualSession );
	return pVirtualSession;
}

void cClientSession::Init()
{
	NetLib::cSession::Init();
}

// 세션 완전 초기화
void cClientSession::Clear()
{
	NetLib::cSession::Clear();
	m_player.Clear();
	m_roomNumber = 0;
	m_roomNumberBefore = 0;
	GameReset();

	m_daily_missions.clear();
	m_lounge_missions.clear();
	m_achievements.clear();

	m_pLostLimit = nullptr;
	m_account_idx = 0;
	m_account_guid.clear();
	m_platform_guid.clear();
	m_mailBox.clear();
	m_market = General::StoreChannel::StoreChannel_None;
	m_deviceInfo = "";
	m_osInfo = "";
	m_ip = "";
	m_joinTime = "";
	m_game_version = "";
	m_roomOutReason = "";
	roomjoinCount = 0;
	//m_cheatRoulette = 999;
}
void cClientSession::SoftClear()
{
	roomjoinCount = 0;
}

// 게임 데이터 리셋
void cClientSession::GameReset()
{
	m_cards.Clear();

	// m_bettings 메모리 삭제
	Server::PlayPhase key;
	std::vector<General::TableAction>* value = nullptr;
	for ( POSITION pos = m_bettings.GetStartPosition(); pos != NULL; ) {
		m_bettings.GetNextAssoc( pos , key , value );
		if ( value != nullptr ) {
			value->clear();
			//delete value;
		}
	}
	m_player.set_folded_out( false );
	m_bettings.RemoveAll();
	m_bettingSlot = 0;
	m_die = false;
	m_allIn = false;
	m_side = false;
	m_sendSide = false;
	m_reserveRoomOut = false;
	m_stockLostMoney = 0;
	m_virtualLostMoney = 0;
	m_passBet = false;
	m_roomPlayingCount = 0;
	m_noActionCounter = 0;
	m_noActionFlag = true;
	InitMoveSlotCount();
	ClearPlayerBet();

	m_updateGameTypes.clear();
	m_updateJokbos.clear();
	m_room_record.Clear();

	m_limit_game_type = General::PlayCategory::PlayCategory_None;	// 한도 초과가 발생한 게임
	m_limit_money_type = General::AssetKind::AssetKind_None;	// 칩 또는 코인 한도 초과
	m_mail_box_check = false;								// true 일 경우 우편함으로 지급 되었습니다. 알림 필요
}

void cClientSession::RoomOutReset()
{
	GameReset();
	SetJoinedRoomNumber( 0 );
}

void cClientSession::DisConnectContext(UINT Entity, UINT nThreadIndex)
{
	if (m_pContext != nullptr)
	{
		if (m_pContext->GetEntity() != Entity) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::DisConnectContext Failed. Entity [ %u ] != Entity[ %u ]" , m_pContext->GetEntity() , Entity );
		}

		m_pContext->SetSession(nullptr);
		SetContext(nullptr);
	}
}

bool cClientSession::SetPlayer( const uint64 accountIdx , const int64 playerIdx, const int db_idx, const std::string& sessionid)
{
	if (m_pContext == nullptr) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cClientSession::SetAccountAddItToTheManager is Failed. Context is nullptr PlayerIdx[ %I64d ]");
		assert(false && "cClientSession::SetPlayer Failed. Context is nullptr");
		return false;
	}

	SetSessionType(Sessions::SESSION_CLIENT);
	SetSessionStatus(E_SESSION_STATUS::E_SESSION_STATUS_CONNECTED);
	
	m_account_idx = accountIdx;
	m_player.Clear();
	m_player.set_member_id(playerIdx);
	m_virtualClientIdx = playerIdx * 1000000;
	return true;
}

void cClientSession::SetPlayer( const uint64 accountIdx , General::ParticipantProfile& player , Server::ParticipantProfileInternal& playerExt )
{
	m_account_idx = accountIdx;
	m_player = player;
	m_playerExt = playerExt;
	m_virtualClientIdx = player.member_id() * 1000000;
}

void cClientSession::SetAvatars( std::map<int , General::AvatarProfile>& avatars )
{
	m_avatars = avatars;

	/*for ( auto& _avatar : avatars ) {
		
		m_avatars.insert( std::pair<int , General::AvatarProfile>( _avatar.avatar_ref_id(), _avatar ) );
	}*/
}

void cClientSession::SetRecords( PmNet::Ledger records )
{
	m_records = records;
}

//const wchar_t* cClientSession::GetNickNameW() const
//{
//	if(m_pAccount)
//	{
//		return m_pAccount->AccountNickNameW;
//	}
//
//	return nullptr;
//}

void cClientSession::SessionLogout( UINT Entity , BOOL bForce )
{
	this->m_roomOutReason = "SessionLogOut";

	// Context 유효성 체크
	if ( Entity != 0 && m_pContext != nullptr )
	{
		if ( m_pContext->GetEntity() != Entity )
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
				"cClientSession::SessionLogout Failed. Entity [%u] != Entity[%u]" , m_pContext->GetEntity() , Entity );
			return;
		}
	}

	/*if ( GetPinballGame() != nullptr )
	{
		LeavePinball( false );
	}*/

	bool bPendingPushed = false;

	if ( m_roomNumber != 0 )
	{
		cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( m_roomNumber );

		if ( pGameRoom != nullptr )
		{
			const uint64 playerIdx = GetPlayerIdx();

			// 관전자거나 유저가 아닌 경우 즉시 퇴장 처리
			if ( pGameRoom->isWatcher( playerIdx ) || !pGameRoom->isPlayer( playerIdx ) )
			{
				NetLib::cSingleton<cGameRoomManager>::GetInstance()->RemoveWatcherResource( this );
				bPendingPushed = true;
			}
			else
			{
				// 플레이어가 대기 상태인 방이면 바로 내보냄
				if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting )
				{
					if ( !NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( this ) )
					{
						SetRoomOutReserve();
						SetPendingTime();
					}
					bPendingPushed = true;
				}
				else
				{
					// 게임 진행 중이면 나가기 예약 처리
					SetRoomOutReserve();
					SetPendingTime();
					bPendingPushed = true;
				}
			}
		}
		else
		{
			// 방 정보가 없다면 바로 펜딩
			bPendingPushed = true;
		}
	}
	else
	{
		// 방에 없을 경우 바로 펜딩
		bPendingPushed = true;
	}

	// 펜딩 세션 이동은 여기서 딱 한 번만 호출
	if ( bPendingPushed )
	{
		NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->PushPendingSession( GetPlayerIdx() );
	}
}


size_t cClientSession::CodedSessionInfo(char* pCodedBuffer, size_t nBufferSize)
{
	if (pCodedBuffer == nullptr || nBufferSize == 0)
	{
		return 0;
	}

	size_t nCodedSize = NetLib::cSession::CodedSessionInfo(pCodedBuffer, nBufferSize);

	if (nCodedSize < nBufferSize)
	{

		//StringCbPrintfA(pCodedBuffer + nCodedSize, nBufferSize - nCodedSize, "AID [ %I64d ]  HeroIDX [ %I64d ] Hero NickName [ %s ]", GetAccountIDX(), GetHeroIDX(), GetHeroNickName());

		StringCbLengthA(pCodedBuffer, nBufferSize, &nCodedSize);
	}

	return nCodedSize;
}

// 방에 참가해 있는 상태라면 false 를 리턴 합니다.
// 단 관전 상태라면 바로 내보냅니다.
bool cClientSession::isRemoveReady()
{
	cGameRoom* pGameRoom = NetLib::cSingleton<cGameRoomManager>::GetInstance()->FindPlayingGameRoom( m_roomNumber );
	if ( pGameRoom == nullptr ) return true;

	if ( pGameRoom->isWatcher( GetPlayerIdx() ) ) {
		//pGameRoom->RemoveWatcher( GetPlayerIdx() );
		return true;
	}

	// 방이 대기 상태여도 즉시 내보낸다.
	if ( pGameRoom->GetRoomStatus() == General::RoomState::RoomState_Waiting ) {
		return true;
	}

	if ( !pGameRoom->isPlayer( GetPlayerIdx() ) )
		return true;

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::isRemoveReady failed. roomNumber [ %d ]" , pGameRoom->GetRoomNumber() );
	
	return false;
}

void cClientSession::CopyPlayer(General::ParticipantProfile* pPlayer)
{
	pPlayer->CopyFrom(m_player);
}

std::future<BOOL> cClientSession::SavePlayer()
{
	return QueryManager::PlayerUpdateByQuery( this->m_player , this->m_playerExt );
}

std::future<BOOL> cClientSession::SaveMoney()
{
	return QueryManager::PlayerUpdateByQuery( this->m_player , this->m_playerExt );
}

std::future<BOOL> cClientSession::AvatarsGet()
{
	return std::async( std::launch::async , [this]() {
		return QueryManager::AvatarsGet( this->m_player.member_id(), this->m_avatars );
	} );
}

std::future<BOOL> cClientSession::PlayerSetAvatar( const int& avatar_id )
{
	const uint64& playerIdx = m_player.member_id();
	m_player.set_equipped_avatar_id( avatar_id );

	return std::async( std::launch::async , [playerIdx , avatar_id]() {
		return QueryManager::PlayerSetAvatar( playerIdx , avatar_id );
	} );
}

//std::future<BOOL> cClientSession::PlayerSetSubPasswd()
//{
//	const uint64& playerIdx = m_player.member_id();
//
//	return std::async( std::launch::async , [this]() {
//		/*QueryManager* queryManager = new QueryManager();
//		return queryManager->PlayerSetSubPasswd( this->m_player.member_id() , this->m_player.sub_passwd_activated(), this->m_player.sub_passwd() );*/
//		return TRUE;
//	} );
//}

BOOL cClientSession::hasAvatar( const int& avatar_id )
{
	auto iter = m_avatars.find( avatar_id );
	if ( iter == m_avatars.end() )
		return FALSE;

	return TRUE;
}

General::AvatarProfile cClientSession::GetAvatar( const int& avatar_id )
{
	auto iter = m_avatars.find( avatar_id );
	if ( iter != m_avatars.end() )
		return iter->second;

	return General::AvatarProfile::default_instance();
}

General::AvatarProfile cClientSession::SetAvatar( const int& avatar_id )
{
	m_player.set_equipped_avatar_id( avatar_id );

	return NetLib::cSingleton<cDataLoader>::GetInstance()->GetAvatar( avatar_id );
}

void cClientSession::CopyAvatars( PmNet::MarketPurchaseRS& response )
{
	for ( auto& avatarPair : m_avatars ) {
		auto add_avatar = response.add_refresh_skins();
		if ( add_avatar != nullptr ) {
			add_avatar->CopyFrom( avatarPair.second );
		}
	}
}

//BOOL cClientSession::UpdateLobby( BOOL& updateMailBox )
//{
//	m_send_limit_popup = false; // 로비로 간 경우에믄 초기화 해줌
//
//	updateMailBox = FALSE;
//	BOOL isUpdated = false;
//	// 플레이어의 로비 갱신 시간을 확인한다.
//	std::string daily_string = GetDailyRefreshTime();
//	std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( daily_string );
//	std::time_t today_start_time = TimeUtils::TodayStartTimeTM();
//	
//	// 초기화 시간이 지났으면 Daily 초기화 할 것들을 처리한다.
//	if ( cur_refresh_time <= today_start_time )
//	{
//		isUpdated = true;
//		UpdateRecordDailyRefresh();
//
//	/*	std::vector<std::future<BOOL>> results;
//		results.push_back( std::async( std::launch::async , &cClientSession::UpdateLowBadukiRecords , this ) );
//
//
//		results.push_back( std::async( std::launch::async , &cClientSession::UpdateLowBadukiRecords , this ) );
//		results.push_back( std::async( std::launch::async , &cClientSession::UpdateHoldemRecords , this ) );
//		results.push_back( std::async( std::launch::async , &cClientSession::UpdateBaccaraRecords , this ) );
//		results.push_back( std::async( std::launch::async , &cClientSession::UpdateBlackjackRecords , this ) );
//		results.push_back( std::async( std::launch::async , &cClientSession::UpdateSlotRecords , this ) );
//
//		for ( auto& result : results )
//			result.wait();*/
//
//
//		std::vector<std::future<BOOL>> results;
//
//		results.push_back( UpdateLowBadukiRecords(true) );
//		results.push_back( UpdateHoldemRecords( true ) );
//		results.push_back( UpdateBaccaraRecords( true ) );
//		results.push_back( UpdateBlackjackRecords( true ) );
//		results.push_back( UpdateSlotRecords() );
//
//		for ( auto& result : results )
//			result.wait();
//
//		UpdateQuestsDailyRefresh();
//
//		// 오늘 0시로 셋팅한다.
//		const auto& tomorrowStartTm = TimeUtils::TomorrowStartTimeTM();
//		const std::string& dailyRefreshTime = TimeUtils::TMToString( tomorrowStartTm );
//		SetDailyRefreshTime( dailyRefreshTime );
//
//		UpdateDailyExpireDateTime();
//
//		UpdateDailyLostLimit();
//
//		// 메일 박스 갱신 클라이언트에
//		if ( UpdateAttendance() ) {
//			updateMailBox = TRUE;
//		}
//
//		// 리필 카운트 초기화
//		if ( m_player.chip_refill_uses() > 0 || m_player.coin_refill_uses() > 0 )
//		{
//			m_player.set_chip_refill_uses( 0 );
//			m_player.set_coin_refill_uses( 0 );
//		}
//
//		// 로우바둑이, 홀덤 1일 획득 재화 초기화
//		ClearTodayGetMoney();
//
//		// 친구 일일 삭제 카운트 초기화
//		SetDeleteFriendCount( 0 );
//
//		// 코인 한도 제한 카운트 수정
//		m_playerExt.set_coin_limit_mail_count( 0 );
//
//		std::future<BOOL> result = SavePlayer();
//		result.wait();
//
//		if ( result.get() )
//		{
//		}
//		else
//		{
//
//		}
//	}
//
//
//	// 월별 초기화 처리할 것들
//	std::string monthly_string = GetMonthlyRefreshTime();
//	std::time_t monthly_refresh_time = TimeUtils::StringToTimeTM( monthly_string );
//	std::string this_month_start_string = TimeUtils::ThisMonthStartTimeString();
//	std::time_t this_month_start_time = TimeUtils::StringToTimeTM( this_month_start_string );
//
//	// 월별 처리 할 일들 처리
//	if ( monthly_refresh_time <= this_month_start_time )
//	{
//		isUpdated = true;
//		UpdateMonthlyLostLimit();
//	}
//	return isUpdated;
//}

BOOL cClientSession::UpdateLobby( BOOL& updateMailBox )
{
	m_send_limit_popup = false; // 로비로 간 경우에믄 초기화 해줌

	updateMailBox = FALSE;
	BOOL isUpdated = false;

	// 00시 초기화
	std::string daily_string = GetDailyRefreshTime();
	std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( daily_string );
	std::time_t today_start_time = TimeUtils::TodayStartTimeTM();
	if ( cur_refresh_time <= today_start_time )
	{
		// 리필 카운트 초기화
		if ( m_player.chip_refill_uses() > 0 || m_player.coin_refill_uses() > 0 )
		{
			m_player.set_chip_refill_uses( 0 );
			m_player.set_coin_refill_uses( 0 );
		}
		m_playerExt.set_coin_limit_mail_count( 0 ); // 코인 한도 제한 카운트 초기화

		isUpdated = true;
		UpdateRecordDailyRefresh();

		std::vector<std::future<BOOL>> results;

		results.push_back( UpdateHoldemRecords( true ) );


		//results.push_back( UpdatePinballRecords( true ) );
		//results.push_back( UpdateRouletteRecords( true ) );


		for ( auto& result : results )
			result.wait();
		ClearTodayGetMoney();

		if ( UpdateAttendance() ) {
			updateMailBox = TRUE;
		}
		UpdateDailyLostLimit();
		UpdateQuestsDailyRefresh();
		SetDeleteFriendCount( 0 );

		const auto& tomorrowStartTm = TimeUtils::TomorrowStartTimeTM();
		cur_refresh_time = tomorrowStartTm;
		const std::string& dailyRefreshTime = TimeUtils::TMToString( tomorrowStartTm );
		SetDailyRefreshTime( dailyRefreshTime );
	}

	// 주간 보상
	std::string ranking_reward_received_date = m_playerExt.ranking_reward_claimed_date();
	std::time_t ranking_reward_received_date_time = TimeUtils::StringToTimeTM( ranking_reward_received_date );
	std::string weelky_monday_date = TimeUtils::ThisMondayDate();
	std::time_t weelky_monday_date_time = TimeUtils::StringToTimeTM( weelky_monday_date );

	if ( true == ranking_reward_received_date.empty() )
	{
		isUpdated = true;
		m_playerExt.set_ranking_reward_claimed_date( weelky_monday_date );

		ranking_reward_received_date = m_playerExt.ranking_reward_claimed_date();
		ranking_reward_received_date_time = TimeUtils::StringToTimeTM( ranking_reward_received_date );
	}

	if ( weelky_monday_date_time > ranking_reward_received_date_time )
	{
		isUpdated = true;
		m_playerExt.set_ranking_reward_claimed_date( weelky_monday_date );

		//CheckSlotRankingEventReward();
	}

	// 월별 초기화 처리할 것들
	std::string monthly_string = GetMonthlyRefreshTime();
	std::time_t monthly_refresh_time = TimeUtils::StringToTimeTM( monthly_string );
	std::string this_month_start_string = TimeUtils::ThisMonthStartTimeString();
	std::time_t this_month_start_time = TimeUtils::StringToTimeTM( this_month_start_string );

	if ( monthly_refresh_time <= this_month_start_time )
	{
		isUpdated = true;
		UpdateMonthlyLostLimit();
	}
	if ( isUpdated )
	{
		UpdateDailyExpireDateTime();

		std::future<BOOL> result = SavePlayer();
		result.wait();
		
		if ( result.get() )
		{
		}
		else
		{
		
		}
	}
	return isUpdated;
}


// 칩 또는 코인이 적은 경우 자동 충전
// 칩은 최대 10회, 코인은 3회
// 팝업 띄워 주어야 한다. ( 얼마가 충전 되었고, 몇회가 남았는지 )
// 금고에 포함된 재화도 포함
void cClientSession::UpdateRefillMoney( PmNet::RefreshAtriumRS& response )
{
	cMoneyLogInstance logInstance( this , Server::AssetLedgerSource::AssetLedger_Refill );

	int chip_refill_count = m_player.chip_refill_uses();
	int coin_refill_count = m_player.coin_refill_uses();
	uint64 cur_chip = GetChip() + GetSafeChip();
	uint64 add_chip = 0;
	uint64 cur_coin = GetCoin() + GetSafeCoin();
	uint64 add_coin = 0;
	int max_chip_refill_count = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillDailyCount );
	uint64	refill_chip_limit = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillThreshold );
	// 칩 리필해 준다.
	if ( chip_refill_count < max_chip_refill_count && cur_chip < refill_chip_limit ) {

		switch ( GetMemberShipClass() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			add_chip = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillNormalAmount );
			break;
		case General::BenefitTier::BenefitTier_Standard:
			add_chip = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillStandardAmount );
			break;
		case General::BenefitTier::BenefitTier_Premium:
			add_chip = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_ChipRefillPremiumAmount );
			break;
		}

		SetChip( General::PlayCategory::PlayCategory_None , GetChip() + add_chip );
		m_player.set_chip_refill_uses( ++chip_refill_count );

		std::future<BOOL> result = SavePlayer();
		result.wait();

		if ( result.get() )
		{
			// 충전된 칩 add_chip , 남은 칩 리필 카운트 - chips_refill_count()
			response.set_refuel_stacks( add_chip );
			response.set_stack_refuel_left( max_chip_refill_count - m_player.chip_refill_uses() );
		}
		else
		{

		}
	}else
		response.set_stack_refuel_left( max_chip_refill_count - m_player.chip_refill_uses() );


	int max_coin_refill_count = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_CoinRefillDailyCount );
	uint64	refill_coin_limit = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_CoinRefillThreshold );
	// 코인 리필해 준다.
	if ( coin_refill_count < max_coin_refill_count && cur_coin < refill_coin_limit ) {

		add_coin = NetLib::cSingleton<cDataLoader>::GetInstance()->GetDefaultValue( Server::BaselineConfigKey::BaselineConfig_CoinRefillAmount );

		SetCoin( General::PlayCategory::PlayCategory_None , GetCoin() + add_coin );
		m_player.set_coin_refill_uses( ++coin_refill_count );

		std::future<BOOL> result = SavePlayer();
		result.wait();

		if ( result.get() )
		{
			// 충전된 칩 add_coin , 남은 칩 리필 카운트 - coin_refill_count()
			response.set_refuel_tokens( add_coin );
			response.set_token_refuel_left( max_coin_refill_count - m_player.coin_refill_uses() );
		}
		else
		{

		}
	}
	else
		response.set_token_refuel_left( max_coin_refill_count - m_player.coin_refill_uses() );
}

BOOL cClientSession::UpdateDailyExpireDateTime()  
{
	std::string daily_refresh_time = GetDailyRefreshTime();

	std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateDailyRefreshTimeUpdateQuery( GetPlayerIdx() , daily_refresh_time ) );
	result.wait();

	return result.get();
}

void cClientSession::CheckLoginReward()
{
	PmNet::SigninBountyRQ req;
	PmNet::SigninBountyRS res;
	const uint64& player_idx = GetPlayerIdx();
	std::string mail_indexs = "";

	for ( auto event : login_event_list )
	{

		auto eventinfo = res.add_drop_info_list();
		eventinfo->CopyFrom( event );
		mail_indexs = "";
		if ( false == receive_login_reward_list.contains( event.drop_name() ) ) {
			if ( !TimeUtils::isCurrentTimeWithin( event.start_ts() , event.end_ts() ) ) {
				continue;
			}

			std::vector<std::pair<std::string , string>> reward_list;
			parseRewards( reward_list , event.bounty() );

			for ( auto reward : reward_list ) {
				mail_indexs += std::format( "{}_" , SendReward( reward.first , reward.second , event.notice() ) );
			}

			if ( !mail_indexs.empty() ) {
				mail_indexs.pop_back(); // 마지막 콤마 제거
			}

			if ( !mail_indexs.empty() ) {
				auto result = QueryManager::InsertLoginRewardLog( player_idx , event.drop_name() , event.bounty() , mail_indexs );
				if ( FALSE == result )
				{
					//에러처리
				}
			}
			receive_login_reward_list.insert( event.drop_name() );
			res.add_got_drop_now( event.drop_name() );
		}
	}

	if ( req.req_kind() & General::LoginRewardAction::LoginReward_Initialize )
	{
		for ( const auto& receive : receive_login_reward_list )
		{
			res.add_got_drop_list( receive );
		}
	}
	std::string errorMessage = "";
	SendRequest( General::Packet_AttendanceClaim , res , General::ResultCode::Result_Success , errorMessage );
}

void cClientSession::parseRewards( std::vector<std::pair<std::string , std::string>>& result , const std::string& rewards )
{
	std::istringstream rewardsStream( rewards );
	std::string reward;

	// 슬래시('/')로 구분된 각 보상 항목을 처리
	while ( std::getline( rewardsStream , reward , '/' ) ) {
		std::istringstream rewardStream( reward );
		std::string name;
		std::string amountStr;
		if ( std::getline( rewardStream , name , ':' ) && std::getline( rewardStream , amountStr ) ) {
			result.emplace_back( name , amountStr );
		}
	}
}

// 한도 초과가 발생된 클라이언트에게 알림 팝업을 발송한다.
void cClientSession::SendLostLimitPopopOnResult()
{
	PmNet::LossCapBannerRS _response;
	std::string errorMessage;
	SendRequest( General::Packet_LossCapPopup , _response , General::ResultCode::Result_Success , errorMessage );
}

void cClientSession::SendMoneyLimitPopopOnResult()
{
	if ( m_limit_game_type != General::PlayCategory::PlayCategory_None &&
		m_limit_money_type != General::AssetKind::AssetKind_None ) {

		// 발송했으면 로비로 가기 전까지는 또 보내지 않음
		if ( m_send_limit_popup )
			return;

		PmNet::FundCapBannerRS _response;
		_response.set_cap_match_kind( m_limit_game_type );		// 한도 초과 발생 게임 타입
		_response.set_cap_fund_kind( m_limit_money_type );	// 머니 타입
		_response.set_inbox_check( m_mail_box_check );		// 메일 박스로 지급 알림

		if ( GetMemberShipClass() == General::BenefitTier::BenefitTier_Basic ) {

			_response.set_tier_buy_flag( true );
		}

		std::string errorMessage;
		SendRequest( General::Packet_BalanceCapPopup , _response , General::ResultCode::Result_Success , errorMessage );
		m_limit_game_type = General::PlayCategory::PlayCategory_None;
		m_limit_money_type != General::AssetKind::AssetKind_None;
	}
}

std::string cClientSession::GetIp()
{
	return m_ip;
	std::string ip_address;

	NetLib::cInterfaceIocpContext* pContext = GetContext();
	if ( pContext != nullptr ) {
		ip_address = ::ConvertIP( pContext->GetIP() );
	}
	return ip_address;
}

// wire 에서 protobuf 시그니처 완전 제거.


bool cClientSession::SendRequest( const UINT nCommand , google::protobuf::Message& _message , General::ResultCode errorCode , std::string& errorMessage )
{
	uint64 playerIdx = GetPlayerIdx();

	NetLib::cInterfaceIocpContext* pContext = GetContext();
	if ( pContext == nullptr || !pContext->IsActive() )
		return false;



	auto sendBuffer = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
	if ( sendBuffer == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::SendRequest is Failed. Serialize Failed. #1" );
		return false;
	}

	if ( !_message.SerializeToArray( sendBuffer->DataBuffer , sizeof( sendBuffer->DataBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::SendRequest is Failed. Serialize Failed. #2" );
		return false;
	}

	size_t nSize = static_cast< size_t >( _message.ByteSizeLong() );

	PmNet::PktBase _response;
	_response.set_payload( sendBuffer->DataBuffer , nSize );
	_response.set_payload_size( static_cast< google::protobuf::uint32 >( nSize ) );
	//_response.set_err_kind( General::ResultCode::Result_Success );
	_response.set_err_kind( errorCode );
	_response.set_errtag( errorMessage );

	if ( !_response.SerializeToArray( sendBuffer->SerializeBuffer , sizeof( sendBuffer->SerializeBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cClientSession::SendRequest is Failed. Serialize Failed. #3" );
		return false;
	}

	UINT uiSize = static_cast< UINT >( _response.ByteSizeLong() );

	NetLib::cPacketStack Packet( CSNet::E_PROTOCOL::E_TCP );
	Packet.Make( nCommand , sendBuffer->SerializeBuffer , uiSize , 0 );

	return pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
}

void cClientSession::GetStepBettingExceptBindBet( const Server::PlayPhase& gameStep , std::vector<General::TableAction>& returnBetting )
{
	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>::CPair* pPair = m_bettings.Lookup( gameStep );
	if ( pPair == nullptr || pPair->m_value == nullptr )
		return;

	std::vector<General::TableAction> bettings;
	bettings = *(pPair->m_value);

	for ( const auto& bet : bettings ) {
		switch ( bet )
		{
		case General::TableAction::TableAction_SmallBlind:
		case General::TableAction::TableAction_BigBlind:
		//case General::TableAction::TableAction_GiantBlind:
		//case General::TableAction::TableAction_RoyalBlind:
		break;
		default:
		{
			returnBetting.push_back( bet );
		}
		break;
		}
	}
}

BOOL cClientSession::GetFreeCharge( const General::AssetKind& _monty_type , uint64& _get_money , uint64& _charged )
{
	_get_money = 0;

	const uint64 _player_idx = GetPlayerIdx();

	// FreeCharger 생성
	auto freeCharger = NetLib::cSingleton<cFreeCharger>::ExistsInstance();
	if ( freeCharger == nullptr ) {
		freeCharger = NetLib::cSingleton<cFreeCharger>::GetInstance();

		auto lottery_datas = NetLib::cSingleton<cDataLoader>::GetInstance()->GetLotteryDatas();
		for ( auto& lottery : lottery_datas ) {
			freeCharger->PushRate( lottery );
		}
	}

	switch ( _monty_type )
	{
	case General::AssetKind::AssetKind_Coin:
	{
		// 코인 업데이트 시간이 지났는지 확인
		std::string _refresh_time = m_player.coin_free_charge_reset_at();
		std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( _refresh_time );

		std::time_t now = std::time( nullptr );

		if ( now < cur_refresh_time )
			return FALSE;

		// 코인 무료 충전은 6시간 마다
		//const int _hours = 10;
		//std::string _next_coin_refresh_time = TimeUtils::GetCurrentKSTDateTimeString( _hours );
		const int _minute = 15;
		std::string _next_coin_refresh_time = TimeUtils::GetCurrentKSTDateTimeString_AddMinutes( _minute );

		// 코인 확률 처리
		_get_money = freeCharger->GetFreeMoney( General::AssetKind::AssetKind_Coin );

		m_player.set_coin_free_charge_reset_at( _next_coin_refresh_time );
		uint64 newCoin = GetCoin() + _get_money;
		_charged = SetFreeCoin( General::PlayCategory::PlayCategory_None , newCoin );

		if ( FALSE == QueryManager::PlayerMoneyUpdate( GetPlayerIdx() , GetCoin() , GetChip() , GetRakeBack() , GetGem() , 0 , 0 , GetPaidGem() ) ) {

			// 재화 갱신 실패 로그 출력
		}

		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateFreeChargeUpdateQuery( _player_idx , _monty_type , _next_coin_refresh_time ) );
		result.wait();
		return result.get();
	}
	break;
	case General::AssetKind::AssetKind_Chip:
	{
		// 코인 업데이트 시간이 지났는지 확인
		std::string _refresh_time = m_player.chip_free_charge_reset_at();
		std::time_t cur_refresh_time = TimeUtils::StringToTimeTM( _refresh_time );

		std::time_t now = std::time( nullptr );

		if ( now < cur_refresh_time )
			return FALSE;

		// 칩 무료 충전은 2시간 마다
		const int _hours = 2;
		std::string _next_coin_refresh_time = TimeUtils::GetCurrentKSTDateTimeString( _hours );

		// 칩 확률 처리
		_get_money = freeCharger->GetFreeMoney( General::AssetKind::AssetKind_Chip );

		m_player.set_chip_free_charge_reset_at( _next_coin_refresh_time );
		uint64 newChip = GetChip() + _get_money;
		_charged = SetFreeChip( General::PlayCategory::PlayCategory_None , newChip );

		UpdateQuests( General::TaskTrigger::TaskTrigger_ChipReach );
		if ( FALSE == QueryManager::PlayerMoneyUpdate( GetPlayerIdx() , GetCoin() , GetChip() , GetRakeBack() , GetGem() , 0 , 0 , GetPaidGem() ) ) {

			// 재화 갱신 실패 로그 출력
		}

		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateFreeChargeUpdateQuery( _player_idx , _monty_type , _next_coin_refresh_time ) );
		result.wait();
		return result.get();
	}
	break;
	}

	return FALSE;
}

void cClientSession::SetCoinFriend( General::PlayCategory gameType , uint64 coin )
{
	uint64 playerIdx = GetPlayerIdx();
	uint64 currentSafeCoin = m_player.vault_coins();

	if ( coin + currentSafeCoin > GetMaxHoldingCoin() ) {

		// TODO : 짜투리 코인은 탑 클라스만 5회 우편으로 보내준다.
		uint64 remainCoin = coin + currentSafeCoin - GetMaxHoldingCoin();
		m_player.set_wallet_coins( GetMaxHoldingCoin() - currentSafeCoin );

		// 게임결과 한도 초과 팝업 처리용
		if ( remainCoin > 0 ) {

			m_limit_game_type = gameType;
			m_limit_money_type = General::AssetKind::AssetKind_Coin;
		}
	}
	else {
		m_player.set_wallet_coins( coin );
	}
}

void cClientSession::SetCoin( General::PlayCategory gameType , uint64 coin ) {
	uint64 playerIdx = GetPlayerIdx();
	uint64 currentSafeCoin = m_player.vault_coins();
	uint64 maxHoldingCoin = GetMaxHoldingCoin( m_player.membership_tier() );
	int daily_coin_limit_mail_count = m_playerExt.coin_limit_mail_count();
	if ( daily_coin_limit_mail_count >= 5 )
	{
		BOOL updateMailBox;
		UpdateLobby( updateMailBox );
		daily_coin_limit_mail_count = m_playerExt.coin_limit_mail_count();
	}
	if ( GetMemberShipClass() != m_player.membership_tier() ) { // 맴버쉽이 만료된경우
		if ( coin + currentSafeCoin > maxHoldingCoin ) {
			uint64 remainCoin = coin + currentSafeCoin - maxHoldingCoin;
			m_player.set_wallet_coins( maxHoldingCoin - currentSafeCoin );

			if ( remainCoin > 0 ) {
				m_limit_game_type = gameType;
				m_limit_money_type = General::AssetKind::AssetKind_Coin;
			}
			General::InboxState mailstate = NetLib::cSingleton<cDataLoader>::GetInstance()->GetMailState();
			if ( m_player.membership_tier() == General::BenefitTier::BenefitTier_Premium ||
				NetLib::cSingleton<cLoungeEvent>::GetInstance()->HasLoungeEvent() ) {

				if ( daily_coin_limit_mail_count < 5 ) {
					uint64 mailAmount = 100000000; // 1억
					uint64 mailsToSend = remainCoin / mailAmount;
					uint64 lastMailAmount = remainCoin% mailAmount;

					if ( mailsToSend > 10 )
						mailsToSend = 10;

					if ( remainCoin > 5000000000 )
					{
						string GameType = protoutil::cProtoUtil::GetEnumString( gameType );
						string mailIdx2 = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
						cMessageLog messageLogInstance( this , 40201 );
						messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , GameType , std::to_string( remainCoin ) );
						messageLogInstance.SetMsgid( mailIdx2 );
						messageLogInstance.SetReward( "ERROR LOG" );
						return;
					}
					
					std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 3 );
					std::u8string room_title = u8"초과금이 지급되었습니다.";
					std::string_view room_title_utf8View( reinterpret_cast< const char* >( room_title.data() ) , room_title.size() );
					
					for ( uint64 i = 0; i < mailsToSend; ++i ) {
						std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
							QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin ,
								General::InboxReason::InboxReason_BalanceLimit , mailAmount , 0 , room_title_utf8View.data() , 0 , getLimitTimeString , (int)mailstate ) );
						result.wait();

						if ( false == result.get() ) {
							// 쿼리 실패에 대한 처리
						}

						string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
						cMessageLog messageLogInstance( this , 40201 );
						messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( mailAmount ) );
						messageLogInstance.SetMsgid( mailIdx );
						messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
					}

					if ( lastMailAmount > 0 ) {
						std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
							QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin ,
								General::InboxReason::InboxReason_BalanceLimit , lastMailAmount , 0 , room_title_utf8View.data() , 0 , getLimitTimeString,( int ) mailstate ) );
						result.wait();

						if ( false == result.get() ) {
							// 쿼리 실패에 대한 처리
						}

						string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
						cMessageLog messageLogInstance( this , 40201 );
						messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( lastMailAmount ) );
						messageLogInstance.SetMsgid( mailIdx );
						messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
					}

					m_mail_box_check = true;

					m_playerExt.set_coin_limit_mail_count( daily_coin_limit_mail_count + 1 );

					std::future<BOOL> result = SavePlayer();
					result.wait();

					if ( false == result.get() ) {
						// 쿼리 실패에 대한 처리
					}
				}
			}
			else {
				// TODO : 버려지는 코인에 대한 로그 누적
			}

		}
		else {
			//m_limit_game_type = gameType;
			//m_limit_money_type = General::AssetKind::AssetKind_Coin;
			m_player.set_wallet_coins( coin );
		}
	}
	else {
		if ( coin + currentSafeCoin > GetMaxHoldingCoin() ) {
			uint64 remainCoin = coin + currentSafeCoin - GetMaxHoldingCoin();
			m_player.set_wallet_coins( GetMaxHoldingCoin() - currentSafeCoin );
			General::InboxState mailstate = NetLib::cSingleton<cDataLoader>::GetInstance()->GetMailState();
			if ( remainCoin > 0 ) {
				m_limit_game_type = gameType;
				m_limit_money_type = General::AssetKind::AssetKind_Coin;
			}

			if ( GetMemberShipClass() == General::BenefitTier::BenefitTier_Premium ||
				NetLib::cSingleton<cLoungeEvent>::GetInstance()->HasLoungeEvent() ) {

				if ( daily_coin_limit_mail_count < 5 ) {
					uint64 mailAmount = 100000000; // 1억
					uint64 mailsToSend =  remainCoin / mailAmount;
					uint64 lastMailAmount = remainCoin% mailAmount;
					std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 3 );
					std::u8string room_title = u8"초과금이 지급되었습니다.";
					std::string_view room_title_utf8View( reinterpret_cast< const char* >( room_title.data() ) , room_title.size() );

					if ( mailsToSend > 10 )
						mailsToSend = 10;

					if ( remainCoin > 5000000000 )
					{
						string GameType = protoutil::cProtoUtil::GetEnumString( gameType );
						string mailIdx2 = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
						cMessageLog messageLogInstance( this , 40201 );
						messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , GameType , std::to_string( remainCoin ) );
						messageLogInstance.SetMsgid( mailIdx2 );
						messageLogInstance.SetReward( "ERROR LOG" );
						return;
					}

					for ( uint64 i = 0; i < mailsToSend; ++i ) {
						std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
							QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin ,
								General::InboxReason::InboxReason_BalanceLimit , mailAmount , 0 , room_title_utf8View.data() , 0 , getLimitTimeString , ( int ) mailstate ) );
						result.wait();

						if ( false == result.get() ) {
							// 쿼리 실패에 대한 처리
						}

						string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
						cMessageLog messageLogInstance( this , 40201 );
						messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( mailAmount ) );
						messageLogInstance.SetMsgid( mailIdx );
						messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
					}

					if ( lastMailAmount > 0 ) {
						std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
							QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin ,
								General::InboxReason::InboxReason_BalanceLimit , lastMailAmount , 0 , room_title_utf8View.data() , 0 , getLimitTimeString , ( int ) mailstate ) );
						result.wait();

						if ( false == result.get() ) {
							// 쿼리 실패에 대한 처리
						}

						string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
						cMessageLog messageLogInstance( this , 40201 );
						messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( lastMailAmount ) );
						messageLogInstance.SetMsgid( mailIdx );
						messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
					}

					m_mail_box_check = true;

					m_playerExt.set_coin_limit_mail_count( daily_coin_limit_mail_count + 1 );

					std::future<BOOL> result = SavePlayer();
					result.wait();

					if ( false == result.get() ) {
						// 쿼리 실패에 대한 처리
					}
				}
			}
			else {
				// TODO : 버려지는 코인에 대한 로그 누적
			}

		}
		else {
			m_player.set_wallet_coins( coin );
		}
	}
}
void cClientSession::SetCoinNoLimit( General::PlayCategory gameType , uint64 coin ) {

	uint64 playerIdx = GetPlayerIdx();
	m_player.set_wallet_coins( coin );
	
	std::future<BOOL> result = SavePlayer();
	result.wait();

	if ( false == result.get() ) {
		// 쿼리 실패에 대한 처리
	}
}


bool cClientSession::SetExpiredCoin( General::PlayCategory gameType , uint64 coin )
{
	uint64 playerIdx = GetPlayerIdx();
	uint64 currentSafeCoin = m_player.vault_coins();
	uint64 beforecoin = m_player.wallet_coins();
	uint64 currentTotalCoin = beforecoin + currentSafeCoin; // 현재 총 보유 코인
	uint64 requestedTotalCoin = coin + currentSafeCoin; // 요청된 총 코인 (기존 소지금 무시하고 새로 설정)

	// 한도 초과 체크: 요청 코인 OR 현재 총보유 중 큰 값이 한도를 초과하는 경우
	uint64 maxHoldingCoin = GetMaxHoldingCoin();
	uint64 targetTotalCoin = std::max( requestedTotalCoin , currentTotalCoin );

	if ( targetTotalCoin > maxHoldingCoin ) {
		uint64 remainCoin = 0;
		uint64 finalCoin = 0;

		// 케이스 1: 새 코인 지급 요청이면서 한도 초과
		if ( coin > 0 && requestedTotalCoin > maxHoldingCoin ) {
			remainCoin = requestedTotalCoin - maxHoldingCoin;
			finalCoin = maxHoldingCoin - currentSafeCoin;
		}
		// 케이스 2: 한도가 줄어들어서 현재 보유량이 초과된 경우
		else if ( coin == 0 && currentTotalCoin > maxHoldingCoin ) {
			remainCoin = currentTotalCoin - maxHoldingCoin;
			finalCoin = maxHoldingCoin - currentSafeCoin;
		}
		// 케이스 3: 기타 초과 상황
		else {
			remainCoin = targetTotalCoin - maxHoldingCoin;
			finalCoin = maxHoldingCoin - currentSafeCoin;
		}

		// 음수 방지: 금고가 한도보다 큰 경우 소지금은 0으로 설정
		if ( currentSafeCoin >= maxHoldingCoin ) {
			finalCoin = 0;
			remainCoin = currentTotalCoin; // 모든 소지금을 우편으로
		}

		m_player.set_wallet_coins( finalCoin );

		uint64 mailAmount = 100000000; // 1??
		uint64 mailsToSend = remainCoin / mailAmount;
		uint64 lastMailAmount = remainCoin % mailAmount;

		if ( mailsToSend > 10 )
			mailsToSend = 10;
		General::InboxState mailstate = NetLib::cSingleton<cDataLoader>::GetInstance()->GetMailState();
		if ( remainCoin > 5000000000 )
		{
			string GameType = protoutil::cProtoUtil::GetEnumString( gameType );
			string mailIdx2 = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40201 );
			messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , GameType , std::to_string( remainCoin ) );
			messageLogInstance.SetMsgid( mailIdx2 );
			messageLogInstance.SetReward( "ERROR LOG" );
			return false;
		}

		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 3 );
		std::u8string room_title = u8"멤버십 만료로 인한 소지한도 초과금이 지급되었습니다.";
		std::string_view room_title_utf8View( reinterpret_cast< const char* >( room_title.data() ) , room_title.size() );

		for ( uint64 i = 0; i < mailsToSend; ++i ) {
			std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
				QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin ,
					General::InboxReason::InboxReason_BalanceLimit , mailAmount , 0 , room_title_utf8View.data() , 0 , getLimitTimeString ) );
			result.wait();

			if ( false == result.get() ) {
			}

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40201 );
			messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( mailAmount ) );
			messageLogInstance.SetMsgid( mailIdx );
			messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
		}

		if ( lastMailAmount > 0 ) {
			std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
				QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeCoin ,
					General::InboxReason::InboxReason_BalanceLimit , lastMailAmount , 0 , room_title_utf8View.data() , 0 , getLimitTimeString , ( int ) mailstate ) );
			result.wait();

			if ( false == result.get() ) {
			}

			string mailIdx = QueryManager::GetLastMailBoxIndexQuery( m_player.member_id() );
			cMessageLog messageLogInstance( this , 40201 );
			messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( lastMailAmount ) );
			messageLogInstance.SetMsgid( mailIdx );
			messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
		}

		m_mail_box_check = true;
		std::future<BOOL> result = SavePlayer();
		result.wait();
		if ( false == result.get() )
		{

		}

	}
	else {
		// 한도 내 처리: 요청된 코인 설정
		m_player.set_wallet_coins( coin );
	}
	return beforecoin != m_player.wallet_coins(); // 코인이 변경되었으면 true
}


//게임타입은 혹시나 나중에 로그를위해!
uint64 cClientSession::SetFreeCoin( General::PlayCategory gameType , uint64 coin )
{
	uint64 currentSafeCoin = m_player.vault_coins();

	if ( coin + currentSafeCoin > GetMaxHoldingCoin() ) {
		uint64 total_coin = GetMaxHoldingCoin() - currentSafeCoin;
		m_player.set_wallet_coins( total_coin );
		return total_coin;

	}
	else {
		m_player.set_wallet_coins( coin );
	}
	return coin;
}


// 칩 소유 제한 처리
bool cClientSession::SetExpiredChip( General::PlayCategory gameType ) {
	uint64 playerIdx = GetPlayerIdx();
	uint64 beforechip = m_player.wallet_chips();
	uint64 currentSafeChip = m_player.vault_chips();
	
	// 각 한도 체크
	uint64 maxHoldingChip = GetMaxHoldingChip();        // 소지 한도
	uint64 maxHoldingSafeChip = GetMaxHoldingSafeChip(); // 금고 한도
	
	// 현재 상태에서 각각 한도 체크
	bool chipOverLimit = beforechip > maxHoldingChip;
	bool safeChipOverLimit = currentSafeChip > maxHoldingSafeChip;
	
	// 케이스 1: 둘 다 한도 초과 - 각각 우편으로 발송
	if ( chipOverLimit && safeChipOverLimit ) {
		// 소지칩 한도 초과분 우편 발송
		uint64 chipExcess = beforechip - maxHoldingChip;
		m_player.set_wallet_chips( maxHoldingChip );
		SendChipToMail( playerIdx, chipExcess, u8"멤버십 만료로 인한 소지한도 초과금이 지급되었습니다." );
		
		// 금고칩 한도 초과분 우편 발송
		uint64 safeChipExcess = currentSafeChip - maxHoldingSafeChip;
		m_player.set_vault_chips( maxHoldingSafeChip );
		SendChipToMail( playerIdx, safeChipExcess, u8"멤버십 만료로 인한 금고한도 초과금이 지급되었습니다." );
		
		m_mail_box_check = true;
		SavePlayer().wait();
		return true;
	}
	
	// 케이스 2: 소지칩만 한도 초과 - 금고로 이동 후 넘치면 우편
	else if ( chipOverLimit && !safeChipOverLimit ) {
		uint64 chipExcess = beforechip - maxHoldingChip;
		uint64 safeChipSpace = maxHoldingSafeChip - currentSafeChip; // 금고 여유공간
		
		if ( chipExcess <= safeChipSpace ) {
			// 금고에 모두 들어감
			m_player.set_wallet_chips( maxHoldingChip );
			m_player.set_vault_chips( currentSafeChip + chipExcess );
		} else {
			// 금고를 가득 채우고 나머지는 우편
			m_player.set_wallet_chips( maxHoldingChip );
			m_player.set_vault_chips( maxHoldingSafeChip );
			uint64 remainChip = chipExcess - safeChipSpace;
			SendChipToMail( playerIdx, remainChip, u8"멤버십 만료로 인한 소지한도 초과금이 지급되었습니다." );
			m_mail_box_check = true;
		}
		
		SavePlayer().wait();
		return true;
	}
	
	// 케이스 3: 금고칩만 한도 초과 - 소지로 이동 후 넘치면 우편
	else if ( !chipOverLimit && safeChipOverLimit ) {
		uint64 safeChipExcess = currentSafeChip - maxHoldingSafeChip;
		uint64 chipSpace = maxHoldingChip - beforechip; // 소지 여유공간
		
		if ( safeChipExcess <= chipSpace ) {
			// 소지에 모두 들어감
			m_player.set_vault_chips( maxHoldingSafeChip );
			m_player.set_wallet_chips( beforechip + safeChipExcess );
		} else {
			// 소지를 가득 채우고 나머지는 우편
			m_player.set_vault_chips( maxHoldingSafeChip );
			m_player.set_wallet_chips( maxHoldingChip );
			uint64 remainChip = safeChipExcess - chipSpace;
			SendChipToMail( playerIdx, remainChip, u8"멤버십 만료로 인한 금고한도 초과금이 지급되었습니다." );
			m_mail_box_check = true;
		}
		
		SavePlayer().wait();
		return true;
	}
	
	// 케이스 4: 둘 다 한도 안 - 아무것도 안함
	return false; // 변경 사항 없음
}

// 우편 발송 헬퍼 함수
void cClientSession::SendChipToMail( uint64 playerIdx, uint64 chipAmount, const std::u8string& titleText ) {
	if ( chipAmount == 0 ) return;
	
	std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 3 );
	std::string_view title_utf8View( reinterpret_cast< const char* >( titleText.data() ) , titleText.size() );
	
	uint64 mailAmount = 1000000000000; // 1조 (TableServer)
	uint64 mailsToSend = chipAmount / mailAmount;
	uint64 lastMailAmount = chipAmount % mailAmount;
	
	if ( mailsToSend > 10 )
		mailsToSend = 10;
	
	// 1조씩 우편 발송
	for ( uint64 i = 0; i < mailsToSend; ++i ) {
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
			QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip ,
				General::InboxReason::InboxReason_BalanceLimit , mailAmount , 0 , title_utf8View.data() , 0 , getLimitTimeString ) );
		result.wait();
		
		if ( false == result.get() ) {
			// 쿼리 실패에 대한 처리
		}
		
		string mailIdx = QueryManager::GetLastMailBoxIndexQuery( playerIdx );
		cMessageLog messageLogInstance( this , 40201 );
		messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( mailAmount ) );
		messageLogInstance.SetMsgid( mailIdx );
		messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
	}
	
	// 나머지 금액 우편 발송
	if ( lastMailAmount > 0 ) {
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD ,
			QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip ,
				General::InboxReason::InboxReason_BalanceLimit , lastMailAmount , 0 , title_utf8View.data() , 0 , getLimitTimeString ) );
		result.wait();
		
		if ( false == result.get() ) {
			// 쿼리 실패에 대한 처리
		}
		
		string mailIdx = QueryManager::GetLastMailBoxIndexQuery( playerIdx );
		cMessageLog messageLogInstance( this , 40201 );
		messageLogInstance.SetData( TimeUtils::GetCurrentDateTime() , std::to_string( m_playerExt.coin_limit_mail_count() ) , std::to_string( lastMailAmount ) );
		messageLogInstance.SetMsgid( mailIdx );
		messageLogInstance.SetReward( "InboxReason_BalanceLimit" );
	}
}

void cClientSession::SetChip( General::PlayCategory gameType , uint64 chip ) {
	uint64 playerIdx = GetPlayerIdx();
	uint64 beforechip = m_player.wallet_chips();
	uint64 maxHoldingChip = GetMaxHoldingChip( m_player.membership_tier() );

	if ( GetMemberShipClass() != m_player.membership_tier() ) { // 맴버쉽이 만료된 경우
		if ( chip > maxHoldingChip ) {
			m_player.set_wallet_chips( maxHoldingChip );
			uint64 remainChip = chip - maxHoldingChip;

			if ( remainChip > 0 ) {
				m_limit_game_type = gameType;
				m_limit_money_type = General::AssetKind::AssetKind_Chip;
			}


			std::future<BOOL> result = SavePlayer();
			result.wait();

			if ( false == result.get() ) {
				// 쿼리 실패에 대한 처리
			}
		}
		else {
			m_limit_game_type = gameType;
			m_limit_money_type = General::AssetKind::AssetKind_Chip;
			m_player.set_wallet_chips( chip );
		}
	}
	else {
		if ( chip > maxHoldingChip ) {
			m_player.set_wallet_chips( maxHoldingChip );
			uint64 remainChip = chip - maxHoldingChip;

			if ( remainChip > 0 ) {
				m_limit_game_type = gameType;
				m_limit_money_type = General::AssetKind::AssetKind_Chip;
			}


			std::future<BOOL> result = SavePlayer();
			result.wait();

			if ( false == result.get() ) {
				// 쿼리 실패에 대한 처리
			}
		}
		else {
			m_player.set_wallet_chips( chip );
		}
	}

	if ( beforechip < m_player.wallet_chips() )
		UpdateQuests( General::TaskTrigger::TaskTrigger_ChipReach );
}


//게임타입은 혹시나 나중에 로그를위해!
uint64 cClientSession::SetFreeChip( General::PlayCategory gameType , uint64 chip )
{

	if ( chip > GetMaxHoldingChip() ) {

		uint64 remainChip = chip - GetMaxHoldingChip();
		m_player.set_wallet_chips( GetMaxHoldingChip() );


		// TODO : 버려지는 칩에 대한 로그 누적
		return GetMaxHoldingChip();

	}
	else {

		m_player.set_wallet_chips( chip );
		return chip;
	}
	return chip;
}
void cClientSession::parsePeriod( const std::string& count , uint64_t& index , uint64_t& period ) {
	size_t dashPos = count.find( '-' );

	if ( dashPos != std::string::npos ) {
		// "1-30" 형식인 경우
		try {
			index = std::stoull( count.substr( 0 , dashPos ) );
			period = std::stoull( count.substr( dashPos + 1 ) );
		}
		catch ( const std::invalid_argument& e ) {
			//throw std::invalid_argument( "Invalid format: " + count );
		}
		catch ( const std::out_of_range& e ) {
			//throw std::out_of_range( "Number out of range: " + count );
		}
	}
	else {
		// "1" 형식인 경우
		try {
			index = std::stoull( count );
			period = 7;//기본7일
		}
		catch ( const std::invalid_argument& e ) {
			//throw std::invalid_argument( "Invalid format: " + count );
		}
		catch ( const std::out_of_range& e ) {
			//throw std::out_of_range( "Number out of range: " + count );
		}
	}
}

std::string cClientSession::SendReward( const std::string& reward_type , const std::string& count ,const std::string& message )
{
	char* endPtr;
	uint64 i_count = 0;
	uint64 period = 0;
	if ( reward_type == "AVATAR" )
	{
		parsePeriod( count , i_count , period );
	}else
	i_count = std::strtoull( count.c_str() , &endPtr , 10 );//10진수로 변환
	uint64 inserted_id;
	

	// ITEM 상품 지급, KickOut Ticket 뿐이 없다.
	if ( reward_type == "KICKOUTTICKET" ) {
		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_KickTicket , General::InboxReason::InboxReason_PushReward , i_count , 0 , message , 0 , getLimitTimeString) , inserted_id);
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}
	
	// 유료 칩 
	if ( reward_type == "PAIDCHIP" ) {

		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_PaidChip , General::InboxReason::InboxReason_PushReward ,i_count , 0 , message , 0 , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}

	// 유료 코인 
	if ( reward_type == "PAIDCOIN" ) {
		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_PaidCoin , General::InboxReason::InboxReason_PushReward ,i_count, 0 , message , 0 , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}

	// 유료 다이아 
	if ( reward_type == "PAIDGEM" ) {
		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_PaidGem , General::InboxReason::InboxReason_PushReward , i_count, 0 , message , 0 , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}

	// 무료 칩 
	if ( reward_type == "CHIP" ) {

		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_PushReward , i_count , 0 , message , 0 , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}

	// 무료 코인 
	if ( reward_type == "COIN" ) {
		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_FreeCoin , General::InboxReason::InboxReason_PushReward , i_count , 0 , message , 0 , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}

	// 무료 다이아 
	if ( reward_type == "GEM" ) {
		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_FreeGem , General::InboxReason::InboxReason_PushReward , i_count , 0 , message , 0 , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}

	//아바타
	if ( reward_type == "AVATAR" ) {
		std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
		std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync_ID( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( m_player.member_id() , General::GrantItemKind::GrantItem_Avatar , General::InboxReason::InboxReason_PushReward , 1 , i_count , message , period , getLimitTimeString ) , inserted_id );
		result.wait();

		if ( FALSE == result.get() )
		{

		}
	}


	//// 아바타 구매
	//if ( reward_type == "PICK" ) {
	//	int add_days = cDataLoader::ExtractNumberAfterDay( value );

	//	//_productData.set_avatar_id( item_id );
	//	//_productData.set_avatar_id( atoi( avatar_id.c_str() ) );
	//	//_productData.set_avatar_add_days( add_days );
	//}
	//if ( key.compare( "CLASS" ) == 0 ) {

	//	if ( value.compare( "REGULAR" ) == 0 ) {
	//		_productData.set_member_ship_class( General::BenefitTier::BenefitTier_Standard ); // 멤버쉽 클라스 REGULAR 아이템 아이디
	//	}
	//	else if ( value.compare( "TOP" ) == 0 ) {
	//		_productData.set_member_ship_class( General::BenefitTier::BenefitTier_Premium ); // 멤버쉽 클라스 TOP  아이템 아이디
	//	}
	//}
	//else if ( key.compare( "PERIODSEC" ) == 0 ) {

	//	int day = atoi( value.c_str() ) / 3600 / 24;
	//	_productData.set_membership_add_days( day );
	//}

	//// 멤버쉽 구매
	//if ( shopProduct.member_ship_class() != General::BenefitTier::BenefitTier_None ) {

	//	std::string expiry_string = m_player.membership_expires_at();
	//	std::time_t expiry_time = TimeUtils::StringToTimeTM( expiry_string );
	//	std::time_t now = std::time( nullptr );

	//	switch ( GetMemberShipClass() )
	//	{
	//	case General::BenefitTier::BenefitTier_Basic:
	//	{
	//		// 멤버쉽 변경
	//		m_player.set_membership_enabled( true );
	//		m_player.set_membership_tier( shopProduct.member_ship_class() );

	//		// 오늘 부터 ~ 기간 처리
	//		std::time_t new_expiry_time = TimeUtils::AddDays( now , shopProduct.membership_add_days() );
	//		std::string _new = TimeUtils::TMToString( new_expiry_time );
	//		m_player.set_membership_expires_at( _new );

	//		std::future<BOOL> result = QueryManager::PlayerUpdateAsync( m_player );
	//		result.wait();

	//		if ( FALSE == result.get() ) {

	//			// 플레이어 클래스 갱신 실패

	//		}
	//		else
	//		{
	//			updatePlayer = true;

	//			_product_data.set_member_ship_class( shopProduct.member_ship_class() );
	//		}

	//		// 칩 보상 지급
	//		// 레귤러 130억
	//		// 탕ㅂ 240 억
	//		uint64 reward_money = 0;
	//		if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Standard )
	//		{
	//			reward_money = 13000000000;
	//		}
	//		else if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Premium )
	//		{
	//			reward_money = 24000000000;
	//		}

	//		if ( reward_money > 0 ) {

	//			std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 30 );
	//			std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_PurchaseReward , reward_money , 0 , "" , getLimitTimeString ) );
	//			result.wait();

	//			if ( FALSE == result.get() )
	//			{

	//			}
	//			else
	//			{
	//				updateMailBox = true;

	//				_product_data.set_chip( reward_money );
	//			}
	//		}
	//	}
	//	break;
	//	case General::BenefitTier::BenefitTier_Standard:
	//	case General::BenefitTier::BenefitTier_Premium:
	//	{
	//		if ( shopProduct.member_ship_class() == GetMemberShipClass() ) {

	//			if ( expiry_time > now ) {

	//				// 기간 연장
	//				std::time_t new_expiry_time = TimeUtils::AddDays( expiry_time , shopProduct.avatar_add_days() );
	//				std::string _new = TimeUtils::TMToString( new_expiry_time );
	//				m_player.set_membership_expires_at( _new );

	//				std::future<BOOL> result = QueryManager::PlayerUpdateAsync( m_player );
	//				result.wait();

	//				if ( FALSE == result.get() ) {

	//					// 플레이어 클래스 갱신 실패

	//				}
	//				else
	//				{
	//					updatePlayer = true;

	//					_product_data.set_member_ship_class( shopProduct.member_ship_class() );
	//				}
	//			}
	//			else {

	//				// 오늘 부터 ~ 기간 처리
	//				std::time_t new_expiry_time = TimeUtils::AddDays( now , shopProduct.avatar_add_days() );
	//				std::string _new = TimeUtils::TMToString( new_expiry_time );
	//				m_player.set_membership_expires_at( _new );

	//				std::future<BOOL> result = QueryManager::PlayerUpdateAsync( m_player );
	//				result.wait();

	//				if ( FALSE == result.get() ) {

	//					// 플레이어 클래스 갱신 실패

	//				}
	//				else
	//				{
	//					updatePlayer = true;

	//					_product_data.set_member_ship_class( shopProduct.member_ship_class() );
	//				}
	//			}
	//		}
	//		// 멤버쉽이 있는데, 다른 멤버쉽 구매 => 구매 처리 못함
	//		else
	//		{
	//		}

	//		// 칩 보상 지급
	//		// 레귤러 130억
	//		// 탕ㅂ 240 억
	//		uint64 reward_money = 0;
	//		if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Standard )
	//		{
	//			reward_money = 13000000000;
	//		}
	//		else if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Premium )
	//		{
	//			reward_money = 24000000000;
	//		}

	//		if ( reward_money > 0 ) {

	//			std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 30 );
	//			std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_PurchaseReward , reward_money , 0 , "" , getLimitTimeString ) );
	//			result.wait();

	//			if ( FALSE == result.get() )
	//			{

	//			}
	//			else
	//			{
	//				updateMailBox = true;

	//				_product_data.set_chip( reward_money );
	//			}
	//		}

	//	}
	//	break;
	//	}

	//}


	return std::to_string(inserted_id);
}

void cClientSession::SyncFriend_Online( cClientSession* pClientSession, General::ContactState status , std::string channelid )
{
	// 로비의 친구 상태 갱신
	{
		Server::SyncFriendInfoStatus syncFriendInfoStatus;
		syncFriendInfoStatus.set_player_idx( pClientSession->GetPlayerIdx() );
		syncFriendInfoStatus.set_friend_status( status );
		syncFriendInfoStatus.set_player_chip( pClientSession->GetChip() );
		syncFriendInfoStatus.set_player_coin( pClientSession->GetCoin() );
		syncFriendInfoStatus.set_player_safe_chip( pClientSession->GetSafeChip() );
		syncFriendInfoStatus.set_player_safe_coin( pClientSession->GetSafeCoin() );
		syncFriendInfoStatus.set_playing_channel_id( channelid );
		General::ParticipantProfile* responsePlayer = syncFriendInfoStatus.mutable_player_data();
		responsePlayer->CopyFrom( pClientSession->GetPlayer() );
		
		// 각 로비서버들에게도 쏘아줌
		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( syncFriendInfoStatus.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SlotLogin SerializeToArray Failed." );
		}

		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeContactStateSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncFriendInfoStatus.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

			// 로비서버 전송 실패
		}
	}

	// 친구 매니저 내 캐쉬 갱신
	PmNet::MateDetail friendInfo;
	if ( General::ContactState::ContactState_Hidden != status )
		friendInfo.set_mate_state( status );
	else
	{
		auto _friendInfo = NetLib::cSingleton<cFriendManager>::GetInstance()->GetFriendInfo( GetPlayerIdx() );
		if ( _friendInfo != nullptr )
			friendInfo.set_mate_state( _friendInfo->mate_state() );
	}

	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	friendInfo.set_node_id( configReader->SID_FOR_MANAGE );
	friendInfo.set_active_ch_token( channelid );
	// 플레이어 데이터 카피
	auto add_player = friendInfo.mutable_member_info();
	add_player->CopyFrom( GetPlayer() );

	// 플레이어 전적 데이터 카피
	auto records = friendInfo.mutable_ledger();
	CopyRecords( records );


	NetLib::cSingleton<cFriendManager>::GetInstance()->SetFriendInfo( friendInfo );
}

void cClientSession::SyncFriend_Online( bool bSyncFriendInfoStatus, cClientSession* pClientSession , General::ContactState status , std::string channelid , int32 serverid )
{
	if ( pClientSession->GetSessionStatus() == E_SESSION_STATUS::E_SESSION_STATUS_PENDING )
		return;

	// 로비의 친구 상태 갱신
	{
		Server::SyncFriendInfoStatus syncFriendInfoStatus;
		syncFriendInfoStatus.set_player_idx( pClientSession->GetPlayerIdx() );
		syncFriendInfoStatus.set_friend_status( status );
		syncFriendInfoStatus.set_player_chip( pClientSession->GetChip() );
		syncFriendInfoStatus.set_player_coin( pClientSession->GetCoin() );
		syncFriendInfoStatus.set_player_safe_chip( pClientSession->GetSafeChip() );
		syncFriendInfoStatus.set_player_safe_coin( pClientSession->GetSafeCoin() );
		syncFriendInfoStatus.set_playing_channel_id( channelid );
		General::ParticipantProfile* responsePlayer = syncFriendInfoStatus.mutable_player_data();
		responsePlayer->CopyFrom( pClientSession->GetPlayer() );

		// 서버아이디는 값이 있는 경우에만 갱신해 준다.
		if ( serverid != 0 )
			syncFriendInfoStatus.set_server_id( serverid );

		if ( true == bSyncFriendInfoStatus )
			NetLib::cSingleton<cFriendManager>::GetInstance()->SyncFriendInfoStatus( syncFriendInfoStatus );

		// 각 로비서버들에게도 쏘아줌
		GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
		pGOOGLE_PROTO_BUFFER->Clear();
		if ( syncFriendInfoStatus.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "SlotLogin SerializeToArray Failed." );
		}

		E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket(
						E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeContactStateSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncFriendInfoStatus.ByteSizeLong() );

		if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

			// 로비서버 전송 실패
		}
	}

	//// 친구 매니저 내 캐쉬 갱신
	//PmNet::MateDetail friendInfo;
	//if ( General::ContactState::ContactState_Hidden != status )
	//	friendInfo.set_mate_state( status );

	//const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	//friendInfo.set_node_id( configReader->SID_FOR_MANAGE );
	//friendInfo.set_active_ch_token( channelid );
	//// 플레이어 데이터 카피
	//auto add_player = friendInfo.mutable_member_info();
	//add_player->CopyFrom( GetPlayer() );

	//// 플레이어 전적 데이터 카피
	//auto records = friendInfo.mutable_ledger();
	//CopyRecords( records );


	//NetLib::cSingleton<cFriendManager>::GetInstance()->SetFriendInfo( friendInfo );
}

void cClientSession::SendMyOfflineToFriendMap()
{
	this->SyncFriend_Online( true, this , General::ContactState::ContactState_Offline , "" );
}


//void cClientSession::LeavePinball( bool b_roomout )
//{
//	PmNet::ExitMarbleRS response;
//	std::string errorMessage;
//
//	BOOL updateMailBox;
//	UpdateLobby( updateMailBox );
//	SetCoin( General::PlayCategory_Pinball , GetCoin() );
//
//	{
//		const bool& has_no_free_spins = HasNotAckedpinballs();
//		response.set_pending_marbles( has_no_free_spins );
//		// 플레이어 코인 갱신
//		response.set_member_token( GetCoin() );
//		//SendRequest( General::Packet_ReelClose , response , General::ResultCode::Result_Success , errorMessage );
//		//리플레이저장
//		const PmNet::ReplayMarbleRS& replaypinballres = GetRePlayPinballRes();
//		if ( replaypinballres.marble_kind() != 0 && replaypinballres.has_marbles_res() )
//			NetLib::cSingleton<cRedisController>::GetInstance()->SetPlayerPinball( GetPlayerIdx() , replaypinballres.marble_kind() , replaypinballres );
//
//		// 메모리 해제
//		auto reelGame = GetPinballGame();
//		if ( reelGame == nullptr ) {
//			std::string errorString = "{ GameType_Pinball instance is null, when LeavePinball ] PlayerIdx [ " + std::to_string( GetPlayerIdx() ) + " ]";
//			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//			return;
//		}
//		SetPinballGame( nullptr );
//		if ( HasMoneyLimit() ) {
//			SendMoneyLimitPopopOnResult();
//		}
//
//		// 플레이어의 슬롯 게임 리소스 초기화
//		LeavePinballGame();
//
//		std::vector<std::future<BOOL>> results;
//		results.push_back( SavePlayer() );
//		results.push_back( UpdatePinballRecords() );
//
//		for ( int n = 0; n < results.size(); ++n ) {
//			auto& result = results[ n ];
//			result.wait();
//
//			std::string errorString = n == 0 ? "[ LeavePinball SavePlayer " : "[ LeavePinball UpdatePinballRecords ";
//
//			if ( FALSE == result.get() ) {
//
//				errorString += " Failed ] PlayerIdx [ " + std::to_string( GetPlayerIdx() ) + " ]";
//				//errorString = protoutil::cProtoUtil::ErrorCodeString( errorString.c_str() );
//				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//			}
//			else {
//				errorString += " Success ] PlayerIdx [ " + std::to_string( GetPlayerIdx() ) + " ]";
//				//errorString = protoutil::cProtoUtil::ErrorCodeString( errorString.c_str() );
//				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
//			}
//		}
//	}
//
//	// 모든 처리가 끝난후에 클라이언트에게 전송한다.
//	if ( b_roomout )
//		SendRequest( Common::GMsg_LeavePinball , response , General::ResultCode::Result_Success , errorMessage );
//
//
//
//}