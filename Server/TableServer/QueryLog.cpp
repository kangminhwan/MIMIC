#include "Query.h"

#include "./mysql/cMySQL.h"
#include "./mysql/cMySQLParserElement.h"
#include "./mysql/cMySQLReader.h"

#include "./mysql/cMySQLConnectionPooler.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Manager/ServerManager.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cProtoUtil.h"
#include "TimeUtils.h"
#include "StringUtil.h"

#include <iostream>
#include <format>


std::string QueryManager::GenerateInsertPaylog(
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
        const std::string& response )
{
    // 쿼리 문자열 생성
    std::ostringstream oss;
    oss << "INSERT INTO paylog (account_guid, platform_guid, order_id, price, product_id, product_data, market, platform, purchase_date, request, response) VALUES ("
        << "'" << account_guid << "', "
        << "'" << platform_guid << "', "
        << "'" << order_id << "', "
        << std::to_string( price ) << ", "
        << "'" << product_id << "', "
        << "'" << product_data << "', "
        << "'" << market << "', "
        << "'" << platform << "', "
        << "'" << purchase_date << "', "
        << "'" << request << "', "
        << "'" << response << "')";
    return oss.str();
}

BOOL QueryManager::InsertPaylog(
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
        const std::string& response )
{
    std::string query = GenerateInsertPaylog( account_guid ,
        platform_guid ,
        order_id ,
        price ,
        product_id ,
        product_data ,
        market ,
        platform ,
        purchase_date ,
        request ,
        response );

    std::future<BOOL> result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
    result.wait();

    if ( FALSE == result.get() )
    {
        return FALSE;
    }
    return TRUE;
}

unsigned int QueryManager::InsertPayCheck(
        const std::string& account_guid ,
        const std::string& platform_guid ,
        const std::string& market ,
        const std::string& order_id ,
        const std::string& product_id )
{
    std::ostringstream oss;
    oss << "INSERT INTO paycheck (account_guid, platform_guid, order_id, product_id, market ) VALUES ("
        << "'" << account_guid << "', "
        << "'" << platform_guid << "', "
        << "'" << order_id << "', "
        << "'" << product_id << "', "
        << "'" << market << "')";
    std::string query =  oss.str();

    std::future<unsigned int> result = PlayerExecuteQueryWithErrorCodeAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );
    result.wait();
    return result.get();
}

std::future<BOOL> QueryManager::InsertLoginLog(
    const std::string& account_guid ,
    const std::string& platform_guid ,
    const std::string& market ,
    const std::string& platform ,
    const std::string& IVE_id ,
    const std::string& login_type ,
    const std::string& nickname ,
    const uint64& exp ,
    const std::string& device_info ,
    const std::string& os ,
    const std::string& ip ,
    const std::string& withrow_reserved_date ,
    const std::string& is_withrow )
{
    std::ostringstream oss;

    // INSERT INTO query generation
    oss << "INSERT INTO loginlog ";
    oss << "(";
    oss << "`account_guid`, `platform_guid`, `market`, `platform`, `IVE_id`, ";
    oss << "`login_type`, `nickname`, `exp`, `device_info`, `os`, `ip`, ";
    oss << "`withrow_reserved_date`, `is_withrow` ";
    oss << ") ";

    // VALUES part generation
    oss << "VALUES ";
    oss << "(";
    oss << "'" << account_guid << "', ";
    oss << "'" << platform_guid << "', ";
    oss << "'" << market << "', ";
    oss << "'" << platform << "', ";
    oss << "'" << IVE_id << "', ";
    oss << "'" << login_type << "', ";
    oss << "'" << nickname << "', ";
    oss << exp << ", ";
    oss << "'" << device_info << "', ";
    oss << "'" << os << "', ";
    oss << "'" << ip << "', ";
    if ( withrow_reserved_date.empty() ) {
        oss << "NULL, ";
    }
    else {
        oss << "'" << withrow_reserved_date << "', ";
    }
    oss << "'" << is_withrow << "' ";
    oss << ");";

    string query = oss.str();
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
}

