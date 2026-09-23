#include "cMySQL.h"
#include "cMySQLParserElement.h"
#include "cMySQLReader.h"
//#include "cParamBinder.h"

#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"

#include "cMySQLConnectionPooler.h"
//#include "../StringUtil.h"

cMySQL::cMySQL()
{
	Init();
}


cMySQL::~cMySQL()
{
	for (int n = 0; n < m_mysql_connections.size(); ++n)
	{
		// 케넥션 종료 합니다.
		mysql_close(m_mysql_connections[n]);
	}
}

void cMySQL::Init()
{
	TServerConfiguration* pTServerConfiguration = NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->GetConfiguration();
	if (pTServerConfiguration == nullptr)
		throw "cMySQL::Init() Failed, ServerConfiguration is nullptr";

	// Account DB

	if (_tcslen(pTServerConfiguration->accountDB.szHost) <= 0)
		throw "cMySQL::Init() Failed, ServerConfiguration szHost is nullptr";

	if (pTServerConfiguration->accountDB.nPORT <= 0)
		throw "cMySQL::Init() Failed, ServerConfiguration nPORT Check Please";

	if (_tcslen(pTServerConfiguration->accountDB.szUser) <= 0)
		throw "cMySQL::Init() Failed, ServerConfiguration szUser is nullptr";

	if (_tcslen(pTServerConfiguration->accountDB.szPASS) <= 0)
		throw "cMySQL::Init() Failed, ServerConfiguration szPASS is nullptr";

	if (_tcslen(pTServerConfiguration->accountDB.szNAME) <= 0)
		throw "cMySQL::Init() Failed, ServerConfiguration szNAME is nullptr";

	stMysqlConnectionInfo global_db_info;

	global_db_info.private_ip = ATL::CW2A(pTServerConfiguration->accountDB.szHost).m_psz;
	global_db_info.auth_id = ATL::CW2A(pTServerConfiguration->accountDB.szUser).m_psz;
	global_db_info.auth_pw = ATL::CW2A(pTServerConfiguration->accountDB.szPASS).m_psz;
	global_db_info.db_name = ATL::CW2A(pTServerConfiguration->accountDB.szNAME).m_psz;
	global_db_info.private_port = pTServerConfiguration->accountDB.nPORT;

	// cMySQLConnectionPooler 인스턴스 생성
	NetLib::cSingleton<cMySQLConnectionPooler>::GetInstance()->SetAccountDB(global_db_info);

	stMysqlConnectionInfo log_db_info;

	log_db_info.private_ip = ATL::CW2A(pTServerConfiguration->szLogHost).m_psz;
	log_db_info.auth_id = ATL::CW2A(pTServerConfiguration->szLogUser).m_psz;
	log_db_info.auth_pw = ATL::CW2A(pTServerConfiguration->szLogPASS).m_psz;
	log_db_info.db_name = ATL::CW2A(pTServerConfiguration->szLogNAME).m_psz;
	log_db_info.private_port = pTServerConfiguration->nLogPORT;

	NetLib::cSingleton<cMySQLConnectionPooler>::GetInstance()->SetLogDB( log_db_info );

	m_reader_pool.Create(0, 32000 );

	// game db 군 추가
	stMysqlConnectionInfo game_dbs[ CSDef::EDef::MAX_SHARD_DB_CNT ];
	for ( int n = 0; n < CSDef::EDef::MAX_SHARD_DB_CNT; ++n ) {

		UINT game_db_port = pTServerConfiguration->gameDBs[ n ].nPORT;

		if ( game_db_port == 0 ) continue;

		game_dbs[ n ].private_ip = ATL::CW2A( pTServerConfiguration->gameDBs[ n ].szHost).m_psz;
		game_dbs[ n ].auth_id = ATL::CW2A( pTServerConfiguration->gameDBs[ n ].szUser ).m_psz;
		game_dbs[ n ].auth_pw = ATL::CW2A( pTServerConfiguration->gameDBs[ n ].szPASS ).m_psz;
		game_dbs[ n ].db_name = ATL::CW2A( pTServerConfiguration->gameDBs[ n ].szNAME ).m_psz;
		game_dbs[ n ].private_port = game_db_port;


		NetLib::cSingleton<cMySQLConnectionPooler>::GetInstance()->InsertGameConnectionInfo( game_dbs[ n ] );
	}

	m_reader_pool.Create( 0 , 32000 );

	// Create Global MySQL DB Connections
	//MYSQL *MySQLConRet, *MySQLConnection;

	//MySQLConRet = MySQLConnection = nullptr;

	//// 초기화
	//MySQLConnection = mysql_init(nullptr);

	//// DB 연결
	//try
	//{
	//	MySQLConRet = mysql_real_connect(MySQLConnection,
	//		ATL::CW2A(pTServerConfiguration->szHost).m_psz,
	//		ATL::CW2A(pTServerConfiguration->szUser).m_psz,
	//		ATL::CW2A(pTServerConfiguration->szPASS).m_psz,
	//		ATL::CW2A(pTServerConfiguration->szNAME).m_psz,
	//		pTServerConfiguration->nPORT,
	//		NULL,
	//		CLIENT_MULTI_STATEMENTS);

	//	if (MySQLConRet == NULL)
	//		throw FFError((char*)mysql_error(MySQLConnection));

	//	//mysql_autocommit(MySQLConnection, bool(true));  // set autocommit to true

	//	// enable auto reconnect option
	//	// when you attempt to send a statement to the server to be executed. If auto-reconnect is enabled, the library tries once to reconnect to the server and send the statement again. 
	//	bool reconnect = true;
	//	mysql_options(MySQLConnection, MYSQL_OPT_RECONNECT, &reconnect);

	//}
	//catch (FFError e)
	//{
	//	throw e.Label.c_str();
	//}
}

