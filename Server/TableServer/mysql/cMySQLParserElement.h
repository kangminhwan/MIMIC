#pragma once

#include <iostream>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip> // std::get_time을 사용하기 위한 <iomanip> 헤더 포함
#include <json/json.h>
#include <mysql_com.h>
#include <nlohmann/json.hpp>

#include "DefProcedure.h"
using json = nlohmann::json;
class cMySQLParserBase
{
public:
	enum_field_types m_field_type;
	bool m_isnull;
	std::string m_field;

	void Clear()
	{
		m_isnull = false;
	}

	void SetField(MYSQL_FIELD* pMYSQL_FIELD, MYSQL_BIND& rs_bind)
	{
		if (pMYSQL_FIELD == nullptr)
			return;

		rs_bind.buffer_type = pMYSQL_FIELD->type;
		rs_bind.is_null = GetBool();
		rs_bind.buffer = GetBuffer();
		rs_bind.buffer_length = GetBufferLength();
		m_field = pMYSQL_FIELD->name;

		//printf("field %s [%p]\n", m_field.c_str(), rs_bind.buffer);
	}

	void SetField(const std::string field)
	{
		m_field = field;
	}

	const enum_field_types& get_field_type() const { return m_field_type; }
	bool* GetBool() { return &m_isnull; }

	virtual void* GetBuffer() = 0;
	virtual int GetBufferLength() = 0;
	virtual void Print() = 0;
	virtual void init_buffer(_In_ const unsigned long max_length = 0) = 0;
};

class cMySQLParserTinyInt : public cMySQLParserBase
{
private:
	BYTE m_data;

public:
	void* GetBuffer() override { return reinterpret_cast<void*>(&m_data); }
	int GetBufferLength() override { return sizeof(m_data); }
	
	void Print() override
	{
		printf("field[ %s ] %d\n", m_field.c_str(), m_data);
	}

	virtual void init_buffer(_In_ const unsigned long max_length = 0) override
	{
		cMySQLParserBase::Clear();
		m_data = 0;
	}

	cMySQLParserTinyInt(/*int max_length = 0*/)
	{
		/*cMySQLParserBase::Clear();
		m_data = 0;
		m_field_type = enum_field_types::MYSQL_TYPE_TINY;*/

		m_field_type = enum_field_types::MYSQL_TYPE_TINY;
	}

	BYTE GetField() { return m_data; }
};

class cMySQLParserLong : public cMySQLParserBase
{
private:
	int m_data;

public:
	void* GetBuffer() override  
	{ 
		int nn = 10;
		return reinterpret_cast<void*>(&m_data); 
	}
	int GetBufferLength() override  { return sizeof(m_data); }

	void Print() override
	{
		printf("field[%s] %d\n", m_field.c_str(), m_data);
	}

	virtual void init_buffer(_In_ const unsigned long max_length = 0) override
	{
		cMySQLParserBase::Clear();
		m_data = 0;
	}

	cMySQLParserLong(/*int max_length = 0*/)
	{
		/*cMySQLParserBase::Clear();
		m_data = 0;
		m_field_type = enum_field_types::MYSQL_TYPE_LONG;*/

		m_field_type = enum_field_types::MYSQL_TYPE_LONG;
	}

	int GetField() { return m_data; }
};

class cMySQLParserLongLong : public cMySQLParserBase
{
private:
	__int64 m_data;

public:
	void* GetBuffer() override { return reinterpret_cast<void*>(&m_data); }
	int GetBufferLength() override { return sizeof(m_data); }

	void Print() override
	{
		printf("field[%s] %I64d\n", m_field.c_str(), m_data);
	}

	virtual void init_buffer(_In_ const unsigned long max_length = 0) override
	{
		cMySQLParserBase::Clear();
		m_data = 0;
	}

	cMySQLParserLongLong(/*int max_length = 0*/)
	{
		/*cMySQLParserBase::Clear();
		m_data = 0;
		m_field_type = enum_field_types::MYSQL_TYPE_LONGLONG;*/

		m_field_type = enum_field_types::MYSQL_TYPE_LONGLONG;
	}

