#include "Query.h"

#include "./mysql/cMySQL.h"
#include "./mysql/cMySQLParserElement.h"
#include "./mysql/cMySQLReader.h"

#include "./mysql/cMySQLConnectionPooler.h"

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Manager/ServerManager.h"
#include "../Include/Netlib/Queue/cLogQueue.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "TimeUtils.h"
#include "cProtoUtil.h"

#include <mysql.h>
#include <mysqld_error.h>

#include <iostream>
#include <format>
#include <string.h>

std::future<BOOL> QueryManager::InsertGameHoldemLog(
    int code,
    const std::string& game_id,
    const std::string& channel,
    int game_cnt,
    const std::string& log_ver,
    const std::string& game_opts,
    const std::string& boss,
    const std::string& host,
    // 플레이어 0
    const std::string& p0_id,
    int64_t p0_asset,
    int p0_play_cnt,
    int64_t p0_change_coin,
    int64_t p0_discard_coin,
    int64_t p0_fee_coin,
    const std::string& p0_fee_info,
    int64_t p0_rakeback,
    int64_t p0_coin,
    const std::string& p0_result,
    // 플레이어 1
    const std::string& p1_id,
    int64_t p1_asset,
    int p1_play_cnt,
    int64_t p1_change_coin,
    int64_t p1_discard_coin,
    int64_t p1_fee_coin,
    const std::string& p1_fee_info,
    int64_t p1_rakeback,
    int64_t p1_coin,
    const std::string& p1_result,
    // 플레이어 2
    const std::string& p2_id,
    int64_t p2_asset,
    int p2_play_cnt,
    int64_t p2_change_coin,
    int64_t p2_discard_coin,
    int64_t p2_fee_coin,
    const std::string& p2_fee_info,
    int64_t p2_rakeback,
    int64_t p2_coin,
    const std::string& p2_result,
    // 플레이어 3
    const std::string& p3_id,
    int64_t p3_asset,
    int p3_play_cnt,
    int64_t p3_change_coin,
    int64_t p3_discard_coin,
    int64_t p3_fee_coin,
    const std::string& p3_fee_info,
    int64_t p3_rakeback,
    int64_t p3_coin,
    const std::string& p3_result,
    // 플레이어 4
    const std::string& p4_id,
    int64_t p4_asset,
    int p4_play_cnt,
    int64_t p4_change_coin,
    int64_t p4_discard_coin,
    int64_t p4_fee_coin,
    const std::string& p4_fee_info,
    int64_t p4_rakeback,
    int64_t p4_coin,
    const std::string& p4_result,
    // 플레이어 5
    const std::string& p5_id,
    int64_t p5_asset,
    int p5_play_cnt,
    int64_t p5_change_coin,
    int64_t p5_discard_coin,
    int64_t p5_fee_coin,
    const std::string& p5_fee_info,
    int64_t p5_rakeback,
    int64_t p5_coin,
    const std::string& p5_result,
    // 플레이어 6
    const std::string& p6_id,
    int64_t p6_asset,
    int p6_play_cnt,
    int64_t p6_change_coin,
    int64_t p6_discard_coin,
    int64_t p6_fee_coin,
    const std::string& p6_fee_info,
    int64_t p6_rakeback,
    int64_t p6_coin,
    const std::string& p6_result,
    // 플레이어 7
    const std::string& p7_id,
    int64_t p7_asset,
    int p7_play_cnt,
    int64_t p7_change_coin,
    int64_t p7_discard_coin,
    int64_t p7_fee_coin,
    const std::string& p7_fee_info,
    int64_t p7_rakeback,
    int64_t p7_coin,
    const std::string& p7_result,
    // 플레이어 8
    const std::string& p8_id,
    int64_t p8_asset,
    int p8_play_cnt,
    int64_t p8_change_coin,
    int64_t p8_discard_coin,
    int64_t p8_fee_coin,
    const std::string& p8_fee_info,
    int64_t p8_rakeback,
    int64_t p8_coin,
    const std::string& p8_result,
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
    int64_t win4_asset,
    const std::string& win5_id,
    int64_t win5_asset,
    const std::string& win6_id,
    int64_t win6_asset,
    const std::string& win7_id,
    int64_t win7_asset,
    const std::string& win8_id,
    int64_t win8_asset
) {
    std::ostringstream query;
    query << "INSERT INTO game_log (CODE, GAME_ID, CHANNEL, GAME_CNT, LOG_VER, GAME_OPTS, BOSS, HOST, "
        << "P0_ID, P0_ASSET, P0_PLAY_CNT, P0_CHANGE_COIN, P0_DISCARD_COIN, P0_FEE_COIN, P0_FEE_INFO, P0_RAKEBACK, P0_COIN, P0_RESULT, "
        << "P1_ID, P1_ASSET, P1_PLAY_CNT, P1_CHANGE_COIN, P1_DISCARD_COIN, P1_FEE_COIN, P1_FEE_INFO, P1_RAKEBACK, P1_COIN, P1_RESULT, "
        << "P2_ID, P2_ASSET, P2_PLAY_CNT, P2_CHANGE_COIN, P2_DISCARD_COIN, P2_FEE_COIN, P2_FEE_INFO, P2_RAKEBACK, P2_COIN, P2_RESULT, "
        << "P3_ID, P3_ASSET, P3_PLAY_CNT, P3_CHANGE_COIN, P3_DISCARD_COIN, P3_FEE_COIN, P3_FEE_INFO, P3_RAKEBACK, P3_COIN, P3_RESULT, "
        << "P4_ID, P4_ASSET, P4_PLAY_CNT, P4_CHANGE_COIN, P4_DISCARD_COIN, P4_FEE_COIN, P4_FEE_INFO, P4_RAKEBACK, P4_COIN, P4_RESULT, "
        << "P5_ID, P5_ASSET, P5_PLAY_CNT, P5_CHANGE_COIN, P5_DISCARD_COIN, P5_FEE_COIN, P5_FEE_INFO, P5_RAKEBACK, P5_COIN, P5_RESULT, "
        << "P6_ID, P6_ASSET, P6_PLAY_CNT, P6_CHANGE_COIN, P6_DISCARD_COIN, P6_FEE_COIN, P6_FEE_INFO, P6_RAKEBACK, P6_COIN, P6_RESULT, "
        << "P7_ID, P7_ASSET, P7_PLAY_CNT, P7_CHANGE_COIN, P7_DISCARD_COIN, P7_FEE_COIN, P7_FEE_INFO, P7_RAKEBACK, P7_COIN, P7_RESULT, "
        << "P8_ID, P8_ASSET, P8_PLAY_CNT, P8_CHANGE_COIN, P8_DISCARD_COIN, P8_FEE_COIN, P8_FEE_INFO, P8_RAKEBACK, P8_COIN, P8_RESULT, "
        << "SKEY, PLAY_TIME, ROOM_ID, WIN0_ID, WIN0_ASSET, WIN1_ID, WIN1_ASSET, WIN2_ID, WIN2_ASSET, WIN3_ID, WIN3_ASSET, WIN4_ID, WIN4_ASSET, "
        << "WIN5_ID, WIN5_ASSET, WIN6_ID, WIN6_ASSET, WIN7_ID, WIN7_ASSET, WIN8_ID, WIN8_ASSET) VALUES ("
        << code << ", "
        << "'" << game_id << "', "
        << "'" << channel << "', "
        << game_cnt << ", "
        << "'" << log_ver << "', "
        << "'" << game_opts << "', "
        << "'" << boss << "', "
        << "'" << host << "', "
        << "'" << p0_id << "', " << p0_asset << ", " << p0_play_cnt << ", " << p0_change_coin << ", " << p0_discard_coin << ", " << p0_fee_coin << ", "
        << "'" << p0_fee_info << "', " << p0_rakeback << ", " << p0_coin << ", '" << p0_result << "', "
        << "'" << p1_id << "', " << p1_asset << ", " << p1_play_cnt << ", " << p1_change_coin << ", " << p1_discard_coin << ", " << p1_fee_coin << ", "
        << "'" << p1_fee_info << "', " << p1_rakeback << ", " << p1_coin << ", '" << p1_result << "', "
        << "'" << p2_id << "', " << p2_asset << ", " << p2_play_cnt << ", " << p2_change_coin << ", " << p2_discard_coin << ", " << p2_fee_coin << ", "
        << "'" << p2_fee_info << "', " << p2_rakeback << ", " << p2_coin << ", '" << p2_result << "', "
        << "'" << p3_id << "', " << p3_asset << ", " << p3_play_cnt << ", " << p3_change_coin << ", " << p3_discard_coin << ", " << p3_fee_coin << ", "
        << "'" << p3_fee_info << "', " << p3_rakeback << ", " << p3_coin << ", '" << p3_result << "', "
        << "'" << p4_id << "', " << p4_asset << ", " << p4_play_cnt << ", " << p4_change_coin << ", " << p4_discard_coin << ", " << p4_fee_coin << ", "
        << "'" << p4_fee_info << "', " << p4_rakeback << ", " << p4_coin << ", '" << p4_result << "', "
        << "'" << p5_id << "', " << p5_asset << ", " << p5_play_cnt << ", " << p5_change_coin << ", " << p5_discard_coin << ", " << p5_fee_coin << ", "
        << "'" << p5_fee_info << "', " << p5_rakeback << ", " << p5_coin << ", '" << p5_result << "', "
        << "'" << p6_id << "', " << p6_asset << ", " << p6_play_cnt << ", " << p6_change_coin << ", " << p6_discard_coin << ", " << p6_fee_coin << ", "
        << "'" << p6_fee_info << "', " << p6_rakeback << ", " << p6_coin << ", '" << p6_result << "', "
        << "'" << p7_id << "', " << p7_asset << ", " << p7_play_cnt << ", " << p7_change_coin << ", " << p7_discard_coin << ", " << p7_fee_coin << ", "
        << "'" << p7_fee_info << "', " << p7_rakeback << ", " << p7_coin << ", '" << p7_result << "', "
        << "'" << p8_id << "', " << p8_asset << ", " << p8_play_cnt << ", " << p8_change_coin << ", " << p8_discard_coin << ", " << p8_fee_coin << ", "
        << "'" << p8_fee_info << "', " << p8_rakeback << ", " << p8_coin << ", '" << p8_result << "', "
        << "'" << skey << "', " << play_time << ", '" << room_id << "', "
        << "'" << win0_id << "', " << win0_asset << ", "
        << "'" << win1_id << "', " << win1_asset << ", "
        << "'" << win2_id << "', " << win2_asset << ", "
        << "'" << win3_id << "', " << win3_asset << ", "
        << "'" << win4_id << "', " << win4_asset << ", "
        << "'" << win5_id << "', " << win5_asset << ", "
        << "'" << win6_id << "', " << win6_asset << ", "
        << "'" << win7_id << "', " << win7_asset << ", "
        << "'" << win8_id << "', " << win8_asset << ")";

    return PlayerExecuteQueryAsync(E_DB_TYPE::E_DB_TYPE_LOG, query.str());
}

