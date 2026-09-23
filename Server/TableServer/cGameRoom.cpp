#include "cGameRoom.h"
#include "..\Include\Netlib\Common/cSingleton.h"
#include "..\Include\Netlib\IOCP/cIocpConnector.h"
#include "..\Include\Netlib\Network/cContextPooler.h"
#include "..\Include\Netlib\Manager/cThreadManager.h"
#include "..\Include\Netlib\Manager/cCommandQueueManager.h"
#include "..\Include\Netlib\Manager/ServerManager.h"
#include "..\Include\Netlib\Manager/cSessionManager.h"
#include "..\Include\Netlib\Manager/cWebServerManager.h"
#include "..\Include\Netlib\Queue/cCommandQueue.h"
#include "..\Include\Netlib\Queue/cLogQueue.h"
#include "..\Include\Netlib\Queue/cWebQueue.h"
#include "..\Include\Netlib\Session/cSession.h"
#include "..\Include/Netlib/Network/cPacketStack.h"

#include "..\Include\Netlib\Scheduler\cScheduler.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cInstancePacketParser.h"
#include "cClientSession.h"
#include "cProtoUtil.h"
#include "cConfigReader.h"
#include "cFriendManager.h"
#include "cMaintenanceManager.h"

#include <format>
#include <string_view>

std::vector<std::u8string> cGameRoom::lowBadugi_titles;
std::vector<std::u8string> cGameRoom::holdem_titles;
std::vector<std::u8string> cGameRoom::blackjack_titles;

int cGameRoom::s_CurRoomCnt = 0;
NetLib::cCriticalSection cGameRoom::s_CurRoomLock;

cGameRoom::cGameRoom()
{
	Init();
}

cGameRoom::~cGameRoom()
{
	Destroy();
}

void cGameRoom::Init()
{
	s_CurRoomLock.Lock();

	++s_CurRoomCnt;
	m_nRoomNumber = s_CurRoomCnt;
	m_bTestRoom = false;
	m_bOpenningRoom = false;
	s_CurRoomLock.Unlock();
}

void cGameRoom::Destroy()
{
	Clear();
}

void cGameRoom::InitializeTitles()
{
	// 방목록 미리 생성
	lowBadugi_titles = {
	   u8"신나고 재미있는 로우바둑이~",
	   u8"바둑이는 내가 최고 고수!",
	   u8"내가 바로 우리 동네 호구?!",
	   u8"우리집 바둑이가 좋아하는 로우바둑이"
	};

	holdem_titles = {
		u8"에어라인 들고 Come on~",
		u8"오늘은 승리의 Party day~!",
		u8"짜릿하게 홀덤 한 판 즐겨요!",
		u8"오늘도 즐거운 홀덤 라이프"
	};

	blackjack_titles = {
		u8"오늘의 목표는 딜러 버스트!",
		u8"킹왕짱! 블랙잭",
		u8"진짜 승부는 블랙잭으로",
		u8"오늘 하루도 즐겁게 놀아 보아요~"
	};
}

void cGameRoom::Clear()
{
	m_mapPlayers.clear();
	m_mapWatchers.clear();
	m_mapCUsers.clear();

	m_roomInfo.Clear();// 방 특성 초기화
	roomPasswd.clear();	// 방 비밀번호

	// 게임 인터페이스도 정리 한다.
	if ( m_gameInterface != nullptr ) {
		m_gameInterface->ResetGame();
		m_gameInterface = nullptr;
	}

	// Clear 할때 방번호를 셋팅해 버린다.
	m_roomInfo.set_room_no( m_nRoomNumber );

	m_room_title.clear();
}

cGameRoom* cGameRoom::CreateRoom(const int RoomNum)
{
	cGameRoom* pRoom = new cGameRoom();
	if (pRoom)
	{
		NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
		if (pServerManager == nullptr)
		{
			assert(false && "cChatRoom::CreateRoom is Failed. ServerManager is nullptr");
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cChatRoom::CreateRoom is Failed. ServerManager is nullptr");
			return nullptr;
		}

		TServerConfiguration* pServerConfig = pServerManager->GetConfiguration();
		if (pServerConfig == nullptr)
		{
			assert(false && "cChatRoom::CreateRoom is Failed. TServerConfiguration is nullptr");
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cChatRoom::CreateRoom is Failed. TServerConfiguration is nullptr");
			return nullptr;
		}

		pRoom->SetRoomNum(RoomNum);
		pRoom->SetCommandThread(RoomNum % pServerConfig->nCommandThreadCnt);
	}
	return pRoom;
}

// 방 세부 정보
void cGameRoom::GetRoomInfo( General::RoomListEntry& roomListInfo )
{
	// m_roomInfo
	roomListInfo.set_room_no( m_nRoomNumber );

	// 서버 아이디를 보내준다.
	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
	roomListInfo.set_server_node_id( configReader->SID_FOR_MANAGE );

	General::PlayCategory gameType = m_roomInfo.play_category();
	
	if ( m_room_title.empty() ) {

		int random_title_array = rand() % 4;

		switch ( gameType )
		{
		case General::PlayCategory::PlayCategory_TexasHoldem:
			//room_title = u8"누구나 재미있게 홀덤 한판";
			m_room_title = holdem_titles[ random_title_array ];
			break;
		}
	}

	std::string_view room_title_utf8View( reinterpret_cast< const char* >( m_room_title.data() ) , m_room_title.size() );
	roomListInfo.set_room_name( room_title_utf8View.data() );

	roomListInfo.set_access_mode( m_roomInfo.access_mode() );
	roomListInfo.set_asset_kind( m_roomInfo.asset_kind() );
	roomListInfo.set_seed_amount( m_roomInfo.seed_amount() ); // 아직 방에 코인 타입이 없음
	roomListInfo.set_rule_profile( m_roomInfo.rule_profile() );
	roomListInfo.set_seat_limit( GetMaxRoomPlayerCnt() );
	roomListInfo.set_seated_count( GetJoinedCnt() );
	roomListInfo.set_room_state( GetRoomStatus() );

	switch ( gameType )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		roomListInfo.set_bet_policy( m_roomInfo.bet_policy() );
	}
	break;
	/*case Common::GameType::GameType_Roulette:
	{
	}*/
	break;
	}

	// TODO 코드 값에 따른 string 테이블 필요
	std::u8string join_condition;
	join_condition = u8"참여 가능";
	//switch ( m_roomInfo.seed_chip_type() )
	//{
	//case Common::SeedChipType_None:
	//	join_condition = u8".....";
	//	break;
	//case Common::SeedChipType_20:		// 20만, 소지칩 500만 이상 ~ 500억 미만
	//	join_condition = u8"20만 이상";
	//	break;
	//case Common::SeedChipType_100:		// 100만, 소지칩 50억 이상 ~ 2000억 미만
	//	join_condition = u8"100만 이상";
	//	break;
	//case Common::SeedChipType_5000:		// 5000만, 소지칩 500억 이상 ~ 무제한
	//	join_condition = u8"5000만 이상";
	//	break;
	//case Common::SeedChipType_10000:		// 1억, 소지칩 2000억 이상 ~ 무제한
	//	join_condition = u8"1억 이상";
	//	break;
	//case Common::SeedChipType_10_Bil:	// 100억, 10 Billion 소지칩 5500억 이상
	//	join_condition = u8"100억 이상";
	//	break;
	//case Common::SeedChipType_1_T:		// 1조, 소지칩 20조 이상 trillion
	//	join_condition = u8"1조 이상";
	//	break;
	//case Common::SeedChipType_Free:		// 자유, 소지칩 10만 이상
	//	join_condition = u8"자유 누구나 참여 가능";
	//	break;
	//}


	const Server::Channel& channel_data = GetGameInterface()->GetChannelData();

	roomListInfo.set_channel_code( channel_data.id() );

	// min, max 관련 방목록에 보내줄 부분 처리
	switch ( channel_data.game_type() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		roomListInfo.set_min_wager( channel_data.money_min() );
		roomListInfo.set_max_wager( channel_data.limit_bet_money() );
	}
	break;
	}

	// 방정보에 현재 남은 카드 덱 정보 추가
	auto gameInterface = GetGameInterface();
	if ( gameInterface != nullptr ) {
		roomListInfo.set_remaining_cards( gameInterface->GetRemainCardDeckCount() );
		roomListInfo.set_deck_card_total( gameInterface->GetTotalCardDeckCount() );
	}

	roomListInfo.set_entry_asset_min( channel_data.money_min() );

	std::string_view join_condition_utf8View( reinterpret_cast< const char* >( join_condition.data() ) , join_condition.size() );
	roomListInfo.set_entry_condition( join_condition_utf8View.data() );

	// 바카라에 대해서만 최근 기록을 response 에 복사
	

	/*if ( GetGameType() == Common::GameType::GameType_Roulette ) {
		auto hot = m_roomInfo.hot_numbers();
		auto cold = m_roomInfo.cold_numbers();
		auto recent = m_roomInfo.hit_numbers();
		for ( auto temp : hot ) 
			roomListInfo.add_hot_numbers( temp );
		for ( auto temp : cold ) 
			roomListInfo.add_cold_numbers( temp );
		for ( auto temp : recent ) 
			roomListInfo.add_hit_numbers( temp );
	}*/
}

