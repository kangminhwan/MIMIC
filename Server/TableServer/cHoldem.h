#pragma once
#include "IGame.h"
#include "cBettingChecker.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"

#include <set>

class cHoldem : public IGame , public cBettingChecker
{
	const int HOLDEM_SLOT_COUNT = 9;

	enum E_BET_ROUND
	{
		NONE = 0 ,
		E_BET_ROUND_1 = 1 ,
		E_BET_ROUND_2 = 2 ,
		E_BET_ROUND_3 = 3 , 
		E_BET_ROUND_4 = 4
	};

	struct stResultServer
	{
		stResultServer()
		{
			player_result.Clear();
			point = 0;
			session = nullptr;
			money_before = money_after = accomulated_money = 0;
		}
		PmNet::MemberOutcome player_result;
		uint64 point;
		cClientSession* session;
		uint64 money_before;
		uint64 money_after;
		uint64 accomulated_money;
	};

	// 조합 결과를 저장하는 구조체
	struct Combination {
		std::vector<General::PlayingCard> cards;
	};

	enum E_DECIDE_WINNER
	{
		E_DECIDE_NONE = 0,
		E_DECIDE_WIN = 1,
		E_DECIDE_LOSE = 2,
		E_DECIDE_DRAW = 3
	};

private:
	static bool CompareResultServer( stResultServer& result1 , stResultServer& result2 );


protected:
	const uint64 LowBaduki_Bet_ChipMax = 1750000000000;
	int m_maxPlayerCnt;
	ATL::CAtlMap<General::TableAction , General::TableAction> m_allowedBetTypes;
	std::vector<cSidePot*> m_sidePots;
	std::vector<General::WinningHandHistory> m_win_jokbos;
	std::vector<General::PlayingCard> m_communityCards; // 커뮤니티 카드
	std::map<uint64 , uint64> m_betting;		// 유저인덱스 , 해당 턴 배팅내역
	std::map<uint64, int> m_raiseCount;

	// Rabbit Hunt / Show Hand (handled per hand, reset alongside m_communityCards)
	std::vector<General::PlayingCard> m_rabbitHuntCards;	// cached rabbit hunt cards for this hand, computed once
	bool m_rabbitHuntDone = false;
	std::set<uint64> m_shownHands;		// players who already voluntarily showed their hand this round

	int32 m_show_down_delay_ms_per_player;		// 홀덤 쇼다운 발생시에 플레이어 1명당 대기 시간 클라에서 설정 ( min 200 max 600 default 400)
	int32 m_show_down_community_delay_ms_per;	// 홀덤 쇼다운 커뮤니티 효과 시간 ( 값이 없는 경우 min 300 max 800 default 500 ), 최대 커뮤니티 카드를 3번 받음으로 3 * delay 시간
	int autostartCnt;
	int64 maxbet;
	int64 roundbet;

protected:
	friend class cGameRoom;

private:
	E_BET_ROUND m_curBetRound;					// 베팅을 돌아야할 바퀴수 [BET_1 = 2회], [BET_2, BET_3, Bet_4 = 3회]
	std::vector<cClientSession*> m_betSequence; // 베팅처리용

	std::vector<cClientSession*>::iterator m_curBetPlayerIter;
	std::vector<cClientSession*>::iterator m_dealerIter;	// 딜러
	uint64 m_lastPlayerBetTotal;
	uint64 m_stepPlayerBet;

	uint64 m_blindBet;							// 블라인드 마지막 베팅 금액 저장
	int32 m_firstidx;

	int p_id_int[ 9 ];
	std::string p_id[ 9 ];
	int64_t p_asset[ 9 ];
	int p_play_cnt[ 9 ];
	int64_t p_change_coin[ 9 ];
	int64_t p_discard_coin[ 9 ];
	int64_t p_fee_coin[ 9 ];
	std::string p_fee_info[ 9 ];
	int64_t p_rakeback[ 9 ];
	int64_t p_coin[ 9 ];
	std::string p_result[ 9 ];

	std::string win_id[ 9 ];
	int64_t win_asset[ 9 ];

	std::string skey;
	int play_time;
	std::string room_id;