std::future<BOOL> QueryManager::InsertGameLowbadugiLog(
    int code,
    const std::string& game_id,
    const std::string& channel,
    int game_cnt,
    const std::string& log_ver,
    const std::string& game_opts,
    const std::string& boss,
    const std::string& host,
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
) {
    std::ostringstream query;
    query << "INSERT INTO game_log (CODE, GAME_ID, CHANNEL, GAME_CNT, LOG_VER, GAME_OPTS, BOSS, HOST, "
        << "P0_ID, P0_ASSET, P0_PLAY_CNT, P0_CHANGE_CHIP, P0_CHANGE_COIN, P0_DISCARD_CHIP, P0_DISCARD_COIN, P0_FEE_CHIP, P0_FEE_COIN, P0_FEE_INFO, P0_RAKEBACK, P0_TODAY_CHIP, P0_CHIP, P0_COIN, P0_RESULT, "
        << "P1_ID, P1_ASSET, P1_PLAY_CNT, P1_CHANGE_CHIP, P1_CHANGE_COIN, P1_DISCARD_CHIP, P1_DISCARD_COIN, P1_FEE_CHIP, P1_FEE_COIN, P1_FEE_INFO, P1_RAKEBACK, P1_TODAY_CHIP, P1_CHIP, P1_COIN, P1_RESULT, "
        << "P2_ID, P2_ASSET, P2_PLAY_CNT, P2_CHANGE_CHIP, P2_CHANGE_COIN, P2_DISCARD_CHIP, P2_DISCARD_COIN, P2_FEE_CHIP, P2_FEE_COIN, P2_FEE_INFO, P2_RAKEBACK, P2_TODAY_CHIP, P2_CHIP, P2_COIN, P2_RESULT, "
        << "P3_ID, P3_ASSET, P3_PLAY_CNT, P3_CHANGE_CHIP, P3_CHANGE_COIN, P3_DISCARD_CHIP, P3_DISCARD_COIN, P3_FEE_CHIP, P3_FEE_COIN, P3_FEE_INFO, P3_RAKEBACK, P3_TODAY_CHIP, P3_CHIP, P3_COIN, P3_RESULT, "
        << "P4_ID, P4_ASSET, P4_PLAY_CNT, P4_CHANGE_CHIP, P4_CHANGE_COIN, P4_DISCARD_CHIP, P4_DISCARD_COIN, P4_FEE_CHIP, P4_FEE_COIN, P4_FEE_INFO, P4_RAKEBACK, P4_TODAY_CHIP, P4_CHIP, P4_COIN, P4_RESULT, "
        << "SKEY, PLAY_TIME, ROOM_ID, WIN0_ID, WIN0_ASSET, WIN1_ID, WIN1_ASSET, WIN2_ID, WIN2_ASSET, WIN3_ID, WIN3_ASSET, WIN4_ID, WIN4_ASSET) VALUES ("
        << code << ", "
        << "'" << game_id << "', "
        << "'" << channel << "', "
        << game_cnt << ", "
        << "'" << log_ver << "', "
        << "'" << game_opts << "', "
        << "'" << boss << "', "
        << "'" << host << "', "
        << "'" << p0_id << "', " << p0_asset << ", " << p0_play_cnt << ", " << p0_change_chip << ", " << p0_change_coin << ", " << p0_discard_chip << ", " << p0_discard_coin << ", " << p0_fee_chip << ", " << p0_fee_coin << ", "
        << "'" << p0_fee_info << "', " << p0_rakeback << ", " << p0_today_chip << ", " << p0_chip << ", " << p0_coin << ", '" << p0_result << "', "
        << "'" << p1_id << "', " << p1_asset << ", " << p1_play_cnt << ", " << p1_change_chip << ", " << p1_change_coin << ", " << p1_discard_chip << ", " << p1_discard_coin << ", " << p1_fee_chip << ", " << p1_fee_coin << ", "
        << "'" << p1_fee_info << "', " << p1_rakeback << ", " << p1_today_chip << ", " << p1_chip << ", " << p1_coin << ", '" << p1_result << "', "
        << "'" << p2_id << "', " << p2_asset << ", " << p2_play_cnt << ", " << p2_change_chip << ", " << p2_change_coin << ", " << p2_discard_chip << ", " << p2_discard_coin << ", " << p2_fee_chip << ", " << p2_fee_coin << ", "
        << "'" << p2_fee_info << "', " << p2_rakeback << ", " << p2_today_chip << ", " << p2_chip << ", " << p2_coin << ", '" << p2_result << "', "
        << "'" << p3_id << "', " << p3_asset << ", " << p3_play_cnt << ", " << p3_change_chip << ", " << p3_change_coin << ", " << p3_discard_chip << ", " << p3_discard_coin << ", " << p3_fee_chip << ", " << p3_fee_coin << ", "
        << "'" << p3_fee_info << "', " << p3_rakeback << ", " << p3_today_chip << ", " << p3_chip << ", " << p3_coin << ", '" << p3_result << "', "
        << "'" << p4_id << "', " << p4_asset << ", " << p4_play_cnt << ", " << p4_change_chip << ", " << p4_change_coin << ", " << p4_discard_chip << ", " << p4_discard_coin << ", " << p4_fee_chip << ", " << p4_fee_coin << ", "
        << "'" << p4_fee_info << "', " << p4_rakeback << ", " << p4_today_chip << ", " << p4_chip << ", " << p4_coin << ", '" << p4_result << "', "
        << "'" << skey << "', " << play_time << ", '" << room_id << "', "
        << "'" << win0_id << "', " << win0_asset << ", "
        << "'" << win1_id << "', " << win1_asset << ", "
        << "'" << win2_id << "', " << win2_asset << ", "
        << "'" << win3_id << "', " << win3_asset << ", "
        << "'" << win4_id << "', " << win4_asset << ")";

    return PlayerExecuteQueryAsync(E_DB_TYPE::E_DB_TYPE_LOG, query.str());
}

