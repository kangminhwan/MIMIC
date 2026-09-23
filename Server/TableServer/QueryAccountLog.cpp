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

std::future<BOOL> QueryManager::InsertAccountLog(
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
        const std::string& etc )
{
    std::ostringstream query;
    query << "INSERT INTO account_log (CODE, UID, ADMINID, ADID, ASSET, CMD, DEVICE, GMSESSID, IDPCODE, IPADDR, OSVER, "
        << "PF, SVCUID, ACCOUNTID, CHIP, CHIP_G, CHIP_S, COIN, COIN_G, COIN_S, SLOTCOIN, GEM, GEM_F, GEM_P, KICKOUTTICKET, "
        << "EXP, FRIEND_CNT, JOINTIME, LOGINTYPE, TYPE, NICKNAME, DELETETIME, REGISTERED, BLOCK_HOUR, LEFT_COUNT, LIMIT_TYPE, "
        << "REQTM, SECOND_PW, REASON, RESULT, _hash, VER, ETC) VALUES ("
        //<< "'" << logtm << "', "
        << code << ", "
        << "'" << uid << "', "
        << "'" << adminid << "', "
        << "'" << adid << "', "
        << "'" << asset << "', "
        << "'" << cmd << "', "
        << "'" << device << "', "
        << "'" << gmsessid << "', "
        << "'" << idpcode << "', "
        << "'" << ipaddr << "', "
        << "'" << osver << "', "
        << "'" << pf << "', "
        << "'" << svcuid << "', "
        << "'" << accountid << "', "
        << chip << ", "
        << chip_g << ", "
        << chip_s << ", "
        << coin << ", "
        << coin_g << ", "
        << coin_s << ", "
        << slotcoin << ", "
        << gem << ", "
        << gem_f << ", "
        << gem_p << ", "
        << kickoutticket << ", "
        << exp << ", "
        << friend_cnt << ", "
        << ( jointime.empty() ? "NULL" : "'" + jointime + "'" ) << ", "
        << "'" << logintype << "', "
        << "'" << type << "', "
        << "'" << nickname << "', "
        << "'" << deletetime << "', "
        << "'" << registered << "', "
        << block_hour << ", "
        << left_count << ", "
        << limit_type << ", "
        << ( reqtm.empty() ? "NULL" : "'" + reqtm + "'" ) << ", "
        << "'" << second_pw << "', "
        << "'" << reason << "', "
        << "'" << result << "', "
        << "'" << hash << "', "
        << "'" << ver << "', "
        << "'" << etc << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}

std::future<BOOL> QueryManager::InsertAssetLog(
    int code ,                             // 로그 코드
    const std::string& uid ,               // UID
    const std::string& cmd ,               // 시퀀스
    const std::string& ipaddr ,            // IP 정보
    const std::string& pf ,                // 스토어 플랫폼(2)
    const std::string& device ,            // 디바이스 모델 정보
    const std::string& idpcode ,           // 로그인 플랫폼
    const std::string& osver ,             // 디바이스 OS버전
    int64_t a_pgem ,                           // 획득한 유료 다이아
    int64_t a_fgem ,                           // 획득한 무료 다이아
    int64_t u_gem ,                            // 사용한 다이아 총 수량
    int64_t u_fgem ,                           // 사용한 무료 다이아 수량
    int64_t u_pgem ,                           // 사용한 유료 다이아 수량
    int64_t gem ,                              // 보유 다이아 (유료+무료)
    int64_t gem_f ,                            // 보유 무료 다이아
    int64_t gem_p ,                            // 보유 유료 다이아
    int64_t a_chip ,                       // 획득한 칩
    int64_t chip ,                         // 보유 칩 (소지+금고)
    int64_t chip_g ,                       // 소지한 칩
    int64_t chip_s ,                       // 금고 보유 칩
    int64_t a_coin ,                       // 획득한 코인 수량
    int64_t u_coin ,                           // 사용한 코인 수량
    int64_t coin ,                         // 보유 코인 (소지+금고)
    int64_t coin_g ,                       // 소지한 코인
    int64_t coin_s ,                       // 금고 보유 코인
    int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
    int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
    int64_t discard_chip ,                 // 초과 칩 삭제
    int64_t discard_coin ,                 // 초과 코인 삭제
    int price ,                            // 구매한 상품의 가격
    int refill_amount ,                    // 리필 금액
    int refill_today ,                     // 당일 리필 횟수
    const std::string& reward ,            // 설정된 보상
    const std::string& rwd_avatar ,        // 획득한 아바타
    const std::string& rwd_membership ,    // 획득한 멤버십
    int64_t amount ,                           // 이동 수량
    const std::string& move ,              // 이동 경로
    int rakeback ,                         // 레이크백 통장 남은 수량
    const std::string& rwd_data ,          // 보상 획득 데이터(사유)
    const std::string& rwd_list ,          // 획득한 보상 리스트
    const std::string& rwd_type ,          // 보상 타입
    const std::string& ver ,               // 접속한 게임 버전
    const std::string& etc )               // 기타
{
    std::ostringstream query;
    query << "INSERT INTO asset_log (CODE, UID, CMD, IPADDR, PF, DEVICE, IDPCODE, OSVER, "
        << "A_PGEM, A_FGEM, U_GEM, U_FGEM, U_PGEM, GEM, GEM_F, GEM_P, A_CHIP, CHIP, CHIP_G, CHIP_S, "
        << "A_COIN, U_COIN, COIN, COIN_G, COIN_S, KICKOUTTICKET, A_KICKOUTTICKET, DISCARD_CHIP, DISCARD_COIN, "
        << "PRICE, REFILL_AMOUNT, REFILL_TODAY, REWARD, RWD_AVATAR, RWD_MEMBERSHIP, AMOUNT, MOVE, RAKEBACK, "
        << "RWD_DATA, RWD_LIST, RWD_TYPE, VER, ETC) VALUES ("
        << code << ", "
        << "'" << uid << "', "
        << "'" << cmd << "', "
        << "'" << ipaddr << "', "
        << "'" << pf << "', "
        << "'" << device << "', "
        << "'" << idpcode << "', "
        << "'" << osver << "', "
        << a_pgem << ", "
        << a_fgem << ", "
        << u_gem << ", "
        << u_fgem << ", "
        << u_pgem << ", "
        << gem << ", "
        << gem_f << ", "
        << gem_p << ", "
        << a_chip << ", "
        << chip << ", "
        << chip_g << ", "
        << chip_s << ", "
        << a_coin << ", "
        << u_coin << ", "
        << coin << ", "
        << coin_g << ", "
        << coin_s << ", "
        << kickoutticket << ", "
        << a_kickoutticket << ", "
        << discard_chip << ", "
        << discard_coin << ", "
        << price << ", "
        << refill_amount << ", "
        << refill_today << ", "
        << "'" << reward << "', "
        << "'" << rwd_avatar << "', "
        << "'" << rwd_membership << "', "
        << amount << ", "
        << "'" << move << "', "
        << rakeback << ", "
        << "'" << rwd_data << "', "
        << "'" << rwd_list << "', "
        << "'" << rwd_type << "', "
        << "'" << ver << "', "
        << "'" << etc << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}