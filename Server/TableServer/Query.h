#pragma once
#include "TableServerHeader.h"

//#include "../../ProtocolBuffer/cpp/General.pb.h"
//#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include <future>
#include <tuple>
#include "./mysql/DefProcedure.h"

using std::string;

class cMySQLReader;
class QueryManager
{
public:
    QueryManager();

    static std::future<BOOL> CreateAccountAsync( string _name_auth_code , string _name_auth_type , string _account_guid , int _db_index , uint64 _lost_limit , int _lost_limit_change_count , string _refresh_loss_limit , uint64 _buy_limit , string _refresh_buy_limit , uint64_t& _user_account_idx );
    static BOOL CreateAccount( string _name_auth_code , string _name_auth_type , string _account_guid , int _db_index , uint64 _lost_limit , int _lost_limit_change_count , string _refresh_loss_limit , uint64 _buy_limit , string _refresh_buy_limit , uint64_t& _user_account_idx );

    static BOOL DeleteAccountAsync( const std::string& _platform_guid );

    static string AccountGetByAuthCode( string _name_auth_code );

    static std::future<General::LossLimitProfile> GetLostLimitAsync( const std::string& account_guid );
    static General::LossLimitProfile GetLostLimit( const std::string& account_guid );

    static BOOL CreatePlatform( string _account_guid , string _platform_guid , string _platform_authcode , BYTE _platform_code , string _MADE_id , string _MADE_password , BYTE _push_token_os , string _push_token , int _db_index , string _sub_password , uint64_t& _platform_idx );
    static std::string GenerateCreatePlatform( string _account_guid , string _platform_guid , string _platform_authcode , BYTE _platform_code , string _MADE_id , string _MADE_password , BYTE _push_token_os , string _push_token , int _db_index , string _sub_password , uint64_t& _platform_idx );
    static std::future<BOOL> FindMADEPlatformAsync( const std::string& _MADE_id , std::string& _account_guid , std::string& _platform_guid , BYTE& _platform_code , std::string& _MADE_password , int& _passwd_retry_count ,
        std::string& _sub_password , int& _sub_passwd_retry_count , BOOL& _game_play_agree , BOOL& _personal_info_agree , BOOL& _advertise_push_agree , BOOL& _night_advertise_push_agree , std::string& expected_withdrawal_date, int& _cancle_withdrawal_first_join );
    static BOOL FindMADEPlatform( const std::string& _MADE_id , std::string& _account_guid , std::string& _platform_guid , BYTE& _platform_code , std::string& _MADE_password , int& _passwd_retry_count ,
        std::string& _sub_password , int& _sub_passwd_retry_count , BOOL& _game_play_agree , BOOL& _personal_info_agree , BOOL& _advertise_push_agree , BOOL& _night_advertise_push_agree , std::string& expected_withdrawal_date, int& _cancle_withdrawal_first_join );
    static BOOL UpdateMADEPasswordRetryCount( std::string _account_guid , std::string _platform_guid , int _passwd_retry_count );
    static BOOL ResetMADEPassword( std::string _account_guid , std::string _platform_guid , std::string _MADE_password );
    static BOOL UpdateSubPassword( const std::string& _account_guid , const std::string& _platform_guid , const std::string& _hashed_sub_password );
    static BOOL UpdateSubPasswordRetryCount( std::string _account_guid , std::string _platform_guid , int _passwd_retry_count );
    static BOOL RemoveSubPasswordRetryCount( std::string _platform_guid );
    static std::future<BOOL> FindPlatformAsync( const std::string _account_guid , const std::string _platform_guid , std::string& _platform_authcode , std::string& _sub_password , int& _sub_passwd_retry_count ,
        BOOL& _game_play_agree , BOOL& _personal_info_agree , BOOL& _advertise_push_agree , BOOL& _night_advertise_push_agree , std::string& _expected_withdrawal_date , int& _cancle_withdrawal_first_join );
    static BOOL FindPlatform( const std::string _account_guid , const std::string _platform_guid , std::string& _platform_authcode , std::string& _sub_password , int& _sub_passwd_retry_count ,
        BOOL& _game_play_agree , BOOL& _personal_info_agree , BOOL& _advertise_push_agree , BOOL& _night_advertise_push_agree , std::string& _expected_withdrawal_date, int& _cancle_withdrawal_first_join );
    static std::future<BOOL> FindPlatformByCidAndPlatformGuidAsync( const std::string _cid , const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count );
    static BOOL FindPlatformByCidAndPlatformGuid( const std::string _cid , const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count );
    static std::future<BOOL> FindSubPasswordByPlatformGuidAsync( const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count, std::string& _sub_password_update_time );
    static BOOL FindSubPasswordByPlatformGuid( const std::string _platform_guid , std::string& _sub_password , int& _sub_passwd_retry_count , std::string& _sub_password_update_time );

    static std::future<BOOL> UpdateTermsAgree( const std::string& _MADE_id , const BOOL _game_play_agree , const BOOL _personal_info_agree , const  BOOL _advertise_push_agree , const BOOL _night_advertise_push_agree );
    static std::future<BOOL> UpdateTermsAgreeByPlatformGuid( const std::string& _platform_guid , const BOOL _game_play_agree , const BOOL _personal_info_agree , const  BOOL _advertise_push_agree , const BOOL _night_advertise_push_agree );
    static std::future<BOOL> UpdatePushAgreeByPlatformGuid( const std::string& _platform_guid , const  BOOL _advertise_push_agree , const BOOL _night_advertise_push_agree );
    static std::future<BOOL> UpdatePushToken( const std::string& _platform_guid , const  BYTE& _push_token_os , const string _push_token );

    static BOOL PlatformGet( string _account_guid , string _platform_authcode , string& _platform_guid , BYTE& _platform_code , string& _MADE_id , BYTE& _push_token_os , string& _push_token , int& _db_index , uint64_t& _platform_idx );

    static std::future<BOOL> FindAccountByPlatformAuthCodeAsync( const std::string& _platform_authcode , std::string& _account_guid , string& _platform_guid );
    static BOOL FindAccountByPlatformAuthCode( const std::string& _platform_authcode , std::string& _account_guid , string& _platform_guid );

    static std::future<string> FindCiExpiryTimeAsync( const std::string& _account_guid );
    static std::string FindCiExpiryTime( const std::string& _account_guid );

    static std::future<string> FindExpiryTimeByCiAsync( const std::string& ci );
    static std::string FindExpiryTimeByCi( const std::string& ci );

    static BOOL UpdateNiceAuthExpiryTime( const std::string& ci , const std::string& expiry_time );

    static BOOL UpdateDailyRefreshTime( const std::string& refresh_time , const std::string& platform_guid );

    static BOOL UpdateSystemData( const std::string& data_type , const std::string& data );

    static BOOL InsertPlayer(
        string _account_guid ,
        string _platform_guid ,
        string _nickname ,
        int _level ,
        uint64 _exp ,
        uint64 _chips ,
        uint64 _coin ,
        uint64 _gem ,
        int _kick_ticket_count ,
        int _avatar_id ,
        bool _membership_activated ,
        string _membership_expiry_time ,
        int _chips_refill_count ,
        int _coin_refill_count ,
        uint64 _remain_slot_coin ,
        string _daily_refresh_time ,
        string _monthly_refresh_time ,
        uint64& _player_idx );