std::future<BOOL> QueryManager::InsertGameBaccaraLog(
    int code,
    const std::string& game_id,
    const std::string& channel,
    int game_cnt,
    const std::string& log_ver,
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
    const std::string& skey,
    int play_time,
    const std::string& room_id
) {
    std::ostringstream query;
    query << "INSERT INTO game_log (CODE, GAME_ID, CHANNEL, GAME_CNT, LOG_VER, "
        << "P0_ID, P0_CHANGE_COIN, P0_DISCARD_COIN, P0_RAKEBACK, P0_COIN, P0_RESULT, "
        << "P1_ID, P1_CHANGE_COIN, P1_DISCARD_COIN, P1_RAKEBACK, P1_COIN, P1_RESULT, "
        << "P2_ID, P2_CHANGE_COIN, P2_DISCARD_COIN, P2_RAKEBACK, P2_COIN, P2_RESULT, "
        << "P3_ID, P3_CHANGE_COIN, P3_DISCARD_COIN, P3_RAKEBACK, P3_COIN, P3_RESULT, "
        << "P4_ID, P4_CHANGE_COIN, P4_DISCARD_COIN, P4_RAKEBACK, P4_COIN, P4_RESULT, "
        << "P5_ID, P5_CHANGE_COIN, P5_DISCARD_COIN, P5_RAKEBACK, P5_COIN, P5_RESULT, "
        << "P6_ID, P6_CHANGE_COIN, P6_DISCARD_COIN, P6_RAKEBACK, P6_COIN, P6_RESULT, "
        << "SKEY, PLAY_TIME, ROOM_ID) VALUES ("
        << code << ", "
        << "'" << game_id << "', "
        << "'" << channel << "', "
        << game_cnt << ", "
        << "'" << log_ver << "', "
        << "'" << p0_id << "', " << p0_change_coin << ", " << p0_discard_coin << ", " << p0_rakeback << ", " << p0_coin << ", '" << p0_result << "', "
        << "'" << p1_id << "', " << p1_change_coin << ", " << p1_discard_coin << ", " << p1_rakeback << ", " << p1_coin << ", '" << p1_result << "', "
        << "'" << p2_id << "', " << p2_change_coin << ", " << p2_discard_coin << ", " << p2_rakeback << ", " << p2_coin << ", '" << p2_result << "', "
        << "'" << p3_id << "', " << p3_change_coin << ", " << p3_discard_coin << ", " << p3_rakeback << ", " << p3_coin << ", '" << p3_result << "', "
        << "'" << p4_id << "', " << p4_change_coin << ", " << p4_discard_coin << ", " << p4_rakeback << ", " << p4_coin << ", '" << p4_result << "', "
        << "'" << p5_id << "', " << p5_change_coin << ", " << p5_discard_coin << ", " << p5_rakeback << ", " << p5_coin << ", '" << p5_result << "', "
        << "'" << p6_id << "', " << p6_change_coin << ", " << p6_discard_coin << ", " << p6_rakeback << ", " << p6_coin << ", '" << p6_result << "', "
        << "'" << skey << "', " << play_time << ", '" << room_id << "')";

    return PlayerExecuteQueryAsync(E_DB_TYPE::E_DB_TYPE_LOG, query.str());
}


