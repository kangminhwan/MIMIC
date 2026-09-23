#pragma once
#include "TableServerHeader.h"
//#include "cPinballCache.h"
#include "TimeUtils.h"
#include "cLoungeEvent.h"

#include "../Include/Netlib/Session/cSession.h"
#include "../Include/Netlib/Common/cSingleton.h"
//#include "cVirtualSession.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"
#include "../../ProtocolBuffer/cpp/Server.pb.h"

#include <atlcoll.h>
#include <thread>
#include <future>
#include <string>

class cVirtualSession;
class cClientSession : public NetLib::cSession //, public cPinballCache
{
	friend class IGame;

	friend class cHoldem;


	friend class cGameRoom;

protected:
	General::ParticipantProfile m_player;
	Server::ParticipantProfileInternal m_playerExt;
	std::map<int, General::AvatarProfile> m_avatars;
	PmNet::Ledger m_records;				// 플레이어 전적 데이터
	General::LossLimitProfile* m_pLostLimit;		// 플레이어 손실 한도 참조 ( 계정에 대해 여러개가 공유 할 수 있다. )
	uint64 m_account_idx;					// 계정 CID 값에 물려 있는 인덱스
	std::string m_account_guid;
	std::string m_platform_guid;
	General::StoreChannel m_market;				// 로그인할때마다 마켓 정보를 갱신한다
	std::string m_game_version;				// 클라이언트 게임 버젼
	General::InPlayRecord m_room_record;		// 현재 플레이중인 방에서의 전적을 누적한다.
	bool is_copyed = false;
	std::string m_deviceInfo;				// 디바이스 정보
	std::string m_osInfo;					// os 정보
	std::string m_joinTime;					// 가입시간
	std::string m_ip;						// IP

	ULONG m_delayTick;
private:
	UINT m_roomNumber;
	UINT m_roomNumberBefore;	// 그전에 들어 갔던방번호
	google::protobuf::RepeatedField<General::PlayingCard> m_cards;
	bool m_reserveRoomOut;		// 방나가기 예약
	int m_roomPlayingCount;		// 한방에서 연속 플레이한 횟수
	int m_noActionCounter;		// 바카라 전용? 방에서 액션을 하지 않았을 경우 2회 이상이면 튕겨 낸다.
	bool m_noActionFlag;		// 바카라 전용? Pass 등의 액션을 하면 false 로 바꿔준다.
	int m_beforeSlotIndex;		// 홀덤 바둑이 블랙잭 바카라 등등 나가기전에 몇번자리에 앉아있는지 알기 위한 변수
	std::string m_roomOutReason; // 방 나간 이유 스트링
	int roomjoinCount;			// 방진입 횟수

#pragma region 로우바둑이 & 홀덤 인게임 처리

	// 베팅
	int m_bettingSlot;
	bool m_die;
	bool m_allIn;
	bool m_maxbet;
	bool m_check;
	bool m_side;
	bool m_sendSide;
	uint64 m_stockLostMoney;
	uint64 m_freeDealerFeeMoney; // 남은 딜러비를 물지 않는 금액

	uint64 m_virtualLostMoney;	// 로우바둑이용 가상의 베팅 금액 생성
	bool isplaying;
	bool b_updateLost;

	inline void InitMoveSlotCount() { m_playerExt.set_remaining_reel_moves( 3 ); }
	inline void MinusMoveSlotCount() {
		int remains = m_playerExt.remaining_reel_moves();
		m_playerExt.set_remaining_reel_moves( --remains );
	}
	inline int GetMoveSlotCount() { return m_playerExt.remaining_reel_moves(); }
	inline bool IsRemainMoveSlotCount() { return m_playerExt.remaining_reel_moves() != 0 ? true : false; }

	inline void SetPlayerBet( const General::TableAction bet , const uint64 bet_money ) { m_player.set_current_table_action( bet ); m_player.set_current_wager_amount( bet_money ); }
	inline void ClearPlayerBet() { m_player.clear_current_table_action(); m_player.clear_current_wager_amount(); }
public:
	ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*> m_bettings;
	inline bool Getisplaying() { return isplaying; }
	inline void Setisplaying( bool b_playing ) { isplaying = b_playing; }
#pragma endregion 로우바둑이 & 홀덤 인게임 처리

#pragma region 로우바둑이 난입 유저 처리용
private:
	std::map<Server::PlayPhase , int> m_card_change_counts; // 시작시에 아침, 점심, 저녁 -1 로 초기화한다.

public:
	inline std::map<Server::PlayPhase , int> GetCardChangeHistory() const { return m_card_change_counts; }

	inline void InitCardChangeHistory() {
		m_card_change_counts.clear();
		m_card_change_counts.insert( std::pair<Server::PlayPhase , int>( Server::PlayPhase::PlayPhase_LowMorning , -1 ));
		m_card_change_counts.insert( std::pair<Server::PlayPhase , int>( Server::PlayPhase::PlayPhase_LowNoon , -1 ) );
		m_card_change_counts.insert( std::pair<Server::PlayPhase , int>( Server::PlayPhase::PlayPhase_LowEvening , -1 ) );
	}
	inline void InitUpdateLost() { b_updateLost = false; }
	inline void SetUpdateLost() { b_updateLost = true; }
	inline bool GetUpdateLost() { return b_updateLost; }

