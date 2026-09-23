#pragma once
#include "../../Include/Netlib/Manager/cWebServerManager.h"

NetLib::cWebServerManager::cWebServerManager()
{
	Clear();
}

NetLib::cWebServerManager::~cWebServerManager()
{
}

void NetLib::cWebServerManager::Clear()
{
	m_Lock.Lock();

	for (int n = 0; n < m_web_servers.size(); ++n)
	{
		if (m_web_servers[n] == nullptr)
			continue;

		delete m_web_servers[n];
	}

	m_web_servers.clear();

	m_Lock.Unlock();
}

void NetLib::cWebServerManager::InitWebServers(std::vector<stWebConnectionInfo*>& web_servers)
{
	m_Lock.Lock();

	for (int n = 0; n < m_web_servers.size(); ++n)
	{
		if (m_web_servers[n] == nullptr)
			continue;

		delete m_web_servers[n];
	}

	m_web_servers.clear();

	m_web_servers = web_servers;

	m_Lock.Unlock();

	return;
}

stWebConnectionInfo* NetLib::cWebServerManager::SelectWebServer()
{
	// 웹서버중에 하나를 선택해서 알려 줍니다.
	// alive 인놈으로 선택합니다.

	std::vector<stWebConnectionInfo*> selected_list;

	for (int n = 0; n < m_web_servers.size(); ++n)
	{
		if (m_web_servers[n] == nullptr)
			continue;

		if (m_web_servers[n]->bAlive == FALSE)
			continue;

		selected_list.push_back(m_web_servers[n]);
	}

	if (selected_list.size() == 0)
		return nullptr;

	int selected_array = rand() % selected_list.size();

	return selected_list[selected_array];
}

std::vector<stWebConnectionInfo*> NetLib::cWebServerManager::GetWebServerList()
{
	return m_web_servers;
}