#include "cGameRoomManager.h"
#include "cGameRoom.h"
#include "cClientSession.h"
#include "cInstancePacketParser.h"
#include "cProtoUtil.h"
#include "cDataLoader.h"
#include "cConfigReader.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../Include/Netlib/Manager/ServerManager.h"
#include "../Include/Netlib/Manager/cSessionManager.h"
#include "../Include/Netlib/Manager/cCommandQueueManager.h"
#include "../Include/Netlib/Network/cPacketStack.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include "../Include/Netlib/Session/cSession.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include <iostream>
#include <format>

// static 으로 처리하는 것들은 환경파일로 뺄것
UINT cGameRoomManager::MAX_ROOMS = 1000;
UINT cGameRoomManager::MAX_USER = 9;

cGameRoomManager::cGameRoomManager()
{
	Init();
}

cGameRoomManager::~cGameRoomManager()
{
	Destroy();
}

void cGameRoomManager::Init()
{
	TServerConfiguration* pServerConfig = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance()->GetConfiguration();
	if (pServerConfig == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cGameRoomManager::Init Failed. TServerConfiguration is nullptr");
		assert(false && "cGameRoomManager::Init() is Failed. TServerConfiguration is nullptr");
		return;
	}

	// 랜덤 디바이스 시드값
	gen.seed( std::random_device{}( ) );

	// 코어 갯수에 따라, 방을 쓰레드별로 할당한다.
	int physicalProcessor, logicalProcessor;
	physicalProcessor = logicalProcessor = 0;
	GetProcessorNumber(physicalProcessor, logicalProcessor);

	DWORD dwCommandQueueCnt = static_cast<DWORD>(NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueueCnt());
	int nNumberOfRoomsPerThread = pServerConfig->nRoomCntPerThread;

	// 총 방의 생성 갯수는 커맨드 쓰레드 갯수 * 쓰레드당 허용하는 방갯수
	size_t stRoomNum = dwCommandQueueCnt * nNumberOfRoomsPerThread;

	for (size_t n = 1; n <= stRoomNum; ++n)
	{
		int nManagedThread = n % dwCommandQueueCnt;// thread 어레이 결정

		cGameRoom* pGameRoom = new cGameRoom();

		pGameRoom->SetCommandThread(nManagedThread);

		m_gameRoomPooler.insert( std::pair<int, cGameRoom*>( pGameRoom->GetRoomNumber() , pGameRoom ));
	}

	// 친구 방 생성한다.
	for ( int n = 0; n < stRoomNum; ++n ) {

		m_friendRoomPooler.push( new cGameRoom() );
	}

	m_friend_dis = std::uniform_int_distribution<>( 1 , 9000 );
	/*m_friend_dis_lobby_1 = std::uniform_int_distribution<>( 1 , 4500 );
	m_friend_dis_lobby_2 = std::uniform_int_distribution<>( 4501 , 9000 );*/

	// 방제 초기화 한다.
	cGameRoom::InitializeTitles();

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
		"cGameRoomManager::Init Room Created, Total Count [ %d ]", stRoomNum);
}

void cGameRoomManager::Destroy()
{
	std::map<int , cGameRoom*>::iterator iter = m_gameRoomPooler.begin();
	while ( iter != m_gameRoomPooler.end() )
	{
		cGameRoom* pGameRoom = iter->second;
		if ( pGameRoom != nullptr )
		{
			delete pGameRoom;
			pGameRoom = nullptr;
		}

		++iter;
	}
}

// 사라지지 않는 방을 만든다.
// ChannelInfo 에 DefaultRoom 갯수가 활성화 되어 있는 만큼 생성


cGameRoom* cGameRoomManager::FindPlayingGameRoom(UINT uiGameRoomNum)
{
	/*const int nManagedThread = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetRoomThreadNumber(uiGameRoomNum);
	if (nManagedThread < 0)
		return nullptr;*/

	auto iter = m_playingContainer.find( uiGameRoomNum );
	if ( iter == m_playingContainer.end() || iter->second == nullptr)
		return nullptr;

	return iter->second;
}