// 방 세부 정보 가져 오기
void cGameRoom::GetGameRoomDetail( PmNet::ChamberEnterRS& roomJoinRes , bool is_watcher )
{
	auto game = GetGameInterface();
	if ( game == nullptr )
		return;

	General::PlayCategory _game_type = game->GetGameType();

	// 바카라는 필요 없다.
	switch ( _game_type )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		roomJoinRes.set_lead_idx( GetGameInterface()->GetBossPlayerIdx() );
		roomJoinRes.set_captain_idx( GetGameInterface()->GetMasterPlayerIdx() );
	}
	break;
	}

	roomJoinRes.set_chamber_no( GetRoomNumber() );
	m_roomInfo.set_play_phase( game->GetGameStep() );
	roomJoinRes.mutable_chamber_info()->CopyFrom( m_roomInfo );
	roomJoinRes.set_ch_token( GetGameInterface()->GetChannelData().id() );

	// 최근 기록을 response 에 복사
	auto histories = GetGameInterface()->GetRecentlyPlayedGames();
	for ( auto history : histories ) {
		General::WinningHandHistory* pJokbo = roomJoinRes.mutable_chamber_info()->add_winning_history();
		pJokbo->CopyFrom( history );
	}
	bool b_result = game->GetGameStep() == Server::PlayPhase::PlayPhase_Judgement ? true : false;
	auto isWonByForfeit = [&]() -> bool {
		if ( !b_result ) {
			return false; // result나 showdown 단계가 아니면 기권승 판단 불가
		}

		int alivePlayerCount = 0;
		std::vector<cClientSession*> players = game->GetPlayersSessionList();
		for ( auto player : players ) {
			if ( player != nullptr && !player->isDie() ) {
				alivePlayerCount++;
			}
		}
		return alivePlayerCount <= 1; // 살아있는 플레이어가 1명 이하면 기권승
	};

	bool wonByForfeit = isWonByForfeit();
	if ( _game_type == General::PlayCategory_TexasHoldem )	{
		auto h_game = dynamic_cast< cHoldem* >( game );
		if ( h_game->m_gameStep == Server::PlayPhase::PlayPhase_Judgement || h_game->m_gameStep == Server::PlayPhase::PlayPhase_HoldemShowdown )
		{
			if ( h_game->GetActivePlayers().size() != 1 )
				is_watcher = false;
		}
	}


	std::vector<cClientSession*> players = GetGameInterface()->GetPlayersSessionList();
	for ( auto player : players ) {
		General::ParticipantProfile* pPlayer = roomJoinRes.add_members();
		if ( player != nullptr ) {
			player->CopyPlayer( pPlayer );

			// 바두기는 아침, 점심, 저녁 카드 교환한 숫자까지 넣어준다.
			

			switch ( _game_type )
			{
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				// 플레이어 상태에 대한 알림 처리, 플레이 중일 때만 상태를 보내도록 변경한다.
				if ( m_roomInfo.room_state() == General::RoomState::RoomState_InPlay ) {

					if ( player->isAllIn() )
						pPlayer->set_all_in_state( true );

					if ( player->isDie() )
						pPlayer->set_folded_out( true );

					if ( player->GetReserved() )
						pPlayer->set_leave_reserved( true );

					if ( player->isSide() )
						pPlayer->set_side_bet_enabled( true );
				}
				for ( const auto& card : player->m_cards ) {
					auto pCard = pPlayer->add_held_cards();
					if ( m_gameInterface->GetGameStep() == Server::PlayPhase::PlayPhase_HoldemShowdown || m_gameInterface->GetGameStep() == Server::PlayPhase::PlayPhase_Judgement )
					{
						// 기권승일 때는 카드를 공개하지 않음
						if ( wonByForfeit ) {
							pCard->CopyFrom( General::PlayingCard::default_instance() );
						}
						else if ( !player->isDie() )
							pCard->CopyFrom( card );
						else
							pCard->CopyFrom( General::PlayingCard::default_instance() );

					}
					else
						pCard->CopyFrom( General::PlayingCard::default_instance() );
				}
			}
			break;
			}
			
			// 현재 토탈 베팅 금액
			pPlayer->set_round_wager_total( player->GetLostMoney() );
		}
		else {
			General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
			pPlayer->CopyFrom( emptyPlayer );
		}
	}

	std::vector<cClientSession*> resvervation_players = GetGameInterface()->GetReservationPlayers();
	for ( auto player : resvervation_players ) {
		General::ParticipantProfile* pPlayer = roomJoinRes.mutable_seat_reservation()->add_hold_members();
		if ( player != nullptr ) {
			player->CopyPlayer( pPlayer );
		}
		else {
			General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
			pPlayer->CopyFrom( emptyPlayer );
		}
	}

	std::vector<cClientSession*> participation_queue = GetGameInterface()->GetParticipationQueue();
	for ( auto player : participation_queue ) {
		if ( player == nullptr ) continue;

		roomJoinRes.mutable_seat_reservation()->add_engage_queue( player->GetPlayerIdx() );
	}
}


void cGameRoom::GetGameRoomDetailCheat( PmNet::ChamberEnterRS& roomJoinRes , bool is_watcher )
{
	auto game = GetGameInterface();
	if ( game == nullptr )
		return;

	General::PlayCategory _game_type = game->GetGameType();

	// 바카라는 필요 없다.
	switch ( _game_type )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		roomJoinRes.set_lead_idx( GetGameInterface()->GetBossPlayerIdx() );
		roomJoinRes.set_captain_idx( GetGameInterface()->GetMasterPlayerIdx() );
	}
	break;
	}

	roomJoinRes.set_chamber_no( GetRoomNumber() );
	m_roomInfo.set_play_phase( game->GetGameStep() );
	roomJoinRes.mutable_chamber_info()->CopyFrom( m_roomInfo );
	roomJoinRes.set_ch_token( GetGameInterface()->GetChannelData().id() );

	// 최근 기록을 response 에 복사
	auto histories = GetGameInterface()->GetRecentlyPlayedGames();
	for ( auto history : histories ) {
		General::WinningHandHistory* pJokbo = roomJoinRes.mutable_chamber_info()->add_winning_history();
		pJokbo->CopyFrom( history );
	}

	std::vector<cClientSession*> players = GetGameInterface()->GetPlayersSessionList();
	for ( auto player : players ) {
		General::ParticipantProfile* pPlayer = roomJoinRes.add_members();
		if ( player != nullptr ) {
			player->CopyPlayer( pPlayer );

			// 바두기는 아침, 점심, 저녁 카드 교환한 숫자까지 넣어준다.
			

			switch ( _game_type )
			{
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				// 플레이어 상태에 대한 알림 처리, 플레이 중일 때만 상태를 보내도록 변경한다.
				if ( m_roomInfo.room_state() == General::RoomState::RoomState_InPlay ) {

					if ( player->isAllIn() )
						pPlayer->set_all_in_state( true );

					if ( player->isDie() )
						pPlayer->set_folded_out( true );

					if ( player->GetReserved() )
						pPlayer->set_leave_reserved( true );

					if ( player->isSide() )
						pPlayer->set_side_bet_enabled( true );
				}
				for ( const auto& card : player->m_cards ) {
					auto pCard = pPlayer->add_held_cards();
						
					pCard->CopyFrom( card );
				}

			}
			break;
			}

			// 현재 토탈 베팅 금액
			pPlayer->set_round_wager_total( player->GetLostMoney() );
		}
		else {
			General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
			pPlayer->CopyFrom( emptyPlayer );
		}
	}

	std::vector<cClientSession*> resvervation_players = GetGameInterface()->GetReservationPlayers();
	for ( auto player : resvervation_players ) {
		General::ParticipantProfile* pPlayer = roomJoinRes.mutable_seat_reservation()->add_hold_members();
		if ( player != nullptr ) {
			player->CopyPlayer( pPlayer );
		}
		else {
			General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
			pPlayer->CopyFrom( emptyPlayer );
		}
	}

	std::vector<cClientSession*> participation_queue = GetGameInterface()->GetParticipationQueue();
	for ( auto player : participation_queue ) {
		if ( player == nullptr ) continue;

		roomJoinRes.mutable_seat_reservation()->add_engage_queue( player->GetPlayerIdx() );
	}
}

// 방 세부 정보 가져 오기
void cGameRoom::GetGameRoomDetailOnRejoin( PmNet::ChamberEnterRS& roomJoinRes , const uint64& rejoin_player_idx )
{
	auto game = GetGameInterface();
	if ( game == nullptr )
		return;
	// 바카라는 필요 없다.
	switch ( game->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		roomJoinRes.set_lead_idx( game->GetBossPlayerIdx() );
		roomJoinRes.set_captain_idx( game->GetMasterPlayerIdx() );
		roomJoinRes.mutable_reconnect_state()->set_kickoff_idx( game->Getfirstidx() );
	}
	break;
	}
	bool b_result = game->GetGameStep() == Server::PlayPhase::PlayPhase_Judgement ? true : false;
	
	// 기권승인지 판단 (다이하지 않은 플레이어가 1명만 남은 경우)
	auto isWonByForfeit = [&]() -> bool {
		if ( !b_result ) {
			return false; // result나 showdown 단계가 아니면 기권승 판단 불가
		}
		
		int alivePlayerCount = 0;
		std::vector<cClientSession*> players = game->GetPlayersSessionList();
		for ( auto player : players ) {
			if ( player != nullptr && !player->isDie() ) {
				alivePlayerCount++;
			}
		}
		return alivePlayerCount <= 1; // 살아있는 플레이어가 1명 이하면 기권승
	};
	
	bool wonByForfeit = isWonByForfeit();

	roomJoinRes.set_chamber_no( GetRoomNumber() );
	m_roomInfo.set_play_phase( game->GetGameStep() );
	roomJoinRes.mutable_chamber_info()->CopyFrom( m_roomInfo );
	roomJoinRes.set_ch_token( game->GetChannelData().id() );
	roomJoinRes.mutable_reconnect_state()->set_cur_wager_round( game->GetCurBettingRound() );

	// 최근 기록을 response 에 복사
	auto histories = game->GetRecentlyPlayedGames();
	for ( auto history : histories ) {
		General::WinningHandHistory* pJokbo = roomJoinRes.mutable_chamber_info()->add_winning_history();
		pJokbo->CopyFrom( history );
	}

	std::vector<cClientSession*> players = game->GetPlayersSessionList();
	for ( auto player : players ) {
		General::ParticipantProfile* pPlayer = roomJoinRes.add_members();
		if ( player != nullptr ) {
			player->CopyPlayer( pPlayer );

			// 바두기는 아침, 점심, 저녁 카드 교환한 숫자까지 넣어준다.
			

			// 플레이어 상태에 대한 알림 처리
			{
				if ( player->isAllIn() )
					pPlayer->set_all_in_state( true );

				if ( player->isDie() )
					pPlayer->set_folded_out( true );

				if ( player->GetReserved() )
					pPlayer->set_leave_reserved( true );

				if ( player->isSide() )
					pPlayer->set_side_bet_enabled( true );
			}
			
			// 현재 토탈 베팅 금액
			pPlayer->set_round_wager_total( player->GetLostMoney() );

			// 이번판에 베팅을 했는지 여부
			bool isBetDone = game->HasBetting( player );
			roomJoinRes.mutable_reconnect_state()->add_wager_done_list( isBetDone );

			// 핸드 카드 모드별로 다름
			if ( m_gameInterface->GetGameStep() != Server::PlayPhase::PlayPhase_Waiting && m_gameInterface->GetGameStep() != Server::PlayPhase::PlayPhase_StartReady )
			{
				switch ( game->GetGameType() )
				{
				case General::PlayCategory::PlayCategory_TexasHoldem:
				{
					for ( const auto& card : player->m_cards ) {
						auto pCard = pPlayer->add_held_cards();
						if ( player->GetPlayerIdx() == rejoin_player_idx ) {
								pCard->CopyFrom( card );
						}
						else
						{
							if ( m_gameInterface->GetGameStep() == Server::PlayPhase::PlayPhase_HoldemShowdown )
							{
								// 기권승일 때는 카드를 공개하지 않음
								if ( wonByForfeit ) {
									pCard->CopyFrom( General::PlayingCard::default_instance() );
								}
								else if ( !player->isDie() )
									pCard->CopyFrom( card );
								else
									pCard->CopyFrom( General::PlayingCard::default_instance() );

							}else
								pCard->CopyFrom( General::PlayingCard::default_instance() );
						}
					}
				}
				break;
				}
			}

		}
		else {
			General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
			pPlayer->CopyFrom( emptyPlayer );
			roomJoinRes.mutable_reconnect_state()->add_wager_done_list(false);
		}
	}

	std::vector<cClientSession*> resvervation_players = GetGameInterface()->GetReservationPlayers();
	for ( auto player : resvervation_players ) {
		General::ParticipantProfile* pPlayer = roomJoinRes.mutable_seat_reservation()->add_hold_members();
		if ( player != nullptr ) {
			player->CopyPlayer( pPlayer );
		}
		else {
			General::ParticipantProfile emptyPlayer = General::ParticipantProfile::default_instance();
			pPlayer->CopyFrom( emptyPlayer );
		}
	}

	std::vector<cClientSession*> participation_queue = GetGameInterface()->GetParticipationQueue();
	for ( auto player : participation_queue ) {
		if ( player == nullptr ) continue;

		roomJoinRes.mutable_seat_reservation()->add_engage_queue( player->GetPlayerIdx() );
	}
}

