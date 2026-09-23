#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"

class cSocialLog
{
private:
	cClientSession* m_pClientSession;

	int m_code; // 소셜 코드 번호
	std::string m_friend_id; // 친구의 UID
	std::string m_log_data; // 친구 데이터

private:
	cSocialLog() { m_pClientSession = nullptr; }

	void BeforeLogs();
	void AfterLogs();
	void WriteLog();

public:
	cSocialLog( cClientSession* pClientSession , const int& code ) {
		this->m_pClientSession = pClientSession;
		m_code = code;
		BeforeLogs();
	}

	void SetFriendId(const std::string& friend_id) {
		m_friend_id = friend_id;
	}

	void SetLogData(const std::string& log_data) {
        m_log_data = log_data;
    }

	~cSocialLog();

};