#include "cClientSession.h"

#include "../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"

#include "cMessageLog.h"
#include "Query.h"

#include <future>

cMessageLog::~cMessageLog()
{
	AfterLogs();

	// 메시지 로그 저장
	WriteLog();
}

void cMessageLog::BeforeLogs()
{
	if ( m_pClientSession == nullptr ) return;


}

void cMessageLog::AfterLogs()
{
	if ( m_pClientSession == nullptr ) return;


}

void cMessageLog::WriteLog()
{
	if ( m_pClientSession == nullptr ) return;

	const General::ParticipantProfile&  player = m_pClientSession->GetPlayer();

	auto context = m_pClientSession->GetContext();
	std::string ipaddr = m_pClientSession->GetIp(); //context != nullptr ? ::ConvertIP( context->GetIP() ) : "";

	switch ( m_code )
	{
	case 40101: // 메시지 삭제 
	{
		std::future<BOOL> insertMessageLogResult = QueryManager::InsertMessageLog(
			m_code , 								// int code ,                  		// 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,        // UID
			"" , 									// const std::string & cmd ,        // 시퀀스
			ipaddr , 								// IP 정보
			// "" , 								// const std::string & jointime ,   // 가입 시간
			m_data , 								// const std::string & data ,       // 초과금 정보
			m_msgid , 								// const std::string & msgid ,      // 메시지 ID
			m_msgtype , 							// const std::string & msgtype ,    // 메시지 타입
			m_reward , 								// const std::string & reward ,     // 메시지 내용물(보상 내용)
			m_sender , 								// const std::string & sender ,     // 메시지 발송 사유
			m_pClientSession->GetGameVersion() , 	// const std::string & ver ,        // 접속한 게임 버전
			m_etc ); 								// const std::string & etc);        // 기타

		insertMessageLogResult.wait();
	}
	break;
	case 40301: // 메시지 발송 
	{
		std::future<BOOL> insertMessageLogResult = QueryManager::InsertMessageLog(
			m_code , 								// int code ,                  		// 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,        // UID
			"" , 									// const std::string & cmd ,        // 시퀀스
			ipaddr , 								// IP 정보
			// "" , 								// const std::string & jointime ,   // 가입 시간
			m_data , 								// const std::string & data ,       // 초과금 정보
			m_msgid , 								// const std::string & msgid ,      // 메시지 ID
			m_msgtype , 							// const std::string & msgtype ,    // 메시지 타입
			m_reward , 								// const std::string & reward ,     // 메시지 내용물(보상 내용)
			m_sender , 								// const std::string & sender ,     // 메시지 발송 사유
			m_pClientSession->GetGameVersion() , 	// const std::string & ver ,        // 접속한 게임 버전
			m_etc ); 								// const std::string & etc);        // 기타

		insertMessageLogResult.wait();
	}
	break;
	case 40201: // 초과금 획득코인 메시지 발송 로그
	{
		std::future<BOOL> insertMessageLogResult = QueryManager::InsertMessageLog(
			m_code , 								// int code ,                  		// 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,        // UID
			"" , 									// const std::string & cmd ,        // 시퀀스
			ipaddr , 								// IP 정보
			// "" , 									// const std::string & jointime ,   // 가입 시간
			m_data , 								// const std::string & data ,       // 초과금 정보
			m_msgid , 								// const std::string & msgid ,      // 메시지 ID
			m_msgtype , 									// const std::string & msgtype ,    // 메시지 타입
			m_reward , 								// const std::string & reward ,     // 메시지 내용물(보상 내용)
			"" , 									// const std::string & sender ,     // 메시지 발송 사유
			"" , 									// const std::string & ver ,        // 접속한 게임 버전
			"" ); 									// const std::string & etc);        // 기타

		insertMessageLogResult.wait();
	}
	break;
	}
}