//MYSQL* cMySQL::GetConnection(const int threadArray)
//{
//	if (threadArray < 0 || threadArray > m_mysql_connections.size())
//		return nullptr;
//
//	MYSQL* pMYSQL = m_mysql_connections[threadArray];
//	if (pMYSQL == nullptr)
//		return nullptr;
//
//	return pMYSQL;
//}

void cMySQL::Ping(const int db_idx)
{
	cConnInstance instance(E_DB_TYPE::E_DB_TYPE_SHARD, db_idx);
	MYSQL* MySQLConnection = instance.GetConnection();
	if (MySQLConnection == nullptr)
	{
		// add error code
		return;
	}

	//If the connection has gone down, the effect of mysql_ping() depends on the auto-reconnect state. If auto-reconnect is enabled, mysql_ping() performs a reconnect. 
	mysql_ping(MySQLConnection);
}

BOOL cMySQL::SelectDB(MYSQL* MySQLConnection, string dbname)
{
	if (MySQLConnection == nullptr)
		return FALSE;

	int status = mysql_select_db(MySQLConnection, dbname.c_str());		// select db

	return status == 0 ? TRUE : FALSE;
}

BOOL cMySQL::ExcuteProcedure(MYSQL* MySQLConnection, cParamBinder& parambinder, string query, NetLib::cVector<cMySQLReader*>& result_set)
{
	if (MySQLConnection == nullptr)
		return FALSE;

	MYSQL_STMT *stmt = nullptr;
	int        status;

	/* initialize and prepare CALL statement with parameter placeholders */
	stmt = mysql_stmt_init(MySQLConnection);
	if (stmt == nullptr)
		goto failure;

	status = mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()));
	if (status)
		goto failure;

	// bind parameters
	status = mysql_stmt_bind_param(stmt, parambinder.GetBinder());
	if (status)
		goto failure;

	status = mysql_stmt_execute(stmt);
	if (status)
		goto failure;

	/* process results until there are no more */
	do {
		int i;
		int num_fields;       /* number of columns in result */
		MYSQL_FIELD *fields;  /* for result set metadata */
		MYSQL_BIND *rs_bind;  /* for output buffers */

							  /* the column count is > 0 if there is a result set */
							  /* 0 if the result is only the final status packet */
		num_fields = mysql_stmt_field_count(stmt);

		if (num_fields > 0)
		{
			cMySQLReader* pReader = NetLib::cSingleton<cMySQL>::GetInstance()->pop_my_sql_reader();

			/* what kind of result set is this? */
			/*if (MySQLConnection->server_status & SERVER_PS_OUT_PARAMS)
			printf("this result set contains OUT/INOUT parameters\n");
			else
			printf("this result set is produced by the procedure\n");*/

			MYSQL_RES *rs_metadata = mysql_stmt_result_metadata(stmt);
			stmt_error(stmt, rs_metadata == NULL);

			fields = mysql_fetch_fields(rs_metadata);

			rs_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * num_fields);
			memset(rs_bind, 0, sizeof(MYSQL_BIND) * num_fields);

			/* set up and bind result set output buffers */
			for (i = 0; i < num_fields; ++i)
			{
				cMySQLParserBase* pMySQLParserBase = pReader->PushField(fields[i].type, fields[i], fields[i].max_length);
				if (pMySQLParserBase == nullptr)
				{
					fprintf(stderr, "ERROR: unexpected type: %d.\n", fields[i].type);
					continue;
				}

				pMySQLParserBase->SetField(&fields[i], rs_bind[i]);
			}

			status = mysql_stmt_bind_result(stmt, rs_bind);
			if (status > 0)
				goto failure;

			/* fetch rows */
			while (true)
			{
				status = mysql_stmt_fetch(stmt);
				if (status == 1 || status == MYSQL_NO_DATA)
				{
					NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( pReader );
					break;
				}
				else
				{
					// 결과 읽어오기
					if (pReader != nullptr)
						result_set.push_back(pReader);

					pReader = nullptr;
				}
			}

			mysql_free_result(rs_metadata); /* free metadata */
			free(rs_bind);                  /* free output buffers */
		}

		/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		status = mysql_stmt_next_result(stmt);
		if (status > 0)
			goto failure;

	} while (status == 0);

	if (stmt != nullptr)
		mysql_stmt_close(stmt);

	return TRUE;

