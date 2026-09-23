#pragma once
#include "TableServerHeader.h"
#include "cHoldem.h"
//#include "cRoulette.h"

#include <vector>
#include <algorithm>
#include <numeric>
#include <optional>

class cClientSession;
class cGameRoom
{
private:
	friend class cGameRoomManager;
	static NetLib::cCriticalSection s_CurRoomLock;
	static int s_CurRoomCnt;

private:
	std::map<__int64 , cClientSession*> m_mapPlayers;
	std::map<__int64, cClientSession*> m_mapWatchers;
	std::map<__int64, cClientSession*> m_mapCUsers;
	int m_nRoomNumber;
	int m_nManagedCommandThreadNumber;;
	int m_nMaxRoomPlayerCnt;
	BOOL m_bImmotal;							// 이 플래그가 셋팅된 방은 인원이 0 명이 되어도 방을 없애지 않는다.
	BOOL m_bOpenningRoom;
	// 홀덤, 로바용 방 통계 누적
	std::vector<uint64> m_pot_statistic;		// 팟 금액 통계
	int m_watcherCount = 0;


public:

	// 방제목 목록. 랜덤으로 내려줌
	static std::vector<std::u8string> lowBadugi_titles;
	static std::vector<std::u8string> holdem_titles;
	static std::vector<std::u8string> blackjack_titles;

	// 방제
	std::u8string m_room_title;

#pragma region 방의 특성
	// public 으로 두고, cLowBaduki 에서 access 하는 방법은 차후 변경하자
public:
	General::RoomSnapshot m_roomInfo;				// 방 정보
	std::string roomPasswd;						// 방 비밀번호

#pragma endregion 방의 특성

private: UINT m_uIp;							// IP Address 의 UINT 값
public: inline UINT GetUip() { return m_uIp; }
public: inline void SetUip( UINT uIp) { m_uIp = uIp; }

#pragma region 게임 인터페이스

private:
	IGame* m_gameInterface;

public:
	inline void SetGameInterface(IGame* pGame) { 
		m_gameInterface = pGame; 
	}
	inline IGame* GetGameInterface() { return m_gameInterface; }
	void DeleteGameInterface() {
		if ( m_gameInterface != nullptr ) {
		
			switch ( m_gameInterface->GetGameType() )
			{
			case General::PlayCategory::PlayCategory_TexasHoldem:
			{
				cHoldem* pGame = static_cast< cHoldem* >( m_gameInterface );
				delete pGame;
			}
			break;
			}
		
			m_gameInterface = nullptr;
		}
	}

#pragma endregion 게임 인터페이스

private:
	void Init();
	void Destroy();

public:
	static void InitializeTitles();

public:
	void Clear();

	size_t GetJoinedCnt() const;
	size_t GetWatcherCnt() const;
	inline int GetRoomNumber() const { return m_nRoomNumber; }
	inline void SetRoomNum(const int RoomNum) { m_nRoomNumber = RoomNum;}

	inline void RoomInfoCopy( General::RoomSnapshot* pRoomInfo ) { pRoomInfo->CopyFrom( m_roomInfo ); }

	inline void SetCommandThread(const int nManagedThread) { this->m_nManagedCommandThreadNumber = nManagedThread; }
	inline int GetCommandThreadArray() { return m_nManagedCommandThreadNumber; }

	inline General::RoomAccessMode GetRoomType() { return m_roomInfo.access_mode(); }
	inline void SetRoomType(const General::RoomAccessMode& roomType) { m_roomInfo.set_access_mode( roomType ); }

	inline General::PlayCategory GetGameType() { return m_roomInfo.play_category(); }
	inline void SetGameType(const General::PlayCategory& gameType) { m_roomInfo.set_play_category( gameType ); }

	inline void SetSeedMoneyTypeAndValue(const General::AssetKind& moneyType, const uint64& seedMoneyValue)
	{
		m_roomInfo.set_asset_kind( moneyType );
		m_roomInfo.set_seed_amount( seedMoneyValue );
	}