//std::future<BOOL> QueryManager::InsertGameRouletteLog(
//    int code ,
//    const std::string& game_id ,
//    const std::string& channel ,
//    int game_cnt ,
//    const std::string& log_ver ,
//    // 플레이어 0
//    const std::string& p0_id ,
//    int64_t p0_change_coin ,
//    int64_t p0_discard_coin ,
//    int64_t p0_rakeback ,
//    int64_t p0_coin ,
//    const std::string& p0_result ,
//    // 플레이어 1
//    const std::string& p1_id ,
//    int64_t p1_change_coin ,
//    int64_t p1_discard_coin ,
//    int64_t p1_rakeback ,
//    int64_t p1_coin ,
//    const std::string& p1_result ,
//    // 플레이어 2
//    const std::string& p2_id ,
//    int64_t p2_change_coin ,
//    int64_t p2_discard_coin ,
//    int64_t p2_rakeback ,
//    int64_t p2_coin ,
//    const std::string& p2_result ,
//    // 플레이어 3
//    const std::string& p3_id ,
//    int64_t p3_change_coin ,
//    int64_t p3_discard_coin ,
//    int64_t p3_rakeback ,
//    int64_t p3_coin ,
//    const std::string& p3_result ,
//    // 추가 데이터
//    const std::string& skey ,
//    int play_time ,
//    const std::string& room_id
//) {
//    std::ostringstream query;
//    query << "INSERT INTO game_log (CODE, GAME_ID, CHANNEL, GAME_CNT, LOG_VER, "
//        << "P0_ID, P0_CHANGE_COIN, P0_DISCARD_COIN, P0_RAKEBACK, P0_COIN, P0_RESULT, "
//        << "P1_ID, P1_CHANGE_COIN, P1_DISCARD_COIN, P1_RAKEBACK, P1_COIN, P1_RESULT, "
//        << "P2_ID, P2_CHANGE_COIN, P2_DISCARD_COIN, P2_RAKEBACK, P2_COIN, P2_RESULT, "
//        << "P3_ID, P3_CHANGE_COIN, P3_DISCARD_COIN, P3_RAKEBACK, P3_COIN, P3_RESULT, "
//        << "SKEY, PLAY_TIME, ROOM_ID) VALUES ("
//        << code << ", "
//        << "'" << game_id << "', "
//        << "'" << channel << "', "
//        << game_cnt << ", "
//        << "'" << log_ver << "', "
//        << "'" << p0_id << "', " << p0_change_coin << ", " << p0_discard_coin << ", " << p0_rakeback << ", " << p0_coin << ", '" << p0_result << "', "
//        << "'" << p1_id << "', " << p1_change_coin << ", " << p1_discard_coin << ", " << p1_rakeback << ", " << p1_coin << ", '" << p1_result << "', "
//        << "'" << p2_id << "', " << p2_change_coin << ", " << p2_discard_coin << ", " << p2_rakeback << ", " << p2_coin << ", '" << p2_result << "', "
//        << "'" << p3_id << "', " << p3_change_coin << ", " << p3_discard_coin << ", " << p3_rakeback << ", " << p3_coin << ", '" << p3_result << "', "
//        << "'" << skey << "', " << play_time << ", '" << room_id << "')";
//
//    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
//}