failure:
	if (stmt != nullptr)
	{
		fprintf(stderr, "Error: %s (errno: %d)\n", mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
		mysql_stmt_close(stmt);
	}
	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );
	return FALSE;
}

cMySQLReader* cMySQL::pop_my_sql_reader()
{
	return m_reader_pool.Pop();
}

void cMySQL::free_my_sql_reader(cMySQLReader* preader)
{
	if (preader != nullptr)
	{
		preader->free_all_mysql_parser_base();
		m_reader_pool.Push(preader);
	}
}

void cMySQL::free_my_sql_reader(NetLib::cVector<cMySQLReader*>& _vec)
{
	NetLib::cVector<cMySQLReader*>::iterator iter = _vec.begin();
	NetLib::cVector<cMySQLReader*>::iterator iter_end = _vec.end();
	for (; iter != iter_end; ++iter)
	{
		(*iter)->free_all_mysql_parser_base();
		m_reader_pool.Push((*iter));
	}

	_vec.clear();
}

void cMySQL::free_my_sql_reader(std::vector<cMySQLReader*>& _vec)
{
	std::vector<cMySQLReader*>::iterator iter = _vec.begin();
	std::vector<cMySQLReader*>::iterator iter_end = _vec.end();
	for (; iter != iter_end; ++iter)
	{
		(*iter)->free_all_mysql_parser_base();
		m_reader_pool.Push((*iter));
	}

	_vec.clear();
}

BOOL cMySQL::ExcuteQuery(MYSQL* MySQLConnection, std::string query, NetLib::cVector<cMySQLReader*>& result_set)
{
	if (MySQLConnection == nullptr)
		return FALSE;

	MYSQL_FIELD   *field;
	MYSQL_RES *sql_result = nullptr;
	MYSQL_ROW sql_row;
	int status;
	if ( std::count( query.begin() , query.end() , ';' ) >= 2 )
		goto failure;
	status = mysql_query(MySQLConnection, query.c_str());
	// Check connection before setting charset
	if ( !cMySQLConnectionPooler::ReconnectIfNeeded( MySQLConnection ) ) {
		fprintf( stderr , "MySQL connection lost and reconnection failed\n" );
		goto failure;
	}
	if (status != 0)
	{
		fprintf(stderr, "Mysql query error : %s", mysql_error(MySQLConnection));
		goto failure;
	}

	sql_result = mysql_store_result(MySQLConnection);
	if (sql_result == nullptr)
		goto failure;

	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		int field_count = mysql_num_fields(sql_result);

		// mysql reader instance
		cMySQLReader* pReader = NetLib::cSingleton<cMySQL>::GetInstance()->pop_my_sql_reader();

		mysql_field_seek(sql_result, 0);

		for (int i = 0; i < field_count; i++)
		{
			field = mysql_fetch_field(sql_result);

			//std::string string( sql_row [ i ]);

			//auto test = StringUtil::Utf8ToWide( sql_row[ i ] );

			cMySQLParserBase* pMySQLParserBase = pReader->PushField(field->type, *field, field->max_length);
			if (pMySQLParserBase == nullptr)
				throw "unknown field type, need more work for parsing";

			// 여기에서 읽어야 해요 ㅠㅠ..
			ReadField(pMySQLParserBase, sql_row[i]);
		}

		result_set.push_back(pReader);
	}
	mysql_free_result( sql_result );

	return TRUE;