	inline void UpdateCardChangeHistory( const Server::PlayPhase& gameStep , const int change_card_count ) {
		const auto& iter = m_card_change_counts.find( gameStep );
		if ( iter != m_card_change_counts.end() )
			iter->second = change_card_count;
	}

#pragma endregion 로우바둑이 난입 유저 처리용

#pragma region 바카라 인게임 처리

private:
	bool m_passBet;

public:
	inline void InitPassBet() { m_passBet = false; }
	inline bool GetPassBet() { return m_passBet; }
	inline void SetPassBet() { m_passBet = true; SetNoActionFlag(false); }

#pragma endregion 바카라 인게임 처리

#pragma region 블랙잭 인게임 처리

private:
	std::vector<cClientSession*> virtualClients;
	uint64 m_virtualClientIdx;

public:
	cVirtualSession* CreateVirtualSession();
	virtual uint64 GetOwnerPlayerIdx() { return 0; }
	virtual bool IsOwnerConnected() { return GetContext() != nullptr; }

#pragma endregion 블랙잭 인게임 처리

public:
	static cClientSession* CreateSession();

public:
	virtual void Init() override;
	virtual void Clear() override;
	void SoftClear();
	void GameReset();
	void RoomOutReset();
	virtual void DisConnectContext(UINT Entity, UINT nThreadIndex) override;
	virtual size_t CodedSessionInfo(char* pCodedBuffer, size_t nBufferSize) override;
	virtual bool isRemoveReady() override;

public:
	General::ParticipantProfile GetPlayer() { return m_player; }
	General::ParticipantProfile& GetPlayerRef() { return m_player; }
	Server::ParticipantProfileInternal GetPlayerExt() { return m_playerExt; }
	Server::ParticipantProfileInternal& GetPlayerExtRef() { return m_playerExt; }
	void SetGuids( const std::string& account_guid , const std::string& platform_guid ) {
		m_account_guid = account_guid;
		m_platform_guid = platform_guid;
	}
	bool SetPlayer(const uint64 accountIdx, const int64 playerIdx, const int db_idx, const std::string& sessionid);
	void SetPlayer( const uint64 accountIdx , General::ParticipantProfile& player , Server::ParticipantProfileInternal& playerExt );
	void SetAvatars( std::map<int , General::AvatarProfile>& avatars );
	void SetRecords( PmNet::Ledger records );
	inline void SetDeviceInfo( std::string deviceInfo ) { m_deviceInfo = deviceInfo; }
	inline std::string GetDeviceInfo() { return m_deviceInfo; }
	inline void SetOSInfo( std::string osInfo) { m_osInfo = osInfo; }
	inline std::string GetOSInfo() { return m_osInfo; }
	inline void SetJoinTime( std::string JoinTime ) { m_joinTime = JoinTime; }
	inline std::string GetJoinTime() { return m_joinTime; }
	inline void SetMarket( const General::StoreChannel _market ) { m_market = _market; }
	inline General::StoreChannel GetMarket() { return m_market; }
	inline void SetGameVersion( const std::string& game_version ) { m_game_version = game_version; }
	inline std::string& GetGameVersion() { return m_game_version; }
	inline std::string GetAccountGuid() const { return m_account_guid; }
	inline std::string GetPlatformGuid() const { return m_platform_guid; }
	inline uint64 GetAccountIdx() { return m_account_idx; }
	inline uint64 GetPlayerIdx() { return m_player.member_id(); }
	inline std::string GetNickName() const { return m_player.display_name(); }
	inline void SetNickName( const std::string& nickname ) { return m_player.set_display_name( nickname ); }
	inline const UINT GetJoinedRoomNumber() { return m_roomNumber; }
	inline void SetJoinedRoomNumber( const int roomNumber ) {
		m_roomNumber = roomNumber;
	}
	inline void SetBeforeSlotIndex( int beforeSlotIndex ) { m_beforeSlotIndex = beforeSlotIndex; }
	inline int GetBeforeSlotIndex() { return m_beforeSlotIndex; }
	inline void SetRoomOutReason( std::string roomOutReason ) { m_roomOutReason = roomOutReason; }
	inline std::string& GetRoomOutReason() { return m_roomOutReason; }
	inline const UINT GetJoinedRoomNumberbefore() { return m_roomNumberBefore; }
	inline void SetJoinedRoomNumberBefore( const int roomNumber ) { m_roomNumberBefore = roomNumber; }
	inline uint64 GetRemainSlotCoin() { return m_playerExt.remaining_reel_coin(); }
	inline void SetRemainSlotCoin( const uint64 remain_slot_coin ) { m_playerExt.set_remaining_reel_coin( remain_slot_coin ); }
	virtual inline uint64 GetChip() { return m_player.wallet_chips(); }
	//virtual inline void SetChip( uint64 chip ) { m_player.set_wallet_chips(chip); }
	virtual inline uint64 GetLostMoney() { return m_stockLostMoney; }
	virtual inline uint64 GetVirtualLostMoney() { return m_virtualLostMoney; }
	virtual inline void PlusVirtualLostMoney( const uint64 money ) { m_virtualLostMoney += money; }
	virtual inline void InitLostMoney() { m_stockLostMoney = 0; }
	virtual inline void InitVirtualLostMoney() { m_virtualLostMoney = 0; }
	virtual inline uint64 GetCoin() { return m_player.wallet_coins(); }
	//virtual inline void SetCoin( uint64 coin ) { m_player.set_wallet_coins( coin ); }
	virtual inline uint64 GetGem() { return m_player.wallet_gems(); }
	virtual inline void SetGem( const uint64& gem ) { m_player.set_wallet_gems( gem ); }
	inline void InitFreeDealerFeeMoney() { m_freeDealerFeeMoney = 0; }
	inline uint64 GetFreeDealerFeeMoney() { return m_freeDealerFeeMoney; }
	inline void SetFreeDealerFeeMoney( const uint64 freeDealerFeeMoney ) { m_freeDealerFeeMoney = freeDealerFeeMoney; }
	inline void MinusFreeDealerFeeMoney( const uint64 money ) { m_freeDealerFeeMoney -= money; }
	inline ATL::CAtlMap<Server::PlayPhase , std::vector<General::TableAction>*>* GetBettings() { return &m_bettings; }
	void GetStepBettingExceptBindBet( const Server::PlayPhase& step, std::vector<General::TableAction>& returnBetting );
	inline void SetBettingSlot( const int bettingSlot ) { m_bettingSlot = bettingSlot; }
	inline int GetBettingSlot() { return m_bettingSlot; }
	inline void SetDie(bool die) { m_die = die; m_player.set_folded_out( die ); }
	inline bool isDie() { return m_die; }
	inline void SetAllIn(bool allIn = true) { m_allIn = allIn; m_player.set_all_in_state( allIn ); }
	inline bool isAllIn() { return m_allIn; }
	inline void SetMaxBet(bool allIn = true) { m_maxbet = allIn; }
	inline bool isMaxBet() { return m_maxbet; }
	inline void SetCheck(bool check = false) { m_check = check;}
	inline bool isCheck() { return m_check; }
	inline void SetSide(bool side = true) { m_side = side; m_player.set_side_bet_enabled( side ); }
	inline bool isSide() { return m_side; }
	inline void SetSendSide( bool sideSide = true ) { m_sendSide = sideSide; }
	inline bool isSendSide() { return m_sendSide; }
	inline ULONG GetDelayTick() { return m_delayTick; }
	inline void SetDelayTick( ULONG tick ) { m_delayTick = tick; }
	inline google::protobuf::RepeatedField<General::PlayingCard>* GetCards() { return &m_cards; }
	inline google::protobuf::RepeatedField<General::PlayingCard> GetCardsClone() { return m_cards; }