	__int64 GetField() { return m_data; }
};

class cMySQLParserDouble : public cMySQLParserBase
{
private:
	double m_data;

public:
	void* GetBuffer() override { return reinterpret_cast<void*>(&m_data); }
	int GetBufferLength() override { return sizeof(m_data); }

	void Print() override
	{
		printf("field[%s] %lf\n", m_field.c_str(), m_data);
	}

	virtual void init_buffer(_In_ const unsigned long max_length = 0) override
	{
		cMySQLParserBase::Clear();
		m_data = 0.0f;
	}

	cMySQLParserDouble(/*int max_length = 0*/)
	{
		Clear();
		/*cMySQLParserBase::Clear();
		m_data = 0.0f;
		m_field_type = enum_field_types::MYSQL_TYPE_DOUBLE;*/

		m_field_type = enum_field_types::MYSQL_TYPE_DOUBLE;
	}

	double GetField() { return m_data; }
};

class cMySQLParserString : public cMySQLParserBase
{
private:
	char m_data[CSDef::EDef::MAX_BUFFER_2048_LEN];
	int m_buffer_length;

public:
	void* GetBuffer() override { return reinterpret_cast<void*>(m_data); }
	int GetBufferLength() override { return m_buffer_length; }

	void Print() override
	{
		printf("field[%s] %s\n", m_field.c_str(), m_data);
	}

	virtual void init_buffer(_In_ const unsigned long max_length = 0) override
	{
		cMySQLParserBase::Clear();

		m_buffer_length = 0;
		memset(m_data, 0x00, sizeof(m_data));
		m_buffer_length = max_length + 1;
	}

	cMySQLParserString(/*int max_length = 0*/)
	{
		/*cMySQLParserBase::Clear();
		if (max_length > 0)
		{
			m_buffer_length = max_length + 1;
			m_data = new char[m_buffer_length];
			memset(m_data, 0x00, m_buffer_length);
		}
		else
		{
			m_buffer_length = 0;
			m_data = nullptr;
		}
		m_field_type = enum_field_types::MYSQL_TYPE_VAR_STRING;*/

		m_field_type = enum_field_types::MYSQL_TYPE_VAR_STRING;
	}

	~cMySQLParserString(){}

	char* GetField() { return m_data; }
};

class cMySQLParserDateTime : public cMySQLParserBase
{
private:
	MYSQL_TIME m_data;
	int m_buffer_length;

public:
	void* GetBuffer() override { return reinterpret_cast<void*>(&m_data); }
	int GetBufferLength() override { return sizeof(m_data); }

	void Print() override
	{
		printf("field[%s] %u-%u-%u %u:%u:%u\n", m_field.c_str(), 
			m_data.year, m_data.month, m_data.day, m_data.hour, m_data.minute, m_data.second);
	}

	virtual void init_buffer(_In_ const unsigned long max_length = 0) override
	{
		cMySQLParserBase::Clear();
		memset(&m_data, 0x00, sizeof(m_data));
	}

	cMySQLParserDateTime(/*int max_length = 0*/)
	{
		/*cMySQLParserBase::Clear();
		memset(&m_data, 0x00, sizeof(m_data));
		m_field_type = enum_field_types::MYSQL_TYPE_DATETIME;*/

		m_field_type = enum_field_types::MYSQL_TYPE_DATETIME;
	}

	MYSQL_TIME GetField() { return m_data; }
};

class cMySQLParserJSON : public cMySQLParserBase
{
private:
	json m_jsonData;

public:
	void* GetBuffer() override { return reinterpret_cast< void* >( &m_jsonData ); }
	int GetBufferLength() override { return sizeof( m_jsonData ); }

	void Print() override
	{
		std::cout << "field[" << m_field << "] " << m_jsonData.dump( 4 ) << std::endl;
	}

	virtual void init_buffer( _In_ const unsigned long max_length = 0 ) override
	{
		cMySQLParserBase::Clear();
		m_jsonData = json(); // JSON 객체 초기화
	}