std::future<BOOL> QueryManager::MoneyLogInsert(
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
    const std::string& target_identifier )
{
    std::ostringstream oss;
    oss << "INSERT INTO moneylog ("
        << "event_type, money_type, account_guid, platform_guid, nickname, ip, "
        << "gain_amount, spent_amount, game_type, slot_game_type, target_identifier) "
        << "VALUES ("
        << event_type << ", "
        << money_type << ", '"
        << account_guid << "', '"
        << platform_guid << "', '"
        << nickname << "', '"
        << ip << "', "
        << gain_amount << ", "
        << spent_amount << ", "
        << game_type << ", "
        << slot_game_type << ", '"
        << target_identifier << "');";

    string query = oss.str();
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
}

std::future<BOOL> QueryManager::GamelogSlotInsert(
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
    const std::string& ip )
{
    std::ostringstream oss;
    oss << "INSERT INTO gamelog_slot ("
        << "slot_game_type, account_guid, platform_guid, spin_code, current_spin_type, next_spin_type, "
        << "spin_auto_count, betting_amount_total, grand_spin_multiple, free_spin_status, prev_amount, "
        << "change_amount, current_amount, earn_amount, ip) VALUES ("
        << slot_game_type << ", '"
        << account_guid << "', '"
        << platform_guid << "', '"
        << spin_code << "', "
        << current_spin_type << ", "
        << next_spin_type << ", '"
        << spin_auto_count << "', "
        << betting_amount_total << ", "
        << grand_spin_multiple << ", "
        << free_spin_status << ", "
        << prev_amount << ", "
        << change_amount << ", "
        << current_amount << ", "
        << earn_amount << ", '"
        << ip << "');";

    string query = oss.str();
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
}

std::future<BOOL> QueryManager::GamelogTableInsert(
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
    const std::string& kicked_platform_guid )
{
    std::ostringstream oss;
    oss << "INSERT INTO gamelog_table ("
        << "game_type, money_type, channel_id, result_code, betting_rule_type, "
        << "lose_platform_guid, lose_ip, lose_held_amount, lose_loss_amount, lose_remain_amount, lose_rake_amount, lose_jokbo, "
        << "win_platform_guid, win_ip, win_held_amount, win_get_amount, win_remain_amount, win_rake_amount, win_jokbo, kicked_platform_guid) VALUES ("
        << game_type << ", "
        << money_type << ", '"
        << channel_id << "', '"
        << result_code << "', "
        << betting_rule_type << ", '"
        << lose_platform_guid << "', '"
        << lose_ip << "', "
        << lose_held_amount << ", "
        << lose_loss_amount << ", "
        << lose_remain_amount << ", "
        << lose_rake_amount << ", '"
        << lose_jokbo << "', '"
        << win_platform_guid << "', '"
        << win_ip << "', "
        << win_held_amount << ", "
        << win_get_amount << ", "
        << win_remain_amount << ", "
        << win_rake_amount << ", '"
        << win_jokbo << "', '"
        << kicked_platform_guid << "');";

    string query = oss.str();
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
}

std::future<BOOL> QueryManager::GamelogTableStatisticInsert(
    const int game_type ,
    const int money_type ,
    const std::string& channel_id ,
    const int user_count ,
    const uint64_t win_money ,
    const uint64_t lose_money ,
    const uint64_t dealer_cost_normal ,
    const uint64_t dealer_cost_regular ,
    const uint64_t dealer_cost_top ,
    const std::string& member_player_index )
{
    std::ostringstream oss;
    oss << "INSERT INTO gamelog_table_statistic ("
        << "game_type, money_type, channel_id, user_count, win_money, lose_money, "
        << "dealer_cost_normal, dealer_cost_regular, dealer_cost_top, player_index_string) VALUES ("
        << game_type << ", "
        << money_type << ", '"
        << channel_id << "', "
        << user_count << ", "
        << win_money << ", "
        << lose_money << ", "
        << dealer_cost_normal << ", "
        << dealer_cost_regular << ", "
        << dealer_cost_top << ", '"
        << member_player_index << "');";

    string query = oss.str();
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
}

