#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"

class cMessageLog
{
private:
	cClientSession* m_pClientSession;

	int m_code; // 메시지 코드 번호
	std::string m_data; // 초과금 정보 ( 날짜, 횟수, 금액 )
	std::string m_msgid; // 메시지 ID
	int m_msgtype; // 메시지 타입
	std::string m_reward; // 메시지 내용물
	std::string m_sender; // 메시지 발송 사유
	std::string m_etc; // 기타

private:
	// cMessageLog() { m_pClientSession = nullptr; }
	cMessageLog() : m_code(0), m_pClientSession(nullptr), m_data(""), m_msgid(""), m_msgtype(0), m_reward(""), m_sender(""), m_etc("") {}

	void BeforeLogs();
	void AfterLogs();
	void WriteLog();

public:
	cMessageLog( cClientSession* pClientSession , const int& code ) {
		this->m_pClientSession = pClientSession;
		m_code = code;
		BeforeLogs();
	}

	void SetData(const std::string& data1, const std::string& data2, const std::string& data3) {
        m_data = data1 + ", " + data2 + ", " + data3;
    }

	void SetData( const std::string& data1 ) {
		m_data = data1;
	}

	void SetMsgid(const std::string& msgid) {
		m_msgid = msgid;
	}

	void SetMsgtype(std::string& msgtype) {

		std::stringstream ss( msgtype );

		int num;
		ss >> num;  // 문자열을 정수로 변환

		if ( ss.fail() )
			m_msgtype = 0;
		else
			m_msgtype = num;
	}

	void SetReward(const std::string& reward) {
		m_reward = reward;
	}

	void SetSender(const std::string& sender) {
		m_sender = sender;
	}

	void SetEtc(const std::string& etc) {
		m_etc = etc;
	}

	~cMessageLog();

};