std::future<BOOL> QueryManager::InsertGameBlackJackLog(
    int code ,
    const std::string& game_id ,
    const std::string& channel ,
    int game_cnt ,
    const std::string& log_ver ,
    const std::string& game_opts ,
    const std::string& p0_id ,
    int64_t p0_asset ,
    int64_t p0_change_coin ,
    int64_t p0_discard_coin ,
    int64_t p0_rakeback ,
    int64_t p0_coin ,
    int p0_playcnt ,
    const std::string& p0_betting ,
    const std::string& p0_records ,
    const std::string& p0_result ,
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
    const std::string& skey ,
    int play_time ,
    const std::string& room_id )
{
    std::ostringstream query;

    query << "INSERT INTO game_log (CODE, CHANNEL, GAME_ID, LOG_VER, GAME_OPTS, GAME_CNT, PLAY_TIME, "
        << "P0_ID, P0_ASSET, P0_CHANGE_COIN, P0_DISCARD_COIN, P0_RAKEBACK, P0_COIN, P0_PLAY_CNT, P0_BETTING, P0_RECORDS, P0_RESULT, "
        << "P1_ID, P1_ASSET, P1_CHANGE_COIN, P1_DISCARD_COIN, P1_RAKEBACK, P1_COIN, P1_PLAY_CNT, P1_BETTING, P1_RECORDS, P1_RESULT, "
        << "P2_ID, P2_ASSET, P2_CHANGE_COIN, P2_DISCARD_COIN, P2_RAKEBACK, P2_COIN, P2_PLAY_CNT, P2_BETTING, P2_RECORDS, P2_RESULT, "
        << "P3_ID, P3_ASSET, P3_CHANGE_COIN, P3_DISCARD_COIN, P3_RAKEBACK, P3_COIN, P3_PLAY_CNT, P3_BETTING, P3_RECORDS, P3_RESULT, "
        << "P4_ID, P4_ASSET, P4_CHANGE_COIN, P4_DISCARD_COIN, P4_RAKEBACK, P4_COIN, P4_PLAY_CNT, P4_BETTING, P4_RECORDS, P4_RESULT, "
        << "SKEY, ROOM_ID) VALUES ("
        << code << ", "
        << "'" << channel << "', "
        << "'" << game_id << "', "
        << "'" << log_ver << "', "
        << "'" << game_opts << "', "
        << game_cnt << ", "
        << play_time << ", "
        << "'" << p0_id << "', "
        << p0_asset << ", " << p0_change_coin << ", " << p0_discard_coin << ", " << p0_rakeback << ", " << p0_coin << ", " << p0_playcnt << ", "
        << "'" << p0_betting << "', "
        << "'" << p0_records << "', "
        << "'" << p0_result << "', "
        << "'" << p1_id << "', "
        << p1_asset << ", " << p1_change_coin << ", " << p1_discard_coin << ", " << p1_rakeback << ", " << p1_coin << ", " << p1_playcnt << ", "
        << "'" << p1_betting << "', "
        << "'" << p1_records << "', "
        << "'" << p1_result << "', "
        << "'" << p2_id << "', "
        << p2_asset << ", " << p2_change_coin << ", " << p2_discard_coin << ", " << p2_rakeback << ", " << p2_coin << ", " << p2_playcnt << ", "
        << "'" << p2_betting << "', "
        << "'" << p2_records << "', "
        << "'" << p2_result << "', "
        << "'" << p3_id << "', "
        << p3_asset << ", " << p3_change_coin << ", " << p3_discard_coin << ", " << p3_rakeback << ", " << p3_coin << ", " << p3_playcnt << ", "
        << "'" << p3_betting << "', "
        << "'" << p3_records << "', "
        << "'" << p3_result << "', "
        << "'" << p4_id << "', "
        << p4_asset << ", " << p4_change_coin << ", " << p4_discard_coin << ", " << p4_rakeback << ", " << p4_coin << ", " << p4_playcnt << ", "
        << "'" << p4_betting << "', "
        << "'" << p4_records << "', "
        << "'" << p4_result << "', "
        << "'" << skey << "', "
        << "'" << room_id << "')";

    //query << "INSERT INTO game_blackjack_log (CODE, CHANNEL, GAME_ID, LOG_VER, GAME_OPTS, GAME_CNT, PLAY_TIME, "
    //    << "P0_ID, P0_ASSET, P0_CHANGE_COIN, P0_DISCARD_COIN, P0_RAKEBACK, P0_COIN, P0_PLAY_CNT, P0_BETTING, P0_RECORDS, P0_RESULT, "
    //    << "P1_ID, P1_ASSET, P1_CHANGE_COIN, P1_DISCARD_COIN, P1_RAKEBACK, P1_COIN, P1_PLAY_CNT, P1_BETTING, P1_RECORDS, P1_RESULT, "
    //    << "P2_ID, P2_ASSET, P2_CHANGE_COIN, P2_DISCARD_COIN, P2_RAKEBACK, P2_COIN, P2_PLAY_CNT, P2_BETTING, P2_RECORDS, P2_RESULT, "
    //    << "P3_ID, P3_ASSET, P3_CHANGE_COIN, P3_DISCARD_COIN, P3_RAKEBACK, P3_COIN, P3_PLAY_CNT, P3_BETTING, P3_RECORDS, P3_RESULT, "
    //    << "P4_ID, P4_ASSET, P4_CHANGE_COIN, P4_DISCARD_COIN, P4_RAKEBACK, P4_COIN, P4_PLAY_CNT, P4_BETTING, P4_RECORDS, P4_RESULT, "
    //    << "SKEY, ROOM_ID) VALUES ("
    //    << code << ", "
    //    << "'" << channel << "', "
    //    << "'" << game_id << "', "
    //    << "'" << log_ver << "', "
    //    << "'" << game_opts << "', "
    //    << game_cnt << ", "
    //    << "'" << play_time << "', "
    //    << "'" << p0_id << "', "
    //    << p0_asset << ", " << p0_change_coin << ", " << p0_discard_coin << ", " << p0_rakeback << ", " << p0_coin << ", " << p0_playcnt << ", "
    //    << "'" << p0_betting << "', "
    //    << "'" << p0_records << "', "
    //    << "'" << p0_result << "', "
    //    << "'" << p1_id << "', "
    //    << p1_asset << ", " << p1_change_coin << ", " << p1_discard_coin << ", " << p1_rakeback << ", " << p1_coin << ", " << p1_playcnt << ", "
    //    << "'" << p1_betting << "', "
    //    << "'" << p1_records << "', "
    //    << "'" << p1_result << "', "
    //    << "'" << p2_id << "', "
    //    << p2_asset << ", " << p2_change_coin << ", " << p2_discard_coin << ", " << p2_rakeback << ", " << p2_coin << ", " << p2_playcnt << ", "
    //    << "'" << p2_betting << "', "
    //    << "'" << p2_records << "', "
    //    << "'" << p2_result << "', "
    //    << "'" << p3_id << "', "
    //    << p3_asset << ", " << p3_change_coin << ", " << p3_discard_coin << ", " << p3_rakeback << ", " << p3_coin << ", " << p3_playcnt << ", "
    //    << "'" << p3_betting << "', "
    //    << "'" << p3_records << "', "
    //    << "'" << p3_result << "', "
    //    << "'" << p4_id << "', "
    //    << p4_asset << ", " << p4_change_coin << ", " << p4_discard_coin << ", " << p4_rakeback << ", " << p4_coin << ", " << p4_playcnt << ", "
    //    << "'" << p4_betting << "', "
    //    << "'" << p4_records << "', "
    //    << "'" << p4_result << "', "
    //    << "'" << skey << "', "
    //    << "'" << room_id << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertGameKickoutLog(
    int code,
    const std::string& game_id,
    const std::string& channel,
    int game_cnt,
    const std::string& log_ver,
    const std::string& host,
    int kickout_ticket,
    int kickout_count,
    const std::string& skey,
    const std::string& target,
    const std::string& room_id
) {
    std::ostringstream query;
    query << "INSERT INTO game_log (CODE, CHANNEL, GAME_ID, GAME_CNT, LOG_VER, HOST, "
        << "KICKOUT_TICKET, KICKOUT_COUNT, SKEY, TARGET, ROOM_ID) VALUES ("
        << code << ", "
        << "'" << channel << "', "
        << "'" << game_id << "', "
        << game_cnt << ", "
        << "'" << log_ver << "', "
        << "'" << host << "', "
        << kickout_ticket << ", "
        << kickout_count << ", "
        << "'" << skey << "', "
        << "'" << target << "', "
        << "'" << room_id << "')";

    return PlayerExecuteQueryAsync(E_DB_TYPE::E_DB_TYPE_LOG, query.str());
}

