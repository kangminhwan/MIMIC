#include "cMaintenanceManager.h"

#include "../Include/Netlib/Common/cSingleton.h"

#include "Query.h"
#include "cGameVersionChecker.h"

BOOL cMaintenanceManager::ReadMaintenance()
{
	Server::MaintenanceMessage message;
	auto result2 = QueryManager::GetCheatListAsync( m_cheat_list );
	result2.wait();

	auto result3 = QueryManager::GetBanListAsync( m_ban_list );
	result3.wait();

	auto result = QueryManager::GetMaintenanceMessageAsync( message );
	result.wait();

	// 메시지를 읽었다.
	// 현재 가지고 있는 메시지 있는지 확인하고 아닌 경우 삭제한다.
	if ( result.get() ) {

		auto iter = m_maintenance_messages.find( message.maintenance_idx() );
		if ( iter != m_maintenance_messages.end() )
			return FALSE;

		m_maintenance_messages.clear();
		m_maintenance_messages.insert( std::make_pair( message.maintenance_idx() , message ) );

		// white 리스트 처리
		m_white_list.clear();
		m_white_list = ParseWhiteList( message.white_list() );

		return TRUE;
	}
	else {

		// 메시지가 없는 경우 기존 메시지 삭제
		m_maintenance_messages.clear();
	}

	return FALSE;
}

BOOL cMaintenanceManager::ReadMaintenance_SystemMessage()
{
	Server::MaintenanceMessage message;

	auto result = QueryManager::GetMaintenanceSystemMessageAsync( message );
	result.wait();

	// 메시지를 읽었다.
	// 현재 가지고 있는 메시지 있는지 확인하고 아닌 경우 삭제한다.
	if ( result.get() ) {

		//auto iter = m_maintenance_systemmessages.find( message.maintenance_idx() );
		//if ( iter != m_maintenance_systemmessages.end() )
		//	return TRUE;

		//m_maintenance_systemmessages.clear();
		//m_maintenance_systemmessages.insert( std::make_pair( message.maintenance_idx() , message ) );

		auto iter = m_maintenance_systemmessages.find( message.maintenance_idx() );
		if ( iter != m_maintenance_systemmessages.end() )
			return FALSE;

		m_maintenance_systemmessages.clear();
		m_maintenance_systemmessages.insert( std::make_pair( message.maintenance_idx() , message ) );

		return TRUE;
	}
	else {

		// 메시지가 없는 경우 기존 메시지 삭제
		m_maintenance_systemmessages.clear();
	}

	return FALSE;
}

BOOL cMaintenanceManager::ReadMaintenance_ForceMessage()
{
	Server::MaintenanceMessage message;

	auto result = QueryManager::GetMaintenanceForcedMessageAsync( message );
	result.wait();

	// 메시지를 읽었다.
	// 현재 가지고 있는 메시지 있는지 확인하고 아닌 경우 삭제한다.
	if ( result.get() ) {

		auto iter = m_maintenance_forcedmessages.find( message.maintenance_idx() );
		if ( iter != m_maintenance_forcedmessages.end() )
			return FALSE;

		m_maintenance_forcedmessages.clear();
		m_maintenance_forcedmessages.insert( std::make_pair( message.maintenance_idx() , message ) );

		return TRUE;
	}
	else {

		// 메시지가 없는 경우 기존 메시지 삭제
		m_maintenance_forcedmessages.clear();
	}

	return FALSE;
}

Server::MaintenanceMessage cMaintenanceManager::GetCurrentMaintenance()
{
	auto iter = m_maintenance_messages.begin();
	if ( iter == m_maintenance_messages.end() )
		return Server::MaintenanceMessage::default_instance();
	return iter->second;
}

Server::MaintenanceMessage cMaintenanceManager::GetCurrentMaintenance_SystemMessage()
{
	auto iter = m_maintenance_systemmessages.begin();
	if ( iter == m_maintenance_systemmessages.end() )
		return Server::MaintenanceMessage::default_instance();
	return iter->second;
}