	inline bool ReverseRoomOutReserve() {
		m_reserveRoomOut = !m_reserveRoomOut;  return m_reserveRoomOut;
	}
	inline void SetRoomOutReserve(bool reserveRoomOut = true) { m_reserveRoomOut = reserveRoomOut; }
	inline void CalcelRoomOutReserve() { m_reserveRoomOut = false; }
	inline bool GetReserved() { return m_reserveRoomOut; }

	inline void IncreaseRoomPlayingCount() { ++m_roomPlayingCount; }
	inline int GetRoomPlayingCount() { return m_roomPlayingCount; }

	inline void InitNoActionCounter() { m_noActionCounter = 0; }
	inline void IncreaseNoActionCounter() { ++m_noActionCounter; }
	inline int GetNoActionCounter() { return m_noActionCounter; }
	inline void SetNoActionFlag( bool noActionFlag ) { m_noActionFlag = noActionFlag; }
	inline bool GetNoActionFlag() { return m_noActionFlag; }

	//inline void SetSubPasswd( const std::string& passwd ) { m_player.set_sub_passwd_activated( true ); m_player.set_sub_passwd( passwd ); }
	//inline std::string GetSubPasswd() { return m_player.sub_passwd(); }

	inline int GetBlackjackStraightWins() { return m_playerExt.blackjack_win_streak(); }
	inline void IncreateBlackjackStraightWins() { m_playerExt.set_blackjack_win_streak( GetBlackjackStraightWins() + 1); }

	inline std::string GetDailyRefreshTime() { return m_player.daily_reset_at(); }
	inline void SetDailyRefreshTime( const std::string& dailyRefreshTime ) { m_player.set_daily_reset_at( dailyRefreshTime ); }

	inline std::string GetMonthlyRefreshTime() { return m_playerExt.monthly_reset_at(); }
	inline void SetMonthlyRefreshTime( const std::string& monthlyRefreshTime ) { m_playerExt.set_monthly_reset_at( monthlyRefreshTime ); }

	inline std::string GetNicknameChangeProhibiteExpiryTime() { return m_player.nickname_change_locked_until(); }
	inline void SetNicknameChangeProhibiteExpiryTime( const std::string& nicknameChangeProhibiteExpiryTim ) {
		m_player.set_nickname_changed_once( true );
		m_player.set_nickname_change_locked_until( nicknameChangeProhibiteExpiryTim ); }

	void CopyPlayer(General::ParticipantProfile* pPlayer);
	std::future<BOOL> SavePlayer();
	std::future<BOOL> SaveMoney();
	std::future<BOOL> AvatarsGet();
	std::future<BOOL> PlayerSetAvatar( const int& avatar_id );

	BOOL hasAvatar( const int& avatar_id );
	General::AvatarProfile GetAvatar( const int& avatar_id );
	General::AvatarProfile SetAvatar( const int& avatar_id );
	void CopyAvatars( PmNet::MarketPurchaseRS& response );

	BOOL UpdateLobby( BOOL& updateMailBox );
	void UpdateRefillMoney( PmNet::RefreshAtriumRS& response );
	BOOL UpdateDailyExpireDateTime();


	// 유료 재화
	//virtual inline uint64 GetPaidChip() { return m_player.paid_chips(); }
	//virtual inline void SetPaidChip( const uint64& paid_chip ) { m_player.set_paid_chips( paid_chip ); }
	//virtual inline uint64 GetPaidCoin() { return m_player.paid_coin(); }
	//virtual inline void SetPaidCoin( const uint64& paid_coin ) { m_player.set_paid_coin( paid_coin ); }
	virtual inline uint64 GetPaidGem() { return m_player.paid_gems(); }
	virtual inline void SetPaidGem( const uint64& paid_gem ) { m_player.set_paid_gems( paid_gem ); }

