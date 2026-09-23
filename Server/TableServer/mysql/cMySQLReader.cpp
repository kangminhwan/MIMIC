#include "cMySQLReader.h"
#include "cMySQLParserElement.h"

#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include <json/json.h>

#include <utility>

cMySQLReader::cMySQLReader()
{
	//Clear();
	m_pool_tiny_int.Create(0, 1000);
	m_pool_long.Create(0, 1000);
	m_pool_longlong.Create(0, 1000);
	m_pool_double.Create(0, 1000);
	m_pool_string.Create(0, 1000);
	m_pool_date_time.Create(0, 1000);
	m_pool_json.Create(0, 1000);
}

cMySQLReader::~cMySQLReader()
{
	free_all_mysql_parser_base();
}

cMySQLParserBase* cMySQLReader::pop_mysql_parser_base(enum_field_types type)
{
	switch (type)
	{
	case enum_field_types::MYSQL_TYPE_TINY:
		return m_pool_tiny_int.Pop();
	case enum_field_types::MYSQL_TYPE_LONG:
		return m_pool_long.Pop();
	case enum_field_types::MYSQL_TYPE_LONGLONG:
		return m_pool_longlong.Pop();
	case enum_field_types::MYSQL_TYPE_DOUBLE:
		return m_pool_double.Pop();
	case enum_field_types::MYSQL_TYPE_VAR_STRING:
		return m_pool_string.Pop();
	case enum_field_types::MYSQL_TYPE_DATETIME:
		return m_pool_date_time.Pop();
	case enum_field_types::MYSQL_TYPE_JSON:
		return m_pool_json.Pop();
	}

	return nullptr;
}

void cMySQLReader::free_all_mysql_parser_base()
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.begin();
	std::map<std::string, cMySQLParserBase*>::iterator iter_end = m_row.end();
	for (; iter != iter_end; ++iter)
	{
		switch ((*iter).second->get_field_type())
		{
		case enum_field_types::MYSQL_TYPE_TINY:
			m_pool_tiny_int.Push(reinterpret_cast<cMySQLParserTinyInt*>((*iter).second));
			break;
		case enum_field_types::MYSQL_TYPE_LONG:
			m_pool_long.Push(reinterpret_cast<cMySQLParserLong*>((*iter).second));
			break;
		case enum_field_types::MYSQL_TYPE_LONGLONG:
			m_pool_longlong.Push(reinterpret_cast<cMySQLParserLongLong*>((*iter).second));
			break;
		case enum_field_types::MYSQL_TYPE_DOUBLE:
			m_pool_double.Push(reinterpret_cast<cMySQLParserDouble*>((*iter).second));
			break;
		case enum_field_types::MYSQL_TYPE_VAR_STRING:
			m_pool_string.Push(reinterpret_cast<cMySQLParserString*>((*iter).second));
			break;
		case enum_field_types::MYSQL_TYPE_DATETIME:
			m_pool_date_time.Push(reinterpret_cast<cMySQLParserDateTime*>((*iter).second));
			break;
		case enum_field_types::MYSQL_TYPE_JSON:
			m_pool_json.Push(reinterpret_cast<cMySQLParserJSON*>((*iter).second));
			break;
		default:
			{
				//	심각한 문제 enum 값은 외부에서 변경 할 수 없게 해났습니다.
				assert(false && "cMySQLReader::free_all_mysql_parser_base Failed. default enum");
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cMySQLReader::free_all_mysql_parser_base Failed. default enum [ %d ]", (*iter).second->get_field_type());
			}
			break;
		}
	}

	m_row.clear();
}

cMySQLParserBase* cMySQLReader::PushField(enum_field_types type, MYSQL_FIELD field, unsigned long max_length)
{
	cMySQLParserBase* pMySQLParserBase = pop_mysql_parser_base(type);
	if (pMySQLParserBase == nullptr)
		return nullptr;

	//if ( strcmp( field.name , "nickname" ) == 0 )
	//	int n = 0;

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field.name);
	if (iter != m_row.end())
	{
		return nullptr;
	}

	pMySQLParserBase->init_buffer( field.length );

	m_row.insert(std::make_pair(field.name, pMySQLParserBase));

	return pMySQLParserBase;
}

bool cMySQLReader::isLong(std::string field_name)
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	if (iter == m_row.end())
		return false;

	if (iter->second->m_field_type != enum_field_types::MYSQL_TYPE_LONG)
		return false;

	return true;
}

bool cMySQLReader::isLongLong(std::string field_name)
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	if (iter == m_row.end())
		return false;

	if (iter->second->m_field_type != enum_field_types::MYSQL_TYPE_LONGLONG)
		return false;

	return true;
}