// 플레이중인 방의 삭제 조건 체크
// 홀덤, 바두기 = 플레이하는 사람이 다 나가면 관전자가 있더라도 방을 삭제한다.
// 바카라, 블랙잭 = 관전자만 있어도 방을 삭제하지 않는다.
bool cGameRoom::PlayingRoomRemoveCheck()
{
	auto gameInterface = GetGameInterface();
	if ( gameInterface == nullptr )
		return true;

	switch ( gameInterface->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	
	{
		if ( GetJoinedCnt() == 0 )
			return true;
	}
	break;
	
	}

	return false;
}

void cGameRoom::PushPotMoneyValue( const uint64 potMoney )
{
	m_pot_statistic.push_back( potMoney );

	if ( m_roomInfo.largest_pot() < potMoney ) {
		m_roomInfo.set_largest_pot( potMoney );
	}
}

// m_pot_statistic 기반으로 General::RoomSnapshot m_roomInfo 정보를 갱신한다.
void cGameRoom::CalcPotMoneyStatistic()
{
	auto optionalValue = GetMaxRecentValue();
	if ( optionalValue.has_value() )
	{
		uint64 recent_max_pot = GetMaxRecentValue().value();
		m_roomInfo.set_recent_largest_pot( recent_max_pot );
	}

	auto _average = GetAverageRecentValue();
	if ( _average > 0 ) {
		m_roomInfo.set_recent_avg_pot_amount( _average );
	}
}

// 최근 5개 중 가장 큰 값 반환
std::optional<uint64> cGameRoom::GetMaxRecentValue()
{
	// 데이터가 없는 경우
	if ( m_pot_statistic.empty() ) {
		return std::nullopt;
	}

	// 데이터가 5개 미만인 경우
	if ( m_pot_statistic.size() < 5 ) {
		return *std::max_element( m_pot_statistic.begin() , m_pot_statistic.end() );
	}

	// 데이터 벡터의 뒤에서 5개만 잘라내어서 사용
	std::vector<uint64> recentData( m_pot_statistic.end() - 5 , m_pot_statistic.end() );

	// 최댓값 반환
	return *std::max_element( recentData.begin() , recentData.end() );
}

// 최근 5개 평균 반환
uint64 cGameRoom::GetAverageRecentValue()
{
	// 데이터가 없는 경우
	if ( m_pot_statistic.empty() ) {
		return 0;
	}

	// 데이터가 5개 미만인 경우
	if ( m_pot_statistic.size() < 5 ) {
		double sum = std::accumulate( m_pot_statistic.begin() , m_pot_statistic.end() , 0.0 );
		return sum / m_pot_statistic.size();
	}

	// 데이터 벡터의 뒤에서 5개만 잘라내어서 사용
	std::vector<uint64> recentData( m_pot_statistic.end() - 5 , m_pot_statistic.end() );

	// 평균 계산
	double sum = std::accumulate( recentData.begin() , recentData.end() , 0.0 );
	return sum / recentData.size();
}

// 테스트 용도로만 쓰고 있음
void cGameRoom::SetDontEnter()
{
	m_bTestRoom = true;
	/*std::u8string join_condition;
	join_condition = u8"들어오지 마시오.";

	roomListInfo.set_channel_code( GetGameInterface()->GetChannelId() );

	std::string_view join_condition_utf8View( reinterpret_cast< const char* >( join_condition.data() ) , join_condition.size() );
	roomListInfo.set_entry_condition( join_condition_utf8View.data() );*/
}

size_t cGameRoom::GetJoinedCnt() const
{
	if ( m_gameInterface != nullptr )
	{
		int reservedCount = m_gameInterface->GetReservationPlayerPlayerCount();
		return m_gameInterface->GetMemberCnt()+ reservedCount;
	}

	return m_mapPlayers.size();
}

size_t cGameRoom::GetWatcherCnt() const
{
	return m_mapWatchers.size();
}

// 조인 가능하는지 판단은 이함수에서 처리 한다.
// 다른곳에서 하지 않는다.
// 관전자 처리 하는 경우에만 수정하도록 한다.
General::ResultCode cGameRoom::JoinPossible()
{
	/*if ( m_roomInfo.room_status() == General::RoomState::RoomState_None ||
		m_roomInfo.room_state() == General::RoomState::RoomState_InPlay )
		return General::ResultCode::Result_RoomStateNotWaiting;*/

	return GetJoinedCnt() < m_nMaxRoomPlayerCnt ? General::ResultCode::Result_Success : General::ResultCode::Result_RoomCapacityReached;
}

// 바카라의 경우 slotNumber 슬롯으로 조인, slotNumber 가 비어 있지 않다면 실패
// 이외에는 서버에서 슬롯을 결정해줌
General::ResultCode cGameRoom::RoomJoin(cClientSession* pClientSession, const int slotNumber)
{
	// 슬롯을 지정해서 들어오는 경우에는 JoinPossible 체크 해제
	if ( slotNumber == 0 ) {
		General::ResultCode errorCode = JoinPossible();
		if ( errorCode != General::ResultCode::Result_Success ) {
			return errorCode;
		}
	}
	
	const __int64 playerIdx = pClientSession->GetPlayerIdx();
	if (pClientSession->GetPlayerIdx() == 0) {
		return General::ResultCode::Result_SessionExpired; // 세션 만료로 응답
	}

	NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
	if (pContext == nullptr) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoom::RoomJoin Failed. Critical Failed Context nullptr PlayerIdx[ %I64d ]", playerIdx);
		return General::ResultCode::Result_SessionIdentityMismatch; // 세션 정보가 정확하지 않음
	}

	if ( m_gameInterface == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoom::RoomJoin Failed. IGame is nullptr PlayerIdx[ %I64d ]" , playerIdx );
		return General::ResultCode::Result_ActionRejected;
	}
	//cheat
	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ischeat( pClientSession->GetPlayerIdx() ) )
	{
		General::ResultCode errorCode = RoomJoinCheat( pClientSession );
	}
	General::ResultCode enterCode;
	auto iter = m_mapPlayers.find( playerIdx );
	if ( iter != m_mapPlayers.end() )
	{
		////더미가 있던경우
		//if ( m_gameInterface->SwapWithDummy( pClientSession ) )
		//{
		//	// 룸에 들어갔으니 Context에는 관리 스레드를 설정하고
		//	NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->ChangeCommandContext( static_cast< NetLib::cIocpContext* >( pContext ) , m_nManagedCommandThreadNumber );

		//	pClientSession->SetCommandQueueIndex( m_nManagedCommandThreadNumber );
		//	pClientSession->SetJoinedRoomNumber( m_nRoomNumber );
		//	pClientSession->SetJoinedRoomNumberBefore( m_nRoomNumber ); // 이전에 조인했던 방은 다시 못들어 가게 하려고 추가한다.
		//	return General::ResultCode::Result_Success;
		//}
		//else
		//{
		//	return General::ResultCode::Result_RoomEntryFailed;
		//}
			
		return General::ResultCode::Result_RoomEntryFailed;
	}
	else
	{
		enterCode = m_gameInterface->EnterSlot( pClientSession , slotNumber );
	}

	// 엔터 성공한 경우만 맵에 추가
	if ( enterCode == General::ResultCode::Result_Success ) {
		m_mapPlayers.insert( std::pair<__int64 , cClientSession*>( playerIdx , const_cast< cClientSession* >( pClientSession ) ) );

		GetWatcherCount();

		// 룸에 들어갔으니 Context에는 관리 스레드를 설정하고
		NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->ChangeCommandContext( static_cast< NetLib::cIocpContext* >( pContext ) , m_nManagedCommandThreadNumber );

		pClientSession->SetCommandQueueIndex( m_nManagedCommandThreadNumber );
		pClientSession->SetJoinedRoomNumber( m_nRoomNumber );
		pClientSession->SetJoinedRoomNumberBefore( m_nRoomNumber ); // 이전에 조인했던 방은 다시 못들어 가게 하려고 추가한다.

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
			"cGameRoom::RoomJoin Success. PlayerIdx[%I64d], Entity[%u], RoomNumber[%u], Max(%u/%u)" ,
			pClientSession->GetPlayerIdx() , pContext->GetEntity() , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );
	}

	return enterCode;
}


General::ResultCode cGameRoom::RoomJoinCheat( cClientSession* pClientSession , const int slotNumber )
{
	General::ResultCode enterCode = General::ResultCode::Result_Success;
	m_mapCUsers[ pClientSession->GetPlayerIdx() ] = pClientSession;
	return enterCode;
}