    static std::future<BOOL> PlayerUpdateByQuery( const General::ParticipantProfile& player , const Server::ParticipantProfileInternal& playerExt );
    //static BOOL PlayerUpdate( const General::ParticipantProfile& player );
    //static std::future<BOOL> PlayerUpdateAsync( const General::ParticipantProfile& player );
    static BOOL PlayerMoneyUpdate( const uint64& _player_idx , const uint64& _coin , const uint64& _chip , const uint64& _rakeback , const uint64& _gem , const uint64& _paid_chips , const uint64& _paid_coin , const uint64& _paid_gem );
    static std::string GeneratePlayerMoneyUpdate( const uint64& _player_idx , const uint64& _coin , const uint64& _chip , const uint64& _rakeback , const uint64& _gem , const uint64& _paid_chips , const uint64& _paid_coin , const uint64& _paid_gem );

    static BOOL PlayerSafeMoneyUpdate( const General::ParticipantProfile& player );

    static BOOL PlayerSetSubPasswd( const uint64& _player_idx , const bool& _sub_passwd_activated , const std::string& _sub_passwd );

    static BOOL GetPlayer( string _account_guid , string _platform_guid , General::ParticipantProfile& _player , Server::ParticipantProfileInternal& _playerExt );

    static BOOL GetPlayerCoin( const uint64& _player_idx, General::ParticipantProfile& _player, Server::ParticipantProfileInternal& _playerExt );

    static BOOL TransactionTest( string _account_guid , string _platform_guid , BYTE _platform_code , string _nickname , BYTE _push_token_os , string _push_token , int _db_index , uint64_t& _platform_idx );

    //static General::ParticipantProfile ParsePlayer( cMySQLReader& reader );

    static BOOL PlayerIdxSearchByNickname( string _nack_name , PmNet::MidLookupByAliasRS& response );

    static BOOL AvatarsGet( const uint64& player_idx , std::map<int , General::AvatarProfile>& avatars );

    static BOOL InsertAvatar( const uint64& _player_idx , const int& _avatar_id , const std::string& expiry_date , uint64& _avatar_idx );
    static std::future<BOOL> InsertAvatarAsync( const uint64& _player_idx , const int& _avatar_id , const std::string& expiry_date , uint64& _avatar_idx );

    static BOOL PlayerSetAvatar( const uint64& _player_idx , const int& _avatar_id );

    static std::string GeneratePlayerUpdateAvatar( const uint64& _player_idx , const int& _avatar_id , const std::string _expiry_date );
    static std::future<BOOL> PlayerUpdateAvatarAsnc( const uint64& _player_idx , const int& _avatar_id , const std::string _expiry_date );
    static BOOL PlayerUpdateAvatar( const uint64& _player_idx , const int& _avatar_id , const std::string _expiry_date );

    static std::string GenerateDailyRefreshTimeUpdateQuery( const uint64& _player_idx , const std::string& _daily_refresh_time );

    static BOOL UpdateNickName( const uint64& _player_idx , const std::string& _nick_name );
    static std::future<BOOL> SelectNickNameAsync( const std::string& _nick_name , std::vector<std::string>& _find_nicks );
    static BOOL SelectNickName( const std::string& _nick_name , std::vector<std::string>& _find_nicks );

    static std::future<string> FindAccountGuidByNicknameAsync( const std::string& _nick_name );
    static std::string FindAccountGuidByNickname( const std::string& _nick_name );

    static std::future<BOOL> SelectByCidAsync( const std::string& _cid , std::vector<std::string>& _MADE_ids );
    static BOOL SelectByCid( const std::string& _cid , std::vector<std::string>& _MADE_ids );
 
    static std::future<BOOL> DeletedByCidAsync( const std::string& _cid , std::vector<std::string>& _platform_guids );
    static BOOL DeletedByCid( const std::string& _cid , std::vector<std::string>& _platform_guids );

    static std::future<BOOL> SelectMADEByCidAsync( const std::string& _cid , std::vector<std::string>& _MADE_ids );
    static BOOL SelectMADEByCid( const std::string& _cid , std::vector<std::string>& _MADE_ids );

    static std::future<BOOL> SelectMADEIdAsync( const std::string& _MADE_id , std::vector<std::string>& _find_ids );
    static BOOL SelectMADEId( const std::string& _MADE_id , std::vector<std::string>& _find_ids );

#pragma region 플레이어 제재

    static std::future<BOOL> SelectAccountSanctionAsync( const std::string& account_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason );
    static BOOL SelectAccountSanction( const std::string& account_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason );

    static std::future<BOOL> SelectPlatformSanctionAsync( const std::string& platform_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason );
    static BOOL SelectPlatformSanction( const std::string& platform_guid , std::string& sanction_period_start , std::string& sanction_period_end , std::string& sanction_reason );

#pragma endregion 플레이어 제재

#pragma region 게임 탈퇴

    static BOOL PlatformWithdrawalGame( const std::string account_guid , const std::string platform_guid , const std::string expected_withdrawal_date );
    static BOOL PlatformCancleWithdrawalFirstJoin( const std::string account_guid , const std::string platform_guid , const int cancle_withdrawal_first_join );

#pragma endregion 게임 탈퇴

#pragma region RECORD

    static BOOL PlayerGetRecords( const uint64& _player_idx , int& reads , PmNet::Ledger& records );
    static std::future<BOOL> CreateRecordsAsync( const uint64& _player_idx );
   /* static std::future<BOOL> CreatePinballRecordsAsync( const uint64& _player_idx );
    static std::future<BOOL> CreateRouletteRecordsAsync( const uint64& _player_idx );*/

    //static BOOL PlayerExecuteQuery( std::string executeQuery );
    //static std::future<BOOL> PlayerExecuteQueryAsync( std::string executeQuery );
    static BOOL PlayerExecuteQuery( const E_DB_TYPE& db_type , std::string executeQuery );
    static BOOL PlayerExecuteQuery_ID( const E_DB_TYPE& db_type , std::string executeQuery , uint64& inserted_id );
    static std::future<BOOL> PlayerExecuteQueryAsync( const E_DB_TYPE& db_type , std::string executeQuery );
    static std::future<BOOL> PlayerExecuteQueryAsync_ID( const E_DB_TYPE& db_type , std::string executeQuery , uint64& inserted_id );

    static std::future<unsigned int> PlayerExecuteQueryWithErrorCodeAsync( const E_DB_TYPE& db_type , std::string executeQuery );
    static unsigned int PlayerExecuteQueryWithErrorCode( const E_DB_TYPE& db_type , std::string executeQuery );

    static BOOL PlayerExecuteQueryWithAffectedRows( const E_DB_TYPE& db_type , std::string executeQuery );
    static std::future<BOOL> PlayerExecuteQueryWithAffectedRowsAsync( const E_DB_TYPE& db_type , std::string executeQuery );

    static BOOL UpdateExecuteQuery( const E_DB_TYPE& db_type , std::string executeQuery );
    static std::future<BOOL> UpdateExecuteQueryAsync( const E_DB_TYPE& db_type , std::string executeQuery );


    static std::string GenerateRecordsInsertQuery( const uint64& player_idx ,
                                const int& game_type ,
                                const std::string& game_type_string ,
                                int participate_count ,
                                int participate_count_total ,
                                int win_count ,
                                int win_count_total ,
                                int lose_count ,
                                int lose_count_total ,
                                int best_get_chip ,
                                int best_get_chip_total ,
                                int best_get_coin ,
                                int best_get_coin_total ,
                                int make_all_in_count_total ,
                                int straight_wins ,
                                int slot_get_coin ,
                                int today_chip ,
                                int today_coin );