	int timeStamp;
	int m_kick_out_count; // 새로 추가
	ULONG m_diedelay;

public:
	void InitGame() override;	// 게임 최초 생성시에 사용
	void InitPlayerData() {}	// 플레이어 데이터만 초기화 한다.
	void ResetGame() override;	// 게임 초기화 시에 사용
	void Shuffle() override;
	int32 Getfirstidx() override { return m_firstidx; }
	int GetTotalCardDeckCount() override { return 13 * 4; } // 최초 카드 덱 카운트
	General::ResultCode EnterSlot( cClientSession* pClientSession, const int slotNumber = 0) override;	// 진입 유저의 슬롯 결정
	bool LeaveSlot( cClientSession* pClientSession , bool b_out = false) override;
	bool SwapWithDummy( cClientSession* pClientSession , cClientSession* pDummyClientSession = nullptr) override;
	General::ResultCode MoveSlot(cClientSession* pClientSession, const int slotNumber) override;
	//void OnMoveSlot();
	bool HasSlot( cClientSession* pClientSession ) override;
	void MakeParticipate( int64 except_playerIdx = 0 ) override;
	void MakeParticipateOnplay( int64 except_playerIdx = 0 );
	cClientSession* GetPlayerSession( uint64 playerIdx ) override;
	cClientSession* GetPlayerALLSession( uint64 playerIdx ) override;
	uint64 GetBossPlayerIdx() override;
	void SetBossFirst() override;
	void SetMaster() override;
	uint64 GetMasterPlayerIdx() override;
	std::vector<cClientSession*> GetPlayersSessionList() override;
	google::protobuf::RepeatedField<General::WinningHandHistory> GetRecentlyPlayedGames() override;
	Server::PlayPhase GetGameStep() { return m_gameStep; }
	int GetCurBettingRound() override { return m_curBetRound; }
	bool HasBetting( cClientSession* pClientSession ) override;
	void SetGameStepEnd() { m_expireTick = ::GetTickCount64(); }
	void SetAutoStartCnt( int startcnt ) { autostartCnt = startcnt; }
	int GetAutoStartCnt() { return autostartCnt; }
	bool CheckSameCI( const std::string& ci ) override;

	void ProcessKickOut() override;
	void ProcessKickOutDieUser();

	bool IsParticipationPlayer( const uint64 playerIdx ) override;
	bool participatelock = false;

	int GetMaxPlayer() { return m_maxPlayerCnt; }
	PmNet::MatchOutcomeRS _result_res;


#pragma region 슬롯 예약

	// 슬롯 예약 관련 함수 모음 EnterSlot 에서도 처리함
	void OnSlotReservation() override;
	General::ResultCode SlotReservation( cClientSession* pClientSession , const int slotNumber = 0 ) override;
	bool HasSlotReservation( cClientSession* pClientSession ) override;
	bool RemoveReservation( cClientSession* pClientSession ) override;
	bool RemoveParticipationQueue( cClientSession* pClientSession ) override;

	
#pragma endregion 슬롯 예약

#pragma region 관전자 관리

	// 관전 신청 ( 플레이어 상태에 따라 즉시 관전, 관전 예약 판별 )
	General::ResultCode TransferWatcher( cClientSession* pClientSession ) override;

	// 플레이중인 유저중 관전 예약한 유저를 관전자로 변경한다.
	void OnWatcherReservation();

	// 플레이중인 유저가 관전을 예약한다.
	General::ResultCode WatcherReservation( cClientSession* pClientSession );

	// 관전 예약을 취소한다.
	General::ResultCode CancelWatcherReservation( cClientSession* pClientSession );

#pragma endregion 관전자 관리

	void Waiting() override;
	General::ResultCode StartGame( cClientSession* pClientSession ) override;
	void Playing() override;
	void Process() override;						// 게임방 스텝처리는 이 함수에서만 하도록!!
	void CheckParticipateQueue();
	bool isTimeOver() override;
	void SetNextPlay() override;					// 다음 스텝으로 진행 시킵니다.

	// 방장관리
	void IncreaseRoomPlayingCount() override;
	void SendMasterChange() override;
	void SendMasterChange( const int64 playerindex , bool b_out = true);

