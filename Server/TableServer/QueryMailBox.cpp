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

using protoutil::cProtoUtil;

void QueryManager::CreateTestMails( const uint64& player_idx )
{
#ifdef _DEBUG
 //   std::vector<std::future<BOOL>> results;
 //   results.push_back( PlayerExecuteQueryAsync( GenerateMailBoxInsertQuery( player_idx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_EventReward , 10000 , 0 , "test", "2024-04-30 23:59:59" ) ) );
 //   results.push_back( PlayerExecuteQueryAsync( GenerateMailBoxInsertQuery( player_idx , General::GrantItemKind::GrantItem_FreeCoin , General::InboxReason::InboxReason_EventReward , 10000 , 0 , "test" , "2024-04-30 23:59:59" ) ) );
	//results.push_back( PlayerExecuteQueryAsync( GenerateMailBoxInsertQuery( player_idx , General::GrantItemKind::GrantItem_Avatar , General::InboxReason::InboxReason_EventReward , 0 , 1 , "test" , "2024-04-30 23:59:59" ) ) );

 //   for ( const auto& future : results )
 //       future.wait();

 //   for ( auto& future : results ) {
 //       if ( false == future.get() ) {
 //           // 쿼리 실패에 대한 처리
 //       }
 //   }
#endif
}

std::string QueryManager::GenerateMailBoxInsertQuery(
        const uint64_t& player_idx ,
        const General::GrantItemKind& rewardType ,
        const General::InboxReason& mailType ,
		const uint64 count ,
		const int itemId ,
		const std::string& title ,
		const int period ,
        const std::string& expireDate ,
		int mail_state  )
{

    std::string rewardTypeString = cProtoUtil::GetEnumString( rewardType );
    std::string mailTypeString = cProtoUtil::GetEnumString( mailType );

    std::ostringstream oss;
    oss << "INSERT INTO mailbox (player_idx, reward_type, reward_type_string, mail_type, mail_type_string,mail_state, count, item_id, title, period, expire_date) VALUES ("
        << player_idx << ", "
        << rewardType << ", '"
        << rewardTypeString << "', "
        << mailType << ", '"
		<< mailTypeString << "', "
		<< mail_state << ", "
        << count << ", "
        << itemId << ", '"
		<< title << "', '"
		<< period<< "', '"
        << expireDate << "');";
    return oss.str();
}

std::string QueryManager::GenerateMailBoxDeleteQuery( const std::vector<uint64>& mailIdxList )
{
	if ( mailIdxList.empty() ) {
		return ""; // If the list is empty, return an empty string
	}

	std::ostringstream oss;
	oss << "DELETE FROM mailbox WHERE mail_idx IN (";
	for ( size_t i = 0; i < mailIdxList.size(); ++i ) {
		oss << mailIdxList[ i ];
		if ( i != mailIdxList.size() - 1 ) {
			oss << ", ";
		}
	}
	oss << ")";
	return oss.str();
}

std::future<BOOL> QueryManager::MailBoxGetAsync( const uint64& player_idx , std::vector<PmNet::InboxDetail>& mail_list )
{
	return std::async( std::launch::async , [&player_idx, &mail_list]() {
		return QueryManager::MailBoxGet( player_idx , mail_list );
	} );
}

BOOL QueryManager::MailBoxGet( const uint64& player_idx , std::vector<PmNet::InboxDetail>& mail_list )
{
	//printf( "MailBoxGet\n" );

	cConnInstance con_ins( E_DB_TYPE::E_DB_TYPE_SHARD );
	MYSQL* MySQLConnection = con_ins.GetConnection();
	if ( MySQLConnection == nullptr )
		return FALSE;

	MYSQL_STMT* stmt = mysql_stmt_init( MySQLConnection );
	if ( stmt == nullptr )
		return FALSE;

	PmNet::InboxDetail _mailinfo;

	std::string executeQuery = std::format( "select * from mailbox where player_idx = '{}' and expire_date >= NOW();" , player_idx );

	try
	{
		NetLib::cVector<cMySQLReader*> result_set;
		cMySQL::ExcuteQuery( MySQLConnection , executeQuery , result_set );

		// 데이터 읽기
		NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set.begin();
		NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set.end();
		for ( ; iter != iter_end; ++iter )
		{
			cMySQLReader* reader = ( *iter );
			if ( reader == nullptr )
				continue;

			_mailinfo.Clear();

			uint64 mail_idx = reader->GetLongLong( "mail_idx" );
			_mailinfo.set_inbox_idx( mail_idx );

			uint64 player_idx = reader->GetLongLong( "player_idx" );

			int reward_type = reader->GetLong( "reward_type" );
			_mailinfo.set_bounty_kind( static_cast<General::GrantItemKind>( reward_type ));

			std::string reward_type_string = reader->GetString( "reward_type_string" );

			int mail_type = reader->GetLong( "mail_type" );
			_mailinfo.set_inbox_kind( static_cast< General::InboxReason >( mail_type ) );

			switch( static_cast< General::GrantItemKind >( reward_type ) )
			{
			case General::GrantItemKind::GrantItem_FreeCoin:
				_mailinfo.set_fund_kind( General::AssetKind::AssetKind_Coin);
				break;
			case General::GrantItemKind::GrantItem_FreeChip:
				_mailinfo.set_fund_kind( General::AssetKind::AssetKind_Chip );
				break;
			}

			int mail_state = reader->GetTinyInt( "mail_state" );
			_mailinfo.set_inbox_state( static_cast< General::InboxState >( mail_state ) );

			std::string mail_type_string = reader->GetString( "mail_type_string" );

			uint64 count = reader->GetLongLong( "count" );
			_mailinfo.set_cnt( count );

			int item_id = reader->GetLong( "item_id" );
			_mailinfo.set_item_no( item_id );

			std::string title = reader->GetString( "title" );
			_mailinfo.set_headline( title );

			std::string desc = reader->GetString( "desc" );
			_mailinfo.set_memo( desc );

			int period = reader->GetLong( "period" );
			_mailinfo.set_span( period );
			/*std::wstring utf_title = StringUtil::Utf8ToWide( title );
			title = StringUtil::WideToUtf8( utf_title );
			_mailinfo.set_headline( title );

			auto test = StringUtil::WstringToUtf8( utf_title );
			std::string_view title_utf8View( reinterpret_cast< const char* >( test.data() ) , test.size() );
			_mailinfo.set_headline( title_utf8View.data() );

			string adflajds = _mailinfo.headline();*/

			MYSQL_TIME expire_date = reader->GetDateTime( "expire_date" );

			//std::string test = reader->GetString( "expire_date" );

			std::string expire_date_string = TimeUtils::MYSQLTimeToString( expire_date );
			_mailinfo.set_expire_ts( expire_date_string );

			mail_list.push_back( _mailinfo );
		}

		// 리소스 반환
		NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	}
	catch ( const runtime_error& e )
	{
		printf( e.what() );
	}

	if ( stmt != nullptr )
		mysql_stmt_close( stmt );

	return TRUE;
}

BOOL QueryManager::DeleteMailBox( std::vector<uint64> delete_list )
{
	std::future<BOOL> result;
	std::string query = GenerateMailBoxDeleteQuery( delete_list );
	result = PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , query );

	result.wait();

	return result.get();
}