std::future<BOOL> QueryManager::InsertGameBaccaraBettingLog(
    int code,
    const std::string& game_id,
    const std::string& channel,
    int game_cnt,
    const std::string& log_ver,
    const std::string& game_opts,
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
) {
    std::ostringstream query;
    query << "INSERT INTO game_log (CODE, GAME_ID, CHANNEL, GAME_CNT, LOG_VER, GAME_OPTS, "
        << "P0_ID, P0_ASSET, P0_PLAY_CNT, P0_BETTING, P0_RECORDS, "
        << "P1_ID, P1_ASSET, P1_PLAY_CNT, P1_BETTING, P1_RECORDS, "
        << "P2_ID, P2_ASSET, P2_PLAY_CNT, P2_BETTING, P2_RECORDS, "
        << "P3_ID, P3_ASSET, P3_PLAY_CNT, P3_BETTING, P3_RECORDS, "
        << "P4_ID, P4_ASSET, P4_PLAY_CNT, P4_BETTING, P4_RECORDS, "
        << "P5_ID, P5_ASSET, P5_PLAY_CNT, P5_BETTING, P5_RECORDS, "
        << "P6_ID, P6_ASSET, P6_PLAY_CNT, P6_BETTING, P6_RECORDS, "
        << "SKEY, ROOM_ID) VALUES ("
        << code << ", "
        << "'" << game_id << "', "
        << "'" << channel << "', "
        << game_cnt << ", "
        << "'" << log_ver << "', "
        << "'" << game_opts << "', "
        << "'" << p0_id << "', " << p0_asset << ", " << p0_play_cnt << ", '" << p0_betting << "', '" << p0_records << "', "
        << "'" << p1_id << "', " << p1_asset << ", " << p1_play_cnt << ", '" << p1_betting << "', '" << p1_records << "', "
        << "'" << p2_id << "', " << p2_asset << ", " << p2_play_cnt << ", '" << p2_betting << "', '" << p2_records << "', "
        << "'" << p3_id << "', " << p3_asset << ", " << p3_play_cnt << ", '" << p3_betting << "', '" << p3_records << "', "
        << "'" << p4_id << "', " << p4_asset << ", " << p4_play_cnt << ", '" << p4_betting << "', '" << p4_records << "', "
        << "'" << p5_id << "', " << p5_asset << ", " << p5_play_cnt << ", '" << p5_betting << "', '" << p5_records << "', "
        << "'" << p6_id << "', " << p6_asset << ", " << p6_play_cnt << ", '" << p6_betting << "', '" << p6_records << "', "
        << "'" << skey << "', "
        << "'" << room_id << "')";

    return PlayerExecuteQueryAsync(E_DB_TYPE::E_DB_TYPE_LOG, query.str());
}