	// Vote System
public:
	/*virtual General::ResultCode StartVote( const General::BallotKind voteType, int32 playersCount , const uint64 issuerPlayerIdx , const uint64 voteRequesterPlayerIdx ) override { return General::ResultCode::Result_Success; }
	virtual void OnVoteAction( cVote& voteAction ) override { return; }
	virtual void OnVoteResult( cVote& voteAction, const bool& isSuccess ) override {}*/

public:
	// Room Management
	void MakeRoomStatusWait();
	void MakeRoomStatusStartWait();
	void MakeRoomAutoStart( const int& memberCount );
	bool IsAutoStartable( const int& memberCount );
	void MakeRoomStatusResult();
	void DecideMaster();
	void DecideDealer( bool forceChange = false );
	void DecideDealer( int64 outuserindx , bool forceChange = false );
	void SendBossChange( const int newBossSlot );
	void SendBossChange( const int newBossSlot ,const int64 outuserindx );
	void SendStatusChange( const Server::PlayPhase beforeStatus , std::string callFunc , int64  except_playerIdx = 0 );
	int GetPlayingPlayerCount();
	int GetActivePlayerCount();
	std::vector<cClientSession*> GetActivePlayers();
	std::vector<cClientSession*> GetActivePlayers(const uint64 exceptPlayerIdx);
	//std::vector<cClientSession*> GetMaxBetPlayers(const uint64 curBetMoney);
	int GetPossibleToHalfBetPlayerCount();
	int GetMinStartPlayerCount() override;	// 방 시작하기 위한 최소 인원
	void GetCurrentStatus( PmNet::MatchStateSwapRS& _res ) override;
	void EndGame() override;
	void OnIntruding( cClientSession* pClientSession , bool ignoreCommunity = false , bool relogin = false) override;	// 난입시에 받을 패킷 처리 처리
	//void OnIntrudingReLogin( cClientSession* pClientSession , bool ignoreCommunity = false );   // 리로그인시 패킷

	// Bet Management
	static void ClearBet( cClientSession* pClientSession , Server::PlayPhase step );
	std::vector<cClientSession*>::iterator DecideHoldemFirstBetPlayer();
	void DecideHoldemBet1();
	void DecideFirstBet();
	void DecideBetSequence();
	//General::ResultCode UserBet( cClientSession* pClientSession , General::TableAction betting , General::HandRank baccaraJokbo = General::HandRank::HandRank_None ) override;
	General::ResultCode UserBet( cClientSession* pClientSession , const PmNet::MatchWagerRQ& userBet ) override;
	void CheckSide();
	//void SendSide() override;
	void BetProcess();
	bool NextBet();
	bool CheckBet();
	bool HasBossAnyBetting();
	bool NoOneCanBet( std::vector<cClientSession*>& activePlayers );
	bool AllPlayerBetMax( std::vector<cClientSession*>& activePlayers );
	void DieCurPlayer();
	bool isBoss( const uint64 playerIdx );
	std::vector<General::TableAction>* GetPlayerBettings( cClientSession* pClientSession );
	std::vector<General::TableAction>* CreateStepPlayerBettings( cClientSession* pClientSession , Server::PlayPhase gameStep );
	General::ResultCode MoreBetCheck( std::vector<General::TableAction>* bettings );
	bool TryingCallTwice( std::vector<General::TableAction>* bettings , General::TableAction curBet );
	General::ResultCode CaclBet( cClientSession* pClientSession , General::TableAction curBetting , BOOL& allin , BOOL& side );
	bool CheckAllPlayingPlayerSameBetWithBetOnce();
	bool CheckAllPlayingPlayerSameBet();
	bool IsSameVirtualBet( std::vector<cClientSession*>& activeUsers );
	std::vector<cClientSession*>::iterator GetFirstBetPlayer();
	std::vector<cClientSession*>::iterator GetLastBetPlayer();
	std::vector<cClientSession*>::iterator GetNextBetPlayer();
	bool GoToNextRound() override;	// 다음 라운드로 진행한다.
	bool GoToDirectResult();		// 결과 처리로 직행한다.
	bool NoNextRoundWhenOneUserLeftForBet();
	uint64 GetMaxBet( uint64& curBet );
	//uint64 GetHoldemMaxBet( cClientSession* ownerPlayer );
	static bool CompareMaxBetChip( cClientSession* player1 , cClientSession* player2 );
	static bool CompareMaxBetCoin( cClientSession* player1 , cClientSession* player2 );

	// BettingChecker 필요 함수 재정의
	virtual void CreateStockers() override;