General::ResultCode cGameRoom::RoomReJoin( cClientSession* pClientSession )
{
	NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
	if ( pContext == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoom::RoomReJoin Failed. Critical Failed Context nullptr PlayerIdx[ %I64d ]" , pClientSession->GetPlayerIdx() );
		return General::ResultCode::Result_SessionIdentityMismatch; // 세션 정보가 정확하지 않음
	}

	if ( m_gameInterface == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoom::RoomReJoin Failed. IGame is nullptr PlayerIdx[ %I64d ]" , pClientSession->GetPlayerIdx() );
		return General::ResultCode::Result_ActionRejected;
	}

	auto iter = m_mapPlayers.find( pClientSession->GetPlayerIdx() );
	if ( iter != m_mapPlayers.end() )
	{
		cClientSession* pSearchingSession = iter->second;
		if ( pSearchingSession != nullptr )
		{
			// 포인터 주소 비교
			// 플레이 중인 유저와 다른 세션을 사용하는 경우
			if ( pSearchingSession != pClientSession )
			{
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
					"cGameRoom::RoomReJoin Failed. Critical Failed. Invalid ClientSession PlayerIdx[ %I64d ]" ,
					pClientSession->GetPlayerIdx() );

				if ( pSearchingSession->GetContext() != nullptr )
				{
					pSearchingSession->GetContext()->SetSession( nullptr );
					pSearchingSession->GetContext()->Disconnect();
					pSearchingSession->SetContext( nullptr );
				}

				// 세션을 로그 아웃 할 필요가 없습니다. 여기서 바로 세션이 있었던 자리를 재활용해서 사용합니다.
				// 해당 다른 세션만 세션매니져에 넣어줍니다.
				NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->Remove( pSearchingSession->GetPlayerIdx() );

				pSearchingSession = nullptr;
			}
			// 플레이 중인 유저와 같은 세션을 사용하는 경우
			// 특별히 할일이 없다.
			else
			{
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
					"cGameRoom::RoomReJoin Success. Using Same ClientSession ClientSession PlayerIdx[ %I64d ]" ,
					pClientSession->GetPlayerIdx() );

				return General::ResultCode::Result_Success;
			}
		}

		//pSearchingSession = const_cast<cClientSession*>(pClientSession);
	}
	else
	{
		// 재진입 유저 처리
		if ( false == m_gameInterface->HasSlot( pClientSession ) )
		{
			//General::ResultCode enterCode = m_gameInterface->EnterSlot( pClientSession );

			//// 엔터 성공한 경우만 맵에 추가
			//if ( enterCode == General::ResultCode::Result_Success ) {
			//	m_mapPlayers.insert( std::pair<__int64 , cClientSession*>( pClientSession->GetPlayerIdx() , const_cast< cClientSession* >( pClientSession )));

			//	// 룸에 들어갔으니 Context에는 관리 스레드를 설정하고
			//	NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->ChangeCommandContext( static_cast< NetLib::cIocpContext* >( pContext ) , m_nManagedCommandThreadNumber );

			//	pClientSession->SetCommandQueueIndex( m_nManagedCommandThreadNumber );
			//	pClientSession->SetJoinedRoomNumber( m_nRoomNumber );
			//	pClientSession->SetJoinedRoomNumberBefore( m_nRoomNumber ); // 이전에 조인했던 방은 다시 못들어 가게 하려고 추가한다.

			//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
			//		"cGameRoom::RoomJoin Success. PlayerIdx[%I64d], Entity[%u], RoomNumber[%u], Max(%u/%u)" ,
			//		pClientSession->GetPlayerIdx() , pContext->GetEntity() , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );
			//}

			//return enterCode;

			General::ResultCode errorCode = RegisterWatcher( pClientSession );
			return errorCode;
		}
		else
		{
			// 이미 점유 중인 슬롯이 존재하는 경우
			// 처리 할일이 없음
			return General::ResultCode::Result_Success;
		}
	}

	return General::ResultCode::Result_RoomEntryFailed;
}

// slotNumber 슬롯으로 조인시킨다.
// slotNumber 가 비어 있지 않다면 실패
General::ResultCode cGameRoom::RoomSlotJoin(cClientSession* pClientSession, const int slotNumber)
{
	return General::ResultCode::Result_RoomEntryFailed;
}

// 바카라 게임의 경우 최초 진입시에 관람모드로 조인한다.
General::ResultCode cGameRoom::RegisterWatcher(cClientSession* pClientSession)
{
	// 관전자 모드로 진입할때는 체크 하지 않는다.
	/*General::ResultCode errorCode = JoinPossible();
	if ( errorCode != General::ResultCode::Result_Success ) {
		return errorCode;
	}*/

	if( pClientSession == nullptr )
		return General::ResultCode::Result_SessionIdentityMismatch;

	const __int64 playerIdx = pClientSession->GetPlayerIdx();
	if ( pClientSession->GetPlayerIdx() == 0 ) {
		return General::ResultCode::Result_SessionExpired; // 세션 만료로 응답
	}

	NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
	if ( pContext == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoom::RegisterWatcher Failed. Critical Failed Context nullptr PlayerIdx[ %I64d ]" , playerIdx );
		return General::ResultCode::Result_SessionIdentityMismatch; // 세션 정보가 정확하지 않음
	}

	if ( m_gameInterface == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoom::RegisterWatcher Failed. IGame is nullptr PlayerIdx[ %I64d ]" , playerIdx );
		return General::ResultCode::Result_ActionRejected;
	}

	// 강제 퇴장 기록 확인
	if ( m_gameInterface->isJoinProhibited( pClientSession->GetPlayerIdx() ) )
		return General::ResultCode::Result_RoomEntryKickBlocked;

	//cheat
	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ischeat( pClientSession->GetPlayerIdx() ) )
	{
		General::ResultCode errorCode = RoomJoinCheat( pClientSession );
	}

	auto iterWatcher = m_mapWatchers.find( playerIdx );
	if ( iterWatcher == m_mapWatchers.end() ) {
		m_mapWatchers.insert( std::pair<__int64 , cClientSession*>( playerIdx , const_cast< cClientSession* >( pClientSession ) ) );
	}

	// 룸에 들어갔으니 Context에는 관리 스레드를 설정하고
	NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->ChangeCommandContext( static_cast< NetLib::cIocpContext* >( pContext ) , m_nManagedCommandThreadNumber );

	pClientSession->SetCommandQueueIndex( m_nManagedCommandThreadNumber );
	pClientSession->SetJoinedRoomNumber( m_nRoomNumber );
	pClientSession->SetJoinedRoomNumberBefore( m_nRoomNumber ); // 이전에 조인했던 방은 다시 못들어 가게 하려고 추가한다.

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		"cGameRoom::RegisterWatcher Success. PlayerIdx[%I64d], Entity[%u], RoomNumber[%u], Max(%u/%u)" ,
		pClientSession->GetPlayerIdx() , pContext->GetEntity() , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

	// 관전자 카운트 알림
	/*PmNet::InformObserverCntRS _watcher_res;
	int reservedCount = m_gameInterface->GetReservationPlayerPlayerCount();
	_watcher_res.set_observer_cnt( m_mapWatchers.size() - reservedCount );
	BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/

	return General::ResultCode::Result_Success;
}

void cGameRoom::RemoveWatcher( const uint64& playerIdx )
{
	auto iter = m_mapWatchers.find( playerIdx );
	if ( iter != m_mapWatchers.end() )
		m_mapWatchers.erase(iter);

	// 관전자 카운트 알림
	/*PmNet::InformObserverCntRS _watcher_res;
	int reservedCount = m_gameInterface->GetReservationPlayerPlayerCount();
	_watcher_res.set_observer_cnt( m_mapWatchers.size() - reservedCount );
	BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );*/
}
int cGameRoom::ClearWrongSession( std::set<cClientSession*>& sessions )
{
	int count = 0;
	std::ostringstream oss;
	oss << "ClearWrongSession: ";

	// 공통 로직을 처리하는 함수 람다 정의
	auto clearSessions = [&]( auto& map , const std::string& mapName ) {
		for ( auto it = map.begin(); it != map.end(); ) {
			if ( it->second == nullptr ||
				it->second->GetJoinedRoomNumber() != GetRoomNumber() ||
				it->first != it->second->GetPlayerIdx() ||
				it->second->GetContext() == nullptr ) {
				if ( it->second != nullptr ) {
					sessions.insert( it->second );
					oss << mapName << " - (" << it->first << "/" << it->second->GetPlayerIdx() << "),";
					count++;
				}
				it = map.erase( it ); // 요소 삭제
			}
			else {
				++it; // 다음 요소로 이동
			}
		}
	};

	// 각 맵에 대해 clearSessions 호출
	clearSessions( m_mapPlayers , "map" );
	clearSessions( m_mapWatchers , "watch" );
	clearSessions( m_mapCUsers , "CUser" );

	// 로그 기록
	if ( count > 0 ) {
		QueryManager::InsertErrorLog( oss.str() );
	}

	return count;
}

