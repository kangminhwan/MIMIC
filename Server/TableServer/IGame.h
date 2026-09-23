#pragma once

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/Server.pb.h"

#include "cSidePot.h"
#include "cVoteSystem.h"

#define MAXRESERVECOUNT 3
// 게임들의 기본 인터페이스
class cGameRoom;
class cClientSession;
//class IGame : public cVoteSystem , public cSidePotManager // 강제 퇴장 시스템 폐기
class IGame : public cSidePotManager
{
protected:
	General::PlayCategory m_gameType;
	cGameRoom* m_pGameRoom;
	std::vector<cClientSession*>::iterator m_masterIter;	// 방장 ( 방 생성자 )
	std::vector<cClientSession*> m_playerSlots;				// 생성자에서만 사이즈가 조절하도록 합니다.
	std::vector<cClientSession*> m_slotReservation;			// 슬롯 예약 처리
	std::vector<cClientSession*> m_participateQueue;		// 플레이 참가 예약 큐
	std::map<uint64 , cClientSession*> m_watcherQueue;		// 결과창 이후 등록된 플레이어를 관전자로 변경한다.

	//int m_hostSlotArray;
	bool m_max_bet_player_exists;							// 맥스 베팅에 도달한 유저가 있는 경우 활성화 된다.

	// 카드 갯수도 방마다 다름으로 특성으로
	std::vector<General::PlayingCard> m_deck;

	// 사용된 카드 저장소
	std::vector<General::PlayingCard> m_stockDeck;

	// Tick 처리도 방마다 필요함
	uint64 m_expireTick;
	uint64 m_autoStartTick;

	// Blackjack, Bust, Heat 연출 시간
	//uint64 m_extraTick;

	// 방에서 관리되는 채널 스트링
	Server::Channel m_channel;
	Server::PlayPhase m_gameStep;

	// 수수료
	double m_dealerFeeRate;

	// Money 관련
	uint64 m_maxBetMoney;	// 베팅 할 수 있는 최대 금액
	uint64 m_minMoney;		// 입장 최소 금액
	uint64 m_maxMoney;		// 입장 최대 금액

	// Turn 처리
	ULONGLONG m_turnExpireTick;					// Tick 처리도 방마다 필요함, 플레이 턴 종료처리, 베팅, 카드 교체
	ULONGLONG m_turnExpireTickMinimum;			// Tick 처리 최소 시간 ( 1초로 셋팅 )

	// 강제 퇴장 기능
	std::map<uint64 , uint64> m_kickedPlayers;	// 해당 게임방에서 6번 째 강제 퇴장 플레이어 발생 시, 1번 째 강제 퇴장 플레이어는 해당 게임방 입장 가능
	std::map<uint64 , uint64> m_joinProhibited; // 방조인 금지된 플레이어 등록

	// 베팅 룰 타입
	General::BetPolicy m_bettingRuleType;