    static std::string UpdateRecordsInsertQuery( const uint64& player_idx ,
                                const std::string& game_type_string ,
                                int participate_count ,
                                int participate_count_total ,
                                int win_count ,
                                int win_count_total ,
                                int lose_count ,
                                int lose_count_total ,
                                int64 best_get_chip ,
                                int64 best_get_chip_total ,
                                int64 best_get_coin ,
                                int64 best_get_coin_total ,
                                int make_all_in_count_total ,
                                int straight_wins ,
                                int64 slot_get_coin,
                                int64 today_chip,
                                int64 today_coin,
                                const std::string& dateTime );

#pragma endregion RECORD

#pragma region JOKBO RECORD

    static BOOL CreateJokboRecords( const uint64& _player_idx );

    static BOOL PlayerGetJokboRecords( const uint64& player_idx , int& reads , PmNet::Ledger& records );

    static std::string InsertRecordsJokboQuery( const uint64_t& player_idx ,
                                                  const std::string& game_type ,
                                                  int game_type_num ,
                                                  const std::string& jokbo ,
                                                  int jokbo_num ,
                                                  int total_count ,
                                                  int win_count );

    static std::string UpdateRecordsJokboQuery( const uint64_t& player_idx ,
                                                  int game_type_num ,
                                                  int jokbo_num ,
                                                  int total_count ,
                                                  int win_count );

#pragma endregion JOKBO RECORD

#pragma region MailBox

    static void CreateTestMails( const uint64& player_idx );

    static std::string GenerateMailBoxInsertQuery(
        const uint64_t& player_idx ,
        const General::GrantItemKind& rewardType ,
        const General::InboxReason& mailType ,
        const uint64 count ,
        const int itemId ,
        const std::string& title ,
        const int period ,
        const std::string& expireDate,
        int mail_state =0);

    static std::string GetLastMailBoxIndexQuery( const uint64_t& player_idx );

    static std::string GenerateMailBoxDeleteQuery( const std::vector<uint64>& mailIdxList );

    static std::future<BOOL> MailBoxGetAsync( const uint64& player_idx , std::vector<PmNet::InboxDetail>& mail_list );
    static BOOL MailBoxGet( const uint64& player_idx , std::vector<PmNet::InboxDetail>& mail_list );

    static BOOL DeleteMailBox( std::vector<uint64> delete_list );

    static std::tuple<std::string, std::string, std::string, std::string, std::string> GetMessageData(const uint64& mail_idx);
    static std::vector<uint64> GetExpiredMessageList(const uint64& player_idx);

#pragma endregion MailBox

#pragma region QUEST

    static void CreateQuests( const uint64 playerIdx );

    //static void CreatePinballQuest( const uint64 playerIdx );

    static BOOL PlayerGetQuests( const uint64& player_idx , int& reads ,
        std::map<uint32 , General::TaskProgress>& missions ,
        std::map<uint32 , General::TaskProgress>& lounges ,
        std::map<uint32 , General::TaskProgress>& achieves );

    static std::string GeneratInsertQuest( uint64 playerIdx , int questId , General::TaskCategory achieveType , General::TaskTrigger achieveEventType , uint64 currentCount , uint64 goalCount );

    static std::string GenerateUpdateQuest( uint64_t playerIdx , int questId , uint64 currentCount );
    static std::string GenerateCompleteQuest( uint64_t playerIdx , int questId , BOOL reward_complete );
    static std::string GenerateResetQuest( uint64_t playerIdx , int questId );

    static std::string GetFriendRequestCount( const uint64& player_idx );
    static std::string GetFriendCount( const uint64& player_idx );

#pragma endregion QUEST

#pragma region FREE CHARGE

    static std::string GenerateFreeChargeUpdateQuery( const uint64& _player_idx , const General::AssetKind& _monty_type , const std::string& _refresh_time );

#pragma endregion FREE CHARGE

#pragma region BUY SHOP

    static BOOL GetPlayerGuids( const uint64& _player_idx , std::string& _account_guid , std::string& _platform_guid );
    static std::future<BOOL> GetPlayerGuidsAsync( const uint64& _player_idx , std::string& _account_guid , std::string& _platform_guid );

    static BOOL GetPlayerPlatform( const std::string& _account_guid , const std::string& _platform_guid , int& platform_code );
    static std::future<BOOL> GetPlayerPlatformAsync( const std::string& _account_guid , const std::string& _platform_guid , int& platform_code );

#pragma endregion BUY SHOP

#pragma region 손실한도

public:
    static BOOL UpdateLostLimit( General::LossLimitProfile* lost_limit );
    static std::string GenerateUpdateLostLimit( General::LossLimitProfile* lost_limit );

#pragma endregion 손실한도

#pragma region 출석부

    static BOOL UpdateAttendance( const uint64& _player_idx , const int& _days );
    static std::string GenerateUpdateAttendance( const uint64& _player_idx , const int& _days );

#pragma endregion 출석부

#pragma region 접속보상

    static BOOL GetLoginRewardList( std::vector<PmNet::DropDetail>& event_list );
    static std::future<BOOL> GetLoginRewardListAsync( std::vector<PmNet::DropDetail>& event_list );
    static BOOL GetLoginRewardByUserID( const int& user_id , std::set<string>& receive_list );
    static std::future<BOOL> GetLoginRewardByUserIDAsync( const int& user_id , std::set<string>& receive_list );
    static BOOL InsertLoginRewardLog( const uint64& user_id , const std::string& event_name , const std::string& reward , std::string& mail_index );

#pragma endregion 접속보상

#pragma region 친구

    static BOOL GetPlatformGuidByPlayerIdx( const uint64& friend_player_idx , std::string& platform_guid );              // player 테이블에서 friend_player_idx 로 platform_guid 를 가져온다.
    static std::future<BOOL> GetPlatformGuidByPlayerIdxAsync( const uint64& friend_player_idx , std::string& platform_guid );

    static BOOL GetFriends( const uint64& player_idx , std::vector<uint64>& friend_player_idx_list );                   // 플레이어의 친구 목록을 가져온다.
    static std::future<BOOL> GetFriendsAsync( const uint64& player_idx , std::vector<uint64>& friend_player_idx_list );

    static std::string GenerateInsertFriendQuery( uint64_t playerIdx , const std::string& friendPlatformGuid , uint64 friendPlayerIdx , int dbIndex );
    static std::future<BOOL> InsertFriendAsync( uint64_t playerIdx , const std::string& friendPlatformGuid , uint64 friendPlayerIdx , int dbIndex );

    static std::string GenerateUpdateFriendQuery( uint64_t playerIdx , const std::string& friendPlatformGuid , uint64 friendPlayerIdx , int dbIndex );


    static std::string GenerateDeleteFriendQuery( uint64 playerIdx , uint64 friendPlayerIdx );
    static std::future<BOOL> DeleteFriendsAsync( const uint64 player_idx , uint64 friendPlayerIdx );

    static BOOL GetPlayerByPlayerIdx( const uint64& player_idx , General::ParticipantProfile& _player , Server::ParticipantProfileInternal& _playerExt );
    static std::future<BOOL> GetPlayerByPlayerIdxAsync( const uint64& player_idx , General::ParticipantProfile& _player , Server::ParticipantProfileInternal& _playerExt );