	inline General::BetPolicy GetBettingRueType() { return m_roomInfo.bet_policy(); }
	inline void SetBettingRuleType(const General::BetPolicy& bettingRuleType ) { m_roomInfo.set_bet_policy( bettingRuleType ); }

	inline General::AssetKind GetMoneyType() { return m_roomInfo.asset_kind(); }
	inline uint64 GetSeedMoneyValue() { return m_roomInfo.seed_amount(); }

	// 방의 형재 상태를 반환한다.
	inline General::RoomState GetRoomStatus() { return m_roomInfo.room_state(); }
	inline void SetRoomStatus(const General::RoomState& roomStatus) { m_roomInfo.set_room_state(roomStatus); }

	inline void SetMaxRoomPlayerCnt( const int playerCnt ) { m_nMaxRoomPlayerCnt = playerCnt; m_roomInfo.set_seat_limit(playerCnt); }
	inline int GetMaxRoomPlayerCnt() { return m_nMaxRoomPlayerCnt; }

	inline void SetImmotalRoom() { m_bImmotal = TRUE; }
	inline BOOL isImmotalRoom() { return m_bImmotal; }

	inline void SetOpenningRoom(BOOL bOpenningRoom) { m_bOpenningRoom = bOpenningRoom; }
	inline BOOL GetOpenningRoom() { return m_bOpenningRoom; }

	General::ResultCode JoinPossible();

	General::ResultCode RoomJoin(cClientSession* pClientSession, const int slotNumber = 0);
	General::ResultCode RoomJoinCheat(cClientSession* pClientSession, const int slotNumber = 0);
	General::ResultCode RoomReJoin( cClientSession* pClientSession );
	General::ResultCode RoomSlotJoin(cClientSession* pClientSession, const int slotNumber);
	General::ResultCode RegisterWatcher(cClientSession* pClientSession);
	void RemoveWatcher(const uint64& playerIdx);
	void RemoveWatchars();
	void RemovePlayer(const uint64& playerIdx);
	void RemoveCheatUser( uint64 playerIdx ) { m_mapCUsers.erase( playerIdx ); }
	int ClearWrongSession( std::set<cClientSession*>& sessions );
	void ClearOutUser();
	void CheckMaintance();
	bool isPlayer(const uint64& playerIdx);
	bool isWatcher(const uint64& playerIdx);
	bool isCheat(const uint64& playerIdx);
	//bool FailOverRoomJoin(const __int64 AID, cClientSession* client);
	bool RoomOut(const __int64 playerIdx, cClientSession* pClientSession, bool kicked = false );										// 방 즉시 나감
	bool DieRoomOut(const __int64 playerIdx, cClientSession* pClientSession, bool kicked = false );										// 방 즉시 나감
	void RoomOutReservationReverse( const __int64 playerIdx , cClientSession* pClientSession, bool reserve, bool reserve_cancel );				// 방 나가기 예약
	void RoomOutReservation( const __int64 playerIdx , cClientSession* pClientSession, bool reserve, bool reserve_cancel );				// 방 나가기 예약
	void KickRoomOutReservedOrAllinOrLostLimitPlayer();																							// 방 나가기 예약자 내보내기
	void RoomOutNotEnoughSeedPlayer();																									// 결과 처리 후 시드머니가 모자란 유저를 방에서 내보냄
	//void ActivateVote();
	cClientSession* FindSession(const __int64 playerIdx);
	cClientSession* FindWatcherSession( const __int64 playerIdx );
	bool MoveRoom(const __int64 AID);
	void SetTriggerChangeInRoomMove(const __int64 AID);
	bool RemoveSpecifiedUser(const __int64 playerIdx);

	//std::list<cClientSession*> GetPlayersSessionList();

