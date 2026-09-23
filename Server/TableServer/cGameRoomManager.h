#pragma once
#include "TableServerHeader.h"

#include "cUsingGameRoomContainer.h"

#include <algorithm>
#include <random> // std::random_device, std::mt19937

class cInstancePacketParser;
class cClientSession;
class cGameRoom;
class cGameRoomManager
{
private:
	std::map<int , cGameRoom*> m_playingContainer;
	std::map<int, cGameRoom*> m_gameRoomPooler;		// Game Room Pooler

	std::queue<cGameRoom*> m_friendRoomPooler;							// 친구 방 전용 Pooler

	std::mt19937 gen;
	std::uniform_int_distribution<> m_dis;
	std::uniform_int_distribution<> m_friend_dis;						// 1000 ~ 9999 까지 생성하도록한다. 999 + ( 1 ~ 9000 )
	//std::uniform_int_distribution<> m_friend_dis_lobby_1;				// 1000 ~ 9999 까지 생성하도록한다. 999 + ( 1 ~ 4500 )
	//std::uniform_int_distribution<> m_friend_dis_lobby_2;				// 1000 ~ 9999 까지 생성하도록한다. 999 + ( 4501 ~ 9000 )

public:
	static UINT MAX_USER;
	static UINT MAX_ROOMS;

public:
	void Init();
	void Destroy();

public:
	cGameRoom* FindPlayingGameRoom(UINT uiGameRoomNum);

	bool CheckUserGameRoomOut( cClientSession* pClientSession );
	bool UserGameRoomOut(cClientSession* pClientSession);
	bool RemoveWatcherResource( cClientSession* pClientSession );
	cClientSession* GameRoomFindSession(const int nManagedThread, const UINT uiGameRoomNum, const General::RoomAccessMode roomType, const int64 playerIdx);
	BOOL GameRoomBroadCast(cClientSession* pClientSession, const int nManagedThread, const UINT nCommand, google::protobuf::Message& _message, BOOL bExceptMe = FALSE);
	BOOL GameRoomBroadCast(const int nGameRoomNumber, const int nManagedThread, const UINT nCommand, google::protobuf::Message& _message);
	BOOL SendRoomJoin(cClientSession* pClientSession, const int nManagedThread);

	int CreateFriendRoomNumber();

	inline bool GoToLobby_1( const int room_number ) {

		if ( 999 < room_number && room_number < 10000 )
			return true;

		return false;
	}	
	//int CreateFriendRoomNumber( const int server_id );
	/*inline int FindServerId( const int room_number ) {

		if ( 999 < room_number && room_number < 5500 )
			return 1;

		if ( 5500 < room_number && room_number < 9999 )
			return 2;

		return 0;
	}*/

	// 방상태 변경
public:
	bool OccupiedRoom( const Server::Channel& channelData,
		const General::RoomAccessMode& roomType,
		const General::BetPolicy& bettingRuleType,
		const int32& show_down_delay_ms_per_player,
		const int32& show_down_community_delay_ms_per,
		cGameRoom* pGameRoom, 
		const int nManagedThread, 
		uint32 uIp,
		const int max_players,
		const bool& createFlag = false);

public:
	cGameRoom* PopPlayingRoom( const Server::Channel& channelData , const General::BetPolicy& bettingRueType, cClientSession* pClientSession , const int nManagedThread );
	cGameRoom* PopPlayingRoomExceptSameIp( General::RoomAccessMode roomType , uint32 uIp , const int nManagedThread );
	cGameRoom* PopForRoomCreation( const int nManagedThread );
	cGameRoom* PopEmptyRoom(General::RoomAccessMode roomType, cClientSession* pClientSession , const int nManagedThread);
	cGameRoom* PopFriendRoom();
	cGameRoom* FindRoom( const int roomNumber , const int nManagedThread );
	cGameRoom* FindPlayingRoom( const int roomNumber , const int nManagedThread );

	void Process(const UINT uiThread);
	BOOL Update(const UINT uiThread, const UINT uiUpdateType);
	
	google::protobuf::RepeatedField<General::RoomListEntry> GetRoomList( const General::PlayCategory& gameType, const std::map<std::string , int>& channel_ids , int& totalCount, int& totalPage , const UINT uiThread , int startIndex , int pageSize );

#pragma region 로비서버 데이터 씽크

public:
	google::protobuf::RepeatedField<General::RoomListEntry> GetPlayingRoomListForLobbySync( const General::PlayCategory gameType );

#pragma endregion 로비서버 데이터 씽크

public:
	size_t UseGameRoomCnt();

public:
	cGameRoomManager();
	~cGameRoomManager();
};