//std::future<BOOL> QueryManager::InsertGameRouletteBettingLog(
//    int code ,
//    const std::string& game_id ,
//    const std::string& channel ,
//    int game_cnt ,
//    const std::string& log_ver ,
//    const std::string& game_opts ,
//    // 플레이어 0
//    const std::string& p0_id ,
//    int64_t p0_asset ,
//    int p0_play_cnt ,
//    const std::string& p0_betting ,
//    const std::string& p0_records ,
//    // 플레이어 1
//    const std::string& p1_id ,
//    int64_t p1_asset ,
//    int p1_play_cnt ,
//    const std::string& p1_betting ,
//    const std::string& p1_records ,
//    // 플레이어 2
//    const std::string& p2_id ,
//    int64_t p2_asset ,
//    int p2_play_cnt ,
//    const std::string& p2_betting ,
//    const std::string& p2_records ,
//    // 플레이어 3
//    const std::string& p3_id ,
//    int64_t p3_asset ,
//    int p3_play_cnt ,
//    const std::string& p3_betting ,
//    const std::string& p3_records ,
//    // 추가 데이터
//    const std::string& skey ,
//    const std::string& room_id
//) {
//    std::ostringstream query;
//    query << "INSERT INTO game_log (CODE, GAME_ID, CHANNEL, GAME_CNT, LOG_VER, GAME_OPTS, "
//        << "P0_ID, P0_ASSET, P0_PLAY_CNT, P0_BETTING, P0_RECORDS, "
//        << "P1_ID, P1_ASSET, P1_PLAY_CNT, P1_BETTING, P1_RECORDS, "
//        << "P2_ID, P2_ASSET, P2_PLAY_CNT, P2_BETTING, P2_RECORDS, "
//        << "P3_ID, P3_ASSET, P3_PLAY_CNT, P3_BETTING, P3_RECORDS, "
//        << "SKEY, ROOM_ID) VALUES ("
//        << code << ", "
//        << "'" << game_id << "', "
//        << "'" << channel << "', "
//        << game_cnt << ", "
//        << "'" << log_ver << "', "
//        << "'" << game_opts << "', "
//        << "'" << p0_id << "', " << p0_asset << ", " << p0_play_cnt << ", '" << p0_betting << "', '" << p0_records << "', "
//        << "'" << p1_id << "', " << p1_asset << ", " << p1_play_cnt << ", '" << p1_betting << "', '" << p1_records << "', "
//        << "'" << p2_id << "', " << p2_asset << ", " << p2_play_cnt << ", '" << p2_betting << "', '" << p2_records << "', "
//        << "'" << p3_id << "', " << p3_asset << ", " << p3_play_cnt << ", '" << p3_betting << "', '" << p3_records << "', "
//        << "'" << skey << "', "
//        << "'" << room_id << "')";
//
//    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
//}