	inline uint32 GetKickOutTicketCount() { return m_player.kick_ticket_balance(); }
	inline void SetKickOutTicketCount( const uint32& ticket_count ) { m_player.set_kick_ticket_balance( ticket_count ); }
	inline uint32 AddKickOutTicket( const uint32 ticket_count ) {
		uint32 new_ticket = m_player.kick_ticket_balance() + ticket_count;
		m_player.set_kick_ticket_balance( new_ticket );
		return m_player.kick_ticket_balance();
	}
	inline BOOL UseKickOutTicketCount( const uint32& ticket_count = 1 ) {
		if ( ticket_count > m_player.kick_ticket_balance() ) return FALSE;
		uint32 new_ticket = m_player.kick_ticket_balance() - ticket_count;
		m_player.set_kick_ticket_balance( new_ticket );
		return TRUE;
	}

#pragma region 전적

	// 전적 읽어오기
	std::future<BOOL> RecordsGet( const uint64& playerIdx , int& rows , PmNet::Ledger& records );
	inline void CopyRecords( PmNet::Ledger* records ) { records->CopyFrom( m_records ); }
	void UpdateRecordDailyRefresh();

	// 갱신

	void UpdateHoldemToday( const bool& isWin , const General::AssetKind moneyType , const int64 getMoney );
	void UpdateHoldemTotal( const bool& isWin , const General::AssetKind moneyType , const int64 getMoney );
	//void UpdateBlackjackStraightWins( const int& straightWins );

	// DB 처리
	std::future<BOOL> UpdateHoldemRecords( bool bDailyInit = false );
	//std::future<BOOL> UpdatePinballRecords( bool bDailyInit = false );
	//std::future<BOOL> UpdateRouletteRecords( bool bDailyInit = false );

	void UpdateJokboRecords( const General::PlayCategory gameType , const bool& isWin , const General::HandRank jokbo , const int blackjackCount = 0);
	std::future<BOOL> UpdateJokboRecord();

	std::vector<General::PlayCategory> m_updateGameTypes;
	std::vector<General::HandRank> m_updateJokbos;

#pragma endregion 전적

#pragma region 레벨

	void UpdateLevel( const uint64& getExp );

#pragma endregion 레벨

#pragma region 보관함

protected:
	std::map<uint64 , PmNet::InboxDetail> m_mailBox;

public:
	void SetMailBoxOnLogin( std::vector<PmNet::InboxDetail>& mail_list );
	General::ResultCode RequestMailBox( const int& pageSize, const int& page , PmNet::InboxIndexRS& _response );
	General::ResultCode OpenMail( google::protobuf::RepeatedField<uint64> mail_idx_list , PmNet::InboxReadRS& _response );

#pragma endregion 보관함

#pragma region 미션, 업적

	std::map<uint32 , General::TaskProgress> m_daily_missions;
	std::map<uint32 , General::TaskProgress> m_lounge_missions;
	std::map<uint32 , General::TaskProgress> m_achievements;

	void SetMissionAndAchieve( std::map<uint32 , General::TaskProgress>& daily_missions, std::map<uint32 , General::TaskProgress>& lounge_missions, std::map<uint32 , General::TaskProgress>& achievements );
	void LoadQuestsOnLogin(); // 로그인시 퀘스트 로딩 및 누락 미션 생성
private:
	void CreateMissingMissions(const std::map<int32, General::TaskProgress>& masterMissions,
		std::map<uint32, General::TaskProgress>& playerMissions, const General::TaskCategory& missionType); // 누락 미션 생성
public:
	void RequestMissionAndAchieve( const General::TaskCategory& achieve_type , PmNet::FetchGoalsRS& _response );
	void RequestMissionAndAchieve( const General::TaskCategory& achieve_type , google::protobuf::RepeatedPtrField<General::TaskProgress>* quests );

	BOOL GetQuestReward( const General::TaskCategory& achieve_type , const int& quest_id , uint64& get_coin , uint64& get_chip );

	std::vector<uint32> UpdateQuests( const General::TaskTrigger& achieve_event_type , const uint64& increaseCount = 1 );
	std::vector<uint32> UpdateExceptLoungeQuests( const General::TaskTrigger& achieve_event_type , const uint64& increaseCount = 1 );
	void UpdateExceptLoungeQuests( const General::TaskTrigger& achieve_event_type , std::map<uint64 , int64>& all_in_map );
	std::vector<uint32> UpdateLoungeQuests( const General::TaskTrigger& achieve_event_type , const uint64& increaseCount = 1 );
	BOOL UpdateQuest( General::TaskProgress& _quest , const uint64& increaseCount );

	void UpdateQuest( General::TaskTrigger achieveEventType , std::map<uint64 , int64>& all_in_map );

	void UpdateQuestsDailyRefresh();

	//static bool IsOver7Made( General::HandRank& jokbo );

#pragma endregion 미션, 업적

#pragma region 무료 충전소

	BOOL GetFreeCharge( const General::AssetKind& _monty_type , uint64& _get_money , uint64& _charged );

	inline std::string CoinFreeChargeTime() { return m_player.coin_free_charge_reset_at(); }
	inline std::string ChipFreeChargeTime() { return m_player.chip_free_charge_reset_at(); }

#pragma endregion 무료 충전소

#pragma region 맴버쉽 & 적립 통장