void cGameRoom::ClearOutUser()
{
	// 나간 유저 맵에서 삭제용
	std::vector<uint64> outUsers;
	std::vector<uint64> outWatcerUsers;

	PmNet::ChamberLeaveRS _res;
	_res.set_chamber_no( m_nRoomNumber );

	for ( auto player : m_mapPlayers )
	{
		cClientSession* pClientSession = player.second;
		if ( pClientSession != nullptr ) {
			if ( pClientSession->GetContext() == nullptr )
			{
				outUsers.push_back( pClientSession->GetPlayerIdx() );
				_res.set_member_idx( pClientSession->GetPlayerIdx() );
				BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
				m_gameInterface->CancelWatcherReservation( player.second );
				
				if ( pClientSession->is_copyed )
					delete pClientSession;
			}
		}
	}
	for ( auto player : m_mapWatchers )
	{
		cClientSession* pClientSession = player.second;
		if ( pClientSession != nullptr ) {
			if ( pClientSession->GetContext() == nullptr )
			{
				outUsers.push_back( pClientSession->GetPlayerIdx() );
				
			}
		}
	}

	for ( auto player : m_mapCUsers )
	{
		cClientSession* pClientSession = player.second;
		if ( pClientSession != nullptr ) {
			if ( pClientSession->GetContext() == nullptr )
			{
				outUsers.push_back( pClientSession->GetPlayerIdx() );
				_res.set_member_idx( pClientSession->GetPlayerIdx() );
				BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
				m_gameInterface->CancelWatcherReservation( player.second );


				if ( pClientSession->is_copyed )
					delete pClientSession;
			}
		}
	}

	


	// 실지로 맵에서 삭제
	for ( auto playerIdx : outUsers ) {
		auto iter = m_mapWatchers.find( playerIdx );
		if ( iter != m_mapWatchers.end() )
		{
			

			if ( m_gameInterface->RemoveReservation( iter->second ) )
			{
				_res.set_member_idx( playerIdx );
				BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
			}
			iter->second->RoomOutReset();
			m_mapWatchers.erase( iter );

		}
	}
	for ( auto playerIdx : outUsers ) {
		auto iter = m_mapPlayers.find( playerIdx );
		if ( iter == m_mapPlayers.end() )
			continue;
		

		if ( m_gameInterface->RemoveReservation( iter->second ) )
		{
			_res.set_member_idx( playerIdx );
			BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
		}


		m_gameInterface->LeaveSlot( iter->second , true );
		//m_gameInterface->OnVoteSystemPlayerRoomOut( playerIdx );

		// 나가면서 초기화 할것들 처리
		iter->second->RoomOutReset();

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		_T( "cGameRoom::KickRoomOutReservedOrAllinPlayer Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)" ) ,
		iter->second->GetPlayerIdx() , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

		m_mapPlayers.erase( iter );
	}

	for ( auto playerIdx : outUsers ) {
		auto iter = m_mapCUsers .find( playerIdx );
		if ( iter == m_mapCUsers.end() )
			continue;

		

		if ( m_gameInterface->RemoveReservation( iter->second ) )
		{
			_res.set_member_idx( playerIdx );
			BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
		}


		m_gameInterface->LeaveSlot( iter->second , true );
		//m_gameInterface->OnVoteSystemPlayerRoomOut( playerIdx );

		// 나가면서 초기화 할것들 처리
		iter->second->RoomOutReset();

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		_T( "cGameRoom::KickRoomOutReservedOrAllinPlayer Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)" ) ,
		iter->second->GetPlayerIdx() , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

		m_mapCUsers.erase( iter );
	}
}
void cGameRoom::CheckMaintance()
{
	std::vector< cClientSession*> outsessions;
	for ( auto player : m_mapPlayers )
	{
		cClientSession* pClientSession = player.second;
		if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenance( pClientSession->GetMarket() , pClientSession->GetIp() , pClientSession->GetGameVersion() ) ) {
			const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();

			std::string errorString;
			PmNet::ServiceNotice system_message;
			system_message.set_notice( message.message() );
			pClientSession->SendRequest( General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_ServicePaused , errorString );
			outsessions.push_back( pClientSession );
		}
	}

	for ( auto player : m_mapWatchers )
	{
		cClientSession* pClientSession = player.second;
		if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenance( pClientSession->GetMarket() , pClientSession->GetIp() , pClientSession->GetGameVersion() ) ) {

			const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();

			std::string errorString;
			PmNet::ServiceNotice system_message;
			system_message.set_notice( message.message() );
			pClientSession->SendRequest( General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_Success , errorString );
			outsessions.push_back( pClientSession );
		}
	}



	

	for ( auto outsession : outsessions )
	{
		outsession->SessionLogout( 0 , 1 );
		if ( outsession == nullptr )
			continue;
		auto pContext = outsession->GetContext();
		if ( pContext == nullptr )
			continue;
		outsession->DisConnectContext( pContext->GetEntity() , 0);
		pContext->Disconnect();

	}
}

void cGameRoom::RemoveWatchars()
{
	PmNet::ChamberLeaveRS _res;

	_res.set_chamber_no( m_nRoomNumber );
	
	std::string errorMessage;

	for ( auto playerPair : m_mapWatchers )
	{
		//playerPair.first;
		auto player = playerPair.second;
		if ( player == nullptr ) continue;

		player->RoomOutReset();

		_res.set_member_idx( player->GetPlayerIdx() );
		_res.set_no_member_flag( true );
		//_res.set_poll_expelled( kicked );
		//BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );

		player->SendRequest( General::Packet_SpaceLeave , _res , General::ResultCode::Result_Success , errorMessage );

	}

	m_mapWatchers.clear();
}

void cGameRoom::RemovePlayer( const uint64& playerIdx )
{
	auto iter = m_mapPlayers.find( playerIdx );
	if ( iter != m_mapPlayers.end() )
	{
		m_mapPlayers.erase( iter );
		m_mapCUsers.erase( playerIdx );
	}
}

bool cGameRoom::isPlayer( const uint64& playerIdx )
{
	auto iter = m_mapPlayers.find( playerIdx );
	return iter != m_mapPlayers.end();
}

bool cGameRoom::isWatcher(const uint64& playerIdx)
{
	auto iter = m_mapWatchers.find( playerIdx );
	return iter != m_mapWatchers.end();
}

bool cGameRoom::isCheat( const uint64& playerIdx )
{
	//return false;
	/*if ( E_SERVER_STAGE::LIVE == NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerStage )
		return false;*/
	auto iter = m_mapCUsers.find( playerIdx );
	return iter != m_mapCUsers.end();
}

bool cGameRoom::RoomOut(const __int64 playerIdx, cClientSession* pClientSession, bool kicked )
{
	if ( pClientSession == nullptr ) return false;
	auto watcherSession = FindWatcherSession( playerIdx );
	if ( m_gameInterface->HasSlotReservation( pClientSession ) )
	{
		switch ( m_gameInterface->GetGameType() )
		{
		
		//case Common::GameType::GameType_Roulette:
		
		case General::PlayCategory::PlayCategory_TexasHoldem:
		{
			m_gameInterface->RemoveReservation( pClientSession );
			PmNet::DropMemberOnSeatRS remove_response;
			remove_response.set_member_idx( pClientSession->GetPlayerIdx() );
			BroadCastToAllPlayer( General::PacketID::Packet_SlotSeatClear , remove_response );
		}
		break;
		}
	}
	if ( m_gameInterface->RemoveParticipationQueue( pClientSession ) )
	{
		PmNet::ChamberEnterRS response;
		GetGameRoomDetail( response , true );
		// 방 전체에 통보
		BroadCastToAllPlayer( General::Packet_SpaceEnter , response );
	}
	if ( watcherSession != nullptr )
	{
		PmNet::ChamberLeaveRS _res;

		_res.set_chamber_no( m_nRoomNumber );
		_res.set_member_idx( playerIdx );
		_res.set_poll_expelled( kicked );
		_res.set_observer_cnt( GetWatcherCnt() - 1 ); // 관전자가 나가는 중이기 때문에 -1을 해서 보낸다.
		BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );

		m_gameInterface->RemoveSubPlayerReservation( playerIdx );



		// 나가면서 초기화 할것들 처리
		pClientSession->RoomOutReset();

		// 참여큐에 있으면 삭제

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		_T( "cGameRoom::RoomOut() Watcher Room Out Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)" ) ,
		playerIdx , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

		m_mapWatchers.erase( playerIdx );
		if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ischeat( pClientSession->GetPlayerIdx() ) )
			RemoveCheatUser( pClientSession->GetPlayerIdx() );

		pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online , "" );
		return true;
	}
	// 자리 예약 된 사람의 경우 통보

	

	/*if ( m_gameInterface->RemoveReservation( pClientSession ) ) {
		
		PmNet::DropMemberOnSeatRS remove_response;
		remove_response.set_member_idx( pClientSession->GetPlayerIdx() );
		BroadCastToAllPlayer( General::PacketID::Packet_SlotSeatClear , remove_response );
	}*/

	m_gameInterface->CancelWatcherReservation( pClientSession );

	//auto session = FindSession( playerIdx );
	//if ( session == nullptr ) return false;

	//// 이 경우는 playerIdx 로 가져온 cClientSession 의 주소값이 동일한 하지 않은 경우
	//if ( session != nullptr && session != pClientSession ) return false;

	// 유저가 나간것을 방의 유저들에게 통보해줍니다.
	//if ( m_gameInterface->GetPlayerSession() )
	if ( m_mapPlayers.size() )
	{
		PmNet::ChamberLeaveRS _res;

		_res.set_chamber_no( m_nRoomNumber );
		_res.set_member_idx( playerIdx );
		_res.set_poll_expelled( kicked );
		_res.set_observer_cnt( GetWatcherCnt() );
		BroadCastToAllPlayer(General::Packet_SpaceLeave, _res);
	}

	m_gameInterface->LeaveSlot( pClientSession , true);
	//m_gameInterface->OnVoteSystemPlayerRoomOut( playerIdx );

	// 나가면서 초기화 할것들 처리
	pClientSession->RoomOutReset();
	if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ischeat( pClientSession->GetPlayerIdx() ) )
		RemoveCheatUser( pClientSession->GetPlayerIdx() );
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, 
		_T("cGameRoom::RoomOut() Room Out Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)"),
		playerIdx, GetRoomNumber(), m_nMaxRoomPlayerCnt , GetJoinedCnt());

	//return m_mapPlayers.RemoveKey(playerIdx);

	pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online , "" );
	m_mapCUsers.erase( playerIdx );
	return m_mapPlayers.erase( playerIdx );
}