	// 게임방 룸ID
	std::string m_gameUid;
	
public:
	virtual void InitGame() = 0;	// 게임 최초 생성시에 사용
	virtual void InitPlayerData() = 0; // 플레이어 데이터만 초기화 한다.
	virtual void ResetGame() = 0;	// 게임 초기화 시에 사용
	virtual int32 Getfirstidx() = 0;
	virtual void Shuffle() = 0;
	virtual int GetTotalCardDeckCount() = 0;						// 최초 카드 덱 카운트
	virtual int GetRemainCardDeckCount() { return m_deck.size(); }	// 남은 카드 덱 카운트
	virtual General::ResultCode EnterSlot(cClientSession* pClientSession, const int slotNumber = 0) = 0;
	virtual bool LeaveSlot( cClientSession* pClientSession , bool b_out = false) = 0;
	virtual bool SwapWithDummy( cClientSession* pClientSession , cClientSession* pDummyClientSession = nullptr) = 0;
	virtual General::ResultCode MoveSlot(cClientSession* pClientSession , const int slotNumber ) = 0;
	virtual bool HasSlot( cClientSession* pClientSession ) = 0;
	virtual bool HasEmptySlot() { return false; } // 로우바둑이 전용
	virtual General::ResultCode InsertParticipationQueue( cClientSession* pClientSession ) {
		int count = 0;
		for ( int slot = 0; slot < m_participateQueue.size(); ++slot ) {
			if ( m_participateQueue[ slot ] == nullptr ) continue;
			if ( m_participateQueue[ slot ] == pClientSession ) return General::ResultCode::Result_PlayQueueAlreadyJoined;
			count++;
		}
		if ( count >= MAXRESERVECOUNT )
			return General::ResultCode::Result_SeatReservationCapacityReached;
		m_participateQueue.push_back( pClientSession );
		return General::ResultCode::Result_Success;
	}
	virtual std::vector<cClientSession*> GetParticipationQueue() { return m_participateQueue; }
	virtual bool IsParticipationPlayer( const uint64 playerIdx ) { return false; }
	virtual bool RemoveParticipationQueue( cClientSession* pClientSession ) = 0; 
	virtual bool CheckSameCI( const std::string& ci ) = 0;
	
	virtual void MakeParticipate(int64 except_playerIdx = 0 ) {};

	virtual General::ResultCode ReservationPlayerTransferWatcher( cClientSession* pClientSession ) { return General::ResultCode::Result_Success; }
	virtual int GetMemberCnt() {
		if ( m_playerSlots.empty() )
			return 0;

		int memberCount = 0;
		for ( int slot = 0; slot < m_playerSlots.size(); ++slot ) {
			if ( m_playerSlots[ slot ] != nullptr ) {
				++memberCount;
			}
		}
		return memberCount;
	}
	virtual int GetPlayerPoolCount() { return m_playerSlots.size(); }
	virtual General::PlayCategory GetGameType() { return m_gameType; }
	virtual int GetCurBettingRound() { return 0; }
	virtual bool HasBetting( cClientSession* pClientSession ) { return false; }
	virtual cClientSession* GetPlayerSession( uint64 playerIdx ) = 0;
	virtual cClientSession* GetPlayerALLSession( uint64 playerIdx ) = 0;
	virtual uint64 GetBossPlayerIdx() = 0;
	virtual void SetBossFirst() = 0;
	virtual std::vector<cClientSession*> GetPlayersSessionList() = 0;
	std::vector<cClientSession*> GetReservationPlayers() { return m_slotReservation; }
	int GetReservationPlayerPlayerCount() {
		int memberCount = 0;
		for ( int slot = 0; slot < m_slotReservation.size(); ++slot ) {
			if ( m_slotReservation[ slot ] != nullptr ) {
				++memberCount;
			}
		}
		return memberCount;
	}
	virtual int GetSubCount( uint64 playeridx ) { return 0; }
	
	virtual google::protobuf::RepeatedField<General::WinningHandHistory> GetRecentlyPlayedGames() = 0;
	cClientSession* FindPlayerByIdx( uint64 playerIdx );


#pragma region 슬롯 예약

	// 슬롯 예약 관련 함수 모음 EnterSlot 에서도 처리함
	virtual void OnSlotReservation() {};
	virtual General::ResultCode SlotReservation( cClientSession* pClientSession , const int slotNumber = 0 ) { return General::ResultCode::Result_Success; }
	virtual bool HasSlotReservation( cClientSession* pClientSession ) { return false; }
	virtual bool RemoveReservation( cClientSession* pClientSession ) { return false; }
	virtual void RemoveSubPlayerReservation( const uint64& playerId ) {};
	cClientSession* GetReservaionSession( const uint64& playeridx );

	

#pragma endregion 슬롯 예약

#pragma region 관전자 관리
	// 관전 신청 ( 플레이어 상태에 따라 즉시 관전, 관전 예약 판별 )
	virtual General::ResultCode TransferWatcher( cClientSession* pClientSession ) = 0;