// 관전자면 true 를 리턴
// 그외에는 모두 false 를 리턴
bool cGameRoomManager::RemoveWatcherResource( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr ) return false;

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = nullptr;
	std::map<int , cGameRoom*>::iterator iter = m_playingContainer.find( roomNumber );
	if ( iter == m_playingContainer.end() ) return false;

	pGameRoom = iter->second;
	if ( pGameRoom == nullptr ) return false;

	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance == nullptr ) return false;
	pGameRoom->RoomOut( pClientSession->GetPlayerIdx() , pClientSession );
	return true;
	// 관전자 리소스 정리
	if ( pGameRoom->isWatcher( pClientSession->GetPlayerIdx() ) )
	{
		//// 참여 대기 취소
		//gameInstance->RemoveParticipationQueue( pClientSession );

		//gameInstance->CancelWatcherReservation( pClientSession );

		//if ( gameInstance->HasSlotReservation( pClientSession ) )
		//{
		//	switch ( gameInstance->GetGameType() )
		//	{
		//	case General::PlayCategory::PlayCategory_Blackjack:
		//	{
		//		// 주인이 나갔으니 더미들도 내보낸다.
		//		static_cast< cBlackjack* >( gameInstance )->RemoveDummyPlayer( pClientSession->GetPlayerIdx() );

		//		gameInstance->RemoveReservation( pClientSession );
		//	}
		//	break;
		//	case General::PlayCategory::PlayCategory_Baccarat:
		//	case General::PlayCategory::PlayCategory_LowBadugi:
		//	case General::PlayCategory::PlayCategory_TexasHoldem:
		//	{
		//		gameInstance->RemoveReservation( pClientSession );
		//	}
		//	break;
		//	}
		//}

		return true;
	}
}
bool cGameRoomManager::CheckUserGameRoomOut( cClientSession* pClientSession )
{
	bool b_roomout = UserGameRoomOut( pClientSession );
	if ( false == b_roomout )
	{
		const int64 playerIdx = pClientSession->GetPlayerIdx();
		UINT roomNumber = pClientSession->GetJoinedRoomNumber();
		cGameRoom* pGameRoom = nullptr;
		std::map<int , cGameRoom*>::iterator iter = m_playingContainer.find( roomNumber );

		if ( iter == m_playingContainer.end() ) {
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
				"cGameRoomManager::UserGameRoomOut Falied. Pair2 is nullptr RoomNumber[ %u ]" , roomNumber );
			return false;
		}

		// 자동으로 나가기 예약
		pGameRoom = iter->second;
		bool b_roomoutreseve = pClientSession->ReverseRoomOutReserve();
		PmNet::ChamberLeaveHoldRS _outRes;
		_outRes.set_chamber_no( pGameRoom->GetRoomNumber() );
		_outRes.set_hold_ok( b_roomoutreseve );
		_outRes.set_revoke_ok( !b_roomoutreseve );
		_outRes.set_member_idx( pClientSession->GetPlayerIdx() );
		pGameRoom->BroadCastToAllPlayer( General::Packet_LeaveReserve , _outRes );
	}
	return b_roomout;
}
bool cGameRoomManager::UserGameRoomOut( cClientSession* pClientSession )
{
	if ( pClientSession == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cGameRoomManager::UserGameRoomOut Falied. cClientSession is nullptr" );
		return false;
	}
	const int64 playerIdx = pClientSession->GetPlayerIdx();
	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	cGameRoom* pGameRoom = nullptr;
	std::map<int , cGameRoom*>::iterator iter = m_playingContainer.find( roomNumber );

	if ( iter == m_playingContainer.end() ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoomManager::UserGameRoomOut Falied. Pair2 is nullptr RoomNumber[ %u ]" , roomNumber );
		return false;
	}

	pGameRoom = iter->second;
	if ( pGameRoom == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoomManager::UserGameRoomOut Falied. GameRoom is nullptr RoomNumber[ %u ] PlayerIdx[ %I64d ]" , roomNumber , playerIdx );
		return false;
	}

	// 관전자이면 방에서 바로 나가고 종료
	auto gameInstance = pGameRoom->GetGameInterface();
	if ( gameInstance != nullptr ) {

		// 관전자 리소스 정리
		if ( pGameRoom->isWatcher( pClientSession->GetPlayerIdx() ) || gameInstance->HasSlotReservation( pClientSession ) )
		{
			pGameRoom->RoomOut( playerIdx , pClientSession );

			//// 참여 대기 취소
			//gameInstance->RemoveParticipationQueue( pClientSession );

			//if ( gameInstance->HasSlotReservation( pClientSession ) )
			//{
			//	switch ( gameInstance->GetGameType() )
			//	{
			//	case General::PlayCategory::PlayCategory_Blackjack:
			//	{
			//		// 주인이 나갔으니 더미들도 내보낸다.
			//		static_cast< cBlackjack* >( gameInstance )->RemoveDummyPlayer( pClientSession->GetPlayerIdx() );

			//		gameInstance->RemoveReservation( pClientSession );
			//	}
			//	break;
			//	case General::PlayCategory::PlayCategory_Baccarat:
			//	case General::PlayCategory::PlayCategory_LowBadugi:
			//	case General::PlayCategory::PlayCategory_TexasHoldem:
			//	{
			//		gameInstance->RemoveReservation( pClientSession );
			//	}
			//	break;
			//	}
			//}


			return true;
		}
	}

	switch ( gameInstance->GetGameType() )
	{
	
	case General::PlayCategory::PlayCategory_TexasHoldem:
	{
		// RoomState_Waiting 이면 즉시 나감
		// RoomState_InPlay 이면 나가기 예약
		// 메시지 전송은 함수 내부에서 처리
		switch ( pGameRoom->GetRoomStatus() )
		{
		case General::RoomState::RoomState_Waiting:
		{
			if ( pGameRoom->RoomOut( playerIdx , pClientSession ) ) {

				// 홀덤, 바둑이, 플레이 가능한 인원이하인 경우로 가면 Wait 상태로 다시 간다.
				switch ( gameInstance->GetGameType() )
				{
				
				case General::PlayCategory::PlayCategory_TexasHoldem:
				{
					const int& minPlayerCount = gameInstance->GetMinStartPlayerCount();
					if ( pGameRoom->GetJoinedCnt() > 0 && pGameRoom->GetJoinedCnt() < minPlayerCount ) {

						if ( gameInstance->GetGameType() == General::PlayCategory::PlayCategory_TexasHoldem ) {
							cHoldem* pHoldem = static_cast< cHoldem* >( gameInstance );
							Server::PlayPhase before = pHoldem->GetGameStep();
							pHoldem->MakeRoomStatusWait();
							pHoldem->SendStatusChange( before , "cGameRoomManager::UserGameRoomOut" );
						}
					}
				}
				break;
				}

				// 인원이 하나도 없으면 풀러로 돌려 줍시다.
				if ( pGameRoom->PlayingRoomRemoveCheck() ) {

					// 불멸의방!! 이 아닌 경우만 지운다.
					if ( false == pGameRoom->isImmotalRoom() ) {

						// 관전자만 있으면 내보낸다.
						pGameRoom->RemoveWatchars();

						pGameRoom->DeleteGameInterface();

						m_playingContainer.erase( iter );

						// 친구방 풀러에 다시 넣어주어야 한다.
						if ( pGameRoom->GetRoomType() == General::RoomAccessMode::RoomAccess_FriendOnly ) {
							m_friendRoomPooler.push( pGameRoom );
						}
					}

				}
				return true;
			}
		}
		break;
		case General::RoomState::RoomState_InPlay:
		{
			bool reserve = true;
			bool reserve_cancel = false;
			if ( pClientSession->isDie() )//다이일경우 바로나가기 
			{
				cClientSession* t_session = new cClientSession( *pClientSession );
				gameInstance->SwapWithDummy( pClientSession , t_session );
				pGameRoom->RoomOut( playerIdx , pClientSession );
				return true;
			}
			else//다이상태가 아닐경우 나가기 예약
			{
				pGameRoom->RoomOutReservationReverse( pClientSession->GetPlayerIdx() , pClientSession , reserve , reserve_cancel );
				return true;
			}
		}
		break;
		default:
		{
			if ( pGameRoom->GetRoomStatus() != General::RoomState::RoomState_Waiting ) {
				PmNet::ChamberLeaveRS response;
				std::string errorString = protoutil::cProtoUtil::ErrorCodeString( "[ RoomOut ] failed. InvalidOperation." );
				pGameRoom->SendRequest( pClientSession , General::PacketID::Packet_SpaceLeave , response , General::ResultCode::Result_ActionRejected , errorString );
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , errorString.c_str() );
			}
		}
		break;
		}
	}
	break;
	// 바카라 룸 아웃
	// Wait 상태 일때는 즉시 RoomOut
	// Bet 상태 일때는 베팅금액 회수하고 RoomOut
	// 그외 상태에서는 베팅금액이 없을때만 RoomOut
	}

	return false;
}