    static BOOL PlayerGetByNickname( const std::string& nickname , std::vector<General::ParticipantProfile>& players );
    static std::future<BOOL> PlayerGetByNicknameAsync( const std::string& nickname , std::vector<General::ParticipantProfile>& players );

    //call PlayerGetByNickname( 'MADE' );

#pragma endregion 친구

#pragma region 점검

    static BOOL GetMaintenanceMessage( Server::MaintenanceMessage& message );
    static std::future<BOOL> GetMaintenanceMessageAsync( Server::MaintenanceMessage& message );

    static BOOL GetMaintenanceSystemMessage( Server::MaintenanceMessage& message );
    static std::future<BOOL> GetMaintenanceSystemMessageAsync( Server::MaintenanceMessage& message );

    static BOOL GetMaintenanceForcedMessage( Server::MaintenanceMessage& message );
    static std::future<BOOL> GetMaintenanceForcedMessageAsync( Server::MaintenanceMessage& message );

    static BOOL GetMaintenanceImageMessage( Server::MaintenanceMessage& message );
    static std::future<BOOL> GetMaintenanceImageMessageAsync( Server::MaintenanceMessage& message );
    static BOOL GetMaintenanceImageMessages( std::vector<Server::MaintenanceMessage>& messages );
    static std::future<BOOL> GetMaintenanceImageMessagesAsync( std::vector<Server::MaintenanceMessage>& messages );

    static BOOL GetCheatList( std::set<UINT64>& CList);
    static std::future<BOOL> GetCheatListAsync( std::set<UINT64>& CList );

    static BOOL GetBanList( std::set<UINT64>& BList );
    static std::future<BOOL> GetBanListAsync( std::set<UINT64>& BList );

    static BOOL GetNoticeMessageInfo( std::vector<PmNet::BulletinDetail>& notices );
    static BOOL GetSetVersions( std::vector<Server::Version>& versions );
    static std::future<BOOL> GetSetVersionsAsync( std::vector<Server::Version>& versions );

#pragma endregion 점검

#pragma region 로그

    static std::string GenerateInsertPaylog(
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& order_id ,
        const int& price ,
        const std::string& product_id ,
        const std::string& product_data ,
        const std::string& market ,
        const std::string& platform ,
        const std::string& purchase_date ,
        const std::string& request ,
        const std::string& response );

    static BOOL InsertPaylog(
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& order_id ,
        const int& price ,
        const std::string& product_id ,
        const std::string& product_data ,
        const std::string& market ,
        const std::string& platform ,
        const std::string& purchase_date ,
        const std::string& request ,
        const std::string& response );

    static unsigned int InsertPayCheck(
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& market ,
        const std::string& order_id ,
        const std::string& product_id);

    static std::future<BOOL> InsertLoginLog(
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& market ,
        const std::string& platform ,
        const std::string& MADE_id ,
        const std::string& login_type ,
        const std::string& nickname ,
        const uint64& exp ,
        const std::string& device_info ,
        const std::string& os ,
        const std::string& ip ,
        const std::string& withrow_reserved_date ,
        const std::string& is_withrow );

    static std::future<BOOL> MoneyLogInsert(
        const int event_type ,
        const int money_type ,
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& nickname ,
        const std::string& ip ,
        const uint64_t gain_amount ,
        const int64 spent_amount ,
        const int game_type ,
        const int slot_game_type ,
        const std::string& target_identifier );

    static std::future<BOOL> GamelogSlotInsert(
        const int slot_game_type ,
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& spin_code ,
        const int current_spin_type ,
        const int next_spin_type ,
        const std::string& spin_auto_count ,
        const uint64_t betting_amount_total ,
        const int grand_spin_multiple ,
        const int free_spin_status ,
        const uint64_t prev_amount ,
        const uint64_t change_amount ,
        const uint64_t current_amount ,
        const uint64_t earn_amount ,
        const std::string& ip );

    static std::future<BOOL> GamelogTableInsert(
        const int game_type ,
        const int money_type ,
        const std::string& channel_id ,
        const std::string& result_code ,
        const int betting_rule_type ,
        const std::string& lose_platform_guid ,
        const std::string& lose_ip ,
        const uint64_t lose_held_amount ,
        const int64_t lose_loss_amount ,
        const uint64_t lose_remain_amount ,
        const uint64_t lose_rake_amount ,
        const std::string& lose_jokbo ,
        const std::string& win_platform_guid ,
        const std::string& win_ip ,
        const uint64_t win_held_amount ,
        const int64_t win_get_amount ,
        const uint64_t win_remain_amount ,
        const uint64_t win_rake_amount ,
        const std::string& win_jokbo ,
        const std::string& kicked_platform_guid );

    static std::future<BOOL> GamelogTableStatisticInsert(
        const int game_type ,
        const int money_type ,
        const std::string& channel_id ,
        const int user_count ,
        const uint64_t win_money ,
        const uint64_t lose_money ,
        const uint64_t dealer_cost_normal ,
        const uint64_t dealer_cost_regular ,
        const uint64_t dealer_cost_top ,
        const std::string& member_player_index );

    static std::future<BOOL> InsertSlotLog(
            int code ,
            const std::string& uid ,
            const std::string& cmd ,
            const std::string& ipaddr ,
            int game_id ,
            const std::string& last_play_time ,
            const std::string& sub_game_key ,
            const std::string& next_sub_game ,
            const std::string& auto_spin , 
            int64_t slotcoin , 
            int64_t total_bet , 
            int64_t win_slotcoin ,
            int64_t discard_slotcoin ,
            int64_t bet_slotcoin ,
            int64_t buy_free_spins ,
            int64_t change_slotcoin ,
            int rakeback ,
            const std::string& slotrecord ,
            int spinprice ,
            const std::string& ver);


    // 2024.07.23 게임레코드 추가
    // Function for log code 20102
    static std::future<BOOL> InsertGameRecordBlackJackReset(
        const int code,
        const int game_type,
        const std::string& uid,
        const std::string& chnl,
        const int gamecnt,
        const std::string& logver,
        const std::string& gamerecord
    );

    // Function for log code 20104
    static std::future<BOOL> InsertGameRecordBlackJackRecord(
        const int code,
        const int game_type,
        const std::string& uid,
        const std::string& roomid,
        const int gamecnt,
        const std::string& logver,
        const std::string& gamerecord,
        const std::string& skey
    );

    // Function for log code 20202
    static std::future<BOOL> InsertGameRecordBaccaraReset(
         const int code ,
        const int game_type ,
        const std::string& uid ,
        const std::string& roomid ,
        const int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey
    );

    // Function for log code 20204
    static std::future<BOOL> InsertGameRecordBaccaraRecord(
        const int code ,
        const int game_type ,
        const std::string& uid ,
        const std::string& roomid ,
        const int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey
    );

    // Function for log code 20205
    /*static std::future<BOOL> InsertGameRecordRouletteRecord(
        const int code ,
        const int game_type ,
        const std::string& uid ,
        const std::string& channel ,
        const std::string& roomid ,
        const int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey
    );*/

    // Function for log code 20302
    static std::future<BOOL> InsertGameRecordLowBadukiRecord(
        const int code ,
        const int game_type ,
        const std::string& uid ,
        const std::string& roomid ,
        const int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey
    );