bool cGameRoom::DieRoomOut( const __int64 playerIdx , cClientSession* pClientSession , bool kicked )
{
	if ( pClientSession == nullptr ) return false;

	cClientSession* t_session = new cClientSession( *pClientSession );
	t_session->SetRoomOutReserve( true );

	auto watcherSession = FindWatcherSession( playerIdx );
	if ( watcherSession != nullptr )
	{
		PmNet::ChamberLeaveRS _res;

		_res.set_chamber_no( m_nRoomNumber );
		_res.set_member_idx( playerIdx );
		_res.set_poll_expelled( kicked );
		_res.set_observer_cnt( GetWatcherCnt() - 1 ); // 관전자가 나가는 중이기 때문에 -1을 해서 보낸다.
		BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );

		m_gameInterface->RemoveSubPlayerReservation( playerIdx );

		// 나가면서 초기화 할것들 처리
		pClientSession->RoomOutReset();

		// 참여큐에 있으면 삭제
		m_gameInterface->RemoveParticipationQueue( pClientSession );

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		_T( "cGameRoom::RoomOut() Watcher Room Out Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)" ) ,
		playerIdx , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

		m_mapWatchers.erase( playerIdx );
		pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online , "" );

		return true;
	}

	// 자리 예약 된 사람의 경우 통보
	if ( m_gameInterface->RemoveReservation( pClientSession ) ) {
		//PmNet::DropMemberOnSeatRS remove_response;
		//remove_response.set_member_idx( pClientSession->GetPlayerIdx() );
		//BroadCastToAllPlayer( General::PacketID::Packet_SlotSeatClear , remove_response );
	}

	m_gameInterface->CancelWatcherReservation( pClientSession );

	auto session = FindSession( playerIdx );
	if ( session == nullptr ) return false;

	// 이 경우는 playerIdx 로 가져온 cClientSession 의 주소값이 동일한 하지 않은 경우
	if ( session != nullptr && session != pClientSession ) return false;

	// 유저가 나간것을 방의 유저들에게 통보해줍니다.
	/*if ( m_mapPlayers.size() )
	{
		PmNet::ChamberLeaveRS _res;
		_res.set_chamber_no( m_nRoomNumber );
		_res.set_member_idx( playerIdx );
		_res.set_poll_expelled( kicked );
		BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
	}*/

	//나간건 자기한테만
	{
		PmNet::ChamberLeaveRS _res;

		_res.set_chamber_no( m_nRoomNumber );
		_res.set_member_idx( playerIdx );
		_res.set_poll_expelled( kicked );
		_res.set_observer_cnt( GetWatcherCnt() );
		std::string errorMessage;
		pClientSession->SendRequest( General::Packet_SpaceLeave , _res , General::ResultCode::Result_Success , errorMessage );
		pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online , "" );
	}
	m_gameInterface->SwapWithDummy( pClientSession ,t_session );
	//m_gameInterface->OnVoteSystemPlayerRoomOut( playerIdx );

	// 나가면서 초기화 할것들 처리
	pClientSession->RoomOutReset();

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		_T( "cGameRoom::RoomOut() Room Out Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)" ) ,
		playerIdx , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

	//return m_mapPlayers.RemoveKey(playerIdx);
	return true;
	//return m_mapPlayers.erase( playerIdx );
}

void cGameRoom::RoomOutReservationReverse( const __int64 playerIdx , cClientSession* pClientSession , bool reserve , bool reserve_cancel )
{
	auto session = FindSession( playerIdx );
	if ( session == nullptr ) return;
	// 이 경우는 playerIdx 로 가져온 cClientSession 의 주소값이 동일한 하지 않은 경우
	if ( session != nullptr && session != pClientSession ) return;

	PmNet::ChamberLeaveHoldRS _res;
	_res.set_chamber_no( m_nRoomNumber );
	_res.set_member_idx( playerIdx );

	//	방나가기 예약 철회
	/*if ( reserve ) {
		pClientSession->SetRoomOutReserve();
		_res.set_hold_ok( true );
	}

	if ( reserve_cancel ) {
		pClientSession->CalcelRoomOutReserve();
		_res.set_revoke_ok( true );
	}*/
	bool b_roomout = pClientSession->ReverseRoomOutReserve();
	_res.set_hold_ok( b_roomout );
	_res.set_revoke_ok( !b_roomout );


	// 전체 인원 전송
	BroadCastToAllPlayer( General::Packet_LeaveReserve , _res );
}
void cGameRoom::RoomOutReservation( const __int64 playerIdx , cClientSession* pClientSession, bool reserve, bool reserve_cancel )
{
	auto session = FindSession( playerIdx );
	if ( session == nullptr ) return;
	// 이 경우는 playerIdx 로 가져온 cClientSession 의 주소값이 동일한 하지 않은 경우
	if ( session != nullptr && session != pClientSession ) return;

	PmNet::ChamberLeaveHoldRS _res;
	_res.set_chamber_no( m_nRoomNumber );
	_res.set_member_idx( playerIdx );

	//방나가기 예약 철회
	if ( reserve ) {
		pClientSession->SetRoomOutReserve();
		_res.set_hold_ok( true );
	}
	
	if ( reserve_cancel ) {
		pClientSession->CalcelRoomOutReserve();
		_res.set_revoke_ok( true );
	}
	if ( pClientSession->is_copyed )
		delete pClientSession;
	//bool b_roomout = pClientSession->ReverseRoomOutReserve();
	//_res.set_hold_ok( b_roomout );
	//_res.set_revoke_ok( !b_roomout );

	// 전체 인원 전송
	BroadCastToAllPlayer( General::Packet_LeaveReserve , _res);
}

// 방 나가기 예약자 내보내기
void cGameRoom::KickRoomOutReservedOrAllinOrLostLimitPlayer()
{
	// 나간 유저 맵에서 삭제용
	std::vector<uint64> outUsers;

	PmNet::ChamberLeaveRS _res;
	_res.set_chamber_no( m_nRoomNumber );

	for ( auto pair : m_mapPlayers ) {

		cClientSession* pClientSession = pair.second;
		if ( pClientSession != nullptr ) {

			bool is_reserved = pClientSession->GetReserved();
			bool is_all_in = pClientSession->isAllIn();
			bool is_lost_limit = pClientSession->IsOverLostLimit();

			// 나가기 예약 또는 올인 또는 손실한도가 발생한 플레이어이다.
			if ( is_reserved || is_all_in || is_lost_limit ) {

				/*_res.set_member_idx(pClientSession->GetPlayerIdx());
				_res.set_loss_cap_hit( is_lost_limit );
				_res.set_observer_cnt( GetWatcherCnt() );*/

				outUsers.push_back( pClientSession->GetPlayerIdx() );

				// 손실한도의 유저경우에는 시간 처리
				// 차후에 튜닝합시다.
				if ( is_lost_limit ) {
					pClientSession->IsOverLostLimit( true );
					pClientSession->SendLostLimitPopopOnResult();
				}
				//BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
				m_gameInterface->CancelWatcherReservation( pair.second );
				if ( pClientSession->HasMoneyLimit() ) {
					pClientSession->SendMoneyLimitPopopOnResult();
				}
				if ( pClientSession->is_copyed )
					delete pClientSession;

				pClientSession->SyncFriend_Online( true , pClientSession , General::ContactState::ContactState_Online , "" );
			}
		}
	}



	// 실지로 맵에서 삭제
	for ( auto playerIdx : outUsers ) {
		auto iter = m_mapPlayers.find( playerIdx );
		if ( iter == m_mapPlayers.end() )
			continue;
		
		RoomOut( playerIdx , iter->second );
		//m_gameInterface->LeaveSlot( iter->second , true);
		//m_gameInterface->OnVoteSystemPlayerRoomOut( playerIdx );
		
		// 나가면서 초기화 할것들 처리
		/*iter->second->RoomOutReset();

		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO ,
		_T( "cGameRoom::KickRoomOutReservedOrAllinPlayer Success. PlayerIdx[%I64d], channel[%u], Max(%u/%u)" ) ,
		iter->second->GetPlayerIdx() , GetRoomNumber() , m_nMaxRoomPlayerCnt , GetJoinedCnt() );

		m_mapPlayers.erase( iter );*/

	}
}

void cGameRoom::RoomOutNotEnoughSeedPlayer()
{
	if ( m_gameInterface != nullptr ) {
		std::vector<uint64> outUsers;
		std::vector<cClientSession*> baccaraOutUsers;

		PmNet::ChamberLeaveRS _res;
		_res.set_chamber_no( m_nRoomNumber );

		const Server::Channel& channelData = m_gameInterface->GetChannelData();

		General::PlayCategory gameType = m_gameInterface->GetGameType();
		General::RoomAccessMode roomType = m_roomInfo.access_mode();
		General::AssetKind moneyType = m_roomInfo.asset_kind();
		uint64 seedMoney = m_roomInfo.seed_amount();
		uint64 moneyKick = channelData.money_kick();

		switch ( gameType )
		{
		
		case General::PlayCategory::PlayCategory_TexasHoldem:
		{
			for ( const auto& pair : m_mapPlayers ) {
				if ( pair.second != nullptr ) 
				{
					uint64 playerMoney = 0;

					if ( moneyType == General::AssetKind::AssetKind_Chip )
						playerMoney = pair.second->GetChip();
					else if ( moneyType == General::AssetKind::AssetKind_Coin )
						playerMoney = pair.second->GetCoin();

					// 로우 바둑이, 홀덤은 MoneyKick 보다 재화가 적은 경우 로비로 튕겨 낸다.
					if ( playerMoney < moneyKick ) {
						_res.set_member_idx( pair.second->GetPlayerIdx() );
						_res.set_observer_cnt( GetWatcherCnt() );
						_res.set_no_fund_flag( true );
						BroadCastToAllPlayer( General::Packet_SpaceLeave , _res );
						m_gameInterface->CancelWatcherReservation( pair.second );
						outUsers.push_back( pair.second->GetPlayerIdx() );
						pair.second->SyncFriend_Online( true , pair.second, General::ContactState::ContactState_Online , "" );
					}
				}
			}

			// 실지로 맵에서 삭제
			for ( auto playerIdx : outUsers ) {
				auto iter = m_mapPlayers.find( playerIdx );
				if ( iter == m_mapPlayers.end() )
					continue;

				m_gameInterface->LeaveSlot( iter->second , true);
				//m_gameInterface->OnVoteSystemPlayerRoomOut( playerIdx );
				iter->second->RoomOutReset(); // 나가면서 초기화 할것들 처리
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cGameRoom::RoomOutNotEnoughSeedPlayer Room Out Player. PlayerIdx[%I64d], RoomNumber[%u]" , 
					iter->second->GetPlayerIdx() , GetRoomNumber() );

				m_mapPlayers.erase( iter );
				m_mapCUsers.erase( playerIdx );
			}
		}
		break;
		
		//case Common::GameType::GameType_Roulette:
		}

	}
}

// 방에 투표 시스템 처리 할 것들이 있으면 처리 한다.
// 강제 퇴장 시스템 처리는 현재 로우바둑이 뿐이 없다. 홀덤은 방장이 진행한다.
//void cGameRoom::ActivateVote()
//{
//	if ( m_gameInterface != nullptr ) {
//		m_gameInterface->OnResultVoteProcess();
//	}
//}

cClientSession* cGameRoom::FindSession( const __int64 playerIdx )
{
	auto iter = m_mapPlayers.find( playerIdx );
	if ( iter == m_mapPlayers.end() )
		return nullptr;

	return iter->second;
}

cClientSession* cGameRoom::FindWatcherSession( const __int64 playerIdx )
{
	auto iter = m_mapWatchers.find( playerIdx );
	if ( iter == m_mapWatchers.end() )
		return nullptr;

	return iter->second;
}