cClientSession* cGameRoomManager::GameRoomFindSession(const int nManagedThread, const UINT uiGameRoomNum, const General::RoomAccessMode roomType, const int64 playerIdx)
{
	auto iter = m_playingContainer.find(uiGameRoomNum);
	if ( iter == m_playingContainer.end() )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::FindSession Falied. Pair2 is nullptr PVP_TYPE[ %d ] ManagedThread[ %d ] RoomNumber[ %u ]",
			roomType,
			nManagedThread,
			uiGameRoomNum);
		return nullptr;
	}

	cGameRoom* pGameRoom = iter->second;
	if (pGameRoom == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::FindSession Falied. pPair1->m_value is nullptr PVP_TYPE[ %d ] ManagedThread[ %d ] RoomNumber[ %u ]",
			roomType,
			nManagedThread,
			uiGameRoomNum);
		return nullptr;
	}

	return pGameRoom->FindSession(playerIdx);
}

BOOL cGameRoomManager::GameRoomBroadCast(cClientSession* pClientSession, const int nManagedThread, const UINT nCommand, google::protobuf::Message& _message, BOOL bExceptMe)
{
	if (pClientSession == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::GameRoomBroadCast Falied. ClientSession is nullptr.");
		return false;
	}

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();

	auto iter = m_playingContainer.find( roomNumber );
	if ( iter == m_playingContainer.end() )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::GameRoomBroadCast Falied. Pair2 is nullptr ManagedThread[ %d ] RoomNumber[ %u ]",
			nManagedThread, roomNumber);
		return false;
	}

	const int64 playerIdx = pClientSession->GetPlayerIdx();

	cGameRoom* pGameRoom = iter->second;
	if (pGameRoom == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::GameRoomBroadCast Falied. GameRoom is nullptr ManagedThread[ %d ] RoomNumber[ %u ] AccountIDX[ %I64d ]",
			nManagedThread, roomNumber);
		return false;
	}