	cMySQLParserJSON()
	{
		m_field_type = enum_field_types::MYSQL_TYPE_JSON;
	}

	~cMySQLParserJSON() {}

	json GetField() { return m_jsonData; }
};

static void ReadField(cMySQLParserBase* pMySQLParserBase, char* sql_row)
{
	if (pMySQLParserBase == nullptr)
		return;

	void* buffer = nullptr;

	switch (pMySQLParserBase->m_field_type)
	{
	case enum_field_types::MYSQL_TYPE_LONG:
		{
			//cMySQLParserLong* pMySQLParserLong = dynamic_cast<cMySQLParserLong*>(pMySQLParserBase);
			//if (pMySQLParserLong == nullptr)
			//{
			//	// add error or throw
			//	return;
			//}
			
			void* buffer = pMySQLParserBase->GetBuffer();
			int length = pMySQLParserBase->GetBufferLength();
			int value = atoi(sql_row);
			if (errno == ERANGE)
				throw "ReadField MYSQL_TYPE_LONG Overflow condition occurred.";

			memcpy(buffer, &value, length);
		}
		break;
	case enum_field_types::MYSQL_TYPE_LONGLONG:
		{
			//cMySQLParserLongLong* pMySQLParserLongLong = dynamic_cast<cMySQLParserLongLong*>(pMySQLParserBase);
			//if (pMySQLParserLongLong == nullptr)
			//{
			//	// add error or throw
			//	return;
			//}

			void* buffer = pMySQLParserBase->GetBuffer();
			int length = pMySQLParserBase->GetBufferLength();
			__int64 value = _atoi64(sql_row);
			if (errno == ERANGE)
				throw "ReadField MYSQL_TYPE_LONGLONG Overflow condition occurred.";

			memcpy(buffer, &value, length);
		}
		break;
	case enum_field_types::MYSQL_TYPE_DOUBLE:
		{
			void* buffer = pMySQLParserBase->GetBuffer();
			int length = pMySQLParserBase->GetBufferLength();
			double value = atof(sql_row);
			if (errno == ERANGE)
				throw "ReadField MYSQL_TYPE_DOUBLE Overflow condition occurred.";

			memcpy(buffer, &value, length);
		}
		break;
	case enum_field_types::MYSQL_TYPE_TINY:
		{
			void* buffer = pMySQLParserBase->GetBuffer();
			int length = pMySQLParserBase->GetBufferLength();
			BYTE value = atoi(sql_row);
			if (errno == ERANGE)
				throw "ReadField MYSQL_TYPE_TINY Overflow condition occurred.";

			memcpy(buffer, &value, length);
		}
		break;
	case enum_field_types::MYSQL_TYPE_VAR_STRING:
		{
			void* buffer = pMySQLParserBase->GetBuffer();
			int length = pMySQLParserBase->GetBufferLength();

			if ( buffer == nullptr || length <= 0 ) {
				break; // 예외 처리
			}

			if ( sql_row != nullptr ) {
				// UTF-8 인코딩을 가정하여 std::string을 이용해 복사
				std::string utf8_string( sql_row );

				// 복사할 문자열 길이 결정
				size_t copy_length = std::min( utf8_string.size() , static_cast< size_t >( length - 1 ) );

				// 버퍼에 복사
				std::memcpy( buffer , utf8_string.c_str() , copy_length );
				// NULL 종료 문자 추가
				reinterpret_cast< char* >( buffer )[ copy_length ] = '\0';
			}
			else {
				// sql_row가 nullptr일 때 빈 문자열로 초기화
				reinterpret_cast< char* >( buffer )[ 0 ] = '\0';
			}
		}
		break;
	case enum_field_types::MYSQL_TYPE_DATETIME:
		{
			// 날짜 및 시간 문자열을 파싱
			std::tm datetime = {};

			if ( sql_row == nullptr ) {
				// Set default date and time (e.g., 1970-01-01 00:00:00)
				datetime.tm_year = 70;  // Year since 1900
				datetime.tm_mon = 0;    // Month [0, 11]
				datetime.tm_mday = 1;   // Day of the month [1, 31]
				datetime.tm_hour = 0;   // Hours since midnight [0, 23]
				datetime.tm_min = 0;    // Minutes after the hour [0, 59]
				datetime.tm_sec = 0;    // Seconds after the minute [0, 59]
			}
			else {
				std::stringstream ss( sql_row );

				// std::get_time을 사용하여 날짜 및 시간을 파싱하고 std::tm 구조체에 저장
				ss >> std::get_time( &datetime , "%Y-%m-%d %H:%M:%S" );

				if ( ss.fail() ) {
					// 날짜 및 시간 문자열 파싱 실패
					throw "ReadField MYSQL_TYPE_DATETIME 날짜 및 시간 문자열 파싱 실패.";
				}
			}

			// MYSQL_TIME 구조체에 날짜 및 시간 정보 채우기
			MYSQL_TIME mysqlTime;
			mysqlTime.year = datetime.tm_year + 1900; // tm_year은 1900년부터의 년 수
			mysqlTime.month = datetime.tm_mon + 1;    // tm_mon은 0부터 시작하므로 1을 더해줌
			mysqlTime.day = datetime.tm_mday;
			mysqlTime.hour = datetime.tm_hour;
			mysqlTime.minute = datetime.tm_min;
			mysqlTime.second = datetime.tm_sec;
			mysqlTime.second_part = 0; // microsecond 부분은 0으로 설정
			mysqlTime.neg = 0;         // 양수로 설정
			mysqlTime.time_type = MYSQL_TIMESTAMP_DATETIME; // 시간 유형 설정

			// MYSQL_TIME 구조체를 버퍼에 복사
			if ( pMySQLParserBase->GetBufferLength() >= sizeof( MYSQL_TIME ) ) {
				memcpy( pMySQLParserBase->GetBuffer() , &mysqlTime , sizeof( MYSQL_TIME ) );
			}
			else {
				// 버퍼 크기가 충분하지 않음
				throw "ReadField MYSQL_TYPE_DATETIME 버퍼 크기 부족.";
			}
		}
		break;
	case enum_field_types::MYSQL_TYPE_JSON:
	{
		void* buffer = pMySQLParserBase->GetBuffer();
		int length = pMySQLParserBase->GetBufferLength();

		if ( buffer == nullptr || length <= 0 ) {
			break; // 예외 처리
		}

		if ( sql_row != nullptr ) {
			// UTF-8 인코딩을 가정하여 std::string을 이용해 복사
			std::string json_string( sql_row );

			try {
				// JSON 파싱
				json& jsonData = *reinterpret_cast< json* >( buffer );
				jsonData = json::parse( json_string );
			}
			catch ( const std::exception& e ) {
				std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
				// 예외 처리
			}
		}
		else {
			// sql_row가 nullptr일 때 빈 JSON 객체로 초기화
			json& jsonData = *reinterpret_cast< json* >( buffer );
			jsonData = json();
		}
	}
	break;


		//{
		//	void* buffer = pMySQLParserBase->GetBuffer();
		//	int length = pMySQLParserBase->GetBufferLength();

		//	// Parse the datetime string
		//	std::tm datetime = {};
		//	std::stringstream ss( sql_row );
		//	ss >> std::get_time( &datetime , "%Y-%m-%d %H:%M:%S" );

		//	if ( ss.fail() ) {
		//		// Failed to parse datetime string
		//		throw "ReadField MYSQL_TYPE_DATETIME Failed to parse datetime string.";
		//	}

		//	// Convert std::tm to time_t
		//	time_t time = mktime( &datetime );

		//	// Copy the time_t value to the buffer
		//	if ( sizeof( time_t ) <= length ) {
		//		memcpy( buffer , &time , sizeof( time_t ) );
		//	}
		//	else {
		//		// Buffer size is insufficient
		//		throw "ReadField MYSQL_TYPE_DATETIME Buffer size insufficient.";
		//	}
		//}
		//break;
	}
}