    // Function for log code 20402
    static std::future<BOOL> InsertGameRecordHoldemRecord(
        const int code ,
        const int game_type ,
        const std::string& uid ,
        const std::string& roomid ,
        const int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey
    );


    static std::future<BOOL> InsertSlotEventLog(
            int player_idx ,
            int mail_idx,
            const std::string& nickname ,
            const std::string& eventcode ,
            const std::string& reward
    );
    

#pragma endregion 로그

#pragma region 신로그
        // 신로그 시스템

    static std::future<BOOL> InsertAccountLog(
        //const std::string& logtm ,
        int code ,
        const std::string& uid ,
        const std::string& adminid ,
        const std::string& adid ,
        const std::string& asset ,
        const std::string& cmd ,
        const std::string& device ,
        const std::string& gmsessid ,
        const std::string& idpcode ,
        const std::string& ipaddr ,
        const std::string& osver ,
        const std::string& pf ,
        const std::string& svcuid ,
        const std::string& accountid ,
        int64_t chip ,
        int64_t chip_g ,
        int64_t chip_s ,
        int64_t coin ,
        int64_t coin_g ,
        int64_t coin_s ,
        int slotcoin ,
        int gem ,
        int gem_f ,
        int gem_p ,
        int kickoutticket ,
        int exp ,
        int friend_cnt ,
        const std::string& jointime ,
        const std::string& logintype ,
        const std::string& type ,
        const std::string& nickname ,
        const std::string& deletetime ,
        const std::string& registered ,
        int block_hour ,
        int left_count ,
        int64_t limit_type ,
        const std::string& reqtm ,
        const std::string& second_pw ,
        const std::string& reason ,
        const std::string& result ,
        const std::string& hash ,
        const std::string& ver ,
        const std::string& etc );

    static std::future<BOOL> InsertAssetLog(
        int code ,
        const std::string& uid ,
        const std::string& cmd ,
        const std::string& ipaddr ,
        const std::string& pf ,
        const std::string& device ,
        const std::string& idpcode ,
        const std::string& osver ,
        int64_t a_pgem ,
        int64_t a_fgem ,
        int64_t u_gem ,
        int64_t u_fgem ,
        int64_t u_pgem ,
        int64_t gem ,
        int64_t gem_f ,
        int64_t gem_p ,
        int64_t a_chip ,
        int64_t chip ,
        int64_t chip_g ,
        int64_t chip_s ,
        int64_t a_coin ,
        int64_t u_coin ,
        int64_t coin ,
        int64_t coin_g ,
        int64_t coin_s ,
        int kickoutticket ,
        int a_kickoutticket ,
        int64_t discard_chip ,
        int64_t discard_coin ,
        int price ,
        int refill_amount ,
        int refill_today ,
        const std::string& reward ,
        const std::string& rwd_avatar ,
        const std::string& rwd_membership ,
        int64_t amount ,
        const std::string& move ,
        int rakeback ,
        const std::string& rwd_data ,
        const std::string& rwd_list ,
        const std::string& rwd_type ,
        const std::string& ver ,
        const std::string& etc );

    static std::future<BOOL> InsertDailyLossBlock(
        int code ,
        const std::string& uid ,
        const std::string& datas ,
        const std::string& gmid ,
        const std::string& total
    );

    static std::future<BOOL> InsertGameHoldemLog(
            int code,                             // 코드
            const std::string& game_id,           // 게임 ID
            const std::string& channel,           // 채널
            int game_cnt,                         // 게임 횟수
            const std::string& log_ver,           // 로그 버전
            const std::string& game_opts,         // 게임 옵션
            const std::string& boss,              // 보스
            const std::string& host,              // 호스트
            // 플레이어 0
            const std::string& p0_id,             // 플레이어 0 ID
            int64_t p0_asset,                     // 플레이어 0 자산
            int p0_play_cnt,                      // 플레이어 0 플레이 횟수
            int64_t p0_change_coin,               // 플레이어 0 코인 변경
            int64_t p0_discard_coin,              // 플레이어 0 코인 삭제
            int64_t p0_fee_coin,                  // 플레이어 0 코인 수수료
            const std::string& p0_fee_info,       // 플레이어 0 수수료 정보
            int64_t p0_rakeback,                  // 플레이어 0 레이크백
            int64_t p0_coin,                      // 플레이어 0 코인
            const std::string& p0_result,         // 플레이어 0 결과
            // 플레이어 1
            const std::string& p1_id,             // 플레이어 1 ID
            int64_t p1_asset,                     // 플레이어 1 자산
            int p1_play_cnt,                      // 플레이어 1 플레이 횟수
            int64_t p1_change_coin,               // 플레이어 1 코인 변경
            int64_t p1_discard_coin,              // 플레이어 1 코인 삭제
            int64_t p1_fee_coin,                  // 플레이어 1 코인 수수료
            const std::string& p1_fee_info,       // 플레이어 1 수수료 정보
            int64_t p1_rakeback,                  // 플레이어 1 레이크백
            int64_t p1_coin,                      // 플레이어 1 코인
            const std::string& p1_result,         // 플레이어 1 결과
            // 플레이어 2
            const std::string& p2_id,             // 플레이어 2 ID
            int64_t p2_asset,                     // 플레이어 2 자산
            int p2_play_cnt,                      // 플레이어 2 플레이 횟수
            int64_t p2_change_coin,               // 플레이어 2 코인 변경
            int64_t p2_discard_coin,              // 플레이어 2 코인 삭제
            int64_t p2_fee_coin,                  // 플레이어 2 코인 수수료
            const std::string& p2_fee_info,       // 플레이어 2 수수료 정보
            int64_t p2_rakeback,                  // 플레이어 2 레이크백
            int64_t p2_coin,                      // 플레이어 2 코인
            const std::string& p2_result,         // 플레이어 2 결과
            // 플레이어 3
            const std::string& p3_id,             // 플레이어 3 ID
            int64_t p3_asset,                     // 플레이어 3 자산
            int p3_play_cnt,                      // 플레이어 3 플레이 횟수
            int64_t p3_change_coin,               // 플레이어 3 코인 변경
            int64_t p3_discard_coin,              // 플레이어 3 코인 삭제
            int64_t p3_fee_coin,                  // 플레이어 3 코인 수수료
            const std::string& p3_fee_info,       // 플레이어 3 수수료 정보
            int64_t p3_rakeback,                  // 플레이어 3 레이크백
            int64_t p3_coin,                      // 플레이어 3 코인
            const std::string& p3_result,         // 플레이어 3 결과
            // 플레이어 4
            const std::string& p4_id,             // 플레이어 4 ID
            int64_t p4_asset,                     // 플레이어 4 자산
            int p4_play_cnt,                      // 플레이어 4 플레이 횟수
            int64_t p4_change_coin,               // 플레이어 4 코인 변경
            int64_t p4_discard_coin,              // 플레이어 4 코인 삭제
            int64_t p4_fee_coin,                  // 플레이어 4 코인 수수료
            const std::string& p4_fee_info,       // 플레이어 4 수수료 정보
            int64_t p4_rakeback,                  // 플레이어 4 레이크백
            int64_t p4_coin,                      // 플레이어 4 코인
            const std::string& p4_result,         // 플레이어 4 결과
            // 플레이어 5
            const std::string& p5_id,             // 플레이어 5 ID
            int64_t p5_asset,                     // 플레이어 5 자산
            int p5_play_cnt,                      // 플레이어 5 플레이 횟수
            int64_t p5_change_coin,               // 플레이어 5 코인 변경
            int64_t p5_discard_coin,              // 플레이어 5 코인 삭제
            int64_t p5_fee_coin,                  // 플레이어 5 코인 수수료
            const std::string& p5_fee_info,       // 플레이어 5 수수료 정보
            int64_t p5_rakeback,                  // 플레이어 5 레이크백
            int64_t p5_coin,                      // 플레이어 5 코인
            const std::string& p5_result,         // 플레이어 5 결과
            // 플레이어 6
            const std::string& p6_id,             // 플레이어 6 ID
            int64_t p6_asset,                     // 플레이어 6 자산
            int p6_play_cnt,                      // 플레이어 6 플레이 횟수
            int64_t p6_change_coin,               // 플레이어 6 코인 변경
            int64_t p6_discard_coin,              // 플레이어 6 코인 삭제
            int64_t p6_fee_coin,                  // 플레이어 6 코인 수수료
            const std::string& p6_fee_info,       // 플레이어 6 수수료 정보
            int64_t p6_rakeback,                  // 플레이어 6 레이크백
            int64_t p6_coin,                      // 플레이어 6 코인
            const std::string& p6_result,         // 플레이어 6 결과
            // 플레이어 7
            const std::string& p7_id,             // 플레이어 7 ID
            int64_t p7_asset,                     // 플레이어 7 자산
            int p7_play_cnt,                      // 플레이어 7 플레이 횟수
            int64_t p7_change_coin,               // 플레이어 7 코인 변경
            int64_t p7_discard_coin,              // 플레이어 7 코인 삭제
            int64_t p7_fee_coin,                  // 플레이어 7 코인 수수료
            const std::string& p7_fee_info,       // 플레이어 7 수수료 정보
            int64_t p7_rakeback,                  // 플레이어 7 레이크백
            int64_t p7_coin,                      // 플레이어 7 코인
            const std::string& p7_result,         // 플레이어 7 결과
            // 플레이어 8
            const std::string& p8_id,             // 플레이어 8 ID
            int64_t p8_asset,                     // 플레이어 8 자산
            int p8_play_cnt,                      // 플레이어 8 플레이 횟수
            int64_t p8_change_coin,               // 플레이어 8 코인 변경
            int64_t p8_discard_coin,              // 플레이어 8 코인 삭제
            int64_t p8_fee_coin,                  // 플레이어 8 코인 수수료
            const std::string& p8_fee_info,       // 플레이어 8 수수료 정보
            int64_t p8_rakeback,                  // 플레이어 8 레이크백
            int64_t p8_coin,                      // 플레이어 8 코인
            const std::string& p8_result,         // 플레이어 8 결과
            // 추가 데이터
            const std::string& skey,              // 키
            int play_time,                        // 플레이 시간
            const std::string& room_id,           // 방 ID
            const std::string& win0_id,           // 승리자 0 ID
            int64_t win0_asset,                   // 승리자 0 자산
            const std::string& win1_id,           // 승리자 1 ID
            int64_t win1_asset,                   // 승리자 1 자산
            const std::string& win2_id,           // 승리자 2 ID
            int64_t win2_asset,                   // 승리자 2 자산
            const std::string& win3_id,           // 승리자 3 ID
            int64_t win3_asset,                   // 승리자 3 자산
            const std::string& win4_id,           // 승리자 4 ID
            int64_t win4_asset,                   // 승리자 4 자산
            const std::string& win5_id,           // 승리자 5 ID
            int64_t win5_asset,                   // 승리자 5 자산
            const std::string& win6_id,           // 승리자 6 ID
            int64_t win6_asset,                   // 승리자 6 자산
            const std::string& win7_id,           // 승리자 7 ID
            int64_t win7_asset,                   // 승리자 7 자산
            const std::string& win8_id,           // 승리자 8 ID
            int64_t win8_asset                    // 승리자 8 자산
    );