// GetProtobufBuffer
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer(nManagedThread);
	if (pGOOGLE_PROTOBUF_BUFFER == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "GameRoomBroadCast is Failed. Serialize Failed. #1");
		return FALSE;
	}

	if (!_message.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, sizeof(pGOOGLE_PROTOBUF_BUFFER->DataBuffer)))
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "GameRoomBroadCast is Failed. Serialize Failed. #2");
		return FALSE;
	}

	uint32 uiMessageSize = static_cast<uint32>(_message.ByteSizeLong());

	PmNet::PktBase _response;
	_response.set_payload(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, uiMessageSize);
	_response.set_payload_size(uiMessageSize);
	_response.set_err_kind( General::ResultCode::Result_Success );

	if (!_response.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, sizeof(pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer)))
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "GameRoomBroadCast is Failed. Serialize Failed. #3");
		return FALSE;
	}

	uint32 uiSize = static_cast<uint32>(_response.ByteSizeLong());

	// 방 전체에게 데이터 전송
	if (bExceptMe)
		pGameRoom->RoomBroadCast(nCommand, pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, uiSize, playerIdx);
	else
		pGameRoom->RoomBroadCast(nCommand, pGOOGLE_PROTOBUF_BUFFER->SerializeBuffer, uiSize);

	return true;
}

BOOL cGameRoomManager::GameRoomBroadCast(const int nGameRoomNumber, const int nManagedThread, const UINT nCommand, google::protobuf::Message& _message)
{
	auto iter = m_playingContainer.find( nGameRoomNumber );
	if ( iter == m_playingContainer.end() )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::GameRoomFindCharacterAttribute Falied. Pair2 is nullptr ManagedThread[ %d ] RoomNumber[ %d ]",
			nManagedThread,
			nGameRoomNumber);
		return FALSE;
	}

	cGameRoom* pGameRoom = iter->second;
	if (pGameRoom == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::GameRoomFindCharacterAttribute Falied. pPair1->m_value is nullptr ManagedThread[ %d ] RoomNumber[ %d ]",
			nManagedThread,
			nGameRoomNumber);
		return FALSE;
	}

	return pGameRoom->RoomBroadCast(nCommand, _message);
}

BOOL cGameRoomManager::SendRoomJoin(cClientSession* pClientSession, const int nManagedThread)
{
	if (pClientSession == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::SendRoomJoin Falied. ClientSession is nullptr.");
		return FALSE;
	}

	UINT roomNumber = pClientSession->GetJoinedRoomNumber();
	auto iter = m_playingContainer.find( roomNumber );
	if ( iter == m_playingContainer.end() )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::SendRoomJoin Falied. Pair2 is nullptr ManagedThread[ %d ] RoomNumber[ %u ]",
			nManagedThread, roomNumber);
		return FALSE;
	}

	const int64 playerIdx = pClientSession->GetPlayerIdx();

	cGameRoom* pGameRoom = iter->second;
	if (pGameRoom == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::SendRoomJoin Falied. GameRoom is nullptr ManagedThread[ %d ] RoomNumber[ %u ] PlayerIdx[ %I64d ]",
			nManagedThread, roomNumber, playerIdx);

		return FALSE;
	}

	SendRoomJoin(pClientSession, nManagedThread);

	return TRUE;
}