bool cGameRoom::MoveRoom( const __int64 playerIdx )
{
	auto iter = m_mapPlayers.find( playerIdx );
	if ( iter == m_mapPlayers.end() )
		return false;

	m_mapPlayers.erase( playerIdx );
	m_mapCUsers.erase( playerIdx );

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO , _T( "cGameRoom::RoomOut() Room Move Success. PlayerIdx[%I64d], channel[%u]" ) , playerIdx , GetRoomNumber() );
	return true;
}

void cGameRoom::SetTriggerChangeInRoomMove(const __int64 AID)
{
	auto iter = m_mapPlayers.find( AID );
	if ( iter == m_mapPlayers.end() )
		return;

	cClientSession* pInfo = iter->second;
	if ( iter->second == nullptr)
		return;

	//pInfo->RoomChangeStart();
}

bool cGameRoom::RemoveSpecifiedUser(const __int64 playerIdx)
{
	auto iter = m_mapPlayers.find( playerIdx );
	if ( iter == m_mapPlayers.end() )
		return false;

	if ( iter->second == nullptr )
		return false;

	if ( iter->second->GetPlayerIdx() != playerIdx )
		return false;

	m_mapPlayers.erase( iter );
	m_mapCUsers.erase( playerIdx );

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO,  _T("cGameRoom::RemoveSpecifiedUser Success. PlayerIdx[%I64d], channel[%u]"), playerIdx, GetRoomNumber());
	return true;
}

bool cGameRoom::RoomBroadCast( BYTE* pData , UINT nLength )
{
	if ( pData == nullptr || nLength <= 0 ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO , _T( "cGameRoom::RoomBroadCastChat Failed. pData or nLength InValid Data " ) ); return false;
	}

	for ( auto& mapPair : m_mapPlayers ) {

		cClientSession* pClientSession = mapPair.second;
		if ( pClientSession == nullptr ) continue;

		NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		pContext->SendRequest( pData , nLength );
	}

	return true;
}

void cGameRoom::RoomBroadCast( const UINT nCommand , BYTE* pData , UINT nLength , int64 except_playerIdx )
{
	if ( pData == nullptr || nLength <= 0 )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO , _T( "cGameRoom::RoomBroadCast Failed. pData or nLength InValid Data " ) );
		return;
	}

	NetLib::cPacketStack Packet( CSNet::E_PROTOCOL::E_TCP );
	Packet.Make( nCommand , pData , nLength , 0 );

	for ( auto& mapPair : m_mapPlayers ) {

		cClientSession* pClientSession = mapPair.second;
		if ( pClientSession == nullptr ) continue;

		// except_playerIdx 한테는 제외하고 보낸다.
		if ( except_playerIdx != 0 && except_playerIdx == pClientSession->GetPlayerIdx() ) continue;

		NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}
}

bool cGameRoom::RoomBroadCast( const UINT nCommand , google::protobuf::Message* _message , int64 except_playerIdx )
{
	if ( _message == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO , _T( "cGameRoom::RoomBroadCast Failed. _message is nullptr" ) ); return false;
	}



	// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( m_nManagedCommandThreadNumber );
	if ( pGOOGLE_PROTOBUF_BUFFER == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "RoomBroadCast is Failed. Serialize Failed. #1" ); return false;
	}

	if ( !_message->SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->DataBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "RoomBroadCast is Failed. Serialize Failed. #2" ); return false;
	}

	size_t nSize = static_cast< size_t >( _message->ByteSizeLong() );

	PmNet::PktBase _response;
	_response.set_payload( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , nSize );
	_response.set_payload_size( static_cast< google::protobuf::uint32 >( nSize ) );
	_response.set_err_kind( General::ResultCode::Result_Success );

	if ( !_response.SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "RoomBroadCast is Failed. Serialize Failed. #3" ); return false;
	}

	UINT uiSize = static_cast< UINT >( _response.ByteSizeLong() );

	NetLib::cPacketStack Packet( CSNet::E_PROTOCOL::E_TCP );
	Packet.Make( nCommand , pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , uiSize , 0 );

	for ( auto& mapPair : m_mapPlayers ) {

		cClientSession* pClientSession = mapPair.second;
		if ( pClientSession == nullptr ) continue;

		// except_playerIdx 한테는 제외하고 보낸다.
		if ( except_playerIdx != 0 && except_playerIdx == pClientSession->GetPlayerIdx() ) continue;

		NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}

	return true;
}

bool cGameRoom::RoomBroadCast(const UINT nCommand, google::protobuf::Message& _message, int64 except_playerIdx)
{


	// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer(m_nManagedCommandThreadNumber);
	if (pGOOGLE_PROTOBUF_BUFFER == nullptr) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "RoomBroadCast is Failed. Serialize Failed. File: %s Line: %d" , _T( __FILE__ ) , __LINE__ ); return false;
	}

	if (!_message.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, sizeof(pGOOGLE_PROTOBUF_BUFFER->DataBuffer))) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "RoomBroadCast is Failed. Serialize Failed. File: %s Line: %d", _T( __FILE__ ) , __LINE__ ); return false;
	}

	size_t nSize = static_cast<size_t>(_message.ByteSizeLong());

	PmNet::PktBase _response;
	_response.set_payload(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, nSize);
	_response.set_payload_size(static_cast<google::protobuf::uint32>(nSize));
	_response.set_err_kind( General::ResultCode::Result_Success );

	if (!_response.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, sizeof(pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer))) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "RoomBroadCast is Failed. Serialize Failed. File: %s Line: %d" , _T( __FILE__ ) , __LINE__ ); return false;
	}

	UINT uiSize = static_cast<UINT>(_response.ByteSizeLong());

	NetLib::cPacketStack Packet(CSNet::E_PROTOCOL::E_TCP);
	Packet.Make(nCommand, pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, uiSize, 0);

	for ( auto& mapPair : m_mapPlayers ) {

		cClientSession* pClientSession = mapPair.second;
		if ( pClientSession == nullptr ) continue;

		// except_playerIdx 한테는 제외하고 보낸다.
		if ( except_playerIdx != 0 && except_playerIdx == pClientSession->GetPlayerIdx() ) continue;

		NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		if ( isCheat( mapPair.first ) ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}

	return true;
}
bool cGameRoom::RoomBroadCastCUser(const UINT nCommand, google::protobuf::Message& _message, int64 except_playerIdx)
{


	// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer(m_nManagedCommandThreadNumber);
	if (pGOOGLE_PROTOBUF_BUFFER == nullptr) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "RoomBroadCast is Failed. Serialize Failed. File: %s Line: %d" , _T( __FILE__ ) , __LINE__ ); return false;
	}

	if (!_message.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, sizeof(pGOOGLE_PROTOBUF_BUFFER->DataBuffer))) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "RoomBroadCast is Failed. Serialize Failed. File: %s Line: %d", _T( __FILE__ ) , __LINE__ ); return false;
	}

	size_t nSize = static_cast<size_t>(_message.ByteSizeLong());

	PmNet::PktBase _response;
	_response.set_payload(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, nSize);
	_response.set_payload_size(static_cast<google::protobuf::uint32>(nSize));
	_response.set_err_kind( General::ResultCode::Result_Success );

	if (!_response.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, sizeof(pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer))) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "RoomBroadCast is Failed. Serialize Failed. File: %s Line: %d" , _T( __FILE__ ) , __LINE__ ); return false;
	}

	UINT uiSize = static_cast<UINT>(_response.ByteSizeLong());

	NetLib::cPacketStack Packet(CSNet::E_PROTOCOL::E_TCP);
	Packet.Make(nCommand, pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, uiSize, 0);

	for ( auto& mapPair : m_mapCUsers ) {

		cClientSession* pClientSession = mapPair.second;
		if ( pClientSession == nullptr ) continue;

		NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}

	return true;
}

bool cGameRoom::WatcherBroadCast( BYTE* pData , UINT nLength )
{
	if ( pData == nullptr || nLength < 1 ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "cGameRoom::WatcherBroadCast Failed. pData or nLength InValid Data " ) ); return false;
	}

	cClientSession* pClientSession = nullptr;
	NetLib::cInterfaceIocpContext* pContext = nullptr;

	for ( auto& watcher : m_mapWatchers ) {

		if ( watcher.second == nullptr ) continue;

		pContext = watcher.second->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		pContext->SendRequest( pData , nLength );
	}

	return true;
}

void cGameRoom::WatcherBroadCast( const UINT nCommand , BYTE* pData , UINT nLength , int64 except_playerIdx )
{
	if ( pData == nullptr || nLength < 1 ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "cGameRoom::WatcherBroadCast Failed. pData or nLength InValid Data " ) ); return;
	}
	
	NetLib::cPacketStack Packet( CSNet::E_PROTOCOL::E_TCP );
	Packet.Make( nCommand , pData , nLength , 0 );

	cClientSession* pClientSession = nullptr;
	NetLib::cInterfaceIocpContext* pContext = nullptr;

	for ( auto& watcher : m_mapWatchers ) {

		if ( watcher.second == nullptr ) continue;

		pContext = watcher.second->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		if ( except_playerIdx != 0 && except_playerIdx == watcher.second->GetPlayerIdx() ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}
}

bool cGameRoom::WatcherBroadCast( const UINT nCommand , google::protobuf::Message* _message , int64 except_playerIdx )
{
	if ( _message == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "cGameRoom::WatcherBroadCast Failed. pData or nLength InValid Data " ) ); return false;
	}



	// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( m_nManagedCommandThreadNumber );
	if ( pGOOGLE_PROTOBUF_BUFFER == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "WatcherBroadCast is Failed. Serialize Failed. File: %s Line: %d" ) , _T( __FILE__ ) , __LINE__ ); return false;
	}

	if ( !_message->SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->DataBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "WatcherBroadCast is Failed. Serialize Failed. File: %s Line: %d" ) , _T( __FILE__ ) , __LINE__ ); return false;
	}

	size_t nSize = static_cast< size_t >( _message->ByteSizeLong() );

	PmNet::PktBase _response;
	_response.set_payload( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , nSize );
	_response.set_payload_size( static_cast< google::protobuf::uint32 >( nSize ) );
	_response.set_err_kind( General::ResultCode::Result_Success );

	if ( !_response.SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "WatcherBroadCast is Failed. Serialize Failed. File: %s Line: %d" ) , _T( __FILE__ ) , __LINE__ ); return false;
	}

	UINT uiSize = static_cast< UINT >( _response.ByteSizeLong() );

	NetLib::cPacketStack Packet( CSNet::E_PROTOCOL::E_TCP );
	Packet.Make( nCommand , pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , uiSize , 0 );

	cClientSession* pClientSession = nullptr;
	NetLib::cInterfaceIocpContext* pContext = nullptr;

	for ( auto& watcher : m_mapWatchers ) {

		if ( watcher.second == nullptr ) continue;

		pContext = watcher.second->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		if ( except_playerIdx != 0 && except_playerIdx == watcher.second->GetPlayerIdx() ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}
	return true;
}