    static std::future<BOOL> InsertGameLowbadugiLog(
        int code,                             // 코드
        const std::string& game_id,           // 게임 ID
        const std::string& channel,           // 채널
        int game_cnt,                         // 게임 횟수
        const std::string& log_ver,           // 로그 버전
        const std::string& game_opts,         // 게임 옵션
        const std::string& boss,              // 보스
        const std::string& host,              // 호스트
        // 플레이어 0
        const std::string& p0_id,
        int64_t p0_asset,
        int p0_play_cnt,
        int64_t p0_change_chip,
        int64_t p0_change_coin,
        int64_t p0_discard_chip,
        int64_t p0_discard_coin,
        int64_t p0_fee_chip,
        int64_t p0_fee_coin,
        const std::string& p0_fee_info,
        int64_t p0_rakeback,
        int64_t p0_today_chip,
        int64_t p0_chip,
        int64_t p0_coin,
        const std::string& p0_result,
        // 플레이어 1
        const std::string& p1_id,
        int64_t p1_asset,
        int p1_play_cnt,
        int64_t p1_change_chip,
        int64_t p1_change_coin,
        int64_t p1_discard_chip,
        int64_t p1_discard_coin,
        int64_t p1_fee_chip,
        int64_t p1_fee_coin,
        const std::string& p1_fee_info,
        int64_t p1_rakeback,
        int64_t p1_today_chip,
        int64_t p1_chip,
        int64_t p1_coin,
        const std::string& p1_result,
        // 플레이어 2
        const std::string& p2_id,
        int64_t p2_asset,
        int p2_play_cnt,
        int64_t p2_change_chip,
        int64_t p2_change_coin,
        int64_t p2_discard_chip,
        int64_t p2_discard_coin,
        int64_t p2_fee_chip,
        int64_t p2_fee_coin,
        const std::string& p2_fee_info,
        int64_t p2_rakeback,
        int64_t p2_today_chip,
        int64_t p2_chip,
        int64_t p2_coin,
        const std::string& p2_result,
        // 플레이어 3
        const std::string& p3_id,
        int64_t p3_asset,
        int p3_play_cnt,
        int64_t p3_change_chip,
        int64_t p3_change_coin,
        int64_t p3_discard_chip,
        int64_t p3_discard_coin,
        int64_t p3_fee_chip,
        int64_t p3_fee_coin,
        const std::string& p3_fee_info,
        int64_t p3_rakeback,
        int64_t p3_today_chip,
        int64_t p3_chip,
        int64_t p3_coin,
        const std::string& p3_result,
        // 플레이어 4
        const std::string& p4_id,
        int64_t p4_asset,
        int p4_play_cnt,
        int64_t p4_change_chip,
        int64_t p4_change_coin,
        int64_t p4_discard_chip,
        int64_t p4_discard_coin,
        int64_t p4_fee_chip,
        int64_t p4_fee_coin,
        const std::string& p4_fee_info,
        int64_t p4_rakeback,
        int64_t p4_today_chip,
        int64_t p4_chip,
        int64_t p4_coin,
        const std::string& p4_result,
        // 추가 데이터
        const std::string& skey,
        int play_time,
        const std::string& room_id,
        const std::string& win0_id,
        int64_t win0_asset,
        const std::string& win1_id,
        int64_t win1_asset,
        const std::string& win2_id,
        int64_t win2_asset,
        const std::string& win3_id,
        int64_t win3_asset,
        const std::string& win4_id,
        int64_t win4_asset
    );