	inline General::BenefitTier GetMemberShipClass()
	{
		// 멤버쉽의 기간이 만료 되었으면 노멀로 내려준다.
		if ( MemberShipExpired() )
			return General::BenefitTier::BenefitTier_Basic;
		else
			return m_player.membership_tier();
	}
	inline void SetMemberShipClass( const General::BenefitTier& member_ship_class ) { m_player.set_membership_tier( member_ship_class ); }
	inline bool MemberShipExpired() {
		std::string expiry_string = m_player.membership_expires_at();
		std::time_t expiry_time = TimeUtils::StringToTimeTM( expiry_string );
		std::time_t now = std::time( nullptr );
		return now >= expiry_time;
	}
	inline uint64 AddRakeBack( const General::PlayCategory& gameType , const uint64& money ) {

		uint64 realMoney = money;

		

		// 라운지 이벤트 기간 동안 증가
		uint64 rake_back_rate = NetLib::cSingleton<cLoungeEvent>::GetInstance()->GetRakeBackRate();

		// 멤버쉽 등급에 따라 적립률이 달라진다.
		uint64 rakeback_money = 0;
		switch ( GetMemberShipClass() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			rakeback_money = realMoney * ( 10 >= rake_back_rate ? 10 : rake_back_rate );
			break;
		case General::BenefitTier::BenefitTier_Standard:
			rakeback_money = realMoney * ( 30 >= rake_back_rate ? 30 : rake_back_rate );
			break;
		case General::BenefitTier::BenefitTier_Premium:
			rakeback_money = realMoney * ( 65 >= rake_back_rate ? 65 : rake_back_rate );
			break;
		}

		rakeback_money /= 10000;

		if ( rakeback_money > 0 ) {
			const uint64 cureRakeBack = m_player.rakeback_balance();
			m_player.set_rakeback_balance( cureRakeBack + rakeback_money );
			return rakeback_money;
		}
		return 0;
	}
	inline uint64 GetRakeBack() { return m_player.rakeback_balance(); }
	inline BOOL UseRakeBack( const uint64& rackback_money ) {

		if ( m_player.rakeback_balance() >= rackback_money ) {

			const uint64 new_balance = m_player.rakeback_balance() - rackback_money;
			m_player.set_rakeback_balance( new_balance );

			const uint64 new_coin = m_player.wallet_coins() + rackback_money;
			m_player.set_wallet_coins( new_coin );
			return TRUE;
		}
		return FALSE;
	}

	inline uint64 GetMaxHoldingChip()
	{
		switch ( GetMemberShipClass() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			return 600000000000; // 6천억
		case General::BenefitTier::BenefitTier_Standard:
			return 1500000000000; // 1조 5천억
		case General::BenefitTier::BenefitTier_Premium:
			return 3000000000000; // 3조
		default:
			return 600000000000; // 6천억
		}
	}
	inline uint64 GetMaxHoldingChip( General::BenefitTier  membership)
	{
		switch ( membership )
		{
		case General::BenefitTier::BenefitTier_Basic:
			return 600000000000; // 6천억
		case General::BenefitTier::BenefitTier_Standard:
			return 1500000000000; // 1조 5천억
		case General::BenefitTier::BenefitTier_Premium:
			return 3000000000000; // 3조
		default:
			return 600000000000; // 6천억
		}
	}

	inline uint64 GetMaxHoldingSafeChip()
	{
		switch ( GetMemberShipClass() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			return 2000000000; // 20억
		case General::BenefitTier::BenefitTier_Standard:
			return 1000000000000; // 1조
		case General::BenefitTier::BenefitTier_Premium:
			return 2000000000000; // 2조
		default:
			return 2000000000; // 20억
		}
	}

	inline uint64 GetMaxHoldingCoin()
	{
		uint64 max_holding_coin = 0;
		switch ( GetMemberShipClass() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			max_holding_coin = 2000000;
			break;
		case General::BenefitTier::BenefitTier_Standard:
			max_holding_coin = 80000000;
			break;
		case General::BenefitTier::BenefitTier_Premium:
			max_holding_coin = 150000000;
			break;
		default:
			max_holding_coin = 2000000; // 6천억
			break;
		}

		// 라운지 이벤트 기간 동안 증가
		uint64 max_event_coin = NetLib::cSingleton<cLoungeEvent>::GetInstance()->GetMaxHoldingCoin();

		// max_holding_coin 이 더 크면 max_holding_coin 리턴
		return max_holding_coin >= max_event_coin ? max_holding_coin : max_event_coin;
	}
	inline uint64 GetMaxHoldingCoin( General::BenefitTier  membership)
	{
		uint64 max_holding_coin = 0;
		switch ( membership )
		{
		case General::BenefitTier::BenefitTier_Basic:
			max_holding_coin = 2000000;
			break;
		case General::BenefitTier::BenefitTier_Standard:
			max_holding_coin = 80000000;
			break;
		case General::BenefitTier::BenefitTier_Premium:
			max_holding_coin = 150000000;
			break;
		default:
			max_holding_coin = 2000000; // 6천억
			break;
		}

		// 라운지 이벤트 기간 동안 증가
		uint64 max_event_coin = NetLib::cSingleton<cLoungeEvent>::GetInstance()->GetMaxHoldingCoin();

		// max_holding_coin 이 더 크면 max_holding_coin 리턴
		return max_holding_coin >= max_event_coin ? max_holding_coin : max_event_coin;
	}

	bool ester_egg = false;
	inline bool GetEsterEgg() { return ester_egg; }
	inline void SetEsterEgg(bool b_ester_egg ) { ester_egg = b_ester_egg; }
	/*virtual inline void SetFreeChip( uint64 chip ) {


	}*/


	// 코인 소유 제한 처리
	virtual inline void SetCoin( General::PlayCategory gameType , uint64 coin );
	virtual inline void SetCoinNoLimit( General::PlayCategory gameType , uint64 coin );
	void SetCoinFriend( General::PlayCategory gameType , uint64 coin );