Server::MaintenanceMessage cMaintenanceManager::GetCurrentMaintenance_ForcedMessage()
{
	auto iter = m_maintenance_forcedmessages.begin();
	if ( iter == m_maintenance_forcedmessages.end() )
		return Server::MaintenanceMessage::default_instance();
	return iter->second;
}

BOOL cMaintenanceManager::CheckWhiteList( std::string ip )
{
	for ( const auto& white_ip : m_white_list ) {

		if ( white_ip._Equal( ip ) )
			return true;
	}
	return false;
}

bool cMaintenanceManager::CheckMaintenanceVersion( int version_min , int version_max , const std::string& version )
{
	int i_version = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( version );
	if ( i_version < version_min ||
		 i_version > version_max )
		return FALSE;

	return TRUE;
}

BOOL cMaintenanceManager::CheckMaintenance( const General::StoreChannel& input_market , const std::string& ip_address , const std::string& version )
{
	auto iter = m_maintenance_messages.begin();
	if ( iter == m_maintenance_messages.end() )
		return FALSE;

	// 화이트 리스트는 접속을 허용해야 한다.
	for ( const auto& white_ip : m_white_list ) {

		if ( white_ip._Equal( ip_address ) )
			return FALSE;
	}
	const Server::MaintenanceMessage& message = GetCurrentMaintenance();
	
	// 점검 시간에 해당하는지 확인
	if ( FALSE == TimeUtils::isCurrentTimeWithin( message.start_date() , message.end_date() ) )
		return FALSE;

	int i_version = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( version );
	int i_versionmin = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( message.version_min() );
	int i_versionmax = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( message.version_max() );
	
	if ( !CheckMaintenanceVersion( i_versionmin , i_versionmax , version ) )
		return FALSE;

	std::vector<General::StoreChannel> markets;

	if ( message.play_store() )
		markets.push_back( General::StoreChannel::StoreChannel_GooglePlay );

	if ( message.app_store() )
		markets.push_back( General::StoreChannel::StoreChannel_AppleAppStore );

	if ( message.one_store() )
		markets.push_back( General::StoreChannel::StoreChannel_OneStore );

	if ( message.pc() )
		markets.push_back( General::StoreChannel::StoreChannel_PC );

	// 해당 마켓이 없으면 처리 하지 않는다.
	if ( markets.size() == 0 )
		return FALSE;

	// 해당하는 마켓의 클라이언트만 끊어낸다.
	for ( const auto& market : markets ) {

		if ( input_market == market ) {
			return TRUE;
		}
	}

	return FALSE;
}

std::vector<std::string> cMaintenanceManager::ParseWhiteList( const std::string& white_list )
{
	std::vector<std::string> ip_list;
	std::stringstream ss( white_list );
	std::string ip;

	while ( std::getline( ss , ip , '/' ) ) {
		ip_list.push_back( ip );
	}

	return ip_list;
}

BOOL cMaintenanceManager::ReadMaintenance_ImageMessage()
{
	std::vector<Server::MaintenanceMessage> messages;

	auto result = QueryManager::GetMaintenanceImageMessagesAsync( messages );
	result.wait();

	// Multiple image messages
	if ( result.get() && !messages.empty() ) {

		m_maintenance_imagemessages.clear();

		for ( const auto& message : messages ) {
			m_maintenance_imagemessages.insert( std::make_pair( message.maintenance_idx() , message ) );
		}

		return TRUE;
	}
	else {

		// No messages, clear existing ones
		m_maintenance_imagemessages.clear();
	}

	return FALSE;
}

Server::MaintenanceMessage cMaintenanceManager::GetCurrentMaintenance_ImageMessage()
{
	auto iter = m_maintenance_imagemessages.begin();
	if ( iter == m_maintenance_imagemessages.end() )
		return Server::MaintenanceMessage::default_instance();
	return iter->second;
}

std::vector<Server::MaintenanceMessage> cMaintenanceManager::GetAllMaintenance_ImageMessages()
{
	std::vector<Server::MaintenanceMessage> messages;
	for ( const auto& pair : m_maintenance_imagemessages ) {
		messages.push_back( pair.second );
	}
	return messages;
}