    static std::future<BOOL> InsertGameBaccaraLog(
        int code,                             // 코드
        const std::string& game_id,           // 게임 ID
        const std::string& channel,           // 채널
        int game_cnt,                         // 게임 횟수
        const std::string& log_ver,           // 로그 버전
        // 플레이어 0
        const std::string& p0_id,
        int64_t p0_change_coin,
        int64_t p0_discard_coin,
        int64_t p0_rakeback,
        int64_t p0_coin,
        const std::string& p0_result,
        // 플레이어 1
        const std::string& p1_id,
        int64_t p1_change_coin,
        int64_t p1_discard_coin,
        int64_t p1_rakeback,
        int64_t p1_coin,
        const std::string& p1_result,
        // 플레이어 2
        const std::string& p2_id,
        int64_t p2_change_coin,
        int64_t p2_discard_coin,
        int64_t p2_rakeback,
        int64_t p2_coin,
        const std::string& p2_result,
        // 플레이어 3
        const std::string& p3_id,
        int64_t p3_change_coin,
        int64_t p3_discard_coin,
        int64_t p3_rakeback,
        int64_t p3_coin,
        const std::string& p3_result,
        // 플레이어 4
        const std::string& p4_id,
        int64_t p4_change_coin,
        int64_t p4_discard_coin,
        int64_t p4_rakeback,
        int64_t p4_coin,
        const std::string& p4_result,
        // 플레이어 5
        const std::string& p5_id,
        int64_t p5_change_coin,
        int64_t p5_discard_coin,
        int64_t p5_rakeback,
        int64_t p5_coin,
        const std::string& p5_result,
        // 플레이어 6
        const std::string& p6_id,
        int64_t p6_change_coin,
        int64_t p6_discard_coin,
        int64_t p6_rakeback,
        int64_t p6_coin,
        const std::string& p6_result,
        // 추가 데이터
        const std::string& skey, // 플레이어 UID
        int play_time,
        const std::string& room_id
    );
    //static std::future<BOOL> InsertGameRouletteLog(
    //  int code ,                             // 코드
    //  const std::string& game_id ,           // 게임 ID
    //  const std::string& channel ,           // 채널
    //  int game_cnt ,                         // 게임 횟수
    //  const std::string& log_ver ,           // 로그 버전
    //  // 플레이어 0
    //  const std::string& p0_id ,
    //  int64_t p0_change_coin ,
    //  int64_t p0_discard_coin ,
    //  int64_t p0_rakeback ,
    //  int64_t p0_coin ,
    //  const std::string& p0_result ,
    //  // 플레이어 1
    //  const std::string& p1_id ,
    //  int64_t p1_change_coin ,
    //  int64_t p1_discard_coin ,
    //  int64_t p1_rakeback ,
    //  int64_t p1_coin ,
    //  const std::string& p1_result ,
    //  // 플레이어 2
    //  const std::string& p2_id ,
    //  int64_t p2_change_coin ,
    //  int64_t p2_discard_coin ,
    //  int64_t p2_rakeback ,
    //  int64_t p2_coin ,
    //  const std::string& p2_result ,
    //  // 플레이어 3
    //  const std::string& p3_id ,
    //  int64_t p3_change_coin ,
    //  int64_t p3_discard_coin ,
    //  int64_t p3_rakeback ,
    //  int64_t p3_coin ,
    //  const std::string& p3_result ,
    //  // 추가 데이터
    //  const std::string& skey , // 플레이어 UID
    //  int play_time ,
    //  const std::string& room_id
    //);


    static std::future<BOOL> InsertGameBlackJackLog(
        int code ,                             // 코드
        const std::string& game_id ,           // 게임 ID
        const std::string& channel ,           // 채널
        int game_cnt ,                         // 게임 횟수
        const std::string& log_ver ,           // 로그 버전
        const std::string& game_opts ,         // 게임 옵션

        const std::string& p0_id ,              // 플레이어0 UID 
        int64_t p0_asset ,                      // 플레이어0 소지재화
        int64_t p0_change_coin ,                // 플레이어0 코인 변동량
        int64_t p0_discard_coin ,               // 최대 소지금액 이상 획득했을경우 버려지는 코인
        int64_t p0_rakeback ,                   // 플레이어0의 적립액
        int64_t p0_coin ,                       // 플레이어0의 해당 방 변동 코인
        int p0_playcnt,                         // 플레이어0의 해당 방 진행 횟수
        const std::string& p0_betting,          // 플레이어0의 블랙잭 베팅 내용
        const std::string& p0_records,          // 플레이어0의 블랙잭 베팅 순서
        const std::string& p0_result,           // 플레이어0의 베팅 결과

        const std::string& p1_id ,      
        int64_t p1_asset ,              
        int64_t p1_change_coin ,        
        int64_t p1_discard_coin ,       
        int64_t p1_rakeback ,           
        int64_t p1_coin ,               
        int p1_playcnt ,                
        const std::string& p1_betting , 
        const std::string& p1_records , 
        const std::string& p1_result ,  

        const std::string& p2_id ,
        int64_t p2_asset ,
        int64_t p2_change_coin ,
        int64_t p2_discard_coin ,
        int64_t p2_rakeback ,
        int64_t p2_coin ,
        int p2_playcnt ,
        const std::string& p2_betting ,
        const std::string& p2_records ,
        const std::string& p2_result ,

        const std::string& p3_id ,
        int64_t p3_asset ,
        int64_t p3_change_coin ,
        int64_t p3_discard_coin ,
        int64_t p3_rakeback ,
        int64_t p3_coin ,
        int p3_playcnt ,
        const std::string& p3_betting ,
        const std::string& p3_records ,
        const std::string& p3_result ,

        const std::string& p4_id ,
        int64_t p4_asset ,
        int64_t p4_change_coin ,
        int64_t p4_discard_coin ,
        int64_t p4_rakeback ,
        int64_t p4_coin ,
        int p4_playcnt ,
        const std::string& p4_betting ,
        const std::string& p4_records ,
        const std::string& p4_result ,

        const std::string& skey ,               // 참여한 유저 UID
        int play_time ,                         // 플레이시간
        const std::string& room_id              // 방번호
    );
    