	//클래스만료시 코인셋
	virtual inline bool SetExpiredCoin( General::PlayCategory gameType , uint64 coin );

	//무료코인획득시
	uint64 SetFreeCoin( General::PlayCategory gameType , uint64 coin );

	// 칩 소유 제한 처리
	virtual inline void SetChip( General::PlayCategory gameType , uint64 chip );

	//클래스 만료시 칩셋
	virtual bool SetExpiredChip( General::PlayCategory gameType );
	void SendChipToMail( uint64 playerIdx, uint64 chipAmount, const std::u8string& titleText );

	//무료칩획득시
	uint64 SetFreeChip( General::PlayCategory gameType , uint64 coin );

	inline double GetDealerFeeRate( const Server::ChannelPlayMode _channel_content_type , double& dealerFeeRate ) {

		double sale_dealerFeeRate = 1.0;

		// 친구 대전은 이벤트 제외
		if ( _channel_content_type == Server::ChannelPlayMode::ChannelPlayMode_None )
			return dealerFeeRate * sale_dealerFeeRate;

		switch ( GetMemberShipClass() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			break;
		case General::BenefitTier::BenefitTier_Standard:
			sale_dealerFeeRate = 0.7;  // 30% 할인이니, 딜러비는 0.7을 곱해주어야 할인율이 된다.
			break;
		case General::BenefitTier::BenefitTier_Premium:
			sale_dealerFeeRate = 0.5;
		}

		if ( _channel_content_type != Server::ChannelPlayMode::ChannelPlayMode_Lounge )
			return dealerFeeRate * sale_dealerFeeRate;
		else {

			// 라운지 이벤트 기간 동안 딜러비 할인 체크
			double loungeEventRate = NetLib::cSingleton<cLoungeEvent>::GetInstance()->GetDealerFee();

			// 딜러비는 비율은 적을수록 좋다.
			// 더 적은놈으로 리턴하도록 한다.
			double result = sale_dealerFeeRate <= loungeEventRate ? dealerFeeRate * sale_dealerFeeRate : loungeEventRate * dealerFeeRate;
			result *= 1000;
			result = ceil( result );
			return result / 1000;
		}
	}

#pragma region 플레이어 데이터 저장

	BOOL PlayerUpdateAsync();

#pragma endregion 플레이어 데이터 저장

#pragma endregion 맴버쉽 & 적립 통장

#pragma region 상점 구매 & 인앱 구매

	General::ResultCode CheckCondition( const Server::ShopProduct& shopProduct );
	Server::ProductData BuyShop( const Server::ShopProduct& shopProduct , bool& updateMoney , bool& updatePlayer , bool& updateAvatar , bool& updateMailBox , std::string& productDetail );

#pragma endregion  상점 구매 & 인앱 구매

#pragma region 유료 & 무료 재화 사용

	inline uint64 GetGemAndPaidGem() { return GetGem() + GetPaidGem(); }
	inline BOOL UseGemAndPaidGem( const uint64& useGem ) {

		if ( useGem > GetGemAndPaidGem() ) return FALSE;

		// 무료 다이아 부터 사용
		if ( GetGem() >= useGem )
		{
			uint64 newFreeGem = m_player.wallet_gems() - useGem;
			m_player.set_wallet_gems( newFreeGem );
		}
		else
		{
			uint64 minusPaidGem = useGem - GetGem();
			m_player.set_wallet_gems( 0 );
			uint64 newPaidGem = GetPaidGem() - minusPaidGem;
			m_player.set_paid_gems( newPaidGem );
		}
		return TRUE;
	}

	virtual inline bool MinusCoin( uint64 coin , bool checkOnly ) {
		if ( m_player.wallet_coins() < coin ) return false;
		if ( false == checkOnly ) {
			m_player.set_wallet_coins( m_player.wallet_coins() - coin );
			m_stockLostMoney += coin;
			m_virtualLostMoney += coin;
		}
		return true;
	}

	// 유, 무료 칩을 같이 계산한다.
	virtual inline bool MinusChip( uint64 chip , bool checkOnly ) {\
		if ( m_player.wallet_chips() < chip ) return false;
		if ( false == checkOnly ) {
			m_player.set_wallet_chips( m_player.wallet_chips() - chip );
			m_stockLostMoney += chip;
			m_virtualLostMoney += chip;
		}
		return true;
	}

	inline uint64 GetMoney( const General::AssetKind moneyType )
	{
		if ( moneyType == General::AssetKind::AssetKind_Chip )
			return GetChip();
		else if ( moneyType == General::AssetKind::AssetKind_Coin )
			return GetCoin();
		else
			return 0;
	}

#pragma endregion 유료 & 무료 재화 사용

#pragma region 손실제한

	void SetLostLimit( General::LossLimitProfile* lostLimit ) { m_pLostLimit = lostLimit; }
	General::LossLimitProfile* GetLostLimit() { return m_pLostLimit; }
	void CopyLostLimit( General::LossLimitProfile* lostLimit ) { lostLimit->CopyFrom( *m_pLostLimit ); }
	void UpdateLostLimitChip( const General::AssetKind& moneyType, const uint64& before_chip , const uint64& after_chip );
	void UpdateLostLimitPrice( const int32& price );
	bool CheckLostLimitPrice( const int32& price );
	bool IsOverLostLimit(bool setLostTime = false);
	bool CheckLostLimitTime();