bool cMySQLReader::IsTinyInt(std::string field_name)
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	if (iter == m_row.end())
		return false;

	if (iter->second->m_field_type != enum_field_types::MYSQL_TYPE_TINY)
		return false;

	return true;
}

bool cMySQLReader::isDouble(std::string field_name)
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	if (iter == m_row.end())
		return false;

	if (iter->second->m_field_type != enum_field_types::MYSQL_TYPE_DOUBLE)
		return false;

	return true;
}

bool cMySQLReader::isString(std::string field_name)
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	if (iter == m_row.end())
		return false;

	if (iter->second->m_field_type != enum_field_types::MYSQL_TYPE_VAR_STRING)
		return false;

	return true;
}

bool cMySQLReader::isDateTime(std::string field_name)
{
	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	if (iter == m_row.end())
		return false;

	if (iter->second->m_field_type != enum_field_types::MYSQL_TYPE_DATETIME)
		return false;

	return true;
}


bool cMySQLReader::isJson( std::string field_name )
{
	std::map<std::string , cMySQLParserBase*>::iterator iter = m_row.find( field_name );
	if ( iter == m_row.end() )
		return false;

	if ( iter->second->m_field_type != enum_field_types::MYSQL_TYPE_JSON )
		return false;

	return true;
}

int  cMySQLReader::GetLong(std::string field_name)
{
	if(isLong(field_name) == false)
		throw std::exception( "Check field is Long" );

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if(pMySQLParserBase == nullptr)
		throw std::exception( "Check field data is nullptr" );

	//	TODO 잘만 하면 빠른 펑셔널을 만들지도..
	/*void* pppp = reinterpret_cast<void*>((*(size_t**)pMySQLParserBase)[0]);

	void* (*pf)() = reinterpret_cast<void* (*)()>(pppp);

	void* ppppp =  pf();

	bool* fddf = pMySQLParserBase->GetBool();*/

	// casting
	return util::down_casting_func<cMySQLParserLong>(pMySQLParserBase)->GetField();
}

__int64  cMySQLReader::GetLongLong(std::string field_name)
{
	if (isLongLong(field_name) == false)
		throw std::exception( "Check field is LongLong" );

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if (pMySQLParserBase == nullptr)
		throw std::exception( "Check field data is nullptr" );

	// casting
	return util::down_casting_func<cMySQLParserLongLong>(pMySQLParserBase)->GetField();
}

BYTE cMySQLReader::GetTinyInt(std::string field_name)
{
	if (IsTinyInt(field_name) == false)
		throw std::exception( "Check field is TinyInt" );

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if (pMySQLParserBase == nullptr)
		throw std::exception( "Check field data is nullptr" );

	// casting
	return util::down_casting_func<cMySQLParserTinyInt>(pMySQLParserBase)->GetField();
}

double cMySQLReader::GetDouble(std::string field_name)
{
	if (isDouble(field_name) == false)
		throw std::exception( "Check field is Double" );

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if (pMySQLParserBase == nullptr)
		throw std::exception( "Check field data is nullptr" );

	// casting
	return util::down_casting_func<cMySQLParserDouble>(pMySQLParserBase)->GetField();
}

char* cMySQLReader::GetString(std::string field_name)
{
	if (isString(field_name) == false)
		throw std::exception( "Check field is String" );

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if (pMySQLParserBase == nullptr)
		throw std::exception( "Check field data is nullptr" );

	// casting
	return util::down_casting_func<cMySQLParserString>(pMySQLParserBase)->GetField();
}

MYSQL_TIME cMySQLReader::GetDateTime(std::string field_name)
{
	if (isDateTime(field_name) == false)
		throw std::exception( "Check field is MYSQL_TIME" );

	std::map<std::string, cMySQLParserBase*>::iterator iter = m_row.find(field_name);
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if (pMySQLParserBase == nullptr)
		throw std::exception( "Check field data is nullptr" );

	// casting
	return util::down_casting_func<cMySQLParserDateTime>(pMySQLParserBase)->GetField();
}

json cMySQLReader::GetJson( std::string field_name )
{
	if ( isJson( field_name ) == false )
	{
		throw std::exception( "Check field is JSON" );
	}

	std::map<std::string , cMySQLParserBase*>::iterator iter = m_row.find( field_name );
	cMySQLParserBase* pMySQLParserBase = iter->second;
	if ( pMySQLParserBase == nullptr )
		throw std::exception( "Check field data is nullptr" );

	// casting
	return util::down_casting_func<cMySQLParserJSON>( pMySQLParserBase )->GetField();
}