failure:
	NetLib::cSingleton<cMySQL>::GetInstance()->free_my_sql_reader( result_set );

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_CRI , mysql_error( MySQLConnection ) );
	if (status != 0)
	{
		fprintf(stderr, "Mysql query error : %s", mysql_error(MySQLConnection));
		
	}

	if (sql_result != nullptr)
	{
		fprintf(stderr, "Error: %s (errno: %d)\n", mysql_error(MySQLConnection), mysql_errno(MySQLConnection));
		mysql_free_result(sql_result);
	}

	return FALSE;
}

/*
BOOL cMySQL::GetShardInfo()
{
	cConnInstance instance(E_DB_TYPE::E_DB_TYPE_ACCOUNT);
	MYSQL* MySQLConnection = instance.GetConnection();
	if (MySQLConnection == nullptr)
	{
		// add error code
		return FALSE;
	}

	MYSQL_FIELD   *field = nullptr;
	MYSQL_RES *sql_result = nullptr;
	MYSQL_ROW sql_row = nullptr;
	int status = 0;

	// Select 쿼리문
	const char *query = "SELECT db_idx, db_group, db_type, db_name, private_ip, private_port, public_ip, public_port, auth_id, auth_pw, status FROM db_config \
					where db_type = 'sharding' or db_type = 'log';";

	mysql_redis_resource_helper _result_helper(resource_db_type::rdt_my_sql);

	NetLib::cVector<cMySQLReader*>* result_set = _result_helper.get_my_sql_reader();
	BOOL bResult = cMySQL::ExcuteQuery(MySQLConnection, query, *result_set);
	if (bResult == FALSE)
	{
		return FALSE;
	}

	NetLib::cVector<cMySQLReader*>::const_iterator iter = result_set->begin();
	NetLib::cVector<cMySQLReader*>::const_iterator iter_end = result_set->end();
	for (; iter != iter_end; ++iter)
	{
		cMySQLReader* pReader = (*iter);

		stMysqlConnectionInfo db_info = ShardInfo_DB_Parse(pReader);

		// db 추가
		if (db_info.db_type.compare("sharding") == 0 || db_info.db_type.compare("log") == 0)
			NetLib::cSingleton<cMySQLConnectionPooler>::GetInstance()->InsertConnectionInfo(db_info);
	}

	return TRUE;
}
*/