	void UpdateDailyLostLimit();
	void UpdateMonthlyLostLimit();
	General::ResultCode ChangeLostLimit( const uint64& new_lost_limit , const int32& new_time_limit );

#pragma endregion 손실제한

#pragma region 출석부

	inline int GetAttendanceDays() { return m_player.attendance_streak_days(); }

	BOOL UpdateAttendance();
	BOOL AttendanceDay1Reward();
	BOOL GiveAttendanceDay1Reward();

#pragma endregion 출석부

#pragma region 안전 금고

	inline uint64 GetSafeChip() { return m_player.vault_chips(); }
	inline void SetSafeChip( const uint64 chip ) { m_player.set_vault_chips( chip ); }
	//inline uint64 GetSafePaidChip() { return m_player.safe_paid_chips(); }
	//inline void SetSafePaidChip( const uint64 chip ) { m_player.set_safe_paid_chips( chip ); }
	inline uint64 GetSafeCoin() { return m_player.vault_coins(); }
	inline void SetSafeCoin( const uint64 coin ) { m_player.set_vault_coins( coin ); }

#pragma endregion 안전 금고

#pragma region 인게임 플레이어 전적 누적

	inline void IngameRecord( const General::PlayCategory& gameType , const General::AssetKind& moneyType , const bool isWin , const uint64 beforeMoney , const uint64 afterMoney ) {
		m_room_record.set_asset_kind( moneyType );
		m_room_record.set_participation_total( m_room_record.participation_total() + 1 );

		// 승,패 만 처리 가능
		if ( isWin ) {
			m_room_record.set_live_wins(m_room_record.live_wins() + 1);
		}
		else {
			m_room_record.set_live_losses(m_room_record.live_losses() + 1);
		}

		if ( afterMoney > beforeMoney ) {
			int64 get_money = m_room_record.earned_amount() + ( afterMoney - beforeMoney );
			m_room_record.set_earned_amount( get_money );

			switch ( moneyType )
			{
			case General::AssetKind::AssetKind_Chip:
			{
				if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
					m_playerExt.set_holdem_daily_chips( m_playerExt.holdem_daily_chips() + ( afterMoney - beforeMoney ) );
				}
			}
			break;
			case General::AssetKind::AssetKind_Coin:
			{
				if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
					m_playerExt.set_holdem_daily_coins( m_playerExt.holdem_daily_coins() + ( afterMoney - beforeMoney ) );
				}
			}
			break;
			}
		}
		else {
			int64 get_money = m_room_record.earned_amount() - ( beforeMoney - afterMoney );
			m_room_record.set_earned_amount( get_money );

			switch ( moneyType )
			{
			case General::AssetKind::AssetKind_Chip:
			{
				if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
					m_playerExt.set_holdem_daily_chips( m_playerExt.holdem_daily_chips() - ( beforeMoney - afterMoney ) );
				}
			}
			break;
			case General::AssetKind::AssetKind_Coin:
			{
				if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
					m_playerExt.set_holdem_daily_coins( m_playerExt.holdem_daily_coins() - ( beforeMoney - afterMoney ) );
				}
			}
			break;
			}
		}
	}

	// 현재 바카라만 사용된다. ( 2개 걸고 둘 다 패배하면 잃은 금액 2배 뻥튀기됨 )
	inline void IngameRecord_bakara( const General::PlayCategory& gameType , const General::AssetKind& moneyType , const bool isWin , const bool isTie , const uint64 getMoney ) {

		m_room_record.set_asset_kind( moneyType );
		m_room_record.set_participation_total( m_room_record.participation_total() + 1 );

		// 승,패 만 처리 가능
		if ( false == isTie )
		{
			// 승,패 만 처리 가능
			if ( isWin ) {
				m_room_record.set_live_wins( m_room_record.live_wins() + 1 );
			}
			else {
				m_room_record.set_live_losses( m_room_record.live_losses() + 1 );
			}
		}

		int64 _getMoney = 0;
		if ( isWin )
		{
			_getMoney = getMoney;
		}
		else
		{
			_getMoney = getMoney * -1;
		}

		int64 get_money = m_room_record.earned_amount() + ( _getMoney );
		m_room_record.set_earned_amount( get_money );

		switch ( moneyType )
		{
		case General::AssetKind::AssetKind_Chip:
		{
			if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
				m_playerExt.set_holdem_daily_chips( m_playerExt.holdem_daily_chips() + ( _getMoney ) );
			}
		}
		break;
		case General::AssetKind::AssetKind_Coin:
		{
			if ( gameType == General::PlayCategory::PlayCategory_TexasHoldem ) {
				m_playerExt.set_holdem_daily_coins( m_playerExt.holdem_daily_coins() + ( _getMoney ) );
			}
		}
		break;
		}
	}

	// 현재 블랙잭에서만 사용한다.
	inline void IngameRecord( const General::PlayCategory& gameType , const General::AssetKind& moneyType , const int winCount , const int loseCount , const uint64 beforeMoney , const uint64 afterMoney ) {
		m_room_record.set_asset_kind( moneyType );
		m_room_record.set_participation_total( m_room_record.participation_total() + 1 );

		// 승,패 만 처리 가능
		if ( winCount ) {
			m_room_record.set_live_wins( m_room_record.live_wins() + winCount );
		}

		if ( loseCount ) {
			m_room_record.set_live_losses( m_room_record.live_losses() + loseCount );
		}

		if ( afterMoney > beforeMoney ) {
			int64 get_money = m_room_record.earned_amount() + ( afterMoney - beforeMoney );
			m_room_record.set_earned_amount( get_money );
		}
		else {
			int64 get_money = m_room_record.earned_amount() - ( beforeMoney - afterMoney );
			m_room_record.set_earned_amount( get_money );
		}
	}

	inline void IngameRecord_blackjack2( const General::PlayCategory& gameType , const General::AssetKind& moneyType , const int winCount , const int loseCount , const int64_t getMoney ) {
		m_room_record.set_asset_kind( moneyType );
		m_room_record.set_participation_total( m_room_record.participation_total() + 1 );

		// 승,패 만 처리 가능
		if ( winCount ) {
			m_room_record.set_live_wins( m_room_record.live_wins() + winCount );
		}

		if ( loseCount ) {
			m_room_record.set_live_losses( m_room_record.live_losses() + loseCount );
		}

		int64 get_money = m_room_record.earned_amount() + ( getMoney );
		m_room_record.set_earned_amount( get_money );
	}

	inline void IngameRecord( const General::PlayCategory& gameType , const General::AssetKind& moneyType , const int64_t getMoney ) {
		int64 get_money = m_room_record.earned_amount() + ( getMoney );
		m_room_record.set_earned_amount( get_money );
	}

	inline void CopyIngameRecord( General::InPlayRecord* pIngameRecord ) {
		if ( pIngameRecord != nullptr )
			pIngameRecord->CopyFrom( m_room_record );
	}

	// 금일 획득 재화 초기화. 매일 0시
	inline void ClearTodayGetMoney() {

		m_playerExt.set_lowbadugi_daily_chips( 0 );
		m_playerExt.set_holdem_daily_chips( 0 );
		m_playerExt.set_lowbadugi_daily_coins( 0 );
		m_playerExt.set_holdem_daily_coins( 0 );
		//m_player.set_pinball_today_coin( 0 );
	}

	// 최적화 필요
	inline void CopyTodayGetMoney( PmNet::FetchMemberDetailRS& _response ) {

		_response.set_lbd_daily_stack( m_playerExt.lowbadugi_daily_chips() );
		_response.set_hold_daily_stack( m_playerExt.holdem_daily_chips() );
		_response.set_lbd_daily_token( m_playerExt.lowbadugi_daily_coins() );
		_response.set_hold_daily_token( m_playerExt.holdem_daily_coins() );
	}