	// BroadCast 플레잉 플레이어
	bool RoomBroadCast(BYTE* pData, UINT nLength);
	void RoomBroadCast(const UINT nCommand, BYTE* pData, UINT nLength, int64 except_playerIdx = 0);
	bool RoomBroadCast(const UINT nCommand, google::protobuf::Message* _message, int64 except_playerIdx = 0);
	bool RoomBroadCast(const UINT nCommand, google::protobuf::Message& _message, int64 except_playerIdx = 0);
	bool RoomBroadCastCUser(const UINT nCommand, google::protobuf::Message& _message, int64 except_playerIdx = 0);

	// BroadCast 관람 플레이어
	bool WatcherBroadCast( BYTE* pData , UINT nLength );
	void WatcherBroadCast( const UINT nCommand , BYTE* pData , UINT nLength , int64 except_playerIdx = 0 );
	bool WatcherBroadCast( const UINT nCommand , google::protobuf::Message* _message , int64 except_playerIdx = 0 );
	bool WatcherBroadCast( const UINT nCommand , google::protobuf::Message& _message , int64 except_playerIdx = 0 );

	// 방안에 존재하는 전체 플레이어에게 전송
	void BroadCastToAllPlayer( const UINT nCommand , google::protobuf::Message& _message , int64 except_playerIdx = 0 ,bool tocheat = true);
	void BroadCastToCheatPlayer( const UINT nCommand , google::protobuf::Message& _message , int64 except_playerIdx = 0 ,bool tocheat = true);

	bool SendRequest(cClientSession* pClientSession, const UINT nCommand, google::protobuf::Message& _message, General::ResultCode errorCode, std::string& errorMessage);

	static cGameRoom* CreateRoom(const int RoomNum);

	void GetRoomInfo( General::RoomListEntry& roomListInfo );		// 방 세부 정보

	void GetGameRoomDetail( PmNet::ChamberEnterRS& roomJoinRes , bool is_watcher = false );
	void GetGameRoomDetailCheat( PmNet::ChamberEnterRS& roomJoinRes , bool is_watcher = false );
	void GetGameRoomDetailOnRejoin( PmNet::ChamberEnterRS& roomJoinRes , const uint64& rejoin_player_idx );

	bool PlayingRoomRemoveCheck(); // 플레이중인 방의 삭제 조건 체크

	void PushPotMoneyValue( const uint64 potMoney );

	void CalcPotMoneyStatistic();

	std::optional<uint64> GetMaxRecentValue();
	uint64 GetAverageRecentValue();

	void CheckWatcherCount()
	{
		if ( m_watcherCount != GetWatcherCnt() - m_gameInterface->GetReservationPlayerPlayerCount() )
		{
			GetWatcherCount();
		}
	}

	void GetWatcherCount()
	{
		m_watcherCount = GetWatcherCnt() - m_gameInterface->GetReservationPlayerPlayerCount();
		PmNet::InformObserverCntRS _watcher_res;
		_watcher_res.set_observer_cnt( m_watcherCount );
		BroadCastToAllPlayer( General::Packet_WatcherCountNotice , _watcher_res );
	}

	void MakeUserLog( std::string& log )
	{
		log += "\n";
		log += std::format( "room :{}  m_mapWatchers : " , GetRoomNumber() );
		for ( auto t : m_mapWatchers )
		{
			log += std::format("{} ,", t.first);
		}
		log += "\n";
		log += std::format( "room :{}  m_mapCUsers : " , GetRoomNumber() );
		for ( auto t : m_mapCUsers )
		{
			log += std::format( "{} ," , t.first );
		}
		log += "\n";
		log += std::format( "room :{}  m_mapPlayers : " , GetRoomNumber() );
		for ( auto t : m_mapPlayers )
		{
			log += std::format( "{} ," , t.first );
		}
		log += "\n";
		log += std::format( "room :{}  m_mapPlayers : " , GetRoomNumber() );
		for ( auto t : m_mapPlayers )
		{
			log += std::format( "{} ," , t.first );
		}
	}

	// 테스트방 설정. 들어오면 안되는방
public:
	bool m_bTestRoom;
	void SetDontEnter();

public:


public:
	cGameRoom();
	~cGameRoom();
};