int cMySQL::CallProcedure()
{
	MYSQL* MySQLConnection = NetLib::cSingleton<cMySQLConnectionPooler>::GetInstance()->PopShard(1);
	if (MySQLConnection == nullptr)
	{
		// add error code
		return 1;
	}

	MY_CHARSET_INFO charset;
	mysql_get_character_set_info(MySQLConnection, &charset);

	MYSQL_STMT *stmt;
	MYSQL_BIND ps_params[3];  /* input parameter buffers */
	int        int_data[3];   /* input/output values */
	bool    is_null[3];    /* output value nullability */
	int        status;

	/* set up stored procedure */
	status = mysql_query(MySQLConnection, "DROP PROCEDURE IF EXISTS p1");
	test_error(MySQLConnection, status);

	status = mysql_query(MySQLConnection,
		"CREATE PROCEDURE p1("
		"  IN p_in INT, "
		"  OUT p_out INT, "
		"  INOUT p_inout INT) "
		"BEGIN "
		"  SELECT p_in, p_out, p_inout; "
		"  SET p_in = 100, p_out = 200, p_inout = 300; "
		"  SELECT p_in, p_out, p_inout; "
		"END");
	test_error(MySQLConnection, status);

	/* initialize and prepare CALL statement with parameter placeholders */
	stmt = mysql_stmt_init(MySQLConnection);
	if (!stmt)
	{
		printf("Could not initialize statement\n");
		exit(1);
	}
	status = mysql_stmt_prepare(stmt, "CALL p1(?, ?, ?)", 16);
	stmt_error(stmt, status);

	/* initialize parameters: p_in, p_out, p_inout (all INT) */
	memset(ps_params, 0, sizeof(ps_params));

	ps_params[0].buffer_type = MYSQL_TYPE_LONG;
	ps_params[0].buffer = (char *)&int_data[0];
	ps_params[0].length = 0;
	ps_params[0].is_null = 0;

	ps_params[1].buffer_type = MYSQL_TYPE_LONG;
	ps_params[1].buffer = (char *)&int_data[1];
	ps_params[1].length = 0;
	ps_params[1].is_null = 0;

	ps_params[2].buffer_type = MYSQL_TYPE_LONG;
	ps_params[2].buffer = (char *)&int_data[2];
	ps_params[2].length = 0;
	ps_params[2].is_null = 0;

	/* bind parameters */
	status = mysql_stmt_bind_param(stmt, ps_params);
	stmt_error(stmt, status);

	/* assign values to parameters and execute statement */
	int_data[0] = 10;  /* p_in */
	int_data[1] = 20;  /* p_out */
	int_data[2] = 30;  /* p_inout */

	status = mysql_stmt_execute(stmt);
	stmt_error(stmt, status);

	/* process results until there are no more */
	do {
		int i;
		int num_fields;       /* number of columns in result */
		MYSQL_FIELD *fields;  /* for result set metadata */
		MYSQL_BIND *rs_bind;  /* for output buffers */

							  /* the column count is > 0 if there is a result set */
							  /* 0 if the result is only the final status packet */
		num_fields = mysql_stmt_field_count(stmt);

		if (num_fields > 0)
		{
			/* there is a result set to fetch */
			printf("Number of columns in result: %d\n", (int)num_fields);

			/* what kind of result set is this? */
			printf("Data: ");
			if (MySQLConnection->server_status & SERVER_PS_OUT_PARAMS)
				printf("this result set contains OUT/INOUT parameters\n");
			else
				printf("this result set is produced by the procedure\n");

			MYSQL_RES *rs_metadata = mysql_stmt_result_metadata(stmt);
			stmt_error(stmt, rs_metadata == NULL);

			fields = mysql_fetch_fields(rs_metadata);

			rs_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * num_fields);
			if (!rs_bind)
			{
				printf("Cannot allocate output buffers\n");
				exit(1);
			}
			memset(rs_bind, 0, sizeof(MYSQL_BIND) * num_fields);

			/* set up and bind result set output buffers */
			for (i = 0; i < num_fields; ++i)
			{
				rs_bind[i].buffer_type = fields[i].type;
				rs_bind[i].is_null = &is_null[i];

				switch (fields[i].type)
				{
				case MYSQL_TYPE_LONG:
					rs_bind[i].buffer = (char *) &(int_data[i]);
					rs_bind[i].buffer_length = sizeof(int_data);
					break;

				default:
					fprintf(stderr, "ERROR: unexpected type: %d.\n", fields[i].type);
					exit(1);
				}
			}

			status = mysql_stmt_bind_result(stmt, rs_bind);
			stmt_error(stmt, status);

			/* fetch and display result set rows */
			while (1)
			{
				status = mysql_stmt_fetch(stmt);

				if (status == 1 || status == MYSQL_NO_DATA)
					break;

				for (i = 0; i < num_fields; ++i)
				{
					switch (rs_bind[i].buffer_type)
					{
					case MYSQL_TYPE_LONG:
						if (*rs_bind[i].is_null)
							printf(" val[%d] = NULL;", i);
						else
							printf(" val[%d] = %ld;",
								i, (long) *((int *)rs_bind[i].buffer));
						break;

					default:
						printf("  unexpected type (%d)\n",
							rs_bind[i].buffer_type);
					}
				}
				printf("\n");
			}

			mysql_free_result(rs_metadata); /* free metadata */
			free(rs_bind);                  /* free output buffers */
		}
		else
		{
			/* no columns = final status packet */
			printf("End of procedure output\n");
		}

		/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		status = mysql_stmt_next_result(stmt);
		if (status > 0)
			stmt_error(stmt, status);
	} while (status == 0);

	mysql_stmt_close(stmt);

	return 0;
}