#pragma endregion 인게임 플레이어 전적 누적

#pragma region 게임결과 한도 초과 팝업 처리

private:
	General::PlayCategory m_limit_game_type;		// 한도 초과가 발생한 게임
	General::AssetKind m_limit_money_type;	// 칩 또는 코인 한도 초과
	bool m_mail_box_check;					// true 일 경우 우편함으로 지급 되었습니다. 알림 필요
	bool m_send_limit_popup;				// 발송한 경우 재 발송은 하지 않는다. 로비로 가는 경우에는 다시 갱신해 준다.

public:

	// 한도 초과가 발생된 클라이언트에게 알림 팝업을 발송한다.
	void SendMoneyLimitPopopOnResult();
	bool HasMoneyLimit() { return m_limit_game_type != General::PlayCategory::PlayCategory_None && m_limit_money_type != General::AssetKind::AssetKind_None; }
	void SendLostLimitPopopOnResult();

#pragma endregion 게임결과 한도 초과 팝업 처리

#pragma region 친구

	inline int GetDeleteFriendCount() { return m_playerExt.friend_deletes_today(); }
	inline void SetDeleteFriendCount( const int& delete_friend_count ) { m_playerExt.set_friend_deletes_today( delete_friend_count ); }

	void SyncFriend_Online( cClientSession* pClientSession , General::ContactState status = General::ContactState::ContactState_Online ,
		std::string channelid = "" );
	void SyncFriend_Online( bool bSyncFriendInfoStatus , cClientSession* pClientSession , General::ContactState status = General::ContactState::ContactState_Online ,
		std::string channelid = "" , int32 serverid = 0 );

	virtual void SendMyOfflineToFriendMap() override;

#pragma endregion 친구

#pragma region 로그

	std::string GetIp();
	inline void SetIp( std::string ip ) { m_ip = ip; };

#pragma endregion 로그

	virtual bool SendRequest( const UINT nCommand , google::protobuf::Message& _message , General::ResultCode errorCode , std::string& errorMessage );


public:
	virtual void SessionLogout(UINT Entity, BOOL bForce = 0) override;

public:
	virtual ~cClientSession();

protected:
	cClientSession();

public:
	cClientSession( const cClientSession& other );
#pragma region QA DATA 및 함수 정의

public:
	std::vector<General::PlayingCard> m_holdem_qa_deck;				// QA 홀덤 플레이어 덱, 시작할때마다 m_holdem_qa_dealer_decks 를 복사
	std::vector<General::PlayingCard> m_baduki_qa_deck;				// QA 바두기 플레이어 덱, 시작할때마다 m_baduki_qa_dealer_decks 를 복사

	static std::map<uint64 , uint64> m_qa_player_coins;
	static std::map<uint64 , uint64> m_qa_player_chips;

#pragma endregion QA DATA 및 함수 정의


#pragma region 로그인 보상

public:
	std::set<std::string> receive_login_reward_list;
	std::vector<PmNet::DropDetail> login_event_list;
	std::string SendReward( const std::string& reward_type , const std::string& count , const std::string& message );
	void parsePeriod( const std::string& count , uint64_t& index , uint64_t& period );
	void parseRewards( std::vector<std::pair<std::string , std::string>>& result , const std::string& rewards );

	void CheckLoginReward();

#pragma endregion 로그인 보상


	void IncreaseRoomJoinCount() {
		roomjoinCount++;
	}
	int GetRoomJoinCount() {
		return roomjoinCount;
	}


};