std::future<BOOL> QueryManager::InsertGameExitLog(
    int code ,
    const std::string& channel ,
    const std::string& game_id ,
    int game_cnt ,
    const std::string& log_ver ,
    const std::string& host ,
    const std::string& p0_id ,
    long long p0_asset ,
    long long p0_today_chip ,
    const std::string& p1_id ,
    long long p1_asset ,
    long long p1_today_chip ,
    const std::string& p2_id ,
    long long p2_asset ,
    long long p2_today_chip ,
    const std::string& p3_id ,
    long long p3_asset ,
    long long p3_today_chip ,
    const std::string& p4_id ,
    long long p4_asset ,
    long long p4_today_chip ,
    const std::string& p5_id ,
    long long p5_asset ,
    const std::string& p6_id ,
    long long p6_asset ,
    const std::string& p7_id ,
    long long p7_asset ,
    const std::string& p8_id ,
    long long p8_asset ,
    const std::string& skey ,
    const std::string& room_id ,
    const std::string& reason ,
    const std::string& state
) {
    std::ostringstream query;
    query << "INSERT INTO game_log (CODE, CHANNEL, GAME_ID, GAME_CNT, LOG_VER, HOST, "
        << "P0_ID, P0_ASSET, P0_TODAY_CHIP, P1_ID, P1_ASSET, P1_TODAY_CHIP, P2_ID, P2_ASSET, P2_TODAY_CHIP, "
        << "P3_ID, P3_ASSET, P3_TODAY_CHIP, P4_ID, P4_ASSET, P4_TODAY_CHIP, P5_ID, P5_ASSET, "
        << "P6_ID, P6_ASSET, P7_ID, P7_ASSET, P8_ID, P8_ASSET, SKEY, ROOM_ID, REASON, STATE) VALUES ("
        << code << ", "
        << "'" << channel << "', "
        << "'" << game_id << "', "
        << game_cnt << ", "
        << "'" << log_ver << "', "
        << "'" << host << "', "
        << "'" << p0_id << "', " << p0_asset << ", " << p0_today_chip << ", "
        << "'" << p1_id << "', " << p1_asset << ", " << p1_today_chip << ", "
        << "'" << p2_id << "', " << p2_asset << ", " << p2_today_chip << ", "
        << "'" << p3_id << "', " << p3_asset << ", " << p3_today_chip << ", "
        << "'" << p4_id << "', " << p4_asset << ", " << p4_today_chip << ", "
        << "'" << p5_id << "', " << p5_asset << ", "
        << "'" << p6_id << "', " << p6_asset << ", "
        << "'" << p7_id << "', " << p7_asset << ", "
        << "'" << p8_id << "', " << p8_asset << ", "
        << "'" << skey << "', "
        << "'" << room_id << "', "
        << "'" << reason << "', "
        << "'" << state << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertGameConcurrentUsersLog(
    const int online_user,
    const int lowbaduiki_play_user_cip,
    const int lowbaduiki_play_user_coin,
    const int lowbaduiki_play_user_friend,
    const int holdem_play_user,
    const int bakara_play_user,
    const int blackjack_play_user,
    const int slot_play_user
    //const int roulette_play_user
) {
    std::ostringstream query;
    query << "INSERT INTO game_concurrent_users_log (online_user, lowbaduki_play_user_cip, lowbaduki_play_user_coin, lowbaduki_play_user_friend, holdem_play_user, bakara_play_user, blackjack_play_user, slot_play_user ) VALUES ("
        << online_user << ", "
        << lowbaduiki_play_user_cip << ", "
        << lowbaduiki_play_user_coin << ", "
        << lowbaduiki_play_user_friend << ", "
        << holdem_play_user << ", "
        << bakara_play_user << ", "
        << blackjack_play_user << ", "
        << slot_play_user << 
        ")";/* ", "
        << roulette_play_user << ")";*/

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertSlotConcurrentUsersLog( const std::map<std::string , int>& channelUserCount ) {
    std::ostringstream query;

    // 데이터를 포맷팅하여 저장
    std::ostringstream formattedData;
    
    std::vector <std::string> enumValues;

    // 0부터 100까지
    for ( int i = 0; i <= 100; ++i ) {
        std::string name = General::SlotCatalog_Name( static_cast< General::SlotCatalog >( i ) );
        if ( !name.empty() ) // string이 있으면 값이 있음.
            enumValues.push_back( name );
    }

    // 1만부터 100만까지
    for ( int i = 10000; i <= 1000000; i += 10000 ) {
        std::string name = General::SlotCatalog_Name( static_cast< General::SlotCatalog >( i ) );
        if ( !name.empty() ) // string이 있으면 값이 있음.
            enumValues.push_back( name );
    }

    for ( string keyValue : enumValues ) 
    {
        if ( channelUserCount.find( keyValue ) != channelUserCount.end() ) 
        {
            // 슬롯 타입
            General::SlotCatalog* type = new General::SlotCatalog();
            General::SlotCatalog_Parse( keyValue , type );

            // 접속자수
            int count = channelUserCount.at( keyValue );

            formattedData << static_cast< int >( *type ) << ":" << count << "/";

            // 메모리 해제
            delete type;
        }
    }

    // 마지막 "/" 제거
    std::string formattedStr = formattedData.str();
    if ( !formattedStr.empty() ) 
    {
        formattedStr.pop_back(); // 마지막 "/" 제거
    }
    else
    {
        formattedStr = "Empty Data";
    }

    // INSERT 쿼리 생성
    query << "INSERT INTO slot_concurrent_users_log (user_count) VALUES ('"
        << formattedStr << "')";

    // 비동기 실행
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}