    static std::future<BOOL> InsertGameBaccaraBettingLog(
    int code,                             // 코드
    const std::string& game_id,           // 게임 ID
    const std::string& channel,           // 채널
    int game_cnt,                         // 게임 횟수
    const std::string& log_ver,           // 로그 버전
    const std::string& game_opts,         // 게임 옵션
    // 플레이어 0
    const std::string& p0_id,
    int64_t p0_asset,
    int p0_play_cnt,
    const std::string& p0_betting,
    const std::string& p0_records,
    // 플레이어 1
    const std::string& p1_id,
    int64_t p1_asset,
    int p1_play_cnt,
    const std::string& p1_betting,
    const std::string& p1_records,
    // 플레이어 2
    const std::string& p2_id,
    int64_t p2_asset,
    int p2_play_cnt,
    const std::string& p2_betting,
    const std::string& p2_records,
    // 플레이어 3
    const std::string& p3_id,
    int64_t p3_asset,
    int p3_play_cnt,
    const std::string& p3_betting,
    const std::string& p3_records,
    // 플레이어 4
    const std::string& p4_id,
    int64_t p4_asset,
    int p4_play_cnt,
    const std::string& p4_betting,
    const std::string& p4_records,
    // 플레이어 5
    const std::string& p5_id,
    int64_t p5_asset,
    int p5_play_cnt,
    const std::string& p5_betting,
    const std::string& p5_records,
    // 플레이어 6
    const std::string& p6_id,
    int64_t p6_asset,
    int p6_play_cnt,
    const std::string& p6_betting,
    const std::string& p6_records,
    // 추가 데이터
    const std::string& skey,
    const std::string& room_id
);

//static std::future<BOOL> InsertGameRouletteBettingLog(
//   int code ,                             // 코드
//   const std::string& game_id ,           // 게임 ID
//   const std::string& channel ,           // 채널
//   int game_cnt ,                         // 게임 횟수
//   const std::string& log_ver ,           // 로그 버전
//   const std::string& game_opts ,         // 게임 옵션
//   // 플레이어 0
//   const std::string& p0_id ,
//   int64_t p0_asset ,
//   int p0_play_cnt ,
//   const std::string& p0_betting ,
//   const std::string& p0_records ,
//   // 플레이어 1
//   const std::string& p1_id ,
//   int64_t p1_asset ,
//   int p1_play_cnt ,
//   const std::string& p1_betting ,
//   const std::string& p1_records ,
//   // 플레이어 2
//   const std::string& p2_id ,
//   int64_t p2_asset ,
//   int p2_play_cnt ,
//   const std::string& p2_betting ,
//   const std::string& p2_records ,
//   // 플레이어 3
//   const std::string& p3_id ,
//   int64_t p3_asset ,
//   int p3_play_cnt ,
//   const std::string& p3_betting ,
//   const std::string& p3_records ,
//   // 추가 데이터
//   const std::string& skey ,
//   const std::string& room_id
//    );


    static std::future<BOOL> InsertGameExitLog(
        int code ,
        const std::string& channel ,
        const std::string& game_id ,
        int game_cnt ,
        const std::string& log_ver ,
        const std::string& host ,
        const std::string& p0_id ,
        int64_t p0_asset ,
        int64_t p0_today_chip ,
        const std::string& p1_id ,
        int64_t p1_asset ,
        int64_t p1_today_chip ,
        const std::string& p2_id ,
        int64_t p2_asset ,
        int64_t p2_today_chip ,
        const std::string& p3_id ,
        int64_t p3_asset ,
        int64_t p3_today_chip ,
        const std::string& p4_id ,
        int64_t p4_asset ,
        int64_t p4_today_chip ,
        const std::string& p5_id ,
        int64_t p5_asset ,
        const std::string& p6_id ,
        int64_t p6_asset ,
        const std::string& p7_id ,
        int64_t p7_asset ,
        const std::string& p8_id ,
        int64_t p8_asset ,
        const std::string& skey ,
        const std::string& room_id ,
        const std::string& reason ,
        const std::string& state
    );

    static std::future<BOOL> InsertGameKickoutLog(
        int code,                             // 코드
        const std::string& game_id,           // 게임 ID
        const std::string& channel,           // 채널
        int game_cnt,                         // 게임 횟수
        const std::string& log_ver,           // 로그 버전
        const std::string& host,              // 호스트
        int kickout_ticket,                   // 퇴장 티켓 수
        int kickout_count,                    // 퇴장 횟수
        const std::string& skey,              // SKEY
        const std::string& target,            // 타겟
        const std::string& room_id            // 방 ID
    );

    static std::future<BOOL> InsertAdsLog(
        int code ,
        const std::string& uid ,
        const std::string& cmd ,
        const std::string& ipaddr ,
        const std::string& type ,
        const std::string& ver );

    static std::future<BOOL> InsertSocialLog(
        int code ,
        const std::string& uid ,
        const std::string& cmd ,
        const std::string& ipaddr ,
        const std::string& friend_id ,
        const std::string& logdata ,
        const std::string& ver );

    static std::future<BOOL> InsertMessageLog(
        int code ,
        const std::string& uid ,
        const std::string& cmd ,
        const std::string& ipaddr ,
        const std::string& data ,
        const std::string& msgid ,
        int msgtype ,
        const std::string& reward ,
        const std::string& sender ,
        const std::string& ver ,
        const std::string& etc );

    static std::future<BOOL> InsertGameConcurrentUsersLog(
        const int online_user ,
        const int lowbaduiki_play_user_cip ,
        const int lowbaduiki_play_user_coin ,
        const int lowbaduiki_play_user_friend ,
        const int holdem_play_user ,
        const int bakara_play_user ,
        const int blackjack_play_user ,
        const int slot_play_user 
    //    const int roulette_play_user 
    );

    /*static std::future<BOOL> PinballLogInsert(
        const int code ,
		const std::string& account_guid ,
		const std::string& platform_guid ,
		const std::string& nickname ,
		const std::string& ver ,
	    const std::string& lastplaytime ,
        const int gamecnt ,
		const std::string& ip ,
		const Common::PinballType game_type ,
		const std::string& regame ,
		const std::string& lucky_num ,
		const std::string& bingo_board ,
		const int bingo_count ,
		const std::string& bingo_multi ,
        const std::string& result ,
        const uint64_t coin ,
        const uint64_t bet_coin ,
        const int64_t change_coin ,
        const int64_t win_coin ,
        const int64_t discard_coin ,
        const int rakeback,
        const uint64 total_coin );*/

    static BOOL GetPlayerJoinTime( const std::string& _account_guid , const std::string& _platform_guid , string& _join_time );
    static std::future<BOOL> GetPlayerJoinTimeAsync( const std::string& _account_guid , const std::string& _platform_guid , string& _join_time );

    static std::future<BOOL> InsertErrorLog( const std::string& _error);

    static std::future<BOOL> InsertAbusingLog( const std::string& _error);


    static std::future<BOOL> InsertSlotEventLog(const int event_type, const std::string& start_date, const std::string& end_date, const int rank, const uint64 player_index, const uint64 score);

    static std::future<BOOL> InsertSlotEventLog(std::ostringstream& query);

    static std::future<BOOL> PingSQL();

    static BOOL GetPlayerIP( const uint64 player_index , std::string& IP );
    static std::future<BOOL> GetPlayerIPAsync( const uint64 player_index , std::string& IP );

    static std::future<BOOL> InsertPlayerIPLog( const uint64 player_index , std::string& IP );


private:
    string m_errorString;

public:
    static std::future<BOOL> InsertSlotConcurrentUsersLog( const std::map<std::string , int>& channelUserCount );
#pragma endregion

#pragma region SlotRewardEvent
    static std::future<BOOL> GetSlotRewardEventAsync( std::vector<PmNet::ReelBountyDrop>& event_list );
    static BOOL GetSlotRewardEventList( std::vector<PmNet::ReelBountyDrop>& event_list );
    static std::future<BOOL> GetSlotLastRewardEventAsync( std::vector<PmNet::ReelBountyDrop>& event_list );
    static BOOL GetSlotLastRewardEventList( std::vector<PmNet::ReelBountyDrop>& event_list );
#pragma endregion
    static BOOL GetSystemData( std::map<std::string , std::string>& data );
    static std::future<BOOL> GetSystemDataAsync( std::map<std::string , std::string>& data );
    /*static std::future<BOOL> GetPInballRatioAsync( std::map<int , int>& ratio );
    static BOOL GetPinballRatio( std::map<int , int>& ratio );
    static std::string GetPinballUpdateDate( const uint64& player_idx );*/


};