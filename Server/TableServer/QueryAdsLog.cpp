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

std::future<BOOL> QueryManager::InsertAdsLog(
    int code ,                             // 로그 코드
    const std::string& uid ,               // UID
    const std::string& cmd ,               // 시퀀스
    const std::string& ipaddr ,            // IP 정보
    const std::string& type ,              // 광고 타입(위치)
    const std::string& ver )               // 접속한 게임 버전
{
    std::ostringstream query;
    query << "INSERT INTO ads_log (CODE, UID, CMD, IP_ADDR, TYPE, VER) VALUES ("
        << code << ", "
        << "'" << uid << "', "
        << "'" << cmd << "', "
        << "'" << ipaddr << "', "
        << "'" << type << "', "
        << "'" << ver << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertSocialLog(
    int code ,                             // 로그 코드
    const std::string& uid ,               // UID
    const std::string& cmd ,               // 시퀀스
    const std::string& ipaddr ,            // IP 정보
    const std::string& friend_id ,         // 친구의 UID
    const std::string& logdata ,           // 친구 데이터
    const std::string& ver )               // 접속한 게임 버전
{
    std::ostringstream query;
    query << "INSERT INTO social_log (CODE, UID, CMD, IP_ADDR, FRIEND_ID, LOG_DATA, VER ) VALUES ("
        << code << ", "
        << "'" << uid << "', "
        << "'" << cmd << "', "
        << "'" << ipaddr << "', "
        << "'" << friend_id << "', "
        << "'" << logdata << "', "
        << "'" << ver << "')";
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertMessageLog(
    int code ,                             // 로그 코드
    const std::string& uid ,               // UID
    const std::string& cmd ,               // 시퀀스
    const std::string& ipaddr ,            // IP 정보
    const std::string& data ,              // 초과금 정보
    const std::string& msgid ,             // 메시지 ID
    int msgtype ,                          // 메시지 타입
    const std::string& reward ,            // 메시지 내용물(보상 내용)
    const std::string& sender ,            // 메시지 발송 사유
    const std::string& ver ,               // 접속한 게임 버전
    const std::string& etc )               // 기타
{
    std::ostringstream query;
    query << "INSERT INTO message_log (CODE, UID, CMD, IP_ADDR, DATA, MSG_ID, MSG_TYPE, REWARD, SENDER, VER, ETC ) VALUES ("
        << code << ", "
        << "'" << uid << "', "
        << "'" << cmd << "', "
        << "'" << ipaddr << "', "
        << "'" << data << "', "
        << "'" << msgid << "', "
        << msgtype << ", "
        << "'" << reward << "', "
        << "'" << sender << "', "
        << "'" << ver << "', "
        << "'" << etc << "')";
    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}