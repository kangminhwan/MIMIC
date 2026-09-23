#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"

// 소멸자 로그처리를 위한 호출을 한다.
class cMoneyLogInstance
{
private:
	cClientSession* m_pClientSession;

	Server::AssetLedgerSource m_money_event_type;

	uint64 m_chip_before;
	uint64 m_chip_after;

	uint64 m_coin_before;
	uint64 m_coin_after;

	uint64 m_gem_before;
	uint64 m_gem_after;

	uint64 m_paid_gem_before;
	uint64 m_paid_gem_after;

	void BeforeLogs();
	void AfterLogs();
	void WriteMoneyLog();

private:
	cMoneyLogInstance() { m_pClientSession = nullptr; }

public:
	cMoneyLogInstance( cClientSession* pClientSession , const Server::AssetLedgerSource money_event_type ) {
		this->m_pClientSession = pClientSession;
		m_money_event_type = money_event_type;

		BeforeLogs();
	}
	~cMoneyLogInstance();
};