	// 플레이중인 유저중 관전 예약한 유저를 관전자로 변경한다.
	virtual void OnWatcherReservation() {};
	// 플레이중인 유저가 관전을 예약한다.
	virtual General::ResultCode WatcherReservation( cClientSession* pClientSession ) { return General::ResultCode::Result_Success; }
	// 관전 예약을 취소한다.
	virtual General::ResultCode CancelWatcherReservation( cClientSession* pClientSession ) { return General::ResultCode::Result_Success; }

	bool IsWatcherReservation( const uint64 playerIdx ) {
		auto iter = m_watcherQueue.find( playerIdx );
		return iter != m_watcherQueue.end();
	}

#pragma endregion 관전자 관리

	// General::HandRank baccaraJokbo 는 바카라 전용으로만 사용된다.
	//virtual General::ResultCode UserBet( cClientSession* pClientSession , General::TableAction betting , General::HandRank baccaraJokbo = General::HandRank::HandRank_None ) = 0;
	virtual General::ResultCode UserBet( cClientSession* pClientSession , const PmNet::MatchWagerRQ& userBet ) = 0;
	virtual void ClearPlayerCache();
	//virtual void SendSide() {}

	/*virtual void CalcMoney( const std::vector<uint64>& winners , 
		std::deque<PmNet::MemberOutcome>& sortedLosers , 
		std::vector<PmNet::MemberOutcome>& playersResults , 
		std::map<uint64 , PmNet::MemberOutcome>& playersResultMap );*/

	virtual void CalcMoneyNew( const int& rank, uint64& potMoney, std::vector<PmNet::MemberOutcome>& playerResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap );
	virtual void CalcMoneyLowBa( const int& rank, uint64& potMoney, std::vector<PmNet::MemberOutcome>& playerResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap );
	void CalcMoneyFriends( const int& rank, uint64& potMoney, std::vector<PmNet::MemberOutcome>& playerResults , std::map<uint64 , PmNet::MemberOutcome>& playersResultMap );

	// 1등 처리후, 2등 이하 로직에서 여러명의 랭킹이 동일한 경우의 PotMoney 처리 및 분배
	// 리턴 값 처리한 playersResults Array 마지막 값 ( 만약에 1등 처리후 2, 3, 4 까지가 공동 2등인경우 리턴 값은 3이 된다. )
	/*virtual int CalcMoneySubWin( 
		uint64& potMoney,
		uint64& beforeSidePotBase,
		int startArray,
		std::vector<PmNet::MemberOutcome>& playersResults ,
		std::map<uint64 , PmNet::MemberOutcome>& playersResultMap );*/

#pragma region 방 상태 처리

	virtual void Waiting() = 0;		// 대기시에 사용
	virtual General::ResultCode StartGame(cClientSession* pClientSession) = 0;	// 게임 시작
	virtual void Playing() = 0;		// 플레이 시에 사용
	virtual void Process() = 0;		// 게임방 스텝처리는 이 함수에서만 하도록!!
	virtual bool isTimeOver() {
		return ::GetTickCount64() >= m_expireTick;
	}
	virtual bool isAutoStartTimeOver() {
		return ::GetTickCount64() >= m_autoStartTick;
	}
	virtual void SetNextPlay() = 0;	// 다음 스텝으로 진행 시킵니다.

	inline Server::PlayPhase GetGameStep() {
		return m_gameStep;
	}

	virtual std::string GetChannelId() const {
		return m_channel.id();
	}

	virtual Server::Channel GetChannelData() const { return m_channel; }

	virtual bool GoToNextRound() = 0;			// 다음 라운드로 진행한다.

	inline virtual General::BetPolicy GetBettingRuleType() { return m_bettingRuleType; }

	virtual void LeftCardNoti( cClientSession* pClientSession = nullptr );

	virtual int GetMinStartPlayerCount() { return 1; }	// 방 시작하기 위한 최소 인원. 바카라는 1명일 때도 시작 할 수 있다.

	virtual void GetCurrentStatus( PmNet::MatchStateSwapRS& _res ) {};