// 방을 사용중인 상태로 변경한다.
// 이미 사용중이면 무시한다.
bool cGameRoomManager::OccupiedRoom(
	const Server::Channel& channelData,
	const General::RoomAccessMode& roomType,
	const General::BetPolicy& bettingRuleType,
	const int32& show_down_delay_ms_per_player ,
	const int32& show_down_community_delay_ms_per ,
	cGameRoom* pGameRoom, 
	const int nManagedThread, 
	uint32 uIp, 
	const int max_players,
	const bool& createFlag )
{
	if ( channelData.game_type() != General::PlayCategory::PlayCategory_TexasHoldem )
		return false;

	if (pGameRoom == nullptr)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
			"cGameRoomManager::OccupiedRoom Falied. cGameRoom is nullptr.");
		return false;
	}
	if ( m_playingContainer.contains( pGameRoom->GetRoomNumber() ) ) {
		// Diagnostic: race vs erase-leak vs pool-mismatch.
		//   sameInstance=1 -> race (same pGameRoom occupied twice concurrently)
		//   sameInstance=0 -> different ptr reused same room number (erase miss / pool bug)
		//   threadIdx differs across consecutive hits -> multi-thread race
		auto existingIt = m_playingContainer.find( pGameRoom->GetRoomNumber() );
		cGameRoom* existing = ( existingIt != m_playingContainer.end() ) ? existingIt->second : nullptr;
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"OccupiedRoom DUP: roomNum=%u newPtr=%p existingPtr=%p sameInstance=%d threadIdx=%d",
			pGameRoom->GetRoomNumber() , pGameRoom , existing ,
			( pGameRoom == existing ) ? 1 : 0 , nManagedThread );
		return false;
	}

	/*auto iter = m_playingContainer.find(  );
	if ( iter != m_playingContainer.end() )
		return false;*/

	m_playingContainer.insert( std::pair<int , cGameRoom*>( pGameRoom->GetRoomNumber() , pGameRoom ) );

	// 게임방 관련된 기본 셋팅
	pGameRoom->Clear();
	pGameRoom->SetGameType( channelData.game_type() );
	pGameRoom->SetRoomStatus(General::RoomState::RoomState_Waiting);
	pGameRoom->SetRoomType( roomType );
	pGameRoom->SetSeedMoneyTypeAndValue( channelData.money_type() , channelData.seed_money() );
#ifdef _DEBUG
	pGameRoom->SetUip(uIp);
#endif
	pGameRoom->SetBettingRuleType( bettingRuleType );
	pGameRoom->m_roomInfo.set_channel_code( channelData.id() );

	// 방 입장인원 셋팅, 방을 생성하는 경우라면, 입력받은 대로 설정한다.
	int max_player_count_set = 0;
	if ( createFlag )
		max_player_count_set = max_players;
	else
	{
		switch ( channelData.game_type() )
		{
		case General::PlayCategory::PlayCategory_TexasHoldem:
			max_player_count_set = 9;
			break;
		}
	}

	//auto& test = channelData->id();

	IGame* pGame = nullptr;
	switch ( channelData.game_type() )
	{
	case General::PlayCategory::PlayCategory_TexasHoldem:
		pGame = new cHoldem( pGameRoom , bettingRuleType , max_player_count_set , channelData, show_down_delay_ms_per_player, show_down_community_delay_ms_per );
		break;
	default:
		throw new std::exception( "OccupiedRoom GameType Error" );

	}
	pGameRoom->SetGameInterface( pGame );

	// 성공 로그
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
		"cGameRoomManager::OccupiedRoom Success. ManagerdThread [ %d ] RoomNumber [ %d ]",
		nManagedThread, pGameRoom->GetRoomNumber());

	return true;
}

void cGameRoomManager::Process(const UINT uiThread)
{
	std::vector<int> removeRoomKeys;

	// 해당 스레드가 관리 하고 있는 방을 전체 돌려줍니다.
	{
		int immotalRoomCount = 0;

		auto iter = m_playingContainer.begin();
		while ( iter != m_playingContainer.end() )
		{
			cGameRoom* pGameRoom = iter->second;
			if ( pGameRoom != nullptr )  {
				IGame* gameInterface = pGameRoom->GetGameInterface();
				if ( gameInterface == nullptr ) continue;

				/*if ( gameInterface->GetGameType() == General::PlayCategory::PlayCategory_LowBadugi ) {
					int n = 1;
				}*/

				// 게임 타입 별로 다르게 처리한다.
				switch ( gameInterface->GetGameType() )
				{
				//case Common::GameType::GameType_Roulette:
				//{
				//	// 바카라는 Openning 모드인 경우에는 플레이어가 없더라도 진행시킨다.
				//	// 사라지지 않는 방은 오프닝을 플레이어가 없더라도 계속
				//	if ( pGameRoom->isImmotalRoom() )
				//	{
				//		gameInterface->Process();
				//	}
				//	else
				//	{
				//		// 일반방은 
				//		if ( pGameRoom->PlayingRoomRemoveCheck() ) {

				//			// 관전자만 있으면 내보낸다.
				//			pGameRoom->RemoveWatchars();

				//			pGameRoom->Clear();
				//			removeRoomKeys.push_back( pGameRoom->GetRoomNumber() );	// 키 값만 모아 둡시다.
				//		}
				//		else
				//		{
				//			gameInterface->Process();
				//		}
				//	}
				//}
				//break;
				default:
				{
					//if ( gameInterface != nullptr && pGameRoom->GetJoinedCnt() != 0 )
					if ( pGameRoom->GetJoinedCnt() != 0 || pGameRoom->GetRoomStatus() == General::RoomState::RoomState_InPlay )
					{
						gameInterface->Process();				// 게임 스텝에 따른 처리
						//gameInterface->OnCheckVoteExpired();	// Vote 시스템 투표 시간 체크
					}
					// 방에 플레이어가 없고 관전자도 없을 경우 초기화 한다.
					else if ( pGameRoom->PlayingRoomRemoveCheck() )
					{
						// 불멸의방!! 이 아닌 경우만 지운다.
						if ( false == pGameRoom->isImmotalRoom() ) {

							// 관전자만 있으면 내보낸다.
							pGameRoom->RemoveWatchars();

							// 친구방 풀러에 다시 넣어주어야 한다.
							if ( pGameRoom->GetRoomType() == General::RoomAccessMode::RoomAccess_FriendOnly ) {
								m_friendRoomPooler.push( pGameRoom );
							}

							pGameRoom->Clear();
							removeRoomKeys.push_back( pGameRoom->GetRoomNumber() );	// 키 값만 모아 둡시다.
						}
						
					}
				}
				break;
				}
			}
			++iter;
		}
	}

	if ( removeRoomKeys.size() <= 0 )
		return;

	for ( auto key : removeRoomKeys ) {
		auto iter = m_playingContainer.find( key );
		if ( iter != m_playingContainer.end() )
			m_playingContainer.erase( key );
	}
	
	return;
}