	// Card Management
	int CalculatePreFlopAnimation();
	void HandCardDistribute( const uint64& seedMoney, PmNet::MatchWagerRS& sb, PmNet::MatchWagerRS& bb, PmNet::MatchWagerRS& gb, PmNet::MatchWagerRS& rb );			// 핸드 카드 분배, PreFlop
	void ReChargeDeck();
	bool NextCardChange();
	bool CheckCardChange();
	void PassCurPlayer();
	bool UserRemoveCard( cClientSession* pClientSession , const General::PlayingCard& remove_card );
	int GetCommunityCard();

	// Money Management
	bool CheckSeed( const uint64 seedMoney );
	bool CheckDieUser();
	uint64 CheckMaxMoney();
	bool MinusPlayerSeed( const uint64& seedMoney );
	void SetPlayerMoney( cClientSession* pClientSession , uint64 moneyAmount );
	bool CheckPlayerMoney( cClientSession* pClientSession , uint64 moneyAmount );
	bool MinusPlayerMoney( cClientSession* pClientSession , uint64 moneyAmount );
	bool MinusPlayersMoney( uint64 moneyAmount , bool checkOnly );
	void ClearStockedMoney();
	uint64 MinusSbBbGbRb( PmNet::MatchWagerRS& response, const uint64 playerIdx, const uint64 betMoney, const General::TableAction bet );

	// Turn Management
	bool AutoCall();
	void SetTurnExpireAndSendPlayerTurn( uint64 waitMs );
	int GetMaxRaiseCountForCurrentRound();
	bool CanPlayerRaise(cClientSession* player);
	std::vector<General::TableAction> GetAvailableActionsForPlayer(cClientSession* player);
	void OnIntrudingSendTurnExpireAndSendPlayerTurn();

	// Result
	void CalcResult();
	void CalcResultNoCommunityCard();
	static std::vector<General::PlayingCard> KickerCards( std::vector<General::PlayingCard>& cards , const google::protobuf::RepeatedPtrField<General::PlayingCard>& jokboCards );
	static std::vector<General::PlayingCard> SortJokboCards( const google::protobuf::RepeatedPtrField<General::PlayingCard>& jokboCards , General::HandRank jokbo );
	void SaveUserData(bool bResult = false);

	std::string game_record_string;

	// ShowDown
	int CalculateShowDownAnimation( int& leftCommunityCards );
	bool isShowDownTime();
	void DoShowDown( int& leftCommunityCards );

	// Rabbit Hunt / Show Hand
	General::ResultCode RabbitHunt( cClientSession* pClientSession , std::vector<General::PlayingCard>& outCards );
	General::ResultCode VoluntaryShowHand( cClientSession* pClientSession , const General::HandRevealScope& scope , std::vector<General::PlayingCard>& outCards );

	static General::HandRank GetJokbo( google::protobuf::RepeatedField<General::PlayingCard> cards , std::vector<General::PlayingCard>& duplicatedCards );
	static int SortAsc( General::PlayingCard& card1 , General::PlayingCard& card2 );
	static int SortDesc( General::PlayingCard& card1 , General::PlayingCard& card2 );
	static bool CompareCardByNumOnlyAsc( General::PlayingCard& card1 , General::PlayingCard& card2 );
	static bool CompareCardBySecondCardNumOnlyAsc( std::vector<General::PlayingCard>& cards1 , std::vector<General::PlayingCard>& cards2 );
	static bool CompareCardByNumAsc( General::PlayingCard& card1 , General::PlayingCard& card2 );
	static bool CompareCardByTypeAsc( const General::PlayingCard& card1 , const General::PlayingCard& card2 );
	static bool ComparePlayerResult( PmNet::MemberOutcome& player1 , PmNet::MemberOutcome& player2 );
	static std::vector<General::PlayingCard> CompareGroupCombinationAndGetMinimum( std::map<General::CardSuit , std::vector<General::PlayingCard>> cardTypeMap );
	static int SumTypeCount( google::protobuf::RepeatedField<General::PlayingCard>& cards );
	static int SameNumCount( google::protobuf::RepeatedField<General::PlayingCard>& cards );
	static std::vector<General::PlayingCard> RemoveDuplicatedCard( google::protobuf::RepeatedField<General::PlayingCard>& cards );
	static std::vector<General::PlayingCard> RemoveDuplicatedCard( const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards );
	static bool ComparePlayerResultByJokbo( PmNet::MemberOutcome& player1 , PmNet::MemberOutcome& player2 );
	static bool ComparePlayerResultDetail( PmNet::MemberOutcome& player1 , PmNet::MemberOutcome& player2 );
	static bool CompareCardPerSequence( const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards1 , const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards2 , int& cardSequence );

