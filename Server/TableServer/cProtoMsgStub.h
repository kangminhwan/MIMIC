#pragma once
#include "TableServerHeader.h"
#include "IStub.h"

#include <future>

class cClientSession;
class cProtoMsgStub : public IStub
{
private:
	virtual void bindmethod( std::function<void( NetLib::cInterfaceIocpContext* /*pContext*/ , UINT /*nCommand*/ , BYTE* /*pData*/ , UINT /*nLength*/ , UINT /*nThreadIndex*/ )>* fp ) override;
	virtual void bindmethod( std::function<void( UINT /*nID*/ , UINT /*nCommand*/ , BYTE* /*pData*/ , UINT /*nLength*/ , UINT /*nThreadIndex*/ )>* fp ) override {};

	virtual void bindmethod( std::function<void( NetLib::cUDPDispatcher* /*pUDPDispatcher*/ , UINT /*nCommand*/ , UINT /*Entity*/ , UINT /*uiPacketSeq*/ , NetLib::cCommandQueueElement* /*pCommandQueueElement*/ , BYTE* /*pData*/ , UINT /*nLength*/ , UINT /*nThreadIndex*/ )>* fp ) override {}

private:
	bool CheckSession( NetLib::cInterfaceIocpContext* pContext );
	bool HasSession( NetLib::cInterfaceIocpContext* pContext );
	static cClientSession* GetSession( NetLib::cInterfaceIocpContext* pContext );
	void SendMessageAndLogWrite( const General::ResultCode& errorCode , NetLib::cInterfaceIocpContext* pContext , UINT nThreadIndex , General::PacketID messageId , google::protobuf::Message& _message );

private:
	//void SYS_NET_SESSION_LOG_OUT(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex);

private:
	void SYS_NET_CONNECT( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SYS_NET_DISCONNECT( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CHEAT_DISCONNECT( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	void SYS_NET_CONNECT_TO_SERVER( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// SERVER TO SERVER BEGIN

	void SERVER_MSG_REQ_AUTHENTICATION( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SERVER_MSG_RES_AUTHENTICATION( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomListSync( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomListToLobby( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomListToLobbyResponse( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void LostLimitSync( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void LostLimitSyncLobbyInit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void LostLimitUpdate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncFriendInfo( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncFriendInfoStatus( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// SERVER TO SERVER END

	void Ping( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PingClient( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CreateAccount( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CreatePlatform( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void GetCreatedPlatformsByCid( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void GetCreatedPlatformsByCidForCreate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CreateMADEPlatform( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void Login( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void ReLogin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void TransferServer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void MADEPassChange( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void ResetSubPassword( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void UpdateTermsAgree( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void UpdateCiExpiryTime( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void WithdrawalGame( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CancelWithdrawalGame( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void NickChange( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void UpdatePush( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	void RoomCreate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void FriendRoomCreate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomList( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomJoin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void MoveRoom( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomOut( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomOutReserve( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PlayStart( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PlayBet( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void GameParticipate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CancelGameParticipate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CancelBet( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void TrasferToWatcher( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CancelTrasferToWatcher( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RoomJoinAsWatcher( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void MoveSlot( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void ResultComplete( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SendEmoticon( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void RabbitHunt( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void ShowHand( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void KickOutPlayer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CancelKickOutPlayer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 슬롯 게임 메시지 정의
	/*void Spin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void EnterSlotGame( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SpinAck( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void LeaveSlotGame( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void ChangeTotalBet( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void MakeAckRemainSpins( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void BuyGrandSpinPopup( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void GetVirtualCoin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );*/

	// 투표 Vote System
	/*void RoomTryVote(NetLib::cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex);
	void RoomVoteAnswer(NetLib::cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex);*/

	// Player Data Update
	void PlayerSetAvatar( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PlayerSetSubPasswd( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PlayerChangeSubPasswd( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PlayerRemoveSubPasswd( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// Lobby 갱신
	void UpdateLobby( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	//Daily갱신
	void DailyRefresh( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 우편함
	void MailBox( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex ); // 메일 목록
	void MailOpen( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );// 메일 오픈
	void NoticeMessage( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );// 쪽지함 ( 공지 메시지 )

	// 미션, 업적처리
	void GetQuests( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void GetQuestReward( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 무료충전소
	void GetFreeCharge( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 적립 통장
	void WithdrawRakeback( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 상점 구매
	void BuyShop( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void BuyShopThread( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 손실 한도
	void ChangeLostLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CheckLostLimitTime( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
		void GetLostLimitOptions( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 안전 금고 입금, 출금
	void SafeMoneyDeposit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SafeMoneyWithdraw( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 광고
	void AdWatch( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 플레이어 정보 보기
	void GetPlayerInfo( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 친구
	void AddFriend( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void DeleteFriend( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void LobbyPlayerList( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void FriendList( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SearchPlayer( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 개발 Cheat Message
	void CheatMoney( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CheatCardChange( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// QA Tool Cheat
	void QALogin( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void CardDeckCheat( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void UidSearchByNickname( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void InitLostLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void InitBuyLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	//void CHEAT_Roulette( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void GetRoomNumber( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	//void CHEAT_PINBALL( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );


	// 접속 보상
	void parseRewards( std::vector<std::pair<std::string , std::string>>& result , const std::string& rewards );//보상 파싱
	void UpdateLoginEvent( NetLib::cInterfaceIocpContext* pContext );
	void UpdateUserLoginEvent( NetLib::cInterfaceIocpContext* pContext );
	//void CheckLoginReward( NetLib::cInterfaceIocpContext* pContext );
	void CheckLoginReward( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	//VIP
	void VIPSaveUser( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void VIPLoadUser( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void ChangeMailState( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	void ChangeDailyExpired( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 운영툴 오퍼레이션
	void OpsPlayerKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncOpsPlayerKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncOpsPlayerKickResponse( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	void OpsUpdateLostLimit( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );


	void OpsMarketKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncOpsMarketKick( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncPlayerInfo( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void SyncPlayerLoginNoti( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	void SyncGameVersionUpdate( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 클라이언트 에러로그
	void ClientErrorLog( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );

	// 슬롯 이벤트

	//핀볼
	/*void EnterPinball( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void BuyPinball( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void PinballAck( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );
	void LeavePinball( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex );*/

public:
	// IIS Request
	static Server::ServiceStatusCode WebPostRequest( const std::string& url ,
		google::protobuf::Message& _postMessage ,
		UINT nCommand ,
		int iServerID ,
		uint64 playerIdx ,
		std::string& data ,
		std::string& billingerror ,
		const uint32 nThreadIndex ,
		const int default_time_out_second = 10 );

	static std::future<Server::ServiceStatusCode> WebPostRequestAsync( const std::string& url ,
		google::protobuf::Message& _postMessage ,
		UINT nCommand ,
		int iServerID ,
		uint64 playerIdx ,
		std::string& data ,
		std::string& billingerror,
		const uint32 nThreadIndex ,
		const int default_time_out_second = 10 );
	std::vector<uint64> EsterUser;

public:
	cProtoMsgStub();
	virtual ~cProtoMsgStub();
};