// 쓰레드에 바로 접근해서 플레이 중인 방을 뽑아온다.
cGameRoom* cGameRoomManager::PopPlayingRoom( const Server::Channel& channelData , const General::BetPolicy& bettingRueType , cClientSession* pClientSession , const int nManagedThread )
{
	const UINT roomNumberBefore = pClientSession->GetJoinedRoomNumberbefore();
	const uint64 playerIdx = pClientSession->GetPlayerIdx();

	/*if ( General::RoomAccessMode::RoomAccess_None >= roomType ||
		General::RoomAccessMode::RoomAccessMode_INT_MAX_SENTINEL_DO_NOT_USE_ < roomType ) {
		std::string log = std::format( "cGameRoomManager::PopPlayingRoom Failed. RoomType Range Over! [{}]" , (int)roomType );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
		return nullptr;
	}*/

	// 플레이 중인 방 검색
	// 해당 스레드가 관리 하고 있는 방을 전체 돌려줍니다.
	std::vector<cGameRoom*> joinableRooms;
	for ( auto& gameRoomPair : m_playingContainer ) {
		if ( gameRoomPair.second != nullptr )
		{
			// 강제 퇴장 당했던 방은 건너뛴다.
			IGame* gameInterface = gameRoomPair.second->GetGameInterface();
			if ( gameInterface != nullptr && gameInterface->isKickedPlayer( playerIdx ) )
				continue;

			// 강제 퇴장 기록 확인
			if ( gameInterface ->isJoinProhibited( playerIdx ) )
				continue;

			if ( channelData.game_type() != gameInterface->GetGameType())
				continue;

			if ( gameInterface->GetChannelId().compare( channelData.id() ) != 0 )
				continue;

			// 홀덤인 경우에는 베팅룰 타입까지 비교해서 검색한다.
			if ( channelData.game_type() == General::PlayCategory::PlayCategory_TexasHoldem ) {
				if ( bettingRueType != gameInterface->GetBettingRuleType() )
					continue;
			}

			// 자유방은 바두기만 있다.
			// ChannelPlayMode_None 이 자유 방이다.
			

			// 친구 방은 검색이 되지 않도록 한다.
			if ( gameRoomPair.second->GetRoomType() == General::RoomAccessMode::RoomAccess_FriendOnly )
				continue;
			
			if ( gameRoomPair.second->GetRoomNumber() != roomNumberBefore &&
				gameRoomPair.second->JoinPossible() == General::ResultCode::Result_Success ) {
				//pGameRoom = gameRoomPair.second;
				//break;
				joinableRooms.push_back( gameRoomPair.second );
			}
		}
	}

	if ( joinableRooms.size() == 0 )
		return nullptr;

	// 방중에 랜덤으로 보내줍니다.
	int randomNumber = m_dis( gen ); // 난수 생성
	int selectedArray = randomNumber % joinableRooms.size();
	return joinableRooms[ selectedArray ];
}

