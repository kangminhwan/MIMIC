#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"

class cAdsLog
{
private:
	cClientSession* m_pClientSession;

	int m_code; // 광고 코드 번호

	std::string m_adType;
private:
	cAdsLog() { m_pClientSession = nullptr; }

	void BeforeLogs();
	void AfterLogs();
	void WriteLog();

public:
	cAdsLog( cClientSession* pClientSession , const int& code , const std::string adType ) {
		this->m_pClientSession = pClientSession;
		m_code = code;
		m_adType = adType;

		BeforeLogs();
	}
	~cAdsLog();

};