std::future<BOOL> QueryManager::InsertSlotLog(
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
        const std::string& ver)
{
    std::ostringstream query;
    query << "INSERT INTO slot_log (CODE, UID, CMD, IP_ADDR, SLOT_GAME_TYPE, "
        << "LAST_PLAY_TIME, SUB_GAME_KEY, NEXT_SUB_GAME, AUTO_SPIN, SLOT_COIN, TOTAL_BET, WIN_SLOTCOIN, DISCARD_SLOTCOIN, "
        << "BET_SLOTCOIN, BUY_FREE_SPINS, CHANGE_SLOTCOIN, RAKEBACK, SLOT_RECORD, SPIN_PRICE, VER) VALUES ("
        //<< "'" << logtm << "', "
        << code << ", "
        << "'" << uid << "', "
        << "'" << cmd << "', "
        << "'" << ipaddr << "', "
        << game_id << ", "
        << "'" << last_play_time << "', "
        << "'" << sub_game_key << "', "
        << "'" << next_sub_game << "', "
        << "'" << auto_spin << "', "
        << slotcoin << ", "
        << total_bet << ", "
        << win_slotcoin << ", "
        << discard_slotcoin << ", "
        << bet_slotcoin << ", "
        << buy_free_spins << ", "
        << change_slotcoin << ", "
        << rakeback << ", "
        << "'" << slotrecord << "', "
        << spinprice << ", "
        << "'" << ver << "') ";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}


