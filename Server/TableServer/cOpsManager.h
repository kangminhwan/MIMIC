#pragma once
#include "TableServerHeader.h"
#include "TimeUtils.h"

#include <iostream>
#include <map>

class cOpsManager
{
private:
	std::map<std::string , Server::OpsPlayerKickReq> m_ops_operation;

public:

	void PushOperation( const std::string& ops_code, const Server::OpsPlayerKickReq& ops_request ) {

		auto iter = m_ops_operation.find( ops_code );
		if ( iter == m_ops_operation.end() )
			m_ops_operation.insert( std::make_pair( ops_code , ops_request ));
	}

	void RemoveOperation( const std::string& ops_code ) {

		auto iter = m_ops_operation.find( ops_code );
		if ( iter != m_ops_operation.end() )
			m_ops_operation.erase( iter );
	}

	Server::OpsPlayerKickReq GetOperation( const std::string& ops_code ) {

		auto iter = m_ops_operation.find( ops_code );
		if ( iter != m_ops_operation.end() )
			return iter->second;
		else
			return Server::OpsPlayerKickReq::default_instance();;
	}
};