cGameRoom* cGameRoomManager::PopPlayingRoomExceptSameIp( General::RoomAccessMode roomType , uint32 uIp , const int nManagedThread )
{
	if ( General::RoomAccessMode::RoomAccess_None >= roomType ||
		General::RoomAccessMode::RoomAccessMode_INT_MAX_SENTINEL_DO_NOT_USE_ < roomType ) {
		std::string log = std::format( "cGameRoomManager::PopPlayingRoom Failed. RoomType Range Over! [{}]" , ( int ) roomType );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
		return nullptr;
	}

	// 플레이 중인 방 검색
	// 해당 스레드가 관리 하고 있는 방을 전체 돌려줍니다.
	cGameRoom* pGameRoom = nullptr;
	auto iter = m_playingContainer.begin();
	while ( iter != m_playingContainer.end() )
	{
		auto iterOld = iter;
		++iter;
		pGameRoom = iterOld->second;

		// 같은 IP 는 걸러낸다.
#ifdef _DEBUG
		if ( pGameRoom->GetUip() == uIp )
			continue;
#endif

		if ( pGameRoom->JoinPossible() )
			break;
	}
	return pGameRoom;
}

// 서버 로딩시 사라지지 않는 방을 생성시에만 쓴다.
cGameRoom* cGameRoomManager::PopForRoomCreation( const int nManagedThread )
{
	// 플레이 중이 아닌 방 검색
	cGameRoom* pGameRoom = nullptr;

	for ( auto roomiter = m_gameRoomPooler.begin(); roomiter != m_gameRoomPooler.end(); ++roomiter ) {
		auto room = roomiter->second;

		if ( room == nullptr )
			continue;

		if ( room->GetGameInterface() != nullptr )
			continue;

		pGameRoom = room; 
		break;
	}

	return pGameRoom;
}

// 현재 쓰레드에서 빈방을 하나 뽑아온다.
cGameRoom* cGameRoomManager::PopEmptyRoom(General::RoomAccessMode roomType, cClientSession* pClientSession , const int nManagedThread)
{
	const UINT roomNumberBefore = pClientSession->GetJoinedRoomNumberbefore();
	const uint64 playerIdx = pClientSession->GetPlayerIdx();

	if (General::RoomAccessMode::RoomAccess_None >= roomType ||
		General::RoomAccessMode::RoomAccessMode_INT_MAX_SENTINEL_DO_NOT_USE_ < roomType) {
		std::string log = std::format( "cGameRoomManager::Pop Failed. RoomType Range Over! [{}]" , int(roomType) );
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , log.c_str() );
		return nullptr;
	}

	// 플레이 중이 아닌 방 검색
	cGameRoom* pGameRoom = nullptr;
	std::vector<cGameRoom*> joinableRooms;

	for ( auto roomiter = m_gameRoomPooler.begin(); roomiter != m_gameRoomPooler.end(); ++roomiter ) {
		auto room = roomiter->second;

		if ( room == nullptr ) continue;

		// 빈방을 검색하는 로직이므로
		// Interface가 있는 방은 생략
		if ( room->GetGameInterface() != nullptr )
			continue;

		// 마지막에 조인했던 방 Pass
		// 빈방이 아니면 Pass
		if ( room->GetRoomNumber() == roomNumberBefore || room->GetJoinedCnt() != 0 ) continue;

		// 친구 방은 검색이 되지 않도록 한다.
		if ( room->GetRoomType() == General::RoomAccessMode::RoomAccess_FriendOnly ) continue;

		joinableRooms.push_back( room );
		//pGameRoom = room; break;
	}

	if ( joinableRooms.size() == 0 ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
			"cGameRoomManager::Pop Failed. Game Room in the Memory Pool is nullptr. No More Room" );
		return nullptr;
	}

	// 방중에 랜덤으로 보내줍니다.
	int randomNumber = m_dis( gen ); // 난수 생성
	int selectedArray = randomNumber % joinableRooms.size();
	return joinableRooms[ selectedArray ];
}

cGameRoom* cGameRoomManager::PopFriendRoom()
{
	if ( m_friendRoomPooler.size() == 0 ) return nullptr;

	cGameRoom* pGameRoom = m_friendRoomPooler.front();
	if ( pGameRoom != nullptr )
		m_friendRoomPooler.pop();

	return pGameRoom;
}

// 방번호를 검색해서 찾아준다.
cGameRoom* cGameRoomManager::FindRoom( const int roomNumber , const int nManagedThread )
{
	auto iter = m_gameRoomPooler.find( roomNumber );
	if ( iter == m_gameRoomPooler.end() )
		return nullptr;

	if ( iter->second != nullptr )
		return iter->second;
	return nullptr;
}

cGameRoom* cGameRoomManager::FindPlayingRoom( const int roomNumber , const int nManagedThread )
{
	auto iter = m_playingContainer.find( roomNumber );
	if ( iter == m_playingContainer.end() )
		return nullptr;

	if ( iter->second != nullptr )
		return iter->second;
	return nullptr;
}

// 방에서 게임중인 유저들의 쿨타임 처리
BOOL cGameRoomManager::Update(const UINT uiThread, const UINT uiUpdateType)
{
	// 해당 스레드가 관리 하고 있는 방을 전체 돌려줍니다.
	auto iter = m_playingContainer.begin();
	while ( iter != m_playingContainer.end() )
	{
		auto iterOld = iter;
		++iter;
		cGameRoom* pGameRoom = iterOld->second;
		/*
		if (pGameRoom && pGameRoom->GetGameState() == GAME_STATE_START)
		{
			pGameRoom->Update(uiThread, uiUpdateType);
		}
		*/
	}
	return TRUE;
}