	virtual void OnIntruding( cClientSession* pClientSession , bool ignoreCommunity = false , bool relogin = false ) {};	// 난입시에 받을 패킷 처리 처리

	virtual bool IsOpenning() { return false; }

	virtual void EndGame() {};

#pragma endregion 방 상태 처리

#pragma region 방장 관리

	inline virtual void InitMaster() { m_masterIter = m_playerSlots.end(); }

	virtual void SetMaster() {
		if ( m_masterIter == m_playerSlots.end() )
			m_masterIter = m_playerSlots.begin();
	}

	virtual std::vector<cClientSession*>::iterator NextMaster()
	{
		// 멤버가 1명 뿐인 경우
		if ( GetMemberCnt() == 1 ) {
			for ( int n = 0; n < m_playerSlots.size(); ++n ) {
				if ( m_playerSlots[ n ] != nullptr ) {
					m_masterIter = m_playerSlots.begin() + n;
					return m_masterIter;
				}
			}
			// 모든 슬롯이 nullptr인 경우 (발생해서는 안됨)
			return m_playerSlots.end();
		}

		size_t checkedSlots = 0;
		size_t totalSlots = m_playerSlots.size();

		do {
			++m_masterIter;
			if ( m_masterIter == m_playerSlots.end() ) {
				m_masterIter = m_playerSlots.begin();
			}
			++checkedSlots;
		} while ( *m_masterIter == nullptr && checkedSlots < totalSlots );

		if ( *m_masterIter == nullptr ) {
			// 모든 슬롯이 nullptr인 경우 (발생해서는 안됨)
			return m_playerSlots.end();
		}

		return m_masterIter;
	}

	virtual bool isMaster( cClientSession* player ) {

		if ( m_masterIter == m_playerSlots.end() )
			return false;

		if ( *m_masterIter == nullptr )
			return false;

		return *m_masterIter == player;
	}

	virtual void IncreaseRoomPlayingCount() = 0;

	virtual void SendMasterChange() = 0;

	inline virtual int GetMasterSlot()
	{
		int masterSearchSlot = std::distance( m_playerSlots.begin() , m_masterIter ) + 1;
		return masterSearchSlot;
	}

	virtual uint64 GetMasterPlayerIdx() = 0;

#pragma endregion 방장 관리

#pragma region 딜러 수수료

	//inline virtual void SetDealerFee( const General::PlayCategory& gameType, const General::AssetKind& moneyType, const General::RoomAccessMode& roomType )
	//{
	//	// 로우바둑이
	//	if ( gameType == General::PlayCategory::PlayCategory_LowBadugi ) {
	//		if ( roomType == General::RoomAccessMode::RoomAccess_Public )  {
	//			if ( moneyType == General::AssetKind::AssetKind_Chip ) {
	//				m_dealerFeeRate = 0.05; return;
	//			}
	//			else if ( moneyType == General::AssetKind::AssetKind_Coin ) {
	//				m_dealerFeeRate = 0.09; return;
	//			}
	//		}
	//		else if ( roomType == General::RoomAccessMode::RoomAccess_FriendOnly ) {
	//			if ( moneyType == General::AssetKind::AssetKind_Chip ) {
	//				m_dealerFeeRate = 0.05; return;
	//			}
	//			else if ( moneyType == General::AssetKind::AssetKind_Coin ) {
	//				m_dealerFeeRate = 0.02; return;
	//			}
	//		}

	//		throw std::runtime_error( "IGame::SetDealerFee GameType_LowBadugi Cant Set dealerFeeRate" );
	//	}
	//	else if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
	//		if ( roomType == General::RoomAccessMode::RoomAccess_Public ) {
	//			if ( moneyType == General::AssetKind::AssetKind_Coin ) {
	//				m_dealerFeeRate = 0.09; return;
	//			}
	//		}