bool cGameRoom::WatcherBroadCast( const UINT nCommand , google::protobuf::Message& _message , int64 except_playerIdx )
{


	// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( m_nManagedCommandThreadNumber );
	if ( pGOOGLE_PROTOBUF_BUFFER == nullptr ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "WatcherBroadCast is Failed. Serialize Failed. File: %s Line: %d" ) , _T( __FILE__ ) , __LINE__ ); return false;
	}

	if ( !_message.SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->DataBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "WatcherBroadCast is Failed. Serialize Failed. File: %s Line: %d" ) , _T( __FILE__ ) , __LINE__ ); return false;
	}

	size_t nSize = static_cast< size_t >( _message.ByteSizeLong() );

	PmNet::PktBase _response;
	_response.set_payload( pGOOGLE_PROTOBUF_BUFFER->DataBuffer , nSize );
	_response.set_payload_size( static_cast< google::protobuf::uint32 >( nSize ) );
	_response.set_err_kind( General::ResultCode::Result_Success );

	if ( !_response.SerializeToArray( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer ) ) ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , _T( "WatcherBroadCast is Failed. Serialize Failed. File: %s Line: %d" ) , _T( __FILE__ ) , __LINE__ ); return false;
	}

	UINT uiSize = static_cast< UINT >( _response.ByteSizeLong() );

	NetLib::cPacketStack Packet( CSNet::E_PROTOCOL::E_TCP );
	Packet.Make( nCommand , pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer , uiSize , 0 );

	cClientSession* pClientSession = nullptr;
	NetLib::cInterfaceIocpContext* pContext = nullptr;

	for ( auto& watcher : m_mapWatchers ) {

		if ( watcher.second == nullptr ) continue;

		pContext = watcher.second->GetContext();
		if ( pContext == nullptr || !pContext->IsActive() ) continue;

		if ( except_playerIdx != 0 && except_playerIdx == watcher.second->GetPlayerIdx() ) continue;

		if ( isCheat( watcher.first ) ) continue;

		pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
	}
	return true;
}

void cGameRoom::BroadCastToCheatPlayer( const UINT nCommand , google::protobuf::Message& _message , int64 except_playerIdx , bool tocheat )
{
	//std::string test = _message.GetTypeName();

	// RoomOut 인 경우 관전자 카운트 전송
	if ( _message.GetTypeName()._Equal( "Game.RoomOutRes" ) ) {

		PmNet::ChamberLeaveRS* _roomOut = dynamic_cast< PmNet::ChamberLeaveRS* >( &_message );
		if ( _roomOut != nullptr ) {

			const uint64& playerIdx = _roomOut->member_idx();
			auto gameInterface = GetGameInterface();
			auto player = gameInterface->GetPlayerSession( playerIdx );

			if ( player != nullptr ) {

				const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

				Server::SyncFriendInfoStatus syncFriendInfoStatus;
				syncFriendInfoStatus.set_player_idx( player->GetPlayerIdx() );
				syncFriendInfoStatus.set_friend_status( General::ContactState::ContactState_Online );
				syncFriendInfoStatus.set_server_id( configReader->SID_FOR_MANAGE );
				syncFriendInfoStatus.set_playing_channel_id( "" );
				General::ParticipantProfile* responsePlayer = syncFriendInfoStatus.mutable_player_data();
				responsePlayer->CopyFrom( player->GetPlayer() );

				NetLib::cSingleton<cFriendManager>::GetInstance()->SyncFriendInfoStatus( syncFriendInfoStatus );

				// 각 로비서버들에게도 쏘아줌
				GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( 0 );
				pGOOGLE_PROTO_BUFFER->Clear();
				if ( syncFriendInfoStatus.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) {
					NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cGameRoom::BroadCastToAllPlayer SyncFriendInfoStatus SerializeToArray Failed." );
				}

				E_ERROR_SEND send_error = NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->BroadCastToConnectors(
								E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeContactStateSync , pGOOGLE_PROTO_BUFFER->SerializeBuffer , syncFriendInfoStatus.ByteSizeLong() );

				if ( send_error == E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET ) {

					// 로비서버 전송 실패
				}
			}
		}
	}

	// BroadCastToAllPlayer 시에 출력할 정보 디버그 모드일 출력
#ifdef _DEBUG

	if ( _message.GetTypeName()._Equal( "Game.PlayMasterChangeRes" ) ) {

		PmNet::MatchCaptainSwapRS* _masterChange = dynamic_cast< PmNet::MatchCaptainSwapRS* >( &_message );
		if ( _masterChange != nullptr ) {

			const int& master_slot = _masterChange->captain_seat();
			const uint64& master_player_idx = _masterChange->captain_idx();

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "PlayMasterChangeRes master_slot : %d , master_player_idx : %IIu" , master_slot , master_player_idx );
		}
	}

	if ( _message.GetTypeName()._Equal( "Game.PlayBossChangeRes" ) ) {

		PmNet::MatchLeadSwapRS* _bossChange = dynamic_cast< PmNet::MatchLeadSwapRS* >( &_message );
		if ( _bossChange != nullptr ) {

			const int& boss_slot = _bossChange->lead_seat();
			const uint64& boss_player_idx = _bossChange->lead_idx();

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "PlayBossChangeRes boss_slot : %d , boss_player_idx : %IIu" , boss_slot , boss_player_idx );
		}
	}

#endif
	RoomBroadCastCUser( nCommand , _message , except_playerIdx );
}

void cGameRoom::BroadCastToAllPlayer( const UINT nCommand , google::protobuf::Message& _message , int64 except_playerIdx ,bool tocheat)
{
	//std::string test = _message.GetTypeName();
	
	// RoomOut 인 경우 관전자 카운트 전송
	if ( _message.GetTypeName()._Equal("Game.RoomOutRes") ) {

		PmNet::ChamberLeaveRS* _roomOut = dynamic_cast< PmNet::ChamberLeaveRS* >( &_message );
		if ( _roomOut != nullptr ) {

			const uint64& playerIdx = _roomOut->member_idx();
			auto gameInterface = GetGameInterface();
			auto player = gameInterface->GetPlayerSession( playerIdx );
		}
	}

	// BroadCastToAllPlayer 시에 출력할 정보 디버그 모드일 출력
#ifdef _DEBUG

	if ( _message.GetTypeName()._Equal( "Game.PlayMasterChangeRes" ) ) {

		PmNet::MatchCaptainSwapRS* _masterChange = dynamic_cast< PmNet::MatchCaptainSwapRS* >( &_message );
		if ( _masterChange != nullptr ) {

			const int& master_slot = _masterChange->captain_seat();
			const uint64& master_player_idx = _masterChange->captain_idx();

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "PlayMasterChangeRes master_slot : %d , master_player_idx : %IIu", master_slot , master_player_idx );
		}
	}

	if ( _message.GetTypeName()._Equal( "Game.PlayBossChangeRes" ) ) {

		PmNet::MatchLeadSwapRS* _bossChange = dynamic_cast< PmNet::MatchLeadSwapRS* >( &_message );
		if ( _bossChange != nullptr ) {

			const int& boss_slot = _bossChange->lead_seat();
			const uint64& boss_player_idx = _bossChange->lead_idx();

			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "PlayBossChangeRes boss_slot : %d , boss_player_idx : %IIu" , boss_slot , boss_player_idx );
		}
	}

#endif
	RoomBroadCast( nCommand , _message , except_playerIdx );
	WatcherBroadCast( nCommand , _message , except_playerIdx );
	if( tocheat )
	RoomBroadCastCUser( nCommand , _message , except_playerIdx );
}

bool cGameRoom::SendRequest(cClientSession* pClientSession, const UINT nCommand, google::protobuf::Message& _message , General::ResultCode errorCode, std::string& errorMessage )
{
	if ( pClientSession == nullptr )
		return false;

	uint64 playerIdx = pClientSession->GetPlayerIdx();

	const auto& iterPlayer = m_mapPlayers.find( playerIdx );
	const auto& iterWatcher = m_mapWatchers.find( playerIdx );

	// 플레이어도 아니고, 관전자도 아니면 전송 실패
	if ( iterPlayer == m_mapPlayers.end() && iterWatcher == m_mapWatchers.end() )
		return false;

	NetLib::cInterfaceIocpContext* pContext = pClientSession->GetContext();
	if ( pContext == nullptr || !pContext->IsActive() )
		return false;



	auto sendBuffer = protoutil::cProtoUtil::GetProtobufBuffer(m_nManagedCommandThreadNumber);
	if ( sendBuffer == nullptr) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cGameRoom::SendRequest is Failed. Serialize Failed. #1");
		return false;
	}

	if (!_message.SerializeToArray( sendBuffer->DataBuffer, sizeof( sendBuffer->DataBuffer))) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cGameRoom::SendRequest is Failed. Serialize Failed. #2");
		return false;
	}

	size_t nSize = static_cast<size_t>(_message.ByteSizeLong());

	PmNet::PktBase _response;
	_response.set_payload( sendBuffer->DataBuffer, nSize);
	_response.set_payload_size(static_cast<google::protobuf::uint32>(nSize));
	//_response.set_err_kind( General::ResultCode::Result_Success );
	_response.set_err_kind( errorCode );
	_response.set_errtag( errorMessage );

	if (!_response.SerializeToArray( sendBuffer->SerializeBuffer, sizeof( sendBuffer->SerializeBuffer))) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cGameRoom::SendRequest is Failed. Serialize Failed. #3");
		return false;
	}

	UINT uiSize = static_cast<UINT>(_response.ByteSizeLong());

	NetLib::cPacketStack Packet(CSNet::E_PROTOCOL::E_TCP);
	Packet.Make(nCommand, sendBuffer->SerializeBuffer, uiSize, 0);

	return pContext->SendRequest( Packet.GetBuffer() , Packet.GetLength() );
}