size_t cGameRoomManager::UseGameRoomCnt()
{
	return m_playingContainer.size();
}

google::protobuf::RepeatedField<General::RoomListEntry> cGameRoomManager::GetRoomList( const General::PlayCategory& gameType , const std::map<std::string , int>& channel_ids , int& totalCount , int& totalPage , const UINT uiThread , int startIndex , int pageSize )
{
	totalCount = 0;
	totalPage = 0;

	std::vector<std::pair<int , cGameRoom*>> roomList;

	// 조건에 해당하는 방 검색
	{
		for ( auto pair : m_playingContainer )
		{
			if ( pair.second == nullptr )
				continue;

			// 채널 타입 정보가 있을 경우에만 채널 타입을 구분한다.
			if ( channel_ids.size() != 0 ) {

				auto iter = channel_ids.find( pair.second->GetGameInterface()->GetChannelId() );
				if ( iter == channel_ids.end() )
					continue;
			}

			// 친구 방은 건너 뛴다.
			if ( pair.second->GetRoomType() == General::RoomAccessMode::RoomAccess_FriendOnly )
				continue;

			// 게임 타입 정보가 있을 경우
			if ( gameType != General::PlayCategory::PlayCategory_None )
				if ( gameType != pair.second->GetGameType() )
					continue;

			roomList.push_back( pair );
		}
	}

	google::protobuf::RepeatedField<General::RoomListEntry> returnList;

	// 방목록이 없으면 그대로 종료
	if ( roomList.size() == 0 )
		return returnList;

	totalCount = roomList.size();

	// pageSize 에 맞게 페이지 자른다.
	int roomCount = roomList.size();
	if ( roomCount % pageSize == 0 ) {
		totalPage = roomCount / pageSize;
	}
	else {
		totalPage = roomCount / pageSize;
		totalPage += 1;
	}

	// startIndex 는 0부터 시작해서 0 + pageSize 까지 내려준다.
	int endIndex = startIndex + pageSize;

	// startIndex 조건 체크
	if ( startIndex > roomList.size() || startIndex < 0 )
		return returnList;

	for ( int roomPos = startIndex; roomPos < endIndex; ++roomPos ) {


		if ( roomPos > roomList.size() - 1 )
			break;

		auto gameroom = roomList[ roomPos ].second;
		if ( gameroom == nullptr )
			continue;

		General::RoomListEntry roomListInfo;
		gameroom->GetRoomInfo( roomListInfo );

		returnList.Add( roomListInfo );
	}

	return returnList;
}

// 10000 ~ 99999 까지 생성하도록한다.
// 9999 + ( 1 ~ 90000 )
//int cGameRoomManager::CreateFriendRoomNumber( const int server_id )
//{
//	// 생성한 방이 플레이 중인 방이면 재 생성
//	int gen_num = -1;
//
//	while ( true )
//	{
//		if ( server_id == 1 )
//			gen_num = 999 + m_friend_dis_lobby_1( gen );
//		else
//			gen_num = 999 + m_friend_dis_lobby_2( gen );
//
//		auto iter = m_playingContainer.find( gen_num );
//		if ( iter == m_playingContainer.end() )
//			break;
//	}
//
//	return gen_num;
//}

// 10000 ~ 99999 까지 생성하도록한다.
// 9999 + ( 1 ~ 90000 )
int cGameRoomManager::CreateFriendRoomNumber()
{
	// 생성한 방이 플레이 중인 방이면 재 생성
	int gen_num = -1;
	while ( true )
	{
		gen_num = 999 + m_friend_dis( gen );
		auto iter = m_playingContainer.find( gen_num );
		if ( iter == m_playingContainer.end() )
			break;
	}

	return gen_num;
}

// 방전체 목록을 뽑아서 로비 서버 끼리 싱크 시도한다.
google::protobuf::RepeatedField<General::RoomListEntry> cGameRoomManager::GetPlayingRoomListForLobbySync( const General::PlayCategory gameType )
{
	google::protobuf::RepeatedField<General::RoomListEntry> returnList;

	// 조건에 해당하는 방 검색
	{
		for ( auto pair : m_playingContainer )
		{
			auto gameroom = pair.second;

			if ( gameroom == nullptr )
				continue;

			// 게임
			if ( gameroom->GetGameType() != gameType )
				continue;
	
			General::RoomListEntry roomListInfo;
			gameroom->GetRoomInfo( roomListInfo );

			returnList.Add( roomListInfo );
		}
	}

	return returnList;
}