	//		throw std::runtime_error( "IGame::SetDealerFee GameType_Holdem Cant Set dealerFeeRate" );
	//	}
	//}
	// 채널 데이터에 설정된 데이터를 100으로 나누어 적용 하여야 한다.
	inline virtual void SetDealerFee( const double& dealerFeeRate ) { m_dealerFeeRate = dealerFeeRate / 100; }
	inline virtual double GetDealerFee() { return m_dealerFeeRate; }

#pragma endregion 딜러 수수료

#pragma region 맥스 베팅 

	// 0 이면 제한 없음
	virtual void SetMaxBetMoney( const uint64 maxBetMoney );
	inline virtual uint64 GetMaxBetMoney() { return m_maxBetMoney; }

	inline virtual void SetMinMoney( const uint64 minMoney ) { m_minMoney = minMoney; }
	inline virtual uint64 GetMinMoney() { return m_minMoney; }

	// 0 이면 소유 금액 제한 없음
	inline virtual void SetMaxMoney( const uint64 maxMoney )
	{
		if ( maxMoney != 0 )
			m_maxMoney = maxMoney;
		else
			m_maxMoney = UINT64_MAX;
	}
	inline virtual uint64 GetMaxMoney() { return m_maxMoney; }

#pragma endregion 맥스 베팅 

#pragma region 턴 처리

	inline virtual bool isTurnTimeOver()
	{
		return isMinumumTurnTimeOver() && ::GetTickCount64() >= m_turnExpireTick;
	}

	// 베팅이 완료 되었더라고, 최소 대기 시간을 기다리도록 만든다.
	// 현재 1 초
	inline virtual bool isMinumumTurnTimeOver()
	{
		return ::GetTickCount64() >= m_turnExpireTickMinimum;
	}

#pragma endregion 턴 처리

#pragma region 강제 퇴장

	inline virtual void RegisterKickPlayer(const uint64 playerIdx) 
	{
		auto iter = m_kickedPlayers.find( playerIdx );
		if ( iter == m_kickedPlayers.end() )
			m_kickedPlayers.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
	}

	inline virtual void CancelKickPlayer( const uint64 playerIdx )
	{
		auto iter = m_kickedPlayers.find( playerIdx );
		if ( iter != m_kickedPlayers.end() )
			m_kickedPlayers.erase( iter );
	}

	inline virtual bool isKickedPlayer( const uint64 playerIdx ) {
		auto iter = m_kickedPlayers.find( playerIdx );
		return iter != m_kickedPlayers.end();
	}

	inline void ClearKickedPlayer() { m_kickedPlayers.clear(); }

	inline bool isJoinProhibited( const uint64 playerIdx ) {
		auto iter = m_joinProhibited.find( playerIdx );
		return iter != m_joinProhibited.end();
	}

	inline void AddJoinProhibited( const uint64 playerIdx ) {
		auto iter = m_joinProhibited.find( playerIdx );
		if ( iter == m_joinProhibited.end() )
			m_joinProhibited.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
	}

	inline void CancelJoinProhibited( const uint64 playerIdx ) {
		auto iter = m_joinProhibited.find( playerIdx );
		if ( iter != m_joinProhibited.end() )
			m_joinProhibited.erase( iter );
	}

	virtual void ProcessKickOut() {}

#pragma endregion 강제 퇴장

#pragma region 베팅 로그

protected:	
	std::vector<std::string> m_betLogs;

public:	
	virtual void InsertBetLog( const std::string betLog ) {
		m_betLogs.push_back( betLog );
	}
	virtual void ClearBetLog() { m_betLogs.clear(); }
	virtual void WriteBetLog() = 0;

#pragma endregion

#pragma region 머니 관리

	virtual uint64 GetPlayerMoney( cClientSession* pClientSession );
	virtual uint64 GetHoldemMaxBet( cClientSession* ownerPlayer );
	virtual std::vector<cClientSession*> GetMaxBetPlayers(const uint64 curBetMoney);

#pragma endregion 머니 관리

#pragma region 손실한도 플레이어 관리

	virtual void KickMoneyHoldingLimitPlayers();

#pragma region 손실한도 플레이어 관리
};