	static std::string GetTypeString( General::CardSuit cardType );
	static std::string GetNumberString( General::CardRank cardNumType );
	static std::string CardsToString( const google::protobuf::RepeatedPtrField<General::PlayingCard>& cards );
	static std::string CardsToString( const std::vector<General::PlayingCard>& cards );

	static std::string CardTypeToString( General::CardSuit type );
	static std::string CardNumTypeToString( General::CardRank num );

	// 맥스 베팅
	// 0 이면 제한 없음
	//void SetMaxBetMoney( const uint64 maxBetMoney ) override;

	// Throw Control
public:
	static std::string EnumToString( int enumValue , const google::protobuf::EnumDescriptor* descriptor );
	static void ThrowGameStepException( std::string caller , Server::PlayPhase gameStep );

	// Debug 용도
public:
	static void DebugCardCheck( google::protobuf::RepeatedField<General::PlayingCard>& cards );
	static void DebugCardCheckVec( std::vector<General::PlayingCard>& cards );
	static void DebugGetRoyalStraightFlush( google::protobuf::RepeatedField<General::PlayingCard>& cards );
	static void DebugMakeCommunityCardRoyalStraightFlush( std::vector<General::PlayingCard>& cards );
	static void DebugMakeKickerSituation( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players );
	static void DebugMakeTwoPairSituation( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players );
	static void DebugMakeTwoPairSameKickerSituation( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players );
	static void DebugTest( std::vector<General::PlayingCard>& cards , std::vector<cClientSession*>& players );

	// Cheat 용도
private: bool m_bCheatCardChange;
private: PmNet::DebugHandSwapRQ m_cheatCardChange;
public: void CheatCardChange( PmNet::DebugHandSwapRQ& request );

	// 베팅 로그
public: virtual void WriteBetLog() override;

#pragma region QA DATA 및 함수 정의

public:
	std::vector<General::PlayingCard> m_qa_community_cards;				// QA 커뮤니티 카드
	std::vector<General::PlayingCard> m_qa_community_deck;				// QA 커뮤니티 덱, 시작할때마다 m_qa_community_cards 를 복사

#pragma endregion QA DATA 및 함수 정의

public:
	cHoldem( cGameRoom* pGameRoom , 
		const General::BetPolicy& bettingRuleType,
		const int maxPlayerCnt , 
		const Server::Channel& channel ,  
		const int32& show_down_delay_ms_per_player , 
		const int32& show_down_community_delay_ms_per )
	{
		if ( pGameRoom == nullptr )
			throw std::exception( "cGameRoom is nullptr" );
		IGame::m_pGameRoom = pGameRoom;
		IGame::m_gameType = channel.game_type();
		IGame::m_channel = channel;
		m_bettingRuleType = bettingRuleType;
		m_maxPlayerCnt = maxPlayerCnt;

		// 홀덤 쇼다운 발생시에 플레이어 1명당 대기 시간 클라에서 설정 ( min 200 max 600 default 400)
		if ( show_down_delay_ms_per_player >= 200 && show_down_delay_ms_per_player <= 600 )
			m_show_down_delay_ms_per_player = show_down_delay_ms_per_player;
		else
			m_show_down_delay_ms_per_player = 400;

		// 홀덤 쇼다운 커뮤니티 효과 시간 ( 값이 없는 경우 min 300 max 800 default 1500 ), 최대 커뮤니티 카드를 3번 받음으로 3 * delay 시간
		if ( show_down_delay_ms_per_player >= 300 && show_down_delay_ms_per_player <= 1500 )
			m_show_down_community_delay_ms_per = show_down_community_delay_ms_per;
		else
			m_show_down_community_delay_ms_per = 500;

		InitGame();
		InitMaster();

		IGame::SetMaxBetMoney( channel.limit_bet_money() );
		SetMinMoney( channel.money_min() );
		SetMaxMoney( channel.money_max() );
	}

private:
	cHoldem() {}
};