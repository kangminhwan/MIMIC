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

std::future<BOOL> QueryManager::InsertDailyLossBlock(
        int code ,
        const std::string& uid ,
        const std::string& datas ,
        const std::string& gmid ,
        const std::string& total
    )
{
    std::ostringstream query;
    query << "INSERT INTO daily_loss_limit_log (CODE, UID, DATAS, GAME_ID, TOTAL) VALUES ("
        //<< "'" << logtm << "', "
        << code << ", "
        << "'" << uid << "', "
        << "'" << datas << "', "
        << "'" << gmid << "', "
        << "'" << total << "')";

    return PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_LOG , query.str() );
}