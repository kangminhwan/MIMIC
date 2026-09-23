#pragma once
#include "../TableServerHeader.h"

#include <map>
#include <mysql.h>
#include <json/json.h>

#include "cMySQLParserElement.h"

class cMySQLReader
{
private:
	std::map<std::string, cMySQLParserBase*> m_row;

	NetLib::cPool<cMySQLParserTinyInt> m_pool_tiny_int;
	NetLib::cPool<cMySQLParserLong> m_pool_long;
	NetLib::cPool<cMySQLParserLongLong> m_pool_longlong;
	NetLib::cPool<cMySQLParserDouble> m_pool_double;
	NetLib::cPool<cMySQLParserString> m_pool_string;
	NetLib::cPool<cMySQLParserDateTime> m_pool_date_time;
	NetLib::cPool<cMySQLParserJSON> m_pool_json;

private:
	cMySQLParserBase* pop_mysql_parser_base(enum_field_types type);

public:
	cMySQLReader();
	~cMySQLReader();

	void free_all_mysql_parser_base();

	cMySQLParserBase* PushField(enum_field_types type, MYSQL_FIELD field, unsigned long max_length);

	bool isLong(std::string field_name);	// (4Byte)LONG 형인지 검사한다. mysql LONG => int
	bool isLongLong(std::string field_name);// (8Byte)LongLong 형인지 검사한다. mysql LongLong => __int64
	bool IsTinyInt(std::string field_name);// (1Byte)TINYINT 형인지 검사한다. mysql TINYINT => char, unsigned char, BYTE
	bool isDouble(std::string field_name);// (8Byte)double 형인지 검사한다. mysql double => double
	bool isString(std::string field_name);// string => char* 형인지 검사한다.
	bool isDateTime(std::string field_name); // DATETIME 타입인지 검사한다.
	bool isJson(std::string field_name); // Json 타입인지 검사한다.

	int  GetLong(std::string field_name);
	__int64 GetLongLong(std::string field_name);
	BYTE GetTinyInt(std::string field_name);
	double GetDouble(std::string field_name);
	char* GetString(std::string field_name);
	MYSQL_TIME GetDateTime(std::string field_name);
	json GetJson( std::string field_name );
};

static stMysqlConnectionInfo ShardInfo_DB_Parse(cMySQLReader* reader)
{
	if (reader == nullptr)
		throw "ShardInfo_DB_Parse reader is nullptr";

	stMysqlConnectionInfo db_info;

	//db_idx, db_group, db_type, db_name, private_ip, private_port, public_ip, public_port, auth_id, auth_pw, status,
	db_info.db_idx = reader->GetLong("db_idx");
	db_info.db_group = reader->GetLong("db_group");
	db_info.db_type = reader->GetString("db_type");
	db_info.db_name = reader->GetString("db_name");
	db_info.private_ip = reader->GetString("private_ip");
	db_info.private_port = reader->GetLong("private_port");
	db_info.auth_id = reader->GetString("auth_id");
	db_info.auth_pw = reader->GetString("auth_pw");

	return db_info;
}