// 2024.07.23 게임레코드 추가
// Function for log code 20102
std::future<BOOL> QueryManager::InsertGameRecordBlackJackReset(
        int code,
        int game_type,
        const std::string& uid,
        const std::string& chnl,
        int gamecnt,
        const std::string& logver,
        const std::string& gamerecord
) 
{
    std::ostringstream query;
    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, CHANNEL, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
          << code << ", "
          << game_type << ", "
          << "'" << uid << "', "
          << "'" << chnl << "', "
          << gamecnt << ", "
          << "'" << logver << "', "
          << "'" << gamerecord << "', "
          << "'" << "" << "')";
    
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

// Function for log code 20104
std::future<BOOL> QueryManager::InsertGameRecordBlackJackRecord(
        int code,
        int game_type,
        const std::string& uid,
        const std::string& roomid,
        int gamecnt,
        const std::string& logver,
        const std::string& gamerecord,
        const std::string& skey
) 
{
    std::ostringstream query;
    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, ROOM_ID, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
          << code << ", "
          << game_type << ", "
          << "'" << uid << "', "
          << "'" << roomid << "', "
          << gamecnt << ", "
          << "'" << logver << "', "
          << "'" << gamerecord << "', "
          << "'" << skey << "')";
    
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

// Function for log code 20202
std::future<BOOL> QueryManager::InsertGameRecordBaccaraReset(
        int code,
        int game_type,
        const std::string& uid,
        const std::string& chnl,
        int gamecnt,
        const std::string& logver,
        const std::string& gamerecord,
        const std::string& skey
) 
{
    std::ostringstream query;
    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, CHANNEL, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
          << code << ", "
          << game_type << ", "
          << "'" << uid << "', "
          << "'" << chnl << "', "
          << gamecnt << ", "
          << "'" << logver << "', "
          << "'" << gamerecord << "', "
          << "'" << skey << "')";
    
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

// Function for log code 20204
std::future<BOOL> QueryManager::InsertGameRecordBaccaraRecord(
               int code ,
        int game_type ,
        const std::string& uid ,
        const std::string& chnl ,
        int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey
) 
{
    std::ostringstream query;
    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, CHANNEL, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
        << code << ", "
        << game_type << ", "
        << "'" << uid << "', "
        << "'" << chnl << "', "
        << gamecnt << ", "
        << "'" << logver << "', "
        << "'" << gamerecord << "', "
        << "'" << skey << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

// Function for log code 20205
//std::future<BOOL> QueryManager::InsertGameRecordRouletteRecord(
//               int code ,
//        int game_type ,
//        const std::string& uid ,
//        const std::string& chnl ,
//        const std::string& roomid ,
//        int gamecnt ,
//        const std::string& logver ,
//        const std::string& gamerecord ,
//        const std::string& skey
//)
//{
//    std::ostringstream query;
//    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, CHANNEL,ROOM_ID, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
//        << code << ", "
//        << game_type << ", "
//        << "'" << uid << "', "
//        << "'" << chnl << "', "
//        << "'" << roomid << "', "
//        << gamecnt << ", "
//        << "'" << logver << "', "
//        << "'" << gamerecord << "', "
//        << "'" << skey << "')";
//
//    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
//}
// 
// Function for log code 20302
std::future<BOOL> QueryManager::InsertGameRecordLowBadukiRecord(
        int code ,
        int game_type ,
        const std::string& uid ,
        const std::string& chnl ,
        int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey) 
{
    std::ostringstream query;
    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, CHANNEL, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
        << code << ", "
        << game_type << ", "
        << "'" << uid << "', "
        << "'" << chnl << "', "
        << gamecnt << ", "
        << "'" << logver << "', "
        << "'" << gamerecord << "', "
        << "'" << skey << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

// Function for log code 20402
std::future<BOOL> QueryManager::InsertGameRecordHoldemRecord(
        int code ,
        int game_type ,
        const std::string& uid ,
        const std::string& chnl ,
        int gamecnt ,
        const std::string& logver ,
        const std::string& gamerecord ,
        const std::string& skey) 
{
    std::ostringstream query;
    query << "INSERT INTO game_record_log (CODE, GAME_TYPE, UID, CHANNEL, GAME_CNT, LOG_VER, GAME_RECORD, SKEY) VALUES ("
        << code << ", "
        << game_type << ", "
        << "'" << uid << "', "
        << "'" << chnl << "', "
        << gamecnt << ", "
        << "'" << logver << "', "
        << "'" << gamerecord << "', "
        << "'" << skey << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertSlotEventLog(
        int player_idx ,
        int mail_idx,
        const std::string& nickname,
        const std::string& eventcode ,
        const std::string& reward )
{
	std::ostringstream query;
    query << "INSERT INTO slot_event_logs(platform_guid , player_idx , mail_index, nickname , event_code  , reward ) SELECT p.platform_guid ,p.player_idx ,"
        << "'" << mail_idx << "',"
        << "'" << nickname << "',"
        << "'" << eventcode << "',"
        << "'" << reward << "'"
        << "FROM tpp.players p WHERE p.player_idx = " << player_idx << ";";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

//std::future<BOOL> QueryManager::PinballLogInsert(
//    const int code ,
//    const std::string& account_guid ,
//    const std::string& platform_guid ,
//    const std::string& nickname ,
//    const std::string& ver,
//    const std::string& lastplaytime ,
//    const int gamecnt,
//    const std::string& ip ,
//    const Common::PinballType game_type ,
//    const std::string& regame ,
//    const std::string& lucky_num ,
//    const std::string& bingo_board ,
//    const int bingo_count ,
//    const std::string& bingo_multi ,
//    const std::string& result ,
//    const uint64_t coin ,
//    const uint64_t bet_coin ,
//    const int64_t change_coin ,
//    const int64_t win_coin ,
//    const int64_t discard_coin ,
//    const int rakeback ,
//    const uint64 total_coin
//)
//{
//    std::ostringstream oss;
//    oss << "INSERT INTO pinball_log ("
//        << "CODE, UID, IPADDR, NICKNAME, VER, LASTPLAYTIME, GAMECNT ,"
//        << "GAMETYPE, REGAME, LUCKY_NUM, BINGO_BOARD, BINGO_COUNT, BINGO_MULTI, RESULT, "
//        << "COIN, BET_COIN, CHANGE_COIN,WIN_COIN, DISCARD_COIN, RAKEBACK , TOTAL_COIN) VALUES ("
//        << code << ", '"
//        << platform_guid << "', '"
//        << ip << "', '"
//        << nickname << "', '"
//        << ver << "',";
//        if ( lastplaytime.empty() )
//            oss << "NULL, '";
//        else
//            oss << "'" << lastplaytime << "', '";
//        oss << gamecnt << "', '"
//        << static_cast< int >( game_type ) << "', '"
//        << regame << "', '"
//        << lucky_num << "', '"
//        << bingo_board << "', '"
//        << bingo_count << "', '"
//        << bingo_multi << "', '"
//        << result << "', "
//        << coin << ", "
//        << bet_coin << ", "
//        << change_coin << ", "
//        << win_coin << ", "
//        << discard_coin << ", "
//        << rakeback << ", "
//        << total_coin << ");";
//
//    std::string query = oss.str();
//    TraceA( query );
//    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query );
//}