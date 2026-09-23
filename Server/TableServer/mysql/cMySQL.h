#pragma once
#include "../TableServerHeader.h"

//#include <my_global.h>
#include <mysql.h>

#pragma comment(lib, "libmysql.lib")

#include "cParamBinder.h"
#include "cMySQLReader.h"
#include "cMySQLConnectionPooler.h"

class FFError
{
public:
	std::string    Label;

	FFError() { Label = (char *)"Generic Error"; }
	FFError(char *message) { Label = message; }
	~FFError() { }
	inline const char*   GetMessage(void) { return Label.c_str(); }
};

static void test_error(MYSQL *mysql, int status)
{
	if (status)
	{
		fprintf(stderr, "Error: %s (errno: %d)\n", mysql_error(mysql), mysql_errno(mysql));
		exit(1);
	}
}

static void stmt_error(MYSQL_STMT *stmt, int status)
{
	if (status)
	{
		fprintf(stderr, "Error: %s (errno: %d)\n", mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
		//exit(1);
	}
}

static void stmt_error_string(MYSQL_STMT* stmt, int status, std::string& error_string)
{
	char error_string_buffer[256] = { 0 };
	if (status)
	{
		snprintf(error_string_buffer, sizeof(error_string), "Error: %s (errno: %d)\n", mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
		error_string = error_string_buffer;
	}
}

static void print_dashes(MYSQL_RES *res_set)
{
	MYSQL_FIELD   *field;
	unsigned int  i, j;

	mysql_field_seek(res_set, 0);
	fputc('+', stdout);
	for (i = 0; i < mysql_num_fields(res_set); i++)
	{
		field = mysql_fetch_field(res_set);
		for (j = 0; j < field->max_length + 2; j++)
			fputc('-', stdout);
		fputc('+', stdout);
	}
	fputc('\n', stdout);
}

static void process_result_set(MYSQL *conn, MYSQL_RES *res_set)
{
	MYSQL_ROW     row;
	/* #@ _COL_WID_CALC_VARS_ */
	MYSQL_FIELD   *field;
	unsigned long col_len;
	unsigned int  i;
	/* #@ _COL_WID_CALC_VARS_ */

	/* #@ _COL_WID_CALCULATIONS_ */
	/* determine column display widths; requires result set to be */
	/* generated with mysql_store_result(), not mysql_use_result() */
	mysql_field_seek(res_set, 0);
	for (i = 0; i < mysql_num_fields(res_set); i++)
	{
		field = mysql_fetch_field(res_set);
		col_len = static_cast<unsigned long>(strlen(field->name));
		if (col_len < field->max_length)
			col_len = field->max_length;
		if (col_len < 4 && !IS_NOT_NULL(field->flags))
			col_len = 4;  /* 4 = length of the word "NULL" */
		field->max_length = col_len;  /* reset column info */
	}
	/* #@ _COL_WID_CALCULATIONS_ */

	print_dashes(res_set);
	fputc('|', stdout);
	mysql_field_seek(res_set, 0);
	for (i = 0; i < mysql_num_fields(res_set); i++)
	{
		field = mysql_fetch_field(res_set);
		/* #@ _PRINT_TITLE_ */
		printf(" %-*s |", (int)field->max_length, field->name);
		/* #@ _PRINT_TITLE_ */
	}
	fputc('\n', stdout);
	print_dashes(res_set);

	while ((row = mysql_fetch_row(res_set)) != NULL)
	{
		mysql_field_seek(res_set, 0);
		fputc('|', stdout);
		for (i = 0; i < mysql_num_fields(res_set); i++)
		{
			field = mysql_fetch_field(res_set);
			/* #@ _PRINT_ROW_VAL_ */
			if (row[i] == NULL)       /* print the word "NULL" */
				printf(" %-*s |", (int)field->max_length, "NULL");
			else if (IS_NUM(field->type))  /* print value right-justified */
				printf(" %*s |", (int)field->max_length, row[i]);
			else              /* print value left-justified */
				printf(" %-*s |", (int)field->max_length, row[i]);
			/* #@ _PRINT_ROW_VAL_ */
		}
		fputc('\n', stdout);
	}
	print_dashes(res_set);
	printf("Number of rows returned: %lu\n",
		(unsigned long)mysql_num_rows(res_set));
}

class cMySQL
{
protected:
	std::vector<MYSQL*> m_mysql_connections;

	NetLib::cPool<cMySQLReader> m_reader_pool;

private:
	cMySQLReader* pop_my_sql_reader();

public:
	cMySQL();
	~cMySQL();

	void Init();
	//MYSQL* GetConnection(const int threadArray);
	void Ping(const int db_idx); // 일단 shard 핑만 처리
	BOOL SelectDB(MYSQL* MySQLConnection, std::string dbname);
	static BOOL ExcuteProcedure(MYSQL* MySQLConnection, cParamBinder& parambinder, std::string query, NetLib::cVector<cMySQLReader*>& result_set);
	static BOOL ExcuteQuery(MYSQL* MySQLConnection, std::string query, NetLib::cVector<cMySQLReader*>& result_set);

	void free_my_sql_reader(cMySQLReader* preader);
	void free_my_sql_reader(NetLib::cVector<cMySQLReader*>& _vec);
	void free_my_sql_reader(std::vector<cMySQLReader*>& _vec);

	// global db config 읽어오기
	//BOOL GetShardInfo();

	int CallProcedure();
	int CallQuery();
	BOOL TransActionTestHyun(const int db_idx, long _hero_idx, int _add_honor_point);

	static std::string MySQLTimeToString( const MYSQL_TIME& mysql_time )
	{
		return std::format( "{:04}-{:02}-{:02} {:02}:{:02}:{:02}" ,
						   mysql_time.year , mysql_time.month , mysql_time.day ,
						   mysql_time.hour , mysql_time.minute , mysql_time.second );
	}
	bool IsDangerousQuery( const std::string& query );
};

