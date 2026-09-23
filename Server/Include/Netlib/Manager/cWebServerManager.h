#pragma once
#include "../Common/Netlib.h"

#include <vector>

BEGIN_NETLIB

class cWebServerManager
{
public:
	cWebServerManager();
	~cWebServerManager();

	void Clear();

	void InitWebServers(std::vector<stWebConnectionInfo*>& web_servers);
	stWebConnectionInfo* SelectWebServer();
	std::vector<stWebConnectionInfo*> GetWebServerList();

private:
	std::vector<stWebConnectionInfo*> m_web_servers;
	NetLib::cCriticalSection m_Lock;
};

END_NETLIB