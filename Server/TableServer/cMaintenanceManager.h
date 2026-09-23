#pragma once
#include "TableServerHeader.h"
#include "TimeUtils.h"

#include <iostream>
#include <map>
#include <set>

#include <google/protobuf/util/json_util.h>


class cMaintenanceManager
{
private:
	std::map<int64 , Server::MaintenanceMessage> m_maintenance_messages;
	std::map<int64 , Server::MaintenanceMessage> m_maintenance_systemmessages;
	std::map<int64 , Server::MaintenanceMessage> m_maintenance_forcedmessages;
	std::map<int64 , Server::MaintenanceMessage> m_maintenance_imagemessages;

	std::vector<std::string> m_white_list;
	std::set<UINT64> m_cheat_list;
	std::vector<std::string> ParseWhiteList( const std::string& white_list );
	std::set<UINT64> m_ban_list;
	std::queue<std::string> m_lost_limit_ci;

public:
	BOOL ReadMaintenance();
	BOOL ReadMaintenance_SystemMessage();
	BOOL ReadMaintenance_ForceMessage();
	BOOL ReadMaintenance_ImageMessage();

	Server::MaintenanceMessage GetCurrentMaintenance();
	Server::MaintenanceMessage GetCurrentMaintenance_SystemMessage();
	Server::MaintenanceMessage GetCurrentMaintenance_ForcedMessage();
	Server::MaintenanceMessage GetCurrentMaintenance_ImageMessage();
	std::vector<Server::MaintenanceMessage> GetAllMaintenance_ImageMessages();

	BOOL CheckMaintenance( const General::StoreChannel& input_market , const std::string& ip_address , const std::string& version );
	BOOL CheckWhiteList( std::string ip );
	
	bool CheckMaintenanceVersion( int version_min , int version_max , const std::string& version );
	bool ischeat( UINT64 playeridx ) { return m_cheat_list.find( playeridx ) == m_cheat_list.end() ? false : true; }
	bool isban( UINT64 playeridx ) { return m_ban_list.find( playeridx ) == m_ban_list.end() ? false : true; }


	void PushbackLostLimit( std::string ci ) { m_lost_limit_ci.push( ci ); }
	std::string PopBackLostLimit() { 
		if ( !m_lost_limit_ci.empty() ) {
			std::string t = m_lost_limit_ci.front();
			m_lost_limit_ci.pop();
			return t;
		}
		else {
			return "";
		}
	}

	
};