int cMySQL::CallQuery()
{
	cConnInstance instance(E_DB_TYPE::E_DB_TYPE_SHARD, 1);
	MYSQL* MySQLConnection = instance.GetConnection();
	if (MySQLConnection == nullptr)
	{
		// add error code
		return 1;
	}

	printf("MySQL client version: %s\n", mysql_get_client_info());

	MYSQL_RES *sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	MY_CHARSET_INFO charset;
	mysql_get_character_set_info(MySQLConnection, &charset);
	// Check connection before setting charset
	if ( !cMySQLConnectionPooler::ReconnectIfNeeded( MySQLConnection ) ) {
		fprintf( stderr , "MySQL connection lost and reconnection failed\n" );
		return 0;
	}
	//한글사용을위해추가.
	mysql_query(MySQLConnection, "set session character_set_connection=euckr;");
	mysql_query(MySQLConnection, "set session character_set_results=euckr;");
	mysql_query(MySQLConnection, "set session character_set_client=euckr;");


	// Select 쿼리문
	const char *query = "SELECT * FROM hero order by hero_idx asc limit 100";
	query_stat = mysql_query(MySQLConnection, query);
	if (query_stat != 0)
	{
		fprintf(stderr, "Mysql query error : %s", mysql_error(MySQLConnection));
		return 1;
	}

	// 결과출력
	sql_result = mysql_store_result(MySQLConnection);
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		printf("%2s %2s %s\n", sql_row[0], sql_row[1], sql_row[2]);
	}
	mysql_free_result(sql_result);

	// DB 연결닫기
	//mysql_close(MySQLConnection);

	return 0;
}

BOOL cMySQL::TransActionTestHyun(const int db_idx, long _hero_idx, int _add_honor_point)
{
	cConnInstance instance(E_DB_TYPE::E_DB_TYPE_SHARD, db_idx);
	MYSQL* MySQLConnection = instance.GetConnection();
	if (MySQLConnection == nullptr)
	{
		// add error code
		return FALSE;
	}

	//mysql_select_db(MySQLConnection, "IVE");		// select db
	bool bSuccess = mysql_autocommit(MySQLConnection, bool(false));  // set autocommit to false

	// set query
	std::string query = "CALL AddHonorPoint(6083, 100); CALL AddHonorPoint(1, 100);";

	int status = mysql_query(MySQLConnection, query.c_str());

	MYSQL_STMT *stmt = nullptr;

	/* initialize and prepare CALL statement with parameter placeholders */
	stmt = mysql_stmt_init(MySQLConnection);
	if (stmt == nullptr)
		return FALSE;
	
	status = mysql_stmt_prepare(stmt, query.c_str(), static_cast<unsigned long>(query.length()));
	if (status)
	{
		return FALSE;
	}

	do
	{
		MYSQL_RES* result = mysql_store_result(MySQLConnection);
		if (result)
		{
			process_result_set(MySQLConnection, result);
			mysql_free_result(result);
		}
		else
		{
			if (mysql_field_count(MySQLConnection) == 0)
			{
				printf("%lld rows affected\n",
				mysql_affected_rows(MySQLConnection));
			}
			else  /* some error occurred */
			{
				printf("Could not retrieve result set\n");
				break;
			}
		}

		if ((status = mysql_next_result(MySQLConnection)) > 0)
			printf("Could not execute statement\n");

	} while (status == 0);
	my_ulonglong affcted_rows = mysql_affected_rows(MySQLConnection);
	if (affcted_rows < 1)
		goto failure;

	//mysql_real_query(MySQLConnection, "COMMIT", 6);
	bSuccess = mysql_commit(MySQLConnection);
	bSuccess = mysql_autocommit(MySQLConnection, bool(true));  // set autocommit to false

	return TRUE;

failure:
	/*mysql_rollback(MySQLConnection);
	if (stmt != nullptr)
		mysql_stmt_close(stmt);*/
	
	if (stmt != nullptr)
	{
		fprintf(stderr, "Error: %s (errno: %d)\n", mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
		mysql_stmt_close(stmt);
	}
	
	mysql_real_query(MySQLConnection, "ROLLBACK", 8);
	//mysql_rollback(MySQLConnection);

	return FALSE;
}

bool cMySQL::IsDangerousQuery( const std::string& query )
{
	std::string lower = query;
	std::transform( lower.begin() , lower.end() , lower.begin() , ::tolower );
	static const std::vector<std::string> blacklist = {
		"drop ", "truncate ", "shutdown", "alter ", "grant ", "revoke "
	};

	for ( auto& word : blacklist )
	{
		if ( lower.find( word ) != std::string::npos )
			return true;
	}
	return false;
}
